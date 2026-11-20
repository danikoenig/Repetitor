#pragma once

#include "PluginProcessor.h"
#include "Look.h"
#include "Knob.h"
#include "Widgets.h"

// Eine Zeile pro Kanal: Ein/Aus, Quelle, Length, Oktave, MIDI-Kanal, Step-Anzeige, Note
class ChannelRow : public juce::Component
{
public:
    ChannelRow (RepetitorProcessor&, int channel);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    friend struct ReelAccess;
    RepetitorProcessor& proc;
    int ch;
    juce::ToggleButton onButton;
    juce::ComboBox srcBox, lenBox, outBox;
    Knob kOct;
    StepView steps;
    int shownNote = -2, shownFlash = -1;
    float glow = 0.0f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAtt, lenAtt, outAtt;
    juce::Rectangle<int> noteArea, badgeArea;
};

class RepetitorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RepetitorEditor (RepetitorProcessor&);
    ~RepetitorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    friend struct ReelAccess;
    void timerCallback() override;
    void updateLengthColour();
    int keyAt (juce::Point<int>) const;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    int hoverKey = -1;

    RepetitorProcessor& processor;
    RepLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 700 };

    juce::TextButton newButton { "NEW" }, resetButton { "INIT" };
    float scale = 1.0f;
    void repaintArea (juce::Rectangle<int> r) { repaint (r.toFloat().transformedBy (juce::AffineTransform::scale (scale)).getSmallestIntegerContainer().expanded (2)); }

    // Pitch
    Knob kRoot, kOctave, kSpread, kDownUp;
    // Scale
    std::unique_ptr<ChoiceSwitch> famSwitch, flavSwitch, countSwitch;
    // Loop
    Knob kLength, kDensity;
    juce::ToggleButton rhythmButton { "RHYTHM" };
    juce::ComboBox gateBox;
    // Vary
    Knob kSkip, kOctChance, kFifth;
    std::unique_ptr<ChoiceSwitch> octDirSwitch;
    // MIDI
    juce::ComboBox inBox;
    juce::ToggleButton thruButton { "THRU" }, resetPlayButton { "RESET ON PLAY" };
    Knob kVel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rhythmAtt, thruAtt, resetPlayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> gateAtt, inAtt;

    std::array<std::unique_ptr<ChannelRow>, numChannels> rows;

    juce::Rectangle<int> secPitch, secScale, secLoop, secVary, secMidi, octDirLabel, scaleInfo, gateLabel, inLabel;
    int shownScale = -1, shownRoot = -1, shownLen = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RepetitorEditor)
};
