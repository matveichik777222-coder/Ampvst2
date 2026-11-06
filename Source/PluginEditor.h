#pragma once
#include "PluginProcessor.h"
#include "UI/AmpLookAndFeel.h"
#include "UI/Pages.h"
#include "UI/LevelMeter.h"

class AmpForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit AmpForgeEditor (AmpForgeProcessor&);
    ~AmpForgeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void showPage (int i);
    void rebuildPresetBox();
    void stepPreset (int delta);
    void promptSave();

    AmpForgeProcessor& proc;
    ui::AmpLookAndFeel laf;

    juce::TextButton tabs[4];
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" };
    std::unique_ptr<ui::Knob> inKnob, outKnob;
    ui::LevelMeter inMeter, outMeter;
    ui::SlotsPage pedals, effects;
    ui::AmpPage ampPage;
    ui::CabinetPage cabPage;
    juce::Component* pages[4] {};
};
