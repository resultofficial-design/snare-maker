#include "PluginEditor.h"

// =============================================================================
// Colour palette  (file-local)
// =============================================================================
namespace
{
    const juce::Colour kBgWindow  { 0xff0d0d1a };
    const juce::Colour kBgPanel   { 0xff181828 };
    const juce::Colour kBgTrack   { 0xff252538 };
    const juce::Colour kBgTitleBar{ 0xff13132a };

    const juce::Colour kTextBright{ 0xffffffff };
    const juce::Colour kTextMuted { 0xff8888aa };

    const juce::Colour kPitchBlue { 0xff4a9eff };
    const juce::Colour kNoiseRed  { 0xffe94560 };
    const juce::Colour kOutTeal   { 0xff00d4aa };

    // Layout constants – must stay in sync with resized()
    //   Phase 2a: 7 columns (4 pitch + 2 noise + 1 output) × 100 px = 700 px
    constexpr int kColW    = 100;
    constexpr int kPanelY  = 50;    // section panels start here
    constexpr int kComboY  = 68;    // pitch-curve combobox y  (kPanelY + 18)
    constexpr int kComboH  = 22;    // pitch-curve combobox height
    constexpr int kTopY    = 100;   // parameter-name labels start here
    constexpr int kLabelH  = 18;
    constexpr int kSliderH = 240;   // includes built-in TextBoxBelow (18 px)
    constexpr int kPadX    = 8;
}

// =============================================================================
// SnareLookAndFeel
// =============================================================================

SnareMakerAudioProcessorEditor::SnareLookAndFeel::SnareLookAndFeel()
{
    // Slider text-box
    setColour (juce::Slider::textBoxTextColourId,         kTextMuted);
    setColour (juce::Slider::textBoxBackgroundColourId,   kBgWindow);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,    kPitchBlue);

    // Labels
    setColour (juce::Label::textColourId, kTextBright);

    // PopupMenu (used by ComboBox dropdown)
    setColour (juce::PopupMenu::backgroundColourId,           juce::Colour (0xff181828));
    setColour (juce::PopupMenu::textColourId,                 kTextBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId,kPitchBlue);
    setColour (juce::PopupMenu::highlightedTextColourId,      kTextBright);
}

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::drawLinearSlider (
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float /*minSliderPos*/,
    float /*maxSliderPos*/,
    juce::Slider::SliderStyle /*style*/,
    juce::Slider& slider)
{
    constexpr float kTrackW = 6.0f;
    constexpr float kThumbR = 9.0f;

    const float cx     = (float) x + (float) width * 0.5f;
    const float top    = (float) y + kThumbR;
    const float bottom = (float) (y + height) - kThumbR;

    g.setColour (kBgTrack);
    g.fillRoundedRectangle (cx - kTrackW * 0.5f, top, kTrackW, bottom - top, 3.0f);

    const juce::Colour accent    = slider.findColour (juce::Slider::trackColourId);
    const float        fillH     = bottom - sliderPos;
    if (fillH > 0.0f)
    {
        g.setColour (accent);
        g.fillRoundedRectangle (cx - kTrackW * 0.5f, sliderPos, kTrackW, fillH, 3.0f);
    }

    g.setColour (accent.withAlpha (0.22f));
    g.fillEllipse (cx - kThumbR * 1.7f, sliderPos - kThumbR * 1.7f,
                   kThumbR * 3.4f, kThumbR * 3.4f);

    g.setColour (juce::Colours::white);
    g.fillEllipse (cx - kThumbR, sliderPos - kThumbR, kThumbR * 2.0f, kThumbR * 2.0f);

    g.setColour (accent);
    g.drawEllipse (cx - kThumbR + 1.5f, sliderPos - kThumbR + 1.5f,
                   (kThumbR - 1.5f) * 2.0f, (kThumbR - 1.5f) * 2.0f, 1.5f);
}

// =============================================================================
// Editor – constructor / destructor
// =============================================================================

SnareMakerAudioProcessorEditor::SnareMakerAudioProcessorEditor (
        SnareMakerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);

    // ── Phase 1 sliders ───────────────────────────────────────────────────────
    setupSlider (bodyFreqSlider,    bodyFreqLabel,    "BODY FREQ",   kPitchBlue);
    setupSlider (pitchAmountSlider, pitchAmountLabel, "PITCH AMT",   kPitchBlue);
    setupSlider (pitchDecaySlider,  pitchDecayLabel,  "PITCH DECAY", kPitchBlue);
    setupSlider (noiseLevelSlider,  noiseLevelLabel,  "NOISE LEVEL", kNoiseRed);
    setupSlider (noiseDecaySlider,  noiseDecayLabel,  "NOISE DECAY", kNoiseRed);
    setupSlider (outputGainSlider,  outputGainLabel,  "OUTPUT GAIN", kOutTeal);

    // ── Phase 2a: Phase Offset slider ────────────────────────────────────────
    setupSlider (phaseOffsetSlider, phaseOffsetLabel, "PHASE OFF",   kPitchBlue);

    // ── Phase 2a: Pitch Curve combobox ────────────────────────────────────────
    pitchCurveCombo.addItem ("Exponential", 1);
    pitchCurveCombo.addItem ("Linear",      2);
    pitchCurveCombo.addItem ("Logarithmic", 3);
    pitchCurveCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff252538));
    pitchCurveCombo.setColour (juce::ComboBox::textColourId,       kTextBright);
    pitchCurveCombo.setColour (juce::ComboBox::outlineColourId,    kPitchBlue.withAlpha (0.5f));
    pitchCurveCombo.setColour (juce::ComboBox::arrowColourId,      kPitchBlue);
    addAndMakeVisible (pitchCurveCombo);

    pitchCurveLabel.setText ("CURVE", juce::dontSendNotification);
    pitchCurveLabel.setJustificationType (juce::Justification::centredRight);
    pitchCurveLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
    pitchCurveLabel.setColour (juce::Label::textColourId, kTextMuted);
    addAndMakeVisible (pitchCurveLabel);

    // ── APVTS attachments ─────────────────────────────────────────────────────
    auto& apvts = audioProcessor.apvts;

    bodyFreqAttachment    = std::make_unique<Attachment> (apvts, "bodyFreq",    bodyFreqSlider);
    pitchAmountAttachment = std::make_unique<Attachment> (apvts, "pitchAmount", pitchAmountSlider);
    pitchDecayAttachment  = std::make_unique<Attachment> (apvts, "pitchDecay",  pitchDecaySlider);
    noiseLevelAttachment  = std::make_unique<Attachment> (apvts, "noiseLevel",  noiseLevelSlider);
    noiseDecayAttachment  = std::make_unique<Attachment> (apvts, "noiseDecay",  noiseDecaySlider);
    outputGainAttachment  = std::make_unique<Attachment> (apvts, "outputGain",  outputGainSlider);
    phaseOffsetAttachment = std::make_unique<Attachment> (apvts, "phaseOffset", phaseOffsetSlider);  // 2a
    pitchCurveAttachment  = std::make_unique<ComboBoxAttachment> (apvts, "pitchCurve", pitchCurveCombo); // 2a

    // 7 columns × 100 px
    setSize (700, 380);
}

SnareMakerAudioProcessorEditor::~SnareMakerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

// =============================================================================
// setupSlider
// =============================================================================

void SnareMakerAudioProcessorEditor::setupSlider (juce::Slider&       slider,
                                                   juce::Label&        label,
                                                   const juce::String& labelText,
                                                   juce::Colour        accent)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    slider.setColour (juce::Slider::trackColourId, accent);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, kTextMuted);
    addAndMakeVisible (label);
}

// =============================================================================
// paint
// =============================================================================

void SnareMakerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll (kBgWindow);

    // Title bar
    g.setColour (kBgTitleBar);
    g.fillRect (0, 0, w, 44);
    g.setColour (kTextBright);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (20.0f).withStyle ("Bold")));
    g.drawText ("SNARE MAKER", 0, 0, w, 44, juce::Justification::centred, false);

    // Section panels
    // Phase 2a layout:  cols 0-3 = Pitch (400 px)  |  4-5 = Noise (200 px)  |  6 = Output (100 px)
    // Panel inset: 4 px from each column boundary.
    const float panelY = (float) kPanelY;
    const float panelH = (float) (h - kPanelY - 4);

    g.setColour (kBgPanel);
    g.fillRoundedRectangle (4.0f,   panelY, 392.0f, panelH, 6.0f);  // Pitch  (0..400)
    g.fillRoundedRectangle (404.0f, panelY, 192.0f, panelH, 6.0f);  // Noise  (400..600)
    g.fillRoundedRectangle (604.0f, panelY,  92.0f, panelH, 6.0f);  // Output (600..700)

    // Section header text
    constexpr int hdrY = kPanelY + 5;
    constexpr int hdrH = 14;
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));

    g.setColour (kPitchBlue);
    g.drawText ("PITCH",  4,   hdrY, 392, hdrH, juce::Justification::centred, false);

    g.setColour (kNoiseRed);
    g.drawText ("NOISE",  404, hdrY, 192, hdrH, juce::Justification::centred, false);

    g.setColour (kOutTeal);
    g.drawText ("OUTPUT", 604, hdrY,  92, hdrH, juce::Justification::centred, false);
}

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    // ── Pitch-curve combobox (lives in the pitch panel, between header & sliders)
    constexpr int comboLabelW = 52;
    pitchCurveLabel.setBounds (kPadX,              kComboY, comboLabelW,                  kComboH);
    pitchCurveCombo.setBounds (kPadX + comboLabelW + 4, kComboY,
                               4 * kColW - kPadX * 2 - comboLabelW - 4, kComboH);

    // ── One slider + label per column ─────────────────────────────────────────
    auto placeColumn = [&] (juce::Slider& slider, juce::Label& label, int col)
    {
        const int x = col * kColW + kPadX;
        const int cw = kColW - kPadX * 2;
        label .setBounds (x, kTopY,           cw, kLabelH);
        slider.setBounds (x, kTopY + kLabelH, cw, kSliderH);
    };

    // Phase 1 columns
    placeColumn (bodyFreqSlider,    bodyFreqLabel,    0);
    placeColumn (pitchAmountSlider, pitchAmountLabel, 1);
    placeColumn (pitchDecaySlider,  pitchDecayLabel,  2);
    // Phase 2a column
    placeColumn (phaseOffsetSlider, phaseOffsetLabel, 3);
    // Noise (shifted from cols 3-4 to 4-5)
    placeColumn (noiseLevelSlider,  noiseLevelLabel,  4);
    placeColumn (noiseDecaySlider,  noiseDecayLabel,  5);
    // Output (shifted from col 5 to col 6)
    placeColumn (outputGainSlider,  outputGainLabel,  6);
}
