#pragma once
#include <JuceHeader.h>

// =============================================================================
// EnvelopeModeToggle  --  SIMPLE | TRUE display-mode selector
//
// A lightweight painted toggle with two mutually-exclusive halves.
// Intended to sit in the top-right corner of each EnvelopeEditor.
// =============================================================================

class EnvelopeModeToggle : public juce::Component
{
public:
    enum class DisplayMode { Simple, True };

    EnvelopeModeToggle()
    {
        setInterceptsMouseClicks (true, false);
    }

    DisplayMode getMode() const noexcept { return mode; }

    void setMode (DisplayMode m)
    {
        if (mode != m)
        {
            mode = m;
            repaint();
            if (onChange)
                onChange (mode);
        }
    }

    std::function<void (DisplayMode)> onChange;

    // ─── paint ──────────────────────────────────────────────────────────────
    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth();
        const float h = (float) getHeight();
        const float half = w * 0.5f;

        const juce::Colour activeBg   (0xff1e2229);
        const juce::Colour inactiveBg (0xff181c22);
        const juce::Colour activeText (0xffffffff);
        const juce::Colour inactiveText (0xff555566);

        const bool simpleActive = (mode == DisplayMode::Simple);

        // Left half (SIMPLE)
        {
            juce::Path p;
            p.addRoundedRectangle (0.0f, 0.0f, half, h, kCorner, kCorner,
                                   true, false, true, false);
            g.setColour (simpleActive ? activeBg : inactiveBg);
            g.fillPath (p);

            g.setColour (simpleActive ? activeText : inactiveText);
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
            g.drawText ("SIMPLE", juce::Rectangle<float> (0.0f, 0.0f, half, h),
                        juce::Justification::centred, false);
        }

        // Right half (TRUE)
        {
            juce::Path p;
            p.addRoundedRectangle (half, 0.0f, w - half, h, kCorner, kCorner,
                                   false, true, false, true);
            g.setColour (simpleActive ? inactiveBg : activeBg);
            g.fillPath (p);

            g.setColour (simpleActive ? inactiveText : activeText);
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (11.0f)));
            g.drawText ("TRUE", juce::Rectangle<float> (half, 0.0f, w - half, h),
                        juce::Justification::centred, false);
        }
    }

    // ─── mouse ──────────────────────────────────────────────────────────────
    void mouseDown (const juce::MouseEvent& e) override
    {
        const float half = (float) getWidth() * 0.5f;
        setMode (e.position.x < half ? DisplayMode::Simple : DisplayMode::True);
    }

private:
    DisplayMode mode = DisplayMode::True;
    static constexpr float kCorner = 8.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeModeToggle)
};
