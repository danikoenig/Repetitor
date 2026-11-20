Free quad random quantized note generator for macOS (Apple Silicon), VST3 and Audio Unit (MIDI FX). By Dani Koenig.

**Download `Repetitor-Installer.pkg`** and double-click it. macOS blocks it the first time: open System Settings > Privacy & Security and click "Open Anyway". Full guide: see INSTALL.txt in the ZIP or in the repository.

### New in v1.5.0
- **Rate and Gate per channel**: every channel has its own speed (1/32 to 1 bar or MIDI IN) and note length (up to legato)
- **Starts right away**: all channels run on 1/16 as soon as the DAW plays, no MIDI input needed
- **Movable loop**: drag the loop box to another place in the sequence, or drag across the steps to pick your own loop. Any length from 1 to 32 per channel
- **New scales**: MAJOR (Penta, Blues, Bright) and MINOR (Penta, Dorian, Dark), 18 in total
- **Piano keyboard**: clear white and black keys for your own scale
- **Velocity and Humanize** instead of Clock Vel; loops always restart when the DAW starts
- Down/Up starts in the centre (around the root); 5th removed from Vary

### v1.1.0
- **Resizable window**: drag the bottom right corner (60 % to 200 %). Opens at 100 %, like the other plugins
- **NEW** now rolls new notes right away, also when the DAW is stopped
- **INIT** (was RESET): sets every control back to its default
- Header in the same style as One Shooter, Quad Shooter and Drift Station

### v1.0.0
- **Custom scale**: click the keys of the mini keyboard to build your own scale (root always stays in, moves with Root)
- **Vary**: Skip, Octave (down / both / up) and 5th chance, rolled fresh on every trigger
- **Repetitor Instrument** (was "Repetitor Inst") for Ableton Live: instrument with MIDI output. The installer removes the old "Repetitor Inst"
- **Manual** as PDF in English and German

### Features
- 4 channels on MIDI channel 1-4, triggered by incoming notes or by an internal clock synced to the DAW tempo
- Root, Octave, Spread (0-24 semitones), Down/Up, octave offset per channel
- 18 scales on three switches: MAJOR / MINOR, Flavor, Count
- Length: INF = always new notes, or loop the most recent 1-32 notes (global or per channel)
- Rhythm mode with Density, Gate per channel
- Custom scale, Vary (Skip, Octave)
- NEW, INIT, loop restart on DAW start, loops saved with the project

Bitwig: put Repetitor before an instrument. Logic Pro: MIDI FX. Ableton Live: Repetitor Instrument on its own track, other tracks take it as "MIDI From".

Requirements: Apple Silicon Mac (M1 or newer), macOS 11 or newer.
