#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct FactoryPreset
{
    juce::String name, category;
    std::vector<std::pair<juce::String, float>> values; // parameter id -> real (denormalised) value
};

const std::vector<FactoryPreset>& getFactoryPresets();
