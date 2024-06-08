/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Pd/Patch.h"

struct ExporterBase : public Component
    , public Value::Listener
    , public ChildProcess
    , public ThreadPool {
    TextButton exportButton = TextButton("Export");

    Value inputPatchValue = SynchronousValue();
    Value projectNameValue;
    Value projectCopyrightValue;

    bool blockDialog = false;

#if JUCE_WINDOWS
    inline static String const exeSuffix = ".exe";
#else
    inline static String const exeSuffix = "";
#endif

    inline static File heavyExecutable = Toolchain::dir.getChildFile("bin").getChildFile("Heavy").getChildFile("Heavy" + exeSuffix);

    bool validPatchSelected = false;

    File patchFile;
    File openedPatchFile;
    File realPatchFile;

    PropertiesPanel panel;

    ExportingProgressView* exportingView;

    bool shouldQuit = false;

    PluginEditor* editor;

    ExporterBase(PluginEditor* pluginEditor, ExportingProgressView* exportView)
        : ThreadPool(1, Thread::osDefaultStackSize, Thread::Priority::highest)
        , exportingView(exportView)
        , editor(pluginEditor)
    {
        addAndMakeVisible(exportButton);

        auto backgroundColour = findColour(PlugDataColour::panelBackgroundColourId);
        exportButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        exportButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        exportButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        Array<PropertiesPanelProperty*> properties;

        auto* patchChooser = new PropertiesPanel::ComboComponent("Patch to export", inputPatchValue, { "Currently opened patch", "Other patch (browse)" });
        patchChooser->comboBox.setTextWhenNothingSelected("Choose a patch to export...");
        patchChooser->comboBox.setSelectedId(-1);
        properties.add(patchChooser);

        auto* nameProperty = new PropertiesPanel::EditableComponent<String>("Project Name (optional)", projectNameValue);
        nameProperty->setInputRestrictions("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
        properties.add(nameProperty);

        properties.add(new PropertiesPanel::EditableComponent<String>("Project Copyright (optional)", projectCopyrightValue));

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("General", properties);

        panel.setContentWidth(400);

        addAndMakeVisible(panel);
        inputPatchValue.addListener(this);
        projectNameValue.addListener(this);
        projectCopyrightValue.addListener(this);

        if (auto* cnv = editor->getCurrentCanvas()) {
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
            Dialogs::showSaveDialog([this](URL url) {
                auto result = url.getLocalFile();
                if (result.getParentDirectory().exists()) {
                    startExport(result);
                }
            },
                "", "HeavyExport", nullptr, true);
        };
    }

    ~ExporterBase() override
    {
        if (openedPatchFile.existsAsFile()) {
            openedPatchFile.deleteFile();
        }

        shouldQuit = true;
        if (isRunning())
            kill();
        removeAllJobs(true, -1);
    }

    virtual ValueTree getState() = 0;
    virtual void setState(ValueTree& state) = 0;

    void startExport(File const& outDir)
    {
        auto patchPath = patchFile.getFullPathName();
        auto const& outPath = outDir.getFullPathName();
        auto projectTitle = projectNameValue.toString();
        auto projectCopyright = projectCopyrightValue.toString();

        if (!projectTitle.unquoted().containsNonWhitespaceChars()) {
            if (!realPatchFile.getFileNameWithoutExtension().isEmpty())
                projectTitle = realPatchFile.getFileNameWithoutExtension();
            else
                projectTitle = "Untitled";
        }

        // Add file location to search paths
        auto searchPaths = StringArray { patchFile.getParentDirectory().getFullPathName() };

        editor->pd->setThis();

        // Get pd's search paths
        char* paths[1024];
        int numItems;
        pd::Interface::getSearchPaths(paths, &numItems);

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
            } else if (idx == 2 && !blockDialog) {
                Dialogs::showOpenDialog([this](URL url) {
                    auto result = url.getLocalFile();
                    if (result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    } else {
                        inputPatchValue = -1;
                        patchFile = "";
                        validPatchSelected = false;
                    }
                },
                    true, false, "*.pd", "HeavyPatchLocation", nullptr);
            }
        }

        exportButton.setEnabled(validPatchSelected);
    }

    void resized() override
    {
        panel.setBounds(0, 0, getWidth(), getHeight() - 50);
        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));
    }

    static String createMetaJson(DynamicObject::Ptr metaJson)
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
