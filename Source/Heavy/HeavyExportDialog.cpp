/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Dialogs/Dialogs.h"
#include "HeavyExportDialog.h"

#include "PluginEditor.h"
#include "Utility/PropertiesPanel.h"
#include "Utility/OSUtils.h"

#if JUCE_LINUX
#    include <unistd.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#endif

#include "Toolchain.h"
#include "ExportingProgressView.h"
#include "ExporterBase.h"
#include "CppExporter.h"
#include "DPFExporter.h"
#include "DaisyExporter.h"

class ExporterSettingsPanel : public Component
    , private ListBoxModel {
public:
    ListBox listBox;

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<ExporterBase> views;

    std::function<void(int)> onChange;

    StringArray items = { "C++", "Daisy", "DPF" };

    ExporterSettingsPanel(PluginEditor* editor, ExportingProgressView* exportingView)
    {
        addChildComponent(views.add(new CppExporter(editor, exportingView)));
        addChildComponent(views.add(new DaisyExporter(editor, exportingView)));
        addChildComponent(views.add(new DPFExporter(editor, exportingView)));

        addAndMakeVisible(listBox);

        listBox.setModel(this);
        listBox.setOutlineThickness(0);
        listBox.selectRow(0);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setRowHeight(28);
    }

    void paint(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(150);

        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRoundedRectangle(listboxBounds.toFloat(), Corners::windowCornerRadius);
        g.fillRect(listboxBounds.removeFromRight(10));
    }

    void paintOverChildren(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(150);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(Line<float>(listboxBounds.getTopRight().toFloat(), listboxBounds.getBottomRight().toFloat()));
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        for (auto* view : views) {
            // Make sure we remember common values when switching views
            if (view->isVisible()) {
                views[lastRowSelected]->patchFile = view->patchFile;
                views[lastRowSelected]->projectNameValue = view->projectNameValue.getValue();
                views[lastRowSelected]->projectCopyrightValue = view->projectCopyrightValue.getValue();
                views[lastRowSelected]->inputPatchValue = view->inputPatchValue.getValue();
            }
            view->setVisible(false);
        }

        views[lastRowSelected]->setVisible(true);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        listBox.setBounds(b.removeFromLeft(150).reduced(4));

        for (auto* view : views) {
            view->setBounds(b);
        }
    }

    int getNumRows() override
    {
        return items.size();
    }

    StringArray getExports() const
    {
        return items;
    }

    void paintListBoxItem(int row, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (isPositiveAndBelow(row, items.size())) {
            if (rowIsSelected) {
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                g.fillRoundedRectangle(5, 3, width - 10, height - 6, Corners::defaultCornerRadius);
            }

            auto const textColour = findColour(rowIsSelected ? PlugDataColour::sidebarActiveTextColourId : PlugDataColour::sidebarTextColourId);

            Fonts::drawText(g, items[row], Rectangle<int>(15, 0, width - 30, height), textColour, 15);
        }
    }

    int getBestHeight(int preferredHeight)
    {
        auto extra = listBox.getOutlineThickness() * 2;
        return jmax(listBox.getRowHeight() * 2 + extra,
            jmin(listBox.getRowHeight() * getNumRows() + extra,
                preferredHeight));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExporterSettingsPanel)
};

HeavyExportDialog::HeavyExportDialog(Dialog* dialog)
    : exportingView(new ExportingProgressView())
    , exporterPanel(new ExporterSettingsPanel(dynamic_cast<PluginEditor*>(dialog->parentComponent), exportingView.get()))
    , installer(new ToolchainInstaller(dynamic_cast<PluginEditor*>(dialog->parentComponent)))
{

    hasToolchain = Toolchain::dir.exists();

    // Create integer versions by removing the dots
    // Compare latest version on github to the currently installed version
    auto const latestVersion = URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/VERSION").readEntireTextStream().trim().removeCharacters(".").getIntValue();

    // Don't do this relative to toolchain variable, that won't work on Windows
    auto const versionFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Toolchain").getChildFile("VERSION");
    auto const installedVersion = versionFile.loadFileAsString().trim().removeCharacters(".").getIntValue();

    if (hasToolchain && latestVersion > installedVersion) {
        installer->needsUpdate = true;
        hasToolchain = false;
    }

    addChildComponent(*installer);
    addChildComponent(*exporterPanel);
    addChildComponent(*exportingView);

    exportingView->setAlwaysOnTop(true);

    installer->toolchainInstalledCallback = [this]() {
        hasToolchain = true;
        exporterPanel->setVisible(true);
        installer->setVisible(false);
    };

    if (hasToolchain) {
        exporterPanel->setVisible(true);
    } else {
        installer->setVisible(true);
    }
}

HeavyExportDialog::~HeavyExportDialog()
{
    // Clean up temp files
    Toolchain::deleteTempFiles();
}

void HeavyExportDialog::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);
}

void HeavyExportDialog::resized()
{
    auto b = getLocalBounds();
    exporterPanel->setBounds(b);
    installer->setBounds(b);
    exportingView->setBounds(b);
}
