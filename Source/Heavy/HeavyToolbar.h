/*
 // Copyright (c) 2026 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "ExportingProgressView.h"
#include "ExporterSettingsPanel.h"
#include "ToolchainInstaller.h"

namespace HeavyToolbarConstants {

constexpr int runWidth = 30;
constexpr int settingsWidth = 22;
constexpr int statusWidth = 28;

// Fixed, so nothing shifts when the label changes: fits the longest target name and export step
constexpr int totalWidth = 240;

// The knockout circles show the toolbar through the symbol, so these are the badge colour itself
auto const successColour = Colour(62, 170, 90);
auto const failureColour = Colour(245, 62, 62);

}

// Shows the exporter's own settings panel, which stays owned by the toolbar
class ExporterCallout final : public Component {
public:
    ExporterCallout(ExporterBase* exporterToShow, std::function<void()> onCloseCallback)
        : exporter(exporterToShow)
        , onClose(std::move(onCloseCallback))
    {
        addAndMakeVisible(*exporter);
        setSize(430, 320);
    }

    ~ExporterCallout() override
    {
        if (exporter)
            removeChildComponent(exporter);

        NullCheckedInvocation::invoke(onClose);
    }

    void resized() override
    {
        if (exporter)
            exporter->setBounds(getLocalBounds().reduced(4));
    }

private:
    SafePointer<ExporterBase> exporter;
    std::function<void()> onClose;
};

// Live view of the export output, for as long as the callout is open
class ConsoleCallout final : public Component {
public:
    explicit ConsoleCallout(ExportingProgressView* view)
        : progressView(view)
    {
        addAndMakeVisible(console.getViewport());
        setSize(500, 320);

        // Both of these run on the message thread, so anything not yet shown arrives through the hook
        console.append(progressView->deliveredOutput);

        progressView->onConsoleOutput = [this](String const& text) {
            console.append(text);
        };
    }

    ~ConsoleCallout() override
    {
        progressView->onConsoleOutput = nullptr;
    }

    void paint(Graphics& g) override
    {
        g.setColour(getThemeColours(*this).sidebarBackgroundColour);
        g.fillRoundedRectangle(console.getViewport().getBounds().toFloat(), Corners::defaultCornerRadius);
    }

    void resized() override
    {
        console.getViewport().setBounds(getLocalBounds().reduced(4));
    }

private:
    ExportingProgressView* progressView;
    ExporterConsole console;
};

// Quick export widget for the main toolbar: [ run | target | settings ] plus the last result.
// Turns into a single install button until the toolchain is there, and into [ run | cancel ] while exporting
class HeavyToolbar final : public Component
    , public MultiTimer
    , public TooltipClient
    , public ToolchainInstall::Listener {
public:
    explicit HeavyToolbar(PluginEditor* parentEditor)
        : editor(parentEditor)
        , exportingView(new ExportingProgressView(false))
        , target(ExporterSettingsPanel::getSelectedTarget())
    {
        exportingView->onStateChange = [this] {
            bool const verifying = verifyOutputDir != File();

            switch (exportingView->state) {
            case ExportingProgressView::Success:
            case ExportingProgressView::BootloaderFlashSuccess:
                editor->pd->logMessage(verifying ? "Patch compiles successfully" : "Export successful");
                break;
            case ExportingProgressView::Failure:
            case ExportingProgressView::BootloaderFlashFailure:
                editor->pd->logError(verifying ? "Patch failed to compile" : "Export failed");
                break;
            case ExportingProgressView::NotExporting:
                editor->pd->logMessage(verifying ? "Verification cancelled" : "Export cancelled");
                break;
            default:
                break;
            }

            updateSpinner();

            // Whichever way it ended, the next export starts out cancellable
            if (!isExporting()) {
                cancelling = false;

                // The generated code was only ever there to prove the patch compiles
                if (verifying) {
                    verifyOutputDir.deleteRecursively();
                    verifyOutputDir = File();
                }
            }

            updateHover();
            repaint();
        };

        exportingView->onStatusChange = [this] { repaint(); };

        ToolchainInstall::getInstance()->addListener(this);
        updateToolchainState();

        setSize(HeavyToolbarConstants::totalWidth, getHeight());
    }

    ~HeavyToolbar() override
    {
        setWindowMovable(true);

        if (auto* install = ToolchainInstall::getInstanceWithoutCreating())
            install->removeListener(this);

        if (exporter)
            saveState();

        exportingView->onStateChange = nullptr;
        exportingView->onStatusChange = nullptr;
    }

    void paint(Graphics& g) override
    {
        // An export that's already running keeps its cancel button, whatever the update check says
        if (isExporting()) {
            paintExportingSections(g);
        } else if (needsToolchain) {
            paintInstallButton(g);
        } else {
            paintExportSections(g);
            paintStatusBadge(g);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        switch (getSectionAt(e.getPosition())) {
        case Install:
            ToolchainInstall::getInstance()->install(editor);
            break;
        case Cancel:
            cancelExport();
            break;
        case Run:
            if (isExporting())
                showConsoleCallout(Run);
            else
                startTimer(HoldTimer, 400);
            break;
        case Target:
            showTargetMenu();
            break;
        case Settings:
            showSettingsCallout();
            break;
        case Status:
            showConsoleCallout(Status);
            break;
        default:
            break;
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        // Holding stops the timer to show the export type menu, so a click is what's left here
        if (!isTimerRunning(HoldTimer))
            return;

        stopTimer(HoldTimer);

        if (getSectionAt(e.getPosition()) == Run)
            runExport();
    }

    void mouseEnter(MouseEvent const&) override
    {
        setWindowMovable(false);
        updateHover();
    }

    void mouseMove(MouseEvent const&) override
    {
        updateHover();
    }

    void mouseExit(MouseEvent const& e) override
    {
        setWindowMovable(true);
        hoveredSection = None;
        repaint();
    }

    void timerCallback(int const timerID) override
    {
        if (timerID == SpinnerTimer) {
            repaint(getSectionBounds(isExporting() ? Run : Install).getSmallestIntegerContainer());
        } else {
            stopTimer(HoldTimer);
            showExportTypeMenu();
        }
    }

    String getTooltip() override
    {
        switch (hoveredSection) {
        case Install: {
            auto const& install = *ToolchainInstall::getInstance();
            if (install.installing)
                return "";
            if (install.error.isNotEmpty())
                return install.error;
            return install.updateAvailable ? "Download the latest Heavy toolchain" : "Download the Heavy toolchain, which compiles and exports patches";
        }
        case Run:
            return isExporting() ? "Show export output" : "Run Heavy export";
        case Target:
            return "Choose export target";
        case Settings:
            return "Export settings";
        case Status:
            return lastExportSucceeded() ? "Last export succeeded" : "Last export failed";
        default:
            return "";
        }
    }

    void visibilityChanged() override
    {
        if (!isVisible())
            return;

        updateToolchainState();

        // Only asked once the toolbar is in use, and only once per session
        if (ExporterBase::toolchainDir.exists())
            ToolchainInstall::getInstance()->checkForUpdate();
    }

    void toolchainInstallChanged() override
    {
        updateToolchainState();
        updateSpinner();
        updateHover();
        repaint();
    }

    void toolchainInstalled() override
    {
        editor->pd->logMessage("Heavy toolchain installed");
    }

    void toolchainInstallFailed(String const& error) override
    {
        editor->pd->logError(error);
    }

private:
    enum Section {
        None,
        Run,
        Target,
        Settings,
        Status,
        Install,
        Cancel
    };

    enum TimerID {
        SpinnerTimer,
        HoldTimer
    };

    void paintExportSections(Graphics& g)
    {
        auto const& colours = getThemeColours(*this);
        auto const bounds = getLocalBounds().toFloat().reduced(0, 3.5f);
        constexpr float cornerRadius = Corners::defaultCornerRadius;

        for (auto const section : { Run, Target, Settings }) {
            auto const sectionBounds = getSectionBounds(section);
            bool const isFirst = section == Run;
            bool const isLast = section == Settings;

            Path p;
            p.addRoundedRectangle(sectionBounds.getX(), sectionBounds.getY(), sectionBounds.getWidth(), sectionBounds.getHeight(),
                cornerRadius, cornerRadius, isFirst, isLast, isFirst, isLast);

            g.setColour(colours.toolbarHoverColour.contrasting(hoveredSection == section ? 0.05f : 0.0f));
            g.fillPath(p);
        }

        g.setColour(colours.toolbarOutlineColour.withAlpha(0.5f));
        g.drawVerticalLine(roundToInt(getSectionBounds(Target).getX()), bounds.getY() + 3.0f, bounds.getBottom() - 3.0f);
        g.drawVerticalLine(roundToInt(getSectionBounds(Settings).getX()), bounds.getY() + 3.0f, bounds.getBottom() - 3.0f);

        auto const runBounds = getSectionBounds(Run);
        auto const triangle = Rectangle<float>(11.0f, 12.0f).withCentre(runBounds.getCentre());

        Path arrow;
        arrow.addTriangle(triangle.getX(), triangle.getY(), triangle.getX(), triangle.getBottom(), triangle.getRight(), triangle.getCentreY());

        g.setColour(colours.toolbarTextColour);
        g.fillPath(arrow.createPathWithRoundedCorners(2.0f));

        // Hints that holding the button opens the export type menu
        if (hoveredSection == Run) {
            auto const chevron = Rectangle<float>(5.0f, 2.5f).withCentre(runBounds.getBottomRight().translated(-5.0f, -4.5f));

            Path hint;
            hint.startNewSubPath(chevron.getX(), chevron.getY());
            hint.lineTo(chevron.getCentreX(), chevron.getBottom());
            hint.lineTo(chevron.getRight(), chevron.getY());

            g.setColour(colours.toolbarTextColour.withAlpha(0.75f));
            g.strokePath(hint, PathStrokeType(1.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        Fonts::drawText(g, ExporterSettingsPanel::targets[target], getSectionBounds(Target).reduced(8, 0), colours.toolbarTextColour, 14, Justification::centred);

        Fonts::drawIcon(g, Icons::ChrevronDownFilled, getSectionBounds(Settings).toNearestInt(), colours.toolbarTextColour, 12);
    }

    // [ spinner | the step the export is on ], where the step turns into a cancel button on hover
    void paintExportingSections(Graphics& g)
    {
        auto const& colours = getThemeColours(*this);
        auto const bounds = getLocalBounds().toFloat().reduced(0, 3.5f);
        constexpr float cornerRadius = Corners::defaultCornerRadius;

        for (auto const section : { Run, Cancel }) {
            auto const sectionBounds = getSectionBounds(section);
            bool const isFirst = section == Run;
            bool const highlighted = hoveredSection == section && !(section == Cancel && cancelling);

            Path p;
            p.addRoundedRectangle(sectionBounds.getX(), sectionBounds.getY(), sectionBounds.getWidth(), sectionBounds.getHeight(),
                cornerRadius, cornerRadius, isFirst, !isFirst, isFirst, !isFirst);

            g.setColour(colours.toolbarHoverColour.contrasting(highlighted ? 0.05f : 0.0f));
            g.fillPath(p);
        }

        g.setColour(colours.toolbarOutlineColour.withAlpha(0.5f));
        g.drawVerticalLine(roundToInt(getSectionBounds(Cancel).getX()), bounds.getY() + 3.0f, bounds.getBottom() - 3.0f);

        auto const runBounds = getSectionBounds(Run);
        getLookAndFeel().drawSpinningWaitAnimation(g, colours.toolbarTextColour, roundToInt(runBounds.getCentreX()) - 7, roundToInt(runBounds.getCentreY()) - 7, 14, 14);

        Fonts::drawText(g, getCancelText(), getSectionBounds(Cancel).reduced(8, 0), colours.toolbarTextColour, 14, Justification::centred);
    }

    // One button over the whole pill, with the download progress as a line along its bottom
    void paintInstallButton(Graphics& g)
    {
        auto const& colours = getThemeColours(*this);
        auto const& install = *ToolchainInstall::getInstance();
        auto const buttonBounds = getSectionBounds(Install);
        constexpr float cornerRadius = Corners::defaultCornerRadius;

        g.setColour(colours.toolbarHoverColour.contrasting(hoveredSection == Install && !install.installing ? 0.05f : 0.0f));
        g.fillRoundedRectangle(buttonBounds, cornerRadius);

        String text;
        if (install.unpacking)
            text = "Installing toolchain...";
        else if (install.installing)
            text = "Downloading toolchain...";
        else if (install.error.isNotEmpty())
            text = install.updateAvailable ? "Update failed, try again" : "Install failed, try again";
        else
            text = install.updateAvailable ? "Update Heavy toolchain" : "Install Heavy toolchain";

        constexpr int iconWidth = 22;
        auto const textWidth = Fonts::getStringWidth(text, Fonts::getDefaultFont().withHeight(14));
        auto content = buttonBounds.withSizeKeepingCentre(std::min(iconWidth + textWidth, buttonBounds.getWidth() - 16.0f), buttonBounds.getHeight());
        auto const iconBounds = content.removeFromLeft(iconWidth);

        if (install.installing)
            getLookAndFeel().drawSpinningWaitAnimation(g, colours.toolbarTextColour, roundToInt(iconBounds.getX()), roundToInt(iconBounds.getCentreY()) - 7, 14, 14);
        else
            Fonts::drawIcon(g, Icons::Download, iconBounds.toNearestInt().withWidth(14), colours.toolbarTextColour, 14);

        Fonts::drawText(g, text, content, colours.toolbarTextColour, 14, Justification::centredLeft);

        if (install.installing) {
            // Kept clear of the rounded corners, so it only runs along the flat part of the bottom edge
            auto const track = buttonBounds.reduced(cornerRadius, 0.0f).removeFromBottom(2.0f);
            auto const progress = install.unpacking ? 1.0f : jlimit(0.0f, 1.0f, install.progress);

            g.setColour(colours.toolbarOutlineColour.withAlpha(0.5f));
            g.fillRoundedRectangle(track, 1.0f);

            g.setColour(colours.toolbarActiveColour);
            g.fillRoundedRectangle(track.withWidth(track.getWidth() * progress), 1.0f);
        }
    }

    void paintStatusBadge(Graphics& g)
    {
        using namespace HeavyToolbarConstants;

        if (!hasExportResult())
            return;

        bool const succeeded = lastExportSucceeded();
        auto const badgeColour = (succeeded ? successColour : failureColour).brighter(hoveredSection == Status ? 0.2f : 0.0f);

        Fonts::drawIcon(g, succeeded ? Icons::CheckmarkCircle : Icons::DismissCircle, getSectionBounds(Status).toNearestInt(), badgeColour, 16);
    }

    void updateSpinner()
    {
        if (!isExporting() && !ToolchainInstall::getInstance()->installing)
            stopTimer(SpinnerTimer);
        else if (!isTimerRunning(SpinnerTimer))
            startTimer(SpinnerTimer, 20);
    }

    void updateHover()
    {
        if (auto const section = getSectionAt(getMouseXYRelative()); section != hoveredSection) {
            hoveredSection = section;
            repaint();
        }
    }

    // The step the export is on, which turns into the cancel action on hover
    String getCancelText() const
    {
        if (cancelling)
            return "Cancelling...";

        if (hoveredSection == Cancel)
            return verifyOutputDir != File() ? "Cancel verification" : "Cancel export";

        auto const& status = exportingView->currentStatus;
        return status.isNotEmpty() ? status : ExporterSettingsPanel::targets[target];
    }

    // Dragging the widget shouldn't drag the whole window along with it
    void setWindowMovable(bool const movable) const
    {
#if JUCE_MAC
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer, movable);
            }
        }
#else
        ignoreUnused(movable);
#endif
    }

    Rectangle<float> getSectionBounds(Section const section) const
    {
        using namespace HeavyToolbarConstants;

        auto bounds = getLocalBounds().toFloat().reduced(0, 3.5f);

        // The result badge sits outside the pill, like Xcode's, and keeps its place when there's no result
        auto const statusBounds = bounds.removeFromRight(statusWidth);

        switch (section) {
        case Run:
            return bounds.removeFromLeft(runWidth);
        case Settings:
            return bounds.removeFromRight(settingsWidth);
        case Target:
            return bounds.withTrimmedLeft(runWidth).withTrimmedRight(settingsWidth);
        case Cancel:
            return bounds.withTrimmedLeft(runWidth);
        case Status:
            return statusBounds;
        default:
            return bounds;
        }
    }

    Section getSectionAt(Point<int> const position) const
    {
        for (auto const section : { Install, Run, Target, Settings, Cancel, Status }) {
            if (isSectionShown(section) && getSectionBounds(section).withTop(0.0f).withBottom(getHeight()).contains(position.toFloat()))
                return section;
        }

        return None;
    }

    bool isSectionShown(Section const section) const
    {
        // Mid-export, all that's left is the output and cancelling
        if (isExporting())
            return section == Run || section == Cancel;

        if (needsToolchain)
            return section == Install;

        switch (section) {
        case Run:
        case Target:
        case Settings:
            return true;
        // The badge only exists once there's a result
        case Status:
            return hasExportResult();
        default:
            return false;
        }
    }

    // Nothing can be exported or configured before the toolchain is there, or while it's out of date
    void updateToolchainState()
    {
        auto const& install = *ToolchainInstall::getInstance();
        needsToolchain = install.installing || install.updateAvailable || !ExporterBase::toolchainDir.exists();
    }

    // Checked again before anything runs, in case the toolchain went missing since the toolbar last looked
    bool hasToolchain()
    {
        updateToolchainState();
        repaint();

        return !needsToolchain;
    }

    void cancelExport()
    {
        if (cancelling || !isExporting())
            return;

        cancelling = true;

        if (auto* running = verifyOutputDir != File() ? verifyExporter.get() : exporter.get())
            running->cancelExport();

        repaint();
    }

    ExporterBase* getExporter()
    {
        if (!exporter) {
            exporter.reset(ExporterSettingsPanel::createExporter(target, editor, exportingView.get()));
            ExporterSettingsPanel::restoreState(exporter.get());

            // Quick export targets the opened patch, unless the saved settings already picked one
            if (!exporter->validPatchSelected && editor->getCurrentCanvas())
                exporter->inputPatchValue = 1;

            // The run button exports here, so the panel doesn't need its own button
            exporter->exportButton.setVisible(false);
        }

        return exporter.get();
    }

    bool isExporting() const
    {
        return exportingView->state == ExportingProgressView::Exporting || exportingView->state == ExportingProgressView::Flashing;
    }

    bool hasExportResult() const
    {
        return exportingView->state >= ExportingProgressView::Success && exportingView->state < ExportingProgressView::NotExporting;
    }

    bool lastExportSucceeded() const
    {
        return exportingView->state == ExportingProgressView::Success || exportingView->state == ExportingProgressView::BootloaderFlashSuccess;
    }

    void saveState() const
    {
        ExporterSettingsPanel::saveState(exporter.get(), target);
    }

    void showTargetMenu()
    {
        PopupMenu menu;
        for (int i = 0; i < ExporterSettingsPanel::targets.size(); i++) {
            menu.addItem(i + 1, ExporterSettingsPanel::targets[i], true, i == target);
        }

        menu.showMenuAsync(PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(getSectionBounds(Target).getSmallestIntegerContainer())),
            [_this = SafePointer(this)](int const result) {
                if (!_this || !result || result - 1 == _this->target)
                    return;

                _this->saveState();
                _this->target = result - 1;
                _this->exporter.reset();
                _this->saveState();

                _this->repaint();
            });
    }

    void showExportTypeMenu()
    {
        if (!hasToolchain())
            return;

        constexpr int verifyMenuId = 1000;

        auto* currentExporter = getExporter();
        auto const exportTypes = currentExporter->getExportTypes();

        PopupMenu menu;
        // These run once and don't change the stored setting, so nothing is ticked
        for (int i = 0; i < exportTypes.size(); i++) {
            menu.addItem(i + 1, exportTypes[i]);
        }

        if (!exportTypes.isEmpty())
            menu.addSeparator();

        menu.addItem(verifyMenuId, "Verify", currentExporter->canPerformExport(), false);

        menu.showMenuAsync(PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(getSectionBounds(Run).getSmallestIntegerContainer())),
            [_this = SafePointer(this)](int const result) {
                if (!_this || !result)
                    return;

                if (result == verifyMenuId)
                    _this->verifyPatch();
                else
                    _this->runExport(result);
            });
    }

    // An exportType other than 0 comes from the menu, and applies to this run only
    void runExport(int const exportType = 0)
    {
        if (!hasToolchain())
            return;

        // Nothing to export yet, so let the user choose a patch first
        if (auto* currentExporter = getExporter(); currentExporter->canPerformExport())
            currentExporter->triggerExport(exportType);
        else
            showSettingsCallout();
    }

    // Compiles the patch to C++ in a temp folder purely to see whether it succeeds
    void verifyPatch()
    {
        auto* currentExporter = getExporter();
        if (!currentExporter->canPerformExport())
            return;

        verifyExporter = std::make_unique<CppExporter>(editor, exportingView.get());
        currentExporter->copyPatchSelectionTo(*verifyExporter);

        verifyOutputDir = File::getSpecialLocation(File::tempDirectory).getChildFile("HeavyVerify-" + Uuid().toString().substring(10));
        verifyExporter->startExport(verifyOutputDir);
    }

    void showSettingsCallout()
    {
        if (!hasToolchain())
            return;

        auto content = std::make_unique<ExporterCallout>(getExporter(), [_this = SafePointer(this)] {
            if (_this)
                _this->saveState();
        });

        showCallout(std::move(content), Settings);
    }

    void showConsoleCallout(Section const anchor)
    {
        showCallout(std::make_unique<ConsoleCallout>(exportingView.get()), anchor);
    }

    // The exporter panel is styled for the panel background, not the lighter popup one
    void showCallout(std::unique_ptr<Component> content, Section const anchor)
    {
        auto& callout = editor->showCalloutBox(std::move(content), localAreaToGlobal(getSectionBounds(anchor).getSmallestIntegerContainer()));
        callout.getProperties().set("PanelBackground", true);
        callout.repaint();
    }

    PluginEditor* editor;
    std::unique_ptr<ExportingProgressView> exportingView;
    std::unique_ptr<ExporterBase> exporter;

    std::unique_ptr<ExporterBase> verifyExporter;
    File verifyOutputDir;

    int target;
    Section hoveredSection = None;

    bool needsToolchain = false;
    bool cancelling = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeavyToolbar)
};
