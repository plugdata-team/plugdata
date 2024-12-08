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

    PatchInfo() = default;

    PatchInfo(var const& jsonData)
    {
        title = jsonData["Title"];
        author = jsonData["Author"];
        releaseDate = jsonData["Release date"];
        download = jsonData["Download"];
        description = jsonData["Description"];
        price = jsonData["Price"];
        thumbnailUrl = jsonData["StoreThumb"];
        version = jsonData["Version"];
        json = JSON::toString(jsonData, false);
    }

    bool isPatchArchive() const
    {
        auto fileName = URL(download).getFileName();
        return fileName.endsWith(".zip") || fileName.endsWith(".plugdata");
    }

    String getNameInPatchFolder() const
    {
        return title.toLowerCase().replace(" ", "-") + "-" + String::toHexString(hash(author));
    }

    bool isPatchInstalled() const
    {
        auto patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");

        for (auto &file: OSUtils::iterateDirectory(patchesFolder, false, false)) {
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
        auto patchesFolder = ProjectInfo::appDataDir.getChildFile("Patches");

        for (auto &file: OSUtils::iterateDirectory(patchesFolder, false, false)) {
            if (OSUtils::isDirectoryFast(file.getFullPathName())) {
                auto patchFileName = getNameInPatchFolder();
                
                if (file.getFileName() == patchFileName) {
                    auto metaFile = file.getChildFile("meta.json");
                    if(metaFile.existsAsFile())
                    {
                        return JSON::parse(metaFile)["Version"].toString() != version;
                    }
                }
            }
        }
        return false;
    }
};
