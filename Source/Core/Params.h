#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace gr
{
constexpr int kNumPreSlots = 4;
constexpr int kNumPostSlots = 3;

enum FxType
{
    FxNone = 0, FxGate, FxComp, FxOverdrive, FxDistortion, FxFuzz, FxWah, FxOctaver, FxPitch,
    FxChorus, FxPhaser, FxFlanger, FxTremolo, FxDelay, FxEcho, FxSpring, FxHall, FxPlate, FxEQ, FxCount
};

struct FxInfo { const char* name; const char* labels[4]; };
const FxInfo& fxInfo (int type);

// "pre1_type", "fx2_p3", ...
juce::String slotId (bool pre, int index, const juce::String& suffix);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace gr
