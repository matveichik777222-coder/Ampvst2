#include "CabinetEngine.h"

namespace gr
{
namespace
{
struct Spk { const char* name; float peakHz, peakDb, peakQ, lpHz, lowHz, lowDb; };
const Spk kSpk[] = {
    { "V30",         3000.f, 5.0f, 1.2f, 5800.f, 110.f, 1.5f },
    { "Greenback",   2000.f, 3.5f, 1.0f, 4600.f, 120.f, 3.0f },
    { "Alnico Blue", 2600.f, 3.0f, 0.9f, 6500.f, 100.f, 2.0f },
    { "Jensen C12N", 4000.f, 2.0f, 0.8f, 7500.f, 100.f, 1.5f },
    { "G12-65",      1800.f, 4.0f, 1.0f, 5000.f, 105.f, 3.0f },
};
struct Cab { float resHz, resDb, hpHz; int taps; };
const Cab kCab[] = { { 120.f, 3.0f, 95.f, 0 }, { 105.f, 4.0f, 85.f, 1 }, { 85.f, 5.5f, 70.f, 2 } };
}

std::vector<float> CabinetEngine::synthesizeIR (double sr, int type, int spk, int mic, float pos, float dist)
{
    type = juce::jlimit (0, 2, type);
    spk = juce::jlimit (0, 4, spk);
    mic = juce::jlimit (0, 3, mic);
    pos = juce::jlimit (0.0f, 1.0f, pos);
    dist = juce::jlimit (0.0f, 1.0f, dist);

    const int N = (int) std::round (sr * 0.085);
    std::vector<float> h ((size_t) N, 0.0f);
    h[0] = 1.0f;

    auto apply = [&] (BQ t, double f, double q, double g)
    {
        Biquad b;
        b.set (t, sr, f, q, g);
        for (auto& s : h) s = b.process (s, 0);
    };

    const Spk& S = kSpk[spk];
    const Cab& C = kCab[type];

    // Cabinet + speaker
    apply (BQ::HighPass, C.hpHz, 0.8, 0.0);
    apply (BQ::Peak, C.resHz, 1.2, C.resDb);
    apply (BQ::LowShelf, S.lowHz, 0.7, S.lowDb);
    apply (BQ::Peak, S.peakHz, S.peakQ, S.peakDb);
    const double lpFc = S.lpHz * (1.0 - 0.4 * pos);
    apply (BQ::LowPass, lpFc, 0.65, 0.0);
    apply (BQ::LowPass, lpFc * 1.15, 0.7, 0.0);

    // Microphone colour
    switch (mic)
    {
        case 0: apply (BQ::Peak, 5500.0, 1.0, 4.0); apply (BQ::HighPass, 80.0, 0.7, 0.0); break;       // SM57
        case 1: apply (BQ::LowShelf, 120.0, 0.7, 2.0); apply (BQ::Peak, 3500.0, 0.8, 3.0); break;      // MD421
        case 2: apply (BQ::LowShelf, 180.0, 0.7, 3.0); apply (BQ::LowPass, 7000.0, 0.7, 0.0);
                apply (BQ::Peak, 3000.0, 0.8, -1.5); break;                                             // R121 ribbon
        default: apply (BQ::HighShelf, 9000.0, 0.7, 3.0); apply (BQ::LowPass, 14000.0, 0.7, 0.0); break; // C414
    }

    // Mic position (cap -> edge) and distance (proximity effect)
    apply (BQ::HighShelf, 4000.0, 0.7, (0.5 - pos) * 8.0);
    apply (BQ::LowShelf, 200.0, 0.7, 1.5 * pos);
    apply (BQ::LowShelf, 150.0, 0.7, (1.0 - dist) * 3.0 + (mic == 2 ? 1.5 : 0.0));

    // Multi-speaker comb filtering
    if (C.taps > 0)
    {
        const std::vector<float> src = h;
        auto add = [&] (double ms, float a)
        {
            const int d = (int) (ms * 0.001 * sr);
            for (int i = d; i < N; ++i) h[(size_t) i] += a * src[(size_t) (i - d)];
        };
        add (0.38, -0.14f);
        if (C.taps > 1) add (0.71, 0.12f);
    }

    // Room reflections scale with mic distance
    if (dist > 0.01f)
    {
        std::vector<float> r ((size_t) N, 0.0f);
        const double tms[] = { 6.3, 11.1, 17.9, 24.7, 33.5, 41.3 };
        const float amp[] = { 0.5f, 0.42f, 0.34f, 0.28f, 0.22f, 0.18f };
        for (int k = 0; k < 6; ++k)
        {
            const int d = (int) (tms[k] * 0.001 * sr);
            if (d < N) r[(size_t) d] += (k % 2 ? -1.0f : 1.0f) * amp[k];
        }
        Biquad rl; rl.set (BQ::LowPass, sr, 2500.0, 0.6, 0.0);
        for (auto& s : r) s = rl.process (s, 0);
        for (int i = 0; i < N; ++i) h[(size_t) i] += dist * 0.5f * r[(size_t) i];
    }

    // Diffuse cone/cabinet tail
    {
        juce::Random rng (1234);
        Biquad nl; nl.set (BQ::LowPass, sr, 3000.0, 0.7, 0.0);
        for (int i = 0; i < N; ++i)
        {
            const float env = std::exp (-(float) i / (float) (sr * 0.012));
            h[(size_t) i] += 0.03f * env * nl.process (rng.nextFloat() * 2.0f - 1.0f, 0);
        }
    }

    // Fade the end, normalise to unit energy
    const int fade = N / 8;
    for (int i = 0; i < fade; ++i)
        h[(size_t) (N - fade + i)] *= 0.5f + 0.5f * std::cos (kPi * (float) i / (float) fade);

    double e = 0.0;
    for (auto s : h) e += (double) s * s;
    const float sc = e > 1.0e-12 ? (float) (1.0 / std::sqrt (e)) : 1.0f;
    for (auto& s : h) s *= sc;
    return h;
}

void CabinetEngine::prepare (double sr, int maxBlock)
{
    sampleRate = sr;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 1 };
    conv.prepare (spec);
    level.prepare (sr);
    reset();
}

void CabinetEngine::reset()
{
    conv.reset();
    hp.reset();
    lp.reset();
}

void CabinetEngine::loadBuiltIn (int type, int speaker, int mic, float micPos, float micDist)
{
    auto ir = synthesizeIR (sampleRate, type, speaker, mic, micPos, micDist);
    juce::AudioBuffer<float> buf (1, (int) ir.size());
    buf.copyFrom (0, 0, ir.data(), (int) ir.size());
    conv.loadImpulseResponse (std::move (buf), sampleRate,
                              juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::no,
                              juce::dsp::Convolution::Normalise::no);
}

void CabinetEngine::loadFile (const juce::File& f)
{
    conv.loadImpulseResponse (f, juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::yes, 0,
                              juce::dsp::Convolution::Normalise::yes);
}

void CabinetEngine::process (juce::AudioBuffer<float>& mono, const Params& p)
{
    if (! p.on) return;

    juce::dsp::AudioBlock<float> blk (mono);
    conv.process (juce::dsp::ProcessContextReplacing<float> (blk));

    hp.set (BQ::HighPass, sampleRate, p.hp, 0.707, 0.0);
    lp.set (BQ::LowPass, sampleRate, p.lp, 0.707, 0.0);
    level.set (dbToGain (p.levelDb));

    float* d = mono.getWritePointer (0);
    const int n = mono.getNumSamples();
    for (int i = 0; i < n; ++i)
        d[i] = lp.process (hp.process (d[i], 0), 0) * level.next();
}
} // namespace gr
