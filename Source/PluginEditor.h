#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeEditor.h"
#include "NoiseFilterVisualizer.h"

// =============================================================================
// SnareMakerAudioProcessorEditor  –  Clean UI (envelope-only)
//
// Window layout (960 × 520 px):
//
//  ┌──────────────────────────────────────────────────────────────────┐
//  │  HEADER  (plugin name + BODY/NOISE tabs)                  50 px │
//  ├──────────────────────────────────────────────────┬─────────────┤
//  │  ENVELOPE EDITOR + PITCH/AMP buttons             │   OUTPUT   │
//  │  (drum visual behind)                            │    95 px   │
//  │                                                  │             │  398 px
//  ├──────────────────────────────────────────────────┴─────────────┤
//  │  ROOM / SPACE ZONE  (full width 960 px)                  72 px │
//  └──────────────────────────────────────────────────────────────────┘
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

        juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

        juce::Font getLabelFont     (juce::Label&) override;
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
        juce::Font getComboBoxFont  (juce::ComboBox&) override;
        juce::Font getPopupMenuFont () override;
        juce::Font getSliderPopupFont (juce::Slider&) override;

        void drawLinearSlider (juce::Graphics&,
                               int x, int y, int width, int height,
                               float sliderPos,
                               float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle,
                               juce::Slider&) override;

        void drawRotarySlider (juce::Graphics&,
                               int x, int y, int width, int height,
                               float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;

        void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
        void drawPopupMenuItem (juce::Graphics&,
                                const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive, bool isHighlighted,
                                bool isTicked, bool hasSubMenu,
                                const juce::String& text,
                                const juce::String& shortcutKeyText,
                                const juce::Drawable* icon,
                                const juce::Colour* textColour) override;
        void getIdealPopupMenuItemSize (const juce::String& text,
                                        bool isSeparator, int standardMenuItemHeight,
                                        int& idealWidth, int& idealHeight) override;

        juce::Component* getParentComponentForMenuOptions (const juce::PopupMenu::Options&) override;
        void preparePopupMenuWindow (juce::Component& window) override;

        // Helpers for paint functions
        juce::Font interRegularFont (float height) const;
        juce::Font interMediumFont  (float height) const;

        juce::Typeface::Ptr interRegular;
        juce::Typeface::Ptr interMedium;
    };

    // ── Top-level tabs ──────────────────────────────────────────────────────
    enum class Tab { Transient, Body, Resonant, Noise, Room, Sauce };
    Tab activeTab  { Tab::Body };
    Tab hoveredTab { Tab::Body };

    juce::Rectangle<int> transientTabBounds;
    juce::Rectangle<int> bodyTabBounds;
    juce::Rectangle<int> resonantTabBounds;
    juce::Rectangle<int> noiseTabBounds;
    juce::Rectangle<int> roomTabBounds;
    juce::Rectangle<int> sauceTabBounds;

    bool tabEnabled[6] { true, true, true, true, true, true };
    bool& tabEnabledFor (Tab tab)       { return tabEnabled[static_cast<int>(tab)]; }
    bool  tabEnabledFor (Tab tab) const { return tabEnabled[static_cast<int>(tab)]; }

    void setActiveTab (Tab tab);    // show/hide controls + repaint
    void paintTabs    (juce::Graphics&) const;

    // ── Zone identifiers ─────────────────────────────────────────────────────
    enum class Zone { None, Output };
    Zone hoveredZone { Zone::None };
    Zone activeZone  { Zone::None };

    // ── Zone rectangles (set in resized()) ───────────────────────────────────
    juce::Rectangle<int> drumAreaBounds;
    juce::Rectangle<int> outputZoneBounds;

    // ── Lifetime-ordered members ──────────────────────────────────────────────
    //    audioProcessor ref first, lnf before all child components,
    //    attachments last (destroyed first).
    SnareMakerAudioProcessor& audioProcessor;
    SnareLookAndFeel           lnf;

    // ── Envelope editor (display-only, sits over drum area) ─────────────────
    EnvelopeEditor             envelopeEditor;
    juce::Rectangle<int>       envEditorFullBounds;   // full-width bounds (set in resized)

    // ── Transient waveform bounds (80% width, set in resized) ────────────────
    juce::Rectangle<int>       transientWaveBounds;

    // ── Envelope mode buttons (below envelope editor) ────────────────────────
    enum class EnvMode { Pitch, TransientAmp, BodyAmp, NoiseAmp, ResonantAmp, RoomAmp };
    EnvMode envMode { EnvMode::Pitch };

    juce::Rectangle<int> envModeBounds;   // hit-test for AMP|PITCH selector
    juce::TextButton noiseAmpBtn  { "AMP" };

    void setEnvMode (EnvMode mode);

    // ── Noise source selector (GEN / SAMPLE, UI only) ────────────────────
    enum class NoiseSrc { Gen, Sample };
    NoiseSrc noiseSrc { NoiseSrc::Gen };
    juce::Rectangle<int> noiseSrcBounds;   // hit-test area (set during paint)

    // ── Noise type selector (WHITE…VIOLET, UI only) ────────────────────
    enum class NoiseType { White, Pink, Brown, Blue, Violet };
    NoiseType noiseType { NoiseType::White };
    juce::Rectangle<int> noiseTypeBounds;  // hit-test area (set during paint)

    // ── Noise filter visualizer (GEN mode) ──────────────────────────────────
    NoiseFilterVisualizer       noiseFilterVis;
    juce::Rectangle<int>        noiseFilterBounds;

    // ── Noise sample panel controls ─────────────────────────────────────────
    NoiseFilterVisualizer       noiseSampleFilterVis;
    juce::Rectangle<int>        noiseSampleFilterBounds;
    juce::Rectangle<int>        noiseSampleBtnBounds;      // LOAD SAMPLE full container
    juce::Rectangle<int>        noiseSampleDropBounds;     // dropdown hit-test
    juce::TextButton            noiseSamplePrevBtn  { "<" };
    juce::TextButton            noiseSampleNextBtn  { ">" };

    // ── Transient filter visualizer ──────────────────────────────────────────
    NoiseFilterVisualizer       transientFilterVis;
    juce::Rectangle<int>        transientFilterBounds;
    juce::Rectangle<int>        transientSampleBtnBounds;  // LOAD SAMPLE full container
    juce::Rectangle<int>        transientDropBounds;       // dropdown hit-test

    // ── Transient sample browser buttons ─────────────────────────────────────
    juce::TextButton            transientPrevBtn  { "<" };
    juce::TextButton            transientNextBtn  { ">" };

    // ── Room control panel ─────────────────────────────────────────────────
    NoiseFilterVisualizer       roomFilterVis;
    juce::Rectangle<int>        roomFilterBounds;
    juce::Rectangle<int>        roomIrBtnBounds;       // LOAD IR full container
    juce::Rectangle<int>        roomDropBounds;        // dropdown hit-test
    juce::TextButton            roomPrevBtn  { "<" };
    juce::TextButton            roomNextBtn  { ">" };

    // ── Resonant control panel ──────────────────────────────────────────────
    NoiseFilterVisualizer       resonantFilterVis;
    juce::Rectangle<int>        resonantFilterBounds;
    juce::Rectangle<int>        resonantSampleBtnBounds;  // LOAD RESONANCE full container
    juce::Rectangle<int>        resonantDropBounds;       // dropdown hit-test
    juce::TextButton            resonantPrevBtn  { "<" };
    juce::TextButton            resonantNextBtn  { ">" };
    juce::TextButton            resonantFollowBodyBtn { "FOLLOW BODY" };

    // ── Dropdown open state (for flattening container bottom corners) ───────
    enum class OpenDropdown { None, TransientSample, NoiseSample, RoomIr, ResonantSample };
    OpenDropdown openDropdown { OpenDropdown::None };

    // ── Sauce knob (visual only, no APVTS) ──────────────────────────────────
    juce::Slider sauceKnob;

    // ── Value popup bubble (shared struct for faders) ──────────────────────
    struct OutputValueBubble : public juce::Component
    {
        juce::String text;
        juce::Typeface::Ptr typeface;
        void paint (juce::Graphics&) override;
    };

    // ── Phase offset fader (BODY tab only, inside envelope area) ─────────────
    juce::Slider phaseOffsetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaseOffsetAttachment;
    OutputValueBubble phaseBubble;
    void updatePhaseBubble();

    // ── Per-layer volume knob (swapped per tab, Phase 9) ──────────────────
    juce::Slider layerVolumeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> layerVolumeAttachment;
    OutputValueBubble volumeBubble;
    void updateVolumeBubble();

    // ── Output fader ─────────────────────────────────────────────────────────
    juce::Slider outputSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    // ── Output value bubble (visible during drag) ─────────────────────────
    OutputValueBubble outputBubble;
    void updateOutputBubble();

    // ── Preset combo (visual only) ─────────────────────────────────────────
    juce::ComboBox presetCombo;

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
    void paintDrumArea  (juce::Graphics&, juce::Rectangle<int> area);
    void paintSnareDrum (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessorEditor)
};
