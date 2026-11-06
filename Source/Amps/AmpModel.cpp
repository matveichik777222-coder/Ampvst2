#include "AmpModel.h"

namespace gr
{
const AmpVoicing kAmpVoicings[kNumAmpModels] = {
    // name, stages, gMin, gMax, bias, tight, couple[3], miller, bassHz, midHz, midQ, trebHz, midOff, bassR, midR, trebR, sag, power, presHz, presR, outLp
    { "American Clean", 1,  -8.f, 26.f, 0.05f,  70.f, { 40.f,  40.f,  40.f }, 11000.f, 100.f,  500.f, 0.7f, 3500.f, -4.f, 14.f, 10.f, 14.f, 0.10f, 0.8f, 5000.f,  7.f, 10000.f },
    { "British Plexi",  2,   4.f, 44.f, 0.18f,  90.f, { 35.f,  60.f,  60.f },  9000.f, 110.f,  700.f, 0.8f, 3000.f,  2.f, 14.f, 12.f, 14.f, 0.35f, 1.5f, 4500.f,  8.f,  7500.f },
    { "JCM800",         2,  10.f, 56.f, 0.22f, 130.f, { 40.f,  90.f,  90.f },  8000.f, 100.f,  800.f, 0.9f, 2800.f,  3.f, 14.f, 12.f, 14.f, 0.22f, 1.1f, 4000.f,  9.f,  7000.f },
    { "Classic Rock",   2,   6.f, 40.f, 0.15f, 100.f, { 30.f,  50.f,  50.f }, 12000.f, 120.f, 1000.f, 0.9f, 4500.f,  1.f, 14.f, 10.f, 14.f, 0.45f, 1.3f, 6000.f,  6.f,  9000.f },
    { "Modern High Gain", 3, 14.f, 66.f, 0.12f, 200.f, { 50.f, 100.f, 120.f },  7500.f,  90.f,  650.f, 1.0f, 3200.f, -2.f, 14.f, 12.f, 14.f, 0.08f, 0.9f, 4500.f, 10.f,  6500.f },
    { "Metal",          3,  18.f, 72.f, 0.10f, 300.f, { 60.f, 130.f, 150.f },  7000.f,  85.f,  500.f, 1.2f, 3500.f, -7.f, 14.f, 12.f, 14.f, 0.05f, 0.8f, 5000.f, 10.f,  6000.f },
    { "Boutique",       2,   0.f, 40.f, 0.30f,  80.f, { 30.f,  45.f,  45.f }, 10000.f, 100.f,  600.f, 0.7f, 3000.f,  4.f, 14.f, 12.f, 14.f, 0.50f, 1.2f, 4500.f,  6.f,  8500.f },
};

void AmpModel::prepare (double sr, int maxBlock)
{
    sampleRate = sr;
    os2.initProcessing ((size_t) maxBlock);
    os4.initProcessing ((size_t) maxBlock);
    stageGain.prepare (sr * 4.0);
    volume.prepare (sr * 4.0);
    drive.prepare (sr * 4.0);
    reset();
}

void AmpModel::reset()
{
    os2.reset(); os4.reset();
    for (auto* b : { &inHp, &pickupFilt, &low, &midPk, &high, &pres, &outHp, &outLp1, &outLp2 }) b->reset();
    for (auto& c : coupling) c.reset();
    for (auto& m : miller) m = 0.0f;
    sagEnv = 0.0f;
}

int AmpModel::getLatencySamples() const
{
    const auto& os = activeOs == 0 ? os2 : os4;
    return (int) std::lround (os.getLatencyInSamples());
}

void AmpModel::updateCoefficients (const AmpVoicing& v, const Params& p, double fs)
{
    stages = v.stages;
    bias = v.bias;
    const float g01 = juce::jlimit (0.0f, 1.0f, p.gain * 0.1f);
    const float gDb = mapLin (std::pow (g01, 1.6f), v.gMinDb, v.gMaxDb);
    stageGain.set (std::pow (dbToGain (gDb), 1.0f / (float) v.stages));
    volume.set (juce::jlimit (0.0f, 1.0f, p.volume * 0.1f) * 2.0f);
    const float m01 = juce::jlimit (0.0f, 1.0f, p.master * 0.1f);
    drive.set (v.power * mapLin (m01 * m01, 0.4f, 3.0f));

    // Pickup voicing: single coils are quieter & brighter, humbuckers hotter & thicker
    if (p.pickup == 0)
    {
        inTrim = dbToGain (3.0f);
        pickupFilt.set (BQ::HighShelf, fs, 5000.0, 0.7, -2.5);
    }
    else
    {
        inTrim = 1.0f;
        pickupFilt.set (BQ::Peak, fs, 300.0, 0.7, -2.0);
    }

    inHp.set (BQ::HighPass, fs, v.tightHz, 0.707, 0.0);
    for (int i = 0; i < 3; ++i) coupling[i].set (BQ::HighPass, fs, v.couple[i], 0.707, 0.0);
    millerCoef = onePoleCoef (mapLin (g01, v.millerHz * 1.4f, v.millerHz * 0.6f), fs);

    low.set (BQ::LowShelf, fs, v.bassHz, 0.7, (p.bass - 5.0f) * 0.2f * v.bassRange);
    midPk.set (BQ::Peak, fs, v.midHz, v.midQ, v.midOffsetDb + (p.mid - 5.0f) * 0.2f * v.midRange);
    high.set (BQ::HighShelf, fs, v.trebHz, 0.7, (p.treble - 5.0f) * 0.2f * v.trebRange);
    pres.set (BQ::HighShelf, fs, v.presHz, 0.7, p.presence * 0.1f * v.presRange);
    outHp.set (BQ::HighPass, fs, 45.0, 0.707, 0.0);
    outLp1.set (BQ::LowPass, fs, v.outLpHz, 0.707, 0.0);
    outLp2.set (BQ::LowPass, fs, v.outLpHz * 1.2f, 0.6, 0.0);

    sagAtk = timeCoef (8.0f, fs);
    sagRel = timeCoef (180.0f, fs);
    sagAmt = v.sag;
}

float AmpModel::processSample (float x) noexcept
{
    float s = inHp.process (x * inTrim, 0);
    s = pickupFilt.process (s, 0);

    // Preamp: cascaded asymmetric triode stages with coupling caps and Miller roll-off
    const float sg = stageGain.next();
    for (int st = 0; st < stages; ++st)
    {
        float y = tube (s * sg + (st == 0 ? bias : bias * 0.6f));
        y = coupling[st].process (y, 0);
        miller[st] += millerCoef * (y - miller[st]);
        s = miller[st];
    }

    // Tone stack
    s = low.process (s, 0);
    s = midPk.process (s, 0);
    s = high.process (s, 0);
    s *= volume.next();

    // Power amp: sag (supply droop follows signal energy) + push-pull saturation
    const float a = std::abs (s);
    sagEnv += (a > sagEnv ? sagAtk : sagRel) * (a - sagEnv);
    const float sagGain = 1.0f / (1.0f + sagAmt * 1.2f * sagEnv);
    const float dr = drive.next();
    const float in = s * dr * sagGain;
    float out = fastTanh (in + 0.05f * in * in);
    out /= (0.6f + 0.4f * dr);

    out = pres.process (out, 0);
    out = outHp.process (out, 0);
    out = outLp2.process (outLp1.process (out, 0), 0);
    return out * 0.7f;
}

void AmpModel::process (juce::AudioBuffer<float>& mono, const Params& p)
{
    const int idx = p.oversampling == 0 ? 0 : 1;
    auto& os = idx == 0 ? os2 : os4;
    if (idx != activeOs) { activeOs = idx; os.reset(); }
    const double fs = sampleRate * (idx == 0 ? 2.0 : 4.0);

    updateCoefficients (kAmpVoicings[juce::jlimit (0, kNumAmpModels - 1, p.model)], p, fs);

    juce::dsp::AudioBlock<float> block (mono);
    auto up = os.processSamplesUp (block);
    float* x = up.getChannelPointer (0);
    const int n = (int) up.getNumSamples();
    for (int i = 0; i < n; ++i)
        x[i] = processSample (x[i]);
    os.processSamplesDown (block);
}
} // namespace gr
