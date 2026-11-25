/*
 // Copyright (c) 2026 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "../Components/ColourPicker.h"


juce::String createSlug(const juce::String& input)
{
    auto slug = input.toLowerCase();
    slug = slug.replaceCharacter(' ', '-');
    slug = slug.replaceCharacter('_', '-');
    slug = slug.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789-");

    while (slug.contains("--"))
        slug = slug.replace("--", "-");

    return slug.trimCharactersAtStart("-").trimCharactersAtEnd("-");
}


class MetaModuleExporter final : public ExporterBase {
public:
    Value brandNameValue;
    Value projectLicenseValue;
    Value versionValue;
    Value descriptionValue;
    Value emailValue;
    Value urlValue;
    Value panelColourValue;


    MetaModuleExporter(PluginEditor* editor, ExportingProgressView* exportingView)
        : ExporterBase(editor, exportingView)
    {
        PropertiesArray properties;
        properties.add(new PropertiesPanel::ColourComponent("Panel Colour", panelColourValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Brand Name (optional)", brandNameValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Project License (optional)", projectLicenseValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Version (optional)", versionValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Description (optional)", descriptionValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Email (optional)", emailValue));
        properties.add(new PropertiesPanel::EditableComponent<String>("Homepage (optional)", urlValue));

        panel.addSection("MetaModule", properties);
    }

    void getState(DynamicObject::Ptr globalState) override
    {
        auto* state = new DynamicObject();
        state->setProperty("input_patch_value", getValue<String>(inputPatchValue));
        state->setProperty("project_name_value", getValue<String>(projectNameValue));
        state->setProperty("project_copyright_value", getValue<String>(projectCopyrightValue));
        state->setProperty("brand_name_value", getValue<String>(brandNameValue));
        state->setProperty("project_license_value", getValue<String>(projectLicenseValue));
        state->setProperty("version_value", getValue<String>(versionValue));
        state->setProperty("description_value", getValue<String>(descriptionValue));
        state->setProperty("email_value", getValue<String>(emailValue));
        state->setProperty("url_value", getValue<String>(urlValue));
        state->setProperty("panel_colour_value", getValue<int>(panelColourValue));
        globalState->setProperty("meta", state);
    }

    void setState(DynamicObject::Ptr globalState) override
    {
        auto const state = globalState->getProperty("meta").getDynamicObject();
        if (!state)
            return;
        inputPatchValue = state->getProperty("input_patch_value");
        projectNameValue = state->getProperty("project_name_value");
        projectCopyrightValue = state->getProperty("project_copyright_value");
        brandNameValue = state->getProperty("brand_name_value");
        projectLicenseValue = state->getProperty("project_license_value");
        versionValue = state->getProperty("version_value");
        descriptionValue = state->getProperty("description_value");
        emailValue = state->getProperty("email_value");
        urlValue = state->getProperty("url_value");
        panelColourValue = state->getProperty("panel_colour_value");
    }

    bool performExport(String const& pdPatch, String const& outdir, String const& name, String const& copyright, StringArray const& searchPaths) override
    {
        auto const heavyPath = pathToString(heavyExecutable);

        StringArray args = { heavyPath.quoted(), pdPatch.quoted(), "-o", outdir.quoted() };

        args.add("-n" + name);

        if (copyright.isNotEmpty()) {
            args.add("--copyright");
            args.add(copyright.quoted());
        }

        auto const brandName = getValue<String>(brandNameValue);
        auto const projectLicense = getValue<String>(projectLicenseValue);
        auto const version = getValue<String>(versionValue);
        auto const description = getValue<String>(descriptionValue);
        auto const email = getValue<String>(emailValue);
        auto const url = getValue<String>(urlValue);
        auto const panelColourInt = getValue<int>(panelColourValue);
        Colour panelColour(panelColourInt);

        DynamicObject::Ptr const metaJson(new DynamicObject());
        var const metaMeta(new DynamicObject());
        metaMeta.getDynamicObject()->setProperty("slug", createSlug(name));
        metaMeta.getDynamicObject()->setProperty("name", name);
        if (brandName.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("brand", brandName);
            metaMeta.getDynamicObject()->setProperty("author", brandName);
        if (projectLicense.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("license", projectLicense);
        if (version.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("version", version);
        if (description.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("description", description);
        if (email.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("email", email);
        if (url.isNotEmpty())
            metaMeta.getDynamicObject()->setProperty("url", url);

        var const metaPanel(new DynamicObject());
        metaPanel.getDynamicObject()->setProperty("color", "#" + panelColour.toDisplayString(false));
        var const metaAssets(new DynamicObject());
        metaAssets.getDynamicObject()->setProperty("panel", metaPanel);

        var const metaPlugin(new DynamicObject());
        metaPlugin.getDynamicObject()->setProperty("slug", createSlug(name));
        metaPlugin.getDynamicObject()->setProperty("name", name);
        if (description.isNotEmpty())
            metaPlugin.getDynamicObject()->setProperty("description", description);
        metaPlugin.getDynamicObject()->setProperty("assets", metaAssets);

        metaMeta.getDynamicObject()->setProperty("modules", Array(metaPlugin));

        metaJson->setProperty("meta", metaMeta);

        auto const metaJsonFile = createMetaJson(metaJson);
        args.add("-m" + pathToString(metaJsonFile));

        args.add("-v");
        args.add("-gmeta");

        args.add("-p");
        for (auto& path : searchPaths) {
            args.add(path);
        }

        if (shouldQuit)
            return true;

        auto const command = args.joinIntoString(" ");
        startShellScript(command);

        waitForProcessToFinish(-1);
        exportingView->flushConsole();

        if (shouldQuit)
            return true;

        auto const outputFile = File(outdir);
        outputFile.getChildFile("ir").deleteRecursively();
        outputFile.getChildFile("hv").deleteRecursively();
        outputFile.getChildFile("c").deleteRecursively();

        metaJsonFile.copyFileTo(outputFile.getChildFile("meta.json"));

        // Delay to get correct exit code
        Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 300);

        return getExitCode();
    }
};
