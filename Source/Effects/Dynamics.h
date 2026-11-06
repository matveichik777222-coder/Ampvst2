#pragma once
#include "Effect.h"

namespace gr
{
class NoiseGate : public Effect
{
public:
    void reset() override { env = 0; g = 1; open = false; hold = 0; }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float thr = dbToGain (mapLin (p[0], -80.0f, -25.0f));
        const float aC = timeCoef (mapExp (p[1], 0.5f, 20.0f), sr);
        const float rC = timeCoef (mapExp (p[2], 20.0f, 600.0f), sr);
        const float range = dbToGain (mapLin (p[3], -12.0f, -90.0f));
        const float envA = timeCoef (0.5f, sr), envR = timeCoef (40.0f, sr);
        const int holdSamples = (int) (0.03 * sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
        {
            float in = 0.0f;
            for (int c = 0; c < nc; ++c) in = std::max (in, std::abs (d[c][i]));
            env += (in > env ? envA : envR) * (in - env);

            if (env > thr) { open = true; hold = holdSamples; }
            else if (open && env < thr * 0.7f) { if (hold > 0) --hold; else open = false; }

            const float target = open ? 1.0f : range;
            g += (target > g ? aC : rC) * (target - g);
            for (int c = 0; c < nc; ++c) d[c][i] *= g;
        }
    }
private:
    float env = 0, g = 1; bool open = false; int hold = 0;
};

class Compressor : public Effect
{
public:
    void reset() override { grDb = 0; }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float thr = mapLin (p[0], -8.0f, -50.0f);
        const float ratio = mapExp (p[1], 2.0f, 20.0f);
        const float slope = 1.0f - 1.0f / ratio;
        const float aC = timeCoef (mapExp (p[2], 1.0f, 60.0f), sr);
        const float rC = timeCoef (200.0f, sr);
        const float makeup = dbToGain (mapLin (p[3], -6.0f, 18.0f));
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();

        for (int i = 0; i < n; ++i)
        {
            float in = 0.0f;
            for (int c = 0; c < nc; ++c) in = std::max (in, std::abs (d[c][i]));
            const float over = gainToDb (in) - thr;
            float target; // desired gain change in dB (<= 0), 6 dB soft knee
            if (over <= -3.0f) target = 0.0f;
            else if (over >= 3.0f) target = -over * slope;
            else { const float x = over + 3.0f; target = -slope * x * x / 12.0f; }
            grDb += (target < grDb ? aC : rC) * (target - grDb);
            const float g = dbToGain (grDb) * makeup;
            for (int c = 0; c < nc; ++c) d[c][i] *= g;
        }
    }
private:
    float grDb = 0;
};
} // namespace gr
