#include "EnvelopeEditor.h"

// =============================================================================
// Constructor / Destructor
// =============================================================================

EnvelopeEditor::EnvelopeEditor()
{
    setOpaque (false);
    for (int i = 0; i < kNumLayers; ++i)
        layerBuffers[i].reserve (kWaveformSamples);
    addAndMakeVisible (modeToggle);
}

EnvelopeEditor::~EnvelopeEditor()
{
    stopTimer();
}

// =============================================================================
// connectToParameters  –  bind to FlexibleEnvelope + waveform preview params
// =============================================================================

void EnvelopeEditor::connectToParameters (juce::AudioProcessorValueTreeState& apvts,
                                           FlexibleEnvelope& env,
                                           juce::SpinLock& envLock,
                                           std::atomic<int>& displayMode)
{
    envelope                = &env;
    pitchEnvelopeForPreview = &env;
    envelopeLock            = &envLock;
    globalDisplayMode       = &displayMode;

    // Waveform preview params (read-only)
    pBodyFreq      = apvts.getRawParameterValue ("bodyFreq");
    pPitchAmount   = apvts.getRawParameterValue ("pitchAmount");
    pPitchDecay    = apvts.getRawParameterValue ("pitchDecay");
    pNoiseLevel    = apvts.getRawParameterValue ("noiseLevel");
    pNoiseDecay    = apvts.getRawParameterValue ("noiseDecay");
    pPhaseOffset   = apvts.getRawParameterValue ("phaseOffset");

    // Wire SIMPLE/TRUE toggle to global display mode
    modeToggle.setMode (displayMode.load() == 0
                        ? EnvelopeModeToggle::DisplayMode::Simple
                        : EnvelopeModeToggle::DisplayMode::True);

    modeToggle.onChange = [this] (EnvelopeModeToggle::DisplayMode m)
    {
        if (globalDisplayMode != nullptr)
            globalDisplayMode->store (m == EnvelopeModeToggle::DisplayMode::True ? 1 : 0);
        repaint();
    };

    regenerateWaveform();
    startTimerHz (30);
}

// =============================================================================
// setEnvelope  –  switch which envelope the editor displays / edits
// =============================================================================

void EnvelopeEditor::setEnvelope (FlexibleEnvelope& env)
{
    envelope     = &env;
    dragIndex    = -1;
    hoveredIndex = -1;
    lastPointsHash = 0;   // force waveform regeneration
    regenerateWaveform();
    repaint();
}

// =============================================================================
// setActiveLayer  –  highlight one waveform layer, dim the rest
// =============================================================================

void EnvelopeEditor::setActiveLayer (WaveLayer layer)
{
    if (activeLayer != layer)
    {
        activeLayer = layer;
        repaint();
    }
}

// =============================================================================
// setLayerSampleData  –  replace a layer with external sample data
// =============================================================================

void EnvelopeEditor::setLayerSampleData (WaveLayer layer, const float* data, int numSamples)
{
    const int idx = static_cast<int> (layer);
    if (idx < 0 || idx >= kNumLayers || data == nullptr || numSamples < 1)
        return;

    // Resample to kWaveformSamples via linear interpolation
    layerBuffers[idx].resize (kWaveformSamples);

    for (int i = 0; i < kWaveformSamples; ++i)
    {
        const float srcPos = (float) i / (float) (kWaveformSamples - 1)
                             * (float) (numSamples - 1);
        const int s0 = (int) srcPos;
        const int s1 = std::min (s0 + 1, numSamples - 1);
        const float frac = srcPos - (float) s0;
        layerBuffers[idx][(size_t) i] = data[s0] + (data[s1] - data[s0]) * frac;
    }

    // Rebuild TRUE (filled) path using the same logic as regenerateWaveform
    const float plotL  = kPadX;
    const float plotR  = (float) getWidth() - kPadX;
    const float plotT  = kPadTop;
    const float plotB  = (float) getHeight() - kPadBottom;
    const float plotW  = plotR - plotL;
    const float plotCY = (plotT + plotB) * 0.5f;
    const float halfH  = (plotB - plotT) * 0.40f;
    const int   cols   = std::max (1, (int) plotW);

    layerPaths[idx].clear();
    if (cols >= 2 && plotW >= 1.0f)
    {
        const auto& buf = layerBuffers[idx];
        const int n = (int) buf.size();

        std::vector<float> topY ((size_t) cols), botY ((size_t) cols);
        for (int c = 0; c < cols; ++c)
        {
            const int s0 = c * n / cols;
            const int s1 = std::min (n - 1, (c + 1) * n / cols);
            float lo = buf[(size_t) s0], hi = lo;
            for (int s = s0 + 1; s <= s1; ++s)
            {
                const float v = buf[(size_t) s];
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }
            topY[(size_t) c] = plotCY - hi * halfH;
            botY[(size_t) c] = plotCY - lo * halfH;
        }

        layerPaths[idx].startNewSubPath (plotL, topY[0]);
        for (int c = 1; c < cols; ++c)
            layerPaths[idx].lineTo (plotL + (float) c, topY[(size_t) c]);
        for (int c = cols - 1; c >= 0; --c)
            layerPaths[idx].lineTo (plotL + (float) c, botY[(size_t) c]);
        layerPaths[idx].closeSubPath();
    }

    // Rebuild SIMPLE path
    constexpr int kSimplePoints = 256;
    simplePaths[idx].clear();
    if (plotW >= 1.0f)
    {
        const auto& buf = layerBuffers[idx];
        const int n = (int) buf.size();
        const int numPts = std::min (kSimplePoints, n);

        std::vector<float> samples ((size_t) numPts);
        for (int i = 0; i < numPts; ++i)
            samples[(size_t) i] = buf[(size_t) (i * (n - 1) / (numPts - 1))];

        auto smooth3 = [] (std::vector<float>& arr)
        {
            const int sz = (int) arr.size();
            if (sz < 3) return;
            float prev = arr[0];
            for (int i = 1; i < sz - 1; ++i)
            {
                const float cur = arr[(size_t) i];
                arr[(size_t) i] = (prev + cur + arr[(size_t) (i + 1)]) / 3.0f;
                prev = cur;
            }
        };
        smooth3 (samples);
        smooth3 (samples);

        const float step = plotW / (float) (numPts - 1);
        simplePaths[idx].startNewSubPath (plotL, plotCY - samples[0] * halfH);
        for (int i = 1; i < numPts; ++i)
            simplePaths[idx].lineTo (plotL + (float) i * step,
                                     plotCY - samples[(size_t) i] * halfH);
    }

    repaint();
}

// =============================================================================
// computePointsHash  –  FNV-1a over all points for change detection
// =============================================================================

uint64_t EnvelopeEditor::computePointsHash() const noexcept
{
    uint64_t hash = 14695981039346656037ULL;  // FNV offset basis

    if (envelope == nullptr)
        return hash;

    // Hash point count so add/remove is always detected
    hash ^= (uint64_t) envelope->points.size();
    hash *= 1099511628211ULL;

    for (const auto& pt : envelope->points)
    {
        uint32_t bits;
        std::memcpy (&bits, &pt.time, sizeof (uint32_t));
        hash ^= (uint64_t) bits;
        hash *= 1099511628211ULL;  // FNV prime

        std::memcpy (&bits, &pt.value, sizeof (uint32_t));
        hash ^= (uint64_t) bits;
        hash *= 1099511628211ULL;

        std::memcpy (&bits, &pt.curve, sizeof (uint32_t));
        hash ^= (uint64_t) bits;
        hash *= 1099511628211ULL;
    }

    return hash;
}

// =============================================================================
// timerCallback  –  sync bound nodes from parameter values (~30 Hz)
// =============================================================================

void EnvelopeEditor::timerCallback()
{
    if (envelope == nullptr)
        return;

    bool needsRepaint = false;

    // ── Regenerate waveform if any relevant parameter changed ────────────────
    if (pBodyFreq != nullptr)
    {
        const float bf  = pBodyFreq->load();
        const float pa  = pPitchAmount->load();
        const float pd  = pPitchDecay->load();
        const float nl  = (pNoiseLevel  != nullptr) ? pNoiseLevel->load()  : 0.0f;
        const float nd  = (pNoiseDecay  != nullptr) ? pNoiseDecay->load()  : 150.0f;
        const float po  = (pPhaseOffset != nullptr) ? pPhaseOffset->load() : 0.0f;

        const uint64_t currentHash = computePointsHash();

        if (std::abs (bf - lastWfBodyFreq)    > 0.1f   ||
            std::abs (pa - lastWfPitchAmount) > 0.05f  ||
            std::abs (pd - lastWfPitchDecay)  > 0.1f   ||
            std::abs (nl - lastWfNoiseLevel)  > 0.005f ||
            std::abs (nd - lastWfNoiseDecay)  > 0.1f   ||
            std::abs (po - lastWfPhaseOffset) > 0.1f   ||
            currentHash != lastPointsHash)
        {
            regenerateWaveform();
            needsRepaint = true;
        }
    }

    // ── Sync SIMPLE/TRUE toggle from global state ──────────────────────────
    if (globalDisplayMode != nullptr)
    {
        const auto expected = (globalDisplayMode->load() == 0)
                              ? EnvelopeModeToggle::DisplayMode::Simple
                              : EnvelopeModeToggle::DisplayMode::True;
        if (modeToggle.getMode() != expected)
        {
            modeToggle.setMode (expected);
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

// =============================================================================
// resized
// =============================================================================

void EnvelopeEditor::resized()
{
    constexpr int kUISpacing = 16;
    constexpr int toggleW = 110;
    constexpr int toggleH = 22;
    const int x = getWidth() - kUISpacing - toggleW;
    const int y = kUISpacing;
    modeToggle.setBounds (x, y, toggleW, toggleH);
}

// =============================================================================
// Coordinate mapping
// =============================================================================

juce::Point<float> EnvelopeEditor::toPixel (juce::Point<float> norm) const noexcept
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    const float plotL = kPadX;
    const float plotR = w - kPadX;
    const float plotT = kPadTop;
    const float plotB = h - kPadBottom;

    return { plotL + norm.x * (plotR - plotL),
             plotB - norm.y * (plotB - plotT) };
}

juce::Point<float> EnvelopeEditor::fromPixel (juce::Point<float> px) const noexcept
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    const float plotL = kPadX;
    const float plotR = w - kPadX;
    const float plotT = kPadTop;
    const float plotB = h - kPadBottom;

    const float rangeX = plotR - plotL;
    const float rangeY = plotB - plotT;

    float nx = (rangeX > 0.0f) ? (px.x - plotL) / rangeX : 0.0f;
    float ny = (rangeY > 0.0f) ? (plotB - px.y) / rangeY : 0.0f;

    return { juce::jlimit (0.0f, 1.0f, nx),
             juce::jlimit (0.0f, 1.0f, ny) };
}

// =============================================================================
// hitTestNode
// =============================================================================

int EnvelopeEditor::hitTestNode (juce::Point<float> px) const noexcept
{
    int   bestIdx  = -1;
    float bestDist = kHitRadius * kHitRadius;
    const int n = envelope->getNumPoints();

    for (int i = 0; i < n; ++i)
    {
        const auto& pt = envelope->points[(size_t) i];
        const auto nodePx = toPixel ({ pt.time, pt.value });
        const float dx = px.x - nodePx.x;
        const float dy = px.y - nodePx.y;
        const float d2 = dx * dx + dy * dy;

        if (d2 < bestDist)
        {
            bestDist = d2;
            bestIdx  = i;
        }
    }

    return bestIdx;
}

// =============================================================================
// buildCurvePath  –  per-segment exponential shaping via piecewise quadratic
// =============================================================================

juce::Path EnvelopeEditor::buildCurvePath() const
{
    juce::Path path;
    const int n = envelope->getNumPoints();

    if (n < 2)
        return path;

    const auto& pts = envelope->points;
    path.startNewSubPath (toPixel ({ pts[0].time, pts[0].value }));

    for (int seg = 0; seg < n - 1; ++seg)
    {
        const auto& pa = pts[(size_t) seg];
        const auto& pb = pts[(size_t) (seg + 1)];
        const float k  = pa.curve;

        auto shapedPixel = [&] (float t) -> juce::Point<float>
        {
            return toPixel ({ pa.time + (pb.time - pa.time) * t,
                              pa.value + (pb.value - pa.value)
                                         * FlexibleEnvelope::shapeCurveF (t, k) });
        };

        for (int sub = 0; sub < kSubSegments; ++sub)
        {
            const float ta = (float) sub       / (float) kSubSegments;
            const float tb = (float) (sub + 1) / (float) kSubSegments;
            const float tm = (ta + tb) * 0.5f;

            const auto p0  = shapedPixel (ta);
            const auto p2  = shapedPixel (tb);
            const auto mid = shapedPixel (tm);

            juce::Point<float> cp {
                2.0f * mid.x - 0.5f * (p0.x + p2.x),
                2.0f * mid.y - 0.5f * (p0.y + p2.y)
            };

            path.quadraticTo (cp, p2);
        }
    }

    return path;
}

// =============================================================================
// regenerateWaveform  –  lightweight mini-synth (runs on UI thread, ~2048 ops)
// =============================================================================

void EnvelopeEditor::regenerateWaveform()
{
    if (pBodyFreq == nullptr || pPitchAmount == nullptr || pPitchDecay == nullptr)
        return;

    const float bodyFreq     = pBodyFreq->load();
    const float pitchAmount  = pPitchAmount->load();
    const float pitchDecayMs = pPitchDecay->load();
    const float noiseLevel   = (pNoiseLevel  != nullptr) ? pNoiseLevel->load()  : 0.0f;
    const float noiseDecayMs = (pNoiseDecay  != nullptr) ? pNoiseDecay->load()  : 150.0f;
    const float phaseOffDeg  = (pPhaseOffset != nullptr) ? pPhaseOffset->load() : 0.0f;

    // Store for change detection
    lastWfBodyFreq    = bodyFreq;
    lastWfPitchAmount = pitchAmount;
    lastWfPitchDecay  = pitchDecayMs;
    lastWfNoiseLevel  = noiseLevel;
    lastWfNoiseDecay  = noiseDecayMs;
    lastWfPhaseOffset = phaseOffDeg;
    lastPointsHash    = computePointsHash();

    const double envDurSec    = std::max (0.001, (double) pitchDecayMs * 0.001);
    const double bodyTauSec   = envDurSec * 2.0;
    const double noiseTauSec  = std::max (0.001, (double) noiseDecayMs * 0.001);
    const double freqRatio    = std::pow (2.0, (double) pitchAmount / 12.0);

    // Transient: very fast impulse decay
    const double transTauSec  = envDurSec * 0.05;
    // Resonant: medium decay, sits between body and noise
    const double resTauSec    = bodyTauSec * 0.5;

    // Duration: longest of body or noise decay x 5, clamped
    const double longestTau = std::max (bodyTauSec, noiseTauSec);
    waveformDuration = (float) juce::jlimit (0.05, 1.0, longestTau * 5.0);

    const double dt = (double) waveformDuration / (double) kWaveformSamples;

    for (int i = 0; i < kNumLayers; ++i)
        layerBuffers[i].resize (kWaveformSamples);

    // Deterministic pseudo-random noise: integer hash -> float in [-1, +1].
    auto detNoise = [] (int idx) -> float
    {
        auto x = (uint32_t) idx;
        x  = ((x >> 16) ^ x) * 0x45d9f3bu;
        x  = ((x >> 16) ^ x) * 0x45d9f3bu;
        x  = (x >> 16) ^ x;
        return (float) (x & 0x7FFFFFFFu) / (float) 0x7FFFFFFFu * 2.0f - 1.0f;
    };

    // Start phase matches engine: phase = phaseOffsetDeg / 360
    double phase = (double) phaseOffDeg / 360.0;

    for (int i = 0; i < kWaveformSamples; ++i)
    {
        const double t = (double) i * dt;

        // Pitch envelope (always from pitchEnvelope) for frequency modulation
        const double tNorm   = (envDurSec > 0.0) ? t / envDurSec : 1.0;
        const double pitchEnv = (pitchEnvelopeForPreview != nullptr)
                                ? pitchEnvelopeForPreview->evaluate (tNorm)
                                : 0.0;

        const double currentFreq = (double) bodyFreq * (1.0 + (freqRatio - 1.0) * pitchEnv);
        phase += currentFreq * dt;

        const double sinVal = std::sin (juce::MathConstants<double>::twoPi * phase);

        // ── Transient: sine at initial frequency × very fast decay ──────────
        const double transAmp = std::exp (-t / transTauSec);
        layerBuffers[static_cast<int> (WaveLayer::Transient)][(size_t) i]
            = (float) (sinVal * transAmp);

        // ── Body: sine with pitch envelope × body decay ─────────────────────
        const double bodyAmp = std::exp (-t / bodyTauSec);
        layerBuffers[static_cast<int> (WaveLayer::Body)][(size_t) i]
            = (float) (sinVal * bodyAmp);

        // ── Resonant: sine at body frequency × medium decay ─────────────────
        const double resAmp = std::exp (-t / resTauSec);
        layerBuffers[static_cast<int> (WaveLayer::Resonant)][(size_t) i]
            = (float) (sinVal * resAmp);

        // ── Noise: deterministic white noise × exponential decay ────────────
        const double noiseAmp = (double) noiseLevel * std::exp (-t / noiseTauSec);
        layerBuffers[static_cast<int> (WaveLayer::Noise)][(size_t) i]
            = (float) ((double) detNoise (i) * noiseAmp);
    }

    // ── Build closed juce::Path per layer for filled rendering ──────────────
    const float plotL  = kPadX;
    const float plotR  = (float) getWidth() - kPadX;
    const float plotT  = kPadTop;
    const float plotB  = (float) getHeight() - kPadBottom;
    const float plotW  = plotR - plotL;
    const float plotCY = (plotT + plotB) * 0.5f;
    const float halfH  = (plotB - plotT) * 0.40f;
    const int   cols   = std::max (1, (int) plotW);

    for (int layer = 0; layer < kNumLayers; ++layer)
    {
        layerPaths[layer].clear();

        if (cols < 2 || plotW < 1.0f)
            continue;

        const auto& buf = layerBuffers[layer];
        const int n     = (int) buf.size();
        if (n < 2) continue;

        // Build top edge (max values) left to right
        std::vector<float> topY (cols), botY (cols);

        for (int c = 0; c < cols; ++c)
        {
            const int s0 = c * n / cols;
            const int s1 = std::min (n - 1, (c + 1) * n / cols);

            float lo = buf[(size_t) s0];
            float hi = lo;

            for (int s = s0 + 1; s <= s1; ++s)
            {
                const float v = buf[(size_t) s];
                if (v < lo) lo = v;
                if (v > hi) hi = v;
            }

            topY[(size_t) c] = plotCY - hi * halfH;
            botY[(size_t) c] = plotCY - lo * halfH;
        }

        // Top edge L→R
        layerPaths[layer].startNewSubPath (plotL, topY[0]);
        for (int c = 1; c < cols; ++c)
            layerPaths[layer].lineTo (plotL + (float) c, topY[(size_t) c]);

        // Bottom edge R→L
        for (int c = cols - 1; c >= 0; --c)
            layerPaths[layer].lineTo (plotL + (float) c, botY[(size_t) c]);

        layerPaths[layer].closeSubPath();
    }

    // ── Build SIMPLE paths (downsampled waveform line, not filled) ─────────
    // Take every Nth sample, apply light smoothing, build a line path that
    // preserves oscillation cycles without micro-detail.
    constexpr int kSimplePoints = 256;   // enough points for clear oscillations

    for (int layer = 0; layer < kNumLayers; ++layer)
    {
        simplePaths[layer].clear();

        if (plotW < 1.0f)
            continue;

        const auto& buf = layerBuffers[layer];
        const int n     = (int) buf.size();
        if (n < 2) continue;

        const int numPts = std::min (kSimplePoints, n);

        // Downsample: pick evenly spaced samples
        std::vector<float> samples (numPts);
        for (int i = 0; i < numPts; ++i)
        {
            const int srcIdx = i * (n - 1) / (numPts - 1);
            samples[(size_t) i] = buf[(size_t) srcIdx];
        }

        // Two passes of 3-tap moving average for gentle smoothing
        auto smooth3 = [] (std::vector<float>& arr)
        {
            const int sz = (int) arr.size();
            if (sz < 3) return;
            float prev = arr[0];
            for (int i = 1; i < sz - 1; ++i)
            {
                const float cur = arr[(size_t) i];
                arr[(size_t) i] = (prev + cur + arr[(size_t) (i + 1)]) / 3.0f;
                prev = cur;
            }
        };

        smooth3 (samples);
        smooth3 (samples);

        // Build open line path (not closed / not filled)
        const float step = plotW / (float) (numPts - 1);
        simplePaths[layer].startNewSubPath (plotL, plotCY - samples[0] * halfH);
        for (int i = 1; i < numPts; ++i)
            simplePaths[layer].lineTo (plotL + (float) i * step,
                                       plotCY - samples[(size_t) i] * halfH);
    }
}

// =============================================================================
// paintWaveform  –  per-pixel min/max filled waveform (zero-allocation)
// =============================================================================

void EnvelopeEditor::paintWaveform (juce::Graphics& g,
                                     float /*plotL*/, float /*plotR*/,
                                     float /*plotT*/, float /*plotB*/) const
{
    const int activeIdx = static_cast<int> (activeLayer);
    const bool isSimple = (globalDisplayMode != nullptr && globalDisplayMode->load() == 0);

    // Draw inactive layers first (dimmed), then active layer on top (brighter)
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 0; i < kNumLayers; ++i)
        {
            const bool isActive = (i == activeIdx);
            if ((pass == 0) == isActive)
                continue;   // pass 0 = inactive only, pass 1 = active only

            const float alpha = isActive ? 1.0f : 0.45f;
            g.setColour (juce::Colour (kLayerColours[i]).withAlpha (alpha));

            if (isSimple)
            {
                // SIMPLE: stroke the downsampled line path (oscillating waveform)
                if (! simplePaths[i].isEmpty())
                    g.strokePath (simplePaths[i],
                                  juce::PathStrokeType (1.5f,
                                      juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
            }
            else
            {
                // TRUE: fill the min/max closed path (full detail)
                if (! layerPaths[i].isEmpty())
                    g.fillPath (layerPaths[i]);
            }
        }
    }
}

// =============================================================================
// Mouse interaction
// =============================================================================

void EnvelopeEditor::mouseDown (const juce::MouseEvent& e)
{
    if (envelope == nullptr)
        return;

    const int hit = hitTestNode (e.position);

    // Right-click: remove interior point
    if (e.mods.isRightButtonDown() && hit > 0
        && hit < envelope->getNumPoints() - 1)
    {
        if (envelopeLock != nullptr)
        {
            juce::SpinLock::ScopedLockType lock (*envelopeLock);
            envelope->removePoint (hit);
        }
        else
        {
            envelope->removePoint (hit);
        }

        dragIndex    = -1;
        hoveredIndex = -1;
        regenerateWaveform();
        repaint();
        return;
    }

    dragIndex = hit;
    if (dragIndex >= 0)
        repaint();
}

void EnvelopeEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIndex < 0)
        return;

    auto norm = fromPixel (e.position);
    const int lastIdx = envelope->getNumPoints() - 1;

    // Pin endpoints: first X=0, last X=1; Y is free for all nodes
    if (dragIndex == 0)
    {
        norm.x = 0.0f;
    }
    else if (dragIndex == lastIdx)
    {
        norm.x = 1.0f;
    }
    else
    {
        // Enforce left-to-right X ordering with a small margin
        const float minX = envelope->points[(size_t) (dragIndex - 1)].time  + kXOrderMargin;
        const float maxX = envelope->points[(size_t) (dragIndex + 1)].time  - kXOrderMargin;
        norm.x = juce::jlimit (minX, maxX, norm.x);
    }

    envelope->points[(size_t) dragIndex].time  = norm.x;
    envelope->points[(size_t) dragIndex].value = norm.y;

    regenerateWaveform();
    repaint();
}

void EnvelopeEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragIndex >= 0)
    {
        dragIndex = -1;
        repaint();
    }
}

void EnvelopeEditor::mouseMove (const juce::MouseEvent& e)
{
    const int hit = hitTestNode (e.position);

    if (hit != hoveredIndex)
    {
        hoveredIndex = hit;
        setMouseCursor (hit >= 0 ? juce::MouseCursor::DraggingHandCursor
                                 : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EnvelopeEditor::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex >= 0)
    {
        hoveredIndex = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void EnvelopeEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (envelope == nullptr)
        return;

    // Only add a point if double-click was on empty area (not an existing node)
    if (hitTestNode (e.position) >= 0)
        return;

    const auto norm = fromPixel (e.position);

    if (envelopeLock != nullptr)
    {
        juce::SpinLock::ScopedLockType lock (*envelopeLock);
        envelope->addPoint (norm.x, norm.y);
    }
    else
    {
        envelope->addPoint (norm.x, norm.y);
    }

    regenerateWaveform();
    repaint();
}

// =============================================================================
// Drag & drop
// =============================================================================

void EnvelopeEditor::setDragDropEnabled (bool enabled)
{
    dragDropEnabled = enabled;
    if (! enabled)
    {
        fileDragOver = false;
        repaint();
    }
}

bool EnvelopeEditor::isInterestedInFileDrag (const juce::StringArray&)
{
    return dragDropEnabled;
}

void EnvelopeEditor::fileDragEnter (const juce::StringArray&, int, int)
{
    fileDragOver = true;
    repaint();
}

void EnvelopeEditor::fileDragExit (const juce::StringArray&)
{
    fileDragOver = false;
    repaint();
}

void EnvelopeEditor::filesDropped (const juce::StringArray& files, int, int)
{
    fileDragOver = false;

    if (files.size() > 0 && onSampleDropped)
        onSampleDropped (files[0]);

    repaint();
}

// =============================================================================
// paint
// =============================================================================

void EnvelopeEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    const float plotL  = kPadX;
    const float plotR  = w - kPadX;
    const float plotT  = kPadTop;
    const float plotB  = h - kPadBottom;
    const juce::Colour accent (kAccentColour);

    // ── 1. Background (rounded fill + clip) ──────────────────────────────────
    constexpr float kCornerRadius = 8.0f;
    const auto bounds = getLocalBounds().toFloat();
    juce::Path clipPath;
    clipPath.addRoundedRectangle (bounds, kCornerRadius);
    g.reduceClipRegion (clipPath);

    g.setColour (juce::Colour (kBgColour));
    g.fillRoundedRectangle (bounds, kCornerRadius);

    // ── Drag overlay (replaces waveform / envelope while dragging) ───────────
    if (fileDragOver)
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::Font (juce::FontOptions {}.withHeight (16.0f)));
        g.drawText ("Drag here", bounds, juce::Justification::centred);
        return;
    }

    // ── 2. Grid lines (interior only) ─────────────────────────────────────────
    g.setColour (juce::Colour (kGridColour));
    for (int i = 1; i < kGridLinesH; ++i)
    {
        const float y = plotT + (float) i / (float) kGridLinesH * (plotB - plotT);
        g.drawHorizontalLine ((int) y, plotL, plotR);
    }
    for (int i = 1; i < kGridLinesV; ++i)
    {
        const float x = plotL + (float) i / (float) kGridLinesV * (plotR - plotL);
        g.drawVerticalLine ((int) x, plotT, plotB);
    }

    // ── 4. Preview waveform (behind envelope curve) ────────────────────────
    paintWaveform (g, plotL, plotR, plotT, plotB);

    // ── 4b. Phase start marker (vertical line + indicator dot at t = 0) ──
    if (pPhaseOffset != nullptr)
    {
        const float phaseOffDeg = pPhaseOffset->load();
        const float phaseRad    = phaseOffDeg / 360.0f
                                  * juce::MathConstants<float>::twoPi;
        const float initValue   = std::sin (phaseRad);   // waveform value at t=0

        const float halfH   = (plotB - plotT) * 0.40f;   // matches paintWaveform
        const float markerX  = plotL;
        const float markerCY = (plotT + plotB) * 0.5f;
        const float markerY  = markerCY - initValue * halfH;

        // Vertical line (full height, subtle)
        g.setColour (accent.withAlpha (0.18f));
        g.drawVerticalLine ((int) markerX, plotT, plotB);

        // Short horizontal tick from marker to the right (visual anchor)
        g.setColour (accent.withAlpha (0.12f));
        g.drawHorizontalLine ((int) markerY, markerX, markerX + 10.0f);

        // Indicator dot at the initial waveform value
        constexpr float dotR = 3.5f;

        g.setColour (accent.withAlpha (0.90f));
        g.fillEllipse (markerX - dotR, markerY - dotR,
                        dotR * 2.0f, dotR * 2.0f);
    }

    // ── 5. Envelope curve ────────────────────────────────────────────────────
    const juce::Colour curveGrey (0xff8A8F98);
    juce::Path curvePath = buildCurvePath();

    if (! curvePath.isEmpty())
    {
        // ── 6. Filled area (subtle under curve) ─────────────────────────────
        {
            juce::Path fillPath (curvePath);
            const auto& lastPt  = envelope->points.back();
            const auto& firstPt = envelope->points.front();
            const auto lastPx   = toPixel ({ lastPt.time,  lastPt.value });
            const auto firstPx  = toPixel ({ firstPt.time, firstPt.value });
            fillPath.lineTo (lastPx.x,  plotB);
            fillPath.lineTo (firstPx.x, plotB);
            fillPath.closeSubPath();

            g.setColour (curveGrey.withAlpha (0.08f));
            g.fillPath (fillPath);
        }

        // ── 7. Main curve stroke (flat grey, no glow) ──────────────────────
        g.setColour (curveGrey);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── 8. Nodes (simple filled circles, no glow / shadow / ring) ───────────
    const int numPts = envelope->getNumPoints();
    constexpr float kSmallR = 4.5f;
    constexpr float kActiveR = 5.5f;

    for (int i = 0; i < numPts; ++i)
    {
        const auto& pt = envelope->points[(size_t) i];
        const auto px = toPixel ({ pt.time, pt.value });
        const bool isDragged = (i == dragIndex);
        const bool isHovered = (i == hoveredIndex);
        const float r = (isDragged || isHovered) ? kActiveR : kSmallR;

        g.setColour (isDragged ? curveGrey.brighter (0.4f)
                     : isHovered ? juce::Colours::white
                     : curveGrey);
        g.fillEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f);
    }
}
