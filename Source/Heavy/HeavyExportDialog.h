/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ExportingProgressView;
class ToolchainInstaller;
class ExporterSettingsPanel;
class Dialog;
class HelpDialog;

class HeavyExportDialog final : public Component {
    bool hasToolchain = false;

    std::unique_ptr<ExportingProgressView> exportingView;
    std::unique_ptr<ToolchainInstaller> installer;
    std::unique_ptr<ExporterSettingsPanel> exporterPanel;
    std::unique_ptr<MainToolbarButton> infoButton;

    // std::unique_ptr<HelpDialog> helpDialog;

public:
    explicit HeavyExportDialog(Dialog* dialog);

    ~HeavyExportDialog() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void resized() override;
};
