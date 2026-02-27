#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

// =============================================================================
// EnvelopePoint  --  a single node in a flexible envelope
//
//   time   normalised X position [0..1]
//   value  normalised Y position [0..1]
//   curve  per-segment curvature for the segment STARTING at this point:
//          [-1..+1] normalised range
//          +1  maximum concave (fast initial change, slow tail)
//          -1  maximum convex  (slow start, fast end)
//           0  linear
// =============================================================================

struct EnvelopePoint
{
    float time  = 0.0f;
    float value = 0.0f;
    float curve = 0.0f;   // normalised [-1..+1]
};

// =============================================================================
// FlexibleEnvelope  --  variable-length piecewise envelope with per-segment
//                        exponential curvature
//
// Two usage modes:
//
//   1. Stateless lookup (UI / waveform preview):
//        evaluate(tNorm)     -- t in [0,1], returns envelope value
//
//   2. Stateful per-sample (DSP / processBlock):
//        prepare(sr)         -- store sample rate (once, from prepareToPlay)
//        setDuration(ms)     -- set total envelope time (once per block)
//        noteOn()            -- reset to start on MIDI note-on
//        getNextSample()     -- advance 1 sample, return value
//        noteOff()           -- no-op for percussive envelopes
//        reset()             -- hard stop (killAll / prepareToPlay)
//
// Per-segment curve shaping:
//   The normalised curve value [-1..+1] is scaled by kMaxCurvature to produce
//   an exponential curvature constant k.  The shaping function maps linear
//   segment progress t in [0,1] to shaped progress:
//     shapeCurve(t, curve) = (1 - exp(-k*t)) / (1 - exp(-k))   where k != 0
//     shapeCurve(t, 0)     = t                                  (linear)
//
// Audio-thread safety: pre-allocate with reserve() in the constructor, then
// only mutate existing elements per-block (no push_back / resize).
// =============================================================================

class FlexibleEnvelope
{
public:
    std::vector<EnvelopePoint> points;

    // Maximum internal curvature constant.
    // curve = +/-1.0 maps to k = +/-kMaxCurvature.
    static constexpr double kMaxCurvature = 5.0;

    // Default constructor: 3-point pitch sweep (useful for snare body).
    FlexibleEnvelope()
        : points ({ { 0.0f, 0.0f, 0.6f },
                    { 0.1f, 1.0f, 0.6f },
                    { 1.0f, 0.0f, 0.0f } }) {}

    explicit FlexibleEnvelope (std::vector<EnvelopePoint> pts)
        : points (std::move (pts)) {}

    int getNumPoints() const noexcept { return (int) points.size(); }

    // ── Stateful per-sample interface (DSP) ─────────────────────────────────

    // Store sample rate.  Call once from prepareToPlay().
    void prepare (double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
    }

    // Set total envelope duration in milliseconds.
    // Safe to call per-block; does not reset the current position.
    void setDuration (double durationMs) noexcept
    {
        durationSamples_ = std::max (1.0, durationMs * 0.001 * sampleRate_);
    }

    // Reset envelope to the start and begin running.  Call on note-on.
    void noteOn () noexcept
    {
        currentSample_ = 0.0;
        running_       = true;
    }

    // Percussive envelope: note-off is a no-op.
    // The envelope runs to natural completion regardless of note-off.
    void noteOff () noexcept {}

    // Hard-stop the envelope immediately.  Call from resetVoiceState / killAll.
    void reset () noexcept
    {
        currentSample_ = 0.0;
        running_       = false;
    }

    // Advance by one sample and return the current envelope value.
    // When the envelope finishes (tNorm >= 1), running_ becomes false and
    // subsequent calls return the last point's value (typically 0).
    double getNextSample () noexcept
    {
        if (!running_)
        {
            const int n = (int) points.size();
            return (n > 0) ? (double) points[(size_t) (n - 1)].value : 0.0;
        }

        const double tNorm = currentSample_ / durationSamples_;
        currentSample_ += 1.0;

        if (currentSample_ >= durationSamples_)
            running_ = false;

        return evaluate (tNorm);
    }

    // True while the envelope is running (between noteOn and completion).
    bool isActive () const noexcept { return running_; }

    // ── Evaluate at normalised time t in [0, 1] ────────────────────────────
    // Piecewise interpolation through all points.  Each segment uses the
    // normalised curvature stored in the starting point.
    double evaluate (double tNorm) const noexcept
    {
        const int n = (int) points.size();
        if (n == 0) return 0.0;
        if (n == 1) return (double) points[0].value;

        if (tNorm <= (double) points[0].time)
            return (double) points[0].value;
        if (tNorm >= (double) points[(size_t) (n - 1)].time)
            return (double) points[(size_t) (n - 1)].value;

        for (int i = 0; i < n - 1; ++i)
        {
            const auto& a = points[(size_t) i];
            const auto& b = points[(size_t) (i + 1)];

            if (tNorm < (double) b.time || i == n - 2)
            {
                const double span = std::max ((double) b.time - (double) a.time, 0.001);
                const double segT = (tNorm - (double) a.time) / span;
                return (double) a.value
                     + ((double) b.value - (double) a.value)
                       * shapeCurve (segT, (double) a.curve);
            }
        }

        return (double) points[(size_t) (n - 1)].value;
    }

    // ── Per-segment curve shaping (double precision, for DSP) ───────────────
    // curve in [-1..+1]:  +1 = max concave,  -1 = max convex,  0 = linear
    static double shapeCurve (double t, double curve) noexcept
    {
        const double k = curve * kMaxCurvature;
        if (std::abs (k) < 0.01)
            return t;
        return (1.0 - std::exp (-k * t)) / (1.0 - std::exp (-k));
    }

    // ── Per-segment curve shaping (float precision, for UI rendering) ───────
    static float shapeCurveF (float t, float curve) noexcept
    {
        const float k = curve * (float) kMaxCurvature;
        if (std::abs (k) < 0.01f)
            return t;
        return (1.0f - std::exp (-k * t)) / (1.0f - std::exp (-k));
    }

    // ── Dynamic point management ────────────────────────────────────────────

    // Insert a new point sorted by time.  Returns the index of the new point.
    int addPoint (float time, float value, float curve = 0.0f)
    {
        EnvelopePoint pt { time, value, curve };

        // Find insertion position (keep sorted by time)
        auto it = std::lower_bound (points.begin(), points.end(), pt,
            [] (const EnvelopePoint& a, const EnvelopePoint& b)
            { return a.time < b.time; });

        auto inserted = points.insert (it, pt);
        return (int) std::distance (points.begin(), inserted);
    }

    // Remove a point by index.  Refuses to remove the first or last point.
    // Returns true if the point was removed.
    bool removePoint (int index)
    {
        const int n = (int) points.size();
        if (n <= 2 || index <= 0 || index >= n - 1)
            return false;

        points.erase (points.begin() + index);
        return true;
    }

private:
    // ── Stateful per-sample runtime (audio thread only) ─────────────────────
    double sampleRate_       = 44100.0;
    double durationSamples_  = 1.0;
    double currentSample_    = 0.0;
    bool   running_          = false;
};
