#pragma once
#include "../DSP/Common.h"
#include "../DSP/Biquad.h"

namespace gr
{
// Every pedal/effect gets 4 normalised (0..1) parameters p[0..3]; meaning is given by FxInfo labels.
class Effect
{
public:
    virtual ~Effect() = default;
    void prepare (double sampleRate, int maxBlock, int numChannels)
    {
        sr = sampleRate; maxBlockSize = maxBlock; nch = numChannels;
        prepareImpl();
        reset();
    }
    virtual void reset() {}
    virtual void process (juce::AudioBuffer<float>& buffer, const float* p) = 0;

protected:
    virtual void prepareImpl() {}
    double sr = 44100.0;
    int maxBlockSize = 512, nch = 1;
};
} // namespace gr
