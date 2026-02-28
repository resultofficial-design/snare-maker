#include "EnvelopeEditor.h"

// =============================================================================
// Constructor / Destructor
// =============================================================================

EnvelopeEditor::EnvelopeEditor()
{
    setOpaque (false);
    waveformBuffer.reserve (kWaveformSamples);
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
                                           juce::SpinLock& envLock)
{
    envelope                = &env;
    pitchEnvelopeForPreview = &env;
    envelopeLock            = &envLock;

    // Waveform preview params (read-only)
    pBodyFreq      = apvts.getRawParameterValue ("bodyFreq");
    pPitchAmount   = apvts.getRawParameterValue ("pitchAmount");
    pPitchDecay    = apvts.getRawParameterValue ("pitchDecay");
    pNoiseLevel    = apvts.getRawParameterValue ("noiseLevel");
    pNoiseDecay    = apvts.getRawParameterValue ("noiseDecay");
    pPhaseOffset   = apvts.getRawParameterValue ("phaseOffset");

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

    if (needsRepaint)
        repaint();
}

// =============================================================================
// resized
// =============================================================================

void EnvelopeEditor::resized()
{
    // Layout is computed from bounds in paint() / toPixel()
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

    // Duration: longest of body or noise decay x 5, clamped
    const double longestTau = std::max (bodyTauSec, noiseTauSec);
    waveformDuration = (float) juce::jlimit (0.05, 1.0, longestTau * 5.0);

    const double dt = (double) waveformDuration / (double) kWaveformSamples;

    waveformBuffer.resize (kWaveformSamples);

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

        // Body: pitch envelope (always from pitchEnvelope) + sine + amplitude decay
        const double tNorm   = (envDurSec > 0.0) ? t / envDurSec : 1.0;
        const double pitchEnv = (pitchEnvelopeForPreview != nullptr)
                                ? pitchEnvelopeForPreview->evaluate (tNorm)
                                : 0.0;

        const double currentFreq = (double) bodyFreq * (1.0 + (freqRatio - 1.0) * pitchEnv);
        phase += currentFreq * dt;

        const double bodyAmp = std::exp (-t / bodyTauSec);
        const double bodySample = std::sin (juce::MathConstants<double>::twoPi * phase) * bodyAmp;

        // Noise: deterministic white noise with exponential decay
        const double noiseAmp = (double) noiseLevel * std::exp (-t / noiseTauSec);
        const double noiseSample = (double) detNoise (i) * noiseAmp;

        waveformBuffer[(size_t) i] = (float) (bodySample + noiseSample);
    }
}

// =============================================================================
// paintWaveform  –  per-pixel min/max filled waveform (zero-allocation)
// =============================================================================

void EnvelopeEditor::paintWaveform (juce::Graphics& g,
                                     float plotL, float plotR,
                                     float plotT, float plotB) const
{
    const int n = (int) waveformBuffer.size();
    if (n < 2)
        return;

    const float plotW  = plotR - plotL;
    const float plotCY = (plotT + plotB) * 0.5f;
    const float halfH  = (plotB - plotT) * 0.40f;
    const int   cols   = std::max (1, (int) plotW);

    g.setColour (juce::Colour (kAccentColour).withAlpha (0.07f));

    for (int c = 0; c < cols; ++c)
    {
        const int s0 = c * n / cols;
        const int s1 = std::min (n - 1, (c + 1) * n / cols);

        float lo = waveformBuffer[(size_t) s0];
        float hi = lo;

        for (int s = s0 + 1; s <= s1; ++s)
        {
            const float v = waveformBuffer[(size_t) s];
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }

        const float yTop = plotCY - hi * halfH;
        const float yBot = plotCY - lo * halfH;

        if (yBot - yTop > 0.5f)
            g.fillRect (plotL + (float) c, yTop, 1.0f, yBot - yTop);
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
    constexpr float kCornerRadius = 10.0f;
    const auto bounds = getLocalBounds().toFloat();
    juce::Path clipPath;
    clipPath.addRoundedRectangle (bounds, kCornerRadius);
    g.reduceClipRegion (clipPath);

    g.setColour (juce::Colour (kBgColour));
    g.fillRoundedRectangle (bounds, kCornerRadius);

    // ── 2. Plot area frame (subtle border) ───────────────────────────────────
    g.setColour (juce::Colour (0xff2A3038));
    g.drawRoundedRectangle (plotL - 1.0f, plotT - 1.0f,
                            plotR - plotL + 2.0f, plotB - plotT + 2.0f,
                            10.0f, 1.0f);

    // ── 3. Grid lines ────────────────────────────────────────────────────────

    // Boundary lines (slightly brighter)
    g.setColour (juce::Colour (0xff2A3038));
    g.drawHorizontalLine ((int) plotT, plotL, plotR);
    g.drawHorizontalLine ((int) plotB, plotL, plotR);
    g.drawVerticalLine   ((int) plotL, plotT, plotB);
    g.drawVerticalLine   ((int) plotR, plotT, plotB);

    // Interior grid (dimmer)
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
        constexpr float dotR = 4.0f;

        // Glow
        g.setColour (accent.withAlpha (0.15f));
        g.fillEllipse (markerX - dotR * 1.8f, markerY - dotR * 1.8f,
                        dotR * 3.6f, dotR * 3.6f);

        // Fill
        g.setColour (accent.withAlpha (0.90f));
        g.fillEllipse (markerX - dotR, markerY - dotR,
                        dotR * 2.0f, dotR * 2.0f);

        // Border
        g.setColour (juce::Colours::white.withAlpha (0.80f));
        g.drawEllipse (markerX - dotR + 0.5f, markerY - dotR + 0.5f,
                        (dotR - 0.5f) * 2.0f, (dotR - 0.5f) * 2.0f, 1.0f);
    }

    // ── 5. Envelope curve ────────────────────────────────────────────────────
    juce::Path curvePath = buildCurvePath();

    if (! curvePath.isEmpty())
    {
        // ── 6. Filled area (rich gradient under curve) ───────────────────────
        {
            juce::Path fillPath (curvePath);
            const auto& lastPt  = envelope->points.back();
            const auto& firstPt = envelope->points.front();
            const auto lastPx   = toPixel ({ lastPt.time,  lastPt.value });
            const auto firstPx  = toPixel ({ firstPt.time, firstPt.value });
            fillPath.lineTo (lastPx.x,  plotB);
            fillPath.lineTo (firstPx.x, plotB);
            fillPath.closeSubPath();

            g.setColour (accent.withAlpha (0.10f));
            g.fillPath (fillPath);
        }

        // ── 7. Soft glow around curve (3 concentric layers) ─────────────────
        {
            const juce::PathStrokeType glowOuter (14.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            const juce::PathStrokeType glowMid (7.0f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
            const juce::PathStrokeType glowInner (3.5f,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

            g.setColour (accent.withAlpha (0.04f));
            g.strokePath (curvePath, glowOuter);

            g.setColour (accent.withAlpha (0.08f));
            g.strokePath (curvePath, glowMid);

            g.setColour (accent.withAlpha (0.16f));
            g.strokePath (curvePath, glowInner);
        }

        // ── 8. Main curve stroke (thick, bright) ────────────────────────────
        g.setColour (accent.withAlpha (0.92f));
        g.strokePath (curvePath, juce::PathStrokeType (2.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── 9. Nodes ─────────────────────────────────────────────────────────────
    const int numPts = envelope->getNumPoints();
    for (int i = 0; i < numPts; ++i)
    {
        const auto& pt = envelope->points[(size_t) i];
        const auto px = toPixel ({ pt.time, pt.value });
        const bool isDragged = (i == dragIndex);
        const bool isHovered = (i == hoveredIndex);
        const float r = (isDragged || isHovered) ? kHoverRadius : kNodeRadius;

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (px.x - r + 1.5f, px.y - r + 2.0f,
                       r * 2.0f, r * 2.0f);

        // Glow halo on hover/drag
        if (isDragged || isHovered)
        {
            g.setColour (accent.withAlpha (isDragged ? 0.35f : 0.20f));
            g.fillEllipse (px.x - r * 1.8f, px.y - r * 1.8f,
                           r * 3.6f, r * 3.6f);
        }

        // Fill: accent when dragging, white otherwise
        g.setColour (isDragged ? accent : juce::Colour (kNodeFill));
        g.fillEllipse (px.x - r, px.y - r, r * 2.0f, r * 2.0f);

        // Border ring
        g.setColour (isDragged ? juce::Colours::white : accent);
        g.drawEllipse (px.x - r + 1.0f, px.y - r + 1.0f,
                       (r - 1.0f) * 2.0f, (r - 1.0f) * 2.0f,
                       1.5f);
    }
}
