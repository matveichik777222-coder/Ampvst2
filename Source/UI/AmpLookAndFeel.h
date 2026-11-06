#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
inline juce::Colour accent() { return juce::Colour (0xffe8a33d); }
inline juce::Colour panel()  { return juce::Colour (0xff1d1e22); }

class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AmpLookAndFeel()
    {
        using namespace juce;
        setColour (ResizableWindow::backgroundColourId, Colour (0xff111214));
        setColour (ComboBox::backgroundColourId, Colour (0xff26272c));
        setColour (ComboBox::outlineColourId, Colour (0xff3a3b42));
        setColour (ComboBox::textColourId, Colour (0xffe2e2e2));
        setColour (ComboBox::arrowColourId, accent());
        setColour (PopupMenu::backgroundColourId, Colour (0xff1d1e22));
        setColour (PopupMenu::highlightedBackgroundColourId, accent().darker (0.4f));
        setColour (PopupMenu::textColourId, Colour (0xffe2e2e2));
        setColour (Slider::textBoxTextColourId, Colour (0xffbfbfbf));
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Label::textColourId, Colour (0xffcfcfcf));
        setColour (TextButton::buttonColourId, Colour (0xff26272c));
        setColour (TextButton::buttonOnColourId, accent());
        setColour (TextButton::textColourOffId, Colour (0xffd0d0d0));
        setColour (TextButton::textColourOnId, Colours::black);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace juce;
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);

        Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (Colour (0xff2c2d33)); g.strokePath (track, PathStrokeType (3.0f));
        Path val; val.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
        g.setColour (accent()); g.strokePath (val, PathStrokeType (3.0f));

        const float kr = r * 0.78f;
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - kr, c.y - kr + 2.0f, kr * 2.0f, kr * 2.0f);
        g.setGradientFill (ColourGradient (Colour (0xff5b5d65), c.x - kr * 0.4f, c.y - kr * 0.7f,
                                           Colour (0xff17181b), c.x + kr * 0.5f, c.y + kr * 0.8f, true));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (Colour (0xff0b0b0d)); g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.5f);

        Path ptr; ptr.addRoundedRectangle (-1.8f, -kr + 3.0f, 3.6f, kr * 0.45f, 1.5f);
        g.setColour (Colours::white); g.fillPath (ptr, AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& bt, bool hl, bool) override
    {
        using namespace juce;
        auto b = bt.getLocalBounds().toFloat();
        const float d = jmin (b.getHeight() - 8.0f, 16.0f);
        Rectangle<float> led (b.getX() + 4.0f, b.getCentreY() - d * 0.5f, d, d);
        const bool on = bt.getToggleState();
        if (on)
        {
            g.setColour (accent().withAlpha (0.25f)); g.fillEllipse (led.expanded (3.0f));
            g.setColour (accent());
        }
        else g.setColour (Colour (0xff3a3b42));
        g.fillEllipse (led);
        g.setColour (Colours::black.withAlpha (0.6f)); g.drawEllipse (led, 1.0f);
        g.setColour (hl ? Colours::white : Colour (0xffcfcfcf));
        g.setFont (13.0f);
        g.drawText (bt.getButtonText(), (int) (led.getRight() + 6.0f), 0, bt.getWidth() - (int) led.getRight() - 6, bt.getHeight(),
                    Justification::centredLeft);
    }
};
} // namespace ui
