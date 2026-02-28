#pragma once
#include <JuceHeader.h>

// =============================================================================
// NoiseFilterVisualizer  --  3-band draggable filter curve (UI only)
//
// Displays a smooth filter-like frequency curve with 3 draggable nodes:
//   HP   (high-pass)  – constrained to X in [0.00, 0.33]
//   Bell (mid bell)   – constrained to X in [0.33, 0.66]
//   LP   (low-pass)   – constrained to X in [0.66, 1.00]
//
// Purely visual – no DSP connection.  Follows EnvelopeEditor paint pattern.
// =============================================================================

class NoiseFilterVisualizer : public juce::Component
{
public:
    NoiseFilterVisualizer();

    void paint     (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    // 3 filter nodes in normalised coords (0..1)
    juce::Point<float> points[3];

    int dragIndex    = -1;   // index of node being dragged, -1 = none
    int hoveredIndex = -1;   // index of node under cursor,  -1 = none

    // Layout constants (matching EnvelopeEditor pattern)
    static constexpr float kPadX       = 12.0f;
    static constexpr float kPadTop     = 14.0f;
    static constexpr float kPadBottom  = 10.0f;
    static constexpr float kNodeRadius = 5.0f;
    static constexpr float kHitRadius  = 12.0f;

    // X-zone constraints per node
    static constexpr float kHpMinX  = 0.00f;
    static constexpr float kHpMaxX  = 0.33f;
    static constexpr float kMidMinX = 0.33f;
    static constexpr float kMidMaxX = 0.66f;
    static constexpr float kLpMinX  = 0.66f;
    static constexpr float kLpMaxX  = 1.00f;

    // Floor Y for implicit edge points
    static constexpr float kFloorY = 0.05f;

    // Palette
    static constexpr juce::uint32 kBgColour     = 0xff181D24;
    static constexpr juce::uint32 kGridColour   = 0xff222830;
    static constexpr juce::uint32 kAccentColour = 0xffe94560;   // noise red
    static constexpr juce::uint32 kNodeDim      = 0xff555566;

    // Coordinate helpers
    juce::Point<float> toPixel   (juce::Point<float> norm) const noexcept;
    juce::Point<float> fromPixel (juce::Point<float> px)   const noexcept;
    int                hitTestNode (juce::Point<float> px)  const noexcept;

    // Curve builder
    juce::Path buildCurvePath () const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoiseFilterVisualizer)
};
