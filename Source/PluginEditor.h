#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditor.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Clean UI (envelope-only)
//
// Window layout (960 × 520 px):
//
//  ┌──────────────────────────────────────────────────────────────────┐
//  │  HEADER  (plugin name + BODY/NOISE tabs)                  50 px │
//  ├──────────┬──────────────────────────┬────────────┬─────────────┤
//  │          │  ENVELOPE EDITOR         │            │             │
//  │  BODY    │  + PITCH/AMP buttons     │   NOISE    │   OUTPUT   │
//  │  195 px  │        470 px            │   200 px   │    95 px   │
//  │          │                          │            │             │  398 px
//  ├──────────┴──────────────────────────┴────────────┴─────────────┤
//  │  ROOM / SPACE ZONE  (full width 960 px)                  72 px │
//  └──────────────────────────────────────────────────────────────────┘
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
    // ── Custom LookAndFeel ────────────────────────────────────────────────────
    struct SnareLookAndFeel : public juce::LookAndFeel_V4
    {
        SnareLookAndFeel();

        void drawLinearSlider (juce::Graphics&,
                               int x, int y, int width, int height,
                               float sliderPos,
                               float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle,
                               juce::Slider&) override;
    };

    // ── Top-level tab (Phase 6-1) ───────────────────────────────────────────
    enum class Tab { Body, Noise };
    Tab activeTab  { Tab::Body };
    Tab hoveredTab { Tab::Body };   // only for paint; no separate "none" needed

    juce::Rectangle<int> bodyTabBounds;
    juce::Rectangle<int> noiseTabBounds;

    void setActiveTab (Tab tab);    // show/hide controls + repaint
    void paintTabs    (juce::Graphics&) const;

    // ── Zone identifiers ─────────────────────────────────────────────────────
    enum class Zone { None, Body, Noise, Output, Room };
    Zone hoveredZone { Zone::None };
    Zone activeZone  { Zone::Body };

    // ── Zone rectangles (set in resized()) ───────────────────────────────────
    juce::Rectangle<int> bodyZoneBounds;
    juce::Rectangle<int> drumAreaBounds;
    juce::Rectangle<int> noiseZoneBounds;
    juce::Rectangle<int> outputZoneBounds;
    juce::Rectangle<int> roomZoneBounds;

    // ── Lifetime-ordered members ──────────────────────────────────────────────
    //    audioProcessor ref first, lnf before all child components,
    //    attachments last (destroyed first).
    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // ── Envelope editor (display-only, sits over drum area) ─────────────────
    EnvelopeEditor             envelopeEditor;

    // ── Envelope mode buttons (below envelope editor) ────────────────────────
    enum class EnvMode { Pitch, BodyAmp, NoiseAmp };
    EnvMode envMode { EnvMode::Pitch };

    juce::TextButton bodyPitchBtn { "PITCH" };
    juce::TextButton bodyAmpBtn   { "AMP" };
    juce::TextButton noiseAmpBtn  { "AMP" };

    void setEnvMode (EnvMode mode);

    // ── Helpers ───────────────────────────────────────────────────────────────
    Zone zoneAt (juce::Point<int> pos) const noexcept;

    void paintHeader   (juce::Graphics&) const;
    void paintZone     (juce::Graphics&,
                        juce::Rectangle<int>     bounds,
                        Zone                     zone,
                        const juce::String&      title,
                        juce::Colour             accent,
                        const juce::StringArray& hints,
                        bool                     hasControls = false) const;
    void paintDrumArea  (juce::Graphics&, juce::Rectangle<int> area) const;
    void paintSnareDrum (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
