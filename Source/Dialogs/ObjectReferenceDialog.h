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

class ObjectReferenceDialog final : public Component {
public:
    ObjectReferenceDialog(PluginEditor const* editor, bool const showBackButton)
        : library(*editor->pd->objectLibrary)
    {
        setBufferedToImage(true);

        if (showBackButton) {
            addAndMakeVisible(backButton);
        }
        backButton.onClick = [this] {
            setVisible(false);
        };

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&content, false);
        viewport.setScrollBarsShown(true, false, false, false);
    }

    void showObject(String const& name)
    {
        if (name.isEmpty()) {
            content.clear();
            objectName = "";
            repaint();
            return;
        }

        auto const& info = library.getObjectInfo(name);
        objectName = name;
        content.setData(name, info);
        setVisible(true);
        resized();
        repaint();
    }

    void resized() override
    {
        backButton.setBounds(2, 0, 40, 40);

        auto bodyBounds = getLocalBounds().withTrimmedTop(40).reduced(1);
        viewport.setBounds(bodyBounds);

        // Account for the vertical scrollbar so content doesn't get clipped under it.
        auto contentWidth = viewport.getMaximumVisibleWidth();
        content.recalculateLayout(contentWidth);
    }

    void paint(Graphics& g) override
    {
        // Panel background
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        // Title bar
        auto titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();
        Path tp;
        tp.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(),
            titlebarBounds.getWidth(), titlebarBounds.getHeight(),
            Corners::windowCornerRadius, Corners::windowCornerRadius,
            true, true, false, false);
        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(tp);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(40, 0.0f, static_cast<float>(getWidth()));

        Fonts::drawStyledText(g, "Object Reference", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), PlugDataColours::panelTextColour, Semibold, 15, Justification::centred);
    }

private:
    static Colour getOriginColour(String const& origin)
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
        return PlugDataColours::panelTextColour.withAlpha(0.6f);
    }

    struct SectionHeader final : public Component {
        String label;
        explicit SectionHeader(String const& l)
            : label(l)
        {
        }

        void paint(Graphics& g) override
        {
            Fonts::drawStyledText(g, label.toUpperCase(),
                getLocalBounds().reduced(0, 4).withTrimmedBottom(8),
                PlugDataColours::panelTextColour.withAlpha(0.6f),
                FontStyle::Semibold, 11.0f, Justification::bottomLeft);

            g.setColour(PlugDataColours::outlineColour);
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
                auto c = getOriginColour(origin);
                addPill(origin.toUpperCase(), c, c.withAlpha(0.10f), c.withAlpha(0.30f));
            }
            for (auto const& cat : categories) {
                if (cat.isEmpty())
                    continue;
                addPill(cat.toUpperCase(),
                    PlugDataColours::panelTextColour.withAlpha(0.65f),
                    Colour(0, 0, 0).withAlpha(0.0f),
                    PlugDataColours::outlineColour);
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

        void setTitle(String const& n) { name = n; }

        void paint(Graphics& g) override
        {
            g.setFont(Fonts::getSemiBoldFont().withHeight(38.0f));
            g.setColour(PlugDataColours::panelTextColour);

            int w = static_cast<int>(Fonts::getStringWidthInt(name, g.getCurrentFont()));
            g.drawText(name, Rectangle<float>(0, 0.0f, static_cast<float>(w) + 6.0f, getHeight()).toNearestInt(),
                Justification::centredLeft);
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
            s.append(text, Fonts::getDefaultFont().withHeight(15.5f), PlugDataColours::panelTextColour.withAlpha(0.8f));
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
            // Card background
            auto cardBounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(PlugDataColours::panelBackgroundColour.brighter(0.025f));
            g.fillRoundedRectangle(cardBounds, 8.0f);
            g.setColour(PlugDataColours::outlineColour);
            g.drawRoundedRectangle(cardBounds, 8.0f, 1.0f);

            // Dot grid
            g.setColour(PlugDataColours::panelTextColour.withAlpha(0.10f));
            constexpr int spacing = 22;
            for (int y = spacing; y < getHeight() - 4; y += spacing) {
                for (int x = spacing; x < getWidth() - 4; x += spacing) {
                    g.fillEllipse(static_cast<float>(x) - 1.0f,
                        static_cast<float>(y) - 1.0f, 2.0f, 2.0f);
                }
            }

            if (unknownLayout) {
                auto qBounds = getLocalBounds().withSizeKeepingCentre(64, 64);
                g.setColour(PlugDataColours::outlineColour);
                g.drawRoundedRectangle(qBounds.toFloat(), 8.0f, 2.0f);
                Fonts::drawText(g, "?", qBounds,
                    PlugDataColours::panelTextColour.withAlpha(0.8f),
                    36, Justification::centred);
                return;
            }

            constexpr int ioletSize = 9;
            int const ioletWidth = (ioletSize + 4) * std::max<int>(static_cast<int>(inletSignals.size()), static_cast<int>(outletSignals.size()));
            int const textW = Fonts::getStringWidthInt(name, 15);
            int const objW = std::max(ioletWidth, textW) + 14;
            int const objH = 22;

            auto centre = getLocalBounds().toFloat().getCentre();
            Rectangle<float> objRect(centre.x - objW * 0.5f,
                centre.y - objH * 0.5f,
                static_cast<float>(objW),
                static_cast<float>(objH));

            // Object box
            g.setColour(PlugDataColours::textObjectBackgroundColour);
            g.fillRoundedRectangle(objRect, Corners::objectCornerRadius);
            g.setColour(PlugDataColours::objectOutlineColour);
            g.drawRoundedRectangle(objRect, Corners::objectCornerRadius, 1.0f);

            Fonts::drawText(g, name, objRect.toNearestInt(), PlugDataColours::canvasTextColour, 15, Justification::centred);

            // Iolets
            auto themeTree = SettingsFile::getInstance()->getCurrentTheme();
            bool squareIolets = static_cast<bool>(themeTree->getProperty("square_iolets"));

            auto drawIolets = [&](SmallArray<bool> const& ports, bool isInletRow) {
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
                    Colour fill = ports[i] ? PlugDataColours::signalColour : PlugDataColours::dataColour;

                    g.setColour(fill);
                    if (squareIolets) {
                        g.fillRect(bb);
                        g.setColour(PlugDataColours::objectOutlineColour);
                        g.drawRect(bb, 1.0f);
                    } else {
                        g.fillEllipse(bb);
                        g.setColour(PlugDataColours::objectOutlineColour);
                        g.drawEllipse(bb, 1.0f);
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
        String tooltip;
        TextLayout layout;
        int contentH = 24;

        IoletCard(int n, bool inlet, bool repeat, String const& t)
            : number(n)
            , isInlet(inlet)
            , repeats(repeat)
            , tooltip(t)
        {
        }

        void recalculateLayout(int width)
        {
            // Reuse the original heuristic: lines containing "(type) ..." render
            // the type as bold inline. This preserves how the existing JSON
            // tooltips read.
            AttributedString str;
            auto lines = StringArray::fromLines(tooltip);
            auto bodyFont = Fonts::getDefaultFont().withHeight(13.5f);
            auto typeFont = Fonts::getSemiBoldFont().withHeight(13.0f);
            auto bodyCol = PlugDataColours::panelTextColour.withAlpha(0.85f);
            auto typeCol = PlugDataColours::panelTextColour;

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

            layout.createLayout(str, jmax(60, width - 28));
            contentH = jmax(20, static_cast<int>(layout.getHeight()));
            setSize(width, 30 + contentH + 12);
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(PlugDataColours::panelBackgroundColour.brighter(0.02f));
            g.fillRoundedRectangle(bounds, 6.0f);
            g.setColour(PlugDataColours::outlineColour);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

            auto titleBounds = getLocalBounds().removeFromTop(36).reduced(12, 12);

            auto badgeBounds = titleBounds.removeFromLeft(18).withSizeKeepingCentre(18, 18);
            g.setColour(PlugDataColours::panelTextColour);
            g.fillRoundedRectangle(badgeBounds.toFloat(), 3.0f);
            Fonts::drawStyledText(g, String(number), badgeBounds,
                PlugDataColours::panelBackgroundColour,
                FontStyle::Bold, 10.5f, Justification::centred);

            titleBounds.removeFromLeft(8);

            auto label = (isInlet ? "Inlet " : "Outlet ") + String(number);
            if (repeats)
                label += "  (repeats)";

            Fonts::drawStyledText(g, label.toUpperCase(), titleBounds,
                PlugDataColours::panelTextColour.withAlpha(0.6f),
                FontStyle::Semibold, 10.5f, Justification::centredLeft);

            // Tooltip body
            auto bodyBounds = getLocalBounds().withTrimmedTop(28).reduced(14, 4);
            layout.draw(g, bodyBounds.toFloat());
        }
    };

    struct IoletsColumn final : public Component {
        bool isInlet;
        OwnedArray<IoletCard> cards;

        explicit IoletsColumn(bool inlet)
            : isInlet(inlet)
        {
        }

        void setPorts(SmallArray<std::pair<String, bool>> const& ports)
        {
            // ports: { tooltip, repeats }
            cards.clear();
            for (int i = 0; i < static_cast<int>(ports.size()); i++) {
                auto* c = cards.add(new IoletCard(i + 1, isInlet, ports[i].second, ports[i].first));
                addAndMakeVisible(*c);
            }
        }

        void recalculateLayout(int width)
        {
            int y = 0;
            for (auto* c : cards) {
                c->recalculateLayout(width);
                c->setTopLeftPosition(0, y);
                y += c->getHeight() + 8;
            }
            int total = cards.isEmpty() ? 28 : (y - 8);
            setSize(width, total);
        }

        void paint(Graphics& g) override
        {
            if (cards.isEmpty()) {
                Fonts::drawText(g, "None.", getLocalBounds(),
                    PlugDataColours::panelTextColour.withAlpha(0.5f),
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
        SmallArray<SmallArray<TextLayout>> cellLayouts;

        static constexpr int rowMinH = 36;
        static constexpr int headerH = 32;
        static constexpr int cellPadX = 14;
        static constexpr int cellPadY = 10;

        void recalculateLayout(int width)
        {
            cellLayouts.clear();
            rowHeights.clear();

            int totalFixed = 0;
            for (int i = 0; i < static_cast<int>(columns.size()) - 1; i++)
                totalFixed += columns[i].width;
            int flexW = jmax(120, width - totalFixed);

            for (auto const& row : rows) {
                int rowH = rowMinH;
                SmallArray<TextLayout> rowLayouts;
                for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                    bool isLast = (i == static_cast<int>(columns.size()) - 1);
                    int colW = isLast ? flexW : columns[i].width;
                    String text = i < row.size() ? row[i] : "";

                    Font f = columns[i].mono
                        ? Fonts::getMonospaceFont().withHeight(13.5f)
                        : Fonts::getDefaultFont().withHeight(13.5f);

                    AttributedString s;
                    s.append(text, f, columns[i].mono ? PlugDataColours::panelTextColour : PlugDataColours::panelTextColour.withAlpha(0.9f));

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
            // Card surface
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(PlugDataColours::panelBackgroundColour.brighter(0.02f));
            g.fillRoundedRectangle(bounds, 6.0f);

            int totalFixed = 0;
            for (int i = 0; i < static_cast<int>(columns.size()) - 1; i++)
                totalFixed += columns[i].width;
            int flexW = jmax(120, getWidth() - totalFixed);

            // Header background
            g.setColour(PlugDataColours::panelTextColour.withAlpha(0.04f));
            g.fillRect(0, 0, getWidth(), headerH);

            // Header texts
            int x = 0;
            for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                bool isLast = (i == static_cast<int>(columns.size()) - 1);
                int colW = isLast ? flexW : columns[i].width;
                Rectangle<int> headerCell(x + cellPadX, 0, colW - cellPadX, headerH);
                Fonts::drawStyledText(g, columns[i].header.toUpperCase(),
                    headerCell, PlugDataColours::panelTextColour.withAlpha(0.55f),
                    FontStyle::Semibold, 10.5f, Justification::centredLeft);
                x += colW;
            }

            // Data rows
            int y = headerH;
            g.setColour(PlugDataColours::outlineColour);
            g.drawHorizontalLine(headerH, 0.0f, static_cast<float>(getWidth()));

            for (int r = 0; r < static_cast<int>(rows.size()); r++) {
                int rh = rowHeights[r];
                x = 0;
                for (int i = 0; i < static_cast<int>(columns.size()); i++) {
                    bool isLast = (i == static_cast<int>(columns.size()) - 1);
                    int colW = isLast ? flexW : columns[i].width;
                    Rectangle<float> cell(static_cast<float>(x + cellPadX),
                        static_cast<float>(y + cellPadY),
                        static_cast<float>(colW - cellPadX * 2),
                        static_cast<float>(rh - cellPadY * 2));
                    cellLayouts[r][i].draw(g, cell);
                    x += colW;
                }
                if (r < static_cast<int>(rows.size()) - 1) {
                    g.setColour(PlugDataColours::outlineColour.withAlpha(0.5f));
                    g.drawHorizontalLine(y + rh, 0.0f, static_cast<float>(getWidth()));
                }
                y += rh;
            }

            // Outer outline drawn last so it sits cleanly on top
            g.setColour(PlugDataColours::outlineColour);
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }
    };

    struct ContentComponent final : public Component {
        PillRow pillRow;
        TitleBlock titleBlock;
        TaglineBlock taglineBlock;
        ObjectPreview objectPreview;

        SectionHeader inletsHeader { "Inlets" };
        SectionHeader outletsHeader { "Outlets" };
        IoletsColumn inletsColumn { true };
        IoletsColumn outletsColumn { false };

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

        ContentComponent()
        {
            addAndMakeVisible(pillRow);
            addAndMakeVisible(titleBlock);
            addAndMakeVisible(taglineBlock);
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
        }

        void clear()
        {
            pillRow.setData("", { });
            titleBlock.setTitle("");
            taglineBlock.setText("");
            inletsColumn.setPorts({ });
            outletsColumn.setPorts({ });
            hasContent = hasArguments = hasMethods = hasFlags = false;
            argumentsHeader.setVisible(false);
            argumentsTable.setVisible(false);
            methodsHeader.setVisible(false);
            methodsTable.setVisible(false);
            flagsHeader.setVisible(false);
            flagsTable.setVisible(false);
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
            argumentsHeader.setVisible(hasArguments);
            argumentsTable.setVisible(hasArguments);

            methodsTable.columns.clear();
            methodsTable.columns.add({ "Message", 200, true });
            methodsTable.columns.add({ "Description", 120, false });
            methodsTable.rows.clear();
            for (auto const& m : info.methods)
                methodsTable.rows.add(StringArray { m.type, m.description });
            hasMethods = info.methods.size() > 0;
            methodsHeader.setVisible(hasMethods);
            methodsTable.setVisible(hasMethods);

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
            flagsHeader.setVisible(hasFlags);
            flagsTable.setVisible(hasFlags);

            hasContent = true;
            repaint();
        }

        void recalculateLayout(int outerWidth)
        {
            constexpr int padX = 32;
            constexpr int padTop = 16;
            constexpr int padBottom = 48;
            constexpr int sectionGap = 28;
            constexpr int sectionHeaderH = 30;

            if (!hasContent) {
                setSize(outerWidth, 200);
                return;
            }

            int contentW = jmax(280, outerWidth - padX * 2);
            int x = padX;
            int y = padTop;

            // Pill row
            pillRow.recalculateLayout(contentW);
            pillRow.setTopLeftPosition(x, y);
            y += pillRow.getHeight() + 14;

            // Title
            titleBlock.setBounds(x, y, contentW, 56);
            y += 56 + 4;

            // Tagline
            taglineBlock.recalculateLayout(contentW);
            taglineBlock.setTopLeftPosition(x, y);
            y += taglineBlock.getHeight();

            y += sectionGap;

            // Viz card
            objectPreview.recalculateLayout(contentW);
            objectPreview.setTopLeftPosition(x, y);
            y += objectPreview.getHeight();

            y += sectionGap;

            // Inlets / Outlets — two columns when wide, stacked when narrow
            bool twoCol = contentW >= 720;
            if (twoCol) {
                int colGap = 24;
                int colW = (contentW - colGap) / 2;

                inletsHeader.setBounds(x, y, colW, sectionHeaderH);
                outletsHeader.setBounds(x + colW + colGap, y, colW, sectionHeaderH);
                int colsY = y + sectionHeaderH + 10;

                inletsColumn.recalculateLayout(colW);
                inletsColumn.setTopLeftPosition(x, colsY);

                outletsColumn.recalculateLayout(colW);
                outletsColumn.setTopLeftPosition(x + colW + colGap, colsY);

                y = colsY + jmax(inletsColumn.getHeight(), outletsColumn.getHeight());
            } else {
                inletsHeader.setBounds(x, y, contentW, sectionHeaderH);
                y += sectionHeaderH + 10;
                inletsColumn.recalculateLayout(contentW);
                inletsColumn.setTopLeftPosition(x, y);
                y += inletsColumn.getHeight() + sectionGap;

                outletsHeader.setBounds(x, y, contentW, sectionHeaderH);
                y += sectionHeaderH + 10;
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
                y += sectionHeaderH + 10;
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
    MainToolbarButton backButton = MainToolbarButton(Icons::Back);
    String objectName;
};
