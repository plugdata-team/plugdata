/*
 // Copyright (c) 2026 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "ExportingProgressView.h"
#include "ExporterSettingsPanel.h"

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

// Quick export widget for the main toolbar: [ run | target | settings ] plus the last result
class HeavyToolbar final : public Component
    , public MultiTimer
    , public TooltipClient {
public:
    explicit HeavyToolbar(PluginEditor* parentEditor)
        : editor(parentEditor)
        , exportingView(new ExportingProgressView())
        , target(ExporterSettingsPanel::getSelectedTarget())
    {
        exportingView->onStateChange = [this] {
            bool const verifying = verifyOutputDir != File();

            switch (exportingView->state) {
            case ExportingProgressView::Exporting:
            case ExportingProgressView::Flashing:
                startTimer(SpinnerTimer, 20);
                break;
            case ExportingProgressView::Success:
            case ExportingProgressView::BootloaderFlashSuccess:
                editor->pd->logMessage(verifying ? "Patch compiles successfully" : "Export successful");
                break;
            case ExportingProgressView::Failure:
            case ExportingProgressView::BootloaderFlashFailure:
                editor->pd->logError(verifying ? "Patch failed to compile" : "Export failed");
                break;
            default:
                break;
            }

            if (!isExporting()) {
                stopTimer(SpinnerTimer);

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

        setSize(HeavyToolbarConstants::totalWidth, getHeight());
    }

    ~HeavyToolbar() override
    {
        setWindowMovable(true);

        if (exporter)
            saveState();

        exportingView->onStateChange = nullptr;
        exportingView->onStatusChange = nullptr;
    }

    void paint(Graphics& g) override
    {
        using namespace HeavyToolbarConstants;

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
        if (isExporting()) {
            getLookAndFeel().drawSpinningWaitAnimation(g, colours.toolbarTextColour, roundToInt(runBounds.getCentreX()) - 7, roundToInt(runBounds.getCentreY()) - 7, 14, 14);
        } else {
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
        }

        Fonts::drawText(g, getLabelText(), getSectionBounds(Target).reduced(8, 0), colours.toolbarTextColour, 14, Justification::centred);

        Fonts::drawIcon(g, Icons::ChrevronDownFilled, getSectionBounds(Settings).toNearestInt(), colours.toolbarTextColour, 12);

        if (hasExportResult()) {
            bool const succeeded = lastExportSucceeded();
            auto const badgeColour = (succeeded ? successColour : failureColour).brighter(hoveredSection == Status ? 0.2f : 0.0f);

            Fonts::drawIcon(g, succeeded ? Icons::CheckmarkCircle : Icons::DismissCircle, getSectionBounds(Status).toNearestInt(), badgeColour, 16);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        switch (getSectionAt(e.getPosition())) {
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

        if (getSectionAt(e.getPosition()) != Run || !hasToolchain())
            return;

        // Nothing to export yet, so let the user choose a patch first
        if (auto* currentExporter = getExporter(); currentExporter->canPerformExport())
            currentExporter->triggerExport();
        else
            showSettingsCallout();
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
            repaint(getSectionBounds(Run).getSmallestIntegerContainer());
        } else {
            stopTimer(HoldTimer);
            showExportTypeMenu();
        }
    }

    String getTooltip() override
    {
        switch (hoveredSection) {
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

private:
    enum Section {
        None,
        Run,
        Target,
        Settings,
        Status
    };

    enum TimerID {
        SpinnerTimer,
        HoldTimer
    };

    void updateHover()
    {
        if (auto const section = getSectionAt(getMouseXYRelative()); section != hoveredSection) {
            hoveredSection = section;
            repaint();
        }
    }

    // The export step takes over the middle section while it runs
    String getLabelText() const
    {
        auto const& status = exportingView->currentStatus;
        return isExporting() && status.isNotEmpty() ? status : ExporterSettingsPanel::targets[target];
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
        case Status:
            return statusBounds;
        default:
            return bounds;
        }
    }

    Section getSectionAt(Point<int> const position) const
    {
        for (auto const section : { Run, Target, Settings, Status }) {
            // The badge only exists once there's a result, and only the output is reachable mid-export
            if (section == Status && !hasExportResult())
                continue;
            if (section != Run && isExporting())
                continue;

            if (getSectionBounds(section).withTop(0.0f).withBottom(getHeight()).contains(position.toFloat()))
                return section;
        }

        return None;
    }

    // Nothing can be exported or configured before the toolchain is there, so send the user to the installer
    bool hasToolchain()
    {
        if (ExporterBase::toolchainDir.exists())
            return true;

        Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
        return false;
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
        int const selected = getValue<int>(currentExporter->exportTypeValue);
        for (int i = 0; i < exportTypes.size(); i++) {
            menu.addItem(i + 1, exportTypes[i], true, i + 1 == selected);
        }

        if (!exportTypes.isEmpty())
            menu.addSeparator();

        menu.addItem(verifyMenuId, "Verify", currentExporter->canPerformExport(), false);

        menu.showMenuAsync(PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(getSectionBounds(Run).getSmallestIntegerContainer())),
            [_this = SafePointer(this)](int const result) {
                if (!_this || !result)
                    return;

                if (result == verifyMenuId) {
                    _this->verifyPatch();
                    return;
                }

                _this->getExporter()->exportTypeValue = result;
                _this->saveState();
            });
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeavyToolbar)
};
