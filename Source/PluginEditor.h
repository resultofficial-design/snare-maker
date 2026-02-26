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

    // ─── Member order: audioProcessor first (ref), lnf before all child
    //     components, attachments last (destroyed first).

    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // ─── Body oscillator sliders (Phase 1 + 2a – unchanged) ──────────────────
    juce::Slider bodyFreqSlider;
    juce::Slider pitchAmountSlider;
    juce::Slider pitchDecaySlider;
    juce::Slider phaseOffsetSlider;

    // ─── Noise sliders ────────────────────────────────────────────────────────
    juce::Slider noiseLevelSlider;

    // Phase 2b ADSR
    juce::Slider noiseAttackSlider;
    juce::Slider noiseDecaySlider;
    juce::Slider noiseSustainSlider;
    juce::Slider noiseReleaseSlider;

    // Phase 2b filter
    juce::Slider noiseFiltFreqSlider;
    juce::Slider noiseFiltQSlider;

    // Phase 2b brightness
    juce::Slider noiseBrightSlider;

    // ─── Output slider ────────────────────────────────────────────────────────
    juce::Slider outputGainSlider;

    // ─── Parameter-name labels ────────────────────────────────────────────────
    juce::Label bodyFreqLabel;
    juce::Label pitchAmountLabel;
    juce::Label pitchDecayLabel;
    juce::Label phaseOffsetLabel;

    juce::Label noiseLevelLabel;
    juce::Label noiseAttackLabel;
    juce::Label noiseDecayLabel;
    juce::Label noiseSustainLabel;
    juce::Label noiseReleaseLabel;
    juce::Label noiseFiltFreqLabel;
    juce::Label noiseFiltQLabel;
    juce::Label noiseBrightLabel;

    juce::Label outputGainLabel;

    // ─── Combo boxes ──────────────────────────────────────────────────────────
    juce::ComboBox pitchCurveCombo;     // Phase 2a
    juce::Label    pitchCurveLabel;

    juce::ComboBox noiseFiltTypeCombo;  // Phase 2b
    juce::Label    noiseFiltTypeLabel;

    // ─── APVTS attachments (declared last → destroyed first) ──────────────────
    using Attachment         = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment    = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // Body
    std::unique_ptr<Attachment>      bodyFreqAttachment;
    std::unique_ptr<Attachment>      pitchAmountAttachment;
    std::unique_ptr<Attachment>      pitchDecayAttachment;
    std::unique_ptr<Attachment>      phaseOffsetAttachment;
    std::unique_ptr<ComboAttachment> pitchCurveAttachment;

    // Noise
    std::unique_ptr<Attachment>      noiseLevelAttachment;
    std::unique_ptr<Attachment>      noiseAttackAttachment;
    std::unique_ptr<Attachment>      noiseDecayAttachment;
    std::unique_ptr<Attachment>      noiseSustainAttachment;
    std::unique_ptr<Attachment>      noiseReleaseAttachment;
    std::unique_ptr<ComboAttachment> noiseFiltTypeAttachment;
    std::unique_ptr<Attachment>      noiseFiltFreqAttachment;
    std::unique_ptr<Attachment>      noiseFiltQAttachment;
    std::unique_ptr<Attachment>      noiseBrightAttachment;

    // Output
    std::unique_ptr<Attachment>      outputGainAttachment;

    // ─── Helper ───────────────────────────────────────────────────────────────
    void setupSlider (juce::Slider&, juce::Label&,
                      const juce::String& labelText, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
