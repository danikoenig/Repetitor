#pragma once

#include <array>
#include <cmath>
#include <vector>

// 27 Skalen wie beim Gamut Repetitor: drei Schalter mit je drei Positionen.
//   Schalter 1 (MAJ / MIN / SYM)  Familie
//   Schalter 2 (FLAVOR)            Art der Skala
//   Schalter 3 (COUNT)             Anzahl Toene: wenige / mittel / alle
// Index = family * 9 + flavor * 3 + count
namespace Scales
{
    // 18 Skalen: MODE (Major / Minor) x FLAVOR (3) x COUNT (wenige / mehr / alle Toene)
    // Index = family * 9 + flavor * 3 + count
    struct Scale
    {
        const char* name;
        std::vector<int> degrees;   // Halbtoene ab Grundton, aufsteigend, immer mit 0
    };

    constexpr int numFamilies = 2;

    inline const std::array<Scale, 18>& all()
    {
        static const std::array<Scale, 18> table {{
            // MAJOR / PENTA
            { "Major Triad",          { 0, 4, 7 } },
            { "Major Pentatonic",     { 0, 2, 4, 7, 9 } },
            { "Major",                { 0, 2, 4, 5, 7, 9, 11 } },
            // MAJOR / BLUES
            { "Dominant 7",           { 0, 4, 7, 10 } },
            { "Major Blues",          { 0, 2, 3, 4, 7, 9 } },
            { "Mixo Blues",           { 0, 2, 3, 4, 5, 7, 9, 10 } },
            // MAJOR / BRIGHT (lydisch)
            { "Major 7",              { 0, 4, 7, 11 } },
            { "Lydian Pentatonic",    { 0, 4, 6, 7, 11 } },
            { "Lydian",               { 0, 2, 4, 6, 7, 9, 11 } },
            // MINOR / PENTA
            { "Minor Triad",          { 0, 3, 7 } },
            { "Minor Pentatonic",     { 0, 3, 5, 7, 10 } },
            { "Minor",                { 0, 2, 3, 5, 7, 8, 10 } },
            // MINOR / DORIAN
            { "Minor 7",              { 0, 3, 7, 10 } },
            { "Dorian Pentatonic",    { 0, 2, 3, 7, 9 } },
            { "Dorian",               { 0, 2, 3, 5, 7, 9, 10 } },
            // MINOR / DARK (phrygisch)
            { "Dark Triad",           { 0, 1, 3, 7 } },
            { "Dark Pentatonic",      { 0, 1, 5, 7, 8 } },
            { "Phrygian",             { 0, 1, 3, 5, 7, 8, 10 } },
        }};
        return table;
    }

    inline int index (int family, int flavor, int count)
    {
        family = family < 0 ? 0 : (family > 1 ? 1 : family);
        return family * 9 + flavor * 3 + count;
    }
    inline const Scale& get (int family, int flavor, int count) { return all()[(size_t) index (family, flavor, count)]; }

    inline const char* familyName (int f) { static const char* n[] { "MAJOR", "MINOR" }; return n[f > 0 ? 1 : 0]; }
    inline const char* flavorName (int family, int flavor)
    {
        static const char* n[2][3] { { "PENTA", "BLUES", "BRIGHT" },
                                     { "PENTA", "DORIAN", "DARK" } };
        return n[family > 0 ? 1 : 0][flavor];
    }
    inline const char* countName (int c) { static const char* n[] { "FEW", "MORE", "ALL" }; return n[c]; }

    // 12-Bit-Maske: Bit d = Halbton d ueber dem Grundton ist in der Skala
    inline int maskOf (const Scale& s)
    {
        int m = 0;
        for (int d : s.degrees) m |= 1 << d;
        return m;
    }

    // Zufallswert v (0..1) -> Halbton-Offset zum Grundton.
    // Fenster: Spread Halbtoene breit, Down/Up (-1..+1) schiebt es unter / ueber den Grundton.
    // Alle Skalentoene im Fenster sind gleich wahrscheinlich.
    inline int quantize (float v, int spread, float downUp, int mask)
    {
        mask |= 1;   // Grundton immer dabei
        const int lo = (int) std::lround ((float) spread * (downUp - 1.0f) * 0.5f);
        const int hi = lo + spread;
        int notes[64];
        int n = 0;
        for (int oct = -3; oct <= 3 && n < 64; ++oct)
            for (int d = 0; d < 12; ++d)
            {
                if ((mask & (1 << d)) == 0)
                    continue;
                const int s = oct * 12 + d;
                if (s >= lo && s <= hi && n < 64)
                    notes[n++] = s;
            }
        if (n == 0)
            return 0;
        int i = (int) (v * (float) n);
        if (i >= n) i = n - 1;
        if (i < 0) i = 0;
        return notes[i];
    }
}
