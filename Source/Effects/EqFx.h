#pragma once
#include "Effect.h"

namespace gr
{
class EqFx : public Effect
{
public:
    void reset() override { lo.reset(); mid.reset(); hi.reset(); }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        lo.set (BQ::LowShelf, sr, 100.0, 0.7, (p[0] - 0.5f) * 24.0f);
        mid.set (BQ::Peak, sr, mapExp (p[3], 300.0f, 3000.0f), 0.9, (p[1] - 0.5f) * 24.0f);
        hi.set (BQ::HighShelf, sr, 4000.0, 0.7, (p[2] - 0.5f) * 24.0f);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int c = 0; c < nc; ++c)
            for (int i = 0; i < n; ++i)
                d[c][i] = hi.process (mid.process (lo.process (d[c][i], c), c), c);
    }
private:
    Biquad lo, mid, hi;
};
} // namespace gr
