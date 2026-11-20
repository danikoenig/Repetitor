#include "PluginEditor.h"
#include "Colours.h"

using namespace RepColours;

namespace
{
    constexpr int W = 1200;
    constexpr int H = 676;
    constexpr int margin = 14;
    constexpr int headerH = 50;
    constexpr int globalH = 196;
    constexpr int rowH = 90;
    constexpr int rowGap = 8;

    const juce::Colour pitchCol = accent;       // Orange
    const juce::Colour scaleCol = env;          // Gelb
    const juce::Colour loopCol  = delayCol;     // Tuerkis
    const juce::Colour midiCol  = reverbCol;    // Lavendel
    const juce::Colour varyCol  = clip;         // Rot

    void drawCard (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour c)
    {
        auto f = r.toFloat();
        g.setColour (card);
        g.fillRoundedRectangle (f, 10.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (f.reduced (0.5f), 10.0f, 1.0f);
        if (title.isNotEmpty())
        {
            g.setColour (c);
            g.fillRoundedRectangle ((float) r.getX() + 12.0f, (float) r.getY() + 13.0f, 3.0f, 11.0f, 1.5f);
            g.setColour (text);
            g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
            g.drawText (title, r.getX() + 21, r.getY() + 8, 200, 20, juce::Justification::centredLeft);
        }
    }

    void smallLabel (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (textDim);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (t, r, j);
    }

    // Farbe des Length-Reglers wie die LED am Modul: rot = durch 2 teilbar, blau = durch 3, violett = beides
    juce::Colour lengthColour (int steps)
    {
        if (steps == 0)      return accent2;
        if (steps % 6 == 0)  return slotD;
        if (steps % 2 == 0)  return clip;
        if (steps % 3 == 0)  return slotB;
        return text;
    }
}

//==============================================================================
ChannelRow::ChannelRow (RepetitorProcessor& p, int channel)
    : proc (p), ch (channel), steps (p, channel)
{
    const auto col = slot (ch);
    auto& a = proc.apvts;

    onButton.getProperties().set ("accent", col.toString());
    onButton.setTooltip ("Channel on / off. Off: the loop keeps running, but no notes are sent.");
    addAndMakeVisible (onButton);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, ParamIDs::chOn (ch), onButton);

    srcBox.addItemList (sourceNames, 1);
    srcBox.setTooltip ("MIDI IN: every incoming note is a trigger. Note values: internal clock synced to the DAW tempo.");
    lenBox.addItem ("GLOBAL", 1);
    lenBox.addItemList (lengthNames, 2);
    lenBox.setTooltip ("Loop length for this channel. GLOBAL follows the big Length knob. INF = always new notes.");
    juce::StringArray outs;
    for (int i = 1; i <= 16; ++i) outs.add ("CH " + juce::String (i));
    outBox.addItemList (outs, 1);
    outBox.setTooltip ("MIDI channel this voice is sent on");
    for (auto* b : { &srcBox, &lenBox, &outBox })
    {
        b->getProperties().set ("accent", col.toString());
        addAndMakeVisible (b);
    }
    srcAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, ParamIDs::chSrc (ch), srcBox);
    lenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, ParamIDs::chLen (ch), lenBox);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, ParamIDs::chOut (ch), outBox);

    kOct.setup (*this, a, ParamIDs::chOct (ch), "Oct", col, true);
    kOct.slider.getProperties().set ("bipolar", true);
    kOct.slider.setTooltip ("Octave offset for this channel");

    addAndMakeVisible (steps);
}

void ChannelRow::paint (juce::Graphics& g)
{
    const auto col = slot (ch);
    auto r = getLocalBounds();
    drawCard (g, r, {}, col);

    // Farbstreifen + Nummer
    g.setColour (col);
    g.fillRoundedRectangle ((float) r.getX(), (float) r.getY() + 10.0f, 4.0f, (float) r.getHeight() - 20.0f, 2.0f);
    g.setColour (col.withAlpha (0.16f));
    g.fillRoundedRectangle (badgeArea.toFloat(), 8.0f);
    g.setColour (col);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (juce::String (ch + 1), badgeArea, juce::Justification::centred);

    smallLabel (g, "SOURCE", srcBox.getBounds().withY (12).withHeight (14));
    smallLabel (g, "LENGTH", lenBox.getBounds().withY (12).withHeight (14));
    smallLabel (g, "MIDI OUT", outBox.getBounds().withY (12).withHeight (14));

    // Note
    auto na = noteArea.toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (na, 8.0f);
    if (glow > 0.01f)
    {
        g.setColour (col.withAlpha (0.35f * glow));
        g.fillRoundedRectangle (na, 8.0f);
    }
    g.setColour (col.withAlpha (0.4f + 0.6f * glow));
    g.drawRoundedRectangle (na.reduced (0.5f), 8.0f, 1.2f);

    const int note = proc.view[(size_t) ch].lastNote.load();
    g.setColour (text);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (noteName (note), noteArea.withTrimmedBottom (16), juce::Justification::centred);
    const int len = proc.effectiveLength (ch);
    smallLabel (g, len == 0 ? juce::String ("INF") : "LOOP " + juce::String (len), noteArea.withTrimmedTop (noteArea.getHeight() - 26).withTrimmedBottom (8),
                juce::Justification::centred);
}

void ChannelRow::resized()
{
    auto r = getLocalBounds().reduced (14, 10);
    auto left = r.removeFromLeft (54);
    badgeArea = left.removeFromTop (36).withSizeKeepingCentre (40, 34);
    onButton.setBounds (left.withSizeKeepingCentre (54, 22).translated (0, 4));
    r.removeFromLeft (10);

    auto combo = [&] (juce::ComboBox& b, int w)
    {
        auto c = r.removeFromLeft (w);
        b.setBounds (c.getX(), 30, c.getWidth(), 26);
        r.removeFromLeft (8);
    };
    combo (srcBox, 98);
    combo (lenBox, 86);
    kOct.setBounds (r.removeFromLeft (56).withTrimmedTop (-2), 14);
    r.removeFromLeft (8);
    combo (outBox, 82);

    noteArea = r.removeFromRight (78);
    r.removeFromRight (12);
    steps.setBounds (r);
}

void ChannelRow::refresh()
{
    auto& v = proc.view[(size_t) ch];
    const int f = v.flash.load();
    if (f != shownFlash)
    {
        shownFlash = f;
        if (v.note.load() >= 0) glow = 1.0f;
    }
    else
        glow *= 0.82f;
    steps.repaint();
    repaint (noteArea.expanded (2));
}

//==============================================================================
RepetitorEditor::RepetitorEditor (RepetitorProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    auto& a = processor.apvts;

    newButton.getProperties().set ("primary", true);
    newButton.setTooltip ("Roll new random values for all four loops");
    newButton.onClick = [this] { processor.requestNew(); };
    resetButton.setTooltip ("Restart all loops from their first step on the next trigger");
    resetButton.onClick = [this] { processor.requestReset(); };
    addAndMakeVisible (newButton);
    addAndMakeVisible (resetButton);

    // Pitch
    kRoot.setup   (*this, a, ParamIDs::root,   "Root",    pitchCol);
    kOctave.setup (*this, a, ParamIDs::octave, "Octave",  pitchCol);
    kSpread.setup (*this, a, ParamIDs::spread, "Spread",  pitchCol);
    kDownUp.setup (*this, a, ParamIDs::downUp, "Down/Up", pitchCol);
    kDownUp.slider.getProperties().set ("bipolar", true);
    kRoot.slider.setTooltip ("Root note of the scale");
    kOctave.slider.setTooltip ("Octave of the root note (C3 = MIDI note 60)");
    kSpread.slider.setTooltip ("How far notes can move away from the root: 0 to 24 semitones");
    kDownUp.slider.setTooltip ("Up: from the root upwards. Down: from the root downwards. Centre: around the root.");

    // Scale
    famSwitch   = std::make_unique<ChoiceSwitch> (*a.getParameter (ParamIDs::family), juce::StringArray { "MAJ", "MIN", "SYM" }, scaleCol);
    flavSwitch  = std::make_unique<ChoiceSwitch> (*a.getParameter (ParamIDs::flavor), juce::StringArray { "-", "-", "-" }, scaleCol);
    countSwitch = std::make_unique<ChoiceSwitch> (*a.getParameter (ParamIDs::count),  juce::StringArray { "FEW", "MORE", "ALL" }, scaleCol);
    for (auto* s : { famSwitch.get(), flavSwitch.get(), countSwitch.get() })
        addAndMakeVisible (s);
    for (auto* s : { famSwitch.get(), flavSwitch.get(), countSwitch.get() })
        s->onClick = [this] { processor.usePresetScale(); };
    famSwitch->setTooltip ("Major, minor or symmetric scales");
    flavSwitch->setTooltip ("Style of the scale");
    countSwitch->setTooltip ("Number of notes: few (chord tones), more (pentatonic), all");

    // Loop
    kLength.setup  (*this, a, ParamIDs::length,  "Length",  loopCol);
    kDensity.setup (*this, a, ParamIDs::density, "Density", loopCol);
    kLength.slider.setTooltip ("INF (fully right): always new random notes. Turn it down to loop the most recent notes "
                               "(1 to 32 steps). Colour: red = divisible by 2, blue = by 3, violet = both.");
    kDensity.slider.setTooltip ("Rhythm mode only: how many steps play a note");
    kLength.slider.onValueChange = [this] { updateLengthColour(); };
    updateLengthColour();

    rhythmButton.getProperties().set ("style", "labelled");
    rhythmButton.getProperties().set ("accent", loopCol.toString());
    rhythmButton.setTooltip ("Off: every trigger plays a note (pass-through). On: random repeating rhythm, set by Density.");
    addAndMakeVisible (rhythmButton);
    rhythmAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, ParamIDs::rhythm, rhythmButton);

    gateBox.addItemList (gateNames, 1);
    gateBox.getProperties().set ("accent", loopCol.toString());
    gateBox.setTooltip ("Note length. INPUT: as long as the incoming note is held (Clock channels use 1/16).");
    addAndMakeVisible (gateBox);
    gateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, ParamIDs::gate, gateBox);

    // Vary
    kSkip.setup      (*this, a, ParamIDs::skip,      "Skip",   varyCol, true);
    kOctChance.setup (*this, a, ParamIDs::octChance, "Octave", varyCol, true);
    kFifth.setup     (*this, a, ParamIDs::fifth,     "5th",    varyCol, true);
    kSkip.slider.setTooltip ("Chance that a note is left out. Rolled fresh on every trigger, also when looping.");
    kOctChance.slider.setTooltip ("Chance that a note jumps one octave (direction below). Rolled fresh on every trigger.");
    kFifth.slider.setTooltip ("Chance that a perfect fifth (+7 semitones) is played instead. Rolled fresh on every trigger.");
    octDirSwitch = std::make_unique<ChoiceSwitch> (*a.getParameter (ParamIDs::octDir), juce::StringArray { "DOWN", "BOTH", "UP" }, varyCol);
    octDirSwitch->setTooltip ("Direction of the octave jumps");
    addAndMakeVisible (*octDirSwitch);

    // MIDI
    inBox.addItemList ({ "ANY NOTE", "C1-D#1" }, 1);
    inBox.getProperties().set ("accent", midiCol.toString());
    inBox.setTooltip ("ANY NOTE: every note triggers all MIDI IN channels. C1-D#1: C1 triggers channel 1, C#1 channel 2, D1 channel 3, D#1 channel 4.");
    addAndMakeVisible (inBox);
    inAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, ParamIDs::inMode, inBox);

    for (auto* b : { &thruButton, &resetPlayButton })
    {
        b->getProperties().set ("style", "labelled");
        b->getProperties().set ("accent", midiCol.toString());
        addAndMakeVisible (b);
    }
    thruButton.setTooltip ("Also pass the incoming notes through");
    resetPlayButton.setTooltip ("Restart all loops when the DAW starts playing");
    thruAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, ParamIDs::thru, thruButton);
    resetPlayAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, ParamIDs::resetPlay, resetPlayButton);

    kVel.setup (*this, a, ParamIDs::velocity, "Clock Vel", midiCol, true);
    kVel.slider.setTooltip ("Velocity for channels running on the internal clock (MIDI IN uses the incoming velocity)");

    for (int c = 0; c < numChannels; ++c)
    {
        rows[(size_t) c] = std::make_unique<ChannelRow> (processor, c);
        addAndMakeVisible (*rows[(size_t) c]);
    }

    timerCallback();
    setSize (W, H);
    startTimerHz (30);
}

RepetitorEditor::~RepetitorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RepetitorEditor::updateLengthColour()
{
    const int idx = juce::jlimit (0, 11, (int) processor.apvts.getRawParameterValue (ParamIDs::length)->load());
    kLength.slider.getProperties().set ("accent", lengthColour (lengthSteps[(size_t) idx]).toString());
    kLength.slider.repaint();
}

void RepetitorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Header
    g.setGradientFill (juce::ColourGradient (accent2, (float) margin, 0.0f, accent, (float) margin + 160.0f, 0.0f, false));
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("REPETITOR", margin + 2, 10, 220, 30, juce::Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (juce::String (REPETITOR_INSTRUMENT ? "INSTRUMENT  /  " : "") + "quad random quantized note generator  /  by Dani Koenig",
                margin + 172, 14, 460, 24, juce::Justification::centredLeft);

    drawCard (g, secPitch, "PITCH", pitchCol);
    drawCard (g, secScale, "SCALE", scaleCol);
    drawCard (g, secLoop,  "LOOP",  loopCol);
    drawCard (g, secVary,  "VARY",  varyCol);
    drawCard (g, secMidi,  "MIDI",  midiCol);
    smallLabel (g, "OCT", octDirLabel);

    // Skalen-Schalter beschriften
    smallLabel (g, "MODE",   famSwitch->getBounds().withX (secScale.getX() + 14).withWidth (60));
    smallLabel (g, "FLAVOR", flavSwitch->getBounds().withX (secScale.getX() + 14).withWidth (60));
    smallLabel (g, "COUNT",  countSwitch->getBounds().withX (secScale.getX() + 14).withWidth (60));
    smallLabel (g, "GATE",   gateLabel);
    smallLabel (g, "INPUT",  inLabel);

    // Skalenname + Mini-Keyboard mit den Skalentoenen
    const int mask = processor.currentMask();
    const bool custom = processor.isCustom();
    const int root = (int) processor.apvts.getRawParameterValue (ParamIDs::root)->load();
    auto info = scaleInfo;
    g.setColour (text);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    auto nameRow = info.removeFromTop (18);
    g.drawText (noteNames[root] + " " + processor.scaleName(), nameRow, juce::Justification::centredLeft);
    smallLabel (g, custom ? "CLICK KEYS / SWITCH = PRESETS" : "CLICK KEYS TO EDIT", nameRow, juce::Justification::centredRight);
    info.removeFromTop (5);

    bool inScale[12] {};
    for (int d = 0; d < 12; ++d) if (mask & (1 << d)) inScale[(root + d) % 12] = true;
    const bool black[12] { false, true, false, true, false, false, true, false, true, false, true, false };
    const float kw = (float) info.getWidth() / 12.0f;
    for (int i = 0; i < 12; ++i)
    {
        auto k = juce::Rectangle<float> ((float) info.getX() + kw * (float) i, (float) info.getY(), kw, (float) info.getHeight()).reduced (1.5f, 0.0f);
        const bool isRoot = i == root;
        g.setColour (inScale[i] ? (isRoot ? accent : scaleCol.withAlpha (black[i] ? 0.65f : 0.9f))
                                : (black[i] ? inset : knobTrack));
        g.fillRoundedRectangle (k, 3.0f);
        if (i == hoverKey && ! isRoot)
        {
            g.setColour (text.withAlpha (0.6f));
            g.drawRoundedRectangle (k, 3.0f, 1.2f);
        }
        g.setColour (inScale[i] ? bgBottom : textDim.withAlpha (0.7f));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText (noteNames[i], k.toNearestInt(), juce::Justification::centredBottom);
    }
}

int RepetitorEditor::keyAt (juce::Point<int> pos) const
{
    auto keys = scaleInfo.withTrimmedTop (23);
    if (! keys.contains (pos))
        return -1;
    return juce::jlimit (0, 11, (int) ((float) (pos.x - keys.getX()) / ((float) keys.getWidth() / 12.0f)));
}

void RepetitorEditor::mouseDown (const juce::MouseEvent& e)
{
    const int k = keyAt (e.getPosition());
    if (k >= 0)
    {
        processor.toggleKey (k);
        repaint (scaleInfo.expanded (4));
    }
}

void RepetitorEditor::mouseMove (const juce::MouseEvent& e)
{
    const int k = keyAt (e.getPosition());
    if (k != hoverKey)
    {
        hoverKey = k;
        setMouseCursor (k >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint (scaleInfo.expanded (4));
    }
}

void RepetitorEditor::mouseExit (const juce::MouseEvent&)
{
    if (hoverKey >= 0) { hoverKey = -1; repaint (scaleInfo.expanded (4)); }
}

void RepetitorEditor::resized()
{
    auto r = getLocalBounds().reduced (margin, 0);
    auto header = r.removeFromTop (headerH);
    newButton.setBounds (header.removeFromRight (84).withSizeKeepingCentre (84, 30));
    header.removeFromRight (8);
    resetButton.setBounds (header.removeFromRight (84).withSizeKeepingCentre (84, 30));

    auto global = r.removeFromTop (globalH);
    secPitch = global.removeFromLeft (282);  global.removeFromLeft (10);
    secScale = global.removeFromLeft (290);  global.removeFromLeft (10);
    secLoop  = global.removeFromLeft (182);  global.removeFromLeft (10);
    secVary  = global.removeFromLeft (190);  global.removeFromLeft (10);
    secMidi  = global;

    // Pitch: 4 Knobs
    {
        auto a = secPitch.reduced (12, 0).withTrimmedTop (40).withTrimmedBottom (26);
        Knob::layoutRow (a, { &kRoot, &kOctave, &kSpread, &kDownUp });
    }
    // Scale: 3 Schalter + Anzeige
    {
        auto a = secScale.reduced (14, 0).withTrimmedTop (36);
        for (auto* s : { famSwitch.get(), flavSwitch.get(), countSwitch.get() })
        {
            s->setBounds (a.removeFromTop (26).withTrimmedLeft (62));
            a.removeFromTop (6);
        }
        a.removeFromTop (4);
        scaleInfo = a.removeFromTop (54);
    }
    // Loop
    {
        auto a = secLoop.reduced (12, 0).withTrimmedTop (36);
        Knob::layoutRow (a.removeFromTop (88), { &kLength, &kDensity });
        a.removeFromTop (4);
        rhythmButton.setBounds (a.removeFromTop (24));
        a.removeFromTop (6);
        auto row = a.removeFromTop (26);
        gateLabel = row.removeFromLeft (38);
        gateBox.setBounds (row);
    }
    // Vary
    {
        auto a = secVary.reduced (12, 0).withTrimmedTop (36);
        Knob::layoutRow (a.removeFromTop (88), { &kSkip, &kOctChance, &kFifth }, 14);
        a.removeFromTop (34);
        auto row = a.removeFromTop (26);
        octDirLabel = row.removeFromLeft (30);
        octDirSwitch->setBounds (row);
    }
    // MIDI
    {
        auto a = secMidi.reduced (12, 0).withTrimmedTop (36);
        auto row = a.removeFromTop (26);
        inLabel = row.removeFromLeft (38);
        inBox.setBounds (row);
        a.removeFromTop (8);
        auto bottom = a.removeFromTop (110);
        auto pills = bottom.removeFromLeft (bottom.getWidth() - 62);
        thruButton.setBounds (pills.removeFromTop (24));
        pills.removeFromTop (8);
        resetPlayButton.setBounds (pills.removeFromTop (24));
        kVel.setBounds (bottom.withTrimmedLeft (6).withHeight (82), 14);
    }

    r.removeFromTop (rowGap + 2);
    for (auto& row : rows)
    {
        row->setBounds (r.removeFromTop (rowH));
        r.removeFromTop (rowGap);
    }
}

void RepetitorEditor::timerCallback()
{
    auto& a = processor.apvts;
    const int fam = (int) a.getRawParameterValue (ParamIDs::family)->load();
    for (auto* s : { famSwitch.get(), flavSwitch.get(), countSwitch.get() })
        s->setDimmed (processor.isCustom());
    flavSwitch->setLabels ({ Scales::flavorName (fam, 0), Scales::flavorName (fam, 1), Scales::flavorName (fam, 2) });

    const int sIdx = processor.isCustom() ? 1000 + processor.currentMask()
                                          : Scales::index (fam, (int) a.getRawParameterValue (ParamIDs::flavor)->load(),
                                                           (int) a.getRawParameterValue (ParamIDs::count)->load());
    const int root = (int) a.getRawParameterValue (ParamIDs::root)->load();
    if (sIdx != shownScale || root != shownRoot)
    {
        shownScale = sIdx;
        shownRoot = root;
        repaint (scaleInfo.expanded (4));
    }
    const int len = (int) a.getRawParameterValue (ParamIDs::length)->load();
    if (len != shownLen)
    {
        shownLen = len;
        updateLengthColour();
    }
    for (auto& row : rows)
        row->refresh();
}
