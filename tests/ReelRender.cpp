// Reel-Renderer: spielt einen festen Ablauf im Plugin ab, spielt die MIDI-Ausgabe mit einem kleinen
// Ambient-Synth (Bells, Glass, Sub, Pad + Delay + Reverb) als WAV, rendert GUI-Frames (JPG, 30 fps)
// und schreibt Cursor/Layout-Daten (events.json) fuer tools/reel/compose.py.
// Build: cmake -DREPETITOR_TEST_SRC=tests/ReelRender.cpp ...  Aufruf: RepetitorTest <out-dir>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <functional>

static constexpr double FPS = 30.0, SR = 48000.0, BPM = 84.0;
static constexpr double BEAT = 60.0 / BPM, BAR = 4.0 * BEAT;
static constexpr double TOTAL = 12.0 * BAR;          // ca. 34 s
static constexpr double GUI_UNTIL = 10.0 * BAR;      // danach Endkarte

//==============================================================================
// Kleiner Ambient-Synth: Timbre je MIDI-Kanal
struct AmbientSynth
{
    struct Voice
    {
        bool active = false, released = false;
        int ch = 0, note = 0;
        double freq = 0, ph1 = 0, ph2 = 0, ph3 = 0, t = 0, relT = 0, relLevel = 0, vel = 0;
        double lp1 = 0, lp2 = 0, lfo = 0;
    };
    std::array<Voice, 48> voices;
    juce::Random rnd { 7 };

    struct Patch { double a, d, s, r, gain, pan; };
    static Patch patch (int ch)
    {
        switch (ch)
        {
            case 1:  return { 0.25, 0.0, 0.85, 2.2, 0.55, 0.50 };   // Sub (warm)
            case 2:  return { 0.003, 2.4, 0.0, 1.6, 0.30, 0.30 };   // Bell
            case 3:  return { 0.006, 3.0, 0.0, 2.0, 0.17, 0.72 };   // Glass
            default: return { 1.6, 0.0, 0.9, 3.0, 0.22, 0.55 };     // Pad
        }
    }

    void noteOn (int ch, int note, int vel)
    {
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) v = &voices[0];
        *v = {};
        v->active = true; v->ch = ch; v->note = note; v->vel = vel / 127.0;
        v->freq = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        v->lfo = rnd.nextDouble() * 6.28;
    }
    void noteOff (int ch, int note)
    {
        for (auto& v : voices)
            if (v.active && ! v.released && v.ch == ch && v.note == note)
            {
                v.relLevel = env (v); v.released = true; v.relT = 0;
            }
    }
    static double env (const Voice& v)
    {
        const auto P = patch (v.ch);
        double e;
        if (v.t < P.a) e = v.t / P.a;
        else if (P.d > 0) e = P.s + (1.0 - P.s) * std::exp (-(v.t - P.a) / (P.d * 0.35));
        else e = P.s;
        return e;
    }
    void render (float& outL, float& outR)
    {
        double L = 0, R = 0;
        const double dt = 1.0 / SR;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const auto P = patch (v.ch);
            double e = v.released ? v.relLevel * std::exp (-v.relT / (P.r * 0.3)) : env (v);
            if (v.released && v.relT > P.r * 2.5) { v.active = false; continue; }
            if (! v.released && P.d > 0 && P.s == 0 && v.t > P.a + P.d * 3.0) { v.active = false; continue; }

            const double tw = juce::MathConstants<double>::twoPi;
            double s = 0;
            switch (v.ch)
            {
                case 1:  // Sub: Sinus + etwas Oktave
                    s = std::sin (v.ph1) + 0.18 * std::sin (2.0 * v.ph1);
                    v.ph1 += tw * v.freq * dt;
                    break;
                case 2:  // Bell: FM, Index faellt mit der Huellkurve
                {
                    const double idx = 1.4 * e + 0.15;
                    s = std::sin (v.ph1 + idx * std::sin (v.ph2));
                    v.ph1 += tw * v.freq * dt;
                    v.ph2 += tw * v.freq * 3.0 * dt;
                    break;
                }
                case 3:  // Glass: zwei leicht verstimmte Sinus + Oberton
                    s = std::sin (v.ph1) + std::sin (v.ph2) * 0.8 + 0.25 * std::sin (v.ph3);
                    v.ph1 += tw * v.freq * dt;
                    v.ph2 += tw * v.freq * 1.004 * dt;
                    v.ph3 += tw * v.freq * 2.0 * dt;
                    s *= 0.5;
                    break;
                default: // Pad: zwei verstimmte Saegezaehne, stark gefiltert, langsame Bewegung
                {
                    auto saw = [] (double ph) { return 2.0 * (ph / juce::MathConstants<double>::twoPi - std::floor (ph / juce::MathConstants<double>::twoPi + 0.5)); };
                    const double raw = 0.5 * (saw (v.ph1) + saw (v.ph2));
                    v.ph1 += tw * v.freq * 0.997 * dt;
                    v.ph2 += tw * v.freq * 1.003 * dt;
                    const double cut = 500.0 + 350.0 * std::sin (v.lfo + v.t * 0.6);
                    const double a = 1.0 - std::exp (-tw * cut * dt);
                    v.lp1 += a * (raw - v.lp1);
                    v.lp2 += a * (v.lp1 - v.lp2);
                    s = v.lp2 * 1.6;
                    break;
                }
            }
            s *= e * P.gain * (0.55 + 0.45 * v.vel);
            const double pan = P.pan + (v.ch == 2 || v.ch == 3 ? 0.12 * std::sin (v.lfo) : 0.0);
            L += s * std::cos (pan * juce::MathConstants<double>::halfPi);
            R += s * std::sin (pan * juce::MathConstants<double>::halfPi);
            v.t += dt;
            if (v.released) v.relT += dt;
        }
        outL = (float) L; outR = (float) R;
    }
};

//==============================================================================
struct ReelAccess
{
    static void tick (RepetitorEditor& e) { e.timerCallback(); }
    static juce::Rectangle<int> in (RepetitorEditor& e, juce::Component& c) { return e.getLocalArea (c.getParentComponent(), c.getBounds()); }

    static juce::var rects (RepetitorEditor& e)
    {
        auto* o = new juce::DynamicObject();
        auto put = [o] (const char* k, juce::Rectangle<int> r)
        {
            juce::Array<juce::var> a { r.getX(), r.getY(), r.getWidth(), r.getHeight() };
            o->setProperty (k, a);
        };
        put ("gui", e.getLocalBounds());
        put ("pitch", e.secPitch);
        put ("scale", e.secScale);
        put ("loop", e.secLoop);
        put ("vary", e.secVary);
        put ("midi", e.secMidi);
        for (int c = 0; c < numChannels; ++c)
            put (("row" + juce::String (c + 1)).toRawUTF8(), e.rows[(size_t) c]->getBounds());
        return juce::var (o);
    }
    static int run (int argc, char** argv);
};

struct ReelPlayHead : juce::AudioPlayHead
{
    double timeSec = 0.0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (BPM);
        p.setTimeSignature (TimeSignature { 4, 4 });
        p.setIsPlaying (true);
        p.setPpqPosition (timeSec * BPM / 60.0);
        return p;
    }
};

static void setParam (RepetitorProcessor& p, const juce::String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static double ease (double x) { x = juce::jlimit (0.0, 1.0, x); return x * x * (3.0 - 2.0 * x); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    return ReelAccess::run (argc, argv);
}

int ReelAccess::run (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: RepetitorTest <out-dir> [seed]\n"); return 1; }
    const juce::File outDir { juce::String (argv[1]) };
    const juce::int64 seed = argc > 2 ? juce::String (argv[2]).getLargeIntValue() : 2026;
    outDir.getChildFile ("frames").createDirectory();

    RepetitorProcessor p;
    ReelPlayHead ph;
    p.setPlayHead (&ph);
    p.prepareToPlay (SR, 512);
    p.rng.setSeed (seed);
    p.regenerate();

    // Ambient-Grundeinstellung: A Moll-Pentatonik, langsam
    setParam (p, ParamIDs::root, 9.0f);          // A
    setParam (p, ParamIDs::octave, 2.0f);        // A2 = 57
    setParam (p, ParamIDs::spread, 12.0f);
    setParam (p, ParamIDs::downUp, 1.0f);
    setParam (p, ParamIDs::family, 1.0f);
    setParam (p, ParamIDs::flavor, 0.0f);
    setParam (p, ParamIDs::count, 1.0f);
    setParam (p, ParamIDs::length, 11.0f);       // INF
    setParam (p, ParamIDs::gate, 6.0f);          // 1/2
    setParam (p, ParamIDs::density, 0.5f);
    // Kanal 1 Sub 1/2 tief, Kanal 2 Bell 1/8, Kanal 3 Glass 1/4 hoch, Kanal 4 Pad 1 Bar
    const float src[] { 8.0f, 5.0f, 7.0f, 9.0f };
    const float oct[] { -1.0f, 0.0f, 1.0f, 0.0f };
    for (int c = 0; c < numChannels; ++c)
    {
        setParam (p, ParamIDs::chSrc (c), src[c]);
        setParam (p, ParamIDs::chOct (c), oct[c]);
        setParam (p, ParamIDs::chOut (c), (float) c);
    }
    setParam (p, ParamIDs::velocity, 90.0f);

    std::unique_ptr<juce::AudioProcessorEditor> edp (p.createEditor());
    auto& ed = *dynamic_cast<RepetitorEditor*> (edp.get());
    ed.resized();
    tick (ed);

    // ---------------------------------------------------------------- Ablauf
    struct Event { double t; std::function<void()> fn; };
    std::vector<Event> ev;
    auto at = [&] (double t, std::function<void()> fn) { ev.push_back ({ t, std::move (fn) }); };
    auto bar = [] (double b) { return b * BAR; };

    juce::Array<juce::var> cursor;
    auto kf = [&] (double t, juce::Point<float> pt, bool pressed = false)
    {
        juce::Array<juce::var> a;
        a.add (t); a.add (pt.x); a.add (pt.y); a.add (pressed);
        cursor.add (a);
    };
    auto centre = [] (juce::Rectangle<int> r) { return r.toFloat().getCentre(); };
    auto click = [&] (double t, juce::Point<float> pt, std::function<void()> fn)
    {
        kf (t - 0.55, pt); kf (t, pt, true); kf (t + 0.18, pt);
        at (t, std::move (fn));
    };
    auto knobDrag = [&] (juce::Slider& knob, const juce::String& id, double t0, double t1, float from, float to, float dy)
    {
        auto kr = in (ed, knob).toFloat();
        const juce::Point<float> kc (kr.getCentreX(), kr.getCentreY() - 4.0f);
        kf (t0 - 0.5, kc); kf (t0, kc, true);
        kf (t1, kc.translated (0.0f, dy), true); kf (t1 + 0.25, kc.translated (0.0f, dy));
        auto* prm = p.apvts.getParameter (id);
        const float n0 = prm->convertTo0to1 (from), n1 = prm->convertTo0to1 (to);
        for (double t = t0; t <= t1 + 1e-6; t += 1.0 / FPS)
        {
            const float v = prm->convertFrom0to1 (n0 + (float) ease ((t - t0) / (t1 - t0)) * (n1 - n0));
            at (t, [&, id, v] { setParam (p, id, v); });
        }
    };

    // Szene 1 (Bar 1-3): zufaellige Noten, NEW klicken
    kf (bar (1.2), { 1000.0f, 330.0f });
    click (bar (2.1), centre (in (ed, ed.newButton)), [&] { p.requestNew(); });

    // Szene 2 (Bar 3-5): Skala waehlen, dann eigene Toene
    {
        auto fs = in (ed, *ed.flavSwitch).toFloat();
        click (bar (3.55), { fs.getX() + fs.getWidth() * 0.5f, fs.getCentreY() }, [&] { setParam (p, ParamIDs::flavor, 1.0f); });   // DORIAN
        auto keys = ed.scaleInfo.withTrimmedTop (23).toFloat();
        auto key = [keys] (int k) { return juce::Point<float> (keys.getX() + keys.getWidth() / 12.0f * ((float) k + 0.5f), keys.getCentreY()); };
        click (bar (4.15), key (2), [&] { p.toggleKey (2); });   // D dazu
        click (bar (4.55), key (6), [&] { p.toggleKey (6); });   // F# weg
    }

    // Szene 3 (Bar 5-7): Length von INF auf 8
    knobDrag (ed.kLength.slider, ParamIDs::length, bar (5.35), bar (6.1), 11.0f, 6.0f, 46.0f);

    // Szene 4 (Bar 7-8.5): Rhythm an, Density
    click (bar (7.25), centre (in (ed, ed.rhythmButton)), [&] { setParam (p, ParamIDs::rhythm, 1.0f); });
    knobDrag (ed.kDensity.slider, ParamIDs::density, bar (7.6), bar (8.15), 0.5f, 0.7f, -30.0f);

    // Szene 5 (Bar 8.5-10): Octave + 5th
    knobDrag (ed.kOctChance.slider, ParamIDs::octChance, bar (8.7), bar (9.15), 0.0f, 0.35f, -40.0f);
    knobDrag (ed.kFifth.slider, ParamIDs::fifth, bar (9.35), bar (9.75), 0.0f, 0.25f, -34.0f);

    std::sort (ev.begin(), ev.end(), [] (auto& x, auto& y) { return x.t < y.t; });

    // ---------------------------------------------------------------- Rendern
    const int totalSamples = (int) std::round (TOTAL * SR);
    juce::AudioBuffer<float> audio (2, totalSamples);
    audio.clear();
    AmbientSynth synth;
    size_t evIdx = 0;
    int pos = 0;
    const int numFrames = (int) std::round (TOTAL * FPS);
    const juce::var layout = rects (ed);
    juce::AudioBuffer<float> none (0, 512);
    int noteCount = 0;

    auto renderTo = [&] (int endSample)
    {
        while (pos < endSample)
        {
            while (evIdx < ev.size() && (int) std::round (ev[evIdx].t * SR) <= pos)
                ev[evIdx++].fn();
            int segEnd = endSample;
            if (evIdx < ev.size())
                segEnd = juce::jmin (segEnd, juce::jmax (pos + 1, (int) std::round (ev[evIdx].t * SR)));
            const int n = juce::jmin (512, segEnd - pos);
            ph.timeSec = pos / SR;
            juce::MidiBuffer midi;
            none.setSize (0, n, false, false, true);
            if (pos < (int) (GUI_UNTIL * SR + 0.5 * BAR * SR))   // gegen Ende keine neuen Noten mehr
                p.processBlock (none, midi);
            auto it = midi.begin();
            for (int i = 0; i < n; ++i)
            {
                while (it != midi.end() && (*it).samplePosition <= i)
                {
                    const auto m = (*it).getMessage();
                    if (m.isNoteOn()) { synth.noteOn (m.getChannel(), m.getNoteNumber(), m.getVelocity()); ++noteCount; }
                    else if (m.isNoteOff()) synth.noteOff (m.getChannel(), m.getNoteNumber());
                    ++it;
                }
                float l, r;
                synth.render (l, r);
                audio.setSample (0, pos + i, l);
                audio.setSample (1, pos + i, r);
            }
            pos += n;
        }
    };

    for (int f = 0; f < numFrames; ++f)
    {
        const double t = f / FPS;
        renderTo (juce::jmin (totalSamples, (int) std::round ((f + 1) / FPS * SR)));
        if (t < GUI_UNTIL)
        {
            tick (ed);
            auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
            auto file = outDir.getChildFile ("frames").getChildFile (juce::String (f).paddedLeft ('0', 4) + ".jpg");
            file.deleteFile();
            juce::FileOutputStream os (file);
            juce::JPEGImageFormat jpg;
            jpg.setQuality (0.93f);
            jpg.writeImageToStream (img, os);
        }
        if (f % 150 == 0)
            std::printf ("frame %d / %d\n", f, numFrames);
    }

    // ---------------------------------------------------------------- FX: Ping-Pong-Delay (1/8 punktiert) + Reverb
    {
        const int dly = (int) std::round (0.75 * BEAT * SR);
        std::vector<float> bl ((size_t) dly, 0.0f), br ((size_t) dly, 0.0f);
        int w = 0;
        for (int i = 0; i < totalSamples; ++i)
        {
            const float inL = audio.getSample (0, i), inR = audio.getSample (1, i);
            const float dl = bl[(size_t) w], dr = br[(size_t) w];
            bl[(size_t) w] = (inL + inR) * 0.5f * 0.6f + dr * 0.5f;
            br[(size_t) w] = dl * 0.5f;
            w = (w + 1) % dly;
            audio.setSample (0, i, inL + dl * 0.38f);
            audio.setSample (1, i, inR + dr * 0.38f);
        }
        juce::Reverb rev;
        rev.setSampleRate (SR);
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.92f; rp.damping = 0.55f; rp.wetLevel = 0.42f; rp.dryLevel = 0.72f; rp.width = 1.0f;
        rev.setParameters (rp);
        rev.processStereo (audio.getWritePointer (0), audio.getWritePointer (1), totalSamples);

        // Ein- und Ausblenden, auf -1 dBFS normalisieren
        const int fadeIn = (int) (0.4 * SR), fadeOut = (int) (2.5 * SR);
        for (int ch = 0; ch < 2; ++ch)
        {
            audio.applyGainRamp (ch, 0, fadeIn, 0.0f, 1.0f);
            audio.applyGainRamp (ch, totalSamples - fadeOut, fadeOut, 1.0f, 0.0f);
        }
        const float peak = audio.getMagnitude (0, totalSamples);
        if (peak > 0.0f)
            audio.applyGain (0.89f / peak);
    }

    {
        auto wav = outDir.getChildFile ("audio.wav");
        wav.deleteFile();
        juce::WavAudioFormat fmt;
        std::unique_ptr<juce::AudioFormatWriter> wr (fmt.createWriterFor (new juce::FileOutputStream (wav), SR, 2, 24, {}, 0));
        wr->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("fps", FPS);
    root->setProperty ("bar", BAR);
    root->setProperty ("guiUntil", GUI_UNTIL);
    root->setProperty ("total", TOTAL);
    root->setProperty ("cursor", cursor);
    root->setProperty ("rects", layout);
    outDir.getChildFile ("events.json").replaceWithText (juce::JSON::toString (juce::var (root)));
    std::printf ("done, %d notes\n", noteCount);
    return 0;
}
