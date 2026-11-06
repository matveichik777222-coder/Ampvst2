#include "Params.h"
#include "../Amps/AmpModel.h"

namespace gr
{
static const FxInfo kFx[FxCount] = {
    { "Off",           { "", "", "", "" } },
    { "Noise Gate",    { "Thresh", "Attack", "Release", "Depth" } },
    { "Compressor",    { "Sustain", "Ratio", "Attack", "Level" } },
    { "Overdrive",     { "Drive", "Tone", "Level", "Tight" } },
    { "Distortion",    { "Dist", "Filter", "Level", "Clip" } },
    { "Fuzz",          { "Fuzz", "Tone", "Level", "Bias" } },
    { "Wah",           { "Pedal", "Q", "Auto", "Level" } },
    { "Octaver",       { "Sub", "Up", "Dry", "Tone" } },
    { "Pitch Shifter", { "Pitch", "Fine", "Mix", "Window" } },
    { "Chorus",        { "Rate", "Depth", "Mix", "Delay" } },
    { "Phaser",        { "Rate", "Depth", "Feedback", "Center" } },
    { "Flanger",       { "Rate", "Depth", "Regen", "Manual" } },
    { "Tremolo",       { "Rate", "Depth", "Shape", "Stereo" } },
    { "Delay",         { "Time", "Feedback", "Tone", "Mix" } },
    { "Echo (Tape)",   { "Time", "Feedback", "Tone", "Mix" } },
    { "Spring Reverb", { "Decay", "Tone", "Mix", "Drip" } },
    { "Hall Reverb",   { "Decay", "Tone", "Mix", "PreDelay" } },
    { "Plate Reverb",  { "Decay", "Tone", "Mix", "PreDelay" } },
    { "EQ",            { "Bass", "Mid", "Treble", "Mid Freq" } },
};

const FxInfo& fxInfo (int type) { return kFx[juce::jlimit (0, (int) FxCount - 1, type)]; }

juce::String slotId (bool pre, int index, const juce::String& suffix)
{
    return juce::String (pre ? "pre" : "fx") + juce::String (index + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;

    auto F = [&L] (const String& id, const String& name, float lo, float hi, float def, float skew = 1.0f)
    {
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                      NormalisableRange<float> (lo, hi, 0.0f, skew), def));
    };
    auto C = [&L] (const String& id, const String& name, const StringArray& items, int def)
    {
        L.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto B = [&L] (const String& id, const String& name, bool def)
    {
        L.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    F ("in_gain", "Input", -24.0f, 24.0f, 0.0f);
    F ("out_gain", "Output", -24.0f, 24.0f, 0.0f);

    StringArray ampNames;
    for (int i = 0; i < kNumAmpModels; ++i) ampNames.add (kAmpVoicings[i].name);
    B ("amp_on", "Amp On", true);
    C ("amp_model", "Amp Model", ampNames, 1);
    C ("pickup", "Pickup", { "Single Coil", "Humbucker" }, 1);
    C ("oversample", "Oversampling", { "2x", "4x" }, 1);
    F ("amp_gain", "Gain", 0.0f, 10.0f, 5.0f);
    F ("amp_volume", "Volume", 0.0f, 10.0f, 5.0f);
    F ("amp_bass", "Bass", 0.0f, 10.0f, 5.0f);
    F ("amp_mid", "Middle", 0.0f, 10.0f, 5.0f);
    F ("amp_treble", "Treble", 0.0f, 10.0f, 5.0f);
    F ("amp_presence", "Presence", 0.0f, 10.0f, 5.0f);
    F ("amp_master", "Master", 0.0f, 10.0f, 5.0f);

    B ("cab_on", "Cabinet On", true);
    C ("cab_type", "Cabinet", { "1x12", "2x12", "4x12" }, 2);
    C ("cab_speaker", "Speaker", { "V30", "Greenback", "Alnico Blue", "Jensen C12N", "G12-65" }, 0);
    C ("cab_mic", "Microphone", { "SM57", "MD421", "R121 Ribbon", "C414 Condenser" }, 0);
    F ("cab_micpos", "Mic Position", 0.0f, 1.0f, 0.3f);
    F ("cab_micdist", "Mic Distance", 0.0f, 1.0f, 0.2f);
    F ("cab_hp", "Low Cut", 20.0f, 500.0f, 70.0f, 0.4f);
    F ("cab_lp", "High Cut", 2000.0f, 20000.0f, 12000.0f, 0.5f);
    F ("cab_level", "Cab Level", -12.0f, 12.0f, 0.0f);
    B ("cab_user", "Use Custom IR", false);

    StringArray fxNames;
    for (int i = 0; i < FxCount; ++i) fxNames.add (kFx[i].name);

    auto addSlots = [&] (bool pre, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const String nm = String (pre ? "Pedal " : "Effect ") + String (i + 1);
            C (slotId (pre, i, "type"), nm + " Type", fxNames, 0);
            B (slotId (pre, i, "on"), nm + " On", true);
            for (int k = 1; k <= 4; ++k)
                F (slotId (pre, i, "p" + String (k)), nm + " Param " + String (k), 0.0f, 1.0f, 0.5f);
        }
    };
    addSlots (true, kNumPreSlots);
    addSlots (false, kNumPostSlots);
    return L;
}
} // namespace gr
