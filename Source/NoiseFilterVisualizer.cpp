#include "NoiseFilterVisualizer.h"

// =============================================================================
// Constructor
// =============================================================================

NoiseFilterVisualizer::NoiseFilterVisualizer()
{
    // Default node positions (normalised 0..1)
    points[0] = { 0.15f, 0.30f };   // HP
    points[1] = { 0.50f, 0.70f };   // Mid Bell
    points[2] = { 0.85f, 0.30f };   // LP
}

// =============================================================================
// Coordinate helpers
// =============================================================================

juce::Point<float> NoiseFilterVisualizer::toPixel (juce::Point<float> norm) const noexcept
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float plotL = kPadX;
    const float plotR = w - kPadX;
    const float plotT = kPadTop;
    const float plotB = h - kPadBottom;

    return { plotL + norm.x * (plotR - plotL),
             plotB - norm.y * (plotB - plotT) };   // y=1 → top
}

juce::Point<float> NoiseFilterVisualizer::fromPixel (juce::Point<float> px) const noexcept
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float plotL = kPadX;
    const float plotR = w - kPadX;
    const float plotT = kPadTop;
    const float plotB = h - kPadBottom;

    const float nx = (px.x - plotL) / std::max (1.0f, plotR - plotL);
    const float ny = (plotB - px.y) / std::max (1.0f, plotB - plotT);

    return { juce::jlimit (0.0f, 1.0f, nx),
             juce::jlimit (0.0f, 1.0f, ny) };
}

int NoiseFilterVisualizer::hitTestNode (juce::Point<float> px) const noexcept
{
    for (int i = 0; i < 3; ++i)
    {
        auto np = toPixel (points[i]);
        if (np.getDistanceFrom (px) <= kHitRadius)
            return i;
    }
    return -1;
}

// =============================================================================
// Curve builder – cubic Bezier through 5 points (edges + 3 nodes)
// =============================================================================

juce::Path NoiseFilterVisualizer::buildCurvePath() const
{
    // 5 ordered points: left-edge, HP, Mid, LP, right-edge
    const juce::Point<float> pts[5] = {
        { 0.0f, kFloorY },
        points[0],
        points[1],
        points[2],
        { 1.0f, kFloorY }
    };

    juce::Path path;
    auto p0 = toPixel (pts[0]);
    path.startNewSubPath (p0);

    for (int i = 0; i < 4; ++i)
    {
        auto a = toPixel (pts[i]);
        auto b = toPixel (pts[i + 1]);

        const float dx = b.x - a.x;
        constexpr float tension = 0.40f;

        juce::Point<float> cp1 { a.x + dx * tension, a.y };
        juce::Point<float> cp2 { b.x - dx * tension, b.y };

        path.cubicTo (cp1, cp2, b);
    }

    return path;
}

// =============================================================================
// paint
// =============================================================================

void NoiseFilterVisualizer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float plotL = kPadX;
    const float plotR = bounds.getWidth() - kPadX;
    const float plotT = kPadTop;
    const float plotB = bounds.getHeight() - kPadBottom;

    // ── 1. Background ────────────────────────────────────────────────────────
    g.setColour (juce::Colour (kBgColour));
    g.fillRoundedRectangle (bounds, 8.0f);

    // Clip to rounded rect
    juce::Path clipPath;
    clipPath.addRoundedRectangle (bounds, 8.0f);
    g.saveState();
    g.reduceClipRegion (clipPath);

    // ── 2. Grid ──────────────────────────────────────────────────────────────
    g.setColour (juce::Colour (kGridColour));
    for (int i = 1; i < 4; ++i)
    {
        const float y = plotT + (float) i / 4.0f * (plotB - plotT);
        g.drawHorizontalLine ((int) y, plotL, plotR);
    }
    for (int i = 1; i < 3; ++i)
    {
        const float x = plotL + (float) i / 3.0f * (plotR - plotL);
        g.drawVerticalLine ((int) x, plotT, plotB);
    }

    // Build the curve path
    auto curvePath = buildCurvePath();

    // ── 3. Filled area under curve ───────────────────────────────────────────
    {
        juce::Path fillPath (curvePath);
        fillPath.lineTo (plotR, plotB);
        fillPath.lineTo (plotL, plotB);
        fillPath.closeSubPath();

        g.setColour (juce::Colour (kAccentColour).withAlpha (0.08f));
        g.fillPath (fillPath);
    }

    // ── 4. Curve stroke ─────────────────────────────────────────────────────
    g.setColour (juce::Colour (kAccentColour).withAlpha (0.85f));
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));

    // ── 5. Nodes ─────────────────────────────────────────────────────────────
    for (int i = 0; i < 3; ++i)
    {
        auto px = toPixel (points[i]);
        const bool active = (hoveredIndex == i || dragIndex == i);

        // Glow ring when active
        if (active)
        {
            g.setColour (juce::Colour (kAccentColour).withAlpha (0.18f));
            g.fillEllipse (px.x - kNodeRadius * 1.8f, px.y - kNodeRadius * 1.8f,
                           kNodeRadius * 3.6f, kNodeRadius * 3.6f);
        }

        g.setColour (active ? juce::Colours::white : juce::Colour (kNodeDim));
        g.fillEllipse (px.x - kNodeRadius, px.y - kNodeRadius,
                       kNodeRadius * 2.0f, kNodeRadius * 2.0f);
    }

    g.restoreState();
}

// =============================================================================
// Mouse interaction
// =============================================================================

void NoiseFilterVisualizer::mouseDown (const juce::MouseEvent& e)
{
    dragIndex = hitTestNode (e.position);
}

void NoiseFilterVisualizer::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIndex < 0) return;

    auto norm = fromPixel (e.position);

    // Enforce per-zone X constraints
    float minX, maxX;
    switch (dragIndex)
    {
        case 0:  minX = kHpMinX;  maxX = kHpMaxX;  break;
        case 1:  minX = kMidMinX; maxX = kMidMaxX; break;
        case 2:  minX = kLpMinX;  maxX = kLpMaxX;  break;
        default: return;
    }

    norm.x = juce::jlimit (minX, maxX, norm.x);
    norm.y = juce::jlimit (0.0f, 1.0f, norm.y);

    points[dragIndex] = norm;
    repaint();
}

void NoiseFilterVisualizer::mouseUp (const juce::MouseEvent&)
{
    dragIndex = -1;
}

void NoiseFilterVisualizer::mouseMove (const juce::MouseEvent& e)
{
    const int idx = hitTestNode (e.position);
    if (idx != hoveredIndex)
    {
        hoveredIndex = idx;
        setMouseCursor (idx >= 0 ? juce::MouseCursor::PointingHandCursor
                                 : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void NoiseFilterVisualizer::mouseExit (const juce::MouseEvent&)
{
    if (hoveredIndex >= 0)
    {
        hoveredIndex = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}
