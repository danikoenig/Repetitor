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
    void timerCallback() override;
    void updateLengthColour();

    RepetitorProcessor& processor;
    RepLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 700 };

    juce::TextButton newButton { "NEW" }, resetButton { "RESET" };

    // Pitch
    Knob kRoot, kOctave, kSpread, kDownUp;
    // Scale
    std::unique_ptr<ChoiceSwitch> famSwitch, flavSwitch, countSwitch;
    // Loop
    Knob kLength, kDensity;
    juce::ToggleButton rhythmButton { "RHYTHM" };
    juce::ComboBox gateBox;
    // MIDI
    juce::ComboBox inBox;
    juce::ToggleButton thruButton { "THRU" }, resetPlayButton { "RESET ON PLAY" };
    Knob kVel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rhythmAtt, thruAtt, resetPlayAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> gateAtt, inAtt;

    std::array<std::unique_ptr<ChannelRow>, numChannels> rows;

    juce::Rectangle<int> secPitch, secScale, secLoop, secMidi, scaleInfo, gateLabel, inLabel;
    int shownScale = -1, shownRoot = -1, shownLen = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RepetitorEditor)
};
