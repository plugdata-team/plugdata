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
    int64 installTime;

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
        if (jsonData.hasProperty("InstallTime")) {
            installTime = static_cast<int64>(jsonData["InstallTime"]);
        } else {
            installTime = 0;
        }
        json = JSON::toString(jsonData, false);
    }

    void setInstallTime(int64 const millisSinceEpoch)
    {
        auto const jsonData = JSON::fromString(json);
        if (auto* obj = jsonData.getDynamicObject()) {
            obj->setProperty("InstallTime", millisSinceEpoch);
            json = JSON::toString(jsonData, false);
        }
    }

    bool isPatchArchive() const
    {
        auto const fileName = URL(download).getFileName();
        return fileName.endsWith(".zip") || fileName.endsWith(".plugdata");
    }

    String getNameInPatchFolder() const
    {
        return title.toLowerCase().replace(" ", "-") + "-" + String::toHexString(hash(author) + hash(version));
    }

    bool isPatchInstalled() const
    {
        auto const patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");

        for (auto& file : OSUtils::iterateDirectory(patchesFolder, false, false)) {
            if (OSUtils::isDirectoryFast(file.getFullPathName())) {
                auto patchFileName = getNameInPatchFolder();
                if (file.getFileName() == patchFileName) {
                    return true;
                }
            }
        }
        return false;
    }

    bool updateAvailable() const
    {
        auto const patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");

        for (auto& file : OSUtils::iterateDirectory(patchesFolder, false, false)) {
            if (OSUtils::isDirectoryFast(file.getFullPathName())) {
                auto patchFileName = getNameInPatchFolder();

                if (file.getFileName() == patchFileName) {
                    auto metaFile = file.getChildFile("meta.json");
                    if (metaFile.existsAsFile() && version.isNotEmpty()) {
                        return JSON::parse(metaFile)["Version"].toString() != version;
                    }
                }
            }
        }
        return false;
    }
};
