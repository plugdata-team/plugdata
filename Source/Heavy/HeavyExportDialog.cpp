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
// #include "Dialogs/HelpDialog.h"

#include "PluginEditor.h"
#include "Components/PropertiesPanel.h"
#include "Utility/OSUtils.h"

#include "ToolchainInstaller.h"
#include "ExportingProgressView.h"
#include "ExporterSettingsPanel.h"
#include "HeavyToolbar.h"

std::unique_ptr<Component> HeavyExportDialog::createHeavyToolbar(PluginEditor* editor)
{
    return std::make_unique<HeavyToolbar>(editor);
}

HeavyExportDialog::HeavyExportDialog(Dialog* dialog)
    : exportingView(new ExportingProgressView())
    , installer(new ToolchainInstaller(dynamic_cast<PluginEditor*>(dialog->parentComponent), dialog))
    , exporterPanel(new ExporterSettingsPanel(dynamic_cast<PluginEditor*>(dialog->parentComponent), exportingView.get()))
    , infoButton(new MainToolbarButton(Icons::Help))
{
    hasToolchain = ExporterBase::toolchainDir.exists();

    // Don't do this relative to toolchain variable, that won't work on Windows
    auto const versionFile = ProjectInfo::appDataDir.getChildFile("Toolchain").getChildFile("VERSION");
    auto const installedVersion = versionFile.loadFileAsString().trim().removeCharacters(".").getIntValue();

    // Create integer versions by removing the dots
    // Compare latest version on github to the currently installed version
    int latestVersion;
    try {
        auto const compatTable = JSON::parse(URL("https://raw.githubusercontent.com/plugdata-team/plugdata-heavy-toolchain/main/COMPATIBILITY").readEntireTextStream());
        // Get latest version
        if (compatTable.isObject()) {
            latestVersion = compatTable.getDynamicObject()->getProperty(String(ProjectInfo::versionString).upToFirstOccurrenceOf("-", false, false)).toString().removeCharacters(".").getIntValue();
        } else {
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

    /*
    infoButton->onClick = [this]() {
        helpDialog = std::make_unique<HelpDialog>(nullptr);
        helpDialog->onClose = [this]() {
            helpDialog.reset(nullptr);
        };
    }; */
    infoButton->onClick = [] {
        URL("https://wasted-audio.github.io/hvcc/latest/getting-started/#what-is-heavy").launchInDefaultBrowser();
    };
    addAndMakeVisible(*infoButton);

    installer->toolchainInstalledCallback = [this] {
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
    Dialogs::dismissFileDialog();

    // Clean up temp files
    ExporterBase::deleteTempFiles();
}

void HeavyExportDialog::paint(Graphics& g)
{
    auto const& colours = getThemeColours(*this);

    g.setColour(colours.panelBackgroundColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

    auto const titlebarBounds = getLocalBounds().removeFromTop(40);

    Path p;
    p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

    g.setColour(colours.toolbarBackgroundColour);
    g.fillPath(p);

    Fonts::drawStyledText(g, "Compiler", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), colours.panelTextColour, Semibold, 15, Justification::centred);
}

void HeavyExportDialog::paintOverChildren(Graphics& g)
{
    g.setColour(getThemeColours(*this).toolbarOutlineColour);
    g.drawHorizontalLine(40, 0.0f, getWidth());
}

void HeavyExportDialog::resized()
{
    auto const b = getLocalBounds().withTrimmedTop(40);
    exporterPanel->setBounds(b);
    installer->setBounds(b);
    exportingView->setBounds(b);
    infoButton->setBounds(Rectangle<int>(40, 40));
}
