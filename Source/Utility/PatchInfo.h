/*
 // Copyright (c) 2024 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class PatchInfo {
public:
    String title;
    String author;
    String releaseDate;
    String download;
    String description;
    String price;
    String thumbnailUrl;
    String size;
    String json;
    String version;
    String folderOverride;

    PatchInfo() = default;

    explicit PatchInfo(var const& jsonData)
    {
        title = jsonData["Title"];
        author = jsonData["Author"];
        releaseDate = jsonData["Release date"];
        download = jsonData["Download"];
        description = jsonData["Description"];
        price = jsonData["Price"];
        thumbnailUrl = jsonData["StoreThumb"];
        version = jsonData["Version"];
        if (jsonData.hasProperty("FolderName")) {
            folderOverride = jsonData["FolderName"];
        }

        json = JSON::toString(jsonData, false);
    }

    bool isPatchArchive() const
    {
        auto const fileName = URL(download).getFileName();
        return fileName.endsWith(".zip") || fileName.endsWith(".plugdata");
    }

    String getNameInPatchFolder() const
    {
        if (folderOverride.isNotEmpty()) {
            return folderOverride;
        }

        return title.toLowerCase().replace(" ", "-") + "-" + String::toHexString(hash(author) + hash(version));
    }

    bool isPatchInstalled() const
    {
        updateInstalledPatches();

        for (auto& patch : installedPatches)
            if (patch["Title"] == title && patch["Author"] == author)
                return true;

        return false;
    }

    bool updateAvailable() const
    {
        if (!isPatchInstalled())
            return false;

        for (auto& patch : installedPatches)
            if (patch["Title"] == title && patch["Author"] == author)
                if (patch["Version"].toString() == version)
                    return false;

        return true;
    }

    static void updateInstalledPatches()
    {
        auto patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");
        auto lastChanged = patchesFolder.getLastModificationTime();

        if (lastChanged != lastUpdate) {
            installedPatches.clear();

            for (auto& file : OSUtils::iterateDirectory(patchesFolder, false, false)) {
                if (OSUtils::isDirectoryFast(file.getFullPathName())) {
                    auto metaFile = file.getChildFile("meta.json");
                    if (metaFile.existsAsFile()) {
                        installedPatches.add(JSON::parse(metaFile));
                    }
                }
            }
            lastUpdate = lastChanged;
        }
    }

    static inline Time lastUpdate;
    static inline SmallArray<var> installedPatches;
};
