//
// Created by alexw on 5/12/2024.
//

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

    bool isPatchInstalled()
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
};