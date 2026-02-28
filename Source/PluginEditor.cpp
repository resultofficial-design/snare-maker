#include "PluginEditor.h"

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

    // ── Side panel gap (Transient / Resonant) ────────────────────────────────
    constexpr int kSideGap = 6;
}

// =============================================================================
// SnareLookAndFeel
// =============================================================================

SnareMakerAudioProcessorEditor::SnareLookAndFeel::SnareLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,              kTextMuted);
    setColour (juce::Slider::textBoxBackgroundColourId,        kBgWindow);
    setColour (juce::Slider::textBoxOutlineColourId,           juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,         kPitchBlue);
    setColour (juce::Label::textColourId,                      kTextBright);
    setColour (juce::PopupMenu::backgroundColourId,            kBgPanel);
    setColour (juce::PopupMenu::textColourId,                  kTextBright);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kPitchBlue);
    setColour (juce::PopupMenu::highlightedTextColourId,       kTextBright);
}

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
// Constructor / Destructor
// =============================================================================

SnareMakerAudioProcessorEditor::SnareMakerAudioProcessorEditor (
        SnareMakerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lnf);

    // ── Envelope editor (overlays drum centre area) ─────────────────────────
    envelopeEditor.connectToParameters (audioProcessor.apvts, audioProcessor.pitchEnvelope,
                                        audioProcessor.envelopeLock);
    addAndMakeVisible (envelopeEditor);

    // ── Preset combo (visual only) ────────────────────────────────────────────
    presetCombo.addItem ("Init Snare",       1);
    presetCombo.addItem ("Trap Snare",       2);
    presetCombo.addItem ("Techno Snare",     3);
    presetCombo.addItem ("Acoustic Hybrid",  4);
    presetCombo.addItem ("Metallic Snap",    5);
    presetCombo.setSelectedId (1, juce::dontSendNotification);
    presetCombo.setColour (juce::ComboBox::backgroundColourId,  juce::Colour (0xff1E2229));
    presetCombo.setColour (juce::ComboBox::outlineColourId,     juce::Colour (0xff2A3038));
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
    bodyPitchBtn.onClick = [this] { setEnvMode (EnvMode::Pitch); };
    bodyAmpBtn  .onClick = [this] { setEnvMode (EnvMode::BodyAmp); };
    noiseAmpBtn .onClick = [this] { setEnvMode (EnvMode::NoiseAmp); };
    initModeBtn (bodyPitchBtn);
    initModeBtn (bodyAmpBtn);
    initModeBtn (noiseAmpBtn);

    // Size first so resized() computes all bounds before visibility/styling
    setSize (kWinW, kWinH);

    // Now that bounds exist, set tab + envelope mode
    setActiveTab (Tab::Body);
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

    const bool showBody     = (tab == Tab::Body);
    const bool showNoise    = (tab == Tab::Noise);
    const bool showResonant = (tab == Tab::Resonant);

    // Envelope mode buttons
    bodyPitchBtn.setVisible (showBody);
    bodyAmpBtn  .setVisible (showBody);
    noiseAmpBtn .setVisible (false);

    // Envelope editor visible for Body, Noise, and Resonant
    envelopeEditor.setVisible (showBody || showNoise || showResonant);

    // Resonant / Noise tab: envelope at 80% of full width (side panel fills the rest)
    if (showResonant || showNoise)
    {
        auto b = envEditorFullBounds;
        b.setWidth (b.getWidth() * 4 / 5 - 30);
        envelopeEditor.setBounds (b);
    }
    else
    {
        envelopeEditor.setBounds (envEditorFullBounds);
    }

    // Default envelope for each tab
    if (showBody)          setEnvMode (EnvMode::Pitch);
    else if (showNoise)    setEnvMode (EnvMode::NoiseAmp);
    else if (showResonant) setEnvMode (EnvMode::ResonantAmp);
    // Room and Sauce: no envelope / no mode buttons (placeholder only)

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

    styleBtn (bodyPitchBtn, mode == EnvMode::Pitch,    kPitchBlue);
    styleBtn (bodyAmpBtn,   mode == EnvMode::BodyAmp,  kPitchBlue);
    styleBtn (noiseAmpBtn,  mode == EnvMode::NoiseAmp, kNoiseRed);

    bodyPitchBtn.repaint();
    bodyAmpBtn  .repaint();
    noiseAmpBtn .repaint();

    switch (mode)
    {
        case EnvMode::Pitch:       envelopeEditor.setEnvelope (audioProcessor.pitchEnvelope);       break;
        case EnvMode::BodyAmp:     envelopeEditor.setEnvelope (audioProcessor.bodyAmpEnvelope);     break;
        case EnvMode::NoiseAmp:    envelopeEditor.setEnvelope (audioProcessor.noiseAmpEnvelope);    break;
        case EnvMode::ResonantAmp: envelopeEditor.setEnvelope (audioProcessor.resonantAmpEnvelope); break;
    }
}

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    const int mainTop = kHeaderH;

    drumAreaBounds   = { 0,                mainTop, kWinW - kOutputW, kMainH };
    outputZoneBounds = { kWinW - kOutputW, mainTop, kOutputW,         kMainH };

    // ── Preset combo in header (right-aligned) ─────────────────────────────
    {
        constexpr int comboW = 150, comboH = 24, padR = 12;
        presetCombo.setBounds (kWinW - padR - comboW,
                               (kHeaderH - comboH) / 2,
                               comboW, comboH);
    }

    // ── Layout: [BODY][NOISE] tabs → EnvelopeEditor → [PITCH][AMP] buttons ──
    {
        constexpr int btnH = 32, btnGap = 4;
        constexpr int envPadX = 20, envPadTop = 6, envPadBot = 6;

        // Tabs directly above envelope editor – two groups
        const int tabY = mainTop + envPadTop;
        const int leftX = drumAreaBounds.getX() + envPadX;          // left-aligned
        transientTabBounds = { leftX,                                     tabY, kTabW, kTabH };
        bodyTabBounds      = { leftX + (kTabW + kTabGap),                tabY, kTabW, kTabH };
        resonantTabBounds  = { leftX + (kTabW + kTabGap) * 2,            tabY, kTabW, kTabH };
        noiseTabBounds     = { leftX + (kTabW + kTabGap) * 3,            tabY, kTabW, kTabH };

        const int rightEnd = drumAreaBounds.getRight() - envPadX;   // right-aligned
        sauceTabBounds     = { rightEnd - kTabW,                          tabY, kTabW, kTabH };
        roomTabBounds      = { rightEnd - kTabW * 2 - kTabGap,           tabY, kTabW, kTabH };

        // Envelope editor below tabs
        const int envTop = tabY + kTabH + envPadTop;
        // Mode buttons below envelope editor
        const int modeBtnY = drumAreaBounds.getBottom() - envPadBot - btnH;
        // Envelope fills the space between tabs and mode buttons
        const int envH = modeBtnY - envPadBot - envTop;
        envEditorFullBounds = { drumAreaBounds.getX() + envPadX, envTop,
                                drumAreaBounds.getWidth() - envPadX * 2, envH };
        envelopeEditor.setBounds (envEditorFullBounds);

        // BODY tab: two buttons spanning full envelope width
        const int envL = envEditorFullBounds.getX();
        const int envW = envEditorFullBounds.getWidth();
        const int halfW = (envW - btnGap) / 2;
        bodyPitchBtn.setBounds (envL,                    modeBtnY, halfW, btnH);
        bodyAmpBtn  .setBounds (envL + halfW + btnGap,   modeBtnY, envW - halfW - btnGap, btnH);

        // NOISE tab: single button spanning full envelope width
        noiseAmpBtn .setBounds (envL,                    modeBtnY, envW, btnH);
    }

}

// =============================================================================
// Zone hit-test
// =============================================================================

SnareMakerAudioProcessorEditor::Zone
SnareMakerAudioProcessorEditor::zoneAt (juce::Point<int> pos) const noexcept
{
    if (outputZoneBounds.contains (pos)) return Zone::Output;
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

                // If we just disabled the active tab, jump to the next enabled one
                if (!tabEnabledFor (tab) && activeTab == tab)
                {
                    constexpr Tab order[] = { Tab::Transient, Tab::Body, Tab::Resonant, Tab::Noise, Tab::Room, Tab::Sauce };
                    for (auto t : order)
                        if (tabEnabledFor (t)) { setActiveTab (t); break; }
                }

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

    paintZone (g, outputZoneBounds, Zone::Output, "OUTPUT", kOutTeal,    {}, true);

    paintDrumArea (g, drumAreaBounds);
    paintTabs (g);

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

    g.setColour (kDivider);
    g.fillRect (0, kHeaderH - 1, w, 1);

    // Small logo text, right-aligned before the preset combo
    constexpr int logoTextW = 95;
    constexpr int padR = 12;
    constexpr int comboW = 150;
    const int logoX = w - padR - comboW - 8 - logoTextW;

    g.setColour (kTextMuted);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f).withStyle ("Bold")));
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
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
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
    g.fillRoundedRectangle (bounds.toFloat().reduced (2.0f), 6.0f);

    // Accent border (left bar)
    const float borderAlpha = isActive ? 0.90f : isHovered ? 0.50f : 0.18f;
    g.setColour (accent.withAlpha (borderAlpha));
    g.fillRoundedRectangle ((float) bounds.getX() + 2.0f,
                            (float) bounds.getY() + 2.0f,
                            3.0f,
                            (float) bounds.getHeight() - 4.0f,
                            1.5f);

    // Title
    const float titleAlpha = isActive ? 1.0f : isHovered ? 0.80f : 0.50f;
    g.setColour (accent.withAlpha (titleAlpha));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f).withStyle ("Bold")));
    g.drawText (title, bounds.getX() + 10, bounds.getY() + 14,
                bounds.getWidth() - 14, 14, juce::Justification::centred, false);

    // Placeholder hints (only when no real controls yet)
    if (!hasControls)
    {
        g.setColour (isActive ? kTextMuted : kTextDim);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f)));

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
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (8.5f)));
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
    juce::Graphics& g, juce::Rectangle<int> area) const
{
    g.setColour (kBgDrum);
    g.fillRect (area);

    g.setColour (kDivider);
    g.fillRect (area.getX(),         area.getY(), 1, area.getHeight());
    g.fillRect (area.getRight() - 1, area.getY(), 1, area.getHeight());

    paintSnareDrum (g, area);

    // Transient / Resonant / Noise: side panel fills from waveform right edge to Output
    if (activeTab == Tab::Transient || activeTab == Tab::Resonant || activeTab == Tab::Noise)
    {
        const int waveW = envEditorFullBounds.getWidth() * 4 / 5 - 30;
        const int sideX = envEditorFullBounds.getX() + waveW + kSideGap;
        const int sideW = outputZoneBounds.getX() - sideX - kSideGap;

        const auto sideBox = juce::Rectangle<int> (sideX, envEditorFullBounds.getY(),
                                                   sideW, envEditorFullBounds.getHeight()).toFloat();
        g.setColour (kBgPanel);
        g.fillRoundedRectangle (sideBox, 8.0f);

        g.setColour (juce::Colour (0xff2A3038));
        g.drawRoundedRectangle (sideBox.reduced (0.5f), 8.0f, 1.0f);

        // Noise tab: split top container inside the side panel
        if (activeTab == Tab::Noise)
        {
            constexpr int innerPad = 6;
            const int innerX = sideX + innerPad;
            const int innerY = envEditorFullBounds.getY() + innerPad;
            const int innerW = sideW - innerPad * 2;
            const int innerH = (int) (envEditorFullBounds.getHeight() * 0.30f);

            const auto innerBox = juce::Rectangle<float> ((float) innerX, (float) innerY,
                                                           (float) innerW, (float) innerH);
            g.setColour (kBgPanel);
            g.fillRoundedRectangle (innerBox, 8.0f);

            g.setColour (juce::Colour (0xff2A3038));
            g.drawRoundedRectangle (innerBox.reduced (0.5f), 8.0f, 1.0f);

            // Vertical divider at centre
            const float divX = innerBox.getCentreX();
            g.setColour (juce::Colour (0xff363E4A));
            g.fillRect (divX, innerBox.getY() + 6.0f, 1.0f, innerBox.getHeight() - 12.0f);

            // Labels
            g.setColour (kNoiseRed.withAlpha (0.50f));
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
            const int halfW = innerW / 2;
            g.drawText ("Noise Generator", innerX, innerY, halfW, innerH,
                        juce::Justification::centred, false);
            g.drawText ("Noise Sampler", innerX + halfW, innerY, halfW, innerH,
                        juce::Justification::centred, false);
        }
    }

    // Transient tab: canvas (no envelope overlay)
    if (activeTab == Tab::Transient)
    {
        auto c = envEditorFullBounds;
        c.setWidth (c.getWidth() * 4 / 5 - 30);
        const auto cf = c.toFloat();

        const float padX = 24.0f, padT = 20.0f, padB = 20.0f;
        const float plotL = cf.getX() + padX;
        const float plotR = cf.getRight() - padX;
        const float plotT = cf.getY() + padT;
        const float plotB = cf.getBottom() - padB;
        // Dark fill
        g.setColour (kBgPanel);
        g.fillRoundedRectangle (cf, 8.0f);

        // Frame
        g.setColour (juce::Colour (0xff2A3038));
        g.drawRoundedRectangle (plotL - 1.0f, plotT - 1.0f,
                                plotR - plotL + 2.0f, plotB - plotT + 2.0f,
                                8.0f, 1.0f);

        // Grid
        g.setColour (juce::Colour (0xff2A3038));
        g.drawHorizontalLine ((int) plotT, plotL, plotR);
        g.drawHorizontalLine ((int) plotB, plotL, plotR);
        g.drawVerticalLine   ((int) plotL, plotT, plotB);
        g.drawVerticalLine   ((int) plotR, plotT, plotB);

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
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f).withStyle ("Bold")));
        g.drawText ("Drag & Drop Sample Here",
                    (int) plotL, (int) plotT, (int) (plotR - plotL), (int) (plotB - plotT),
                    juce::Justification::centred, false);
    }

    // Room / Sauce: centred placeholder text
    if (activeTab == Tab::Room)
    {
        g.setColour (kRoomPurple.withAlpha (0.40f));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (15.0f).withStyle ("Bold")));
        g.drawText ("Room Module (Coming Soon)",
                    area, juce::Justification::centred, false);
    }
    else if (activeTab == Tab::Sauce)
    {
        g.setColour (kSaucePink.withAlpha (0.40f));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (15.0f).withStyle ("Bold")));
        g.drawText ("Sauce Module (Coming Soon)",
                    area, juce::Justification::centred, false);
    }

    g.setColour (kTextDim);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)));
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
