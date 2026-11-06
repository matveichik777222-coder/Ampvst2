#pragma once
#include "Effect.h"

namespace gr
{
// Tube-Screamer style: mid-hump clipping stage in parallel with the full-range signal
class Overdrive : public Effect
{
public:
    void prepareImpl() override { drive.prepare (sr); level.prepare (sr); }
    void reset() override { for (int c = 0; c < 2; ++c) { clip[c].reset(); tl[c] = ls[c] = lt[c] = 0; } }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        drive.set (1.0f + 60.0f * p[0] * p[0]);
        level.set (dbToGain (mapLin (p[2], -18.0f, 6.0f)));
        const float cT = onePoleCoef (mapExp (p[1], 700.0f, 7000.0f), sr);
        const float cS = onePoleCoef (720.0f, sr);
        const float cTight = onePoleCoef (mapExp (p[3], 30.0f, 500.0f), sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int i = 0; i < n; ++i)
        {
            const float g = drive.next(), lv = level.next();
            for (int c = 0; c < nc; ++c)
            {
                tl[c] += cTight * (d[c][i] - tl[c]);
                const float xt = d[c][i] - tl[c];
                ls[c] += cS * (xt - ls[c]);
                const float y = (xt + (xt - ls[c]) * g) * 1.5f;
                const float cl = clip[c].process (y) * 0.8f;
                lt[c] += cT * (cl - lt[c]);
                d[c][i] = lt[c] * lv;
            }
        }
    }
private:
    AdaaTanh clip[2]; float tl[2] {}, ls[2] {}, lt[2] {}; SV drive, level;
};

// RAT-style distortion: high gain into a soft/hard clipper blend, then a variable low-pass
class Distortion : public Effect
{
public:
    void prepareImpl() override { gain.prepare (sr); level.prepare (sr); }
    void reset() override { for (int c = 0; c < 2; ++c) { sc[c].reset(); hc[c].reset(); hp[c] = lp[c] = 0; } }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        gain.set (mapExp (p[0], 3.0f, 600.0f));
        level.set (dbToGain (mapLin (p[2], -24.0f, 0.0f)));
        const float cLp = onePoleCoef (mapExp (1.0f - p[1], 1500.0f, 16000.0f), sr);
        const float cHp = onePoleCoef (90.0f, sr);
        const float blend = p[3];
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int i = 0; i < n; ++i)
        {
            const float g = gain.next(), lv = level.next();
            for (int c = 0; c < nc; ++c)
            {
                hp[c] += cHp * (d[c][i] - hp[c]);
                const float y = (d[c][i] - hp[c]) * g * 0.5f;
                const float z = (1.0f - blend) * sc[c].process (y) + blend * hc[c].process (y);
                lp[c] += cLp * (z - lp[c]);
                d[c][i] = lp[c] * lv;
            }
        }
    }
private:
    AdaaTanh sc[2]; AdaaClip hc[2]; float hp[2] {}, lp[2] {}; SV gain, level;
};

// Fuzz Face style: asymmetric hard clipping with adjustable bias (gating/sputter at extremes)
class Fuzz : public Effect
{
public:
    void prepareImpl() override { gain.prepare (sr); level.prepare (sr); }
    void reset() override { for (int c = 0; c < 2; ++c) { hc[c].reset(); sf[c].reset(); dc[c] = lp[c] = 0; } }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        gain.set (mapExp (p[0], 4.0f, 300.0f));
        level.set (dbToGain (mapLin (p[2], -24.0f, 0.0f)));
        const float bias = (p[3] - 0.5f) * 0.9f;
        const float cLp = onePoleCoef (mapExp (p[1], 600.0f, 8000.0f), sr);
        const float cDc = onePoleCoef (25.0f, sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int i = 0; i < n; ++i)
        {
            const float g = gain.next(), lv = level.next();
            for (int c = 0; c < nc; ++c)
            {
                float y = hc[c].process (d[c][i] * g * 0.5f + bias);
                y = sf[c].process (y * 1.5f);
                dc[c] += cDc * (y - dc[c]);
                y -= dc[c];
                lp[c] += cLp * (y - lp[c]);
                d[c][i] = lp[c] * lv;
            }
        }
    }
private:
    AdaaClip hc[2]; AdaaTanh sf[2]; float dc[2] {}, lp[2] {}; SV gain, level;
};
} // namespace gr
