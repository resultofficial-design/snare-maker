#pragma once
#include <cmath>

// =============================================================================
// BiquadFilter – sample-accurate biquad using Transposed Direct Form II.
//
// Coefficient formulas from "Audio EQ Cookbook" by Robert Bristow-Johnson.
// No heap allocation.  No juce_dsp dependency.
// =============================================================================
struct BiquadFilter
{
    // ── Coefficient setters ───────────────────────────────────────────────────

    void setLowPass (double sampleRate, double freq, double q) noexcept
    {
        const double w0   = k2Pi * freq / sampleRate;
        const double a    = std::sin(w0) / (2.0 * q);
        const double cosW = std::cos(w0);
        setCoeffs ((1.0 - cosW) * 0.5,
                    1.0 - cosW,
                   (1.0 - cosW) * 0.5,
                    1.0 + a,
                   -2.0 * cosW,
                    1.0 - a);
    }

    void setHighPass (double sampleRate, double freq, double q) noexcept
    {
        const double w0   = k2Pi * freq / sampleRate;
        const double a    = std::sin(w0) / (2.0 * q);
        const double cosW = std::cos(w0);
        setCoeffs ( (1.0 + cosW) * 0.5,
                   -(1.0 + cosW),
                    (1.0 + cosW) * 0.5,
                    1.0 + a,
                   -2.0 * cosW,
                    1.0 - a);
    }

    void setBandPass (double sampleRate, double freq, double q) noexcept
    {
        const double w0   = k2Pi * freq / sampleRate;
        const double a    = std::sin(w0) / (2.0 * q);
        const double sinW = std::sin(w0);
        const double cosW = std::cos(w0);
        setCoeffs ( sinW * 0.5,
                    0.0,
                   -sinW * 0.5,
                    1.0 + a,
                   -2.0 * cosW,
                    1.0 - a);
    }

    // gainDb > 0 → boost high frequencies;  < 0 → cut (tilt EQ / brightness)
    void setHighShelf (double sampleRate, double freq, double q, double gainDb) noexcept
    {
        const double A    = std::pow (10.0, gainDb / 40.0); // = sqrt(10^(dB/20))
        const double w0   = k2Pi * freq / sampleRate;
        const double a    = std::sin(w0) / (2.0 * q);
        const double cosW = std::cos(w0);
        const double sqA  = std::sqrt(A);

        setCoeffs (
             A  * ((A+1.0) + (A-1.0)*cosW + 2.0*sqA*a),
            -2.0*A * ((A-1.0) + (A+1.0)*cosW),
             A  * ((A+1.0) + (A-1.0)*cosW - 2.0*sqA*a),
             (A+1.0) - (A-1.0)*cosW + 2.0*sqA*a,
             2.0 * ((A-1.0) - (A+1.0)*cosW),
             (A+1.0) - (A-1.0)*cosW - 2.0*sqA*a);
    }

    // ── Per-sample processing ─────────────────────────────────────────────────

    float process (float x) noexcept
    {
        const double y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return (float) y;
    }

    void reset() noexcept { s1 = s2 = 0.0; }

private:
    static constexpr double k2Pi = 6.283185307179586;

    // Normalised coefficients (divided by a0 at construction time)
    double b0 { 1.0 }, b1 { 0.0 }, b2 { 0.0 };
    double                   a1 { 0.0 }, a2 { 0.0 };

    // TDF-II delay state
    double s1 { 0.0 }, s2 { 0.0 };

    void setCoeffs (double nb0, double nb1, double nb2,
                    double na0, double na1, double na2) noexcept
    {
        const double inv = 1.0 / na0;
        b0 = nb0 * inv;  b1 = nb1 * inv;  b2 = nb2 * inv;
        a1 = na1 * inv;  a2 = na2 * inv;
    }
};
