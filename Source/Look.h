#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

// Look & Feel wie One Shooter (Knobs, Pills, Segment-Schalter, Scrollbars)
class RepLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RepLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.0f, juce::Font::bold); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// Vertikales Meter (Pegel oder Gain Reduction)
class Meter : public juce::Component
{
public:
    Meter (juce::Colour c, bool reduction) : colour (c), isReduction (reduction) {}
    void setValue (float v) { if (std::abs (v - value) > 0.001f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::Colour colour;
    bool isReduction;
    float value = 0.0f;
};
