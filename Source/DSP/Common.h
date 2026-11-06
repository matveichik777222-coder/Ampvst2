#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace gr
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-6f)); }
inline float mapLin (float v, float lo, float hi) noexcept { return lo + (hi - lo) * v; }
inline float mapExp (float v, float lo, float hi) noexcept { return lo * std::pow (hi / lo, v); }
inline float onePoleCoef (float fc, double sr) noexcept
{
    return 1.0f - std::exp (-kTwoPi * std::min (fc, 0.45f * (float) sr) / (float) sr);
}
inline float timeCoef (float ms, double sr) noexcept
{
    return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sr));
}

// Rational tanh approximation (cheap, smooth, exact +-1 beyond |x|>3)
inline float fastTanh (float x) noexcept
{
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// First-order antiderivative anti-aliasing (ADAA) waveshapers: alias reduction without oversampling latency
inline float logCosh (float x) noexcept
{
    x = std::abs (x);
    return x + std::log1p (std::exp (-2.0f * x)) - 0.69314718f;
}

struct AdaaTanh
{
    float x1 = 0.0f;
    void reset() { x1 = 0.0f; }
    float process (float x) noexcept
    {
        const float d = x - x1;
        const float r = std::abs (d) < 1.0e-4f ? std::tanh (0.5f * (x + x1)) : (logCosh (x) - logCosh (x1)) / d;
        x1 = x;
        return r;
    }
};

struct AdaaClip
{
    float x1 = 0.0f;
    void reset() { x1 = 0.0f; }
    static float F (float x) noexcept { const float a = std::abs (x); return a <= 1.0f ? 0.5f * x * x : a - 0.5f; }
    float process (float x) noexcept
    {
        const float d = x - x1;
        const float r = std::abs (d) < 1.0e-4f ? std::clamp (0.5f * (x + x1), -1.0f, 1.0f) : (F (x) - F (x1)) / d;
        x1 = x;
        return r;
    }
};

// Smoothed parameter that snaps to its first value
struct SV
{
    juce::SmoothedValue<float> v;
    bool init = false;
    void prepare (double sr, double seconds = 0.03) { v.reset (sr, seconds); init = false; }
    void set (float x)
    {
        if (! init) { v.setCurrentAndTargetValue (x); init = true; }
        else v.setTargetValue (x);
    }
    float next() noexcept { return v.getNextValue(); }
};

// Circular delay line with Hermite interpolation. read(d): d samples ago (d >= 2)
struct DelayBuf
{
    std::vector<float> b;
    int mask = 0, w = 0;

    void prepare (int maxSamples)
    {
        int n = 1;
        while (n < maxSamples + 8) n <<= 1;
        b.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); w = 0; }
    inline void push (float x) noexcept { b[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float d) const noexcept
    {
        d = std::clamp (d, 2.0f, (float) (mask - 4));
        const int di = (int) d;
        const float f = d - (float) di;
        const float ym1 = b[(size_t) ((w - (di - 1)) & mask)];
        const float y0  = b[(size_t) ((w - di) & mask)];
        const float y1  = b[(size_t) ((w - di - 1) & mask)];
        const float y2  = b[(size_t) ((w - di - 2) & mask)];
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + y0;
    }
};
} // namespace gr
