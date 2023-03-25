/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "../Libraries/cpath/cpath.h"


inline Array<File> iterateDirectoryRecurse(cpath::Dir&& dir, bool recursive, bool onlyFiles)
{
    Array<File> result;
    
    while (cpath::Opt<cpath::File, cpath::Error::Type> file = dir.GetNextFile()) {
        auto isDir = file->IsDir();
        
        if(isDir && recursive && !file->IsSpecialHardLink()) {
            iterateDirectoryRecurse(std::move(file->ToDir().GetRaw()), recursive, onlyFiles);
        }
        if((isDir && !onlyFiles) || !isDir) {
            result.add(File(String(file->Path().GetRawPath()->buf)));
        }
    }
    
    return result;
}

inline Array<File> iterateDirectory(const File& directory, bool recursive, bool onlyFiles)
{
    auto pathName = directory.getFullPathName();
    auto dir = cpath::Dir(pathName.toRawUTF8());
    return iterateDirectoryRecurse(std::move(dir), recursive, onlyFiles);
    
}
