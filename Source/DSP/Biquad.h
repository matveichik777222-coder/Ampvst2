#pragma once
#include "Common.h"

namespace gr
{
enum class BQ { LowPass, HighPass, BandPass, Peak, LowShelf, HighShelf };

struct BiquadCoefs { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

// RBJ cookbook
inline BiquadCoefs designBiquad (BQ t, double sr, double f, double q, double gainDb)
{
    f = std::clamp (f, 10.0, sr * 0.45);
    q = std::max (q, 0.1);
    const double w0 = 2.0 * 3.14159265358979323846 * f / sr;
    const double c = std::cos (w0), s = std::sin (w0);
    const double alpha = s / (2.0 * q);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double sqA2a = 2.0 * std::sqrt (A) * alpha;
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (t)
    {
        case BQ::LowPass:
            b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
        case BQ::HighPass:
            b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
        case BQ::BandPass:
            b0 = alpha; b1 = 0; b2 = -alpha; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
        case BQ::Peak:
            b0 = 1 + alpha * A; b1 = -2 * c; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * c; a2 = 1 - alpha / A; break;
        case BQ::LowShelf:
            b0 = A * ((A + 1) - (A - 1) * c + sqA2a);
            b1 = 2 * A * ((A - 1) - (A + 1) * c);
            b2 = A * ((A + 1) - (A - 1) * c - sqA2a);
            a0 = (A + 1) + (A - 1) * c + sqA2a;
            a1 = -2 * ((A - 1) + (A + 1) * c);
            a2 = (A + 1) + (A - 1) * c - sqA2a; break;
        case BQ::HighShelf:
            b0 = A * ((A + 1) + (A - 1) * c + sqA2a);
            b1 = -2 * A * ((A - 1) + (A + 1) * c);
            b2 = A * ((A + 1) + (A - 1) * c - sqA2a);
            a0 = (A + 1) - (A - 1) * c + sqA2a;
            a1 = 2 * ((A - 1) - (A + 1) * c);
            a2 = (A + 1) - (A - 1) * c - sqA2a; break;
    }
    BiquadCoefs r;
    r.b0 = (float) (b0 / a0); r.b1 = (float) (b1 / a0); r.b2 = (float) (b2 / a0);
    r.a1 = (float) (a1 / a0); r.a2 = (float) (a2 / a0);
    return r;
}

// Transposed direct form II, up to 2 channels, allocation-free coefficient updates
struct Biquad
{
    BiquadCoefs c;
    float z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void set (BQ t, double sr, double f, double q, double gainDb) { c = designBiquad (t, sr, f, q, gainDb); }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
    inline float process (float x, int ch) noexcept
    {
        const float y = c.b0 * x + z1[ch];
        z1[ch] = c.b1 * x - c.a1 * y + z2[ch];
        z2[ch] = c.b2 * x - c.a2 * y;
        return y;
    }
};

// Cytomic/Simper state-variable filter (stable under fast modulation)
struct Svf
{
    float ic1[2] { 0, 0 }, ic2[2] { 0, 0 };
    float a1 = 0, a2 = 0, a3 = 0, k = 1;

    void reset() { ic1[0] = ic1[1] = ic2[0] = ic2[1] = 0.0f; }
    void set (float fc, float q, double sr)
    {
        const float g = std::tan (kPi * std::min (fc, 0.45f * (float) sr) / (float) sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    // unity-gain-at-peak band-pass
    inline float bandpass (float v0, int ch) noexcept
    {
        const float v3 = v0 - ic2[ch];
        const float v1 = a1 * ic1[ch] + a2 * v3;
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
        ic1[ch] = 2.0f * v1 - ic1[ch];
        ic2[ch] = 2.0f * v2 - ic2[ch];
        return k * v1;
    }
};
} // namespace gr
