#pragma once
#include "../DSP/Common.h"
#include "../DSP/Biquad.h"

namespace gr
{
class CabinetEngine
{
public:
    struct Params { bool on = true; float hp = 70.0f, lp = 12000.0f, levelDb = 0.0f; };

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>& mono, const Params& p);

    // message thread only
    void loadBuiltIn (int type, int speaker, int mic, float micPos, float micDist);
    void loadFile (const juce::File& file);

    static std::vector<float> synthesizeIR (double sr, int type, int speaker, int mic, float micPos, float micDist);

private:
    double sampleRate = 44100.0;
    juce::dsp::Convolution conv { juce::dsp::Convolution::NonUniform { 256 } }; // zero-latency partitioned convolution
    Biquad hp, lp;
    SV level;
};
} // namespace gr
