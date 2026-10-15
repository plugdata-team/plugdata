/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <array>
#include <optional>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Pd/Patch.h"
#include "Dialogs/Dialogs.h"
#include "Components/PropertiesPanel.h"
#include "ExportingProgressView.h"

// Runs a Heavy export described by a JSON file in the toolchain's exporters folder: which script to run,
// the settings to show, and how those settings become the script's arguments.
//
// {
//   "name": "Pd External",                         shown in the list of exporters
//   "order": 5,                                    position in that list
//   "script": "build-pd.sh",                       in the toolchain's run folder
//   "quick_menu": "mode",                          combo setting offered when holding the toolbar's run button
//   "arguments": [ "--pd-bin", "{pd_bin_dir}" ],   always passed
//   "sections": [ { "name": "Pd", "settings": [ ... ] } ]
// }
//
// Each setting has an "id", "label", "type" (combo, int, bool or string), an optional "default", and the
// "argument" it is passed as. Settings disabled by "enabled_if": { "other_id": value or [values] } are not passed.

class HeavyExporter final : public Component
    , public Value::Listener
    , public ThreadPool {
public:
    // What the export button does: write to a folder the user picks, or run straight onto the device
    enum ExportAction {
        Export,
        Flash,
        FlashBootloader
    };

    struct Definition {
        String id; // the JSON file's name
        String name;
        var json;
    };

    static inline File const toolchainDir = ProjectInfo::appDataDir.getChildFile("Toolchain");

    // The toolchain's exporters in list order, read again whenever the toolchain is replaced
    static Array<Definition> const& getDefinitions()
    {
        static Array<Definition> definitions;
        static Time loadedFolderTime;

        auto const folder = toolchainDir.getChildFile("exporters");
        if (auto const folderTime = folder.getLastModificationTime(); folderTime != loadedFolderTime) {
            loadedFolderTime = folderTime;
            definitions.clear();

            for (auto const& file : folder.findChildFiles(File::findFiles, false, "*.json")) {
                auto const json = JSON::parse(file);
                if (json["name"].isString() && json["script"].isString())
                    definitions.add({ file.getFileNameWithoutExtension(), json["name"].toString(), json });
            }

            std::stable_sort(definitions.begin(), definitions.end(), [](Definition const& a, Definition const& b) {
                return static_cast<int>(a.json["order"]) < static_cast<int>(b.json["order"]);
            });
        }

        return definitions;
    }

    static std::optional<Definition> findDefinition(String const& id)
    {
        for (auto const& definition : getDefinitions()) {
            if (definition.id == id)
                return definition;
        }
        return std::nullopt;
    }

    // The exporter last chosen in the export dialog or the toolbar
    static String getSelectedDefinitionId()
    {
        return getSavedState()->getProperty("selected_exporter").toString();
    }

    static String pathToString(File const& file)
    {
#if JUCE_WINDOWS
        return file.getFullPathName().replaceCharacter('\\', '/');
#else
        return file.getFullPathName();
#endif
    }

    static void deleteTempFileLater(File const& file)
    {
        tempFilesToDelete.add(file);
    }

    static void deleteTempFiles()
    {
        for (auto& file : tempFilesToDelete) {
            if (file.existsAsFile())
                file.deleteFile();
            if (file.isDirectory())
                file.deleteRecursively();
        }
    }

    HeavyExporter(PluginEditor* pluginEditor, ExportingProgressView* exportView)
        : ThreadPool(1, Thread::osDefaultStackSize, Thread::Priority::highest)
        , exportingView(exportView)
        , editor(pluginEditor)
    {
        addAndMakeVisible(exportButton);

        auto const backgroundColour = getThemeColours(*this).panelBackgroundColour;
        exportButton.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        exportButton.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        exportButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        exportButton.onClick = [this] { triggerExport(); };

        unsavedLabel.setColour(Label::textColourId, Colours::orange);
        addChildComponent(unsavedLabel);

        panel.setContentWidth(400);
        addAndMakeVisible(panel);

        inputPatchValue.addListener(this);
        projectNameValue.addListener(this);
        projectCopyrightValue.addListener(this);

        if (auto const* cnv = editor->getCurrentCanvas()) {
            hasOpenedPatch = true;
            openedPatchFile = File::createTempFile(".pd");
            deleteTempFileLater(openedPatchFile);
            patchFile = cnv->patch.getCurrentFile();
            if (!patchFile.existsAsFile()) {
                openedPatchFile.replaceWithText(cnv->patch.getCanvasContent(), false, false, "\n");
                realPatchFile = patchFile;
                patchFile = openedPatchFile;
                isTempFile = true;
                inputPatchValue = 1;
            } else {
                canvasDirty = cnv->patch.isDirty();
                openedPatchFile = patchFile;
                realPatchFile = patchFile;
            }

            if (realPatchFile.existsAsFile())
                projectNameValue = realPatchFile.getFileNameWithoutExtension();
        }

        buildPanel();
    }

    ~HeavyExporter() override
    {
        if (openedPatchFile.existsAsFile() && isTempFile)
            openedPatchFile.deleteFile();

        // Stops a running export without reporting it as cancelled: there's no one left to tell
        {
            ScopedLock lock(processLock);
            shouldQuit = true;
            if (process.isRunning())
                process.kill();
        }

        removeAllJobs(true, -1);
    }

    // Shows another exporter's settings, as they were last saved, without saving this one's
    void setDefinition(Definition const& newDefinition)
    {
        // The panel's properties refer to the old settings
        panel.clear();
        settings.clear();
        definition = newDefinition;

        auto const saved = getSavedState()->getProperty("exporters")[Identifier(definition.id)];
        for (auto const& section : getArray(definition.json, "sections")) {
            for (auto const& json : getArray(section, "settings")) {
                auto* setting = settings.add(new Setting { json, json["id"].toString(), json["type"].toString(), SynchronousValue() });
                setLogicalValue(*setting, saved.hasProperty(setting->id) ? saved[Identifier(setting->id)] : json["default"]);
                setting->value.addListener(this);
            }
        }

        buildPanel();
        updateEnablement();
        updateExportButton();
        resized();
    }

    // Keeps the current exporter's settings, then shows the next one's
    void switchTo(Definition const& next)
    {
        if (definition.id.isNotEmpty())
            saveState();
        setDefinition(next);
        saveState();
    }

    void restoreGeneralState()
    {
        auto const state = getSavedState();
        blockDialog = true;
        for (auto const& [name, value] : getGeneralState()) {
            if (state->hasProperty(name))
                *value = state->getProperty(name);
        }
        blockDialog = false;
        updateExportButton();
    }

    // Keeps the patch selection, the chosen exporter, and the exporter's settings by option value
    void saveState()
    {
        auto const state = getSavedState();
        for (auto const& [name, value] : getGeneralState())
            state->setProperty(name, value->getValue());

        if (definition.id.isNotEmpty()) {
            DynamicObject::Ptr exporters = state->getProperty("exporters").getDynamicObject();
            if (!exporters)
                exporters = new DynamicObject();

            auto* values = new DynamicObject();
            for (auto const* setting : settings)
                values->setProperty(setting->id, getLogicalValue(*setting));

            exporters->setProperty(definition.id, values);
            state->setProperty("exporters", var(exporters.get()));
            state->setProperty("selected_exporter", definition.id);
        }

        SettingsFile::getInstance()->setProperty("heavy_state", var(state.get()));
    }

    // Labels of the "quick_menu" setting's options, empty if the exporter has none
    StringArray getQuickExportOptions() const
    {
        StringArray labels;
        if (auto const* setting = getQuickMenuSetting()) {
            for (auto const& option : getArray(setting->json, "options"))
                labels.add(getOptionLabel(option));
        }
        return labels;
    }

    ExportAction getExportAction() const
    {
        auto const action = getChosenOptionField("action");
        if (action == "flash")
            return Flash;
        return action == "flash_bootloader" ? FlashBootloader : Export;
    }

    // Everything the exporter needs before it can run
    bool canPerformExport() const
    {
        if (!validPatchSelected || definition.id.isEmpty())
            return false;

        return std::none_of(settings.begin(), settings.end(), [this](Setting const* setting) {
            return setting->type == "string" && static_cast<bool>(setting->json["required"]) && isEnabled(*setting) && setting->value.toString().isEmpty();
        });
    }

    // Hands the chosen patch over to another exporter
    void copyPatchSelectionTo(HeavyExporter& other) const
    {
        other.patchFile = patchFile;
        other.realPatchFile = realPatchFile;
        other.projectNameValue = projectNameValue.getValue();
        other.projectCopyrightValue = projectCopyrightValue.getValue();

        other.blockDialog = true;
        other.inputPatchValue = inputPatchValue.getValue();
        other.blockDialog = false;
    }

    // Runs the export: flashing goes into a temp folder, everything else into a folder the user picks.
    // A quick option other than 0 applies to this run only.
    void triggerExport(int const quickOption = 0)
    {
        quickOptionOverride = quickOption;

        if (getExportAction() == Export) {
            Dialogs::showSaveDialog([this](URL const& url) {
                auto const result = url.getLocalFile();
                if (result.getParentDirectory().exists()) {
                    startExport(result);
                } else {
                    quickOptionOverride = 0;
                }
            },
                "", "HeavyExport", nullptr, true);
        } else {
            auto const tempFolder = File::getSpecialLocation(File::tempDirectory).getChildFile("Heavy-" + Uuid().toString().substring(10));
            deleteTempFileLater(tempFolder);
            startExport(tempFolder);
        }
    }

    void startExport(File const& outDir)
    {
        // Heavy names become C identifiers: fall back to the patch's name, and replace dashes and spaces
        auto name = projectNameValue.toString();
        if (!name.unquoted().containsNonWhitespaceChars())
            name = realPatchFile.getFileNameWithoutExtension().isNotEmpty() ? realPatchFile.getFileNameWithoutExtension() : "Untitled";
        name = name.replaceCharacter('-', '_').replaceCharacter(' ', '_');

        // The patch's own folder, then Pd's search paths
        StringArray searchPaths;
        if (realPatchFile.existsAsFile())
            searchPaths.add(pathToString(realPatchFile.getParentDirectory()));

        editor->pd->setThis();
        char* paths[1024];
        int numPaths;
        pd::Interface::getSearchPaths(paths, &numPaths);
        for (int i = 0; i < numPaths; i++)
            searchPaths.add(String(paths[i]).replaceCharacter('\\', '/'));
        searchPaths.removeDuplicates(false);

        // Settings are read here, on the message thread, while the export itself runs on the pool
        auto const action = getExportAction();
        auto const outPath = pathToString(outDir);
        auto const replacementScript = getChosenOptionField("script");
        auto const script = replacementScript.isNotEmpty() ? replacementScript : definition.json["script"].toString();
        auto const arguments = replacementScript.isNotEmpty() ? StringArray() : getScriptArguments(pathToString(patchFile), outPath, name, projectCopyrightValue.toString(), searchPaths);

        // A quick menu choice was only for this run
        quickOptionOverride = 0;
        updateExportButton();

        shouldQuit = false;
        cancelled = false;

        addJob([this, action, script, arguments, outPath] {
            exportingView->monitorProcessOutput(&process);
            exportingView->showState(action == Export ? ExportingProgressView::Exporting : ExportingProgressView::Flashing);
            exportingView->reportStatus(action == FlashBootloader ? "Flashing bootloader" : "Generating code");

            FileSystemWatcher::addGlobalIgnorePath(outPath);

            auto const failed = runToolchainScript(script, arguments);

            // The exporter is going away, so there's no one left to tell
            if (shouldQuit && !cancelled)
                return;

            if (cancelled)
                exportingView->showState(ExportingProgressView::NotExporting);
            else if (action == FlashBootloader)
                exportingView->showState(failed ? ExportingProgressView::BootloaderFlashFailure : ExportingProgressView::BootloaderFlashSuccess);
            else
                exportingView->showState(failed ? ExportingProgressView::Failure : ExportingProgressView::Success);

            exportingView->stopMonitoring();
            FileSystemWatcher::removeGlobalIgnorePath(outPath);
        });
    }

    // The export finishes as soon as the step it's on is killed, and doesn't start another
    void cancelExport()
    {
        ScopedLock lock(processLock);
        cancelled = true;
        shouldQuit = true;
        if (process.isRunning())
            process.kill();
    }

    // Returns true if the script failed or didn't run. Arguments stay separate from shell syntax,
    // including an empty copyright and paths with spaces.
    bool runToolchainScript(String const& scriptName, StringArray const& scriptArguments)
    {
        auto const script = toolchainDir.getChildFile("run").getChildFile(scriptName);
#if JUCE_WINDOWS
        auto const shell = toolchainDir.getChildFile("bin").getChildFile("sh.exe");
#else
        auto const shell = File("/bin/sh");
#endif
        StringArray arguments { pathToString(shell), pathToString(script) };
        arguments.addArray(scriptArguments);

        bool started = false;
        {
            ScopedLock lock(processLock);
            if (shouldQuit)
                return true;

            if (!script.existsAsFile() || !shell.existsAsFile()) {
                exportingView->logToConsole("Missing toolchain script or shell. Please reinstall the Heavy toolchain.\n");
                return true;
            }
#if JUCE_WINDOWS
            // JUCE's StringArray overload drops empty arguments on Windows. Encode the
            // native command line explicitly so the mandatory empty copyright survives.
            StringArray quotedArguments;
            for (auto const& argument : arguments) {
                String quoted = "\"";
                int backslashes = 0;
                for (auto const character : argument) {
                    if (character == '\\') {
                        ++backslashes;
                        continue;
                    }
                    quoted += String::repeatedString("\\", character == '"' ? backslashes * 2 + 1 : backslashes);
                    quoted += String::charToString(character);
                    backslashes = 0;
                }
                quoted += String::repeatedString("\\", backslashes * 2) + "\"";
                quotedArguments.add(quoted);
            }
            started = process.start(quotedArguments.joinIntoString(" "), ChildProcess::wantStdOut | ChildProcess::wantStdErr);
#else
            // This JUCE version also skips empty argv entries on POSIX. Prefix each
            // argument, then strip that prefix in a fixed shell wrapper (no eval).
            StringArray shellArguments { pathToString(shell), "-c",
                "for arg do set -- \"$@\" \"${arg#?}\"; shift; done; exec /bin/sh \"$@\"", "toolchain-run" };
            for (int i = 1; i < arguments.size(); ++i)
                shellArguments.add("x" + arguments[i]);
            // Preserve the PTY session so cancellation also hangs up child compilers.
            started = process.start(shellArguments, ChildProcess::wantStdOut | ChildProcess::wantStdErr | ChildProcess::wantTtyOut);
#endif
        }
        if (!started) {
            exportingView->logToConsole("Failed to start " + scriptName + "\n");
            return true;
        }
        process.waitForProcessToFinish(-1);
        exportingView->flushConsole();
        return shouldQuit || process.getExitCode() != 0;
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

        updateEnablement();
        updateExportButton();
    }

    void resized() override
    {
        auto const bottomStrip = exportButton.isVisible() || unsavedLabel.isVisible() ? 50 : 0;

        unsavedLabel.setBounds(10, getHeight() - 42, getWidth(), 42);
        panel.setBounds(0, 0, getWidth(), getHeight() - bottomStrip);
        exportButton.setBounds(getLocalBounds().removeFromBottom(23).removeFromRight(80).translated(-10, -10));
    }

    // Shared with the export dialog, the toolbar and the tests

    TextButton exportButton = TextButton("Export");

    Value inputPatchValue = SynchronousValue();
    Value projectNameValue;
    Value projectCopyrightValue;

    // Keeps setting inputPatchValue to 2 from opening the browse dialog
    bool blockDialog = false;
    bool validPatchSelected = false;

    File patchFile;
    File realPatchFile;

    // Stops the export where it is: set when the exporter goes away, or when the user cancels
    AtomicValue<bool> shouldQuit = false;

    // An option of the "quick_menu" setting chosen for one run, which leaves the stored setting alone
    int quickOptionOverride = 0;

    // One setting each, with the property that shows it (owned by the panel)
    struct Setting {
        var json;
        String id;
        String type;
        Value value; // combo: 1-based option index
        PropertiesPanelProperty* property = nullptr;
    };

    OwnedArray<Setting> settings;

    Setting* findSetting(String const& id) const
    {
        for (auto* setting : settings) {
            if (setting->id == id)
                return setting;
        }
        return nullptr;
    }

    // The value a setting stands for: the chosen option's value for a combo
    var getLogicalValue(Setting const& setting) const
    {
        if (setting.type == "combo")
            return getOptionValue(getSelectedOption(setting));
        if (setting.type == "bool")
            return getValue<bool>(setting.value);
        if (setting.type == "int")
            return getValue<int>(setting.value);
        return setting.value.toString();
    }

    // A setting is enabled when every setting named in its "enabled_if" has one of the listed values
    bool isEnabled(Setting const& setting) const
    {
        auto const* conditions = setting.json["enabled_if"].getDynamicObject();
        if (!conditions)
            return true;

        for (auto const& condition : conditions->getProperties()) {
            auto const* other = findSetting(condition.name.toString());
            auto const actual = other ? getLogicalValue(*other).toString() : String();
            auto const matches = [&actual](var const& expected) { return expected.toString() == actual; };
            auto const* candidates = condition.value.getArray();

            if (!other || (candidates ? std::none_of(candidates->begin(), candidates->end(), matches) : !matches(condition.value)))
                return false;
        }
        return true;
    }

    // What the script gets: the patch, name and copyright, each enabled setting, the fixed arguments, then the search paths
    StringArray getScriptArguments(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) const
    {
        StringArray arguments { pdPatch, name, copyright, "--output", outdir };

        for (auto const* setting : settings) {
            auto const argument = setting->json["argument"].toString();
            if (argument.isEmpty() || !isEnabled(*setting))
                continue;

            if (setting->type == "bool") {
                if (getValue<bool>(setting->value)) {
                    arguments.add(argument);
                    if (setting->json.hasProperty("value"))
                        arguments.add(substituteVariables(setting->json["value"].toString()));
                }
            } else if (setting->type == "string") {
                auto value = setting->value.toString();
                if (setting->json.hasProperty("browse"))
                    value = value.replaceCharacter('\\', '/');
                if (value.isNotEmpty())
                    arguments.addArray({ argument, value });
            } else {
                arguments.addArray({ argument, getLogicalValue(*setting).toString() });
            }
        }

        for (auto const& argument : getArray(definition.json, "arguments"))
            arguments.add(substituteVariables(argument.toString()));

        arguments.add("--");
        arguments.addArray(searchPaths);
        return arguments;
    }

private:
    // heavy_state = { selected_exporter, input_patch_value, project_name_value, project_copyright_value,
    //                 exporters: { exporter id: { setting id: value } } }
    static DynamicObject::Ptr getSavedState()
    {
        DynamicObject::Ptr state = SettingsFile::getInstance()->getProperty<DynamicObject>("heavy_state");
        return state ? state : DynamicObject::Ptr(new DynamicObject());
    }

    // The patch selection, name and copyright, which stay the same across exporters
    std::array<std::pair<char const*, Value*>, 3> getGeneralState()
    {
        return { { { "input_patch_value", &inputPatchValue },
            { "project_name_value", &projectNameValue },
            { "project_copyright_value", &projectCopyrightValue } } };
    }

    // The array at json[key], or an empty one if the definition leaves it out
    static Array<var> const& getArray(var const& json, char const* key)
    {
        static Array<var> const empty;
        auto const* array = json[key].getArray();
        return array ? *array : empty;
    }

    static String getOptionLabel(var const& option)
    {
        return option.isObject() ? option["label"].toString() : option.toString();
    }

    static String getOptionValue(var const& option)
    {
        return option.isObject() ? option["value"].toString() : option.toString();
    }

    void setLogicalValue(Setting& setting, var const& logicalValue)
    {
        if (setting.type == "combo") {
            auto const& options = getArray(setting.json, "options");
            int index = 1;
            for (int i = 0; i < options.size(); i++) {
                if (getOptionValue(options[i]) == logicalValue.toString())
                    index = i + 1;
            }
            setting.value = index;
        } else if (setting.type == "bool") {
            setting.value = static_cast<bool>(logicalValue);
        } else if (setting.type == "int") {
            setting.value = logicalValue.isVoid() ? static_cast<int>(setting.json["min"]) : static_cast<int>(logicalValue);
        } else {
            setting.value = logicalValue.toString();
        }
    }

    Setting const* getQuickMenuSetting() const
    {
        auto const* setting = findSetting(definition.json["quick_menu"].toString());
        return setting && setting->type == "combo" ? setting : nullptr;
    }

    // The option a combo shows, or the one picked from the quick menu for this run
    var getSelectedOption(Setting const& setting) const
    {
        auto const index = quickOptionOverride > 0 && &setting == getQuickMenuSetting() ? quickOptionOverride : getValue<int>(setting.value);
        auto const& options = getArray(setting.json, "options");
        return index >= 1 && index <= options.size() ? options.getReference(index - 1) : var();
    }

    // A field of the option chosen in any enabled combo: its "action", or a "script" that replaces the exporter's own
    String getChosenOptionField(char const* field) const
    {
        for (auto const* setting : settings) {
            if (setting->type == "combo" && isEnabled(*setting)) {
                if (auto const value = getSelectedOption(*setting)[field].toString(); value.isNotEmpty())
                    return value;
            }
        }
        return { };
    }

    String substituteVariables(String const& text) const
    {
        return text.replace("{externals_dir}", pathToString(ProjectInfo::appDataDir.getChildFile("Externals")))
            .replace("{pd_bin_dir}", pathToString(getPdBinDir()))
            .replace("{theme}", getThemeDescription());
    }

    // Windows Pd externals link against the pd.dll that comes with plugdata
    static File getPdBinDir()
    {
        return ProjectInfo::isStandalone
            ? File::getSpecialLocation(File::currentApplicationFile).getParentDirectory()
            : File::getSpecialLocation(File::globalApplicationsDirectory).getChildFile("plugdata");
    }

    // The editor's look, as "key=value" pairs that the DPF script turns into a NanoVG UI theme
    String getThemeDescription() const
    {
        auto const& colours = getThemeColours(*this);
        auto const hex = [](Colour const colour) { return "#" + colour.toDisplayString(false); };

        StringArray const pairs {
            "obj_corner_radius=" + String(getObjectCornerRadius(*this)),
            "cnv_color=" + hex(colours.canvasBackgroundColour),
            "cnv_txt_color=" + hex(colours.canvasTextColour),
            "io_color=" + hex(colours.guiObjectInternalOutlineColour),
            "bg_color=" + hex(colours.guiObjectBackgroundColour),
            "sel_color=" + hex(colours.objectSelectedOutlineColour),
            "com_txt_color=" + hex(colours.commentTextColour),
            "out_color=" + hex(colours.outlineColour)
        };
        return pairs.joinIntoString(" ");
    }

    PropertiesPanelProperty* createProperty(Setting& setting) const
    {
        auto const label = setting.json["label"].toString();

        if (setting.type == "combo") {
            StringArray labels;
            for (auto const& option : getArray(setting.json, "options"))
                labels.add(getOptionLabel(option));
            return new PropertiesPanel::ComboComponent(label, setting.value, labels);
        }
        if (setting.type == "bool")
            return new PropertiesPanel::BoolComponent(label, setting.value, { "No", "Yes" });
        if (setting.type == "int") {
            auto const maximum = setting.json.hasProperty("max") ? static_cast<double>(setting.json["max"]) : static_cast<double>(1 << 30);
            auto* property = new PropertiesPanel::EditableComponent<int>(label, setting.value, true, static_cast<double>(setting.json["min"]), maximum);
            property->editableOnClick(false);
            return property;
        }
        if (setting.type == "string") {
            auto const browse = setting.json["browse"].toString();
            if (browse.isNotEmpty())
                return new PropertiesPanel::PathComponent(label, setting.value, browse == "file", setting.json["pattern"].toString());
            return new PropertiesPanel::EditableComponent<String>(label, setting.value);
        }
        return nullptr;
    }

    // The general section, then one section for each of the exporter's
    void buildPanel()
    {
        panel.clear();

        PropertiesArray general;

        auto* patchChooser = new PropertiesPanel::ComboComponent("Patch to export", inputPatchValue, { "Currently opened patch", "Other patch (browse)" });
        patchChooser->comboBox.setTextWhenNothingSelected("Choose a patch to export...");
        patchChooser->comboBox.setItemEnabled(1, hasOpenedPatch);
        general.add(patchChooser);

        auto* nameProperty = new PropertiesPanel::EditableComponent<String>("Project Name (optional)", projectNameValue);
        nameProperty->setInputRestrictions("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
        general.add(nameProperty);

        general.add(new PropertiesPanel::EditableComponent<String>("Project Copyright (optional)", projectCopyrightValue));

        for (auto* property : general)
            property->setPreferredHeight(28);

        panel.addSection("General", general);

        for (auto const& section : getArray(definition.json, "sections")) {
            PropertiesArray properties;
            for (auto const& json : getArray(section, "settings")) {
                auto* setting = findSetting(json["id"].toString());
                if (auto* property = setting ? createProperty(*setting) : nullptr) {
                    property->setPreferredHeight(28);
                    setting->property = property;
                    properties.add(property);
                }
            }
            panel.addSection(section["name"].toString(), properties);
        }
    }

    void updateEnablement()
    {
        for (auto* setting : settings) {
            if (setting->property)
                setting->property->setEnabled(isEnabled(*setting));
        }
    }

    void updateExportButton()
    {
        static char const* const actionNames[] = { "Export", "Flash", "Bootloader" };
        exportButton.setEnabled(canPerformExport());
        exportButton.setButtonText(actionNames[getExportAction()]);
    }

    static inline SmallArray<File> tempFilesToDelete;

    Definition definition;
    PropertiesPanel panel;
    ChildProcess process;
    ExportingProgressView* exportingView;
    PluginEditor* editor;
    Label unsavedLabel = Label("", "Warning: patch has unsaved changes");

    File openedPatchFile;
    bool hasOpenedPatch = false;
    bool canvasDirty = false;
    bool isTempFile = false;

    AtomicValue<bool> cancelled = false;

    // Keeps a cancel from killing the process while the export thread swaps in the next one
    CriticalSection processLock;
};
