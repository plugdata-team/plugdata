/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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
    bool canvasDirty = false;
    bool isTempFile = false;

    File patchFile;
    File openedPatchFile;
    File realPatchFile;

    PropertiesPanel panel;

    ExportingProgressView* exportingView;

    bool shouldQuit = false;

    Label unsavedLabel = Label("", "Warning: patch has unsaved changes");
    PluginEditor* editor;

    ExporterBase(PluginEditor* pluginEditor, ExportingProgressView* exportView)
        : ThreadPool(1, Thread::osDefaultStackSize, Thread::Priority::highest)
        , exportingView(exportView)
        , editor(pluginEditor)
    {
        addAndMakeVisible(exportButton);

        auto const backgroundColour = findColour(PlugDataColour::panelBackgroundColourId);
        exportButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        exportButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        exportButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

        PropertiesArray properties;

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

        if (auto const* cnv = editor->getCurrentCanvas()) {
            openedPatchFile = File::createTempFile(".pd");
            Toolchain::deleteTempFileLater(openedPatchFile);
            patchFile = cnv->patch.getCurrentFile();
            if (!patchFile.existsAsFile()) {
                openedPatchFile.replaceWithText(cnv->patch.getCanvasContent(), false, false, "\n");
                patchChooser->comboBox.setItemEnabled(1, true);
                patchChooser->comboBox.setSelectedId(1);
                realPatchFile = patchFile;
                patchFile = openedPatchFile;
                canvasDirty = false;
                isTempFile = true;
            } else {
                canvasDirty = cnv->patch.isDirty();
                openedPatchFile = patchFile;
                realPatchFile = patchFile;
            }

            if (realPatchFile.existsAsFile()) {
                projectNameValue = realPatchFile.getFileNameWithoutExtension();
            }
        } else {
            patchChooser->comboBox.setItemEnabled(1, false);
            patchChooser->comboBox.setSelectedId(0);
            validPatchSelected = false;
            canvasDirty = false;
        }

        exportButton.onClick = [this] {
            Dialogs::showSaveDialog([this](URL const& url) {
                auto const result = url.getLocalFile();
                if (result.getParentDirectory().exists()) {
                    startExport(result);
                }
            },
                "", "HeavyExport", nullptr, true);
        };

        unsavedLabel.setColour(Label::textColourId, Colours::orange);
        addChildComponent(unsavedLabel);
    }

    // TODO: hides non-virtual destructor from juce::ChildProcess! modify JUCE to make it virtual?
    ~ExporterBase() override
    {
        if (openedPatchFile.existsAsFile() && isTempFile) {
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
#if JUCE_WINDOWS
        auto const patchPath = patchFile.getFullPathName().replaceCharacter('\\', '/');
        auto const& outPath = outDir.getFullPathName().replaceCharacter('\\', '/');
#else
        auto patchPath = patchFile.getFullPathName();
        auto const& outPath = outDir.getFullPathName();
#endif

        auto projectTitle = projectNameValue.toString();
        auto projectCopyright = projectCopyrightValue.toString();

        if (!projectTitle.unquoted().containsNonWhitespaceChars()) {
            if (!realPatchFile.getFileNameWithoutExtension().isEmpty())
                projectTitle = realPatchFile.getFileNameWithoutExtension();
            else
                projectTitle = "Untitled";
        }
        // Replace dash and space with underscore
        projectTitle = projectTitle.replaceCharacter('-', '_').replaceCharacter(' ', '_');

        // Add original file location to search paths
        auto searchPaths = StringArray {};
        if (realPatchFile.existsAsFile() && !realPatchFile.isRoot()) // Make sure file actually exists
        {
#if JUCE_WINDOWS
            searchPaths.add(realPatchFile.getParentDirectory().getFullPathName().replaceCharacter('\\', '/').quoted());
#else
            searchPaths.add(realPatchFile.getParentDirectory().getFullPathName().quoted());
#endif
        }
        editor->pd->setThis();

        // Get pd's search paths
        char* paths[1024];
        int numItems;
        pd::Interface::getSearchPaths(paths, &numItems);

        for (int i = 0; i < numItems; i++) {
            searchPaths.add(String(paths[i]).quoted());
        }

        // Make sure we don't add the file location twice
        searchPaths.removeDuplicates(false);
        addJob([this, patchPath, outPath, projectTitle, projectCopyright, searchPaths]() mutable {
            exportingView->monitorProcessOutput(this);
            exportingView->showState(ExportingProgressView::Exporting);

            auto const result = performExport(patchPath, outPath, projectTitle, projectCopyright, searchPaths);

            if (shouldQuit)
                return;

            exportingView->showState(result ? ExportingProgressView::Failure : ExportingProgressView::Success);

            exportingView->stopMonitoring();

            MessageManager::callAsync([this] {
                repaint();
            });
        });
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(inputPatchValue)) {
            int const idx = getValue<int>(v);

            if (idx == 1) {
                patchFile = openedPatchFile;
                validPatchSelected = true;
                unsavedLabel.setVisible(canvasDirty);
            } else if (idx == 2 && !blockDialog) {
                Dialogs::showOpenDialog([this](URL const& url) {
                    auto const result = url.getLocalFile();
                    if (result.existsAsFile()) {
                        patchFile = result;
                        validPatchSelected = true;
                    } else {
                        inputPatchValue = -1;
                        patchFile = "";
                        validPatchSelected = false;
                    }
                    unsavedLabel.setVisible(false);
                },
                    true, false, "*.pd", "HeavyPatchLocation", nullptr);
            }
        }

        exportButton.setEnabled(validPatchSelected);
    }

    void resized() override
    {
        unsavedLabel.setBounds(10, getHeight() - 42, getWidth(), 42);
        panel.setBounds(0, 0, getWidth(), getHeight() - 50);
        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));
    }

    static File createMetaJson(DynamicObject::Ptr const& metaJson)
    {
        auto const metadata = File::createTempFile(".json");
        Toolchain::deleteTempFileLater(metadata);
        String const metaString = JSON::toString(var(metaJson.get()));
        metadata.replaceWithText(metaString, false, false, "\n");
        return metadata;
    }

private:
    virtual bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) = 0;
};
