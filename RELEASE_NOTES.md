Free quad random quantized note generator for macOS (Apple Silicon), VST3 and Audio Unit (MIDI FX). By Dani Koenig.

**Download `Repetitor-Installer.pkg`** and double-click it. macOS blocks it the first time: open System Settings > Privacy & Security and click "Open Anyway". Full guide: see INSTALL.txt in the ZIP or in the repository.

### New in v1.0.0
- **Custom scale**: click the keys of the mini keyboard to build your own scale (root always stays in, moves with Root)
- **Vary**: Skip, Octave (down / both / up) and 5th chance, rolled fresh on every trigger
- **Repetitor Instrument** (was "Repetitor Inst") for Ableton Live: instrument with MIDI output. The installer removes the old "Repetitor Inst"
- **Manual** as PDF in English and German

### Features
- 4 channels on MIDI channel 1-4, triggered by incoming notes or by an internal clock synced to the DAW tempo
- Root, Octave, Spread (0-24 semitones), Down/Up, octave offset per channel
- 27 scales on three switches: MAJ / MIN / SYM, Flavor, Count
- Length: INF = always new notes, or loop the most recent 1-32 notes (global or per channel)
- Rhythm mode with Density, Gate (INPUT or 1/64 to 1/2)
- Custom scale, Vary (Skip, Octave, 5th)
- NEW, RESET, reset on DAW start, loops saved with the project

Bitwig: put Repetitor before an instrument. Logic Pro: MIDI FX. Ableton Live: Repetitor Instrument on its own track, other tracks take it as "MIDI From".

Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer.
