#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui
{
class Knob : public juce::Component
{
public:
    enum class Fmt { Raw, Ten, Hz, Db };

    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, Fmt fmt = Fmt::Raw)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        switch (fmt)
        {
            case Fmt::Ten:
                slider.textFromValueFunction = [] (double v) { return juce::String (v * 10.0, 1); };
                slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() / 10.0; };
                break;
            case Fmt::Hz:
                slider.textFromValueFunction = [] (double v) { return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String ((int) v) + " Hz"; };
                slider.valueFromTextFunction = [] (const juce::String& t)
                { auto v = t.getDoubleValue(); return t.containsIgnoreCase ("k") ? v * 1000.0 : v; };
                break;
            case Fmt::Db:
                slider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };
                slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
                break;
            default: slider.setNumDecimalPlacesToDisplay (1); break;
        }
        if (auto* p = s.getParameter (id))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        name.setJustificationType (juce::Justification::centred);
        name.setText (text, juce::dontSendNotification);
        name.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        addAndMakeVisible (slider);
        addAndMakeVisible (name);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    }

    void setLabel (const juce::String& t) { name.setText (t, juce::dontSendNotification); setVisible (t.isNotEmpty()); }

    void resized() override
    {
        auto r = getLocalBounds();
        name.setBounds (r.removeFromTop (16));
        slider.setBounds (r);
    }

private:
    juce::Slider slider;
    juce::Label name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

inline void fillCombo (juce::ComboBox& cb, juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        cb.addItemList (p->choices, 1);
}
} // namespace ui
