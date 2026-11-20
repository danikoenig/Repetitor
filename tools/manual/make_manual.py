#!/usr/bin/env python3
"""Erstellt das Repetitor-Manual als PDF (Englisch und Deutsch).

Aufruf:  python3 tools/manual/make_manual.py <version> <screenshot.png> <ausgabeordner>
"""
import sys
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.units import mm
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Table,
                                TableStyle, Image, PageBreak, KeepTogether)

ORANGE = colors.HexColor("#ff7a2f")
AMBER = colors.HexColor("#ffb347")
DARK = colors.HexColor("#17181c")
CARD = colors.HexColor("#f3f3f5")
TEXT = colors.HexColor("#1d1f24")
DIM = colors.HexColor("#6b707a")
SEC = {
    "pitch": colors.HexColor("#ff7a2f"),
    "scale": colors.HexColor("#e0b000"),
    "loop": colors.HexColor("#1fb3c8"),
    "vary": colors.HexColor("#ff4d5e"),
    "midi": colors.HexColor("#7f8bd0"),
    "ch": colors.HexColor("#4fc3f7"),
}

H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=17, leading=21, textColor=TEXT, spaceBefore=6, spaceAfter=8)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=12.5, leading=16, textColor=TEXT, spaceBefore=10, spaceAfter=5)
BODY = ParagraphStyle("body", fontName="Helvetica", fontSize=10, leading=14.2, textColor=TEXT, spaceAfter=5)
SMALL = ParagraphStyle("small", parent=BODY, fontSize=9, leading=12.5, textColor=DIM)
CELL = ParagraphStyle("cell", parent=BODY, fontSize=9.2, leading=12.4, spaceAfter=0)
CELLB = ParagraphStyle("cellb", parent=CELL, fontName="Helvetica-Bold")
BULLET = ParagraphStyle("bullet", parent=BODY, leftIndent=12, bulletIndent=2, spaceAfter=3)

SCALES = [
    ("MAJOR", "Penta",  "Major Triad",   "Major Pentatonic",  "Major"),
    ("MAJOR", "Blues",  "Dominant 7",    "Major Blues",       "Mixo Blues"),
    ("MAJOR", "Bright", "Major 7",       "Lydian Pentatonic", "Lydian"),
    ("MINOR", "Penta",  "Minor Triad",   "Minor Pentatonic",  "Minor"),
    ("MINOR", "Dorian", "Minor 7",       "Dorian Pentatonic", "Dorian"),
    ("MINOR", "Dark",   "Dark Triad",    "Dark Pentatonic",   "Phrygian"),
]

T = {
"en": {
 "subtitle": "Quad random quantized note generator",
 "kind": "MIDI effect for macOS (Apple Silicon)  ·  VST3 and Audio Unit",
 "by": "by Dani Koenig",
 "intro": ("Repetitor generates random notes that always fit a scale, on four channels at once. "
           "Press play: every channel runs on its own speed (RATE) and every step creates a new note. Or trigger it with your own MIDI notes. "
           "When you hear something you like, turn Length down: the most recent notes (1 to 32 steps) loop. "
           "MIDI goes in, MIDI comes out. Repetitor makes no sound itself, it plays your synth."),
 "inspired": "Inspired by the Noise Engineering Gamut Repetitor Eurorack module, rebuilt as a MIDI plugin. Not affiliated with Noise Engineering.",
 "versions": "Two versions",
 "versions_rows": [
   ("Repetitor", "MIDI effect. For Bitwig, Logic Pro (MIDI FX), Reaper, Cubase and other DAWs that accept MIDI effects."),
   ("Repetitor Instrument", "Instrument with MIDI output, for Ableton Live. Makes no sound, it only sends MIDI to other tracks."),
 ],
 "quick": "Quick start",
 "daws": [
   ("Bitwig", "Put <b>Repetitor</b> on an instrument track before the synth and press play. For MIDI triggers set RATE to MIDI IN."),
   ("Logic Pro", "Instrument track: MIDI FX slot &gt; Audio Units &gt; Dani Koenig &gt; Repetitor."),
   ("Ableton Live", "1. MIDI track 1: load <b>Repetitor Instrument</b> (it runs on 1/16 as soon as you press play).<br/>"
                    "2. MIDI track 2 with your synth: <i>MIDI From</i> = track 1, below that <i>Repetitor Instrument</i>. Monitor = <i>In</i>.<br/>"
                    "Ableton merges all MIDI channels of a plugin, so all four voices arrive on track 2. For one voice per track, switch the other channels off."),
 ],
 "first": ("First patch: load Repetitor before a synth and press play: all four channels run on 1/16 right away. "
           "Pick a scale, set Spread. When a phrase sounds good, turn Length to 8 or 16. Press <b>NEW</b> for new notes."),
 "overview": "The window",
 "controls": "Controls",
 "secs": [
  ("pitch", "PITCH", [
    ("Root", "Root note of the scale (C to B)."),
    ("Octave", "Octave of the root. C3 = MIDI note 60 (as in Bitwig and Ableton)."),
    ("Spread", "How far notes can move away from the root: 0 to 24 semitones. 0 = only the root."),
    ("Down/Up", "Where the range sits. Centre (default): around the root. UP: from the root upwards. DOWN: from the root downwards."),
  ]),
  ("scale", "SCALE", [
    ("MODE", "Major or minor."),
    ("FLAVOR", "Major: PENTA, BLUES, BRIGHT (lydian). Minor: PENTA, DORIAN, DARK (phrygian)."),
    ("COUNT", "Number of notes: FEW (chord tones), MORE (pentatonic), ALL (full scale)."),
    ("Keyboard", "Shows the notes of the scale, the root in orange. <b>Click a key</b> to add or remove a note: this makes your own CUSTOM scale. "
                 "The root always stays in. Your scale moves with Root. Click one of the three switches to go back to the presets."),
  ]),
  ("loop", "LOOP", [
    ("Length", "INF (fully right): always new notes. 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32: loop the most recent notes. "
               "Colour like the module LED: red = divisible by 2, blue = by 3, violet = both."),
    ("Density", "Only in Rhythm mode: how many steps play a note."),
    ("RHYTHM", "Off: every trigger plays a note. On: random repeating rhythm. The rhythm is part of the loop."),
  ]),
  ("vary", "VARY", [
    ("Skip", "Chance that a note is left out."),
    ("Octave", "Chance that a note jumps one octave. OCT switch: DOWN, BOTH or UP."),
    ("", "VARY is rolled fresh on every trigger, also when looping. The loop stays, but it never sounds static."),
  ]),
  ("midi", "MIDI", [
    ("INPUT", "ANY NOTE: every incoming note triggers all MIDI IN channels. C1-D#1: C1 triggers channel 1, C#1 channel 2, D1 channel 3, D#1 channel 4."),
    ("THRU", "Also pass the incoming notes through."),
    ("Velocity", "Velocity of the notes. MIDI IN channels use the velocity of the incoming note."),
    ("Humanize", "Random velocity variation per note (up to ±40), so it sounds less mechanical."),
  ]),
  ("ch", "CHANNELS 1-4", [
    ("ON", "Channel on / off. Off: the loop keeps running, but no notes are sent."),
    ("RATE", "Speed of the channel, synced to the DAW tempo: 1/32, 1/16T, 1/16 (default), 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 BAR. Runs as soon as the DAW plays. Or MIDI IN: every incoming note is a trigger."),
    ("LENGTH", "GLOBAL follows the big Length knob, or any length from 1 to 32 per channel, or INF."),
    ("GATE", "Note length in % of the step. Fully right = legato. With MIDI IN the step is the time between the incoming notes."),
    ("Oct", "Octave offset for this channel (-3 to +3)."),
    ("MIDI OUT", "MIDI channel of this voice (default 1 to 4). Set all to the same channel to play one synth."),
    ("Display", "The 32 stored notes: oldest left, newest right. The bright box is the loop, the frame shows the step that plays. "
                "<b>Drag the box</b> to move the loop. <b>Drag across the steps</b> to pick your own loop (start, end and length). Double-click = back to GLOBAL."),
  ]),
  ("pitch", "HEADER", [
    ("NEW", "Roll new random notes for all four loops, right away (also when the DAW is stopped)."),
    ("INIT", "Set every control back to its default (scale, spread, length, vary, channels ...). Your notes stay, press NEW for new ones."),
    ("Size", "Drag the bottom right corner to make the window bigger or smaller (60 % to 200 %). It always opens at 100 %."),
  ]),
 ],
 "scales": "The 18 scales",
 "scales_head": ("Mode", "Flavor", "Few", "More", "All"),
 "recipes": "Recipes",
 "recipe_rows": [
   ("Bassline", "Channel 1 on 1/16, Oct -1 or -2, Minor Pentatonic, Spread 7 to 12, RHYTHM on, Density about 60 %, Length 8 or 16. "
                "Add Octave 20 % and Skip 10 % for movement. Switch off channels 2 to 4."),
   ("Melody and counter-melody", "Channel 1 on 1/8, channel 2 on 1/4 with Oct +1, both on the same synth or on two synths. Different lengths per channel (e.g. 6 and 8) make the phrases shift against each other."),
   ("One note, a lot of groove", "Custom scale with only one note (click all other keys off). Octave 40 % BOTH, Skip 20 %."),
   ("Chords", "All four channels on the same RATE, MIDI OUT all on one channel, COUNT = FEW. Every step plays a random chord."),
 ],
 "tips": "Good to know",
 "tip_rows": [
   "Root, Scale and Spread are applied live: change them and the loop follows.",
   "Your loops, loop positions and own scale are saved with the project.",
   "The channels run while the DAW plays (RATE on a note value).",
   "All parameters can be automated in the DAW.",
 ],
 "install": "Install",
 "install_text": ("Download <b>Repetitor-Installer.pkg</b> from github.com/danikoenig/Repetitor/releases and double-click it. "
                  "macOS blocks it the first time: System Settings &gt; Privacy &amp; Security &gt; <i>Open Anyway</i>. "
                  "The installer puts Repetitor and Repetitor Instrument into the VST3 and Components folders. "
                  "Restart the DAW and rescan the plugins. To remove everything, use Repetitor-Uninstaller.pkg. "
                  "Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer."),
 "page": "Page",
},
"de": {
 "subtitle": "Quad Random Quantized Note Generator",
 "kind": "MIDI-Effekt für macOS (Apple Silicon)  ·  VST3 und Audio Unit",
 "by": "von Dani Koenig",
 "intro": ("Repetitor erzeugt zufällige Noten, die immer in eine Skala passen, auf vier Kanälen gleichzeitig. "
           "Drück Play: Jeder Kanal läuft in seinem eigenen Tempo (RATE), und jeder Schritt erzeugt eine neue Note. Oder du triggerst ihn mit eigenen MIDI-Noten. "
           "Gefällt dir, was du hörst, drehst du Length runter: Die letzten Noten (1 bis 32 Schritte) loopen. "
           "MIDI rein, MIDI raus. Repetitor macht selbst keinen Ton, er spielt deinen Synth."),
 "inspired": "Inspiriert vom Eurorack-Modul Gamut Repetitor von Noise Engineering, nachgebaut als MIDI-Plugin. Keine Verbindung zu Noise Engineering.",
 "versions": "Zwei Versionen",
 "versions_rows": [
   ("Repetitor", "MIDI-Effekt. Für Bitwig, Logic Pro (MIDI-FX), Reaper, Cubase und andere DAWs mit MIDI-Effekten."),
   ("Repetitor Instrument", "Instrument mit MIDI-Ausgang, für Ableton Live. Macht keinen Ton, schickt nur MIDI an andere Spuren."),
 ],
 "quick": "Schnellstart",
 "daws": [
   ("Bitwig", "<b>Repetitor</b> auf einer Instrumentenspur vor den Synth legen und Play drücken. Für MIDI-Trigger RATE auf MIDI IN stellen."),
   ("Logic Pro", "Instrumentenspur: MIDI-FX-Slot &gt; Audio Units &gt; Dani Koenig &gt; Repetitor."),
   ("Ableton Live", "1. MIDI-Spur 1: <b>Repetitor Instrument</b> laden (läuft in 1/16, sobald du Play drückst).<br/>"
                    "2. MIDI-Spur 2 mit deinem Synth: <i>MIDI From</i> = Spur 1, darunter <i>Repetitor Instrument</i>. Monitor = <i>In</i>.<br/>"
                    "Ableton legt alle MIDI-Kanäle eines Plugins zusammen, alle vier Stimmen kommen also auf Spur 2 an. Für eine Stimme pro Spur die anderen Kanäle ausschalten."),
 ],
 "first": ("Erster Patch: Repetitor vor einen Synth legen und Play drücken: Alle vier Kanäle laufen sofort in 1/16. "
           "Skala wählen, Spread einstellen. Klingt eine Phrase gut, Length auf 8 oder 16 drehen. <b>NEW</b> würfelt neue Noten."),
 "overview": "Das Fenster",
 "controls": "Bedienelemente",
 "secs": [
  ("pitch", "PITCH", [
    ("Root", "Grundton der Skala (C bis B)."),
    ("Octave", "Oktave des Grundtons. C3 = MIDI-Note 60 (wie in Bitwig und Ableton)."),
    ("Spread", "Wie weit sich die Noten vom Grundton entfernen dürfen: 0 bis 24 Halbtöne. 0 = nur der Grundton."),
    ("Down/Up", "Wo der Bereich liegt. Mitte (Grundstellung): um den Grundton herum. UP: ab dem Grundton nach oben. DOWN: ab dem Grundton nach unten."),
  ]),
  ("scale", "SCALE", [
    ("MODE", "Dur (MAJOR) oder Moll (MINOR)."),
    ("FLAVOR", "Major: PENTA, BLUES, BRIGHT (lydisch). Minor: PENTA, DORIAN, DARK (phrygisch)."),
    ("COUNT", "Anzahl Töne: FEW (Akkordtöne), MORE (Pentatonik), ALL (ganze Skala)."),
    ("Tastatur", "Zeigt die Töne der Skala, den Grundton in Orange. <b>Klick auf eine Taste</b> fügt einen Ton hinzu oder nimmt ihn raus: So entsteht deine eigene Skala (CUSTOM). "
                 "Der Grundton bleibt immer drin. Deine Skala wandert mit Root. Klick auf einen der drei Schalter = zurück zu den Presets."),
  ]),
  ("loop", "LOOP", [
    ("Length", "INF (ganz rechts): immer neue Noten. 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32: die letzten Noten loopen. "
               "Farbe wie die LED am Modul: rot = durch 2 teilbar, blau = durch 3, violett = beides."),
    ("Density", "Nur im Rhythm-Modus: wie viele Schritte eine Note spielen."),
    ("RHYTHM", "Aus: Jeder Trigger spielt eine Note. An: zufälliger, sich wiederholender Rhythmus. Der Rhythmus ist Teil des Loops."),
  ]),
  ("vary", "VARY", [
    ("Skip", "Wie oft eine Note ausgelassen wird."),
    ("Octave", "Wie oft eine Note eine Oktave springt. Schalter OCT: DOWN, BOTH oder UP."),
    ("", "VARY wird bei jedem Trigger neu gewürfelt, auch im Loop. Der Loop bleibt, klingt aber nie statisch."),
  ]),
  ("midi", "MIDI", [
    ("INPUT", "ANY NOTE: Jede Eingangsnote triggert alle MIDI-IN-Kanäle. C1-D#1: C1 triggert Kanal 1, C#1 Kanal 2, D1 Kanal 3, D#1 Kanal 4."),
    ("THRU", "Die Eingangsnoten zusätzlich durchlassen."),
    ("Velocity", "Velocity der Noten. MIDI-IN-Kanäle nehmen die Velocity der Eingangsnote."),
    ("Humanize", "Zufällige Velocity-Schwankung pro Note (bis ±40), damit es weniger mechanisch klingt."),
  ]),
  ("ch", "KANÄLE 1-4", [
    ("ON", "Kanal an / aus. Aus: Der Loop läuft weiter, es werden aber keine Noten gesendet."),
    ("RATE", "Tempo des Kanals, synchron zur DAW: 1/32, 1/16T, 1/16 (Grundstellung), 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 BAR. Läuft los, sobald die DAW spielt. Oder MIDI IN: Jede Eingangsnote ist ein Trigger."),
    ("LENGTH", "GLOBAL folgt dem grossen Length-Regler, oder jede Länge von 1 bis 32 pro Kanal, oder INF."),
    ("GATE", "Notenlänge in % des Schritts. Ganz rechts = legato. Bei MIDI IN ist der Schritt der Abstand zwischen den Eingangsnoten."),
    ("Oct", "Oktav-Versatz für diesen Kanal (-3 bis +3)."),
    ("MIDI OUT", "MIDI-Kanal dieser Stimme (Standard 1 bis 4). Alle auf denselben Kanal = ein Synth spielt alles."),
    ("Anzeige", "Die 32 gespeicherten Noten: älteste links, neueste rechts. Die helle Box ist der Loop, der Rahmen zeigt den Schritt, der gerade spielt. "
                "<b>Box ziehen</b> = Loop verschieben. <b>Über die Schritte ziehen</b> = eigenen Loop wählen (Start, Ende, Länge). Doppelklick = zurück zu GLOBAL."),
  ]),
  ("pitch", "KOPFZEILE", [
    ("NEW", "Sofort neue Zufallsnoten für alle vier Loops würfeln (auch wenn die DAW steht)."),
    ("INIT", "Alle Regler auf Grundstellung (Skala, Spread, Length, Vary, Kanäle ...). Deine Noten bleiben, NEW würfelt neue."),
    ("Grösse", "Unten rechts an der Ecke ziehen: Fenster grösser oder kleiner (60 % bis 200 %). Es öffnet immer in 100 %."),
  ]),
 ],
 "scales": "Die 18 Skalen",
 "scales_head": ("Mode", "Flavor", "Few", "More", "All"),
 "recipes": "Rezepte",
 "recipe_rows": [
   ("Bassline", "Kanal 1 auf 1/16, Oct -1 oder -2, Minor Pentatonic, Spread 7 bis 12, RHYTHM an, Density um 60 %, Length 8 oder 16. "
                "Mit Octave 20 % und Skip 10 % kommt Bewegung rein. Kanäle 2 bis 4 ausschalten."),
   ("Melodie und Gegenmelodie", "Kanal 1 auf 1/8, Kanal 2 auf 1/4 mit Oct +1, beide auf denselben oder auf zwei Synths. Unterschiedliche Längen pro Kanal (z.B. 6 und 8) verschieben die Phrasen gegeneinander."),
   ("Eine Note, viel Groove", "Eigene Skala mit nur einem Ton (alle anderen Tasten abwählen). Octave 40 % BOTH, Skip 20 %."),
   ("Akkorde", "Alle vier Kanäle auf derselben RATE, MIDI OUT alle auf einen Kanal, COUNT = FEW. Jeder Schritt spielt einen zufälligen Akkord."),
 ],
 "tips": "Gut zu wissen",
 "tip_rows": [
   "Root, Skala und Spread wirken sofort: Änderst du sie, folgt der Loop.",
   "Deine Loops, Loop-Positionen und deine eigene Skala werden mit dem Projekt gespeichert.",
   "Die Kanäle laufen, solange die DAW spielt (RATE auf einem Notenwert).",
   "Alle Parameter lassen sich in der DAW automatisieren.",
 ],
 "install": "Installation",
 "install_text": ("Lade <b>Repetitor-Installer.pkg</b> von github.com/danikoenig/Repetitor/releases und doppelklicke es. "
                  "macOS blockiert es beim ersten Mal: Systemeinstellungen &gt; Datenschutz &amp; Sicherheit &gt; <i>Dennoch öffnen</i>. "
                  "Der Installer legt Repetitor und Repetitor Instrument in die Ordner VST3 und Components. "
                  "DAW neu starten und Plugins neu scannen. Entfernen: Repetitor-Uninstaller.pkg. "
                  "Voraussetzungen: Mac mit Apple Silicon (M1 oder neuer), macOS 11 oder neuer."),
 "page": "Seite",
},
}


def build(lang, version, shot, out):
    t = T[lang]
    W, H = A4
    M = 18 * mm

    def first_page(c, d):
        c.saveState()
        c.setFillColor(DARK)
        c.rect(0, H - 62 * mm, W, 62 * mm, stroke=0, fill=1)
        c.setFillColor(ORANGE)
        c.setFont("Helvetica-Bold", 34)
        c.drawString(M, H - 30 * mm, "REPETITOR")
        c.setFillColor(colors.white)
        c.setFont("Helvetica", 13)
        c.drawString(M, H - 39 * mm, t["subtitle"])
        c.setFillColor(colors.HexColor("#9a9fa8"))
        c.setFont("Helvetica", 9.5)
        c.drawString(M, H - 46 * mm, t["kind"])
        c.drawString(M, H - 51.5 * mm, t["by"] + "  ·  v" + version + "  ·  github.com/danikoenig/Repetitor")
        c.setFillColor(AMBER)
        c.rect(0, H - 62 * mm, W, 1.2 * mm, stroke=0, fill=1)
        footer(c, d)
        c.restoreState()

    def later_page(c, d):
        c.saveState()
        c.setFillColor(DARK)
        c.rect(0, H - 13 * mm, W, 13 * mm, stroke=0, fill=1)
        c.setFillColor(ORANGE)
        c.setFont("Helvetica-Bold", 11)
        c.drawString(M, H - 8.5 * mm, "REPETITOR")
        c.setFillColor(colors.HexColor("#9a9fa8"))
        c.setFont("Helvetica", 8.5)
        c.drawRightString(W - M, H - 8.5 * mm, "Manual v" + version)
        footer(c, d)
        c.restoreState()

    def footer(c, d):
        c.setFillColor(DIM)
        c.setFont("Helvetica", 8)
        c.drawString(M, 10 * mm, "Repetitor v" + version + "  ·  Dani Koenig")
        c.drawRightString(W - M, 10 * mm, "%s %d" % (t["page"], d.page))

    doc = BaseDocTemplate(out, pagesize=A4, leftMargin=M, rightMargin=M, topMargin=20 * mm, bottomMargin=18 * mm,
                          title="Repetitor Manual", author="Dani Koenig")
    fw = W - 2 * M
    doc.addPageTemplates([
        PageTemplate("first", [Frame(M, 18 * mm, fw, H - 62 * mm - 26 * mm, id="f1")], onPage=first_page),
        PageTemplate("later", [Frame(M, 18 * mm, fw, H - 13 * mm - 26 * mm, id="f2")], onPage=later_page),
    ])

    s = []
    from reportlab.platypus.doctemplate import NextPageTemplate
    s.append(NextPageTemplate("later"))
    s.append(Paragraph(t["intro"], BODY))
    s.append(Paragraph(t["inspired"], SMALL))
    s.append(Spacer(1, 4))

    img = Image(shot)
    ratio = img.imageHeight / float(img.imageWidth)
    img.drawWidth = fw
    img.drawHeight = fw * ratio
    s.append(img)
    s.append(Spacer(1, 6))

    s.append(Paragraph(t["versions"], H2))
    s.append(kv_table(t["versions_rows"], fw, ORANGE))

    s.append(Paragraph(t["quick"], H2))
    s.append(kv_table(t["daws"], fw, ORANGE))
    s.append(Spacer(1, 4))
    s.append(Paragraph(t["first"], BODY))

    s.append(Spacer(1, 6))
    s.append(Paragraph(t["controls"], H1))
    for key, title, rows in t["secs"]:
        s.append(KeepTogether([section_title(title, SEC[key], fw), Spacer(1, 3), kv_table(rows, fw, SEC[key]), Spacer(1, 8)]))

    s.append(PageBreak())
    s.append(Paragraph(t["scales"], H1))
    head = [Paragraph(h, CELLB) for h in t["scales_head"]]
    data = [head] + [[Paragraph(c, CELLB if i < 2 else CELL) for i, c in enumerate(r)] for r in SCALES]
    tb = Table(data, colWidths=[16 * mm, 24 * mm, (fw - 40 * mm) / 3] + [(fw - 40 * mm) / 3] * 2)
    style = [
        ("BACKGROUND", (0, 0), (-1, 0), SEC["scale"]),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
        ("LINEBELOW", (0, 0), (-1, -1), 0.4, colors.HexColor("#dddde2")),
    ]
    for r in (4, 5, 6):
        style.append(("BACKGROUND", (0, r), (-1, r), CARD))
    style = [x for x in style if not (x[0] == "BACKGROUND" and x[1][1] == 6)]
    tb.setStyle(TableStyle(style))
    s.append(tb)
    s.append(Spacer(1, 4))
    s.append(Paragraph("FEW / MORE / ALL = COUNT.  CUSTOM: " + ("click keys on the keyboard." if lang == "en" else "Tasten auf der Tastatur anklicken."), SMALL))

    s.append(Paragraph(t["recipes"], H2))
    s.append(kv_table(t["recipe_rows"], fw, SEC["loop"]))
    s.append(Paragraph(t["tips"], H2))
    for r in t["tip_rows"]:
        s.append(Paragraph(r, BULLET, bulletText="•"))
    s.append(Paragraph(t["install"], H2))
    s.append(Paragraph(t["install_text"], BODY))

    doc.build(s)


def section_title(title, col, fw):
    tb = Table([[Paragraph('<font color="white"><b>%s</b></font>' % title, CELL)]], colWidths=[fw])
    tb.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), col),
                            ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
                            ("LEFTPADDING", (0, 0), (-1, -1), 8)]))
    return tb


def kv_table(rows, fw, col):
    data = [[Paragraph(k, CELLB), Paragraph(v, CELL)] for k, v in rows]
    tb = Table(data, colWidths=[38 * mm, fw - 38 * mm])
    style = [("VALIGN", (0, 0), (-1, -1), "TOP"),
             ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
             ("LINEBELOW", (0, 0), (-1, -1), 0.4, colors.HexColor("#dddde2")),
             ("LINEBEFORE", (0, 0), (0, -1), 2, col)]
    tb.setStyle(TableStyle(style))
    return tb


if __name__ == "__main__":
    version, shot, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
    build("en", version, shot, outdir + "/Repetitor-Manual-EN.pdf")
    build("de", version, shot, outdir + "/Repetitor-Manual-DE.pdf")
    print("ok")
