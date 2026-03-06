#pragma once
#include <JuceHeader.h>
#include "FlexibleEnvelope.h"
#include "EnvelopeModeToggle.h"
#include <cstring>

// =============================================================================
// EnvelopeEditor  --  generic N-point envelope display + drag editing
//
// Draws a smooth N-point envelope curve with grid, filled area, and nodes.
// All points are normalized (0..1).  Nodes are draggable with the mouse;
// X ordering is enforced.  First/last node X is pinned at 0/1; Y is free.
//
// Curve rendering uses per-segment exponential shaping via piecewise quadratic
// Bezier sub-segments, matching the FlexibleEnvelope::evaluate() in the
// processor.  Per-segment curvature is stored in each EnvelopePoint::curve.
//
// A lightweight preview waveform is rendered behind the envelope curve.
//
// The editor is the sole source of truth for envelope points.
// Double-click to add points, right-click to remove interior points.
// A juce::SpinLock (from the processor) guards add/remove operations.
// =============================================================================

class EnvelopeEditor : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    // Waveform layer identifiers
    enum class WaveLayer { Transient, Body, Resonant, Noise, COUNT };

    EnvelopeEditor();
    ~EnvelopeEditor() override;

    // Call once after construction to bind to the processor's
    // FlexibleEnvelope (shared, not copied), SpinLock, waveform preview params,
    // and global display mode.
    void connectToParameters (juce::AudioProcessorValueTreeState& apvts,
                              FlexibleEnvelope& pitchEnv,
                              juce::SpinLock& envLock,
                              std::atomic<int>& displayMode);

    // Switch which envelope the editor displays and edits.
    // The waveform preview always uses the pitch envelope set in connectToParameters.
    void setEnvelope (FlexibleEnvelope& env);

    // Set which waveform layer is highlighted (full alpha); others are dimmed.
    void setActiveLayer (WaveLayer layer);

    // Replace a layer's waveform with externally-loaded sample data.
    // Resamples to kWaveformSamples and rebuilds paths.
    void setLayerSampleData (WaveLayer layer, const float* data, int numSamples);

    // Clear the sample flag for a layer so regenerateWaveform() produces synth output again.
    void clearLayerSampleFlag (WaveLayer layer);

    // Bind per-layer amplitude envelopes (each shapes its layer's waveform preview).
    void setAmpEnvelopes (FlexibleEnvelope& transient,
                          FlexibleEnvelope& body,
                          FlexibleEnvelope& resonant,
                          FlexibleEnvelope& noise,
                          FlexibleEnvelope& room);

    // Override a single layer's amp envelope (used for Body/Room layer sharing).
    void setLayerAmpEnvelope (WaveLayer layer, FlexibleEnvelope& env);

    // Bind playback position from processor (elapsed seconds, <0 = idle).
    void setPlaybackPosition (std::atomic<float>& pos) { playbackPos = &pos; }

    void paint     (juce::Graphics&) override;
    void resized   () override;

    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDrag        (const juce::MouseEvent&) override;
    void mouseUp          (const juce::MouseEvent&) override;
    void mouseMove        (const juce::MouseEvent&) override;
    void mouseExit        (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    EnvelopeModeToggle::DisplayMode getDisplayMode() const { return modeToggle.getMode(); }

    // ── Drag & drop support ────────────────────────────────────────────────
    void setDragDropEnabled (bool enabled);
    std::function<void (const juce::String&)> onSampleDropped;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit  (const juce::StringArray& files) override;
    void filesDropped  (const juce::StringArray& files, int x, int y) override;

private:
    EnvelopeModeToggle modeToggle;
    // Timer: polls parameters to keep display in sync with sliders / automation
    void timerCallback() override;

    // Pointer to the currently-edited FlexibleEnvelope (not owned).
    // Set via connectToParameters() or setEnvelope().
    FlexibleEnvelope* envelope = nullptr;

    // Always points to the pitch envelope for waveform preview accuracy.
    FlexibleEnvelope* pitchEnvelopeForPreview = nullptr;

    // Interaction state – nodes
    int dragIndex    = -1;   // index of node being dragged, -1 = none
    int hoveredIndex = -1;   // index of node under cursor,  -1 = none

    // Interaction state – segment curve dragging
    int   hoveredSegment     = -1;    // segment index under cursor (-1 = none)
    int   curveDragSegment   = -1;    // segment being curve-dragged (-1 = none)
    float curveDragStartY    = 0.0f;  // mouse Y at drag start (pixels)
    float curveDragStartCurve = 0.0f; // curve value at drag start

    // SpinLock from processor (guards add/remove of points)
    juce::SpinLock* envelopeLock = nullptr;

    // FNV-1a hash over all envelope points for change detection
    uint64_t computePointsHash () const noexcept;
    uint64_t lastPointsHash = 0;

    // ── Additional raw pointers for waveform preview generation ─────────────
    std::atomic<float>*         pBodyFreq        = nullptr;
    std::atomic<float>*         pPitchAmount     = nullptr;
    std::atomic<float>*         pPitchDecay      = nullptr;
    std::atomic<float>*         pNoiseLevel      = nullptr;
    std::atomic<float>*         pNoiseDecay      = nullptr;
    std::atomic<float>*         pPhaseOffset     = nullptr;

    // ── Preview waveform (multi-layer) ──────────────────────────────────────
    static constexpr int kWaveformSamples = 2048;
    static constexpr int kNumLayers       = static_cast<int> (WaveLayer::COUNT);

    std::vector<float> layerBuffers[kNumLayers];
    juce::Path         layerPaths      [kNumLayers];   // TRUE paths (full detail)
    juce::Path         simplePaths     [kNumLayers];   // SIMPLE paths (smoothed RMS)
    bool               layerUsesSample [kNumLayers] {};  // true = loaded sample, skip synth regen
    FlexibleEnvelope*  ampEnvForLayer  [kNumLayers] {};  // per-layer amp envelopes (not owned)
    WaveLayer          activeLayer { WaveLayer::Body };

    // Pointer to global display mode (owned by processor, 0=Simple, 1=True)
    std::atomic<int>*  globalDisplayMode = nullptr;

    // Per-layer colours (must match tab accent colours in PluginEditor.cpp)
    static constexpr juce::uint32 kLayerColours[kNumLayers] {
        0xffffaa33,   // Transient – matches kTransientOrng
        0xff4a9eff,   // Body      – matches kPitchBlue
        0xff55dd77,   // Resonant  – matches kResonantGrn
        0xffe94560    // Noise     – matches kNoiseRed
    };

    float              waveformDuration = 0.5f;   // seconds, derived from params

    // Playback position (from processor, not owned)
    std::atomic<float>* playbackPos = nullptr;

    // Change detection: last values that produced the current waveform
    float lastWfBodyFreq    = -1.0f;
    float lastWfPitchAmount = -1.0f;
    float lastWfPitchDecay  = -1.0f;
    float lastWfNoiseLevel  = -1.0f;
    float lastWfNoiseDecay  = -1.0f;
    float lastWfPhaseOffset = -1.0f;

    void regenerateWaveform ();
    void paintWaveform (juce::Graphics&, float plotL, float plotR,
                        float plotT, float plotB) const;

    // Drawing constants
    static constexpr float kNodeRadius   = 6.0f;
    static constexpr float kHitRadius    = 12.0f;  // generous grab area
    static constexpr float kHoverRadius  = 8.0f;   // visual size when hovered
    static constexpr int   kGridLinesH   = 4;
    static constexpr int   kGridLinesV   = 5;
    static constexpr float kPadX         = 24.0f;
    static constexpr float kPadTop       = 20.0f;
    static constexpr float kPadBottom    = 20.0f;
    static constexpr float kXOrderMargin = 0.005f;  // min gap between adjacent X

    // Per-segment curvature now stored in EnvelopePoint::curve (FlexibleEnvelope)
    static constexpr int   kSubSegments  = 6;      // quadratic bezier pieces per segment

    // Palette (matches main editor)
    static constexpr juce::uint32 kBgColour    = 0xff1E2229;
    static constexpr juce::uint32 kGridColour  = 0xff222830;
    static constexpr juce::uint32 kAccentColour = 0xff4a9eff;   // pitch blue
    static constexpr juce::uint32 kNodeFill    = 0xffffffff;
    static constexpr juce::uint32 kNodeBorder  = 0xff4a9eff;

    // Convert normalised (0..1) to component pixel coords
    juce::Point<float> toPixel (juce::Point<float> norm) const noexcept;

    // Convert component pixel coords to normalised (0..1), clamped
    juce::Point<float> fromPixel (juce::Point<float> px) const noexcept;

    // Find nearest node within kHitRadius of a pixel position, or -1
    int hitTestNode (juce::Point<float> px) const noexcept;

    // Find nearest segment within kHitRadius of a pixel position, or -1
    // Only returns a hit if no node is under the cursor (nodes take priority).
    int hitTestSegment (juce::Point<float> px) const noexcept;

    // Build exponentially-shaped path through all points
    // (piecewise quadratic Bezier, per-segment curvature)
    juce::Path buildCurvePath () const;

    // ── Drag & drop state ────────────────────────────────────────────────
    bool dragDropEnabled = false;
    bool fileDragOver    = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeEditor)
};
