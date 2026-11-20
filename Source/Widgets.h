#pragma once

#include "PluginProcessor.h"
#include "Colours.h"

// Segment-Schalter (z.B. MAJ | MIN | SYM), an einen Choice-Parameter gebunden
class ChoiceSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    ChoiceSwitch (juce::RangedAudioParameter& prm, juce::StringArray labelsIn, juce::Colour c)
        : labels (std::move (labelsIn)), colour (c),
          att (prm, [this] (float v) { selected = juce::roundToInt (v); repaint(); })
    {
        att.sendInitialUpdate();
    }

    void setLabels (const juce::StringArray& l)
    {
        if (l != labels) { labels = l; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        using namespace RepColours;
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (inset);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const int n = labels.size();
        const float w = r.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
            const bool on = i == selected;
            if (on)
            {
                g.setColour (colour.withAlpha (0.9f));
                g.fillRoundedRectangle (seg, 4.0f);
            }
            else if (i == hover)
            {
                g.setColour (cardBorder);
                g.fillRoundedRectangle (seg, 4.0f);
            }
            g.setColour (on ? bgBottom : textDim);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawFittedText (labels[i], seg.toNearestInt(), juce::Justification::centred, 1, 0.6f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, labels.size() - 1, (int) ((float) e.x / (float) getWidth() * (float) labels.size()));
        att.setValueAsCompleteGesture ((float) i);
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = (int) ((float) e.x / (float) getWidth() * (float) labels.size());
        if (h != hover) { hover = h; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    juce::StringArray labels;
    juce::Colour colour;
    int selected = 0, hover = -1;
    juce::ParameterAttachment att;
};

// Anzeige der 32 Speicherplaetze eines Kanals: aelteste links, juengste rechts.
// Der Loop (juengste <Length> Werte) ist hell, der gespielte Schritt leuchtet.
class StepView : public juce::Component
{
public:
    StepView (RepetitorProcessor& p, int channel) : proc (p), ch (channel) {}

    void paint (juce::Graphics& g) override
    {
        using namespace RepColours;
        const auto col = slot (ch);
        auto r = getLocalBounds().toFloat();
        g.setColour (inset);
        g.fillRoundedRectangle (r, 6.0f);

        auto& v = proc.view[(size_t) ch];
        const int head = v.head.load();
        const int len = proc.effectiveLength (ch);
        const int playIdx = v.playIndex.load();
        const bool rhythm = proc.apvts.getRawParameterValue (ParamIDs::rhythm)->load() > 0.5f;
        const float density = proc.apvts.getRawParameterValue (ParamIDs::density)->load();
        const bool on = proc.apvts.getRawParameterValue (ParamIDs::chOn (ch))->load() > 0.5f;
        const int spread = (int) proc.apvts.getRawParameterValue (ParamIDs::spread)->load();
        const float du = proc.apvts.getRawParameterValue (ParamIDs::downUp)->load();
        const auto& degrees = proc.currentScale().degrees;
        const int lo = (int) std::lround ((float) spread * (du - 1.0f) * 0.5f);

        auto inner = r.reduced (6.0f, 6.0f);
        const float cw = inner.getWidth() / (float) maxSteps;

        // Loop-Bereich
        if (len > 0)
        {
            auto loopR = inner.withLeft (inner.getRight() - cw * (float) len);
            g.setColour (col.withAlpha (0.08f));
            g.fillRoundedRectangle (loopR.expanded (2.0f, 3.0f), 4.0f);
            g.setColour (col.withAlpha (0.35f));
            g.drawRoundedRectangle (loopR.expanded (2.0f, 3.0f), 4.0f, 1.0f);
        }

        for (int j = 0; j < maxSteps; ++j)
        {
            const int idx = (head + 1 + j) % maxSteps;
            const float pv = v.pitch[(size_t) idx].load();
            const float gv = v.gate[(size_t) idx].load();
            const int off = Scales::quantize (pv, spread, du, degrees);
            const float h = spread > 0 ? (float) (off - lo) / (float) spread : 0.5f;
            const bool inLoop = len == 0 ? (j == maxSteps - 1) : (j >= maxSteps - len);
            const bool fires = ! rhythm || gv < density;

            auto cell = juce::Rectangle<float> (inner.getX() + cw * (float) j, inner.getY(), cw, inner.getHeight()).reduced (1.0f, 0.0f);
            const float barH = juce::jmax (3.0f, cell.getHeight() * (0.12f + 0.88f * h));
            auto bar = cell.withTop (cell.getBottom() - barH);

            float alpha = inLoop ? 0.85f : 0.22f;
            if (! fires) alpha *= 0.3f;
            if (! on) alpha *= 0.4f;
            g.setColour (col.withAlpha (alpha));
            g.fillRoundedRectangle (bar, 1.5f);

            // Notenkopf (Tonhoehe als Strich)
            g.setColour (text.withAlpha (inLoop && fires ? 0.75f : 0.15f));
            g.fillRect (bar.getX(), bar.getY(), bar.getWidth(), 1.5f);

            if (idx == playIdx)
            {
                g.setColour (text.withAlpha (on ? 0.9f : 0.4f));
                g.drawRoundedRectangle (cell.expanded (0.5f, 2.0f), 2.0f, 1.2f);
            }
        }
    }

private:
    RepetitorProcessor& proc;
    int ch;
};
