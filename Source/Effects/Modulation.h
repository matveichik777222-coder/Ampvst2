#pragma once
#include "Effect.h"

namespace gr
{
class Chorus : public Effect
{
public:
    void prepareImpl() override { chorus.prepare ({ sr, (juce::uint32) maxBlockSize, 2 }); }
    void reset() override { chorus.reset(); }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        chorus.setRate (mapExp (p[0], 0.1f, 6.0f));
        chorus.setDepth (p[1]);
        chorus.setMix (p[2]);
        chorus.setCentreDelay (mapLin (p[3], 5.0f, 30.0f));
        chorus.setFeedback (0.0f);
        juce::dsp::AudioBlock<float> blk (b);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }
private:
    juce::dsp::Chorus<float> chorus;
};

class Flanger : public Effect
{
public:
    void prepareImpl() override { fl.prepare ({ sr, (juce::uint32) maxBlockSize, 2 }); }
    void reset() override { fl.reset(); }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        fl.setRate (mapExp (p[0], 0.05f, 5.0f));
        fl.setDepth (p[1] * 0.8f);
        fl.setFeedback ((p[2] - 0.5f) * 1.8f);
        fl.setCentreDelay (mapLin (p[3], 0.8f, 6.0f));
        fl.setMix (0.5f);
        juce::dsp::AudioBlock<float> blk (b);
        fl.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }
private:
    juce::dsp::Chorus<float> fl;
};

class Phaser : public Effect
{
public:
    void prepareImpl() override { ph.prepare ({ sr, (juce::uint32) maxBlockSize, 2 }); }
    void reset() override { ph.reset(); }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        ph.setRate (mapExp (p[0], 0.05f, 8.0f));
        ph.setDepth (p[1]);
        ph.setFeedback ((p[2] - 0.2f) * 1.1f);
        ph.setCentreFrequency (mapExp (p[3], 300.0f, 3000.0f));
        ph.setMix (0.5f);
        juce::dsp::AudioBlock<float> blk (b);
        ph.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }
private:
    juce::dsp::Phaser<float> ph;
};

class Tremolo : public Effect
{
public:
    void reset() override { phase = 0; }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        const float rate = mapExp (p[0], 0.5f, 15.0f);
        const float depth = p[1];
        const float k = mapLin (p[2], 0.2f, 8.0f);
        const float norm = 1.0f / std::tanh (k);
        const float offs = p[3] * 0.5f;
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < nc; ++c)
            {
                float ph = phase + (c == 1 ? offs : 0.0f);
                ph -= std::floor (ph);
                const float lfo = std::tanh (k * std::sin (kTwoPi * ph)) * norm;
                d[c][i] *= 1.0f - depth * (0.5f - 0.5f * lfo);
            }
            phase += rate / (float) sr;
            phase -= std::floor (phase);
        }
    }
private:
    float phase = 0;
};

// Auto-/pedal-wah: unity-peak band-pass swept by pedal position plus envelope
class Wah : public Effect
{
public:
    void prepareImpl() override { pedal.prepare (sr, 0.02); }
    void reset() override { svf.reset(); env = 0; }
    void process (juce::AudioBuffer<float>& b, const float* p) override
    {
        pedal.set (p[0]);
        const float q = mapLin (p[1], 2.0f, 9.0f);
        const float sens = p[2] * p[2] * 4.0f;
        const float level = dbToGain (mapLin (p[3], -12.0f, 12.0f));
        const float eA = timeCoef (5.0f, sr), eR = timeCoef (80.0f, sr);
        const int n = b.getNumSamples(), nc = std::min (b.getNumChannels(), 2);
        float** d = b.getArrayOfWritePointers();
        for (int i = 0; i < n; ++i)
        {
            float in = 0.0f;
            for (int c = 0; c < nc; ++c) in = std::max (in, std::abs (d[c][i]));
            env += (in > env ? eA : eR) * (in - env);
            const float pos = juce::jlimit (0.0f, 1.0f, pedal.next() + sens * env);
            svf.set (mapExp (pos, 350.0f, 2200.0f), q, sr);
            for (int c = 0; c < nc; ++c)
                d[c][i] = (svf.bandpass (d[c][i], c) * 1.5f + d[c][i] * 0.25f) * level;
        }
    }
private:
    Svf svf; float env = 0; SV pedal;
};
} // namespace gr
