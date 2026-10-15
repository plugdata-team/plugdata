/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "HeavyExporter.h"

// The toolchain's exporters on the left, and the selected one's settings on the right
class ExporterSettingsPanel final : public Component
    , private ListBoxModel {
public:
    ListBox listBox;
    int listBoxWidth = 160;

    HeavyExporter exporter;
    Array<HeavyExporter::Definition> definitions;

    ExporterSettingsPanel(PluginEditor* editor, ExportingProgressView* exportingView)
        : exporter(editor, exportingView)
        , definitions(HeavyExporter::getDefinitions())
    {
        addAndMakeVisible(exporter);
        addAndMakeVisible(listBox);

        listBox.setModel(this);
        listBox.setOutlineThickness(0);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setRowHeight(28);

        exporter.restoreGeneralState();

        int selectedRow = 0;
        for (int i = 0; i < definitions.size(); i++) {
            if (definitions[i].id == HeavyExporter::getSelectedDefinitionId())
                selectedRow = i;
        }
        listBox.selectRow(selectedRow);
    }

    ~ExporterSettingsPanel() override
    {
        exporter.saveState();
    }

    void paint(Graphics& g) override
    {
        auto const listboxBounds = getLocalBounds().removeFromLeft(listBoxWidth);

        Path p;
        p.addRoundedRectangle(listboxBounds.getX(), listboxBounds.getY(), listboxBounds.getWidth(), listboxBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, false);

        g.setColour(getThemeColours(*this).sidebarBackgroundColour);
        g.fillPath(p);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto const listboxBounds = getLocalBounds().removeFromLeft(listBoxWidth);

        g.setColour(getThemeColours(*this).toolbarOutlineColour);
        g.drawLine(Line<float>(listboxBounds.getTopRight().toFloat(), listboxBounds.getBottomRight().toFloat()));
    }

    void selectedRowsChanged(int const lastRowSelected) override
    {
        if (isPositiveAndBelow(lastRowSelected, definitions.size()))
            exporter.switchTo(definitions[lastRowSelected]);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        listBox.setBounds(b.removeFromLeft(listBoxWidth).reduced(4));
        exporter.setBounds(b);
    }

    int getNumRows() override
    {
        return definitions.size();
    }

    void paintListBoxItem(int const row, Graphics& g, int const width, int const height, bool const rowIsSelected) override
    {
        auto const& colours = getThemeColours(*this);

        if (isPositiveAndBelow(row, definitions.size())) {
            if (rowIsSelected) {
                g.setColour(colours.sidebarActiveBackgroundColour);
                g.fillRoundedRectangle(Rectangle<float>(3, 3, width - 6, height - 6), Corners::defaultCornerRadius);
            }

            Fonts::drawText(g, definitions[row].name, Rectangle<int>(15, 0, width - 30, height), colours.sidebarTextColour, 15);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExporterSettingsPanel)
};
