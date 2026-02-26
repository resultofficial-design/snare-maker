#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Phase 3a-2 (complete)
//
// Window layout (960 × 520 px):
//
//  ┌──────────────────────────────────────────────────────────────────┐
//  │  HEADER  (plugin name + tagline)                          50 px │
//  ├──────────┬──────────────────────────┬────────────┬─────────────┤
//  │          │                          │            │             │
//  │  BODY    │    SNARE DRUM VISUAL     │   NOISE    │   OUTPUT   │
//  │  195 px  │        470 px            │   200 px   │    95 px   │
//  │          │                          │            │             │  398 px
//  ├──────────┴──────────────────────────┴────────────┴─────────────┤
//  │  ROOM / SPACE ZONE  (full width 960 px)                  72 px │
//  └──────────────────────────────────────────────────────────────────┘
//
// Body zone  (195 px, 2 cols × 2 rows + pitchCurve combo in header):
//   Col 0: bodyFreq (row 1), pitchAmount (row 2)
//   Col 1: phaseOffset (row 1), pitchDecay (row 2)
//
// Noise zone (200 px, 2 cols × 4 rows + noiseFiltType combo in header):
//   Col 0: noiseLevel · noiseDecay · noiseRelease · noiseFiltQ
//   Col 1: noiseAttack · noiseSustain · noiseFiltFreq · noiseBright
//
// Output strip (95 px): outputGain
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

    // ── Body controls ─────────────────────────────────────────────────────────
    juce::Slider   bodyFreqSlider;      juce::Label bodyFreqLabel;
    juce::Slider   phaseOffsetSlider;   juce::Label phaseOffsetLabel;
    juce::Slider   pitchAmountSlider;   juce::Label pitchAmountLabel;
    juce::Slider   pitchDecaySlider;    juce::Label pitchDecayLabel;
    juce::ComboBox pitchCurveCombo;     juce::Label pitchCurveLabel;

    // ── Noise controls ────────────────────────────────────────────────────────
    juce::Slider   noiseLevelSlider;    juce::Label noiseLevelLabel;
    juce::Slider   noiseAttackSlider;   juce::Label noiseAttackLabel;
    juce::Slider   noiseDecaySlider;    juce::Label noiseDecayLabel;
    juce::Slider   noiseSustainSlider;  juce::Label noiseSustainLabel;
    juce::Slider   noiseReleaseSlider;  juce::Label noiseReleaseLabel;
    juce::Slider   noiseFiltFreqSlider; juce::Label noiseFiltFreqLabel;
    juce::Slider   noiseFiltQSlider;    juce::Label noiseFiltQLabel;
    juce::Slider   noiseBrightSlider;   juce::Label noiseBrightLabel;
    juce::ComboBox noiseFiltTypeCombo;  juce::Label noiseFiltTypeLabel;

    // ── Output controls ───────────────────────────────────────────────────────
    juce::Slider   outputGainSlider;    juce::Label outputGainLabel;

    // ── APVTS attachments (declared last → destroyed first) ───────────────────
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SA> bodyFreqAttachment;
    std::unique_ptr<SA> phaseOffsetAttachment;
    std::unique_ptr<SA> pitchAmountAttachment;
    std::unique_ptr<SA> pitchDecayAttachment;
    std::unique_ptr<CA> pitchCurveAttachment;

    std::unique_ptr<SA> noiseLevelAttachment;
    std::unique_ptr<SA> noiseAttackAttachment;
    std::unique_ptr<SA> noiseDecayAttachment;
    std::unique_ptr<SA> noiseSustainAttachment;
    std::unique_ptr<SA> noiseReleaseAttachment;
    std::unique_ptr<SA> noiseFiltFreqAttachment;
    std::unique_ptr<SA> noiseFiltQAttachment;
    std::unique_ptr<SA> noiseBrightAttachment;
    std::unique_ptr<CA> noiseFiltTypeAttachment;

    std::unique_ptr<SA> outputGainAttachment;

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

    void setupSlider (juce::Slider&, juce::Label&,
                      const juce::String& labelText, juce::Colour accent);
    void setupCombo  (juce::ComboBox&, juce::Label&,
                      const juce::String& labelText, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
