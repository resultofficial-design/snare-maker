#include "PluginEditor.h"
#include "BinaryData.h"

// =============================================================================
// File-local colour palette and layout constants
// =============================================================================

namespace
{
    // ── Colours ───────────────────────────────────────────────────────────────
    const juce::Colour kBgWindow   { 0xff101318 };
    const juce::Colour kBgDrum     { 0xff101318 };
    const juce::Colour kBgPanel    { 0xff1E2229 };
    const juce::Colour kBgPanelHov { 0xff242930 };
    const juce::Colour kBgPanelAct { 0xff2A3038 };
    const juce::Colour kBgTrack    { 0xff2A3038 };
    const juce::Colour kTextBright { 0xffffffff };
    const juce::Colour kTextMuted  { 0xff8888aa };
    const juce::Colour kTextDim    { 0xff444460 };
    const juce::Colour kDivider    { 0xff2A3038 };

    const juce::Colour kPitchBlue  { 0xff4a9eff };
    const juce::Colour kNoiseRed   { 0xffe94560 };
    const juce::Colour kOutTeal    { 0xff00d4aa };
    const juce::Colour kTransientOrng { 0xffffaa33 };
    const juce::Colour kResonantGrn   { 0xff55dd77 };
    const juce::Colour kRoomPurple    { 0xffaa55ff };
    const juce::Colour kSaucePink     { 0xffff6699 };

    // ── Window / section widths ────────────────────────────────────────────────
    constexpr int kWinW    = 960;
    constexpr int kWinH    = 520;
    constexpr int kHeaderH = 50;
    constexpr int kOutputW = 95;
    constexpr int kMainH   = kWinH - kHeaderH;                     // 470

    // ── Tab geometry ──────────────────────────────────────────────────────────
    constexpr int kNumTabs  = 6;
    constexpr int kTabW     = 110;
    constexpr int kTabH     = 28;
    constexpr int kTabGap   = 6;

    // ── Uniform layout spacing ───────────────────────────────────────────────
    constexpr int kUISpacing = 16;
}

// =============================================================================
// SnareLookAndFeel
// =============================================================================

SnareMakerAudioProcessorEditor::SnareLookAndFeel::SnareLookAndFeel()
{
    interRegular = juce::Typeface::createSystemTypefaceFor (
        BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
    interMedium  = juce::Typeface::createSystemTypefaceFor (
        BinaryData::InterMedium_ttf,  BinaryData::InterMedium_ttfSize);

    setColour (juce::Slider::textBoxTextColourId,              kTextMuted);
    setColour (juce::Slider::textBoxBackgroundColourId,        kBgWindow);
    setColour (juce::Slider::textBoxOutlineColourId,           juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,         kPitchBlue);
    setColour (juce::Label::textColourId,                      kTextBright);
    setColour (juce::PopupMenu::backgroundColourId,            kBgPanel);
    setColour (juce::PopupMenu::textColourId,                  kTextBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kBgPanelHov);
    setColour (juce::PopupMenu::highlightedTextColourId,       kTextBright);
}

juce::Typeface::Ptr SnareMakerAudioProcessorEditor::SnareLookAndFeel::getTypefaceForFont (
    const juce::Font& font)
{
    if (font.getTypefaceStyle().containsIgnoreCase ("Bold"))
        return interMedium;
    return interRegular;
}

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::getLabelFont (juce::Label&)
{ return juce::Font (juce::FontOptions{}.withTypeface (interRegular)).withHeight (14.0f); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{ return juce::Font (juce::FontOptions{}.withTypeface (interMedium)).withHeight ((float) buttonHeight * 0.45f); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::getComboBoxFont (juce::ComboBox&)
{ return juce::Font (juce::FontOptions{}.withTypeface (interRegular)).withHeight (14.0f); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::getPopupMenuFont ()
{ return juce::Font (juce::FontOptions{}.withTypeface (interRegular)).withHeight (14.0f); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::getSliderPopupFont (juce::Slider&)
{ return juce::Font (juce::FontOptions{}.withTypeface (interRegular)).withHeight (13.0f); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::interRegularFont (float height) const
{ return juce::Font (juce::FontOptions{}.withTypeface (interRegular)).withHeight (height); }

juce::Font SnareMakerAudioProcessorEditor::SnareLookAndFeel::interMediumFont (float height) const
{ return juce::Font (juce::FontOptions{}.withTypeface (interMedium)).withHeight (height); }

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::drawLinearSlider (
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float /*minSliderPos*/, float /*maxSliderPos*/,
    juce::Slider::SliderStyle /*style*/,
    juce::Slider& slider)
{
    constexpr float kTrackW = 6.0f;
    constexpr float kThumbR = 9.0f;

    const float cx     = (float) x + (float) width  * 0.5f;
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

    // Flat thumb – no glow, no accent ring
    g.setColour (juce::Colour (0xffaaaaaa));
    g.fillEllipse (cx - kThumbR, sliderPos - kThumbR, kThumbR * 2.0f, kThumbR * 2.0f);
}

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::drawRotarySlider (
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    const float diameter = (float) juce::jmin (width, height);
    const float radius   = diameter * 0.5f;
    const float cx       = (float) x + (float) width  * 0.5f;
    const float cy       = (float) y + (float) height * 0.5f;

    const juce::Colour accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // ── Base circle ──────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff1E2229));
    g.fillEllipse (cx - radius, cy - radius, diameter, diameter);

    // ── Value arc ────────────────────────────────────────────────────────────
    const float arcRadius  = radius - 6.0f;
    const float angle      = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    constexpr float arcW   = 4.0f;

    // Glow behind the value arc
    if (sliderPos > 0.0f)
    {
        juce::Path glowArc;
        glowArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, angle, true);
        g.setColour (accent.withAlpha (0.15f));
        g.strokePath (glowArc, juce::PathStrokeType (arcW + 6.0f));
    }

    // Background arc (full range, dim)
    {
        juce::Path bgArc;
        bgArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff2A3038));
        g.strokePath (bgArc, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Active arc (start → current value)
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // ── Pointer line ─────────────────────────────────────────────────────────
    {
        const float pointerLen  = arcRadius - 10.0f;
        const float pointerTail = 8.0f;
        const float px = std::sin (angle);
        const float py = -std::cos (angle);

        g.setColour (juce::Colours::white);
        g.drawLine (cx + px * pointerTail, cy + py * pointerTail,
                    cx + px * pointerLen,  cy + py * pointerLen, 2.0f);
    }
}

// =============================================================================
// PopupMenu drawing
// =============================================================================

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::drawPopupMenuBackground (
    juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colours::transparentBlack);

    juce::Path bg;
    bg.addRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 8.0f);

    g.setColour (findColour (juce::PopupMenu::backgroundColourId));
    g.fillPath (bg);
}

juce::Component* SnareMakerAudioProcessorEditor::SnareLookAndFeel::getParentComponentForMenuOptions (
    const juce::PopupMenu::Options&)
{
    return nullptr;
}

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::preparePopupMenuWindow (juce::Component& window)
{
    window.setOpaque (false);
}

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::drawPopupMenuItem (
    juce::Graphics& g,
    const juce::Rectangle<int>& area,
    bool /*isSeparator*/, bool isActive, bool isHighlighted,
    bool /*isTicked*/, bool /*hasSubMenu*/,
    const juce::String& text,
    const juce::String& /*shortcutKeyText*/,
    const juce::Drawable* /*icon*/,
    const juce::Colour* textColour)
{
    constexpr int hPad = 10;
    auto textArea = area.reduced (hPad, 0);

    if (isHighlighted && isActive)
    {
        g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
        g.fillRoundedRectangle (area.reduced (4, 1).toFloat(), 4.0f);
    }

    g.setColour (textColour != nullptr ? *textColour
                 : findColour (isHighlighted ? juce::PopupMenu::highlightedTextColourId
                                             : juce::PopupMenu::textColourId)
                       .withAlpha (isActive ? 1.0f : 0.4f));
    g.setFont (interRegularFont (13.0f));
    g.drawText (text, textArea, juce::Justification::centredLeft, true);
}

void SnareMakerAudioProcessorEditor::SnareLookAndFeel::getIdealPopupMenuItemSize (
    const juce::String& text,
    bool isSeparator, int /*standardMenuItemHeight*/,
    int& idealWidth, int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth  = 50;
        idealHeight = 8;
        return;
    }

    idealHeight = 28;

    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (interRegularFont (13.0f), text, 0.0f, 0.0f);
    idealWidth = (int) glyphs.getBoundingBox (0, -1, true).getWidth() + 28;
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

SnareMakerAudioProcessorEditor::SnareMakerAudioProcessorEditor (
        SnareMakerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);

    // ── Envelope editor (overlays drum centre area) ─────────────────────────
    envelopeEditor.connectToParameters (audioProcessor.apvts, audioProcessor.pitchEnvelope,
                                        audioProcessor.envelopeLock,
                                        audioProcessor.waveformDisplayMode);
    envelopeEditor.setAmpEnvelopes (audioProcessor.transientAmpEnvelope,
                                     audioProcessor.bodyAmpEnvelope,
                                     audioProcessor.resonantAmpEnvelope,
                                     audioProcessor.noiseAmpEnvelope,
                                     audioProcessor.roomAmpEnvelope);
    envelopeEditor.setPlaybackPosition (audioProcessor.playbackPositionSec);
    addAndMakeVisible (envelopeEditor);

    envelopeEditor.onSampleDropped = [this] (const juce::String& filePath)
    {
        if (activeTab == Tab::Transient)
        {
            if (audioProcessor.loadSampleFromFile (filePath,
                    audioProcessor.transientSampleBuffer,
                    audioProcessor.transientSamplePath))
            {
                envelopeEditor.setLayerSampleData (
                    EnvelopeEditor::WaveLayer::Transient,
                    audioProcessor.transientSampleBuffer.getReadPointer (0),
                    audioProcessor.transientSampleBuffer.getNumSamples());
                repaint();
            }
        }
        else if (activeTab == Tab::Resonant)
        {
            if (audioProcessor.loadSampleFromFile (filePath,
                    audioProcessor.resonantSampleBuffer,
                    audioProcessor.resonantSamplePath))
            {
                envelopeEditor.setLayerSampleData (
                    EnvelopeEditor::WaveLayer::Resonant,
                    audioProcessor.resonantSampleBuffer.getReadPointer (0),
                    audioProcessor.resonantSampleBuffer.getNumSamples());
                repaint();
            }
        }
        else if (activeTab == Tab::Noise)
        {
            if (noiseSrc == NoiseSrc::Gen)
            {
                noiseSrc = NoiseSrc::Sample;
                noiseFilterVis.setVisible (false);
                noiseSampleFilterVis.setVisible (true);
                noiseSampleFilterVis.setBounds (noiseSampleFilterBounds);
                noiseSamplePrevBtn.setVisible (true);
                noiseSampleNextBtn.setVisible (true);
            }
            if (audioProcessor.loadSampleFromFile (filePath,
                    audioProcessor.noiseSampleBuffer,
                    audioProcessor.noiseSamplePath))
            {
                envelopeEditor.setLayerSampleData (
                    EnvelopeEditor::WaveLayer::Noise,
                    audioProcessor.noiseSampleBuffer.getReadPointer (0),
                    audioProcessor.noiseSampleBuffer.getNumSamples());
                repaint();
            }
        }
        else if (activeTab == Tab::Room)
        {
            if (audioProcessor.loadSampleFromFile (filePath,
                    audioProcessor.roomIRBuffer,
                    audioProcessor.roomIRPath))
            {
                envelopeEditor.setLayerSampleData (
                    EnvelopeEditor::WaveLayer::Body,
                    audioProcessor.roomIRBuffer.getReadPointer (0),
                    audioProcessor.roomIRBuffer.getNumSamples());
                repaint();
            }
        }
    };

    // ── Noise filter visualizer (hidden until Noise/GEN) ──────────────────
    addChildComponent (noiseFilterVis);

    // ── Noise sample filter visualizer + buttons (hidden until Noise/SAMPLE)
    addChildComponent (noiseSampleFilterVis);   // default accent = noise red
    for (auto* btn : { &noiseSamplePrevBtn, &noiseSampleNextBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::buttonOnColourId,  juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::textColourOffId,   juce::Colours::white.withAlpha (0.7f));
        btn->setColour (juce::TextButton::textColourOnId,    juce::Colours::white);
        btn->setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        addChildComponent (*btn);
    }
    noiseSamplePrevBtn.onClick = [this] { /* placeholder: previous noise sample */ };
    noiseSampleNextBtn.onClick = [this] { /* placeholder: next noise sample */ };

    // ── Transient filter visualizer (hidden until Transient tab) ─────────
    transientFilterVis.setAccentColour (0xffffaa33);   // transient orange
    addChildComponent (transientFilterVis);

    // ── Transient sample browser prev/next buttons ───────────────────────
    for (auto* btn : { &transientPrevBtn, &transientNextBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::buttonOnColourId,  juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::textColourOffId,   juce::Colours::white.withAlpha (0.7f));
        btn->setColour (juce::TextButton::textColourOnId,    juce::Colours::white);
        btn->setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        addChildComponent (*btn);
    }
    transientPrevBtn.onClick = [this] { /* placeholder: previous sample */ };
    transientNextBtn.onClick = [this] { /* placeholder: next sample */ };

    // ── Room filter visualizer + IR browser buttons ─────────────────────
    roomFilterVis.setAccentColour (0xffaa55ff);   // room purple
    addChildComponent (roomFilterVis);

    for (auto* btn : { &roomPrevBtn, &roomNextBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::buttonOnColourId,  juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::textColourOffId,   juce::Colours::white.withAlpha (0.7f));
        btn->setColour (juce::TextButton::textColourOnId,    juce::Colours::white);
        btn->setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        addChildComponent (*btn);
    }
    roomPrevBtn.onClick = [this] { /* placeholder: previous IR */ };
    roomNextBtn.onClick = [this] { /* placeholder: next IR */ };

    // ── Resonant filter visualizer + sample browser + follow body ─────────
    resonantFilterVis.setAccentColour (0xff55dd77);   // resonant green
    addChildComponent (resonantFilterVis);

    for (auto* btn : { &resonantPrevBtn, &resonantNextBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::buttonOnColourId,  juce::Colours::transparentBlack);
        btn->setColour (juce::TextButton::textColourOffId,   juce::Colours::white.withAlpha (0.7f));
        btn->setColour (juce::TextButton::textColourOnId,    juce::Colours::white);
        btn->setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
        addChildComponent (*btn);
    }
    resonantPrevBtn.onClick = [this] { /* placeholder: previous resonance sample */ };
    resonantNextBtn.onClick = [this] { /* placeholder: next resonance sample */ };

    resonantFollowBodyBtn.setClickingTogglesState (true);
    resonantFollowBodyBtn.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    resonantFollowBodyBtn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff55dd77).withAlpha (0.15f));
    resonantFollowBodyBtn.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff55dd77).withAlpha (0.5f));
    resonantFollowBodyBtn.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff55dd77));
    resonantFollowBodyBtn.setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
    addChildComponent (resonantFollowBodyBtn);

    // ── Sauce knob (visual only, no APVTS) ────────────────────────────────
    sauceKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    sauceKnob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    sauceKnob.setRange (0.0, 1.0, 0.01);
    sauceKnob.setValue (0.0);
    sauceKnob.setColour (juce::Slider::rotarySliderFillColourId, kSaucePink);
    addChildComponent (sauceKnob);   // hidden until Sauce tab

    // ── Phase offset fader (BODY tab, inside envelope container left side) ──
    phaseOffsetSlider.setSliderStyle (juce::Slider::LinearVertical);
    phaseOffsetSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    phaseOffsetSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff5a5a6a));
    addChildComponent (phaseOffsetSlider);   // hidden until Body tab
    phaseOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "phaseOffset", phaseOffsetSlider);

    // Phase offset value bubble
    phaseBubble.typeface = lnf.interRegular;
    addChildComponent (phaseBubble);
    phaseOffsetSlider.onDragStart   = [this] { phaseBubble.setVisible (true);  updatePhaseBubble(); };
    phaseOffsetSlider.onDragEnd     = [this] { phaseBubble.setVisible (false); };
    phaseOffsetSlider.onValueChange = [this] { if (phaseBubble.isVisible()) updatePhaseBubble(); };

    // ── Output fader (attached to outputGain APVTS param) ────────────────────
    outputSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    outputSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff5a5a6a));
    addAndMakeVisible (outputSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputSlider);

    // ── Output value bubble ───────────────────────────────────────────────────
    outputBubble.typeface = lnf.interRegular;
    addChildComponent (outputBubble);   // hidden until drag
    outputSlider.onDragStart   = [this] { outputBubble.setVisible (true);  updateOutputBubble(); };
    outputSlider.onDragEnd     = [this] { outputBubble.setVisible (false); };
    outputSlider.onValueChange = [this] { if (outputBubble.isVisible()) updateOutputBubble(); };

    // ── Preset combo (visual only) ────────────────────────────────────────────
    presetCombo.addItem ("Init Snare",       1);
    presetCombo.addItem ("Trap Snare",       2);
    presetCombo.addItem ("Techno Snare",     3);
    presetCombo.addItem ("Acoustic Hybrid",  4);
    presetCombo.addItem ("Metallic Snap",    5);
    presetCombo.setSelectedId (1, juce::dontSendNotification);
    presetCombo.setColour (juce::ComboBox::backgroundColourId,  juce::Colour (0xff1E2229));
    presetCombo.setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
    presetCombo.setColour (juce::ComboBox::textColourId,        juce::Colour (0xffffffff));
    presetCombo.setColour (juce::ComboBox::arrowColourId,       juce::Colour (0xff8888aa));
    addAndMakeVisible (presetCombo);

    // ── Envelope mode buttons ─────────────────────────────────────────────────
    auto initModeBtn = [&] (juce::TextButton& btn)
    {
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff1E2229));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1E2229));
        btn.setColour (juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
        addAndMakeVisible (btn);
    };
    noiseAmpBtn .onClick = [this] { setEnvMode (EnvMode::NoiseAmp); };
    initModeBtn (noiseAmpBtn);

    // Size first so resized() computes all bounds before visibility/styling
    setSize (kWinW, kWinH);

    // Now that bounds exist, set tab + envelope mode
    setActiveTab (Tab::Body);

    // Sync initial layer-enabled state to processor
    audioProcessor.transientEnabled.store (tabEnabledFor (Tab::Transient), std::memory_order_relaxed);
    audioProcessor.bodyEnabled     .store (tabEnabledFor (Tab::Body),      std::memory_order_relaxed);
    audioProcessor.resonantEnabled .store (tabEnabledFor (Tab::Resonant),  std::memory_order_relaxed);
    audioProcessor.noiseEnabled    .store (tabEnabledFor (Tab::Noise),     std::memory_order_relaxed);
}

SnareMakerAudioProcessorEditor::~SnareMakerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

// =============================================================================
// setActiveTab  (Phase 6-1)
// =============================================================================

void SnareMakerAudioProcessorEditor::setActiveTab (Tab tab)
{
    activeTab = tab;

    const bool showTransient = (tab == Tab::Transient);
    const bool showBody      = (tab == Tab::Body);
    const bool showNoise     = (tab == Tab::Noise);
    const bool showResonant  = (tab == Tab::Resonant);
    const bool showRoom      = (tab == Tab::Room);

    // Envelope mode buttons
    noiseAmpBtn .setVisible (false);

    // Phase offset fader: visible only on Body tab
    phaseOffsetSlider.setVisible (showBody);
    phaseBubble.setVisible (false);
    if (showBody)
    {
        // Center fader in the 24px kPadX margin of the envelope editor
        constexpr int kEnvPadX = 24;   // matches EnvelopeEditor::kPadX
        constexpr int faderW   = 20;
        constexpr int vInset   = 20;   // top + bottom inset
        const int faderX = envEditorFullBounds.getX() + (kEnvPadX - faderW) / 2;
        phaseOffsetSlider.setBounds (faderX,
                                     envEditorFullBounds.getY() + vInset,
                                     faderW,
                                     envEditorFullBounds.getHeight() - vInset * 2);
    }

    // Envelope editor visible for all layer tabs (waveform overlay)
    envelopeEditor.setVisible (showTransient || showBody || showNoise || showResonant || showRoom);

    // Map tab → waveform layer highlight
    if      (showTransient) envelopeEditor.setActiveLayer (EnvelopeEditor::WaveLayer::Transient);
    else if (showBody)      envelopeEditor.setActiveLayer (EnvelopeEditor::WaveLayer::Body);
    else if (showResonant)  envelopeEditor.setActiveLayer (EnvelopeEditor::WaveLayer::Resonant);
    else if (showNoise)     envelopeEditor.setActiveLayer (EnvelopeEditor::WaveLayer::Noise);
    else if (showRoom)      envelopeEditor.setActiveLayer (EnvelopeEditor::WaveLayer::Body);

    // Body and Room share WaveLayer::Body — manage sample flag + amp envelope on tab switch
    if (showBody)
    {
        envelopeEditor.clearLayerSampleFlag (EnvelopeEditor::WaveLayer::Body);
        envelopeEditor.setLayerAmpEnvelope (EnvelopeEditor::WaveLayer::Body,
                                             audioProcessor.bodyAmpEnvelope);
    }
    else if (showRoom)
    {
        envelopeEditor.setLayerAmpEnvelope (EnvelopeEditor::WaveLayer::Body,
                                             audioProcessor.roomAmpEnvelope);
        if (audioProcessor.roomIRBuffer.getNumSamples() > 0)
            envelopeEditor.setLayerSampleData (
                EnvelopeEditor::WaveLayer::Body,
                audioProcessor.roomIRBuffer.getReadPointer (0),
                audioProcessor.roomIRBuffer.getNumSamples());
    }

    // Transient / Resonant / Noise / Room: 80% width (side panel fills the rest)
    if (showTransient)
    {
        envelopeEditor.setBounds (transientWaveBounds);
    }
    else if (showResonant || showNoise || showRoom)
    {
        auto b = envEditorFullBounds;
        b.setWidth (b.getWidth() * 4 / 5 - 30);
        envelopeEditor.setBounds (b);
    }
    else
    {
        envelopeEditor.setBounds (envEditorFullBounds);
    }

    // Noise filter visualizer: visible only in Noise tab + GEN mode
    noiseFilterVis.setVisible (showNoise && noiseSrc == NoiseSrc::Gen);
    if (showNoise)
        noiseFilterVis.setBounds (noiseFilterBounds);

    // Noise sample controls: visible only in Noise tab + SAMPLE mode
    const bool noiseSample = showNoise && noiseSrc == NoiseSrc::Sample;
    noiseSampleFilterVis.setVisible (noiseSample);
    noiseSamplePrevBtn.setVisible (noiseSample);
    noiseSampleNextBtn.setVisible (noiseSample);
    if (noiseSample)
        noiseSampleFilterVis.setBounds (noiseSampleFilterBounds);

    // Transient filter visualizer + sample buttons: visible only in Transient tab
    transientFilterVis.setVisible (showTransient);
    transientPrevBtn.setVisible (showTransient);
    transientNextBtn.setVisible (showTransient);
    if (showTransient)
        transientFilterVis.setBounds (transientFilterBounds);

    // Room filter visualizer + IR buttons: visible only in Room tab
    roomFilterVis.setVisible (showRoom);
    roomPrevBtn.setVisible (showRoom);
    roomNextBtn.setVisible (showRoom);
    if (showRoom)
        roomFilterVis.setBounds (roomFilterBounds);

    // Resonant filter visualizer + sample buttons + follow body: visible only in Resonant tab
    resonantFilterVis.setVisible (showResonant);
    resonantPrevBtn.setVisible (showResonant);
    resonantNextBtn.setVisible (showResonant);
    resonantFollowBodyBtn.setVisible (showResonant);
    if (showResonant)
        resonantFilterVis.setBounds (resonantFilterBounds);

    // Sauce knob: visible only on Sauce tab
    const bool showSauce = (tab == Tab::Sauce);
    sauceKnob.setVisible (showSauce);
    if (showSauce)
    {
        constexpr int knobSize = 180;
        const int knobX = drumAreaBounds.getCentreX() - knobSize / 2;
        const int knobY = drumAreaBounds.getCentreY() - knobSize / 2 - 16;
        sauceKnob.setBounds (knobX, knobY, knobSize, knobSize);
    }

    // Default envelope for each tab
    if (showTransient)     setEnvMode (EnvMode::TransientAmp);
    else if (showBody)     setEnvMode (EnvMode::Pitch);
    else if (showNoise)    setEnvMode (EnvMode::NoiseAmp);
    else if (showResonant) setEnvMode (EnvMode::ResonantAmp);
    else if (showRoom)     setEnvMode (EnvMode::RoomAmp);
    // Sauce: no envelope / no mode buttons (placeholder only)

    // Drag & drop: enabled for sample-accepting tabs only
    envelopeEditor.setDragDropEnabled (showTransient || showResonant || showNoise || showRoom);

    repaint();
}

// =============================================================================
// setEnvMode
// =============================================================================

void SnareMakerAudioProcessorEditor::setEnvMode (EnvMode mode)
{
    envMode = mode;

    auto styleBtn = [&] (juce::TextButton& btn, bool active, juce::Colour /*accent*/)
    {
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff1E2229));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1E2229));
        btn.setColour (juce::TextButton::textColourOffId,
                       active ? juce::Colours::white : juce::Colour (0xff555566));
        btn.setColour (juce::TextButton::textColourOnId,
                       active ? juce::Colours::white : juce::Colour (0xff555566));
    };

    styleBtn (noiseAmpBtn,  mode == EnvMode::NoiseAmp, kNoiseRed);

    noiseAmpBtn .repaint();
    repaint();

    switch (mode)
    {
        case EnvMode::Pitch:        envelopeEditor.setEnvelope (audioProcessor.pitchEnvelope);        break;
        case EnvMode::TransientAmp: envelopeEditor.setEnvelope (audioProcessor.transientAmpEnvelope); break;
        case EnvMode::BodyAmp:      envelopeEditor.setEnvelope (audioProcessor.bodyAmpEnvelope);      break;
        case EnvMode::NoiseAmp:     envelopeEditor.setEnvelope (audioProcessor.noiseAmpEnvelope);     break;
        case EnvMode::ResonantAmp:  envelopeEditor.setEnvelope (audioProcessor.resonantAmpEnvelope);  break;
        case EnvMode::RoomAmp:      envelopeEditor.setEnvelope (audioProcessor.roomAmpEnvelope);      break;
    }
}

// =============================================================================
// OutputValueBubble
// =============================================================================

void SnareMakerAudioProcessorEditor::OutputValueBubble::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1E2229));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colours::white);
    if (typeface != nullptr)
        g.setFont (juce::Font (juce::FontOptions{}.withTypeface (typeface)).withHeight (11.0f));
    else
        g.setFont (11.0f);
    g.drawText (text, getLocalBounds(), juce::Justification::centred, false);
}

void SnareMakerAudioProcessorEditor::updateOutputBubble()
{
    const double val = outputSlider.getValue();
    outputBubble.text = (val <= outputSlider.getMinimum())
        ? juce::String ("-inf dB")
        : juce::String (val, 1) + " dB";

    const float thumbY = (float) outputSlider.getPositionOfValue (val);
    constexpr int bubbleW = 56, bubbleH = 22, gap = 6;
    outputBubble.setBounds (outputSlider.getX() - bubbleW - gap,
                            outputSlider.getY() + (int) thumbY - bubbleH / 2,
                            bubbleW, bubbleH);
    outputBubble.repaint();
}

void SnareMakerAudioProcessorEditor::updatePhaseBubble()
{
    const int val = (int) phaseOffsetSlider.getValue();
    phaseBubble.text = juce::String (val) + juce::String::charToString (0x00B0);   // degree sign

    const float thumbY = (float) phaseOffsetSlider.getPositionOfValue (phaseOffsetSlider.getValue());
    constexpr int bubbleW = 46, bubbleH = 22, gap = 4;
    phaseBubble.setBounds (phaseOffsetSlider.getRight() + gap,
                           phaseOffsetSlider.getY() + (int) thumbY - bubbleH / 2,
                           bubbleW, bubbleH);
    phaseBubble.repaint();
}

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    // ── Content area with uniform margins ───────────────────────────────────
    auto contentArea = getLocalBounds();
    contentArea.removeFromTop (kHeaderH);
    contentArea.removeFromTop (kUISpacing);       // header → content gap
    contentArea.reduce (kUISpacing, 0);            // left / right margins
    contentArea.removeFromBottom (kUISpacing);     // bottom margin

    // Split: main area | kUISpacing gap | output zone
    drumAreaBounds = contentArea.withWidth (contentArea.getWidth() - kOutputW - kUISpacing);
    // outputZoneBounds set below after tabY / modeBtnY are known

    // ── Preset combo in header (right-aligned) ─────────────────────────────
    {
        constexpr int comboW = 150, comboH = 24, padR = 12;
        presetCombo.setBounds (kWinW - padR - comboW,
                               (kHeaderH - comboH) / 2,
                               comboW, comboH);
    }

    // ── Layout: tabs → envelope → mode selector ──────────────────────────────
    {
        constexpr int btnH = 32;

        // Tabs at top of drum area – two groups with kUISpacing inside each
        const int tabY    = drumAreaBounds.getY();
        const int leftX   = drumAreaBounds.getX();
        const int rightEnd = drumAreaBounds.getRight();
        const int tabStep = kTabW + kUISpacing;

        // Left group (left-aligned): Transient · Body · Resonant · Noise
        transientTabBounds = { leftX,              tabY, kTabW, kTabH };
        bodyTabBounds      = { leftX + tabStep,    tabY, kTabW, kTabH };
        resonantTabBounds  = { leftX + tabStep * 2, tabY, kTabW, kTabH };
        noiseTabBounds     = { leftX + tabStep * 3, tabY, kTabW, kTabH };

        // Right group (right-aligned): Room · Sauce
        sauceTabBounds     = { rightEnd - kTabW,                 tabY, kTabW, kTabH };
        roomTabBounds      = { rightEnd - kTabW * 2 - kUISpacing, tabY, kTabW, kTabH };

        // Envelope editor below tabs (kUISpacing gap)
        const int envTop   = tabY + kTabH + kUISpacing;
        // Mode selector at bottom of drum area
        const int modeBtnY = drumAreaBounds.getBottom() - btnH;
        // Envelope fills space between tabs and mode selector (kUISpacing gaps)
        const int envH     = modeBtnY - kUISpacing - envTop;
        envEditorFullBounds = { leftX, envTop, rightEnd - leftX, envH };
        envelopeEditor.setBounds (envEditorFullBounds);

        // Transient waveform: 80% width (same split as envelope tabs)
        transientWaveBounds = { leftX, envTop,
                                (rightEnd - leftX) * 4 / 5 - 30, envH };

        // BODY tab: AMP|PITCH segmented selector
        const int envL = envEditorFullBounds.getX();
        const int envW = envEditorFullBounds.getWidth();
        envModeBounds = { envL, modeBtnY, envW, btnH };

        // NOISE tab: single button spanning full envelope width
        noiseAmpBtn .setBounds (envL,                    modeBtnY, envW, btnH);

        // Output zone: top aligns with tabs, bottom aligns with mode selector
        outputZoneBounds = { contentArea.getRight() - kOutputW, tabY,
                             kOutputW, (modeBtnY + btnH) - tabY };
    }

    // ── Output fader (centered in output zone, large) ──────────────────────
    {
        constexpr int padTop = 36, padBot = 20, sliderW = 40;
        const int sliderH = outputZoneBounds.getHeight() - padTop - padBot;
        const int sliderX = outputZoneBounds.getCentreX() - sliderW / 2;
        const int sliderY = outputZoneBounds.getY() + padTop;
        outputSlider.setBounds (sliderX, sliderY, sliderW, sliderH);
    }

    // ── Noise filter visualizer bounds (computed from side panel geometry) ──
    {
        const int waveW = envEditorFullBounds.getWidth() * 4 / 5 - 30;
        const int sideX = envEditorFullBounds.getX() + waveW + kUISpacing;
        const int sideW = outputZoneBounds.getX() - sideX - kUISpacing;

        constexpr int innerPad = 6;
        constexpr int selH     = 28;
        constexpr int secGap   = 6;
        constexpr int typeH    = 24;
        constexpr int knobSize = 40;
        constexpr int labelH   = 14;

        const int selX  = sideX + innerPad;
        const int selY  = envEditorFullBounds.getY() + innerPad;
        const int selW  = sideW - innerPad * 2;
        const int typeY = selY + selH + secGap;
        const int knobY = typeY + typeH + secGap + 4;
        const int filtY = knobY + knobSize + labelH + secGap + 4;
        const int filtH = envEditorFullBounds.getBottom() - innerPad - filtY;

        noiseFilterBounds = { selX, filtY, selW, filtH / 2 };

        // Noise SAMPLE panel: same side panel geometry, different vertical layout
        const int smpSelY  = selY + selH + secGap;
        const int smpKnobY = smpSelY + selH + secGap + 4;
        const int smpRow2Y = smpKnobY + knobSize + labelH + secGap;
        const int smpFiltY = smpRow2Y + knobSize + labelH + secGap + 4;
        const int smpFiltH = envEditorFullBounds.getBottom() - innerPad - smpFiltY;

        noiseSampleBtnBounds    = { selX, smpSelY, selW, selH };
        noiseSampleFilterBounds = { selX, smpFiltY, selW, smpFiltH };
    }

    // ── Transient filter visualizer + LOAD SAMPLE bounds ─────────────────────
    {
        const int sideX = transientWaveBounds.getRight() + kUISpacing;
        const int sideW = outputZoneBounds.getX() - sideX - kUISpacing;

        constexpr int innerPad = 6;
        constexpr int selH     = 28;
        constexpr int secGap   = 6;
        constexpr int knobSize = 40;
        constexpr int labelH   = 14;

        const int selX  = sideX + innerPad;
        const int selY  = envEditorFullBounds.getY() + innerPad;
        const int selW  = sideW - innerPad * 2;

        transientSampleBtnBounds = { selX, selY, selW, selH };

        const int knobY = selY + selH + secGap + 4;
        const int filtY = knobY + knobSize + labelH + secGap + 4;
        const int filtH = envEditorFullBounds.getBottom() - innerPad - filtY;

        transientFilterBounds = { selX, filtY, selW, filtH / 2 };
    }

    // ── Room control panel bounds ────────────────────────────────────────────
    {
        const int waveW = envEditorFullBounds.getWidth() * 4 / 5 - 30;
        const int sideX = envEditorFullBounds.getX() + waveW + kUISpacing;
        const int sideW = outputZoneBounds.getX() - sideX - kUISpacing;

        constexpr int innerPad = 6;
        constexpr int selH     = 28;
        constexpr int secGap   = 6;
        constexpr int knobSize = 40;
        constexpr int labelH   = 14;

        const int selX  = sideX + innerPad;
        const int selY  = envEditorFullBounds.getY() + innerPad;
        const int selW  = sideW - innerPad * 2;

        roomIrBtnBounds = { selX, selY, selW, selH };

        const int knobY  = selY + selH + secGap + 4;
        const int row2Y  = knobY + knobSize + labelH + secGap;
        const int filtY  = row2Y + knobSize + labelH + secGap + 4;
        const int filtH  = envEditorFullBounds.getBottom() - innerPad - filtY;

        roomFilterBounds = { selX, filtY, selW, filtH };
    }

    // ── Resonant control panel bounds ────────────────────────────────────────
    {
        const int waveW = envEditorFullBounds.getWidth() * 4 / 5 - 30;
        const int sideX = envEditorFullBounds.getX() + waveW + kUISpacing;
        const int sideW = outputZoneBounds.getX() - sideX - kUISpacing;

        constexpr int innerPad = 6;
        constexpr int selH     = 28;
        constexpr int secGap   = 6;
        constexpr int knobSize = 40;
        constexpr int labelH   = 14;
        constexpr int toggleH  = 24;

        const int selX2  = sideX + innerPad;
        const int selY2  = envEditorFullBounds.getY() + innerPad;
        const int selW2  = sideW - innerPad * 2;

        resonantSampleBtnBounds = { selX2, selY2, selW2, selH };

        const int knobY  = selY2 + selH + secGap + 4;
        const int row2Y  = knobY + knobSize + labelH + secGap;
        const int togY   = row2Y + knobSize + labelH + secGap + 4;
        const int filtY  = togY + toggleH + secGap;
        const int filtH  = envEditorFullBounds.getBottom() - innerPad - filtY;

        resonantFilterBounds = { selX2, filtY, selW2, filtH };
    }
}

// =============================================================================
// Zone hit-test
// =============================================================================

SnareMakerAudioProcessorEditor::Zone
SnareMakerAudioProcessorEditor::zoneAt (juce::Point<int> pos) const noexcept
{
    (void) pos;
    return Zone::None;
}

// =============================================================================
// Mouse events
// =============================================================================

void SnareMakerAudioProcessorEditor::mouseMove (const juce::MouseEvent& e)
{
    const Zone z = zoneAt (e.getPosition());
    if (z != hoveredZone) { hoveredZone = z; repaint(); }
}

void SnareMakerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // ── Right-click: toggle tab enabled state ──────────────────────────────
    if (e.mods.isPopupMenu())
    {
        auto tryToggle = [&] (juce::Rectangle<int> bounds, Tab tab)
        {
            if (bounds.contains (e.getPosition()))
            {
                tabEnabledFor (tab) = !tabEnabledFor (tab);

                constexpr Tab order[] = { Tab::Transient, Tab::Body, Tab::Resonant,
                                          Tab::Noise, Tab::Room, Tab::Sauce };

                if (!tabEnabledFor (tab) && activeTab == tab)
                {
                    // Just disabled the active tab — find another enabled tab
                    bool found = false;
                    for (auto t : order)
                        if (tabEnabledFor (t)) { setActiveTab (t); found = true; break; }

                    // All tabs disabled — hide all tab content
                    if (!found)
                    {
                        envelopeEditor.setVisible (false);
                        noiseAmpBtn   .setVisible (false);
                        noiseFilterVis.setVisible (false);
                        sauceKnob     .setVisible (false);
                    }
                }
                else if (tabEnabledFor (tab))
                {
                    // Just enabled a tab — if it's the only one, activate it
                    bool onlyEnabled = true;
                    for (auto t : order)
                        if (t != tab && tabEnabledFor (t)) { onlyEnabled = false; break; }

                    if (onlyEnabled)
                        setActiveTab (tab);
                }

                // Sync layer-enabled state to processor for audio rendering
                audioProcessor.transientEnabled.store (tabEnabledFor (Tab::Transient), std::memory_order_relaxed);
                audioProcessor.bodyEnabled     .store (tabEnabledFor (Tab::Body),      std::memory_order_relaxed);
                audioProcessor.resonantEnabled .store (tabEnabledFor (Tab::Resonant),  std::memory_order_relaxed);
                audioProcessor.noiseEnabled    .store (tabEnabledFor (Tab::Noise),     std::memory_order_relaxed);

                repaint();
                return true;
            }
            return false;
        };
        if (tryToggle (transientTabBounds, Tab::Transient)) return;
        if (tryToggle (bodyTabBounds,      Tab::Body))      return;
        if (tryToggle (resonantTabBounds,  Tab::Resonant))  return;
        if (tryToggle (noiseTabBounds,     Tab::Noise))     return;
        if (tryToggle (roomTabBounds,      Tab::Room))      return;
        if (tryToggle (sauceTabBounds,     Tab::Sauce))     return;
    }

    // ── Left-click: switch active tab (only if enabled) ─────────────────────
    auto tryTab = [&] (juce::Rectangle<int> bounds, Tab tab)
    {
        if (bounds.contains (e.getPosition()) && activeTab != tab && tabEnabledFor (tab))
        { setActiveTab (tab); return true; }
        return false;
    };
    if (tryTab (transientTabBounds, Tab::Transient)) return;
    if (tryTab (bodyTabBounds,      Tab::Body))      return;
    if (tryTab (resonantTabBounds,  Tab::Resonant))  return;
    if (tryTab (noiseTabBounds,     Tab::Noise))     return;
    if (tryTab (roomTabBounds,      Tab::Room))      return;
    if (tryTab (sauceTabBounds,     Tab::Sauce))     return;

    // ── Noise GEN/SAMPLE selector toggle ────────────────────────────────
    if (activeTab == Tab::Noise && !noiseSrcBounds.isEmpty()
        && noiseSrcBounds.contains (e.getPosition()) && !e.mods.isPopupMenu())
    {
        noiseSrc = (noiseSrc == NoiseSrc::Gen) ? NoiseSrc::Sample : NoiseSrc::Gen;
        const bool genNow = (noiseSrc == NoiseSrc::Gen);
        noiseFilterVis.setVisible (genNow);
        noiseSampleFilterVis.setVisible (!genNow);
        noiseSampleFilterVis.setBounds (noiseSampleFilterBounds);
        noiseSamplePrevBtn.setVisible (!genNow);
        noiseSampleNextBtn.setVisible (!genNow);
        repaint();
        return;
    }

    // ── Body AMP|PITCH selector click ────────────────────────────────────
    if (activeTab == Tab::Body && !envModeBounds.isEmpty()
        && envModeBounds.contains (e.getPosition()) && !e.mods.isPopupMenu())
    {
        const int halfW = envModeBounds.getWidth() / 2;
        const bool clickedLeft = (e.getPosition().x - envModeBounds.getX()) < halfW;
        setEnvMode (clickedLeft ? EnvMode::BodyAmp : EnvMode::Pitch);
        return;
    }

    // ── Noise type selector click ────────────────────────────────────────
    if (activeTab == Tab::Noise && !noiseTypeBounds.isEmpty()
        && noiseTypeBounds.contains (e.getPosition()) && !e.mods.isPopupMenu())
    {
        const float segW = (float) noiseTypeBounds.getWidth() / 5.0f;
        const int idx = (int) ((float) (e.getPosition().x - noiseTypeBounds.getX()) / segW);
        noiseType = static_cast<NoiseType> (juce::jlimit (0, 4, idx));
        repaint();
        return;
    }

    // ── Noise sample browser clicks ─────────────────────────────────────
    if (activeTab == Tab::Noise && noiseSrc == NoiseSrc::Sample && !e.mods.isPopupMenu())
    {
        if (!noiseSampleDropBounds.isEmpty()
            && noiseSampleDropBounds.contains (e.getPosition()))
        {
            if (openDropdown == OpenDropdown::NoiseSample)
            {
                openDropdown = OpenDropdown::None;
                repaint();
                return;
            }
            juce::PopupMenu menu;
            menu.addItem (1, "White");
            menu.addItem (2, "Analog");
            menu.addItem (3, "Tape");
            menu.addItem (4, "Dust");
            menu.addItem (5, "Vinyl");
            menu.addItem (6, "Static");
            menu.addItem (7, "Perc Noise");
            menu.addItem (8, "User");
            menu.setLookAndFeel (&lnf);
            openDropdown = OpenDropdown::NoiseSample;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options()
                .withTargetScreenArea (localAreaToGlobal (noiseSampleBtnBounds))
                .withMinimumWidth (noiseSampleBtnBounds.getWidth())
                .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards)
                .withStandardItemHeight (28),
                [this] (int) { openDropdown = OpenDropdown::None; repaint(); });
            return;
        }
    }

    // ── Transient sample browser clicks ─────────────────────────────────
    if (activeTab == Tab::Transient && !e.mods.isPopupMenu())
    {
        if (!transientDropBounds.isEmpty()
            && transientDropBounds.contains (e.getPosition()))
        {
            if (openDropdown == OpenDropdown::TransientSample)
            {
                openDropdown = OpenDropdown::None;
                repaint();
                return;
            }
            juce::PopupMenu menu;
            menu.addItem (1, "Kick");
            menu.addItem (2, "Snare");
            menu.addItem (3, "Percussion");
            menu.addItem (4, "FX");
            menu.setLookAndFeel (&lnf);
            openDropdown = OpenDropdown::TransientSample;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options()
                .withTargetScreenArea (localAreaToGlobal (transientSampleBtnBounds))
                .withMinimumWidth (transientSampleBtnBounds.getWidth())
                .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards)
                .withStandardItemHeight (28),
                [this] (int) { openDropdown = OpenDropdown::None; repaint(); });
            return;
        }
    }

    // ── Resonant sample browser clicks ─────────────────────────────────
    if (activeTab == Tab::Resonant && !e.mods.isPopupMenu())
    {
        if (!resonantDropBounds.isEmpty()
            && resonantDropBounds.contains (e.getPosition()))
        {
            if (openDropdown == OpenDropdown::ResonantSample)
            {
                openDropdown = OpenDropdown::None;
                repaint();
                return;
            }
            juce::PopupMenu menu;
            menu.addItem (1, "Metal");
            menu.addItem (2, "Shell");
            menu.addItem (3, "Spring");
            menu.addItem (4, "Plate");
            menu.addItem (5, "Analog");
            menu.addItem (6, "FM");
            menu.addItem (7, "User");
            menu.setLookAndFeel (&lnf);
            openDropdown = OpenDropdown::ResonantSample;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options()
                .withTargetScreenArea (localAreaToGlobal (resonantSampleBtnBounds))
                .withMinimumWidth (resonantSampleBtnBounds.getWidth())
                .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards)
                .withStandardItemHeight (28),
                [this] (int) { openDropdown = OpenDropdown::None; repaint(); });
            return;
        }
    }

    // ── Room IR browser clicks ───────────────────────────────────────────
    if (activeTab == Tab::Room && !e.mods.isPopupMenu())
    {
        if (!roomDropBounds.isEmpty()
            && roomDropBounds.contains (e.getPosition()))
        {
            if (openDropdown == OpenDropdown::RoomIr)
            {
                openDropdown = OpenDropdown::None;
                repaint();
                return;
            }
            juce::PopupMenu menu;
            menu.addItem (1, "Small Room");
            menu.addItem (2, "Studio");
            menu.addItem (3, "Plate");
            menu.addItem (4, "Chamber");
            menu.addItem (5, "Hall");
            menu.addItem (6, "Drum Room");
            menu.setLookAndFeel (&lnf);
            openDropdown = OpenDropdown::RoomIr;
            repaint();
            menu.showMenuAsync (juce::PopupMenu::Options()
                .withTargetScreenArea (localAreaToGlobal (roomIrBtnBounds))
                .withMinimumWidth (roomIrBtnBounds.getWidth())
                .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards)
                .withStandardItemHeight (28),
                [this] (int) { openDropdown = OpenDropdown::None; repaint(); });
            return;
        }
    }

    const Zone z = zoneAt (e.getPosition());
    if (z != Zone::None && z != activeZone) { activeZone = z; repaint(); }
}

void SnareMakerAudioProcessorEditor::mouseExit (const juce::MouseEvent&)
{
    if (hoveredZone != Zone::None) { hoveredZone = Zone::None; repaint(); }
}

// =============================================================================
// paint
// =============================================================================

void SnareMakerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBgWindow);

    paintHeader (g);

    paintZone (g, outputZoneBounds, Zone::Output, "OUTPUT", kTextMuted,  {}, true);

    // ── dB scale markings alongside output fader ─────────────────────────
    {
        const float slTop    = (float) outputSlider.getY() + 9.0f;   // top of travel (kThumbR)
        const float slBot    = (float) outputSlider.getBottom() - 9.0f;
        const double dbMin   = outputSlider.getMinimum();             // -24
        const double dbMax   = outputSlider.getMaximum();             // +6
        const float  travel  = slBot - slTop;

        const int labelX = outputZoneBounds.getX() + 4;
        const int labelW = outputSlider.getX() - labelX - 2;

        g.setFont (lnf.interRegularFont (8.0f));
        g.setColour (kTextDim);

        struct Mark { double db; const char* text; };
        const Mark marks[] = {
            {  6, "+6" }, {  3, "+3" }, {  0,  "0" }, { -3, "-3" },
            { -6, "-6" }, { -10, "-10" }, { -20, "-20" }
        };

        for (auto& m : marks)
        {
            const float norm = (float) ((m.db - dbMin) / (dbMax - dbMin));
            const float yPos = slBot - norm * travel;
            g.drawText (m.text, labelX, (int) yPos - 5, labelW, 10,
                        juce::Justification::centredRight, false);
        }

        // -inf at the very bottom (slider minimum)
        g.drawText ("-inf", labelX, (int) slBot - 5, labelW, 10,
                    juce::Justification::centredRight, false);
    }

    paintDrumArea (g, drumAreaBounds);
    paintTabs (g);

    // ── Body tab: AMP | PITCH segmented selector ─────────────────────────
    if (activeTab == Tab::Body && !envModeBounds.isEmpty())
    {
        const auto b  = envModeBounds;
        const int  hw = b.getWidth() / 2;
        const bool ampActive = (envMode == EnvMode::BodyAmp);

        // Left half (AMP)
        {
            juce::Path clip;
            clip.addRoundedRectangle ((float) b.getX(), (float) b.getY(),
                                      (float) hw, (float) b.getHeight(),
                                      8.0f, 8.0f, true, false, true, false);
            g.saveState();
            g.reduceClipRegion (clip);
            g.setColour (ampActive ? juce::Colour (0xff1E2229) : juce::Colour (0xff181C22));
            g.fillRect (b.getX(), b.getY(), hw, b.getHeight());
            g.restoreState();
        }

        // Right half (PITCH)
        {
            juce::Path clip;
            clip.addRoundedRectangle ((float) (b.getX() + hw), (float) b.getY(),
                                      (float) (b.getWidth() - hw), (float) b.getHeight(),
                                      8.0f, 8.0f, false, true, false, true);
            g.saveState();
            g.reduceClipRegion (clip);
            g.setColour (ampActive ? juce::Colour (0xff181C22) : juce::Colour (0xff1E2229));
            g.fillRect (b.getX() + hw, b.getY(), b.getWidth() - hw, b.getHeight());
            g.restoreState();
        }

        // Divider
        g.setColour (juce::Colour (0xff363E4A));
        g.fillRect ((float) (b.getX() + hw), (float) b.getY(), 1.0f, (float) b.getHeight());

        // Labels
        g.setFont (lnf.interMediumFont (10.0f));

        g.setColour (ampActive ? juce::Colours::white : juce::Colour (0xff555566));
        g.drawText ("AMP", b.getX(), b.getY(), hw, b.getHeight(),
                    juce::Justification::centred, false);

        g.setColour (ampActive ? juce::Colour (0xff555566) : juce::Colours::white);
        g.drawText ("PITCH", b.getX() + hw, b.getY(), b.getWidth() - hw, b.getHeight(),
                    juce::Justification::centred, false);
    }

    // Global grey-out when every layer tab is disabled
    if (!tabEnabledFor (Tab::Transient) && !tabEnabledFor (Tab::Body)
        && !tabEnabledFor (Tab::Resonant) && !tabEnabledFor (Tab::Noise)
        && !tabEnabledFor (Tab::Room) && !tabEnabledFor (Tab::Sauce))
    {
        g.setColour (juce::Colour (0xbb101318));
        g.fillRect (getLocalBounds());
    }
}

// =============================================================================
// paintHeader
// =============================================================================

void SnareMakerAudioProcessorEditor::paintHeader (juce::Graphics& g) const
{
    const int w = getWidth();

    g.setColour (kBgPanel);
    g.fillRect (0, 0, w, kHeaderH);

    // Small logo text, right-aligned before the preset combo
    constexpr int logoTextW = 95;
    constexpr int padR = 12;
    constexpr int comboW = 150;
    const int logoX = w - padR - comboW - 8 - logoTextW;

    g.setColour (kTextMuted);
    g.setFont (lnf.interMediumFont (13.0f));
    g.drawText ("Snare Maker", logoX, 0, logoTextW, kHeaderH,
                juce::Justification::centredRight, false);
}

// =============================================================================
// paintTabs  (Phase 6-1)
// =============================================================================

void SnareMakerAudioProcessorEditor::paintTabs (juce::Graphics& g) const
{
    auto drawTab = [&] (juce::Rectangle<int> bounds, const juce::String& text,
                        juce::Colour accent, bool isActive, bool enabled)
    {
        const auto bf = bounds.toFloat().reduced (0.5f);
        constexpr float cr = 8.0f;

        // Background fill
        const juce::Colour bgCol = !enabled  ? juce::Colour (0xff101318)
                                   : isActive ? juce::Colour (0xff1E2229)
                                              : juce::Colour (0xff1E2229);
        g.setColour (bgCol);
        g.fillRoundedRectangle (bf, cr);

        // Outline stroke (active tab only)
        if (isActive)
        {
            g.setColour (accent.withAlpha (enabled ? 0.35f : 0.12f));
            g.drawRoundedRectangle (bf, cr, 1.0f);
        }

        // Text
        const juce::Colour col = enabled ? accent : accent.withSaturation (0.0f).withAlpha (0.18f);
        const float alpha = isActive ? (enabled ? 1.0f : 0.30f) : (enabled ? 0.35f : 0.12f);
        g.setColour (col.withAlpha (alpha));
        g.setFont (lnf.interMediumFont (10.0f));
        g.drawText (text, bounds, juce::Justification::centred, false);

    };

    drawTab (transientTabBounds, "TRANSIENT", kTransientOrng, activeTab == Tab::Transient, tabEnabledFor (Tab::Transient));
    drawTab (bodyTabBounds,      "BODY",      kPitchBlue,     activeTab == Tab::Body,      tabEnabledFor (Tab::Body));
    drawTab (resonantTabBounds,  "RESONANT",  kResonantGrn,   activeTab == Tab::Resonant,  tabEnabledFor (Tab::Resonant));
    drawTab (noiseTabBounds,     "NOISE",     kNoiseRed,      activeTab == Tab::Noise,     tabEnabledFor (Tab::Noise));
    drawTab (roomTabBounds,      "ROOM",      kRoomPurple,    activeTab == Tab::Room,      tabEnabledFor (Tab::Room));
    drawTab (sauceTabBounds,     "SAUCE",     kSaucePink,     activeTab == Tab::Sauce,     tabEnabledFor (Tab::Sauce));
}

// =============================================================================
// paintZone
// =============================================================================

void SnareMakerAudioProcessorEditor::paintZone (
    juce::Graphics&          g,
    juce::Rectangle<int>     bounds,
    Zone                     zone,
    const juce::String&      title,
    juce::Colour             accent,
    const juce::StringArray& hints,
    bool                     hasControls) const
{
    const bool isHovered = (hoveredZone == zone);
    const bool isActive  = (activeZone  == zone);

    // Background
    g.setColour (isActive ? kBgPanelAct : isHovered ? kBgPanelHov : kBgPanel);
    g.fillRoundedRectangle (bounds.toFloat().reduced (2.0f), 8.0f);

    // Title
    const float titleAlpha = isActive ? 1.0f : isHovered ? 0.80f : 0.50f;
    g.setColour (kTextMuted.withAlpha (titleAlpha));
    g.setFont (lnf.interRegularFont (11.0f));
    g.drawText (title, bounds.getX() + 10, bounds.getY() + 14,
                bounds.getWidth() - 14, 14, juce::Justification::centred, false);

    // Placeholder hints (only when no real controls yet)
    if (!hasControls)
    {
        g.setColour (isActive ? kTextMuted : kTextDim);
        g.setFont (lnf.interRegularFont (9.5f));

        const int hintStartY = bounds.getY() + 40;
        for (int i = 0; i < hints.size(); ++i)
            g.drawText (hints[i],
                        bounds.getX() + 8, hintStartY + i * 22,
                        bounds.getWidth() - 16, 16,
                        juce::Justification::centred, false);

        if (!isActive)
        {
            const float cAlpha = isHovered ? 0.45f : 0.18f;
            g.setColour (accent.withAlpha (cAlpha));
            g.setFont (lnf.interRegularFont (8.5f));
            g.drawText ("CLICK TO EXPAND",
                        bounds.getX() + 8, bounds.getBottom() - 20,
                        bounds.getWidth() - 16, 14,
                        juce::Justification::centred, false);
        }
    }
}

// =============================================================================
// paintDrumArea
// =============================================================================

void SnareMakerAudioProcessorEditor::paintDrumArea (
    juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (activeTab == Tab::Sauce ? kBgPanel : kBgDrum);
    g.fillRect (area);

    if (activeTab != Tab::Sauce)
        paintSnareDrum (g, area);

    // Transient / Resonant / Noise / Room: side panel fills from waveform right edge to Output
    if (activeTab == Tab::Transient || activeTab == Tab::Resonant || activeTab == Tab::Noise || activeTab == Tab::Room)
    {
        const int waveW = envEditorFullBounds.getWidth() * 4 / 5 - 30;
        const int sideX = envEditorFullBounds.getX() + waveW + kUISpacing;
        const int sideW = outputZoneBounds.getX() - sideX - kUISpacing;

        const auto sideBox = juce::Rectangle<int> (sideX, envEditorFullBounds.getY(),
                                                   sideW, envEditorFullBounds.getHeight()).toFloat();
        g.setColour (kBgPanel);
        g.fillRoundedRectangle (sideBox, 8.0f);

        // Transient tab: LOAD SAMPLE button + WIDTH/PITCH knobs
        if (activeTab == Tab::Transient)
        {
            constexpr int innerPad = 6;
            constexpr int selH     = 28;
            constexpr int secGap   = 6;
            constexpr int knobSize = 40;
            constexpr int labelH   = 14;
            const int selX2 = sideX + innerPad;
            const int selY2 = envEditorFullBounds.getY() + innerPad;
            const int selW2 = sideW - innerPad * 2;

            // ── LOAD SAMPLE inline browser: [ < ] LOAD SAMPLE ▼ [ > ]
            transientSampleBtnBounds = { selX2, selY2, selW2, selH };

            g.setColour (juce::Colour (0xff2A3038));
            {
                const bool dropOpen = (openDropdown == OpenDropdown::TransientSample);
                juce::Path containerPath;
                containerPath.addRoundedRectangle ((float) selX2, (float) selY2,
                                                   (float) selW2, (float) selH,
                                                   8.0f, 8.0f,
                                                   true, true,          // top-left, top-right rounded
                                                   !dropOpen, !dropOpen); // bottom corners flat when open
                g.fillPath (containerPath);
            }

            constexpr int arrowW = 22;
            const int prevX = selX2;
            const int nextX = selX2 + selW2 - arrowW;
            const int midX  = prevX + arrowW;
            const int midW  = selW2 - arrowW * 2;

            // Dropdown hit-test (centre region)
            transientDropBounds = { midX, selY2, midW, selH };

            // Position real button components
            transientPrevBtn.setBounds (prevX, selY2, arrowW, selH);
            transientNextBtn.setBounds (nextX, selY2, arrowW, selH);

            // Divider lines
            g.setColour (juce::Colour (0xff363E4A));
            g.fillRect ((float) (prevX + arrowW), (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);
            g.fillRect ((float) nextX, (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);

            // Centre label
            g.setColour (juce::Colours::white);
            g.setFont (lnf.interMediumFont (10.0f));
            {
                const auto label = audioProcessor.transientSamplePath.isEmpty()
                    ? juce::String ("LOAD SAMPLE")
                    : juce::File (audioProcessor.transientSamplePath).getFileName();
                g.drawText (label, midX, selY2, midW - 14, selH,
                            juce::Justification::centred, false);
            }

            // Dropdown triangle (drawn via path)
            {
                const float triX = (float) (midX + midW) - 16.0f;
                const float triY = (float) selY2 + (float) selH * 0.5f - 2.0f;
                juce::Path tri;
                tri.addTriangle (triX, triY, triX + 7.0f, triY, triX + 3.5f, triY + 5.0f);
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.fillPath (tri);
            }

            // ── WIDTH & PITCH knobs ──────────────────────────────────
            const int knobY = selY2 + selH + secGap + 4;
            const int knobArea = selW2 / 2;
            const int knob1X = selX2 + knobArea / 2 - knobSize / 2;
            const int knob2X = selX2 + knobArea + knobArea / 2 - knobSize / 2;

            // Width knob
            g.setColour (juce::Colour (0xff2A3038));
            g.fillEllipse ((float) knob1X, (float) knobY,
                           (float) knobSize, (float) knobSize);
            g.setColour (juce::Colour (0xff363E4A));
            g.drawEllipse ((float) knob1X + 0.5f, (float) knobY + 0.5f,
                           (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            {
                const float kcx = (float) knob1X + (float) knobSize * 0.5f;
                g.drawLine (kcx, (float) knobY + (float) knobSize * 0.5f,
                            kcx, (float) knobY + 4.0f, 1.5f);
            }
            g.setColour (kTextMuted);
            g.setFont (lnf.interRegularFont (9.0f));
            g.drawText ("Width", knob1X - 5, knobY + knobSize + 2,
                        knobSize + 10, labelH, juce::Justification::centred, false);

            // Pitch knob
            g.setColour (juce::Colour (0xff2A3038));
            g.fillEllipse ((float) knob2X, (float) knobY,
                           (float) knobSize, (float) knobSize);
            g.setColour (juce::Colour (0xff363E4A));
            g.drawEllipse ((float) knob2X + 0.5f, (float) knobY + 0.5f,
                           (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            {
                const float kcx = (float) knob2X + (float) knobSize * 0.5f;
                g.drawLine (kcx, (float) knobY + (float) knobSize * 0.5f,
                            kcx, (float) knobY + 4.0f, 1.5f);
            }
            g.setColour (kTextMuted);
            g.drawText ("Pitch", knob2X - 5, knobY + knobSize + 2,
                        knobSize + 10, labelH, juce::Justification::centred, false);

            // Filter section: handled by transientFilterVis component
        }

        // Room tab: IR loader + SIZE/DECAY/PRE-DELAY/WIDTH knobs + filter
        if (activeTab == Tab::Room)
        {
            constexpr int innerPad = 6;
            constexpr int selH     = 28;
            constexpr int secGap   = 6;
            constexpr int knobSize = 40;
            constexpr int labelH   = 14;
            const int selX2 = sideX + innerPad;
            const int selY2 = envEditorFullBounds.getY() + innerPad;
            const int selW2 = sideW - innerPad * 2;

            // ── LOAD IR inline browser: [ < ] LOAD IR ▼ [ > ]
            roomIrBtnBounds = { selX2, selY2, selW2, selH };

            g.setColour (juce::Colour (0xff2A3038));
            {
                const bool dropOpen = (openDropdown == OpenDropdown::RoomIr);
                juce::Path containerPath;
                containerPath.addRoundedRectangle ((float) selX2, (float) selY2,
                                                   (float) selW2, (float) selH,
                                                   8.0f, 8.0f,
                                                   true, true,
                                                   !dropOpen, !dropOpen);
                g.fillPath (containerPath);
            }

            constexpr int arrowW = 22;
            const int prevX = selX2;
            const int nextX = selX2 + selW2 - arrowW;
            const int midX  = prevX + arrowW;
            const int midW  = selW2 - arrowW * 2;

            roomDropBounds = { midX, selY2, midW, selH };

            // Position button components
            roomPrevBtn.setBounds (prevX, selY2, arrowW, selH);
            roomNextBtn.setBounds (nextX, selY2, arrowW, selH);

            // Divider lines
            g.setColour (juce::Colour (0xff363E4A));
            g.fillRect ((float) (prevX + arrowW), (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);
            g.fillRect ((float) nextX, (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);

            // Centre label
            g.setColour (juce::Colours::white);
            g.setFont (lnf.interMediumFont (10.0f));
            {
                const auto label = audioProcessor.roomIRPath.isEmpty()
                    ? juce::String ("LOAD IR")
                    : juce::File (audioProcessor.roomIRPath).getFileName();
                g.drawText (label, midX, selY2, midW - 14, selH,
                            juce::Justification::centred, false);
            }

            // Dropdown triangle
            {
                const float triX = (float) (midX + midW) - 16.0f;
                const float triY = (float) selY2 + (float) selH * 0.5f - 2.0f;
                juce::Path tri;
                tri.addTriangle (triX, triY, triX + 7.0f, triY, triX + 3.5f, triY + 5.0f);
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.fillPath (tri);
            }

            // ── 2×2 knob grid: SIZE / DECAY / PRE-DELAY / WIDTH ─────────
            const int knobY  = selY2 + selH + secGap + 4;
            const int knobArea = selW2 / 2;
            const int k1X = selX2 + knobArea / 2 - knobSize / 2;
            const int k2X = selX2 + knobArea + knobArea / 2 - knobSize / 2;
            const int row2Y = knobY + knobSize + labelH + secGap;

            auto drawKnob = [&] (int kx, int ky, const char* label)
            {
                g.setColour (juce::Colour (0xff2A3038));
                g.fillEllipse ((float) kx, (float) ky,
                               (float) knobSize, (float) knobSize);
                g.setColour (juce::Colour (0xff363E4A));
                g.drawEllipse ((float) kx + 0.5f, (float) ky + 0.5f,
                               (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                {
                    const float kcx = (float) kx + (float) knobSize * 0.5f;
                    g.drawLine (kcx, (float) ky + (float) knobSize * 0.5f,
                                kcx, (float) ky + 4.0f, 1.5f);
                }
                g.setColour (kTextMuted);
                g.setFont (lnf.interRegularFont (9.0f));
                g.drawText (label, kx - 10, ky + knobSize + 2,
                            knobSize + 20, labelH, juce::Justification::centred, false);
            };

            drawKnob (k1X, knobY, "Size");
            drawKnob (k2X, knobY, "Decay");
            drawKnob (k1X, row2Y, "Pre-Delay");
            drawKnob (k2X, row2Y, "Width");

            // Filter section: handled by roomFilterVis component
        }

        // Resonant tab: LOAD RESONANCE + PITCH/DECAY/WIDTH/DRIVE knobs + FOLLOW BODY toggle + filter
        if (activeTab == Tab::Resonant)
        {
            constexpr int innerPad = 6;
            constexpr int selH     = 28;
            constexpr int secGap   = 6;
            constexpr int knobSize = 40;
            constexpr int labelH   = 14;
            constexpr int toggleH  = 24;
            const int selX2 = sideX + innerPad;
            const int selY2 = envEditorFullBounds.getY() + innerPad;
            const int selW2 = sideW - innerPad * 2;

            // ── LOAD RESONANCE inline browser: [ < ] LOAD RESONANCE ▼ [ > ]
            resonantSampleBtnBounds = { selX2, selY2, selW2, selH };

            g.setColour (juce::Colour (0xff2A3038));
            {
                const bool dropOpen = (openDropdown == OpenDropdown::ResonantSample);
                juce::Path containerPath;
                containerPath.addRoundedRectangle ((float) selX2, (float) selY2,
                                                   (float) selW2, (float) selH,
                                                   8.0f, 8.0f,
                                                   true, true,
                                                   !dropOpen, !dropOpen);
                g.fillPath (containerPath);
            }

            constexpr int arrowW = 22;
            const int prevX = selX2;
            const int nextX = selX2 + selW2 - arrowW;
            const int midX  = prevX + arrowW;
            const int midW  = selW2 - arrowW * 2;

            resonantDropBounds = { midX, selY2, midW, selH };

            resonantPrevBtn.setBounds (prevX, selY2, arrowW, selH);
            resonantNextBtn.setBounds (nextX, selY2, arrowW, selH);

            // Divider lines
            g.setColour (juce::Colour (0xff363E4A));
            g.fillRect ((float) (prevX + arrowW), (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);
            g.fillRect ((float) nextX, (float) selY2 + 5.0f,
                        1.0f, (float) selH - 10.0f);

            // Centre label
            g.setColour (juce::Colours::white);
            g.setFont (lnf.interMediumFont (10.0f));
            {
                const auto label = audioProcessor.resonantSamplePath.isEmpty()
                    ? juce::String ("LOAD RESONANCE")
                    : juce::File (audioProcessor.resonantSamplePath).getFileName();
                g.drawText (label, midX, selY2, midW - 14, selH,
                            juce::Justification::centred, false);
            }

            // Dropdown triangle
            {
                const float triX = (float) (midX + midW) - 16.0f;
                const float triY = (float) selY2 + (float) selH * 0.5f - 2.0f;
                juce::Path tri;
                tri.addTriangle (triX, triY, triX + 7.0f, triY, triX + 3.5f, triY + 5.0f);
                g.setColour (juce::Colours::white.withAlpha (0.7f));
                g.fillPath (tri);
            }

            // ── 2×2 knob grid: PITCH / DECAY / WIDTH / DRIVE ─────────
            const int knobY  = selY2 + selH + secGap + 4;
            const int knobArea = selW2 / 2;
            const int k1X = selX2 + knobArea / 2 - knobSize / 2;
            const int k2X = selX2 + knobArea + knobArea / 2 - knobSize / 2;
            const int row2Y = knobY + knobSize + labelH + secGap;

            auto drawKnob = [&] (int kx, int ky, const char* label)
            {
                g.setColour (juce::Colour (0xff2A3038));
                g.fillEllipse ((float) kx, (float) ky,
                               (float) knobSize, (float) knobSize);
                g.setColour (juce::Colour (0xff363E4A));
                g.drawEllipse ((float) kx + 0.5f, (float) ky + 0.5f,
                               (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                {
                    const float kcx = (float) kx + (float) knobSize * 0.5f;
                    g.drawLine (kcx, (float) ky + (float) knobSize * 0.5f,
                                kcx, (float) ky + 4.0f, 1.5f);
                }
                g.setColour (kTextMuted);
                g.setFont (lnf.interRegularFont (9.0f));
                g.drawText (label, kx - 10, ky + knobSize + 2,
                            knobSize + 20, labelH, juce::Justification::centred, false);
            };

            drawKnob (k1X, knobY, "Pitch");
            drawKnob (k2X, knobY, "Decay");
            drawKnob (k1X, row2Y, "Width");
            drawKnob (k2X, row2Y, "Drive");

            // ── FOLLOW BODY toggle button ─────────────────────────────
            const int togY = row2Y + knobSize + labelH + secGap + 4;
            resonantFollowBodyBtn.setBounds (selX2, togY, selW2, toggleH);

            // Filter section: handled by resonantFilterVis component
        }

        // Noise tab: compact GEN / SAMPLE selector at top of side panel
        if (activeTab == Tab::Noise)
        {
            constexpr int innerPad = 6;
            constexpr int selH     = 28;
            const int selX = sideX + innerPad;
            const int selY = envEditorFullBounds.getY() + innerPad;
            const int selW = sideW - innerPad * 2;
            const int halfW = selW / 2;

            // Store bounds for hit-testing
            noiseSrcBounds = { selX, selY, selW, selH };

            const bool genActive = (noiseSrc == NoiseSrc::Gen);

            // Left half (GEN) — clip to left rounded rect
            {
                juce::Path leftClip;
                leftClip.addRoundedRectangle ((float) selX, (float) selY,
                                              (float) halfW, (float) selH,
                                              8.0f, 8.0f, true, false, true, false);
                g.saveState();
                g.reduceClipRegion (leftClip);
                g.setColour (genActive ? juce::Colour (0xff2A3038) : juce::Colour (0xff181D24));
                g.fillRect (selX, selY, halfW, selH);
                g.restoreState();
            }

            // Right half (SAMPLE) — clip to right rounded rect
            {
                juce::Path rightClip;
                rightClip.addRoundedRectangle ((float) (selX + halfW), (float) selY,
                                               (float) (selW - halfW), (float) selH,
                                               8.0f, 8.0f, false, true, false, true);
                g.saveState();
                g.reduceClipRegion (rightClip);
                g.setColour (genActive ? juce::Colour (0xff181D24) : juce::Colour (0xff2A3038));
                g.fillRect (selX + halfW, selY, selW - halfW, selH);
                g.restoreState();
            }

            // Vertical divider
            const float divX = (float) selX + (float) halfW;
            g.setColour (juce::Colour (0xff363E4A));
            g.fillRect (divX, (float) selY, 1.0f, (float) selH);

            // Labels
            g.setFont (lnf.interMediumFont (10.0f));

            g.setColour (genActive ? juce::Colours::white : juce::Colour (0xff555566));
            g.drawText ("GEN", selX, selY, halfW, selH,
                        juce::Justification::centred, false);

            g.setColour (genActive ? juce::Colour (0xff555566) : juce::Colours::white);
            g.drawText ("SAMPLE", selX + halfW, selY, selW - halfW, selH,
                        juce::Justification::centred, false);

            constexpr int secGap = 6;

            if (genActive)
            {
                // ── Noise Type selector ─────────────────────────────────
                constexpr int typeH = 24;
                const int typeY = selY + selH + secGap;
                noiseTypeBounds = { selX, typeY, selW, typeH };

                constexpr int kNumTypes = 5;
                const char* typeNames[] = { "WHITE", "PINK", "BROWN", "BLUE", "VIOLET" };
                const int typeIdx = static_cast<int> (noiseType);

                g.setColour (juce::Colour (0xff181D24));
                g.fillRoundedRectangle ((float) selX, (float) typeY,
                                        (float) selW, (float) typeH, 8.0f);

                g.setFont (lnf.interMediumFont (8.5f));
                const float segW = (float) selW / (float) kNumTypes;
                for (int i = 0; i < kNumTypes; ++i)
                {
                    const float segX = (float) selX + segW * (float) i;
                    const bool isTypeActive = (i == typeIdx);

                    if (isTypeActive)
                    {
                        juce::Path segClip;
                        segClip.addRoundedRectangle ((float) selX, (float) typeY,
                                                     (float) selW, (float) typeH, 8.0f);
                        g.saveState();
                        g.reduceClipRegion (segClip);
                        g.setColour (juce::Colour (0xff2A3038));
                        g.fillRect (segX, (float) typeY, segW, (float) typeH);
                        g.restoreState();
                    }

                    g.setColour (isTypeActive ? juce::Colours::white : juce::Colour (0xff555566));
                    g.drawText (typeNames[i], (int) segX, typeY, (int) segW, typeH,
                                juce::Justification::centred, false);
                }

                // ── Width & Pan knobs ───────────────────────────────────
                constexpr int knobSize = 40;
                constexpr int labelH   = 14;
                const int knobY = typeY + typeH + secGap + 4;
                const int knobArea = selW / 2;
                const int knob1X = selX + knobArea / 2 - knobSize / 2;
                const int knob2X = selX + knobArea + knobArea / 2 - knobSize / 2;

                // Width knob
                g.setColour (juce::Colour (0xff2A3038));
                g.fillEllipse ((float) knob1X, (float) knobY,
                               (float) knobSize, (float) knobSize);
                g.setColour (juce::Colour (0xff363E4A));
                g.drawEllipse ((float) knob1X + 0.5f, (float) knobY + 0.5f,
                               (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                {
                    const float kcx = (float) knob1X + (float) knobSize * 0.5f;
                    g.drawLine (kcx, (float) knobY + (float) knobSize * 0.5f,
                                kcx, (float) knobY + 4.0f, 1.5f);
                }
                g.setColour (kTextMuted);
                g.setFont (lnf.interRegularFont (9.0f));
                g.drawText ("Width", knob1X - 5, knobY + knobSize + 2,
                            knobSize + 10, labelH, juce::Justification::centred, false);

                // Pan knob
                g.setColour (juce::Colour (0xff2A3038));
                g.fillEllipse ((float) knob2X, (float) knobY,
                               (float) knobSize, (float) knobSize);
                g.setColour (juce::Colour (0xff363E4A));
                g.drawEllipse ((float) knob2X + 0.5f, (float) knobY + 0.5f,
                               (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
                g.setColour (juce::Colours::white.withAlpha (0.6f));
                {
                    const float kcx = (float) knob2X + (float) knobSize * 0.5f;
                    g.drawLine (kcx, (float) knobY + (float) knobSize * 0.5f,
                                kcx, (float) knobY + 4.0f, 1.5f);
                }
                g.setColour (kTextMuted);
                g.drawText ("Pan", knob2X - 5, knobY + knobSize + 2,
                            knobSize + 10, labelH, juce::Justification::centred, false);

                // Filter section: handled by NoiseFilterVisualizer component
            }
            else
            {
                // ── SAMPLE panel ─────────────────────────────────────────
                noiseTypeBounds = {};

                constexpr int smpSelH   = 28;
                constexpr int knobSize  = 40;
                constexpr int labelH    = 14;

                const int smpY = selY + selH + secGap;

                // ── LOAD SAMPLE inline browser ───────────────────────────
                noiseSampleBtnBounds = { selX, smpY, selW, smpSelH };

                g.setColour (juce::Colour (0xff2A3038));
                {
                    const bool dropOpen = (openDropdown == OpenDropdown::NoiseSample);
                    juce::Path containerPath;
                    containerPath.addRoundedRectangle ((float) selX, (float) smpY,
                                                       (float) selW, (float) smpSelH,
                                                       8.0f, 8.0f,
                                                       true, true,
                                                       !dropOpen, !dropOpen);
                    g.fillPath (containerPath);
                }

                constexpr int arrowW = 22;
                const int prevX2 = selX;
                const int nextX2 = selX + selW - arrowW;
                const int midX2  = prevX2 + arrowW;
                const int midW2  = selW - arrowW * 2;

                noiseSampleDropBounds = { midX2, smpY, midW2, smpSelH };

                noiseSamplePrevBtn.setBounds (prevX2, smpY, arrowW, smpSelH);
                noiseSampleNextBtn.setBounds (nextX2, smpY, arrowW, smpSelH);

                // Divider lines
                g.setColour (juce::Colour (0xff363E4A));
                g.fillRect ((float) (prevX2 + arrowW), (float) smpY + 5.0f,
                            1.0f, (float) smpSelH - 10.0f);
                g.fillRect ((float) nextX2, (float) smpY + 5.0f,
                            1.0f, (float) smpSelH - 10.0f);

                // Centre label
                g.setColour (juce::Colours::white);
                g.setFont (lnf.interMediumFont (10.0f));
                {
                    const auto label = audioProcessor.noiseSamplePath.isEmpty()
                        ? juce::String ("LOAD SAMPLE")
                        : juce::File (audioProcessor.noiseSamplePath).getFileName();
                    g.drawText (label, midX2, smpY, midW2 - 14, smpSelH,
                                juce::Justification::centred, false);
                }

                // Dropdown triangle
                {
                    const float triX = (float) (midX2 + midW2) - 16.0f;
                    const float triY = (float) smpY + (float) smpSelH * 0.5f - 2.0f;
                    juce::Path tri;
                    tri.addTriangle (triX, triY, triX + 7.0f, triY, triX + 3.5f, triY + 5.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.7f));
                    g.fillPath (tri);
                }

                // ── 2×2 knob grid: START / LENGTH / PITCH / WIDTH ───────
                const int knobY2  = smpY + smpSelH + secGap + 4;
                const int knobArea = selW / 2;
                const int k1X = selX + knobArea / 2 - knobSize / 2;
                const int k2X = selX + knobArea + knobArea / 2 - knobSize / 2;
                const int row2Y = knobY2 + knobSize + labelH + secGap;

                auto drawKnob = [&] (int kx, int ky, const char* label)
                {
                    g.setColour (juce::Colour (0xff2A3038));
                    g.fillEllipse ((float) kx, (float) ky,
                                   (float) knobSize, (float) knobSize);
                    g.setColour (juce::Colour (0xff363E4A));
                    g.drawEllipse ((float) kx + 0.5f, (float) ky + 0.5f,
                                   (float) knobSize - 1.0f, (float) knobSize - 1.0f, 1.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.6f));
                    {
                        const float kcx = (float) kx + (float) knobSize * 0.5f;
                        g.drawLine (kcx, (float) ky + (float) knobSize * 0.5f,
                                    kcx, (float) ky + 4.0f, 1.5f);
                    }
                    g.setColour (kTextMuted);
                    g.setFont (lnf.interRegularFont (9.0f));
                    g.drawText (label, kx - 10, ky + knobSize + 2,
                                knobSize + 20, labelH, juce::Justification::centred, false);
                };

                drawKnob (k1X, knobY2, "Start");
                drawKnob (k2X, knobY2, "Length");
                drawKnob (k1X, row2Y,  "Pitch");
                drawKnob (k2X, row2Y,  "Width");

                // Filter section: handled by noiseSampleFilterVis component
            }
        }
    }

    // Transient tab: canvas (80% width, matching envelope tab layout)
    if (activeTab == Tab::Transient)
    {
        const auto cf = transientWaveBounds.toFloat();

        const float padX = 24.0f, padT = 20.0f, padB = 20.0f;
        const float plotL = cf.getX() + padX;
        const float plotR = cf.getRight() - padX;
        const float plotT = cf.getY() + padT;
        const float plotB = cf.getBottom() - padB;
        // Dark fill
        g.setColour (kBgPanel);
        g.fillRoundedRectangle (cf, 8.0f);

        // Grid
        g.setColour (juce::Colour (0xff222830));
        for (int i = 1; i < 4; ++i)
        {
            const float y = plotT + (float) i / 4.0f * (plotB - plotT);
            g.drawHorizontalLine ((int) y, plotL, plotR);
        }
        for (int i = 1; i < 5; ++i)
        {
            const float x = plotL + (float) i / 5.0f * (plotR - plotL);
            g.drawVerticalLine ((int) x, plotT, plotB);
        }

        // Centred placeholder label
        g.setColour (kTransientOrng.withAlpha (0.35f));
        g.setFont (lnf.interMediumFont (13.0f));
        g.drawText ("Drag & Drop Sample Here",
                    (int) plotL, (int) plotT, (int) (plotR - plotL), (int) (plotB - plotT),
                    juce::Justification::centred, false);
    }

    // Sauce: centred placeholder text
    if (activeTab == Tab::Sauce)
    {
        // "SECRET SAUCE" label centered below the knob
        constexpr int knobSize = 180;
        const int labelY = area.getCentreY() - knobSize / 2 - 16 + knobSize + 8;
        g.setColour (juce::Colours::white);
        g.setFont (lnf.interMediumFont (12.0f));
        g.drawText ("SECRET SAUCE",
                    area.getX(), labelY, area.getWidth(), 18,
                    juce::Justification::centred, false);
    }

    g.setColour (kTextDim);
    g.setFont (lnf.interRegularFont (9.0f));
    g.drawText ("SNARE MAKER",
                area.getX(), area.getBottom() - 18,
                area.getWidth(), 14, juce::Justification::centred, false);
}

// =============================================================================
// paintSnareDrum
// =============================================================================

void SnareMakerAudioProcessorEditor::paintSnareDrum (
    juce::Graphics& g, juce::Rectangle<int> area) const
{
    const float cx     = (float) area.getCentreX();
    const float cy     = (float) area.getY() + (float) area.getHeight() * 0.43f;
    const float headRx = std::min ((float) area.getWidth() * 0.40f, 145.0f);
    const float headRy = headRx * 0.28f;
    const float shellH = headRx * 0.42f;
    const float rimExt = 5.0f;

    // ── 1. Shell body ─────────────────────────────────────────────────────────
    {
        g.setColour (juce::Colour (0xff202038));
        g.fillRect (cx - headRx, cy, headRx * 2.0f, shellH);

        g.setColour (juce::Colour (0xff353552));
        g.fillRect (cx - headRx,        cy, 2.0f, shellH);
        g.fillRect (cx + headRx - 2.0f, cy, 2.0f, shellH);
    }

    // ── 2. Lug casings ────────────────────────────────────────────────────────
    {
        const float lugW = 7.0f;
        const float lugH = 14.0f;
        const float gap  = (shellH - lugH * 4.0f) / 5.0f;

        for (int i = 0; i < 4; ++i)
        {
            const float ly = cy + gap + i * (lugH + gap);

            g.setColour (juce::Colour (0xff303050));
            g.fillRoundedRectangle (cx - headRx - lugW + 1.5f, ly, lugW, lugH, 2.0f);
            g.fillRoundedRectangle (cx + headRx - 1.5f,        ly, lugW, lugH, 2.0f);

            g.setColour (juce::Colour (0xff5c5c78));
            g.fillEllipse (cx - headRx - lugW + 2.5f, ly + 1.5f, 3.0f, 3.0f);
            g.fillEllipse (cx + headRx + 0.5f,        ly + 1.5f, 3.0f, 3.0f);
        }
    }

    // ── 3. Snare-side (bottom) half-ellipse + wires ───────────────────────────
    {
        g.setColour (juce::Colour (0xff141428));
        g.fillEllipse (cx - headRx, cy + shellH - headRy,
                       headRx * 2.0f, headRy * 2.0f);

        g.setColour (juce::Colour (0x55aaaacc));
        const float span = headRx * 0.56f;
        const float wTop = cy + shellH - headRy * 0.5f;
        const float wBot = cy + shellH + headRy * 0.6f;
        for (int i = 0; i < 7; ++i)
        {
            const float wx = cx - span + (float) i * (span * 2.0f / 6.0f);
            g.drawLine (wx, wTop, wx, wBot, 0.9f);
        }
    }

    // ── 4. Bottom rim ─────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff787890));
    g.drawEllipse (cx - headRx - rimExt,
                   cy + shellH - headRy - rimExt * 0.35f,
                   (headRx + rimExt) * 2.0f,
                   (headRy + rimExt * 0.35f) * 2.0f,
                   2.5f);

    // ── 5. Top drum head ──────────────────────────────────────────────────────
    {
        g.setColour (juce::Colour (0xffb8b8c8));
        g.fillEllipse (cx - headRx, cy - headRy, headRx * 2.0f, headRy * 2.0f);

        g.setColour (juce::Colour (0x20000000));
        g.drawEllipse (cx - headRx * 0.32f, cy - headRy * 0.32f,
                       headRx * 0.64f, headRy * 0.64f, 0.8f);
    }

    // ── 6. Top rim ────────────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff8888a0));
    g.drawEllipse (cx - headRx - rimExt,
                   cy - headRy - rimExt * 0.35f,
                   (headRx + rimExt) * 2.0f,
                   (headRy + rimExt * 0.35f) * 2.0f,
                   3.0f);

    g.setColour (juce::Colour (0xffb8b8cc));
    g.drawEllipse (cx - headRx - rimExt,
                   cy - headRy - rimExt * 0.35f,
                   (headRx + rimExt) * 2.0f,
                   (headRy + rimExt * 0.35f) * 2.0f,
                   1.0f);

    // ── 7. Snare strainer lever ───────────────────────────────────────────────
    {
        const float sx = cx + headRx + rimExt + 5.0f;
        const float sy = cy + shellH * 0.36f;

        g.setColour (juce::Colour (0xff3a3a54));
        g.fillRoundedRectangle (sx, sy, 18.0f, 9.0f, 3.0f);

        g.setColour (juce::Colour (0xff606078));
        g.fillRoundedRectangle (sx + 13.0f, sy - 5.0f, 6.0f, 19.0f, 2.0f);

        g.setColour (juce::Colour (0xff5a5a70));
        g.fillEllipse (sx + 2.5f, sy + 2.0f, 5.0f, 5.0f);
    }
}
