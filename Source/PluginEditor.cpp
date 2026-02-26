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
    const juce::Colour kTextBright { 0xffffffff };
    const juce::Colour kTextMuted  { 0xff8888aa };
    const juce::Colour kTextDim    { 0xff444460 };
    const juce::Colour kDivider    { 0xff252538 };

    const juce::Colour kPitchBlue  { 0xff4a9eff };
    const juce::Colour kNoiseRed   { 0xffe94560 };
    const juce::Colour kRoomPurple { 0xffaa55ff };

    // ── Window / layout constants ─────────────────────────────────────────────
    constexpr int kWinW    = 860;
    constexpr int kWinH    = 520;
    constexpr int kHeaderH = 50;
    constexpr int kRoomH   = 72;
    constexpr int kSideW   = 195;
    constexpr int kMainH   = kWinH - kHeaderH - kRoomH;  // 398
    constexpr int kDrumW   = kWinW - kSideW * 2;         // 470
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

SnareMakerAudioProcessorEditor::SnareMakerAudioProcessorEditor (
        SnareMakerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (kWinW, kWinH);
}

SnareMakerAudioProcessorEditor::~SnareMakerAudioProcessorEditor() = default;

// =============================================================================
// resized
// =============================================================================

void SnareMakerAudioProcessorEditor::resized()
{
    const int mainTop = kHeaderH;
    const int roomTop = kWinH - kRoomH;

    bodyZoneBounds  = { 0,              mainTop, kSideW,  kMainH };
    drumAreaBounds  = { kSideW,         mainTop, kDrumW,  kMainH };
    noiseZoneBounds = { kSideW + kDrumW, mainTop, kSideW, kMainH };
    roomZoneBounds  = { 0,              roomTop, kWinW,   kRoomH };
}

// =============================================================================
// Zone hit-test
// =============================================================================

SnareMakerAudioProcessorEditor::Zone
SnareMakerAudioProcessorEditor::zoneAt (juce::Point<int> pos) const noexcept
{
    if (bodyZoneBounds.contains  (pos)) return Zone::Body;
    if (noiseZoneBounds.contains (pos)) return Zone::Noise;
    if (roomZoneBounds.contains  (pos)) return Zone::Room;
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

    paintZone (g, bodyZoneBounds,  Zone::Body,  "BODY",
               kPitchBlue,
               { "Body Freq", "Pitch Amount", "Pitch Decay",
                 "Phase Offset", "Pitch Curve" });

    paintZone (g, noiseZoneBounds, Zone::Noise, "NOISE",
               kNoiseRed,
               { "Level", "Attack  ·  Decay",
                 "Sustain  ·  Release",
                 "Filter Type  ·  Freq  ·  Q",
                 "Brightness" });

    paintZone (g, roomZoneBounds,  Zone::Room,  "ROOM",
               kRoomPurple,
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

    // Bottom separator
    g.setColour (kDivider);
    g.fillRect (0, kHeaderH - 1, w, 1);

    // Plugin name
    g.setColour (kTextBright);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (22.0f).withStyle ("Bold")));
    g.drawText ("SNARE MAKER", 0, 2, w, 30, juce::Justification::centred, false);

    // Tagline
    g.setColour (kTextDim);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)));
    g.drawText ("PERCUSSIVE SYNTHESIZER", 0, 31, w, 13,
                juce::Justification::centred, false);
}

// =============================================================================
// paintZone  –  reusable for Body, Noise, Room
// =============================================================================

void SnareMakerAudioProcessorEditor::paintZone (
    juce::Graphics&          g,
    juce::Rectangle<int>     bounds,
    Zone                     zone,
    const juce::String&      title,
    juce::Colour             accent,
    const juce::StringArray& hints) const
{
    const bool isHovered = (hoveredZone == zone);
    const bool isActive  = (activeZone  == zone);
    const bool isRoom    = (zone == Zone::Room);

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour (isActive ? kBgZoneAct : isHovered ? kBgZoneHov : kBgZone);
    g.fillRoundedRectangle (bounds.toFloat().reduced (2.0f), 6.0f);

    // ── Accent border line ────────────────────────────────────────────────────
    const float borderAlpha = isActive ? 0.90f : isHovered ? 0.50f : 0.18f;
    g.setColour (accent.withAlpha (borderAlpha));

    if (isRoom)
    {
        // Top line for the room zone
        g.fillRect ((float) bounds.getX() + 2.0f,
                    (float) bounds.getY() + 2.0f,
                    (float) bounds.getWidth() - 4.0f, 2.0f);
    }
    else
    {
        // Left accent bar for the side zones
        g.fillRoundedRectangle ((float) bounds.getX() + 2.0f,
                                (float) bounds.getY() + 2.0f,
                                3.0f,
                                (float) bounds.getHeight() - 4.0f,
                                1.5f);
    }

    // ── Title ─────────────────────────────────────────────────────────────────
    const float titleAlpha = isActive ? 1.0f : isHovered ? 0.80f : 0.50f;
    g.setColour (accent.withAlpha (titleAlpha));
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f).withStyle ("Bold")));

    if (isRoom)
        g.drawText (title, bounds.getX() + 14, bounds.getY() + 10,
                    70, 14, juce::Justification::centredLeft, false);
    else
        g.drawText (title, bounds.getX() + 10, bounds.getY() + 14,
                    bounds.getWidth() - 14, 14, juce::Justification::centred, false);

    // ── Hint labels ───────────────────────────────────────────────────────────
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

    // ── "Click to expand" footer hint ─────────────────────────────────────────
    if (!isActive)
    {
        const float hintAlpha = isHovered ? 0.45f : 0.18f;
        g.setColour (accent.withAlpha (hintAlpha));
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

// =============================================================================
// paintDrumArea  –  background, glow, dividers, drum, footer
// =============================================================================

void SnareMakerAudioProcessorEditor::paintDrumArea (
    juce::Graphics& g, juce::Rectangle<int> area) const
{
    // Background
    g.setColour (kBgDrum);
    g.fillRect (area);

    // Radial glow behind drum
    const float cx = (float) area.getCentreX();
    const float cy = (float) area.getCentreY();
    juce::ColourGradient glow (
        juce::Colour (0xff1a1a32), cx, cy,
        kBgDrum,                   cx + 200.0f, cy,
        true);
    g.setGradientFill (glow);
    g.fillRect (area);

    // Vertical edge dividers
    g.setColour (kDivider);
    g.fillRect (area.getX(),         area.getY(), 1, area.getHeight());
    g.fillRect (area.getRight() - 1, area.getY(), 1, area.getHeight());

    paintSnareDrum (g, area);

    // Footer instruction
    g.setColour (kTextDim);
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)));
    g.drawText ("CLICK A ZONE TO EDIT PARAMETERS",
                area.getX(), area.getBottom() - 18,
                area.getWidth(), 14, juce::Justification::centred, false);
}

// =============================================================================
// paintSnareDrum  –  stylised top-view snare illustration
// =============================================================================

void SnareMakerAudioProcessorEditor::paintSnareDrum (
    juce::Graphics& g, juce::Rectangle<int> area) const
{
    // ── Geometry (scales with area width, capped for consistency) ─────────────
    const float cx     = (float) area.getCentreX();
    const float cy     = (float) area.getY() + (float) area.getHeight() * 0.43f;
    const float headRx = std::min ((float) area.getWidth() * 0.40f, 145.0f);
    const float headRy = headRx * 0.28f;   // perspective flattening
    const float shellH = headRx * 0.42f;   // snare drum is shallower than a tom
    const float rimExt = 5.0f;             // rim extends beyond head

    // ── 1. Shell body ─────────────────────────────────────────────────────────
    {
        juce::ColourGradient sg (
            juce::Colour (0xff272742), cx - headRx, cy,
            juce::Colour (0xff181830), cx + headRx, cy,
            false);
        g.setGradientFill (sg);
        g.fillRect (cx - headRx, cy, headRx * 2.0f, shellH);

        // Edge highlights
        g.setColour (juce::Colour (0xff353552));
        g.fillRect (cx - headRx,        cy, 2.0f, shellH);
        g.fillRect (cx + headRx - 2.0f, cy, 2.0f, shellH);
    }

    // ── 2. Lug casings  (4 on each side, evenly distributed) ─────────────────
    {
        const float lugW = 7.0f;
        const float lugH = 14.0f;
        const float gap  = (shellH - lugH * 4.0f) / 5.0f;

        for (int i = 0; i < 4; ++i)
        {
            const float ly = cy + gap + i * (lugH + gap);

            // Casing body
            g.setColour (juce::Colour (0xff303050));
            g.fillRoundedRectangle (cx - headRx - lugW + 1.5f, ly, lugW, lugH, 2.0f);
            g.fillRoundedRectangle (cx + headRx - 1.5f,        ly, lugW, lugH, 2.0f);

            // Tension rod screw heads
            g.setColour (juce::Colour (0xff5c5c78));
            g.fillEllipse (cx - headRx - lugW + 2.5f, ly + 1.5f, 3.0f, 3.0f);
            g.fillEllipse (cx + headRx + 0.5f,        ly + 1.5f, 3.0f, 3.0f);
        }
    }

    // ── 3. Snare-side (bottom) half-ellipse ───────────────────────────────────
    {
        g.setColour (juce::Colour (0xff141428));
        g.fillEllipse (cx - headRx, cy + shellH - headRy,
                       headRx * 2.0f, headRy * 2.0f);

        // Snare wire coils (seven thin lines)
        g.setColour (juce::Colour (0x55aaaacc));
        const float span   = headRx * 0.56f;
        const float wTop   = cy + shellH - headRy * 0.5f;
        const float wBot   = cy + shellH + headRy * 0.6f;
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

    // ── 5. Top drum head  (coated mylar gradient) ─────────────────────────────
    {
        juce::ColourGradient headGrad (
            juce::Colour (0xffe2e2ec), cx, cy,
            juce::Colour (0xff888898), cx + headRx * 0.88f, cy + headRy * 0.78f,
            true);
        g.setGradientFill (headGrad);
        g.fillEllipse (cx - headRx, cy - headRy, headRx * 2.0f, headRy * 2.0f);

        // Subtle centre target ring (common on coated heads)
        g.setColour (juce::Colour (0x20000000));
        g.drawEllipse (cx - headRx * 0.32f, cy - headRy * 0.32f,
                       headRx * 0.64f, headRy * 0.64f, 0.8f);
    }

    // ── 6. Top rim ────────────────────────────────────────────────────────────
    // Main rim
    g.setColour (juce::Colour (0xff8888a0));
    g.drawEllipse (cx - headRx - rimExt,
                   cy - headRy - rimExt * 0.35f,
                   (headRx + rimExt) * 2.0f,
                   (headRy + rimExt * 0.35f) * 2.0f,
                   3.0f);

    // Rim highlight (top arc, slightly brighter to sell the 3-D feel)
    g.setColour (juce::Colour (0xffb8b8cc));
    g.drawEllipse (cx - headRx - rimExt,
                   cy - headRy - rimExt * 0.35f,
                   (headRx + rimExt) * 2.0f,
                   (headRy + rimExt * 0.35f) * 2.0f,
                   1.0f);

    // ── 7. Snare strainer lever  (right side of shell) ────────────────────────
    {
        const float sx = cx + headRx + rimExt + 5.0f;
        const float sy = cy + shellH * 0.36f;

        // Strainer body
        g.setColour (juce::Colour (0xff3a3a54));
        g.fillRoundedRectangle (sx, sy, 18.0f, 9.0f, 3.0f);

        // Lever arm (vertical toggle)
        g.setColour (juce::Colour (0xff606078));
        g.fillRoundedRectangle (sx + 13.0f, sy - 5.0f, 6.0f, 19.0f, 2.0f);

        // Screw detail
        g.setColour (juce::Colour (0xff5a5a70));
        g.fillEllipse (sx + 2.5f, sy + 2.0f, 5.0f, 5.0f);
    }
}
