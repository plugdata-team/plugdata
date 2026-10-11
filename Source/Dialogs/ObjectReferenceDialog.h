#pragma once
/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

// Implemented by whatever hosts the reference content: the reference dialog paints itself on a
// panel surface, the sidebar panel on a sidebar surface. The content looks up its nearest host so
// the same layout reads correctly in both places.
struct ObjectReferenceSurface {
    virtual ~ObjectReferenceSurface() = default;
    virtual Colour getReferenceBackgroundColour() const = 0;
    virtual Colour getReferenceTextColour() const = 0;
};

inline ObjectReferenceSurface const* findObjectReferenceSurface(Component const& context)
{
    for (auto* parent = context.getParentComponent(); parent; parent = parent->getParentComponent()) {
        if (auto const* surface = dynamic_cast<ObjectReferenceSurface const*>(parent))
            return surface;
    }
    return nullptr;
}

inline Colour getReferenceBackgroundColour(Component const& context)
{
    if (auto const* surface = findObjectReferenceSurface(context))
        return surface->getReferenceBackgroundColour();
    return getThemeColours(context).panelBackgroundColour;
}

inline Colour getReferenceTextColour(Component const& context)
{
    if (auto const* surface = findObjectReferenceSurface(context))
        return surface->getReferenceTextColour();
    return getThemeColours(context).panelTextColour;
}

// The scrollable object documentation itself, shared by the reference dialog and the sidebar
// reference panel. Lays itself out for whatever width it gets, down to sidebar widths.
class ObjectReferenceView final : public Component {
public:
    // The sidebar panel is far narrower than the dialog, so it drops the object drawing, tightens
    // the spacing and lists arguments/methods/flags with their description on its own line.
    enum class Layout { Full,
        Compact };

    explicit ObjectReferenceView(pd::Library& objectLibrary, Layout const layout = Layout::Full)
        : library(objectLibrary)
        , content(layout == Layout::Compact)
    {
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&content, false);
        viewport.setScrollBarsShown(true, false, false, false);
    }

    void showObject(String const& name)
    {
        if (name.isEmpty()) {
            clear();
            return;
        }

        auto const& info = library.getObjectInfo(name);
        objectName = name;
        content.setData(name, info);
        viewport.setViewPosition(0, 0);
        resized();
        repaint();
    }

    void clear()
    {
        content.clear();
        objectName = "";
        viewport.setViewPosition(0, 0);
        resized();
        repaint();
    }

    String const& getObjectName() const { return objectName; }

    bool isShowingObject() const { return objectName.isNotEmpty(); }

    void resized() override
    {
        viewport.setBounds(getLocalBounds());

        // Account for the vertical scrollbar so content doesn't get clipped under it.
        auto contentWidth = viewport.getMaximumVisibleWidth();
        content.recalculateLayout(contentWidth);
    }

private:
    static Colour getOriginColour(Component const& context, String const& origin)
    {
        if (origin == "ELSE")
            return Colour::fromRGB(245, 124, 38); // orange
        if (origin == "cyclone")
            return Colour::fromRGB(12, 110, 232); // blue
        if (origin == "vanilla")
            return Colour::fromRGB(36, 143, 95); // green
        if (origin == "Gem")
            return Colour::fromRGB(140, 51, 204); // purple
        if (origin == "heavylib")
            return Colour::fromRGB(217, 38, 105); // pink
        if (origin == "pdlua")
            return Colour::fromRGB(34, 130, 195); // cyan
        if (origin == "plugdata")
            return Colour::fromRGB(184, 145, 20); // gold
        return getReferenceTextColour(context).withAlpha(0.6f);
    }

    struct SectionHeader final : public Component {
        String label;
        explicit SectionHeader(String const& l)
            : label(l)
        {
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            // Keep the label sitting just above the rule, whatever height the layout gave us
            auto const isShort = getHeight() < 28;
            Fonts::drawStyledText(g, label.toUpperCase(),
                getLocalBounds().reduced(0, isShort ? 1 : 4).withTrimmedBottom(isShort ? 5 : 8),
                getReferenceTextColour(*this).withAlpha(0.6f),
                FontStyle::Semibold, 11.0f, Justification::bottomLeft);

            g.setColour(colours.outlineColour);
            g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
        }
    };

    struct PillRow final : public Component {
        struct Pill {
            Rectangle<int> bounds;
            String text;
            Colour fg;
            Colour bg; // alpha=0 means no fill
            Colour stroke;
        };

        String origin;
        StringArray categories;
        SmallArray<Pill> pills;

        void setData(String const& orig, StringArray const& cats)
        {
            origin = orig;
            categories = cats;
        }

        void recalculateLayout(int width)
        {
            auto const& colours = getThemeColours(*this);

            pills.clear();
            constexpr int h = 22;
            constexpr int hGap = 6, vGap = 6;
            int x = 0, y = 0;
            int rowBottom = h;

            auto addPill = [&](String const& txt, Colour fg, Colour bg, Colour stroke) {
                auto font = Fonts::getSemiBoldFont().withHeight(11.0f);
                int textW = Fonts::getStringWidthInt(txt, font);
                int w = textW + 16;
                if (x > 0 && x + w > width) {
                    x = 0;
                    y += h + vGap;
                    rowBottom = y + h;
                }
                pills.add({ Rectangle<int>(x, y, w, h), txt, fg, bg, stroke });
                x += w + hGap;
            };

            if (origin.isNotEmpty()) {
                auto c = getOriginColour(*this, origin);
                addPill(origin.toUpperCase(), c, c.withAlpha(0.10f), c.withAlpha(0.30f));
            }
            for (auto const& cat : categories) {
                if (cat.isEmpty())
                    continue;
                addPill(cat.toUpperCase(),
                    getReferenceTextColour(*this).withAlpha(0.65f),
                    Colour(0, 0, 0).withAlpha(0.0f),
                    colours.outlineColour);
            }

            setSize(width, jmax(h, rowBottom));
        }

        void paint(Graphics& g) override
        {
            for (auto const& pill : pills) {
                if (pill.bg.getAlpha() > 0) {
                    g.setColour(pill.bg);
                    g.fillRoundedRectangle(pill.bounds.toFloat(), 4.0f);
                }
                g.setColour(pill.stroke);
                g.drawRoundedRectangle(pill.bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
                Fonts::drawStyledText(g, pill.text, pill.bounds, pill.fg,
                    FontStyle::Semibold, 11.0f, Justification::centred);
            }
        }
    };

    struct TitleBlock final : public Component {
        String name;
        float maxFontHeight = 38.0f;
        float fontHeight = 38.0f;

        void setTitle(String const& n) { name = n; }

        void recalculateLayout(int width)
        {
            // Shrink the title until it fits, so long object names stay readable in a narrow sidebar
            fontHeight = maxFontHeight;
            while (fontHeight > 16.0f && Fonts::getStringWidthInt(name, Fonts::getSemiBoldFont().withHeight(fontHeight)) > width)
                fontHeight -= 2.0f;

            setSize(width, roundToInt(fontHeight * 1.35f));
        }

        void paint(Graphics& g) override
        {
            g.setFont(Fonts::getSemiBoldFont().withHeight(fontHeight));
            g.setColour(getReferenceTextColour(*this));

            g.drawText(name, getLocalBounds(), Justification::centredLeft);
        }
    };

    struct TaglineBlock final : public Component {
        String text;
        TextLayout layout;

        void setText(String const& t) { text = t; }

        void recalculateLayout(int width)
        {
            if (text.isEmpty()) {
                setSize(width, 0);
                return;
            }
            AttributedString s;
            s.append(text, Fonts::getDefaultFont().withHeight(15.5f), getReferenceTextColour(*this).withAlpha(0.8f));
            layout.createLayout(s, jmin(width, 640));
            setSize(width, jmax(20, static_cast<int>(layout.getHeight())));
        }

        void paint(Graphics& g) override
        {
            if (text.isEmpty())
                return;
            layout.draw(g, getLocalBounds().toFloat());
        }
    };

    struct ObjectPreview final : public Component {
        String name;
        SmallArray<bool> inletSignals;
        SmallArray<bool> outletSignals;
        bool unknownLayout = false;

        void setData(String const& n, SmallArray<bool> ins, SmallArray<bool> outs, bool unknown)
        {
            name = n;
            inletSignals = ins;
            outletSignals = outs;
            unknownLayout = unknown;
        }

        void recalculateLayout(int width)
        {
            setSize(width, 80);
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            // Card background
            auto cardBounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(getReferenceBackgroundColour(*this).brighter(0.025f));
            g.fillRoundedRectangle(cardBounds, 8.0f);
            g.setColour(colours.outlineColour);
            g.drawRoundedRectangle(cardBounds, 8.0f, 1.0f);

            // Dot grid
            g.setColour(getReferenceTextColour(*this).withAlpha(0.10f));
            constexpr int spacing = 22;
            for (int y = spacing; y < getHeight() - 4; y += spacing) {
                for (int x = spacing; x < getWidth() - 4; x += spacing) {
                    g.fillEllipse(static_cast<float>(x) - 1.0f,
                        static_cast<float>(y) - 1.0f, 2.0f, 2.0f);
                }
            }

            if (unknownLayout) {
                auto qBounds = getLocalBounds().withSizeKeepingCentre(64, 64);
                g.setColour(colours.outlineColour);
                g.drawRoundedRectangle(qBounds.toFloat(), 8.0f, 2.0f);
                Fonts::drawText(g, "?", qBounds,
                    getReferenceTextColour(*this).withAlpha(0.8f),
                    36, Justification::centred);
                return;
            }

            constexpr int ioletSize = 9;
            int const ioletWidth = (ioletSize + 4) * std::max<int>(static_cast<int>(inletSignals.size()), static_cast<int>(outletSignals.size()));
            int const textW = Fonts::getStringWidthInt(name, 15);
            int const objW = std::min(std::max(ioletWidth, textW) + 14, getWidth() - 20);
            int const objH = 22;

            auto centre = getLocalBounds().toFloat().getCentre();
            Rectangle<float> objRect(centre.x - objW * 0.5f,
                centre.y - objH * 0.5f,
                static_cast<float>(objW),
                static_cast<float>(objH));

            // Object box
            g.setColour(colours.textObjectBackgroundColour);
            g.fillRoundedRectangle(objRect, getPlugDataLook(*this).getObjectCornerRadius());
            g.setColour(colours.objectOutlineColour);
            g.drawRoundedRectangle(objRect, getPlugDataLook(*this).getObjectCornerRadius(), 1.0f);

            Fonts::drawText(g, name, objRect.toNearestInt(), colours.canvasTextColour, 15, Justification::centred);

            // Iolets
            auto themeTree = SettingsFile::getInstance()->getCurrentTheme();
            bool squareIolets = static_cast<bool>(themeTree->getProperty("square_iolets"));

            auto drawIolets = [&](SmallArray<bool> const& ports, bool isInletRow) {
                auto const& colours = getThemeColours(*this);

                int const total = static_cast<int>(ports.size());
                if (total == 0)
                    return;

                float const y = isInletRow ? (objRect.getY() - ioletSize * 0.5f)
                                           : (objRect.getBottom() - ioletSize * 0.5f);
                auto ioletStrip = objRect.reduced(8.0f, 0.0f);

                for (int i = 0; i < total; i++) {
                    float x;
                    if (total == 1) {
                        x = objW < 40 ? ioletStrip.getCentreX() - ioletSize * 0.5f
                                      : ioletStrip.getX();
                    } else {
                        float ratio = (ioletStrip.getWidth() - ioletSize) / static_cast<float>(total - 1);
                        x = ioletStrip.getX() + ratio * i;
                    }
                    Rectangle<float> bb(x, y, static_cast<float>(ioletSize), static_cast<float>(ioletSize));
                    Colour fill = ports[i] ? colours.signalColour : colours.dataColour;

                    g.setColour(colours.objectOutlineColour);
                    if (squareIolets) {
                        g.drawRect(bb, 1.0f);
                        g.setColour(fill);
                        g.fillRect(bb.reduced(0.5f));
                    } else {
                        g.drawEllipse(bb, 1.0f);
                        g.setColour(fill);
                        g.fillEllipse(bb.reduced(0.5f));
                    }
                }
            };

            g.saveState();
            g.reduceClipRegion(objRect.getSmallestIntegerContainer());
            drawIolets(inletSignals, true);
            drawIolets(outletSignals, false);
            g.restoreState();
        }
    };

    struct IoletCard final : public Component {
        int number;
        bool isInlet;
        bool repeats;
        bool compact;
        String tooltip;
        TextLayout layout;
        int contentH = 24;

        IoletCard(int n, bool inlet, bool repeat, bool isCompact, String const& t)
            : number(n)
            , isInlet(inlet)
            , repeats(repeat)
            , compact(isCompact)
            , tooltip(t)
        {
        }

        int getTitleHeight() const { return compact ? 26 : 36; }

        void recalculateLayout(int width)
        {
            // Reuse the original heuristic: lines containing "(type) ..." render
            // the type as bold inline. This preserves how the existing JSON
            // tooltips read.
            AttributedString str;
            auto lines = StringArray::fromLines(tooltip);
            auto bodyFont = Fonts::getDefaultFont().withHeight(13.5f);
            auto typeFont = Fonts::getSemiBoldFont().withHeight(13.0f);
            auto bodyCol = getReferenceTextColour(*this).withAlpha(0.85f);
            auto typeCol = getReferenceTextColour(*this);

            for (auto const& line : lines) {
                if (line.contains("(") && line.contains(")")) {
                    auto type = line.fromFirstOccurrenceOf("(", false, false)
                                    .upToFirstOccurrenceOf(")", false, false);
                    auto rest = line.fromFirstOccurrenceOf(")", false, false);
                    str.append(type + ":", typeFont, typeCol);
                    str.append(rest + "\n", bodyFont, bodyCol);
                } else {
                    str.append(line + "\n", bodyFont, bodyCol);
                }
            }

            layout.createLayout(str, jmax(60, width - (compact ? 22 : 28)));
            contentH = jmax(20, static_cast<int>(layout.getHeight()));
            setSize(width, getTitleHeight() - (compact ? 4 : 6) + contentH + (compact ? 8 : 12));
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(getReferenceBackgroundColour(*this).brighter(0.02f));
            g.fillRoundedRectangle(bounds, 6.0f);
            g.setColour(colours.outlineColour);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

            auto titleBounds = getLocalBounds().removeFromTop(getTitleHeight()).reduced(compact ? 10 : 12, compact ? 7 : 12);

            auto const badgeSize = compact ? 16 : 18;
            auto badgeBounds = titleBounds.removeFromLeft(badgeSize).withSizeKeepingCentre(badgeSize, badgeSize);
            g.setColour(getReferenceTextColour(*this));
            g.fillRoundedRectangle(badgeBounds.toFloat(), 3.0f);
            Fonts::drawStyledText(g, String(number), badgeBounds,
                getReferenceBackgroundColour(*this),
                FontStyle::Bold, 10.5f, Justification::centred);

            titleBounds.removeFromLeft(8);

            auto label = (isInlet ? "Inlet " : "Outlet ") + String(number);
            if (repeats)
                label += "  (repeats)";

            Fonts::drawStyledText(g, label.toUpperCase(), titleBounds,
                getReferenceTextColour(*this).withAlpha(0.6f),
                FontStyle::Semibold, 10.5f, Justification::centredLeft);

            // Tooltip body
            auto bodyBounds = getLocalBounds().withTrimmedTop(getTitleHeight() - (compact ? 4 : 8)).reduced(compact ? 11 : 14, compact ? 3 : 4);
            layout.draw(g, bodyBounds.toFloat());
        }
    };

    struct IoletsColumn final : public Component {
        bool isInlet;
        bool compact;
        OwnedArray<IoletCard> cards;

        IoletsColumn(bool inlet, bool isCompact)
            : isInlet(inlet)
            , compact(isCompact)
        {
        }

        void setPorts(SmallArray<std::pair<String, bool>> const& ports)
        {
            // ports: { tooltip, repeats }
            cards.clear();
            for (int i = 0; i < static_cast<int>(ports.size()); i++) {
                auto* c = cards.add(new IoletCard(i + 1, isInlet, ports[i].second, compact, ports[i].first));
                addAndMakeVisible(*c);
            }
        }

        void recalculateLayout(int width)
        {
            int const gap = compact ? 5 : 8;
            int y = 0;
            for (auto* c : cards) {
                c->recalculateLayout(width);
                c->setTopLeftPosition(0, y);
                y += c->getHeight() + gap;
            }
            int total = cards.isEmpty() ? (compact ? 20 : 28) : (y - gap);
            setSize(width, total);
        }

        void paint(Graphics& g) override
        {
            if (cards.isEmpty()) {
                Fonts::drawText(g, "None.", getLocalBounds(),
                    getReferenceTextColour(*this).withAlpha(0.5f),
                    13, Justification::topLeft);
            }
        }
    };

    struct DataTable final : public Component {
        struct Column {
            String header;
            int width;
            bool mono;
        };

        SmallArray<Column> columns;
        SmallArray<StringArray> rows;
        SmallArray<int> rowHeights;
        SmallArray<int> columnWidths;
        SmallArray<SmallArray<TextLayout>> cellLayouts;

        // Stacked mode puts each row's description on its own line below the message/flag/argument
        // it belongs to. Two narrow columns are unreadable at sidebar width.
        bool stacked = false;
        SmallArray<TextLayout> termLayouts;
        SmallArray<TextLayout> descriptionLayouts;

        static constexpr int rowMinH = 36;
        static constexpr int headerH = 32;
        static constexpr int cellPadX = 14;
        static constexpr int cellPadY = 10;
        static constexpr int stackedPadX = 10;
        static constexpr int stackedPadY = 7;
        static constexpr int stackedGap = 3;

        // The last column is flexible; the fixed ones shrink together when there isn't room for them,
        // so the table still fits inside a sidebar-width panel.
        void calculateColumnWidths(int width)
        {
            columnWidths.clear();
            if (columns.empty())
                return;

            int totalFixed = 0;
            for (int i = 0; i < static_cast<int>(columns.size()) - 1; i++)
                totalFixed += columns[i].width;

            int const maxFixed = jmax(60, roundToInt(width * 0.55f));
            float const scale = totalFixed > maxFixed ? maxFixed / static_cast<float>(totalFixed) : 1.0f;

            int fixedUsed = 0;
            for (int i = 0; i < static_cast<int>(columns.size()) - 1; i++) {
                int const colW = jmax(30, roundToInt(columns[i].width * scale));
                columnWidths.add(colW);
                fixedUsed += colW;
            }
            columnWidths.add(jmax(60, width - fixedUsed));
        }

        // Everything but the last column identifies the entry; the last column describes it
        static String getTermForRow(StringArray const& row)
        {
            StringArray terms;
            for (int i = 0; i < row.size() - 1; i++) {
                if (row[i].isNotEmpty() && row[i] != String::fromUTF8("\xe2\x80\x94"))
                    terms.add(row[i]);
            }
            return terms.joinIntoString("  ");
        }

        void recalculateLayoutStacked(int width)
        {
            termLayouts.clear();
            descriptionLayouts.clear();
            rowHeights.clear();

            auto const textWidth = jmax(40, width - stackedPadX * 2);

            int total = 0;
            for (auto const& row : rows) {
                AttributedString term;
                term.append(getTermForRow(row), Fonts::getMonospaceFont().withHeight(13.0f), getReferenceTextColour(*this));
                TextLayout termLayout;
                termLayout.createLayout(term, textWidth);

                AttributedString description;
                description.append(row.isEmpty() ? String() : row[row.size() - 1],
                    Fonts::getDefaultFont().withHeight(13.0f), getReferenceTextColour(*this).withAlpha(0.75f));
                TextLayout descriptionLayout;
                descriptionLayout.createLayout(description, textWidth);

                int const rowH = stackedPadY * 2 + roundToInt(termLayout.getHeight()) + stackedGap + roundToInt(descriptionLayout.getHeight());
                rowHeights.add(rowH);
                termLayouts.add(termLayout);
                descriptionLayouts.add(descriptionLayout);
                total += rowH;
            }

            setSize(width, jmax(1, total));
        }

        void paintStacked(Graphics& g)
        {
            auto const& colours = getThemeColours(*this);

            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(getReferenceBackgroundColour(*this).brighter(0.02f));
            g.fillRoundedRectangle(bounds, 6.0f);

            if (rowHeights.size() == rows.size()) {
                auto const textWidth = static_cast<float>(jmax(40, getWidth() - stackedPadX * 2));

                int y = 0;
                for (int r = 0; r < static_cast<int>(rows.size()); r++) {
                    auto const rowH = rowHeights[r];
                    auto const termH = termLayouts[r].getHeight();

                    termLayouts[r].draw(g, Rectangle<float>(static_cast<float>(stackedPadX),
                        static_cast<float>(y + stackedPadY), textWidth, termH));
                    descriptionLayouts[r].draw(g, Rectangle<float>(static_cast<float>(stackedPadX),
                        static_cast<float>(y + stackedPadY) + termH + stackedGap, textWidth,
                        descriptionLayouts[r].getHeight()));

                    if (r < static_cast<int>(rows.size()) - 1) {
                        g.setColour(colours.outlineColour.withAlpha(0.5f));
                        g.drawHorizontalLine(y + rowH, 0.0f, static_cast<float>(getWidth()));
                    }
                    y += rowH;
                }
            }

            g.setColour(colours.outlineColour);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }

        void recalculateLayout(int width)
        {
            if (stacked) {
                recalculateLayoutStacked(width);
                return;
            }

            cellLayouts.clear();
            rowHeights.clear();
            calculateColumnWidths(width);

            for (auto const& row : rows) {
                int rowH = rowMinH;
                SmallArray<TextLayout> rowLayouts;
                for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                    int colW = columnWidths[i];
                    String text = i < row.size() ? row[i] : "";

                    Font f = columns[i].mono
                        ? Fonts::getMonospaceFont().withHeight(13.5f)
                        : Fonts::getDefaultFont().withHeight(13.5f);

                    AttributedString s;
                    s.append(text, f, columns[i].mono ? getReferenceTextColour(*this) : getReferenceTextColour(*this).withAlpha(0.9f));

                    TextLayout l;
                    l.createLayout(s, jmax(40, colW - cellPadX * 2));
                    rowH = jmax(rowH, static_cast<int>(l.getHeight()) + cellPadY * 2);
                    rowLayouts.add(l);
                }
                rowHeights.add(rowH);
                cellLayouts.add(rowLayouts);
            }

            int total = headerH;
            for (auto h : rowHeights)
                total += h;
            setSize(width, total);
        }

        void paint(Graphics& g) override
        {
            if (stacked) {
                paintStacked(g);
                return;
            }

            auto const& colours = getThemeColours(*this);

            // Card surface
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(getReferenceBackgroundColour(*this).brighter(0.02f));
            g.fillRoundedRectangle(bounds, 6.0f);

            if (columnWidths.size() != columns.size())
                return;

            // Header background
            g.setColour(getReferenceTextColour(*this).withAlpha(0.04f));
            g.fillRect(0, 0, getWidth(), headerH);

            // Header texts
            int x = 0;
            for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                int colW = columnWidths[i];
                Rectangle<int> headerCell(x + cellPadX, 0, colW - cellPadX, headerH);
                Fonts::drawStyledText(g, columns[i].header.toUpperCase(),
                    headerCell, getReferenceTextColour(*this).withAlpha(0.55f),
                    FontStyle::Semibold, 10.5f, Justification::centredLeft);
                x += colW;
            }

            // Data rows
            int y = headerH;
            g.setColour(colours.outlineColour);
            g.drawHorizontalLine(headerH, 0.0f, static_cast<float>(getWidth()));

            for (int r = 0; r < static_cast<int>(rows.size()); r++) {
                int rh = rowHeights[r];
                x = 0;
                for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                    int colW = columnWidths[i];
                    Rectangle<float> cell(static_cast<float>(x + cellPadX),
                        static_cast<float>(y + cellPadY),
                        static_cast<float>(colW - cellPadX * 2),
                        static_cast<float>(rh - cellPadY * 2));
                    cellLayouts[r][i].draw(g, cell);
                    x += colW;
                }
                if (r < static_cast<int>(rows.size()) - 1) {
                    g.setColour(colours.outlineColour.withAlpha(0.5f));
                    g.drawHorizontalLine(y + rh, 0.0f, static_cast<float>(getWidth()));
                }
                y += rh;
            }

            // Outer outline drawn last so it sits cleanly on top
            g.setColour(colours.outlineColour);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }
    };

    struct ContentComponent final : public Component {
        bool compact;

        PillRow pillRow;
        TitleBlock titleBlock;
        TaglineBlock taglineBlock;
        ObjectPreview objectPreview;

        SectionHeader inletsHeader { "Inlets" };
        SectionHeader outletsHeader { "Outlets" };
        IoletsColumn inletsColumn;
        IoletsColumn outletsColumn;

        SectionHeader argumentsHeader { "Arguments" };
        SectionHeader methodsHeader { "Methods" };
        SectionHeader flagsHeader { "Flags" };

        DataTable argumentsTable;
        DataTable methodsTable;
        DataTable flagsTable;

        bool hasContent : 1 = false;
        bool hasArguments : 1 = false;
        bool hasMethods : 1 = false;
        bool hasFlags : 1 = false;

        explicit ContentComponent(bool const isCompact)
            : compact(isCompact)
            , inletsColumn(true, isCompact)
            , outletsColumn(false, isCompact)
        {
            titleBlock.maxFontHeight = compact ? 26.0f : 38.0f;

            argumentsTable.stacked = compact;
            methodsTable.stacked = compact;
            flagsTable.stacked = compact;

            addAndMakeVisible(pillRow);
            addAndMakeVisible(titleBlock);
            addAndMakeVisible(taglineBlock);
            // The object drawing is a poor fit for a narrow sidebar, so the compact layout skips it
            if (!compact)
                addAndMakeVisible(objectPreview);
            addAndMakeVisible(inletsHeader);
            addAndMakeVisible(outletsHeader);
            addAndMakeVisible(inletsColumn);
            addAndMakeVisible(outletsColumn);
            addChildComponent(argumentsHeader);
            addChildComponent(methodsHeader);
            addChildComponent(flagsHeader);
            addChildComponent(argumentsTable);
            addChildComponent(methodsTable);
            addChildComponent(flagsTable);

            updateSectionVisibility();
        }

        // recalculateLayout does nothing without content, so the sections have to be hidden
        // explicitly: otherwise they keep painting their last object at their last bounds.
        void updateSectionVisibility()
        {
            pillRow.setVisible(hasContent);
            titleBlock.setVisible(hasContent);
            taglineBlock.setVisible(hasContent);
            objectPreview.setVisible(hasContent && !compact);
            inletsHeader.setVisible(hasContent);
            outletsHeader.setVisible(hasContent);
            inletsColumn.setVisible(hasContent);
            outletsColumn.setVisible(hasContent);
            argumentsHeader.setVisible(hasContent && hasArguments);
            argumentsTable.setVisible(hasContent && hasArguments);
            methodsHeader.setVisible(hasContent && hasMethods);
            methodsTable.setVisible(hasContent && hasMethods);
            flagsHeader.setVisible(hasContent && hasFlags);
            flagsTable.setVisible(hasContent && hasFlags);
        }

        void clear()
        {
            pillRow.setData("", { });
            titleBlock.setTitle("");
            taglineBlock.setText("");
            inletsColumn.setPorts({ });
            outletsColumn.setPorts({ });
            hasContent = hasArguments = hasMethods = hasFlags = false;
            updateSectionVisibility();
            repaint();
        }

        void setData(String const& name, pd::Library::ObjectReferenceTable const& info)
        {
            StringArray cats;
            for (auto const& c : info.categories)
                cats.add(c);
            pillRow.setData(info.origin.isNotEmpty() ? info.origin : "Unknown", cats);

            titleBlock.setTitle(name);
            taglineBlock.setText(info.body.isNotEmpty() ? info.body : info.description);

            SmallArray<bool> inletsSig, outletsSig;
            bool unknownLayout = false;
            for (auto const& il : info.inlets) {
                if (il.repeating)
                    unknownLayout = true;
                inletsSig.add(il.tooltip.upToFirstOccurrenceOf(":", false, false).contains("signal"));
            }
            for (auto const& ol : info.outlets) {
                if (ol.repeating)
                    unknownLayout = true;
                outletsSig.add(ol.tooltip.upToFirstOccurrenceOf(":", false, false).contains("signal"));
            }
            objectPreview.setData(name, inletsSig, outletsSig, unknownLayout);

            SmallArray<std::pair<String, bool>> inletPorts;
            for (auto const& il : info.inlets)
                inletPorts.add({ il.tooltip, il.repeating });
            inletsColumn.setPorts(inletPorts);

            SmallArray<std::pair<String, bool>> outletPorts;
            for (auto const& ol : info.outlets)
                outletPorts.add({ ol.tooltip, ol.repeating });
            outletsColumn.setPorts(outletPorts);

            argumentsTable.columns.clear();
            argumentsTable.columns.add({ "#", 48, true });
            argumentsTable.columns.add({ "Type", 110, true });
            argumentsTable.columns.add({ "Description", 120, false });
            argumentsTable.rows.clear();
            for (int i = 0; i < static_cast<int>(info.arguments.size()); i++) {
                auto const& a = info.arguments[i];
                argumentsTable.rows.add(StringArray {
                    String(i + 1),
                    a.type.isNotEmpty() ? a.type : String("—"),
                    a.description });
            }
            hasArguments = info.arguments.size() > 0;

            methodsTable.columns.clear();
            methodsTable.columns.add({ "Message", 200, true });
            methodsTable.columns.add({ "Description", 120, false });
            methodsTable.rows.clear();
            for (auto const& m : info.methods)
                methodsTable.rows.add(StringArray { m.type, m.description });
            hasMethods = info.methods.size() > 0;

            flagsTable.columns.clear();
            flagsTable.columns.add({ "Flag", 200, true });
            flagsTable.columns.add({ "Description", 120, false });
            flagsTable.rows.clear();
            for (auto const& f : info.flags) {
                String fname = f.type;
                if (!fname.startsWith("-"))
                    fname = "- " + fname;
                flagsTable.rows.add(StringArray { fname, f.description });
            }
            hasFlags = info.flags.size() > 0;

            hasContent = true;
            updateSectionVisibility();
            repaint();
        }

        void recalculateLayout(int outerWidth)
        {
            int const padX = compact ? 10 : 32;
            int const padTop = compact ? 10 : 16;
            int const padBottom = compact ? 16 : 48;
            int const sectionGap = compact ? 12 : 28;
            int const headerGap = compact ? 4 : 10;
            int const sectionHeaderH = compact ? 24 : 30;

            if (!hasContent) {
                setSize(outerWidth, 200);
                return;
            }

            int contentW = jmax(120, outerWidth - padX * 2);
            int x = padX;
            int y = padTop;

            // Pill row
            pillRow.recalculateLayout(contentW);
            pillRow.setTopLeftPosition(x, y);
            y += pillRow.getHeight() + (compact ? 6 : 14);

            // Title
            titleBlock.recalculateLayout(contentW);
            titleBlock.setTopLeftPosition(x, y);
            y += titleBlock.getHeight() + (compact ? 2 : 4);

            // Tagline
            taglineBlock.recalculateLayout(contentW);
            taglineBlock.setTopLeftPosition(x, y);
            y += taglineBlock.getHeight();

            y += sectionGap;

            // Viz card, only in the full layout
            if (!compact) {
                objectPreview.recalculateLayout(contentW);
                objectPreview.setTopLeftPosition(x, y);
                y += objectPreview.getHeight();

                y += sectionGap;
            }

            // Inlets / Outlets — two columns when wide, stacked when narrow
            bool twoCol = contentW >= 720;
            if (twoCol) {
                int colGap = 24;
                int colW = (contentW - colGap) / 2;

                inletsHeader.setBounds(x, y, colW, sectionHeaderH);
                outletsHeader.setBounds(x + colW + colGap, y, colW, sectionHeaderH);
                int colsY = y + sectionHeaderH + headerGap;

                inletsColumn.recalculateLayout(colW);
                inletsColumn.setTopLeftPosition(x, colsY);

                outletsColumn.recalculateLayout(colW);
                outletsColumn.setTopLeftPosition(x + colW + colGap, colsY);

                y = colsY + jmax(inletsColumn.getHeight(), outletsColumn.getHeight());
            } else {
                inletsHeader.setBounds(x, y, contentW, sectionHeaderH);
                y += sectionHeaderH + headerGap;
                inletsColumn.recalculateLayout(contentW);
                inletsColumn.setTopLeftPosition(x, y);
                y += inletsColumn.getHeight() + sectionGap;

                outletsHeader.setBounds(x, y, contentW, sectionHeaderH);
                y += sectionHeaderH + headerGap;
                outletsColumn.recalculateLayout(contentW);
                outletsColumn.setTopLeftPosition(x, y);
                y += outletsColumn.getHeight();
            }

            // Optional full-width tables
            auto laySection = [&](SectionHeader& header, DataTable& table, bool visible) {
                if (!visible)
                    return;
                y += sectionGap;
                header.setBounds(x, y, contentW, sectionHeaderH);
                y += sectionHeaderH + headerGap;
                table.recalculateLayout(contentW);
                table.setTopLeftPosition(x, y);
                y += table.getHeight();
            };

            laySection(argumentsHeader, argumentsTable, hasArguments);
            laySection(methodsHeader, methodsTable, hasMethods);
            laySection(flagsHeader, flagsTable, hasFlags);

            y += padBottom;
            setSize(outerWidth, y);
        }
    };

    pd::Library& library;
    BouncingViewport viewport;
    ContentComponent content;
    String objectName;
};

class ObjectReferenceDialog final : public Component
    , public ObjectReferenceSurface {
public:
    ObjectReferenceDialog(PluginEditor const* editor, bool const showBackButton)
        : referenceView(*editor->pd->objectLibrary)
    {
        if (showBackButton) {
            addAndMakeVisible(backButton);
        }
        backButton.onClick = [this] {
            setVisible(false);
        };

        addAndMakeVisible(referenceView);
    }

    void showObject(String const& name)
    {
        if (name.isEmpty()) {
            referenceView.clear();
            repaint();
            return;
        }

        referenceView.showObject(name);
        setVisible(true);
        resized();
        repaint();
    }

    Colour getReferenceBackgroundColour() const override { return getThemeColours(*this).panelBackgroundColour; }
    Colour getReferenceTextColour() const override { return getThemeColours(*this).panelTextColour; }

    void resized() override
    {
        backButton.setBounds(2, 0, 40, 40);
        referenceView.setBounds(getLocalBounds().withTrimmedTop(40).reduced(1));
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        // Panel background
        g.setColour(colours.panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        // Title bar
        auto titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();
        Path tp;
        tp.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(),
            titlebarBounds.getWidth(), titlebarBounds.getHeight(),
            Corners::windowCornerRadius, Corners::windowCornerRadius,
            true, true, false, false);
        g.setColour(colours.toolbarBackgroundColour);
        g.fillPath(tp);

        g.setColour(colours.toolbarOutlineColour);
        g.drawHorizontalLine(40, 0.0f, static_cast<float>(getWidth()));

        Fonts::drawStyledText(g, "Object Reference", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), colours.panelTextColour, Semibold, 15, Justification::centred);
    }

private:
    ObjectReferenceView referenceView;
    MainToolbarButton backButton = MainToolbarButton(Icons::Back);
};
