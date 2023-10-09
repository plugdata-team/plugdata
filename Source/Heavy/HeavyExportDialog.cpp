/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
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
#include "PdExporter.h"

class ExporterSettingsPanel : public Component
    , private ListBoxModel {
public:
    ListBox listBox;
    int listBoxWidth = 160;

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<ExporterBase> views;

    std::function<void(int)> onChange;

    StringArray items = {
        "C++ Code",
        "Electro-Smith Daisy",
        "DPF Audio Plugin",
        "Pd External"
    };

    ExporterSettingsPanel(PluginEditor* editor, ExportingProgressView* exportingView)
    {
        addChildComponent(views.add(new CppExporter(editor, exportingView)));
        addChildComponent(views.add(new DaisyExporter(editor, exportingView)));
        addChildComponent(views.add(new DPFExporter(editor, exportingView)));
        addChildComponent(views.add(new PdExporter(editor, exportingView)));

        addAndMakeVisible(listBox);

        listBox.setModel(this);
        listBox.setOutlineThickness(0);
        listBox.selectRow(0);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setRowHeight(28);
    }

    void paint(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(listBoxWidth);

        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRoundedRectangle(listboxBounds.toFloat(), Corners::windowCornerRadius);
        g.fillRect(listboxBounds.removeFromRight(10));
    }

    void paintOverChildren(Graphics& g) override
    {
        auto listboxBounds = getLocalBounds().removeFromLeft(listBoxWidth);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
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
        listBox.setBounds(b.removeFromLeft(listBoxWidth).reduced(4));

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
                PlugDataLook::fillSmoothedRectangle(g, Rectangle<float>(3, 3, width - 6, height - 6), Corners::defaultCornerRadius);
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
    , infoButton(new MainToolbarButton(Icons::Help))
{
    hasToolchain = Toolchain::dir.exists();
    

    // Don't do this relative to toolchain variable, that won't work on Windows
    auto const versionFile = ProjectInfo::appDataDir.getChildFile("Toolchain").getChildFile("VERSION");
    auto const installedVersion = versionFile.loadFileAsString().trim().removeCharacters(".").getIntValue();

    
    // Create integer versions by removing the dots
    // Compare latest version on github to the currently installed version
    int latestVersion;
    try {
        auto compatTable = JSON::parse(URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/COMPATIBILITY").readEntireTextStream());
        // Get latest version
        if(compatTable.isObject())
        {
            latestVersion = compatTable.getDynamicObject()->getProperty(String(ProjectInfo::versionString).upToFirstOccurrenceOf("-", false, false)).toString().removeCharacters(".").getIntValue();
        }
        else {
            latestVersion = installedVersion;
        }
    }
    // Network error, JSON error or empty version string somehow
    catch (...) {
        latestVersion = installedVersion;
        return;
    }

    if (hasToolchain && latestVersion > installedVersion) {
        installer->needsUpdate = true;
        hasToolchain = false;
    }

    addChildComponent(*installer);
    addChildComponent(*exporterPanel);
    addChildComponent(*exportingView);

    exportingView->setAlwaysOnTop(true);

    infoButton->onClick = [](){
        URL("https://wasted-audio.github.io/hvcc/docs/01.introduction.html#what-is-heavy").launchInDefaultBrowser();
    };
    addAndMakeVisible(*infoButton);
    
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
    
    auto titlebarBounds = getLocalBounds().removeFromTop(40);
    
    Path p;
    p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, true, true, false, false);

    g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
    g.fillPath(p);
        
    Fonts::drawStyledText(g, "Compiler", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
}

void HeavyExportDialog::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawHorizontalLine(40, 0.0f, getWidth());
}

void HeavyExportDialog::resized()
{
    auto b = getLocalBounds().withTrimmedTop(40);
    exporterPanel->setBounds(b);
    installer->setBounds(b);
    exportingView->setBounds(b);
    infoButton->setBounds(Rectangle<int>(40, 40));
}
