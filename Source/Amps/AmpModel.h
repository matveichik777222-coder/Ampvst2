#pragma once
#include "../DSP/Common.h"
#include "../DSP/Biquad.h"

namespace gr
{
constexpr int kNumAmpModels = 7;

// Circuit-style voicing of one amplifier: preamp stages, bias, tone stack, power amp
struct AmpVoicing
{
    const char* name;
    int stages;              // cascaded triode gain stages
    float gMinDb, gMaxDb;    // total preamp gain range
    float bias;              // grid bias offset -> even harmonics / asymmetry
    float tightHz;           // input high-pass (tightness)
    float couple[3];         // coupling-cap high-pass per stage
    float millerHz;          // stage low-pass (Miller capacitance), at mid gain
    float bassHz, midHz, midQ, trebHz;
    float midOffsetDb;       // built-in mid contour of the tone stack
    float bassRange, midRange, trebRange; // +- dB at knob extremes
    float sag;               // power-supply sag amount
    float power;             // power-stage drive
    float presHz, presRange; // presence (negative-feedback) shelf
    float outLpHz;           // output transformer / speaker load roll-off
};

extern const AmpVoicing kAmpVoicings[kNumAmpModels];

class AmpModel
{
public:
    struct Params
    {
        int model = 1, pickup = 1, oversampling = 1; // pickup: 0 single coil, 1 humbucker; oversampling: 0=2x, 1=4x
        float gain = 5, volume = 5, bass = 5, mid = 5, treble = 5, presence = 5, master = 5;
    };

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>& mono, const Params& p);
    int getLatencySamples() const;

private:
    float processSample (float x) noexcept;
    void updateCoefficients (const AmpVoicing& v, const Params& p, double fs);

    static float tube (float x) noexcept
    {   // asymmetric triode curve: positive half saturates earlier (grid conduction)
        return x >= 0.0f ? fastTanh (1.25f * x) / 1.25f : fastTanh (0.9f * x) / 0.9f;
    }

    double sampleRate = 44100.0;
    juce::dsp::Oversampling<float> os2 { 1, 1, juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::Oversampling<float> os4 { 1, 2, juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true, true };
    int activeOs = 1;

    Biquad inHp, pickupFilt, low, midPk, high, pres, outHp, outLp1, outLp2;
    Biquad coupling[3];
    float miller[3] { 0, 0, 0 };
    float millerCoef = 0.5f;
    float sagEnv = 0.0f, sagAtk = 0.001f, sagRel = 0.0001f, sagAmt = 0.0f;
    float inTrim = 1.0f;
    int stages = 2;
    float bias = 0.1f;
    SV stageGain, volume, drive;
};
} // namespace gr
