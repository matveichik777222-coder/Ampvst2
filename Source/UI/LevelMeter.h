#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

namespace ui
{
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (std::atomic<float>& src, const juce::String& t) : source (src), title (t) { startTimerHz (30); }
    ~LevelMeter() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        auto bar = r.withTrimmedTop (12.0f).reduced (0.0f, 1.0f);
        g.setColour (Colour (0xff0d0d0f)); g.fillRoundedRectangle (bar, 2.0f);
        const float db = Decibels::gainToDecibels (shown, -60.0f);
        const float f = jlimit (0.0f, 1.0f, jmap (db, -60.0f, 3.0f, 0.0f, 1.0f));
        auto fill = bar.withWidth (bar.getWidth() * f);
        g.setGradientFill (ColourGradient (Colour (0xff3fbf5f), bar.getX(), 0.0f, Colour (0xffe84a3d), bar.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, 2.0f);
        g.setColour (Colour (0xff9a9a9a)); g.setFont (10.0f);
        g.drawText (title, r.removeFromTop (12.0f).toNearestInt(), Justification::centredLeft);
    }

private:
    void timerCallback() override
    {
        const float v = source.load();
        shown = v > shown ? v : shown * 0.9f + v * 0.1f;
        repaint();
    }
    std::atomic<float>& source;
    juce::String title;
    float shown = 0.0f;
};
} // namespace ui
