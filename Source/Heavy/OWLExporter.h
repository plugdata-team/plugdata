#pragma once
/*
 // Copyright (c) 2024 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class OWLExporter final : public ExporterBase {
public:
    Value targetBoardValue = Value(var(2));
    Value storeSlotValue = SynchronousValue(var(1));

    PropertiesPanelProperty* storeSlotProperty;

    OWLExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        exportTypeValue = var(3);

        Array<PropertiesPanelProperty*> properties;
        properties.add(new PropertiesPanel::ComboComponent("Target board", targetBoardValue, { "OWL1", "OWL2", "OWL3" }));
        properties.add(new PropertiesPanel::ComboComponent("Export type", exportTypeValue, getExportTypes()));
        storeSlotProperty = new PropertiesPanel::ComboComponent(
            "Store slot", storeSlotValue, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F" });
        properties.add(storeSlotProperty);

        for (auto* property : properties) {
            property->setPreferredHeight(28);
        }

        panel.addSection("OWL", properties);

        targetBoardValue.addListener(this);
        exportTypeValue.addListener(this);
        storeSlotValue.addListener(this);
    }

    StringArray getExportTypes() const override
    {
        return { "Source code", "Binary", "Load", "Store" };
    }

    ExportAction getExportAction() const override
    {
        int const exportType = getExportType();
        return exportType == 3 || exportType == 4 ? Flash : Export;
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("export_type_value", getValue<int>(exportTypeValue));
        state->setProperty("target_board_value", getValue<int>(targetBoardValue));
        state->setProperty("store_slot_value", getValue<int>(storeSlotValue));
        globalState->setProperty("owl", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("owl").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        exportTypeValue = state->getProperty("export_type_value");
        targetBoardValue = state->getProperty("target_board_value");
        storeSlotValue = state->getProperty("store_slot_value");
    }

    void valueChanged(Value& v) override
    {
        ExporterBase::valueChanged(v);

        storeSlotProperty->setEnabled(getValue<int>(exportTypeValue) == 4);
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto const exportType = getExportType();
        auto const mode = exportType == 1 ? "source" : exportType == 3 ? "load" : exportType == 4 ? "store" : "build";
        return runExportScript("owl", pdPatch, outdir, name, copyright, searchPaths,
            { "--mode", mode, "--board", "OWL" + String(getValue<int>(targetBoardValue)),
                "--slot", String(getValue<int>(storeSlotValue)) });
    }
};
