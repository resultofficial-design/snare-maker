#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditor.h"
#include "NoiseFilterVisualizer.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Clean UI (envelope-only)
//
// Window layout (960 × 520 px):
//
//  ┌──────────────────────────────────────────────────────────────────┐
//  │  HEADER  (plugin name + BODY/NOISE tabs)                  50 px │
//  ├──────────────────────────────────────────────────┬─────────────┤
//  │  ENVELOPE EDITOR + PITCH/AMP buttons             │   OUTPUT   │
//  │  (drum visual behind)                            │    95 px   │
//  │                                                  │             │  398 px
//  ├──────────────────────────────────────────────────┴─────────────┤
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

        void drawRotarySlider (juce::Graphics&,
                               int x, int y, int width, int height,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;
    };

    // ── Top-level tabs ──────────────────────────────────────────────────────
    enum class Tab { Transient, Body, Resonant, Noise, Room, Sauce };
    Tab activeTab  { Tab::Body };
    Tab hoveredTab { Tab::Body };

    juce::Rectangle<int> transientTabBounds;
    juce::Rectangle<int> bodyTabBounds;
    juce::Rectangle<int> resonantTabBounds;
    juce::Rectangle<int> noiseTabBounds;
    juce::Rectangle<int> roomTabBounds;
    juce::Rectangle<int> sauceTabBounds;

    bool tabEnabled[6] { true, true, true, true, true, true };
    bool& tabEnabledFor (Tab tab)       { return tabEnabled[static_cast<int>(tab)]; }
    bool  tabEnabledFor (Tab tab) const { return tabEnabled[static_cast<int>(tab)]; }

    void setActiveTab (Tab tab);    // show/hide controls + repaint
    void paintTabs    (juce::Graphics&) const;

    // ── Zone identifiers ─────────────────────────────────────────────────────
    enum class Zone { None, Output };
    Zone hoveredZone { Zone::None };
    Zone activeZone  { Zone::None };

    // ── Zone rectangles (set in resized()) ───────────────────────────────────
    juce::Rectangle<int> drumAreaBounds;
    juce::Rectangle<int> outputZoneBounds;

    // ── Lifetime-ordered members ──────────────────────────────────────────────
    //    audioProcessor ref first, lnf before all child components,
    //    attachments last (destroyed first).
    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // ── Envelope editor (display-only, sits over drum area) ─────────────────
    EnvelopeEditor             envelopeEditor;
    juce::Rectangle<int>       envEditorFullBounds;   // full-width bounds (set in resized)

    // ── Envelope mode buttons (below envelope editor) ────────────────────────
    enum class EnvMode { Pitch, BodyAmp, NoiseAmp, ResonantAmp };
    EnvMode envMode { EnvMode::Pitch };

    juce::TextButton bodyPitchBtn { "PITCH" };
    juce::TextButton bodyAmpBtn   { "AMP" };
    juce::TextButton noiseAmpBtn  { "AMP" };

    void setEnvMode (EnvMode mode);

    // ── Noise source selector (GEN / SAMPLE, UI only) ────────────────────
    enum class NoiseSrc { Gen, Sample };
    NoiseSrc noiseSrc { NoiseSrc::Gen };
    juce::Rectangle<int> noiseSrcBounds;   // hit-test area (set during paint)

    // ── Noise type selector (WHITE…VIOLET, UI only) ────────────────────
    enum class NoiseType { White, Pink, Brown, Blue, Violet };
    NoiseType noiseType { NoiseType::White };
    juce::Rectangle<int> noiseTypeBounds;  // hit-test area (set during paint)

    // ── Noise filter visualizer (GEN mode) ──────────────────────────────────
    NoiseFilterVisualizer       noiseFilterVis;
    juce::Rectangle<int>        noiseFilterBounds;

    // ── Sauce knob (visual only, no APVTS) ──────────────────────────────────
    juce::Slider sauceKnob;

    // ── Preset combo (visual only) ─────────────────────────────────────────
    juce::ComboBox presetCombo;

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
    void paintDrumArea  (juce::Graphics&, juce::Rectangle<int> area);
    void paintSnareDrum (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
