#pragma once
#include <array>
#include <memory>
#include "../Effects/EffectRegistry.h"

namespace gr
{
// One position in the signal chain. The user picks the effect type per slot, so the
// order of effects is simply the order of slots.
class EffectSlot
{
public:
    void prepare (double sr, int maxBlock, int numChannels)
    {
        for (int t = 1; t < FxCount; ++t)
        {
            fx[(size_t) t] = createEffect (t);
            if (fx[(size_t) t]) fx[(size_t) t]->prepare (sr, maxBlock, numChannels);
        }
        lastType = 0; wasActive = false;
    }

    void process (juce::AudioBuffer<float>& buf, int type, bool on, const float* p)
    {
        type = juce::jlimit (0, (int) FxCount - 1, type);
        const bool active = on && type != FxNone;
        if ((type != lastType || (active && ! wasActive)) && active && fx[(size_t) type])
            fx[(size_t) type]->reset();
        lastType = type;
        wasActive = active;
        if (active && fx[(size_t) type]) fx[(size_t) type]->process (buf, p);
    }
private:
    std::array<std::unique_ptr<Effect>, FxCount> fx;
    int lastType = 0;
    bool wasActive = false;
};
} // namespace gr
