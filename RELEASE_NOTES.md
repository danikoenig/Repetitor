Free quad random quantized note generator for macOS (Apple Silicon), VST3 and Audio Unit (MIDI FX). By Dani Koenig.

**Download `Repetitor-Installer.pkg`** and double-click it. macOS blocks it the first time: open System Settings > Privacy & Security and click "Open Anyway". Full guide: see INSTALL.txt in the ZIP or in the repository.

### New in v0.2.0
- **Repetitor Inst** for Ableton Live: instrument version with MIDI output. Load it on one MIDI track and pick it under "MIDI From" on another track with your synth. Installed together with Repetitor.

### Features
- 4 channels on MIDI channel 1-4, triggered by incoming notes or by an internal clock synced to the DAW tempo
- Root, Octave, Spread (0-24 semitones), Down/Up, octave offset per channel
- 27 scales on three switches: MAJ / MIN / SYM, Flavor, Count
- Length: INF = always new notes, or loop the most recent 1-32 notes (global or per channel)
- Rhythm mode with Density, Gate (INPUT or 1/64 to 1/2)
- NEW, RESET, reset on DAW start, loops saved with the project

Bitwig: put Repetitor before an instrument. Logic Pro: MIDI FX. Ableton Live: Repetitor Inst on its own track, other tracks take it as "MIDI From".

Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer.
