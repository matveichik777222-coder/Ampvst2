#pragma once
#include <memory>
#include "../Core/Params.h"
#include "Dynamics.h"
#include "Drive.h"
#include "Modulation.h"
#include "TimeFx.h"
#include "PitchFx.h"
#include "EqFx.h"

namespace gr
{
inline std::unique_ptr<Effect> createEffect (int type)
{
    switch (type)
    {
        case FxGate:       return std::make_unique<NoiseGate>();
        case FxComp:       return std::make_unique<Compressor>();
        case FxOverdrive:  return std::make_unique<Overdrive>();
        case FxDistortion: return std::make_unique<Distortion>();
        case FxFuzz:       return std::make_unique<Fuzz>();
        case FxWah:        return std::make_unique<Wah>();
        case FxOctaver:    return std::make_unique<Octaver>();
        case FxPitch:      return std::make_unique<PitchShifter>();
        case FxChorus:     return std::make_unique<Chorus>();
        case FxPhaser:     return std::make_unique<Phaser>();
        case FxFlanger:    return std::make_unique<Flanger>();
        case FxTremolo:    return std::make_unique<Tremolo>();
        case FxDelay:      return std::make_unique<DelayFx> (false);
        case FxEcho:       return std::make_unique<DelayFx> (true);
        case FxSpring:     return std::make_unique<SpringReverb>();
        case FxHall:       return std::make_unique<ReverbFx> (false);
        case FxPlate:      return std::make_unique<ReverbFx> (true);
        case FxEQ:         return std::make_unique<EqFx>();
        default:           return nullptr;
    }
}
} // namespace gr
