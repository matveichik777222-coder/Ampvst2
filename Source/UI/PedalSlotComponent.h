#pragma once
#include "../PluginProcessor.h"
#include "Knob.h"
#include "AmpLookAndFeel.h"

namespace ui
{
class PedalSlotComponent : public juce::Component
{
public:
    PedalSlotComponent (AmpForgeProcessor& p, bool preSlot, int index, int count)
        : proc (p), isPre (preSlot), idx (index)
    {
        auto& s = proc.apvts;
        fillCombo (typeBox, s, gr::slotId (isPre, idx, "type"));
        addAndMakeVisible (typeBox);
        typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, gr::slotId (isPre, idx, "type"), typeBox);

        onBtn.setButtonText ("ON");
        addAndMakeVisible (onBtn);
        onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, gr::slotId (isPre, idx, "on"), onBtn);
        onBtn.onStateChange = [this] { repaint(); };

        for (int k = 0; k < 4; ++k)
        {
            knobs[k] = std::make_unique<Knob> (s, gr::slotId (isPre, idx, "p" + juce::String (k + 1)), "", Knob::Fmt::Ten);
            addAndMakeVisible (*knobs[k]);
        }

        leftBtn.setButtonText ("<");  rightBtn.setButtonText (">");
        leftBtn.setEnabled (idx > 0); rightBtn.setEnabled (idx < count - 1);
        leftBtn.onClick = [this] { proc.swapSlots (isPre, idx, idx - 1); };
        rightBtn.onClick = [this] { proc.swapSlots (isPre, idx, idx + 1); };
        addAndMakeVisible (leftBtn); addAndMakeVisible (rightBtn);

        typeBox.onChange = [this] { updateLabels(); };
        updateLabels();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (4.0f);
        g.setColour (panel()); g.fillRoundedRectangle (r, 8.0f);
        g.setColour (onBtn.getToggleState() && typeBox.getSelectedItemIndex() > 0 ? accent().withAlpha (0.8f) : juce::Colour (0xff33343a));
        g.drawRoundedRectangle (r, 8.0f, 1.5f);
        g.setColour (juce::Colour (0xff8a8a8a)); g.setFont (12.0f);
        g.drawText ((isPre ? "PEDAL " : "FX ") + juce::String (idx + 1), getLocalBounds().reduced (14, 10).removeFromTop (18),
                    juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 10);
        auto head = r.removeFromTop (22);
        rightBtn.setBounds (head.removeFromRight (26)); head.removeFromRight (4);
        leftBtn.setBounds (head.removeFromRight (26));
        r.removeFromTop (6);
        auto row = r.removeFromTop (28);
        onBtn.setBounds (row.removeFromRight (56));
        typeBox.setBounds (row.reduced (0, 0).withTrimmedRight (6));
        r.removeFromTop (10);
        const int kh = r.getHeight() / 2, kw = r.getWidth() / 2;
        for (int k = 0; k < 4; ++k)
            knobs[k]->setBounds (r.getX() + (k % 2) * kw, r.getY() + (k / 2) * kh, kw, kh);
    }

private:
    void updateLabels()
    {
        const auto& info = gr::fxInfo (typeBox.getSelectedItemIndex());
        for (int k = 0; k < 4; ++k) knobs[k]->setLabel (info.labels[k]);
        repaint();
    }

    AmpForgeProcessor& proc;
    bool isPre; int idx;
    juce::ComboBox typeBox;
    juce::ToggleButton onBtn;
    juce::TextButton leftBtn, rightBtn;
    std::unique_ptr<Knob> knobs[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
};
} // namespace ui
