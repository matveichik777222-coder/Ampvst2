#pragma once
#include "Effect.h"

namespace gr
{
// Monophonic analog-style octaver: zero-crossing divider (-1 oct) and full-wave rectifier (+1 oct)
class Octaver : public Effect
{
public:
    void reset() override
    {
        for (auto* f : { &lpIn, &lpSub, &hpUp, &lpOut }) f->reset();
        for (int c = 0; c < 2; ++c) { env[c] = 0; flip[c] = false; armed[c] = false; }
    }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        lpIn.set (BQ::LowPass, sr, 600.0, 0.707, 0.0);
        lpSub.set (BQ::LowPass, sr, 350.0, 0.707, 0.0);
        hpUp.set (BQ::HighPass, sr, 120.0, 0.707, 0.0);
        lpOut.set (BQ::LowPass, sr, mapExp (p[3], 500.0f, 6000.0f), 0.707, 0.0);
        const float subL = p[0] * 2.0f, upL = p[1] * 1.5f, dry = p[2] * 2.0f;
        const float eA = timeCoef (5.0f, sr), eR = timeCoef (50.0f, sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int c = 0; c < nc; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = d[c][i];
                const float s = lpIn.process (x, c);
                const float a = std::abs (s);
                env[c] += (a > env[c] ? eA : eR) * (a - env[c]);
                const float h = std::max (0.005f, env[c] * 0.2f);
                if (s > h && ! armed[c]) { armed[c] = true; flip[c] = ! flip[c]; }
                else if (s < -h) armed[c] = false;
                const float sub = lpSub.process ((flip[c] ? 1.0f : -1.0f) * env[c] * 1.6f, c);
                const float up = hpUp.process (a * 2.0f, c);
                d[c][i] = x * dry + lpOut.process (sub * subL + up * upL, c);
            }
    }
private:
    Biquad lpIn, lpSub, hpUp, lpOut;
    float env[2] {}; bool flip[2] {}, armed[2] {};
};

// Two-tap crossfaded delay-line pitch shifter (+-12 semitones, fine tune)
class PitchShifter : public Effect
{
public:
    void prepareImpl() override { for (int c = 0; c < 2; ++c) buf[c].prepare ((int) (sr * 0.25)); }
    void reset() override { for (int c = 0; c < 2; ++c) buf[c].clear(); phase = 0; }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float semis = std::round (mapLin (p[0], -12.0f, 12.0f)) + (p[1] - 0.5f);
        const float ratio = std::pow (2.0f, semis / 12.0f);
        const float W = mapLin (p[3], 0.04f, 0.12f) * (float) sr;
        const float mix = p[2];
        const bool bypass = std::abs (ratio - 1.0f) < 1.0e-4f;
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
        {
            const float ph2 = phase + 0.5f - std::floor (phase + 0.5f);
            const float d1 = phase * W, d2 = ph2 * W;
            const float w1 = 0.5f - 0.5f * std::cos (kTwoPi * phase);
            const float w2 = 0.5f - 0.5f * std::cos (kTwoPi * ph2);
            for (int c = 0; c < nc; ++c)
            {
                const float x = d[c][i];
                buf[c].push (x);
                if (! bypass)
                {
                    const float wet = w1 * buf[c].read (d1) + w2 * buf[c].read (d2);
                    d[c][i] = x * (1.0f - mix) + wet * mix;
                }
            }
            phase += (1.0f - ratio) / W;
            phase -= std::floor (phase);
        }
    }
private:
    DelayBuf buf[2];
    float phase = 0;
};
} // namespace gr
