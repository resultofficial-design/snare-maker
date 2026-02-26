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

    // ── Layout constants ──────────────────────────────────────────────────
    //
    //  Phase 2b layout: 13 columns × 100 px = 1300 px wide, 400 px tall
    //
    //  Cols  0-3  : Pitch panel  (body freq, pitch amt, pitch decay, phase off)
    //  Cols  4-11 : Noise panel  (level, attack, decay, sustain, release,
    //                             filt freq, filt Q, brightness)
    //  Col  12    : Output panel (output gain)
    //
    constexpr int kColW    = 100;
    constexpr int kPanelY  = 50;    // section panels start here
    constexpr int kComboY  = 68;    // combo-box row (between title bar and sliders)
    constexpr int kComboH  = 22;
    constexpr int kTopY    = 100;   // parameter-name labels start here
    constexpr int kLabelH  = 18;
    constexpr int kSliderH = 240;   // includes built-in TextBoxBelow (18 px)
    constexpr int kPadX    = 8;

    // Column indices – kept as named constants for clarity
    constexpr int kColBodyFreq     = 0;
    constexpr int kColPitchAmount  = 1;
    constexpr int kColPitchDecay   = 2;
    constexpr int kColPhaseOffset  = 3;

    constexpr int kColNoiseLevel   = 4;
    constexpr int kColNoiseAttack  = 5;
    constexpr int kColNoiseDecay   = 6;
    constexpr int kColNoiseSustain = 7;
    constexpr int kColNoiseRelease = 8;
    constexpr int kColFiltFreq     = 9;
    constexpr int kColFiltQ        = 10;
    constexpr int kColBrightness   = 11;

    constexpr int kColOutput       = 12;
}

// =============================================================================
// SnareLookAndFeel  (unchanged from Phase 1 / 2a)
// =============================================================================

SnareMakerAudioProcessorEditor::SnareLookAndFeel::SnareLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,         kTextMuted);
    setColour (juce::Slider::textBoxBackgroundColourId,   kBgWindow);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,    kPitchBlue);
    setColour (juce::Label::textColourId,                 kTextBright);

    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff181828));
    setColour (juce::PopupMenu::textColourId,                  kTextBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kPitchBlue);
    setColour (juce::PopupMenu::highlightedTextColourId,       kTextBright);
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

    const juce::Colour accent = slider.findColour (juce::Slider::trackColourId);
    const float        fillH  = bottom - sliderPos;
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

    // ── Body oscillator sliders (Phase 1 + 2a – unchanged) ───────────────────
    setupSlider (bodyFreqSlider,    bodyFreqLabel,    "BODY FREQ",   kPitchBlue);
    setupSlider (pitchAmountSlider, pitchAmountLabel, "PITCH AMT",   kPitchBlue);
    setupSlider (pitchDecaySlider,  pitchDecayLabel,  "PITCH DECAY", kPitchBlue);
    setupSlider (phaseOffsetSlider, phaseOffsetLabel, "PHASE OFF",   kPitchBlue);

    // Pitch-curve ComboBox (Phase 2a – unchanged)
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

    // ── Noise sliders (Phase 2b) ──────────────────────────────────────────────
    setupSlider (noiseLevelSlider,   noiseLevelLabel,   "LEVEL",     kNoiseRed);
    setupSlider (noiseAttackSlider,  noiseAttackLabel,  "ATTACK",    kNoiseRed);
    setupSlider (noiseDecaySlider,   noiseDecayLabel,   "DECAY",     kNoiseRed);
    setupSlider (noiseSustainSlider, noiseSustainLabel, "SUSTAIN",   kNoiseRed);
    setupSlider (noiseReleaseSlider, noiseReleaseLabel, "RELEASE",   kNoiseRed);
    setupSlider (noiseFiltFreqSlider,noiseFiltFreqLabel,"FILT FREQ", kNoiseRed);
    setupSlider (noiseFiltQSlider,   noiseFiltQLabel,   "FILT Q",    kNoiseRed);
    setupSlider (noiseBrightSlider,  noiseBrightLabel,  "BRIGHT",    kNoiseRed);

    // Noise filter-type ComboBox (Phase 2b)
    noiseFiltTypeCombo.addItem ("High Pass", 1);
    noiseFiltTypeCombo.addItem ("Band Pass", 2);
    noiseFiltTypeCombo.addItem ("Low Pass",  3);
    noiseFiltTypeCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff252538));
    noiseFiltTypeCombo.setColour (juce::ComboBox::textColourId,       kTextBright);
    noiseFiltTypeCombo.setColour (juce::ComboBox::outlineColourId,    kNoiseRed.withAlpha (0.5f));
    noiseFiltTypeCombo.setColour (juce::ComboBox::arrowColourId,      kNoiseRed);
    addAndMakeVisible (noiseFiltTypeCombo);

    noiseFiltTypeLabel.setText ("FILTER", juce::dontSendNotification);
    noiseFiltTypeLabel.setJustificationType (juce::Justification::centredRight);
    noiseFiltTypeLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
    noiseFiltTypeLabel.setColour (juce::Label::textColourId, kTextMuted);
    addAndMakeVisible (noiseFiltTypeLabel);

    // ── Output slider ─────────────────────────────────────────────────────────
    setupSlider (outputGainSlider, outputGainLabel, "OUTPUT GAIN", kOutTeal);

    // ── APVTS attachments ─────────────────────────────────────────────────────
    auto& apvts = audioProcessor.apvts;

    bodyFreqAttachment      = std::make_unique<Attachment>      (apvts, "bodyFreq",     bodyFreqSlider);
    pitchAmountAttachment   = std::make_unique<Attachment>      (apvts, "pitchAmount",  pitchAmountSlider);
    pitchDecayAttachment    = std::make_unique<Attachment>      (apvts, "pitchDecay",   pitchDecaySlider);
    phaseOffsetAttachment   = std::make_unique<Attachment>      (apvts, "phaseOffset",  phaseOffsetSlider);
    pitchCurveAttachment    = std::make_unique<ComboAttachment> (apvts, "pitchCurve",   pitchCurveCombo);

    noiseLevelAttachment    = std::make_unique<Attachment>      (apvts, "noiseLevel",   noiseLevelSlider);
    noiseAttackAttachment   = std::make_unique<Attachment>      (apvts, "noiseAttack",  noiseAttackSlider);
    noiseDecayAttachment    = std::make_unique<Attachment>      (apvts, "noiseDecay",   noiseDecaySlider);
    noiseSustainAttachment  = std::make_unique<Attachment>      (apvts, "noiseSustain", noiseSustainSlider);
    noiseReleaseAttachment  = std::make_unique<Attachment>      (apvts, "noiseRelease", noiseReleaseSlider);
    noiseFiltTypeAttachment = std::make_unique<ComboAttachment> (apvts, "noiseFiltType",noiseFiltTypeCombo);
    noiseFiltFreqAttachment = std::make_unique<Attachment>      (apvts, "noiseFiltFreq",noiseFiltFreqSlider);
    noiseFiltQAttachment    = std::make_unique<Attachment>      (apvts, "noiseFiltQ",   noiseFiltQSlider);
    noiseBrightAttachment   = std::make_unique<Attachment>      (apvts, "noiseBright",  noiseBrightSlider);

    outputGainAttachment    = std::make_unique<Attachment>      (apvts, "outputGain",   outputGainSlider);

    // 13 columns × 100 px wide, 400 px tall
    setSize (1300, 400);
}

SnareMakerAudioProcessorEditor::~SnareMakerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

// =============================================================================
// setupSlider  (unchanged helper)
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

    // ── Section panels ────────────────────────────────────────────────────────
    // Panel x = firstCol * kColW + 4
    // Panel w = numCols  * kColW - 8
    const float panelY = (float) kPanelY;
    const float panelH = (float) (h - kPanelY - 4);

    g.setColour (kBgPanel);
    // Pitch  (cols 0-3)
    g.fillRoundedRectangle (  4.0f, panelY, 392.0f, panelH, 6.0f);
    // Noise  (cols 4-11)
    g.fillRoundedRectangle (404.0f, panelY, 792.0f, panelH, 6.0f);
    // Output (col 12)
    g.fillRoundedRectangle (1204.0f, panelY, 92.0f, panelH, 6.0f);

    // ── Section header text ───────────────────────────────────────────────────
    constexpr int hdrY = kPanelY + 5;
    constexpr int hdrH = 14;
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));

    g.setColour (kPitchBlue);
    g.drawText ("PITCH",  4,    hdrY, 392, hdrH, juce::Justification::centred, false);

    g.setColour (kNoiseRed);
    g.drawText ("NOISE",  404,  hdrY, 792, hdrH, juce::Justification::centred, false);

    g.setColour (kOutTeal);
    g.drawText ("OUTPUT", 1204, hdrY,  92, hdrH, juce::Justification::centred, false);

    // ── Noise sub-section dividers (light vertical lines) ─────────────────────
    // Separates: ADSR | FILTER | BRIGHT
    g.setColour (kBgTrack);
    const float divTop = (float)(kPanelY + 20);
    const float divBot = panelY + panelH - 4.0f;

    // After RELEASE (end of col 8 = x 900)
    g.fillRect (900.0f, divTop, 1.0f, divBot - divTop);
    // After FILT Q (end of col 10 = x 1100)
    g.fillRect (1100.0f, divTop, 1.0f, divBot - divTop);
}

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    // ── Helper: place one slider + its label into a column ────────────────────
    auto placeColumn = [&] (juce::Slider& slider, juce::Label& label, int col)
    {
        const int x  = col * kColW + kPadX;
        const int cw = kColW - kPadX * 2;
        label .setBounds (x, kTopY,           cw, kLabelH);
        slider.setBounds (x, kTopY + kLabelH, cw, kSliderH);
    };

    // ── Pitch panel (Phase 2a – unchanged) ───────────────────────────────────
    constexpr int comboLabelW = 52;
    pitchCurveLabel.setBounds (kPadX, kComboY, comboLabelW, kComboH);
    pitchCurveCombo.setBounds (kPadX + comboLabelW + 4, kComboY,
                               4 * kColW - kPadX * 2 - comboLabelW - 4, kComboH);

    placeColumn (bodyFreqSlider,    bodyFreqLabel,    kColBodyFreq);
    placeColumn (pitchAmountSlider, pitchAmountLabel, kColPitchAmount);
    placeColumn (pitchDecaySlider,  pitchDecayLabel,  kColPitchDecay);
    placeColumn (phaseOffsetSlider, phaseOffsetLabel, kColPhaseOffset);

    // ── Noise panel ───────────────────────────────────────────────────────────

    // Filter-type ComboBox sits in the header row of the filter sub-section
    // (cols 9-10), mirroring the style of the pitchCurve combo.
    const int filtSectionX = kColFiltFreq * kColW;                // = 900
    const int filtSectionW = 2 * kColW;                           // = 200
    noiseFiltTypeLabel.setBounds (filtSectionX + kPadX, kComboY,
                                  comboLabelW, kComboH);
    noiseFiltTypeCombo.setBounds (filtSectionX + kPadX + comboLabelW + 4, kComboY,
                                  filtSectionW - kPadX * 2 - comboLabelW - 4, kComboH);

    // Noise sliders
    placeColumn (noiseLevelSlider,    noiseLevelLabel,    kColNoiseLevel);
    placeColumn (noiseAttackSlider,   noiseAttackLabel,   kColNoiseAttack);
    placeColumn (noiseDecaySlider,    noiseDecayLabel,    kColNoiseDecay);
    placeColumn (noiseSustainSlider,  noiseSustainLabel,  kColNoiseSustain);
    placeColumn (noiseReleaseSlider,  noiseReleaseLabel,  kColNoiseRelease);
    placeColumn (noiseFiltFreqSlider, noiseFiltFreqLabel, kColFiltFreq);
    placeColumn (noiseFiltQSlider,    noiseFiltQLabel,    kColFiltQ);
    placeColumn (noiseBrightSlider,   noiseBrightLabel,   kColBrightness);

    // ── Output panel ──────────────────────────────────────────────────────────
    placeColumn (outputGainSlider, outputGainLabel, kColOutput);
}
