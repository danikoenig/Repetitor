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

    void setDimmed (bool d) { if (d != dimmed) { dimmed = d; repaint(); } }

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
                g.setColour (dimmed ? colour.withAlpha (0.25f) : colour.withAlpha (0.9f));
                g.fillRoundedRectangle (seg, 4.0f);
            }
            else if (i == hover)
            {
                g.setColour (cardBorder);
                g.fillRoundedRectangle (seg, 4.0f);
            }
            g.setColour (on ? (dimmed ? text.withAlpha (0.7f) : bgBottom) : textDim);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawFittedText (labels[i], seg.toNearestInt(), juce::Justification::centred, 1, 0.6f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = juce::jlimit (0, labels.size() - 1, (int) ((float) e.x / (float) getWidth() * (float) labels.size()));
        att.setValueAsCompleteGesture ((float) i);
        if (onClick) onClick();
    }

    std::function<void()> onClick;
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
    bool dimmed = false;
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
        const int mask = proc.currentMask();
        const int lo = (int) std::lround ((float) spread * (du - 1.0f) * 0.5f);

        auto inner = r.reduced (6.0f, 6.0f);
        const float cw = inner.getWidth() / (float) maxSteps;

        // Loop-Bereich (verschiebbar)
        const int lpos = proc.loopPos (ch);
        const int loopEnd = maxSteps - 1 - lpos;              // Zelle ganz rechts im Loop
        const int loopStart = loopEnd - len + 1;
        if (len > 0)
        {
            auto loopR = juce::Rectangle<float> (inner.getX() + cw * (float) loopStart, inner.getY(), cw * (float) len, inner.getHeight());
            g.setColour (col.withAlpha (0.08f));
            g.fillRoundedRectangle (loopR.expanded (2.0f, 3.0f), 4.0f);
            g.setColour (col.withAlpha (0.35f));
            g.drawRoundedRectangle (loopR.expanded (2.0f, 3.0f), 4.0f, dragMode == 1 || hoverLoop ? 1.8f : 1.0f);
        }
        if (dragMode == 2)
        {
            const int a = juce::jmin (selA, selB), b = juce::jmax (selA, selB);
            auto selR = juce::Rectangle<float> (inner.getX() + cw * (float) a, inner.getY(), cw * (float) (b - a + 1), inner.getHeight());
            g.setColour (text.withAlpha (0.10f));
            g.fillRoundedRectangle (selR.expanded (2.0f, 3.0f), 4.0f);
            g.setColour (text.withAlpha (0.6f));
            g.drawRoundedRectangle (selR.expanded (2.0f, 3.0f), 4.0f, 1.2f);
        }

        for (int j = 0; j < maxSteps; ++j)
        {
            const int idx = (head + 1 + j) % maxSteps;
            const float pv = v.pitch[(size_t) idx].load();
            const float gv = v.gate[(size_t) idx].load();
            const int off = Scales::quantize (pv, spread, du, mask);
            const float h = spread > 0 ? (float) (off - lo) / (float) spread : 0.5f;
            const bool inLoop = len == 0 ? (j == maxSteps - 1) : (j >= loopStart && j <= loopEnd);
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

    // Maus: Loop-Box ziehen = verschieben, daneben ziehen = neuen Bereich waehlen, Doppelklick = zurueck
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int j = cellAt (e.x);
        const int len = proc.effectiveLength (ch);
        const int end = maxSteps - 1 - proc.loopPos (ch);
        if (len > 0 && j >= end - len + 1 && j <= end)
        {
            dragMode = 1;
            grab = j - (end - len + 1);
            begin (ParamIDs::chPos (ch));
        }
        else
        {
            dragMode = 2;
            selA = selB = j;
        }
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int j = cellAt (e.x);
        if (dragMode == 1)
        {
            const int len = proc.effectiveLength (ch);
            if (len <= 0) return;
            const int start = juce::jlimit (0, maxSteps - len, j - grab);
            setValue (ParamIDs::chPos (ch), (float) (maxSteps - len - start));
        }
        else if (dragMode == 2)
            selB = j;
        repaint();
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragMode == 1)
            end (ParamIDs::chPos (ch));
        else if (dragMode == 2)
        {
            const int a = juce::jmin (selA, selB), b = juce::jmax (selA, selB);
            const int len = b - a + 1;
            // eigene Laenge (1..32) und Position fuer diesen Kanal
            begin (ParamIDs::chLen (ch)); setValue (ParamIDs::chLen (ch), (float) len); end (ParamIDs::chLen (ch));
            begin (ParamIDs::chPos (ch)); setValue (ParamIDs::chPos (ch), (float) (maxSteps - 1 - b)); end (ParamIDs::chPos (ch));
        }
        dragMode = 0;
        repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        begin (ParamIDs::chLen (ch)); setValue (ParamIDs::chLen (ch), 0.0f); end (ParamIDs::chLen (ch));
        begin (ParamIDs::chPos (ch)); setValue (ParamIDs::chPos (ch), 0.0f); end (ParamIDs::chPos (ch));
        dragMode = 0;
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int j = cellAt (e.x);
        const int len = proc.effectiveLength (ch);
        const int end = maxSteps - 1 - proc.loopPos (ch);
        const bool over = len > 0 && j >= end - len + 1 && j <= end;
        if (over != hoverLoop) { hoverLoop = over; repaint(); }
        setMouseCursor (over ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::CrosshairCursor);
    }
    void mouseExit (const juce::MouseEvent&) override { hoverLoop = false; repaint(); }

private:
    RepetitorProcessor& proc;
    int ch;
    int dragMode = 0, grab = 0, selA = 0, selB = 0;   // 1 = verschieben, 2 = Bereich waehlen
    bool hoverLoop = false;

    int cellAt (int x) const
    {
        const float inner = (float) getWidth() - 12.0f;
        return juce::jlimit (0, maxSteps - 1, (int) (((float) x - 6.0f) / (inner / (float) maxSteps)));
    }
    void begin (const juce::String& id) { proc.apvts.getParameter (id)->beginChangeGesture(); }
    void end (const juce::String& id)   { proc.apvts.getParameter (id)->endChangeGesture(); }
    void setValue (const juce::String& id, float plain)
    {
        auto* prm = proc.apvts.getParameter (id);
        prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
    }
};
