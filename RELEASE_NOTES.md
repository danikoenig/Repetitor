Free quad random quantized note generator for macOS (Apple Silicon), VST3 and Audio Unit (MIDI FX). By Dani Koenig.

**Download `Repetitor-Installer.pkg`** and double-click it. macOS blocks it the first time: open System Settings > Privacy & Security and click "Open Anyway". Full guide: see INSTALL.txt in the ZIP or in the repository.

### v0.1.0, first version
- 4 channels on MIDI channel 1-4, triggered by incoming notes or by an internal clock synced to the DAW tempo
- Root, Octave, Spread (0-24 semitones), Down/Up, octave offset per channel
- 27 scales on three switches: MAJ / MIN / SYM, Flavor, Count
- Length: INF = always new notes, or loop the most recent 1-32 notes (global or per channel)
- Rhythm mode with Density, Gate (INPUT or 1/64 to 1/2)
- NEW, RESET, reset on DAW start, loops saved with the project

Use it in Bitwig (before an instrument) or in Logic Pro (MIDI FX). Ableton Live does not support MIDI effect plugins.

Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer.
