#include "PluginEditor.h"

// =============================================================================
// File-local colour palette and layout constants
// =============================================================================

namespace
{
    // ── Colours ───────────────────────────────────────────────────────────────
    const juce::Colour kBgWindow   { 0xff0d0d1a };
    const juce::Colour kBgDrum     { 0xff0e0e1e };
    const juce::Colour kBgZone     { 0xff111122 };
    const juce::Colour kBgZoneHov  { 0xff181832 };
    const juce::Colour kBgZoneAct  { 0xff1c1c38 };
    const juce::Colour kBgTrack    { 0xff252538 };
    const juce::Colour kTextBright { 0xffffffff };
    const juce::Colour kTextMuted  { 0xff8888aa };
    const juce::Colour kTextDim    { 0xff444460 };
    const juce::Colour kDivider    { 0xff252538 };

    const juce::Colour kPitchBlue  { 0xff4a9eff };
    const juce::Colour kNoiseRed   { 0xffe94560 };
    const juce::Colour kOutTeal    { 0xff00d4aa };
    const juce::Colour kRoomPurple { 0xffaa55ff };

    // ── Window / section widths ────────────────────────────────────────────────
    //
    //   kBodyW + kDrumW + kNoiseW + kOutputW = kWinW
    //   195    + 470    + 200     + 95       = 960
    //
    constexpr int kWinW    = 960;
    constexpr int kWinH    = 520;
    constexpr int kHeaderH = 50;
    constexpr int kRoomH   = 72;
    constexpr int kBodyW   = 195;
    constexpr int kNoiseW  = 200;
    constexpr int kOutputW = 95;
    constexpr int kDrumW   = kWinW - kBodyW - kNoiseW - kOutputW;  // 470
    constexpr int kMainH   = kWinH - kHeaderH - kRoomH;            // 398

    // ── Tab geometry (Phase 6-1) ────────────────────────────────────────────────
    constexpr int kTabW     = 70;
    constexpr int kTabH     = 18;
    constexpr int kTabGap   = 6;   // gap between the two tabs
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
    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff181828));
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

    // ── Envelope mode buttons ─────────────────────────────────────────────────
    bodyPitchBtn.onClick = [this] { setEnvMode (EnvMode::Pitch); };
    bodyAmpBtn  .onClick = [this] { setEnvMode (EnvMode::BodyAmp); };
    noiseAmpBtn .onClick = [this] { setEnvMode (EnvMode::NoiseAmp); };
    addAndMakeVisible (bodyPitchBtn);
    addAndMakeVisible (bodyAmpBtn);
    addAndMakeVisible (noiseAmpBtn);

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

    const bool showBody  = (tab == Tab::Body);
    const bool showNoise = (tab == Tab::Noise);

    // Envelope mode buttons
    bodyPitchBtn.setVisible (showBody);
    bodyAmpBtn  .setVisible (showBody);
    noiseAmpBtn .setVisible (showNoise);

    // Default envelope for each tab
    setEnvMode (showBody ? EnvMode::Pitch : EnvMode::NoiseAmp);

    repaint();
}

// =============================================================================
// setEnvMode
// =============================================================================

void SnareMakerAudioProcessorEditor::setEnvMode (EnvMode mode)
{
    envMode = mode;

    auto styleBtn = [&] (juce::TextButton& btn, bool active, juce::Colour accent)
    {
        btn.setColour (juce::TextButton::buttonColourId,
                       active ? accent : juce::Colour (0xff181828));
        btn.setColour (juce::TextButton::textColourOffId,
                       active ? juce::Colours::white : juce::Colour (0xff8888aa));
    };

    styleBtn (bodyPitchBtn, mode == EnvMode::Pitch,    kPitchBlue);
    styleBtn (bodyAmpBtn,   mode == EnvMode::BodyAmp,  kPitchBlue);
    styleBtn (noiseAmpBtn,  mode == EnvMode::NoiseAmp, kNoiseRed);

    bodyPitchBtn.repaint();
    bodyAmpBtn  .repaint();
    noiseAmpBtn .repaint();

    switch (mode)
    {
        case EnvMode::Pitch:    envelopeEditor.setEnvelope (audioProcessor.pitchEnvelope);    break;
        case EnvMode::BodyAmp:  envelopeEditor.setEnvelope (audioProcessor.bodyAmpEnvelope);  break;
        case EnvMode::NoiseAmp: envelopeEditor.setEnvelope (audioProcessor.noiseAmpEnvelope); break;
    }
}

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    const int mainTop = kHeaderH;
    const int roomTop = kWinH - kRoomH;

    // ── Tab hit-test rectangles (centred in header) ─────────────────────────
    {
        const int totalTabW = kTabW * 2 + kTabGap;
        const int tabX = (kWinW - totalTabW) / 2;
        const int tabY = kHeaderH - kTabH - 4;   // 4px above header bottom edge
        bodyTabBounds  = { tabX,                    tabY, kTabW, kTabH };
        noiseTabBounds = { tabX + kTabW + kTabGap,  tabY, kTabW, kTabH };
    }

    bodyZoneBounds   = { 0,                          mainTop, kBodyW,   kMainH };
    drumAreaBounds   = { kBodyW,                     mainTop, kDrumW,   kMainH };
    noiseZoneBounds  = { kBodyW + kDrumW,            mainTop, kNoiseW,  kMainH };
    outputZoneBounds = { kBodyW + kDrumW + kNoiseW,  mainTop, kOutputW, kMainH };
    roomZoneBounds   = { 0,                          roomTop, kWinW,    kRoomH };

    // ── Envelope editor (inset inside drum centre area) ────────────────────────
    envelopeEditor.setBounds (drumAreaBounds.reduced (20, 40));

    // ── Envelope mode buttons (centred below envelope editor) ────────────────
    {
        constexpr int btnW = 70, btnH = 22, btnGap = 8;
        const auto envB = envelopeEditor.getBounds();
        const int  btnY = envB.getBottom() + 6;
        const int  cx   = drumAreaBounds.getCentreX();

        // BODY tab: two buttons side by side
        bodyPitchBtn.setBounds (cx - btnW - btnGap / 2, btnY, btnW, btnH);
        bodyAmpBtn  .setBounds (cx + btnGap / 2,        btnY, btnW, btnH);

        // NOISE tab: single centred button
        noiseAmpBtn .setBounds (cx - btnW / 2,          btnY, btnW, btnH);
    }

}

// =============================================================================
// Zone hit-test
// =============================================================================

SnareMakerAudioProcessorEditor::Zone
SnareMakerAudioProcessorEditor::zoneAt (juce::Point<int> pos) const noexcept
{
    if (bodyZoneBounds.contains   (pos)) return Zone::Body;
    if (noiseZoneBounds.contains  (pos)) return Zone::Noise;
    if (outputZoneBounds.contains (pos)) return Zone::Output;
    if (roomZoneBounds.contains   (pos)) return Zone::Room;
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
    // ── Tab click (Phase 6-1) ────────────────────────────────────────────────
    if (bodyTabBounds.contains (e.getPosition()) && activeTab != Tab::Body)
    {
        setActiveTab (Tab::Body);
        return;
    }
    if (noiseTabBounds.contains (e.getPosition()) && activeTab != Tab::Noise)
    {
        setActiveTab (Tab::Noise);
        return;
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

    paintZone (g, bodyZoneBounds,   Zone::Body,   "BODY",   kPitchBlue,  {}, true);
    paintZone (g, noiseZoneBounds,  Zone::Noise,  "NOISE",  kNoiseRed,   {}, true);
    paintZone (g, outputZoneBounds, Zone::Output, "OUTPUT", kOutTeal,    {}, true);
    paintZone (g, roomZoneBounds,   Zone::Room,   "ROOM",   kRoomPurple,
               { "Reverb  ·  Pre-Delay  ·  Size  ·  Damping" });

    paintDrumArea (g, drumAreaBounds);
}

// =============================================================================
// paintHeader
// =============================================================================

void SnareMakerAudioProcessorEditor::paintHeader (juce::Graphics& g) const
{
    const int w = getWidth();

    juce::ColourGradient hGrad (
        juce::Colour (0xff0a0a1e), 0.0f, 0.0f,
        juce::Colour (0xff14142c), 0.0f, (float) kHeaderH,
        false);
    g.setGradientFill (hGrad);
    g.fillRect (0, 0, w, kHeaderH);

    g.setColour (kDivider);
    g.fillRect (0, kHeaderH - 1, w, 1);

    g.setColour (kTextBright);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold")));
    g.drawText ("SNARE MAKER", 0, 2, w, 30, juce::Justification::centred, false);

    // Tabs replace the old tagline
    paintTabs (g);
}

// =============================================================================
// paintTabs  (Phase 6-1)
// =============================================================================

void SnareMakerAudioProcessorEditor::paintTabs (juce::Graphics& g) const
{
    auto drawTab = [&] (juce::Rectangle<int> bounds, const juce::String& text,
                        juce::Colour accent, bool isActive)
    {
        const float alpha = isActive ? 1.0f : 0.35f;
        g.setColour (accent.withAlpha (alpha));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.0f).withStyle ("Bold")));
        g.drawText (text, bounds, juce::Justification::centred, false);

        // Active underline
        if (isActive)
        {
            g.setColour (accent);
            g.fillRoundedRectangle ((float) bounds.getX() + 8.0f,
                                    (float) bounds.getBottom() - 2.0f,
                                    (float) bounds.getWidth() - 16.0f,
                                    2.0f, 1.0f);
        }
    };

    drawTab (bodyTabBounds,  "BODY",  kPitchBlue, activeTab == Tab::Body);
    drawTab (noiseTabBounds, "NOISE", kNoiseRed,  activeTab == Tab::Noise);
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
    const bool isRoom    = (zone == Zone::Room);

    // Background
    g.setColour (isActive ? kBgZoneAct : isHovered ? kBgZoneHov : kBgZone);
    g.fillRoundedRectangle (bounds.toFloat().reduced (2.0f), 6.0f);

    // Accent border
    const float borderAlpha = isActive ? 0.90f : isHovered ? 0.50f : 0.18f;
    g.setColour (accent.withAlpha (borderAlpha));

    if (isRoom)
        g.fillRect ((float) bounds.getX() + 2.0f,
                    (float) bounds.getY() + 2.0f,
                    (float) bounds.getWidth() - 4.0f, 2.0f);
    else
        g.fillRoundedRectangle ((float) bounds.getX() + 2.0f,
                                (float) bounds.getY() + 2.0f,
                                3.0f,
                                (float) bounds.getHeight() - 4.0f,
                                1.5f);

    // Title
    const float titleAlpha = isActive ? 1.0f : isHovered ? 0.80f : 0.50f;
    g.setColour (accent.withAlpha (titleAlpha));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f).withStyle ("Bold")));

    if (isRoom)
        g.drawText (title, bounds.getX() + 14, bounds.getY() + 10,
                    70, 14, juce::Justification::centredLeft, false);
    else
        g.drawText (title, bounds.getX() + 10, bounds.getY() + 14,
                    bounds.getWidth() - 14, 14, juce::Justification::centred, false);

    // Placeholder hints (only when no real controls yet)
    if (!hasControls)
    {
        g.setColour (isActive ? kTextMuted : kTextDim);
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f)));

        if (isRoom)
        {
            g.drawText (hints[0], bounds.getX() + 96, bounds.getY() + 10,
                        bounds.getWidth() - 110, 14,
                        juce::Justification::centredLeft, false);
        }
        else
        {
            const int hintStartY = bounds.getY() + 40;
            for (int i = 0; i < hints.size(); ++i)
                g.drawText (hints[i],
                            bounds.getX() + 8, hintStartY + i * 22,
                            bounds.getWidth() - 16, 16,
                            juce::Justification::centred, false);
        }

        if (!isActive)
        {
            const float cAlpha = isHovered ? 0.45f : 0.18f;
            g.setColour (accent.withAlpha (cAlpha));
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (8.5f)));

            if (isRoom)
                g.drawText ("CLICK TO EXPAND",
                            bounds.getRight() - 120, bounds.getY() + 10,
                            110, 14, juce::Justification::centredRight, false);
            else
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

    const float cx = (float) area.getCentreX();
    const float cy = (float) area.getCentreY();
    juce::ColourGradient glow (
        juce::Colour (0xff1a1a32), cx, cy,
        kBgDrum,                   cx + 200.0f, cy,
        true);
    g.setGradientFill (glow);
    g.fillRect (area);

    g.setColour (kDivider);
    g.fillRect (area.getX(),         area.getY(), 1, area.getHeight());
    g.fillRect (area.getRight() - 1, area.getY(), 1, area.getHeight());

    paintSnareDrum (g, area);

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
        juce::ColourGradient sg (
            juce::Colour (0xff272742), cx - headRx, cy,
            juce::Colour (0xff181830), cx + headRx, cy,
            false);
        g.setGradientFill (sg);
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
        juce::ColourGradient headGrad (
            juce::Colour (0xffe2e2ec), cx, cy,
            juce::Colour (0xff888898), cx + headRx * 0.88f, cy + headRy * 0.78f,
            true);
        g.setGradientFill (headGrad);
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
