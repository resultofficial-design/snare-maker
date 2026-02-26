#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Phase 3a-1: Basic UI Layout
//
// Window layout (860 × 520 px):
//
//  ┌──────────────────────────────────────────────────────────┐
//  │  HEADER  (plugin name + tagline)                   50 px │
//  ├─────────────┬──────────────────────────┬─────────────────┤
//  │             │                          │                 │
//  │  BODY ZONE  │    SNARE DRUM VISUAL     │  NOISE ZONE     │
//  │   195 px    │        470 px            │   195 px        │
//  │             │                          │                 │  398 px
//  ├─────────────┴──────────────────────────┴─────────────────┤
//  │  ROOM / SPACE ZONE  (full width)                   72 px │
//  └──────────────────────────────────────────────────────────┘
//
// Clicking a zone activates it (accent border + brighter hints).
// Sliders/knobs are added in Phase 3a-2 and later.
// =============================================================================

class SnareMakerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SnareMakerAudioProcessorEditor (SnareMakerAudioProcessor&);
    ~SnareMakerAudioProcessorEditor() override;

    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    // ── Zone identifiers ─────────────────────────────────────────────────────
    enum class Zone { None, Body, Noise, Room };

    // ── Interaction state ─────────────────────────────────────────────────────
    Zone hoveredZone { Zone::None };
    Zone activeZone  { Zone::Body };    // Body section active by default

    // ── Zone rectangles (computed in resized()) ───────────────────────────────
    juce::Rectangle<int> bodyZoneBounds;
    juce::Rectangle<int> drumAreaBounds;
    juce::Rectangle<int> noiseZoneBounds;
    juce::Rectangle<int> roomZoneBounds;

    // ── Paint helpers ─────────────────────────────────────────────────────────
    Zone zoneAt        (juce::Point<int> pos) const noexcept;

    void paintHeader   (juce::Graphics&) const;

    void paintZone     (juce::Graphics&,
                        juce::Rectangle<int>    bounds,
                        Zone                    zone,
                        const juce::String&     title,
                        juce::Colour            accent,
                        const juce::StringArray& hints) const;

    void paintDrumArea  (juce::Graphics&, juce::Rectangle<int> area) const;
    void paintSnareDrum (juce::Graphics&, juce::Rectangle<int> area) const;

    // ─────────────────────────────────────────────────────────────────────────
    SnareMakerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
