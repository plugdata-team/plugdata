/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "../Libraries/tinydir/tinydir.h"

// Fast dir iterator class based on tinydir
// From my tests, this is about 2x faster than JUCE's RangedDirectoryIterator
// std::filesystem would be even faster, but unfortunately it doesn't work on old macOS and old GCC
class FastDirectoryIterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = File;
    using reference         = File;
    using pointer           = void;
    using iterator_category = std::input_iterator_tag;

    // The default-constructed iterator acts as the 'end' sentinel.
    FastDirectoryIterator() = default;

    FastDirectoryIterator (const File& directory)
    {
        if(!directory.exists()) return;
        
        auto filePath = directory.getFullPathName();
        tinydir_open(&dir, filePath.toRawUTF8());
        tinydir_next(&dir); // Skip ".." entry
        tinydir_readfile(&dir, &file);
        currentFile = File(String::fromUTF8(file.path)).getChildFile(file.name);
        
    }

    // Returns true if both iterators are in their end/sentinel state, otherwise returns false.
    bool operator== (const FastDirectoryIterator& other) const noexcept
    {
        return !dir.has_next && !other.currentFile.exists();
    }

    // Returns the inverse of operator==
    bool operator!= (const FastDirectoryIterator& other) const noexcept
    {
        return ! operator== (other);
    }

    // Return an object containing metadata about the file or folder to
    // which the iterator is currently pointing.
    const File& operator* () const noexcept {
        return currentFile;
        
    }
    const File* operator->() const noexcept {

        return &currentFile;
    }

    // Moves the iterator along to the next file
    FastDirectoryIterator& operator++()
    {
        next();
        return *this;
    }

    // Moves the iterator along to the next file.
    File operator++ (int)
    {
        auto result = *(*this);
        ++(*this);
        return result;
    }
    
    static Array<File> recurse(const File& directory);
    
private:
    bool next()
    {
        if(dir.has_next)
        {
            tinydir_next(&dir);
            tinydir_readfile(&dir, &file);
            currentFile = File(String::fromUTF8(file.path));
            return true;
        }
        
        currentFile = File();
        tinydir_close(&dir);
        
        return false;
    }
    
    
    tinydir_file file;
    tinydir_dir dir;
    
    File currentFile;
};

inline FastDirectoryIterator begin(const FastDirectoryIterator& it) { return it; }

inline FastDirectoryIterator end(const FastDirectoryIterator&) { return {}; }

inline Array<File> FastDirectoryIterator::recurse(const File& directory)
{
    Array<File> result;
    for(auto file : FastDirectoryIterator(directory))
    {
        if(file.isDirectory() && file != directory)
        {
            result.addArray(recurse(file));
        }
        else {
            result.add(file);
        }
    }
    
    return result;
}
