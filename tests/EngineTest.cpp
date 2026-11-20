// Engine-Test (nur lokal): Trigger, Quantizer, Loop, Rhythm, Clock, State
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <set>

static int fails = 0;
#define CHECK(c, msg) do { if (! (c)) { std::cout << "FAIL: " << msg << "\n"; ++fails; } else std::cout << "ok:   " << msg << "\n"; } while (0)

struct FakePlayHead : juce::AudioPlayHead
{
    double ppq = 0.0, bpm = 120.0;
    bool playing = false;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setIsPlaying (playing);
        p.setPpqPosition (ppq);
        return p;
    }
};

static void setP (RepetitorProcessor& p, const juce::String& id, float plain)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
}

// Ein Block mit Note-Ons an den angegebenen Positionen, liefert gesendete Note-Ons (ch, note)
static std::vector<std::pair<int, int>> run (RepetitorProcessor& p, int n, std::vector<int> trigs, juce::MidiBuffer* all = nullptr)
{
    juce::AudioBuffer<float> buf (0, n);
    juce::MidiBuffer m;
    for (int t : trigs)
    {
        m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), t);
        m.addEvent (juce::MidiMessage::noteOff (1, 60), juce::jmin (n - 1, t + 10));
    }
    p.processBlock (buf, m);
    std::vector<std::pair<int, int>> ons;
    for (const auto meta : m)
        if (meta.getMessage().isNoteOn())
            ons.push_back ({ meta.getMessage().getChannel(), meta.getMessage().getNoteNumber() });
    if (all) *all = m;
    return ons;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    RepetitorProcessor p;
    FakePlayHead ph;
    p.setPlayHead (&ph);
    p.prepareToPlay (48000.0, 512);

    // Default: Minor Pentatonic auf C, Spread 12 nach oben, 4 Kanaele auf CH 1-4
    auto ons = run (p, 512, { 0 });
    CHECK (ons.size() == 4, "ein Trigger -> 4 Noten (normalisiert auf alle Kanaele)");
    std::set<int> chs;
    const std::set<int> pent { 0, 3, 5, 7, 10 };
    bool inScale = true, inRange = true;
    for (auto [c, nn] : ons) { chs.insert (c); inScale &= pent.count ((nn - 60) % 12) > 0; inRange &= nn >= 60 && nn <= 72; }
    CHECK (chs.size() == 4, "Kanaele 1-4");
    CHECK (inScale, "alle Noten in C Moll-Pentatonik");
    CHECK (inRange, "alle Noten zwischen C3 und C4 (Spread 12, Up)");

    // Zufall laeuft bei INF
    std::vector<int> seq;
    for (int i = 0; i < 64; ++i) seq.push_back (run (p, 512, { 0 })[0].second);
    std::set<int> distinct (seq.begin(), seq.end());
    CHECK (distinct.size() >= 4, "INF erzeugt verschiedene Noten");

    // Length 4: die letzten 4 Werte loopen
    setP (p, ParamIDs::length, 3.0f);   // Index 3 = 4 Steps
    std::vector<int> loop;
    for (int i = 0; i < 12; ++i) loop.push_back (run (p, 512, { 0 })[0].second);
    bool repeats = true;
    for (int i = 4; i < 12; ++i) repeats &= loop[(size_t) i] == loop[(size_t) i - 4];
    CHECK (repeats, "Length 4 wiederholt sich alle 4 Schritte");
    std::vector<int> last4 (seq.end() - 4, seq.end());
    CHECK (std::vector<int> (loop.begin(), loop.begin() + 4) == last4, "Loop = die 4 juengsten Werte");

    // Reset
    p.requestReset();
    CHECK (run (p, 512, { 0 })[0].second == loop[0], "Reset startet beim ersten Schritt");

    // Spread 0 -> nur Grundton
    setP (p, ParamIDs::spread, 0.0f);
    ons = run (p, 512, { 0 });
    CHECK (ons[0].second == 60, "Spread 0 -> Grundton C3");
    setP (p, ParamIDs::spread, 12.0f);

    // Down
    setP (p, ParamIDs::downUp, -1.0f);
    bool down = true;
    for (int i = 0; i < 8; ++i) for (auto [c, nn] : run (p, 512, { 0 })) down &= nn <= 60 && nn >= 48;
    CHECK (down, "Down: Noten unter dem Grundton");
    setP (p, ParamIDs::downUp, 1.0f);

    // Kanal-Oktave
    setP (p, ParamIDs::chOct (1), -2.0f);
    ons = run (p, 512, { 0 });
    bool oct = false;
    for (auto [c, nn] : ons) if (c == 2) oct = nn >= 36 && nn <= 48;
    CHECK (oct, "Kanal 2 zwei Oktaven tiefer");
    setP (p, ParamIDs::chOct (1), 0.0f);

    // Rhythm-Modus, Density 0 -> stumm, Density 1 -> alles
    setP (p, ParamIDs::rhythm, 1.0f);
    setP (p, ParamIDs::density, 0.0f);
    CHECK (run (p, 512, { 0 }).empty(), "Rhythm mit Density 0: keine Noten");
    setP (p, ParamIDs::density, 1.0f);
    CHECK (run (p, 512, { 0 }).size() == 4, "Rhythm mit Density 1: alle Noten");
    setP (p, ParamIDs::density, 0.5f);
    std::vector<int> pat;
    for (int i = 0; i < 8; ++i) pat.push_back ((int) run (p, 512, { 0 }).size());
    CHECK (std::equal (pat.begin(), pat.begin() + 4, pat.begin() + 4), "Rhythmus wiederholt sich im Loop");
    setP (p, ParamIDs::rhythm, 0.0f);

    // Note-Offs: jede Note bekommt ein Note-Off
    {
        int onsN = 0, offsN = 0;
        for (int i = 0; i < 20; ++i)
        {
            juce::MidiBuffer m;
            run (p, 4800, { 0 }, &m);
            for (const auto meta : m) { onsN += meta.getMessage().isNoteOn(); offsN += meta.getMessage().isNoteOff(); }
        }
        CHECK (onsN == offsN, "gleich viele Note-Ons und Note-Offs (Gate 1/16)");
    }

    // Clock: Kanal 1 auf 1/16, DAW spielt 1 Takt bei 120 BPM = 16 Trigger
    for (int c = 0; c < numChannels; ++c) setP (p, ParamIDs::chOn (c), c == 0 ? 1.0f : 0.0f);
    setP (p, ParamIDs::chSrc (0), 3.0f);
    ph.playing = true;
    int clockOns = 0;
    const int blk = 480;
    const double sr = 48000.0;
    const int totalSamples = (int) (2.0 * sr);   // 4 Beats bei 120 BPM
    for (int s = 0; s < totalSamples; s += blk)
    {
        ph.ppq = (double) s / sr * 2.0;
        for (auto [c, nn] : run (p, blk, {})) clockOns += c == 1;
    }
    CHECK (clockOns == 16, "Clock 1/16: 16 Noten pro Takt (" + juce::String (clockOns) + ")");

    // State speichern / laden
    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    RepetitorProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    bool same = true;
    for (int c = 0; c < numChannels; ++c)
        for (int i = 0; i < maxSteps; ++i)
            same &= std::abs (p.view[(size_t) c].pitch[(size_t) i].load() - q.view[(size_t) c].pitch[(size_t) i].load()) < 1e-4f;
    CHECK (same, "Loops werden im Projekt gespeichert");
    CHECK ((int) q.apvts.getRawParameterValue (ParamIDs::chSrc (0))->load() == 3, "Parameter werden gespeichert");

    // Eigene Skala: nur Root (D) und A -> nur D und A
    setP (p, ParamIDs::chSrc (0), 0.0f);
    ph.playing = false;
    setP (p, ParamIDs::root, 2.0f);
    p.toggleKey (2);   // Root: bleibt drin
    CHECK (! p.isCustom(), "Klick auf Root aendert nichts");
    for (int k = 0; k < 12; ++k)
        if (k != 2 && (p.currentMask() & (1 << ((k - 2 + 12) % 12))) != 0)
            p.toggleKey (k);
    p.toggleKey (9);   // A dazu
    CHECK (p.isCustom() && p.currentMask() == ((1 << 0) | (1 << 7)), "eigene Skala D + A");
    bool onlyDA = true;
    for (int i = 0; i < 20; ++i) for (auto [c, nn] : run (p, 512, { 0 })) onlyDA &= (nn % 12 == 2 || nn % 12 == 9);
    CHECK (onlyDA, "nur D und A werden gespielt");
    setP (p, ParamIDs::root, 4.0f);
    bool moved = true;
    for (int i = 0; i < 20; ++i) for (auto [c, nn] : run (p, 512, { 0 })) moved &= (nn % 12 == 4 || nn % 12 == 11);
    CHECK (moved, "eigene Skala wandert mit dem Root (E + B)");
    p.usePresetScale();
    CHECK (! p.isCustom(), "Schalter -> zurueck zu den Presets");
    setP (p, ParamIDs::root, 2.0f);
    p.toggleKey (5); p.toggleKey (0);   // F weg, C dazu -> Custom fuer Screenshot

    // Variation: nur D, Oktave UP 100% -> D eine Oktave hoeher; 5th 100% -> A; Skip 100% -> nichts
    {
        for (int k = 0; k < 12; ++k) if (k != 2 && (p.currentMask() & (1 << ((k - 2 + 12) % 12)))) p.toggleKey (k);
        setP (p, ParamIDs::spread, 0.0f);
        for (int c = 0; c < numChannels; ++c) setP (p, ParamIDs::chOn (c), 1.0f);
        CHECK (run (p, 512, { 0 })[0].second == 62, "nur D, Spread 0 -> D3");
        setP (p, ParamIDs::octChance, 1.0f); setP (p, ParamIDs::octDir, 2.0f);
        CHECK (run (p, 512, { 0 })[0].second == 74, "Oktave 100% UP -> D4");
        setP (p, ParamIDs::fifth, 1.0f);
        CHECK (run (p, 512, { 0 })[0].second == 81, "Oktave + Quinte -> A4");
        setP (p, ParamIDs::octChance, 0.0f); setP (p, ParamIDs::fifth, 0.0f);
        setP (p, ParamIDs::skip, 1.0f);
        CHECK (run (p, 512, { 0 }).empty(), "Skip 100% -> keine Noten");
        setP (p, ParamIDs::skip, 0.3f);
        int played = 0;
        for (int i = 0; i < 200; ++i) played += (int) run (p, 512, { 0 }).size();
        CHECK (played > 450 && played < 670, "Skip 30% -> ca. 70% gespielt (" + juce::String (played) + "/800)");
        setP (p, ParamIDs::skip, 0.15f); setP (p, ParamIDs::octChance, 0.3f); setP (p, ParamIDs::fifth, 0.2f);
        setP (p, ParamIDs::spread, 12.0f);
        p.toggleKey (9);   // A dazu, fuer Screenshot
    }

    // NEW ohne Audio-Verarbeitung: Werte aendern sich sofort
    {
        const float before = p.view[0].pitch[5].load();
        p.requestNew();
        CHECK (std::abs (p.view[0].pitch[5].load() - before) > 1e-6f, "NEW wuerfelt sofort neu");
    }

    // INIT: alles auf Grundstellung
    {
        RepetitorProcessor q2;
        setP (q2, ParamIDs::spread, 3.0f); setP (q2, ParamIDs::downUp, -1.0f); setP (q2, ParamIDs::chLen (2), 5.0f);
        setP (q2, ParamIDs::octChance, 0.8f); q2.toggleKey (4);
        q2.initAll();
        bool allDefault = true;
        for (auto* prm : q2.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                allDefault &= std::abs (r->getValue() - r->getDefaultValue()) < 1e-6f;
        CHECK (allDefault && ! q2.isCustom(), "INIT setzt alle Parameter zurueck");
    }

    // GUI erstellen
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        CHECK (ed != nullptr && ed->getWidth() > 0, "Editor laesst sich oeffnen");
        ed->setSize (1800, 1014);
        CHECK (ed->isResizable() && std::abs (p.uiScale.load() - 1.5f) < 0.01f, "Editor skaliert (1.5x)");
        juce::Image big (juce::Image::RGB, ed->getWidth(), ed->getHeight(), true);
        juce::Graphics gb (big);
        ed->paintEntireComponent (gb, false);
        juce::FileOutputStream osb (juce::File ("/tmp/repetitor_big.png"));
        juce::PNGImageFormat().writeImageToStream (big, osb);
        ed->setSize (1200, 676);
        juce::Image img (juce::Image::RGB, ed->getWidth(), ed->getHeight(), true);
        juce::Graphics g (img);
        ed->paintEntireComponent (g, false);
        juce::File ("/tmp/repetitor.png").deleteFile();
        juce::FileOutputStream os (juce::File ("/tmp/repetitor.png"));
        juce::PNGImageFormat().writeImageToStream (img, os);
    }

    std::cout << (fails == 0 ? "ALL OK" : juce::String (fails) + " FAILED") << "\n";
    return fails;
}
