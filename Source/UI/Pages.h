#pragma once
#include "PedalSlotComponent.h"

namespace ui
{
// Row of pedal/effect slots (Pedals tab = pre-amp, Effects tab = post-cabinet)
class SlotsPage : public juce::Component
{
public:
    SlotsPage (AmpForgeProcessor& p, bool pre, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            slots.push_back (std::make_unique<PedalSlotComponent> (p, pre, i, count));
            addAndMakeVisible (*slots.back());
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        const int w = r.getWidth() / (int) slots.size();
        for (auto& s : slots) s->setBounds (r.removeFromLeft (w));
    }
private:
    std::vector<std::unique_ptr<PedalSlotComponent>> slots;
};

class AmpPage : public juce::Component
{
public:
    explicit AmpPage (AmpForgeProcessor& p) : proc (p)
    {
        auto& s = proc.apvts;
        fillCombo (model, s, "amp_model"); fillCombo (pickup, s, "pickup"); fillCombo (os, s, "oversample");
        for (auto* c : { &model, &pickup, &os }) addAndMakeVisible (*c);
        modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "amp_model", model);
        pickupAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "pickup", pickup);
        osAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "oversample", os);
        onBtn.setButtonText ("AMP ON"); addAndMakeVisible (onBtn);
        onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "amp_on", onBtn);

        const char* ids[7] = { "amp_gain", "amp_volume", "amp_bass", "amp_mid", "amp_treble", "amp_presence", "amp_master" };
        const char* names[7] = { "GAIN", "VOLUME", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "MASTER" };
        for (int i = 0; i < 7; ++i)
        {
            knobs[i] = std::make_unique<Knob> (s, ids[i], names[i], Knob::Fmt::Raw);
            addAndMakeVisible (*knobs[i]);
        }
        for (auto* l : { &lModel, &lPickup, &lOs })
        { l->setFont (juce::Font (juce::FontOptions (12.0f))); l->setColour (juce::Label::textColourId, juce::Colour (0xff8a8a8a)); addAndMakeVisible (*l); }
        lModel.setText ("AMPLIFIER", juce::dontSendNotification);
        lPickup.setText ("PICKUP", juce::dontSendNotification);
        lOs.setText ("OVERSAMPLING", juce::dontSendNotification);
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (8.0f);
        g.setColour (panel()); g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff33343a)); g.drawRoundedRectangle (r, 10.0f, 1.5f);
        g.setColour (accent()); g.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
        g.drawText (model.getText().toUpperCase(), getLocalBounds().reduced (30, 24).removeFromTop (44), juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (30, 24);
        r.removeFromTop (54);
        auto row = r.removeFromTop (46);
        auto col = [&] (juce::Label& l, juce::Component& c, int w)
        { auto a = row.removeFromLeft (w); l.setBounds (a.removeFromTop (16)); c.setBounds (a.reduced (0, 2)); row.removeFromLeft (14); };
        col (lModel, model, 240); col (lPickup, pickup, 150); col (lOs, os, 110);
        onBtn.setBounds (row.removeFromLeft (110).withTrimmedTop (16));
        r.removeFromTop (30);
        const int w = r.getWidth() / 7;
        for (int i = 0; i < 7; ++i) knobs[i]->setBounds (r.getX() + i * w, r.getY(), w, 170);
    }
    void parentHierarchyChanged() override { model.onChange = [this] { repaint(); }; }
private:
    AmpForgeProcessor& proc;
    juce::ComboBox model, pickup, os;
    juce::Label lModel, lPickup, lOs;
    juce::ToggleButton onBtn;
    std::unique_ptr<Knob> knobs[7];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt, pickupAtt, osAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
};

class CabinetPage : public juce::Component, private juce::Timer
{
public:
    explicit CabinetPage (AmpForgeProcessor& p) : proc (p)
    {
        auto& s = proc.apvts;
        fillCombo (type, s, "cab_type"); fillCombo (speaker, s, "cab_speaker"); fillCombo (mic, s, "cab_mic");
        for (auto* c : { &type, &speaker, &mic }) addAndMakeVisible (*c);
        typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "cab_type", type);
        spkAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "cab_speaker", speaker);
        micAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "cab_mic", mic);

        onBtn.setButtonText ("CAB ON"); userBtn.setButtonText ("USE CUSTOM IR");
        addAndMakeVisible (onBtn); addAndMakeVisible (userBtn);
        onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "cab_on", onBtn);
        userAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, "cab_user", userBtn);

        loadBtn.setButtonText ("Load IR (.wav)...");
        loadBtn.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Load impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  { auto f = fc.getResult(); if (f.existsAsFile()) proc.loadUserIR (f); });
        };
        addAndMakeVisible (loadBtn);
        irName.setColour (juce::Label::textColourId, juce::Colour (0xff9a9a9a));
        addAndMakeVisible (irName);

        knobs[0] = std::make_unique<Knob> (s, "cab_micpos", "MIC POS", Knob::Fmt::Raw);
        knobs[1] = std::make_unique<Knob> (s, "cab_micdist", "DISTANCE", Knob::Fmt::Raw);
        knobs[2] = std::make_unique<Knob> (s, "cab_hp", "LOW CUT", Knob::Fmt::Hz);
        knobs[3] = std::make_unique<Knob> (s, "cab_lp", "HIGH CUT", Knob::Fmt::Hz);
        knobs[4] = std::make_unique<Knob> (s, "cab_level", "LEVEL", Knob::Fmt::Db);
        for (auto& k : knobs) addAndMakeVisible (*k);

        const char* t[3] = { "CABINET", "SPEAKER", "MICROPHONE" };
        for (int i = 0; i < 3; ++i)
        { lbl[i].setText (t[i], juce::dontSendNotification); lbl[i].setColour (juce::Label::textColourId, juce::Colour (0xff8a8a8a)); addAndMakeVisible (lbl[i]); }
        startTimerHz (4);
    }
    ~CabinetPage() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (8.0f);
        g.setColour (panel()); g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff33343a)); g.drawRoundedRectangle (r, 10.0f, 1.5f);
        g.setColour (accent()); g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
        g.drawText ("CABINET / IR", getLocalBounds().reduced (30, 24).removeFromTop (40), juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (30, 24);
        r.removeFromTop (50);
        auto row = r.removeFromTop (46);
        juce::Component* cs[3] = { &type, &speaker, &mic };
        const int ws[3] = { 140, 200, 200 };
        for (int i = 0; i < 3; ++i)
        { auto a = row.removeFromLeft (ws[i]); lbl[i].setBounds (a.removeFromTop (16)); cs[i]->setBounds (a.reduced (0, 2)); row.removeFromLeft (14); }
        onBtn.setBounds (row.removeFromLeft (100).withTrimmedTop (16));
        r.removeFromTop (16);
        auto row2 = r.removeFromTop (30);
        loadBtn.setBounds (row2.removeFromLeft (170)); row2.removeFromLeft (10);
        userBtn.setBounds (row2.removeFromLeft (160)); row2.removeFromLeft (10);
        irName.setBounds (row2);
        r.removeFromTop (24);
        const int w = r.getWidth() / 5;
        for (int i = 0; i < 5; ++i) knobs[i]->setBounds (r.getX() + i * w, r.getY(), w, 170);
    }
private:
    void timerCallback() override
    {
        const auto n = proc.getUserIRName();
        irName.setText (n.isEmpty() ? "No custom IR loaded (using built-in synthesized cabinet)" : n, juce::dontSendNotification);
    }
    AmpForgeProcessor& proc;
    juce::ComboBox type, speaker, mic;
    juce::Label lbl[3], irName;
    juce::ToggleButton onBtn, userBtn;
    juce::TextButton loadBtn;
    std::unique_ptr<Knob> knobs[5];
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt, spkAtt, micAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt, userAtt;
};
} // namespace ui
