#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Phase 3a-2a
//
// Phase 3a-1 layout (860 × 520 px, 3 zones + drum visual) is preserved.
// Phase 3a-2a adds the first real control: bodyFreq LinearVertical slider
// with an APVTS attachment, placed inside the Body zone.
//
// Member declaration order (JUCE lifetime rules):
//   1. audioProcessor ref  – must come before lnf
//   2. lnf                 – constructed before sliders, destroyed after them
//   3. slider + label      – components
//   4. attachment          – destroyed first (before slider)
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

    // ── Zone enum & state ─────────────────────────────────────────────────────
    enum class Zone { None, Body, Noise, Room };
    Zone hoveredZone { Zone::None };
    Zone activeZone  { Zone::Body };

    // ── Zone rectangles (set in resized()) ───────────────────────────────────
    juce::Rectangle<int> bodyZoneBounds;
    juce::Rectangle<int> drumAreaBounds;
    juce::Rectangle<int> noiseZoneBounds;
    juce::Rectangle<int> roomZoneBounds;

    // ── Lifetime-ordered members ──────────────────────────────────────────────
    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // Phase 3a-2a/b: Body controls
    juce::Slider bodyFreqSlider;
    juce::Label  bodyFreqLabel;

    juce::Slider phaseOffsetSlider;
    juce::Label  phaseOffsetLabel;

    // Phase 3a-2c: Noise ADSR controls
    juce::Slider noiseAttackSlider;
    juce::Label  noiseAttackLabel;

    juce::Slider noiseDecaySlider;
    juce::Label  noiseDecayLabel;

    juce::Slider noiseSustainSlider;
    juce::Label  noiseSustainLabel;

    juce::Slider noiseReleaseSlider;
    juce::Label  noiseReleaseLabel;

    // Phase 3a-2d: Noise filter freq + tilt EQ brightness
    juce::Slider noiseFiltFreqSlider;
    juce::Label  noiseFiltFreqLabel;

    juce::Slider noiseBrightSlider;
    juce::Label  noiseBrightLabel;

    // APVTS attachments (destroyed before sliders)
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> bodyFreqAttachment;
    std::unique_ptr<Attachment> phaseOffsetAttachment;
    std::unique_ptr<Attachment> noiseAttackAttachment;
    std::unique_ptr<Attachment> noiseDecayAttachment;
    std::unique_ptr<Attachment> noiseSustainAttachment;
    std::unique_ptr<Attachment> noiseReleaseAttachment;
    std::unique_ptr<Attachment> noiseFiltFreqAttachment;
    std::unique_ptr<Attachment> noiseBrightAttachment;

    // ── Paint helpers ─────────────────────────────────────────────────────────
    Zone zoneAt (juce::Point<int> pos) const noexcept;

    void paintHeader   (juce::Graphics&) const;

    // hasControls = true → skip hint labels and "click to expand" text
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
