/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PluginEditor.h"
#include "Pd/Patch.h"

struct ExporterBase : public Component
    , public Value::Listener
    , public ChildProcess
    , public ThreadPool {
    TextButton exportButton = TextButton("Export");

    Value inputPatchValue;
    Value projectNameValue;
    Value projectCopyrightValue;

#if JUCE_WINDOWS
    const inline static String exeSuffix = ".exe";
#else
    const inline static String exeSuffix = "";
#endif

    inline static File heavyExecutable = Toolchain::dir.getChildFile("bin").getChildFile("Heavy").getChildFile("Heavy" + exeSuffix);

    bool validPatchSelected = false;

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    OwnedArray<PropertiesPanel::Property> properties;

    File patchFile;
    File openedPatchFile;
    File realPatchFile;

    ExportingProgressView* exportingView;

    int labelWidth = 180;
    bool shouldQuit = false;

    PluginEditor* editor;

    ExporterBase(PluginEditor* pluginEditor, ExportingProgressView* exportView)
        : ThreadPool(1)
        , exportingView(exportView)
        , editor(pluginEditor)
    {
        addAndMakeVisible(exportButton);

        auto* patchChooser = new PropertiesPanel::ComboComponent("Patch to export", inputPatchValue, { "Currently opened patch", "Other patch (browse)" });
        properties.add(patchChooser);
        patchChooser->comboBox.setTextWhenNothingSelected("Choose a patch to export...");
        patchChooser->comboBox.setSelectedId(-1);

        properties.add(new PropertiesPanel::EditableComponent<String>("Project Name (optional)", projectNameValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Project Copyright (optional)", projectCopyrightValue));

        inputPatchValue.addListener(this);
        projectNameValue.addListener(this);
        projectCopyrightValue.addListener(this);

        for (auto* panel : properties) {

            panel->setColour(ComboBox::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));
            addAndMakeVisible(panel);
        }

        if (auto* cnv = editor->getCurrentCanvas(false)) {
            openedPatchFile = File::createTempFile(".pd");
            Toolchain::deleteTempFileLater(openedPatchFile);
            openedPatchFile.replaceWithText(cnv->patch.getCanvasContent(), false, false, "\n");
            patchChooser->comboBox.setItemEnabled(1, true);
            patchChooser->comboBox.setSelectedId(1);
            patchFile = openedPatchFile;
            realPatchFile = cnv->patch.getCurrentFile();

            if (realPatchFile.existsAsFile()) {
                projectNameValue = realPatchFile.getFileNameWithoutExtension();
            }
        } else {
            patchChooser->comboBox.setItemEnabled(1, false);
            patchChooser->comboBox.setSelectedId(0);
            validPatchSelected = false;
        }

        exportButton.onClick = [this]() {

#if JUCE_LINUX
            constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
#else
            constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles;
#endif

            saveChooser = std::make_unique<FileChooser>("Choose a location...", File::getSpecialLocation(File::userHomeDirectory), "", true);

            saveChooser->launchAsync(folderChooserFlags,
                [this](FileChooser const& fileChooser) {
                    const auto file = fileChooser.getResult();

                    if (file.getParentDirectory().exists()) {
                        startExport(file);
                    }
                });
        };
    }

    ~ExporterBase()
    {
        if (openedPatchFile.existsAsFile()) {
            openedPatchFile.deleteFile();
        }

        shouldQuit = true;
        if (isRunning())
            kill();
        removeAllJobs(true, -1);
    }

    void startExport(File outDir)
    {

        auto patchPath = patchFile.getFullPathName();
        auto outPath = outDir.getFullPathName();
        auto projectTitle = projectNameValue.toString();
        auto projectCopyright = projectCopyrightValue.toString();

        if (projectTitle.isEmpty())
            projectTitle = "Untitled";

        // Add file location to search paths
        auto searchPaths = StringArray { patchFile.getParentDirectory().getFullPathName() };

        // Get pd's search paths
        char* paths[1024];
        int numItems;
        libpd_get_search_paths(paths, &numItems);

        if (realPatchFile.existsAsFile()) {
            searchPaths.add(realPatchFile.getParentDirectory().getFullPathName());
        }

        for (int i = 0; i < numItems; i++) {
            searchPaths.add(paths[i]);
        }

        // Make sure we don't add the file location twice
        searchPaths.removeDuplicates(false);

        addJob([this, patchPath, outPath, projectTitle, projectCopyright, searchPaths]() mutable {
            exportingView->monitorProcessOutput(this);

            exportingView->showState(ExportingProgressView::Busy);

            auto result = performExport(patchPath, outPath, projectTitle, projectCopyright, searchPaths);

            if (shouldQuit)
                return;

            exportingView->showState(result ? ExportingProgressView::Failure : ExportingProgressView::Success);

            exportingView->stopMonitoring();

            MessageManager::callAsync([this]() {
                repaint();
            });
        });
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(inputPatchValue)) {
            int idx = getValue<int>(v);

            if (idx == 1) {
                patchFile = openedPatchFile;
                validPatchSelected = true;
            } else if (idx == 2) {
                // Open file browser
                openChooser = std::make_unique<FileChooser>("Choose file to open", File::getSpecialLocation(File::userHomeDirectory), "*.pd", true);

                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](FileChooser const& fileChooser) {
                    auto result = fileChooser.getResult();
                    if (result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    } else {
                        inputPatchValue = -1;
                        patchFile = "";
                        validPatchSelected = false;
                    }
                });
            }
        }

        exportButton.setEnabled(validPatchSelected);
    }

    void resized() override
    {
        auto b = Rectangle<int>(0, 50, getWidth(), getHeight() - 38);

        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));

        for (auto* panel : properties) {
            auto panelBounds = b.removeFromTop(23);
            panel->setBounds(panelBounds);
            b.removeFromTop(5);
        }
    }

    String createMetaJson(DynamicObject::Ptr metaJson)
    {
        auto metadata = File::createTempFile(".json");
        Toolchain::deleteTempFileLater(metadata);
        String metaString = JSON::toString(var(metaJson.get()));
        metadata.replaceWithText(metaString, false, false, "\n");
        return metadata.getFullPathName();
    }

private:
    virtual bool performExport(String pdPatch, String outdir, String name, String copyright, StringArray searchPaths) = 0;
};
