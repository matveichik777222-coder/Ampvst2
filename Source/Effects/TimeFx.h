#pragma once
#include "Effect.h"

namespace gr
{
// Digital delay (tape = false) and tape-style echo (tape = true: wow/flutter, saturation, darker repeats)
class DelayFx : public Effect
{
public:
    explicit DelayFx (bool tapeMode) : tape (tapeMode) {}
    void prepareImpl() override
    {
        for (int c = 0; c < 2; ++c) line[c].prepare ((int) (sr * 1.4));
    }
    void reset() override
    {
        for (int c = 0; c < 2; ++c) { line[c].clear(); lp[c] = hp[c] = 0; }
        cur = -1.0f; lfo1 = lfo2 = 0;
    }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float target = mapExp (p[0], 0.03f, 1.0f) * (float) sr;
        if (cur < 0.0f) cur = target;
        const float dC = timeCoef (60.0f, sr);
        const float fb = p[1] * 0.92f;
        const float cLp = onePoleCoef (tape ? mapExp (p[2], 800.0f, 5000.0f) : mapExp (p[2], 1500.0f, 12000.0f), sr);
        const float cHp = onePoleCoef (150.0f, sr);
        const float mix = p[3];
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
        {
            cur += dC * (target - cur);
            float t = cur;
            if (tape)
            {
                lfo1 += 0.6f / (float) sr; lfo1 -= std::floor (lfo1);
                lfo2 += 7.3f / (float) sr; lfo2 -= std::floor (lfo2);
                t += (float) sr * (0.0006f * std::sin (kTwoPi * lfo1) + 0.00012f * std::sin (kTwoPi * lfo2));
            }
            for (int c = 0; c < nc; ++c)
            {
                const float x = d[c][i];
                const float y = line[c].read (t);
                lp[c] += cLp * (y - lp[c]);
                float f = lp[c];
                if (tape)
                {
                    hp[c] += cHp * (f - hp[c]);
                    f -= hp[c];
                    f = fastTanh (f * 1.2f) / 1.2f;
                }
                line[c].push (x + f * fb);
                d[c][i] = x + (tape ? f : y) * mix;
            }
        }
    }
private:
    bool tape;
    DelayBuf line[2];
    float lp[2] {}, hp[2] {}, cur = -1.0f, lfo1 = 0, lfo2 = 0;
};

// Hall / Plate built on the Freeverb network with predelay, tone and (plate) input high-pass
class ReverbFx : public Effect
{
public:
    explicit ReverbFx (bool plateMode) : plate (plateMode) {}
    void prepareImpl() override
    {
        verb.setSampleRate (sr);
        wet.setSize (2, maxBlockSize);
        for (int c = 0; c < 2; ++c) pre[c].prepare ((int) (sr * 0.15));
    }
    void reset() override
    {
        verb.reset();
        for (int c = 0; c < 2; ++c) { pre[c].clear(); hp[c] = 0; }
    }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = plate ? mapLin (p[0], 0.4f, 0.95f) : mapLin (p[0], 0.55f, 0.985f);
        rp.damping = plate ? mapLin (1.0f - p[1], 0.0f, 0.4f) : mapLin (1.0f - p[1], 0.05f, 0.9f);
        rp.wetLevel = 0.33f; rp.dryLevel = 0.0f; rp.width = 1.0f; rp.freezeMode = 0.0f;
        verb.setParameters (rp);

        const float mix = p[2];
        const float preSamples = std::max (2.0f, p[3] * 0.1f * (float) sr);
        const float cHp = onePoleCoef (250.0f, sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int c = 0; c < nc; ++c)
        {
            float* w = wet.getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                float x = d[c][i];
                if (plate) { hp[c] += cHp * (x - hp[c]); x -= hp[c]; }
                pre[c].push (x);
                w[i] = pre[c].read (preSamples);
            }
        }
        if (nc == 2) verb.processStereo (wet.getWritePointer (0), wet.getWritePointer (1), n);
        else verb.processMono (wet.getWritePointer (0), n);

        for (int c = 0; c < nc; ++c)
            for (int i = 0; i < n; ++i)
                d[c][i] += wet.getSample (c, i) * mix * 1.5f;
    }
private:
    bool plate;
    juce::Reverb verb;
    juce::AudioBuffer<float> wet;
    DelayBuf pre[2];
    float hp[2] {};
};

// Spring reverb approximation: two feedback loops containing a cascade of dispersive all-passes ("drip")
class SpringReverb : public Effect
{
public:
    void prepareImpl() override
    {
        for (int c = 0; c < 2; ++c) for (int s = 0; s < 2; ++s) line[c][s].prepare ((int) (sr * 0.06));
    }
    void reset() override
    {
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < 2; ++s)
            {
                line[c][s].clear(); lp[c][s] = 0;
                for (auto& z : ap[c][s]) z = 0;
            }
    }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float g = mapLin (p[0], 0.3f, 0.93f);
        const float cLp = onePoleCoef (mapExp (p[1], 1500.0f, 6000.0f), sr);
        const float mix = p[2];
        const float a = mapLin (p[3], 0.4f, 0.85f);
        const float tms[2] = { 29.0f, 37.0f };
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
            for (int c = 0; c < nc; ++c)
            {
                const float x = d[c][i];
                float wetSum = 0.0f;
                for (int s = 0; s < 2; ++s)
                {
                    float y = line[c][s].read (tms[s] * 0.001f * (float) sr);
                    for (int k = 0; k < kStages; ++k)       // first-order all-pass: y = -a x + z ; z = x + a y
                    {
                        const float o = -a * y + ap[c][s][k];
                        ap[c][s][k] = y + a * o;
                        y = o;
                    }
                    lp[c][s] += cLp * (y - lp[c][s]);
                    y = lp[c][s];
                    line[c][s].push (x * 0.5f + y * g);
                    wetSum += y;
                }
                d[c][i] = x + wetSum * mix * 0.8f;
            }
    }
private:
    static constexpr int kStages = 10;
    DelayBuf line[2][2];
    float lp[2][2] {};
    float ap[2][2][kStages] {};
};
} // namespace gr
