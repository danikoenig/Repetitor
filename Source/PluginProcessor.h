#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "Scales.h"

constexpr int numChannels = 4;
constexpr int maxSteps = 32;

namespace ParamIDs
{
    // Global
    inline const juce::String root      { "root" };
    inline const juce::String octave    { "octave" };
    inline const juce::String spread    { "spread" };
    inline const juce::String downUp    { "downUp" };
    inline const juce::String family    { "family" };
    inline const juce::String flavor    { "flavor" };
    inline const juce::String count     { "count" };
    inline const juce::String length    { "length" };
    inline const juce::String rhythm    { "rhythm" };
    inline const juce::String density   { "density" };
    inline const juce::String gate      { "gate" };
    inline const juce::String inMode    { "inMode" };
    inline const juce::String thru      { "thru" };
    inline const juce::String velocity  { "velocity" };
    inline const juce::String resetPlay { "resetOnPlay" };
    inline const juce::String custom    { "customScale" };
    inline const juce::String skip      { "skip" };
    inline const juce::String octChance { "octChance" };
    inline const juce::String octDir    { "octDir" };
    inline const juce::String fifth     { "fifthChance" };
    inline const juce::String mask      { "customMask" };   // eigene Skala, Bits relativ zum Root

    // Pro Kanal (1..4)
    inline juce::String chOn  (int c) { return "on"  + juce::String (c + 1); }
    inline juce::String chSrc (int c) { return "src" + juce::String (c + 1); }
    inline juce::String chLen (int c) { return "len" + juce::String (c + 1); }
    inline juce::String chOct (int c) { return "oct" + juce::String (c + 1); }
    inline juce::String chOut (int c) { return "out" + juce::String (c + 1); }
}

// Length-Stufen wie am Modul (12 Stellungen), 0 = unendlich (immer neue Werte)
inline const std::array<int, 12> lengthSteps { 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32, 0 };
inline const juce::StringArray lengthNames { "1", "2", "3", "4", "5", "6", "8", "12", "16", "24", "32", "INF" };

// Trigger-Quelle pro Kanal: MIDI-Eingang oder Clock synchron zum DAW-Tempo
inline const juce::StringArray sourceNames { "MIDI IN", "1/32", "1/16T", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 BAR" };
inline const std::array<double, 10> sourceBeats { 0.0, 0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.5, 2.0 / 3.0, 1.0, 2.0, 4.0 };

inline const juce::StringArray gateNames { "INPUT", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2" };
inline const std::array<double, 7> gateBeats { 0.0, 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0 };

inline const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline juce::String noteName (int midi) { return midi < 0 ? juce::String ("-") : noteNames[midi % 12] + juce::String (midi / 12 - 2); } // C3 = 60 (Bitwig / Ableton)

class RepetitorProcessor : public juce::AudioProcessor
{
public:
    RepetitorProcessor();
    ~RepetitorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet().isDisabled() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

   #ifndef REPETITOR_INSTRUMENT
    #define REPETITOR_INSTRUMENT 0
   #endif
    const juce::String getName() const override { return REPETITOR_INSTRUMENT ? "Repetitor Instrument" : "Repetitor"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return ! REPETITOR_INSTRUMENT; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // Bedienung aus dem GUI
    void requestReset()  { resetRequest.store (true); }
    void requestNew()    { newRequest.store (true); }

    // Anzeige (vom GUI gelesen)
    struct ChannelView
    {
        std::array<std::atomic<float>, maxSteps> pitch {};   // Zufallswert 0..1 je Speicherplatz
        std::array<std::atomic<float>, maxSteps> gate {};    // Zufallswert fuer den Rhythmus
        std::atomic<int> head { maxSteps - 1 };              // juengster Wert
        std::atomic<int> playIndex { -1 };                   // zuletzt gespielter Speicherplatz
        std::atomic<int> note { -1 };                        // klingende Note (-1 = keine)
        std::atomic<int> lastNote { -1 };                    // zuletzt erzeugte Note
        std::atomic<int> flash { 0 };                        // zaehlt bei jedem Trigger hoch
    };
    std::array<ChannelView, numChannels> view;

    // Hilfen fuer GUI und Test
    int effectiveLength (int ch) const;                      // 0 = unendlich
    int noteForValue (int ch, float v) const;
    int currentMask() const;                                 // aktive Skala als 12-Bit-Maske (relativ zum Root)
    bool isCustom() const { return p (ParamIDs::custom) > 0.5f; }
    juce::String scaleName() const;
    void toggleKey (int pitchClass);                         // Taste auf dem Mini-Keyboard (C = 0 ... B = 11)
    void usePresetScale();                                   // zurueck zu den Schaltern

private:
    friend struct ReelAccess;   // nur fuer den Reel-Renderer (tests/ReelRender.cpp)
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Channel
    {
        int pos = 0;
        bool pendingReset = false;
        int prevLen = -1;
        int playingNote = -1;
        int playingOutCh = 1;
        juce::int64 offAt = -1;          // absolute Sample-Position fuer Note-Off, -1 = keine
        int heldInput = -1;              // ausloesende Eingangsnote (Gate = INPUT)
        juce::int64 lastClockStep = std::numeric_limits<juce::int64>::min();
    };
    std::array<Channel, numChannels> chans;

    struct Event
    {
        int offset;
        int ch;
        int kind;      // 0 = Trigger, 1 = Eingangsnote losgelassen
        int velocity;
        int inNote;
    };
    std::vector<Event> events;

    juce::Random rng;
    double sr = 44100.0;
    juce::int64 sampleClock = 0;
    bool wasPlaying = false;
    double lastPpqEnd = -1.0;
    std::atomic<bool> resetRequest { false }, newRequest { false };

    void trigger (int ch, int offset, int velocity, int inNote, double bpm, juce::MidiBuffer& out);
    void noteOff (int ch, int offset, juce::MidiBuffer& out);
    void allNotesOff (int offset, juce::MidiBuffer& out);
    void regenerate();

    float p (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RepetitorProcessor)
};
