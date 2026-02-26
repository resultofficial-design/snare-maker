#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// =============================================================================
// SnareMakerAudioProcessorEditor
// =============================================================================
class SnareMakerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SnareMakerAudioProcessorEditor (SnareMakerAudioProcessor&);
    ~SnareMakerAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    // =========================================================================
    // Custom LookAndFeel
    // =========================================================================
    struct SnareLookAndFeel : public juce::LookAndFeel_V4
    {
        SnareLookAndFeel();

        void drawLinearSlider (juce::Graphics&,
                               int x, int y, int width, int height,
                               float sliderPos,
                               float minSliderPos,
                               float maxSliderPos,
                               juce::Slider::SliderStyle,
                               juce::Slider&) override;
    };

    // ─── Member order matters: audioProcessor first (reference), lnf before
    //     all child components so it outlives them, attachments last so they
    //     are destroyed first (before the controls they reference).

    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // ─── Sliders (Phase 1) ────────────────────────────────────────────────────
    juce::Slider bodyFreqSlider;
    juce::Slider pitchAmountSlider;
    juce::Slider pitchDecaySlider;
    juce::Slider noiseLevelSlider;
    juce::Slider noiseDecaySlider;
    juce::Slider outputGainSlider;

    // ─── Sliders (Phase 2a additions) ─────────────────────────────────────────
    juce::Slider phaseOffsetSlider;

    // ─── Parameter-name labels ────────────────────────────────────────────────
    juce::Label bodyFreqLabel;
    juce::Label pitchAmountLabel;
    juce::Label pitchDecayLabel;
    juce::Label noiseLevelLabel;
    juce::Label noiseDecayLabel;
    juce::Label outputGainLabel;
    juce::Label phaseOffsetLabel;   // Phase 2a

    // ─── Pitch-curve selector (Phase 2a) ──────────────────────────────────────
    juce::ComboBox pitchCurveCombo;
    juce::Label    pitchCurveLabel;

    // ─── APVTS attachments (declared last → destroyed first) ──────────────────
    using Attachment          = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<Attachment> bodyFreqAttachment;
    std::unique_ptr<Attachment> pitchAmountAttachment;
    std::unique_ptr<Attachment> pitchDecayAttachment;
    std::unique_ptr<Attachment> noiseLevelAttachment;
    std::unique_ptr<Attachment> noiseDecayAttachment;
    std::unique_ptr<Attachment> outputGainAttachment;
    std::unique_ptr<Attachment>         phaseOffsetAttachment;  // Phase 2a
    std::unique_ptr<ComboBoxAttachment> pitchCurveAttachment;   // Phase 2a

    // ─── Helper ───────────────────────────────────────────────────────────────
    void setupSlider (juce::Slider&, juce::Label&,
                      const juce::String& labelText, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
