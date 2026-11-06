#pragma once
#include "DSP/Common.h"
#include "Core/Params.h"
#include "Core/EffectSlot.h"
#include "Amps/AmpModel.h"
#include "Cabinet/CabinetEngine.h"
#include "Presets/PresetManager.h"

class AmpForgeProcessor : public juce::AudioProcessor,
                          private juce::AudioProcessorValueTreeState::Listener,
                          private juce::AsyncUpdater
{
public:
    AmpForgeProcessor();
    ~AmpForgeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AmpForge"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Message-thread API used by the editor
    void loadUserIR (const juce::File& file);
    juce::String getUserIRName() const { return userIRPath.isEmpty() ? juce::String() : juce::File (userIRPath).getFileName(); }
    void swapSlots (bool pre, int a, int b);

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f };

private:
    struct SlotPtrs { std::atomic<float>* type = nullptr; std::atomic<float>* on = nullptr; std::atomic<float>* p[4] {}; };
    SlotPtrs fetchSlot (bool pre, int i);
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    void rebuildCabinet();
    void processChunk (juce::AudioBuffer<float>& buffer);

    gr::EffectSlot preSlots[gr::kNumPreSlots], postSlots[gr::kNumPostSlots];
    SlotPtrs preP[gr::kNumPreSlots], postP[gr::kNumPostSlots];
    gr::AmpModel amp;
    gr::CabinetEngine cab;

    std::atomic<float> *inGainP = nullptr, *outGainP = nullptr, *ampOnP = nullptr, *ampModelP = nullptr, *pickupP = nullptr,
                       *osP = nullptr, *gainP = nullptr, *volP = nullptr, *bassP = nullptr, *midP = nullptr, *trebP = nullptr,
                       *presP = nullptr, *masterP = nullptr, *cabOnP = nullptr, *cabTypeP = nullptr, *cabSpkP = nullptr,
                       *cabMicP = nullptr, *cabPosP = nullptr, *cabDistP = nullptr, *cabHpP = nullptr, *cabLpP = nullptr,
                       *cabLevelP = nullptr, *cabUserP = nullptr;

    gr::SV inGain, outGain;
    double currentSampleRate = 44100.0;
    int maxBlock = 512;
    juce::String userIRPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmpForgeProcessor)
};
