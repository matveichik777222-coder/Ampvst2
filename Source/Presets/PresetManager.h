#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"

class PresetManager
{
public:
    struct Entry { juce::String name, category; bool factory = true; int factoryIndex = -1; juce::File file; };

    explicit PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s) { refresh(); }

    void refresh();
    const std::vector<Entry>& getEntries() const { return entries; }
    void load (int index);
    bool saveUser (const juce::String& name);
    static juce::File getUserDirectory();

private:
    void resetToDefaults();
    void setReal (const juce::String& id, float realValue);

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Entry> entries;
};
