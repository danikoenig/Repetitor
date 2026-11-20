#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

using APVTS = juce::AudioProcessorValueTreeState;

RepetitorProcessor::RepetitorProcessor()
#if REPETITOR_INSTRUMENT
    // Ableton-Variante: Instrument mit (stillem) Stereo-Ausgang und MIDI-Ausgang
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#else
    : AudioProcessor (BusesProperties()),
#endif
      apvts (*this, nullptr, "Repetitor", createLayout())
{
    regenerate();
}

APVTS::ParameterLayout RepetitorProcessor::createLayout()
{
    APVTS::ParameterLayout l;
    using namespace ParamIDs;
    auto id = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };

    // Tonhoehe
    l.add (std::make_unique<juce::AudioParameterChoice> (id (root), "Root", noteNames, 0));
    l.add (std::make_unique<juce::AudioParameterInt> (id (octave), "Octave", -1, 6, 3,
        juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return "C" + juce::String (v); })));
    l.add (std::make_unique<juce::AudioParameterInt> (id (spread), "Spread", 0, 24, 12,
        juce::AudioParameterIntAttributes().withLabel ("st")));
    l.add (std::make_unique<juce::AudioParameterFloat> (id (downUp), "Down/Up", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            if (v > 0.99f)  return juce::String ("UP");
            if (v < -0.99f) return juce::String ("DOWN");
            if (std::abs (v) < 0.01f) return juce::String ("CENTER");
            return juce::String (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + "%";
        })));

    // Skala (3 Schalter)
    l.add (std::make_unique<juce::AudioParameterChoice> (id (family), "Maj/Min/Sym", juce::StringArray { "Major", "Minor", "Symmetric" }, 1));
    l.add (std::make_unique<juce::AudioParameterChoice> (id (flavor), "Flavor", juce::StringArray { "L", "C", "R" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (id (count), "Count", juce::StringArray { "Few", "More", "All" }, 1));
    l.add (std::make_unique<juce::AudioParameterBool> (id (custom), "Custom Scale", false));
    l.add (std::make_unique<juce::AudioParameterInt> (id (mask), "Custom Notes", 1, 4095, Scales::maskOf (Scales::get (1, 0, 1))));

    // Loop / Rhythmus
    l.add (std::make_unique<juce::AudioParameterChoice> (id (length), "Length", lengthNames, 11));
    l.add (std::make_unique<juce::AudioParameterBool> (id (rhythm), "Rhythm Mode", false));
    l.add (std::make_unique<juce::AudioParameterFloat> (id (density), "Density", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })));
    l.add (std::make_unique<juce::AudioParameterChoice> (id (gate), "Gate", gateNames, 3));

    // Variation (wird bei jedem Trigger neu gewuerfelt, auch im Loop)
    auto pct = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; });
    l.add (std::make_unique<juce::AudioParameterFloat> (id (skip), "Skip Chance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f, pct));
    l.add (std::make_unique<juce::AudioParameterFloat> (id (octChance), "Octave Chance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f, pct));
    l.add (std::make_unique<juce::AudioParameterChoice> (id (octDir), "Octave Direction", juce::StringArray { "DOWN", "BOTH", "UP" }, 1));
    l.add (std::make_unique<juce::AudioParameterFloat> (id (fifth), "Fifth Chance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f, pct));

    // MIDI
    l.add (std::make_unique<juce::AudioParameterChoice> (id (inMode), "Input", juce::StringArray { "ANY NOTE", "C1-D#1" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (id (thru), "Thru", false));
    l.add (std::make_unique<juce::AudioParameterInt> (id (velocity), "Clock Velocity", 1, 127, 100));
    l.add (std::make_unique<juce::AudioParameterBool> (id (resetPlay), "Reset on Play", true));

    // Kanaele
    juce::StringArray lenChoices { "GLOBAL" };
    lenChoices.addArray (lengthNames);
    juce::StringArray outChoices;
    for (int i = 1; i <= 16; ++i)
        outChoices.add ("CH " + juce::String (i));

    for (int c = 0; c < numChannels; ++c)
    {
        const auto n = " " + juce::String (c + 1);
        l.add (std::make_unique<juce::AudioParameterBool> (id (chOn (c)), "On" + n, true));
        l.add (std::make_unique<juce::AudioParameterChoice> (id (chSrc (c)), "Source" + n, sourceNames, 0));
        l.add (std::make_unique<juce::AudioParameterChoice> (id (chLen (c)), "Length" + n, lenChoices, 0));
        l.add (std::make_unique<juce::AudioParameterInt> (id (chOct (c)), "Octave" + n, -3, 3, 0,
            juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return (v > 0 ? "+" : "") + juce::String (v); })));
        l.add (std::make_unique<juce::AudioParameterChoice> (id (chOut (c)), "MIDI Out" + n, outChoices, c));
    }
    return l;
}

void RepetitorProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    for (auto& c : chans)
    {
        c.playingNote = -1;
        c.offAt = -1;
        c.heldInput = -1;
        c.lastClockStep = std::numeric_limits<juce::int64>::min();
    }
    for (auto& v : view)
        v.note.store (-1);
    wasPlaying = false;
    lastPpqEnd = -1.0;
}

//==============================================================================
int RepetitorProcessor::currentMask() const
{
    if (isCustom())
        return ((int) p (ParamIDs::mask)) | 1;
    return Scales::maskOf (Scales::get ((int) p (ParamIDs::family), (int) p (ParamIDs::flavor), (int) p (ParamIDs::count)));
}

juce::String RepetitorProcessor::scaleName() const
{
    if (isCustom())
        return "Custom";
    return Scales::get ((int) p (ParamIDs::family), (int) p (ParamIDs::flavor), (int) p (ParamIDs::count)).name;
}

static void setParam (juce::AudioProcessorValueTreeState& a, const juce::String& id, float plain)
{
    auto* prm = a.getParameter (id);
    prm->beginChangeGesture();
    prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
    prm->endChangeGesture();
}

void RepetitorProcessor::toggleKey (int pitchClass)
{
    const int rel = ((pitchClass - (int) p (ParamIDs::root)) % 12 + 12) % 12;
    if (rel == 0)
        return;   // Grundton bleibt immer drin
    const int m = currentMask() ^ (1 << rel);
    setParam (apvts, ParamIDs::mask, (float) (m | 1));
    if (! isCustom())
        setParam (apvts, ParamIDs::custom, 1.0f);
}

void RepetitorProcessor::usePresetScale()
{
    if (isCustom())
        setParam (apvts, ParamIDs::custom, 0.0f);
}

int RepetitorProcessor::effectiveLength (int ch) const
{
    const int own = (int) p (ParamIDs::chLen (ch));
    const int idx = own == 0 ? (int) p (ParamIDs::length) : own - 1;
    return lengthSteps[(size_t) juce::jlimit (0, 11, idx)];
}

int RepetitorProcessor::noteForValue (int ch, float v) const
{
    const int off = Scales::quantize (v, (int) p (ParamIDs::spread), p (ParamIDs::downUp), currentMask());
    const int base = ((int) p (ParamIDs::octave) + 2) * 12 + (int) p (ParamIDs::root);
    return juce::jlimit (0, 127, base + (int) p (ParamIDs::chOct (ch)) * 12 + off);
}

void RepetitorProcessor::requestNew()
{
    // Direkt im Message-Thread: Werte sind atomar, funktioniert auch, wenn die DAW gerade nichts verarbeitet
    for (auto& v : view)
        for (int i = 0; i < maxSteps; ++i)
        {
            v.pitch[(size_t) i].store (guiRng.nextFloat());
            v.gate[(size_t) i].store (guiRng.nextFloat());
        }
    resetRequest.store (true);
}

void RepetitorProcessor::initAll()
{
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            r->beginChangeGesture();
            r->setValueNotifyingHost (r->getDefaultValue());
            r->endChangeGesture();
        }
    resetRequest.store (true);
}

void RepetitorProcessor::regenerate()
{
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < maxSteps; ++i)
        {
            view[(size_t) ch].pitch[(size_t) i].store (rng.nextFloat());
            view[(size_t) ch].gate[(size_t) i].store (rng.nextFloat());
        }
        chans[(size_t) ch].pendingReset = true;
    }
}

//==============================================================================
void RepetitorProcessor::noteOff (int ch, int offset, juce::MidiBuffer& out)
{
    auto& c = chans[(size_t) ch];
    if (c.playingNote >= 0)
        out.addEvent (juce::MidiMessage::noteOff (c.playingOutCh, c.playingNote), offset);
    c.playingNote = -1;
    c.offAt = -1;
    c.heldInput = -1;
    view[(size_t) ch].note.store (-1);
}

void RepetitorProcessor::allNotesOff (int offset, juce::MidiBuffer& out)
{
    for (int ch = 0; ch < numChannels; ++ch)
        noteOff (ch, offset, out);
}

void RepetitorProcessor::trigger (int ch, int offset, int velocity, int inNote, double bpm, juce::MidiBuffer& out)
{
    auto& c = chans[(size_t) ch];
    auto& v = view[(size_t) ch];
    const int len = effectiveLength (ch);
    int head = v.head.load();
    int idx;

    if (len == 0)
    {
        // Length auf unendlich: immer neue Zufallswerte, die letzten 32 werden gemerkt
        head = (head + 1) % maxSteps;
        v.pitch[(size_t) head].store (rng.nextFloat());
        v.gate[(size_t) head].store (rng.nextFloat());
        v.head.store (head);
        idx = head;
        c.pos = 0;
    }
    else
    {
        // Loop ueber die juengsten <len> Werte
        if (c.prevLen != len)
            c.pos = (c.prevLen <= 0) ? 0 : c.pos % len;
        if (c.pendingReset)
            c.pos = 0;
        idx = ((head - len + 1 + c.pos) % maxSteps + maxSteps) % maxSteps;
        c.pos = (c.pos + 1) % len;
    }
    c.prevLen = len;
    c.pendingReset = false;
    v.playIndex.store (idx);
    v.flash.fetch_add (1);

    int note = noteForValue (ch, v.pitch[(size_t) idx].load());

    // Variation: frisch gewuerfelt, damit der Loop nicht statisch wird
    const bool skipped = rng.nextFloat() < p (ParamIDs::skip);
    if (rng.nextFloat() < p (ParamIDs::octChance))
    {
        const int dir = (int) p (ParamIDs::octDir);
        const int jump = dir == 0 ? -12 : dir == 2 ? 12 : (rng.nextBool() ? 12 : -12);
        if (note + jump >= 0 && note + jump <= 127) note += jump;
    }
    if (rng.nextFloat() < p (ParamIDs::fifth) && note + 7 <= 127)
        note += 7;
    v.lastNote.store (note);

    const bool on = p (ParamIDs::chOn (ch)) > 0.5f;
    const bool rhythm = p (ParamIDs::rhythm) > 0.5f;
    const bool fire = on && ! skipped && (! rhythm || v.gate[(size_t) idx].load() < p (ParamIDs::density));
    if (! fire)
        return;

    noteOff (ch, offset, out);
    const int outCh = (int) p (ParamIDs::chOut (ch)) + 1;
    out.addEvent (juce::MidiMessage::noteOn (outCh, note, (juce::uint8) juce::jlimit (1, 127, velocity)), offset);
    c.playingNote = note;
    c.playingOutCh = outCh;
    v.note.store (note);

    const int gateIdx = (int) p (ParamIDs::gate);
    if (gateIdx == 0 && inNote >= 0)
    {
        c.heldInput = inNote;   // Note-Off kommt mit dem Loslassen der Eingangsnote
        c.offAt = -1;
    }
    else
    {
        const double beats = gateIdx == 0 ? 0.25 : gateBeats[(size_t) gateIdx];
        const double samples = juce::jmax (sr * 0.001, beats * 60.0 / bpm * sr);
        c.offAt = sampleClock + offset + (juce::int64) samples;
    }
}

void RepetitorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    juce::MidiBuffer out;

    if (newRequest.exchange (false))
        regenerate();
    if (resetRequest.exchange (false))
        for (auto& c : chans)
            c.pendingReset = true;

    // Transport
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, hasPpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 999.0, *b);
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; hasPpq = true; }
        }

    if (playing && ! wasPlaying)
    {
        if (p (ParamIDs::resetPlay) > 0.5f)
            for (auto& c : chans)
                c.pendingReset = true;
        for (auto& c : chans)
            c.lastClockStep = std::numeric_limits<juce::int64>::min();
    }
    if (! playing && wasPlaying)
    {
        allNotesOff (0, out);
        lastPpqEnd = -1.0;
    }

    events.clear();

    // MIDI-Eingang: Noten sind Trigger (wie die Trigger-Eingaenge am Modul, normalisiert)
    const bool split = (int) p (ParamIDs::inMode) == 1;
    const bool thru = p (ParamIDs::thru) > 0.5f;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos = juce::jlimit (0, n - 1, meta.samplePosition);
        if (msg.isNoteOn())
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if ((int) p (ParamIDs::chSrc (ch)) != 0)
                    continue;
                if (split && msg.getNoteNumber() != 36 + ch)
                    continue;
                events.push_back ({ pos, ch, 0, msg.getVelocity(), msg.getNoteNumber() });
            }
            if (thru) out.addEvent (msg, pos);
        }
        else if (msg.isNoteOff())
        {
            for (int ch = 0; ch < numChannels; ++ch)
                events.push_back ({ pos, ch, 1, 0, msg.getNoteNumber() });
            if (thru) out.addEvent (msg, pos);
        }
        else
        {
            out.addEvent (msg, pos);   // CC, Pitchbend usw. gehen durch
        }
    }

    // Interne Clock, synchron zum DAW-Tempo (laeuft nur, wenn die DAW spielt)
    if (playing && hasPpq)
    {
        const double bps = bpm / 60.0 / sr;
        const double ppqEnd = ppq + bps * n;
        if (lastPpqEnd >= 0.0 && (ppq < lastPpqEnd - 1.0e-4 || ppq > lastPpqEnd + 0.25))
            for (auto& c : chans)   // Sprung (Loop in der DAW, Locator versetzt)
                c.lastClockStep = std::numeric_limits<juce::int64>::min();

        const int vel = (int) p (ParamIDs::velocity);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const int src = (int) p (ParamIDs::chSrc (ch));
            if (src == 0)
                continue;
            const double r = sourceBeats[(size_t) src];
            for (auto k = (juce::int64) std::ceil (ppq / r - 1.0e-6); (double) k * r < ppqEnd; ++k)
            {
                if (k <= chans[(size_t) ch].lastClockStep)
                    continue;
                chans[(size_t) ch].lastClockStep = k;
                const int off = juce::jlimit (0, n - 1, (int) std::llround (((double) k * r - ppq) / bps));
                events.push_back ({ off, ch, 0, vel, -1 });
            }
        }
        lastPpqEnd = ppqEnd;
    }

    std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b)
    {
        if (a.offset != b.offset) return a.offset < b.offset;
        return a.kind > b.kind;   // Loslassen vor neuem Trigger
    });

    auto flushOffs = [&] (int upTo)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& c = chans[(size_t) ch];
            if (c.playingNote >= 0 && c.offAt >= 0 && c.offAt - sampleClock <= upTo)
                noteOff (ch, juce::jlimit (0, n - 1, (int) (c.offAt - sampleClock)), out);
        }
    };

    for (const auto& e : events)
    {
        flushOffs (e.offset);
        if (e.kind == 0)
            trigger (e.ch, e.offset, e.velocity, e.inNote, bpm, out);
        else if (chans[(size_t) e.ch].heldInput >= 0 && chans[(size_t) e.ch].heldInput == e.inNote)
            noteOff (e.ch, e.offset, out);
    }
    flushOffs (n - 1);

    midi.swapWith (out);
    sampleClock += n;
    wasPlaying = playing;
}

//==============================================================================
void RepetitorProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    juce::ValueTree loops ("Loops");
    for (int ch = 0; ch < numChannels; ++ch)
    {
        juce::String ps, gs;
        for (int i = 0; i < maxSteps; ++i)
        {
            ps << juce::String (view[(size_t) ch].pitch[(size_t) i].load(), 5) << " ";
            gs << juce::String (view[(size_t) ch].gate[(size_t) i].load(), 5) << " ";
        }
        juce::ValueTree c ("Ch");
        c.setProperty ("p", ps.trim(), nullptr);
        c.setProperty ("g", gs.trim(), nullptr);
        c.setProperty ("h", view[(size_t) ch].head.load(), nullptr);
        loops.setProperty ("uiScale", uiScale.load(), nullptr);
        loops.appendChild (c, nullptr);
    }
    state.removeChild (state.getChildWithName ("Loops"), nullptr);
    state.appendChild (loops, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void RepetitorProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr)
        return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType()))
        return;

    if (auto loops = state.getChildWithName ("Loops"); loops.isValid())
    {
        uiScale.store (juce::jlimit (0.6f, 2.0f, (float) loops.getProperty ("uiScale", 1.0f)));
        for (int ch = 0; ch < numChannels && ch < loops.getNumChildren(); ++ch)
        {
            auto c = loops.getChild (ch);
            juce::StringArray ps, gs;
            ps.addTokens (c["p"].toString(), " ", {});
            gs.addTokens (c["g"].toString(), " ", {});
            for (int i = 0; i < maxSteps; ++i)
            {
                if (i < ps.size()) view[(size_t) ch].pitch[(size_t) i].store (juce::jlimit (0.0f, 0.9999f, ps[i].getFloatValue()));
                if (i < gs.size()) view[(size_t) ch].gate[(size_t) i].store (juce::jlimit (0.0f, 0.9999f, gs[i].getFloatValue()));
            }
            view[(size_t) ch].head.store (juce::jlimit (0, maxSteps - 1, (int) c.getProperty ("h", maxSteps - 1)));
        }
        state.removeChild (loops, nullptr);
        resetRequest.store (true);
    }
    apvts.replaceState (state);
}

juce::AudioProcessorEditor* RepetitorProcessor::createEditor() { return new RepetitorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RepetitorProcessor(); }
