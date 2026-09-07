/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "ExporterBase.h"
#include "CppExporter.h"
#include "DPFExporter.h"
#include "DaisyExporter.h"
#include "OWLExporter.h"
#include "PdExporter.h"
#include "WASMExporter.h"
#include "FMODExporter.h"
#include "UnityExporter.h"
#include "WwiseExporter.h"

class ExporterSettingsPanel final : public Component
    , private ListBoxModel {
public:
    ListBox listBox;
    int listBoxWidth = 160;

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<ExporterBase> views;

    std::function<void(int)> onChange;

    static inline StringArray const targets = {
        "C++ Code",
        "Electro-Smith Daisy",
        "DPF Audio Plugin",
        "OWL Platform",
        "Pd External",
        "WebAssembly",
        "FMOD Plugin",
        "Unity Plugin",
        "Wwise Plugin"
    };

    static ExporterBase* createExporter(int const target, PluginEditor* editor, ExportingProgressView* exportingView)
    {
        switch (target) {
        case 1:
            return new DaisyExporter(editor, exportingView);
        case 2:
            return new DPFExporter(editor, exportingView);
        case 3:
            return new OWLExporter(editor, exportingView);
        case 4:
            return new PdExporter(editor, exportingView);
        case 5:
            return new WASMExporter(editor, exportingView);
        case 6:
            return new FMODExporter(editor, exportingView);
        case 7:
            return new UnityExporter(editor, exportingView);
        case 8:
            return new WwiseExporter(editor, exportingView);
        default:
            return new CppExporter(editor, exportingView);
        }
    }

    // The exporter selected in the panel, shared with the quick export toolbar
    static int getSelectedTarget()
    {
        auto const state = SettingsFile::getInstance()->getProperty<DynamicObject>("heavy_state");
        return state ? std::clamp(static_cast<int>(state->getProperty("selected_exporter")), 0, targets.size() - 1) : 0;
    }

    static void saveState(ExporterBase* exporter, int const target)
    {
        DynamicObject::Ptr state = SettingsFile::getInstance()->getProperty<DynamicObject>("heavy_state");
        if (!state)
            state = new DynamicObject();

        state->setProperty("selected_exporter", target);
        if (exporter)
            exporter->getState(state);

        SettingsFile::getInstance()->setProperty("heavy_state", var(state.get()));
    }

    static void restoreState(ExporterBase* exporter)
    {
        if (auto const state = SettingsFile::getInstance()->getProperty<DynamicObject>("heavy_state")) {
            exporter->blockDialog = true;
            exporter->setState(state);
            exporter->blockDialog = false;
        }
        exporter->updateExportButton();
    }

    ExporterSettingsPanel(PluginEditor* editor, ExportingProgressView* exportingView)
    {
        for (int i = 0; i < targets.size(); i++) {
            addChildComponent(views.add(createExporter(i, editor, exportingView)));
        }

        addAndMakeVisible(listBox);

        listBox.setModel(this);
        listBox.setOutlineThickness(0);
        listBox.selectRow(0);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setRowHeight(28);

        listBox.selectRow(getSelectedTarget());
        for (auto* view : views) {
            restoreState(view);
        }
    }

    ~ExporterSettingsPanel() override
    {
        DynamicObject::Ptr state = new DynamicObject();
        state->setProperty("selected_exporter", listBox.getSelectedRow());
        for (auto* view : views) {
            view->getState(state);
        }
        SettingsFile::getInstance()->setProperty("heavy_state", var(state.get()));
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
        for (auto* view : views) {
            if (view->isVisible()) {
                views[lastRowSelected]->patchFile = view->patchFile;
                views[lastRowSelected]->projectNameValue = view->projectNameValue.getValue();
                views[lastRowSelected]->projectCopyrightValue = view->projectCopyrightValue.getValue();

                views[lastRowSelected]->blockDialog = true;
                views[lastRowSelected]->inputPatchValue = view->inputPatchValue.getValue();
                views[lastRowSelected]->blockDialog = false;
            }
            view->setVisible(false);
        }

        views[lastRowSelected]->setVisible(true);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        listBox.setBounds(b.removeFromLeft(listBoxWidth).reduced(4));

        for (auto* view : views) {
            view->setBounds(b);
        }
    }

    int getNumRows() override
    {
        return targets.size();
    }

    void paintListBoxItem(int const row, Graphics& g, int const width, int const height, bool const rowIsSelected) override
    {
        auto const& colours = getThemeColours(*this);

        if (isPositiveAndBelow(row, targets.size())) {
            if (rowIsSelected) {
                g.setColour(colours.sidebarActiveBackgroundColour);
                g.fillRoundedRectangle(Rectangle<float>(3, 3, width - 6, height - 6), Corners::defaultCornerRadius);
            }

            auto const textColour = colours.sidebarTextColour;

            Fonts::drawText(g, targets[row], Rectangle<int>(15, 0, width - 30, height), textColour, 15);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExporterSettingsPanel)
};
