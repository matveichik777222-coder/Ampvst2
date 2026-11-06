#include "FactoryPresets.h"
#include "../Core/Params.h"

using namespace gr;

namespace
{
struct B
{
    FactoryPreset pr;
    B (const char* cat, const char* name) { pr.category = cat; pr.name = name; }
    B& set (const juce::String& id, float v) { pr.values.push_back ({ id, v }); return *this; }
    B& amp (int model, int pickup, float gain, float vol, float bass, float mid, float treb, float pres, float master)
    {
        return set ("amp_model", (float) model).set ("pickup", (float) pickup).set ("amp_gain", gain).set ("amp_volume", vol)
              .set ("amp_bass", bass).set ("amp_mid", mid).set ("amp_treble", treb).set ("amp_presence", pres).set ("amp_master", master);
    }
    B& cab (int type, int spk, int mic, float pos, float dist, float hp = 70.0f, float lp = 12000.0f)
    {
        return set ("cab_type", (float) type).set ("cab_speaker", (float) spk).set ("cab_mic", (float) mic)
              .set ("cab_micpos", pos).set ("cab_micdist", dist).set ("cab_hp", hp).set ("cab_lp", lp);
    }
    B& pre (int i, int type, float a, float b, float c, float d) { return slot (true, i, type, a, b, c, d); }
    B& fx (int i, int type, float a, float b, float c, float d) { return slot (false, i, type, a, b, c, d); }
    B& slot (bool isPre, int i, int type, float a, float b, float c, float d)
    {
        set (slotId (isPre, i, "type"), (float) type).set (slotId (isPre, i, "on"), 1.0f);
        return set (slotId (isPre, i, "p1"), a).set (slotId (isPre, i, "p2"), b)
              .set (slotId (isPre, i, "p3"), c).set (slotId (isPre, i, "p4"), d);
    }
    operator FactoryPreset() const { return pr; }
};
// amp models: 0 American Clean, 1 Plexi, 2 JCM800, 3 Classic Rock, 4 Modern High Gain, 5 Metal, 6 Boutique
// cabs: 0 1x12, 1 2x12, 2 4x12; speakers: 0 V30, 1 Greenback, 2 Alnico Blue, 3 Jensen, 4 G12-65; mics: 0 SM57, 1 MD421, 2 R121, 3 C414
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        // ---- Clean
        B ("Clean", "Glassy Clean").amp (0, 0, 3.0f, 5.0f, 5.0f, 5.0f, 6.0f, 4.0f, 5.0f).cab (1, 3, 3, 0.35f, 0.3f)
            .pre (0, FxComp, 0.35f, 0.4f, 0.5f, 0.45f).fx (0, FxChorus, 0.25f, 0.4f, 0.3f, 0.5f).fx (1, FxSpring, 0.45f, 0.55f, 0.25f, 0.5f),
        B ("Clean", "Studio Sparkle").amp (6, 1, 3.5f, 5.0f, 5.0f, 5.5f, 6.0f, 5.0f, 5.0f).cab (1, 2, 1, 0.25f, 0.25f)
            .pre (0, FxComp, 0.4f, 0.4f, 0.4f, 0.5f).fx (0, FxPlate, 0.4f, 0.6f, 0.2f, 0.1f),
        // ---- Blues
        B ("Blues", "Edge of Breakup").amp (6, 1, 5.0f, 5.0f, 5.0f, 6.0f, 5.5f, 4.5f, 5.0f).cab (1, 2, 1, 0.3f, 0.3f)
            .pre (0, FxOverdrive, 0.3f, 0.5f, 0.55f, 0.35f).fx (0, FxSpring, 0.5f, 0.5f, 0.3f, 0.5f),
        B ("Blues", "Texas Crunch").amp (1, 0, 5.5f, 5.0f, 5.0f, 6.0f, 6.0f, 5.0f, 5.5f).cab (1, 1, 0, 0.3f, 0.25f)
            .pre (0, FxOverdrive, 0.4f, 0.55f, 0.55f, 0.4f).fx (0, FxEcho, 0.35f, 0.3f, 0.45f, 0.25f).fx (1, FxSpring, 0.4f, 0.5f, 0.2f, 0.5f),
        // ---- Rock
        B ("Rock", "Plexi Rock").amp (1, 1, 6.5f, 5.0f, 5.0f, 6.0f, 6.0f, 5.5f, 6.0f).cab (2, 1, 0, 0.3f, 0.2f)
            .fx (0, FxPlate, 0.35f, 0.5f, 0.12f, 0.05f),
        B ("Rock", "JCM Hard Rock").amp (2, 1, 6.5f, 5.0f, 5.0f, 6.0f, 6.0f, 5.5f, 5.5f).cab (2, 0, 0, 0.3f, 0.2f)
            .pre (0, FxOverdrive, 0.2f, 0.6f, 0.6f, 0.5f).fx (0, FxDelay, 0.3f, 0.25f, 0.5f, 0.15f),
        B ("Rock", "Classic Chime").amp (3, 0, 5.0f, 5.0f, 4.5f, 5.5f, 6.5f, 5.0f, 5.5f).cab (1, 4, 1, 0.3f, 0.3f)
            .fx (0, FxSpring, 0.4f, 0.55f, 0.25f, 0.55f),
        // ---- Metal
        B ("Metal", "Modern Metal").amp (5, 1, 7.5f, 5.0f, 6.0f, 4.0f, 6.0f, 6.0f, 5.0f).cab (2, 0, 0, 0.25f, 0.15f)
            .pre (0, FxGate, 0.5f, 0.2f, 0.3f, 0.5f).pre (1, FxOverdrive, 0.2f, 0.5f, 0.55f, 0.8f),
        B ("Metal", "Djent Tight").amp (4, 1, 7.0f, 5.0f, 5.5f, 5.0f, 6.0f, 6.0f, 5.0f).cab (2, 0, 1, 0.3f, 0.15f, 90.0f, 9000.0f)
            .pre (0, FxGate, 0.55f, 0.15f, 0.25f, 0.6f).pre (1, FxOverdrive, 0.15f, 0.55f, 0.55f, 0.9f),
        // ---- Shoegaze
        B ("Shoegaze", "Wall of Fuzz").amp (1, 0, 4.0f, 5.0f, 5.5f, 4.5f, 6.0f, 4.0f, 6.0f).cab (2, 1, 3, 0.4f, 0.5f)
            .pre (0, FxFuzz, 0.7f, 0.45f, 0.5f, 0.5f).fx (0, FxChorus, 0.2f, 0.6f, 0.5f, 0.6f)
            .fx (1, FxDelay, 0.55f, 0.55f, 0.35f, 0.4f).fx (2, FxHall, 0.9f, 0.45f, 0.5f, 0.3f),
        B ("Shoegaze", "Dream Haze").amp (0, 0, 4.5f, 5.0f, 5.0f, 4.0f, 5.5f, 4.0f, 6.0f).cab (1, 3, 3, 0.4f, 0.5f)
            .pre (0, FxFuzz, 0.55f, 0.4f, 0.5f, 0.45f).fx (0, FxPhaser, 0.15f, 0.6f, 0.55f, 0.5f)
            .fx (1, FxEcho, 0.6f, 0.6f, 0.35f, 0.4f).fx (2, FxHall, 0.95f, 0.4f, 0.6f, 0.4f),
        // ---- Indie
        B ("Indie", "Jangle").amp (3, 0, 4.0f, 5.0f, 4.5f, 5.0f, 6.5f, 5.0f, 5.5f).cab (1, 2, 1, 0.3f, 0.3f)
            .pre (0, FxComp, 0.4f, 0.4f, 0.4f, 0.5f).fx (0, FxChorus, 0.3f, 0.35f, 0.3f, 0.45f).fx (1, FxSpring, 0.4f, 0.55f, 0.25f, 0.5f),
        B ("Indie", "Garage Crunch").amp (6, 1, 6.0f, 5.0f, 5.0f, 5.5f, 5.5f, 5.0f, 5.5f).cab (1, 4, 0, 0.3f, 0.3f)
            .pre (0, FxDistortion, 0.3f, 0.5f, 0.6f, 0.4f).fx (0, FxTremolo, 0.35f, 0.4f, 0.2f, 0.0f).fx (1, FxPlate, 0.4f, 0.5f, 0.2f, 0.05f),
        // ---- Ambient
        B ("Ambient", "Infinite Hall").amp (0, 0, 2.0f, 5.0f, 5.0f, 5.0f, 5.5f, 4.0f, 4.5f).cab (1, 3, 3, 0.4f, 0.5f)
            .pre (0, FxComp, 0.5f, 0.4f, 0.5f, 0.5f).fx (0, FxDelay, 0.6f, 0.7f, 0.4f, 0.45f).fx (1, FxHall, 0.95f, 0.45f, 0.65f, 0.5f),
        B ("Ambient", "Shimmer Pad").amp (0, 0, 2.5f, 5.0f, 5.0f, 4.5f, 5.5f, 4.0f, 4.5f).cab (1, 3, 3, 0.4f, 0.5f)
            .fx (0, FxPitch, 1.0f, 0.5f, 0.4f, 0.6f).fx (1, FxEcho, 0.6f, 0.6f, 0.3f, 0.4f).fx (2, FxHall, 0.95f, 0.5f, 0.7f, 0.5f),
    };
    return presets;
}
