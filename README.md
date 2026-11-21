# Repetitor

**Manual (PDF):** [English](https://github.com/danikoenig/Repetitor/releases/latest/download/Repetitor-Manual-EN.pdf) | [Deutsch](https://github.com/danikoenig/repetitor/releases/latest/download/Repetitor-Manual-DE.pdf)

Free quad random quantized note generator for macOS (Apple Silicon), VST3, CLAP and Audio Unit (MIDI FX, plus an instrument version with MIDI out for Ableton Live).
By Dani Koenig. Inspired by the Noise Engineering Gamut Repetitor Eurorack module, rebuilt as a MIDI effect: MIDI in, MIDI out.

![Repetitor](docs/screenshot.png)

## How it works

Send in notes (or use the internal clock) and turn Length up. Every trigger creates a random note in the chosen scale.
When you hear something you like, turn Length down to loop the most recent notes (1 to 32 steps) and loop away.

## Features

- **4 channels**, each on its own MIDI channel (default 1-4), with its own colour and step display
- **Rate per channel**: own speed synced to the DAW tempo (1/32 to 1 bar, triplets, default 1/16, runs as soon as you press play) or MIDI IN (every incoming note is a trigger)
- **Gate per channel**: note length in % of the step, up to legato
- **Input mode**: ANY NOTE triggers all MIDI IN channels, or C1-D#1 for one note per channel
- **Pitch**: Root, Octave, Spread (0 to 24 semitones), Down/Up (around, above or below the root), Octave offset per channel
- **Velocity** and **Humanize** (random velocity variation)
- **18 scales** on three switches: MAJOR / MINOR, Flavor (Major: Penta, Blues, Bright. Minor: Penta, Dorian, Dark), Count (few, more, all notes)
- **Custom scale**: click the keys of the mini keyboard to add or remove notes. The root always stays in, the scale moves with Root
- **Length**: INF = always new notes; 1, 2, 3, 4, 5, 6, 8, 12, 16, 24, 32 = loop the most recent notes. Colour like the module LED: red = divisible by 2, blue = by 3, violet = both. Global or per channel (any length 1 to 32). Drag the loop box to move it, or drag across the steps to pick your own loop
- **Rhythm mode**: random repeating rhythm instead of pass-through, Density sets how many steps play. The rhythm is part of the loop
- **Vary**: Skip (leave notes out) and Octave (jump down / both / up), each with a chance. Rolled fresh on every trigger, so the loop never sounds static
- **NEW** rolls new notes right away, **INIT** sets every control back to its default (only channel 1 on, channels 2 to 4 off), optional loop restart when the DAW starts
- **Resizable window**: drag the bottom right corner (60 % to 200 %), opens at 100 %
- Scale, Root and Spread changes are applied live to the loop
- Loops are saved with the project

The 18 scales (mode / flavor / count):

| | Few | More | All |
|---|---|---|---|
| MAJOR Penta | Major Triad | Major Pentatonic | Major |
| MAJOR Blues | Dominant 7 | Major Blues | Mixo Blues |
| MAJOR Bright | Major 7 | Lydian Pentatonic | Lydian |
| MINOR Penta | Minor Triad | Minor Pentatonic | Minor |
| MINOR Dorian | Minor 7 | Dorian Pentatonic | Dorian |
| MINOR Dark | Dark Triad | Dark Pentatonic | Phrygian |

## Use

- **Bitwig**: put Repetitor on an instrument track before the instrument.
- **Logic Pro**: MIDI FX slot > Audio Units > Dani Koenig > Repetitor.
- **Ableton Live**: use **Repetitor Instrument** (instrument with MIDI output, installed together with Repetitor):
  1. MIDI track 1: load *Repetitor Instrument*. Put a clip with notes on this track (or set the channels to the internal clock).
  2. MIDI track 2 with your synth: *MIDI From* = track 1, below that choose *Repetitor Instrument*. Set *Monitor* to *In*.
  3. Ableton merges all MIDI channels of a plugin into one, so all four voices arrive on track 2. For one voice per track, switch the other channels off.

## Install

Download the latest release: [Releases](https://github.com/danikoenig/Repetitor/releases).
`Repetitor-Installer.pkg` installs VST3, AU and CLAP. See [INSTALL.txt](INSTALL.txt).

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release --target Repetitor_VST3 Repetitor_AU
```

JUCE 8.0.4 is downloaded automatically. GitHub Actions builds every push on `main`; a tag `v*` or a commit message with `[release]` creates a release.

Local engine test (triggers, scales, loop, rhythm, clock, state):

```
cmake -B build -DREPETITOR_TEST_SRC=tests/EngineTest.cpp
cmake --build build --target RepetitorTest && ./build/RepetitorTest
```
