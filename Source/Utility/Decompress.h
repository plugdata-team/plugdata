/*
 // Copyright (c) 2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


#pragma once
#include <fstream>
#include <xz/src/liblzma/api/lzma.h>

struct Decompress
{

static bool extractXz(uint8_t const* data, int dataSize, HeapArray<uint8_t>& decompressedData)
{
    lzma_stream strm = LZMA_STREAM_INIT;
    if (lzma_stream_decoder(&strm, UINT64_MAX, 0) != LZMA_OK) {
        return false;
    }
    
    strm.next_in = data;
    strm.avail_in = dataSize;
    
    uint8_t buffer[8192];
    lzma_ret ret;
    
    do {
        strm.next_out = buffer;
        strm.avail_out = sizeof(buffer);
        
        ret = lzma_code(&strm, LZMA_FINISH);
        size_t written = sizeof(buffer) - strm.avail_out;
        decompressedData.insert(decompressedData.end(), buffer, buffer + written);
        
        if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            return false;
        }
    } while (ret != LZMA_STREAM_END);
    
    lzma_end(&strm);
    return true;
}

static bool extractTar(const uint8_t* data, size_t size, const File& destRoot)
{
    size_t offset = 0;
    std::string longLinkName; // For GNU tar @@LongLink entries
    
    while (offset + 512 <= size) {
        const uint8_t* header = data + offset;
        if (header[0] == '\0') break; // End of archive
        
        // Get file name - handle both name and prefix fields for long paths
        std::string name;
        
        if (!longLinkName.empty()) {
            // Use the long name from previous @@LongLink entry
            name = longLinkName;
            longLinkName.clear();
        } else {
            // Extract name from header (100 bytes at offset 0)
            char nameField[101] = {0};
            std::memcpy(nameField, header, 100);
            name = std::string(nameField);
            
            // Check if there's a prefix field (155 bytes at offset 345)
            char prefixField[156] = {0};
            std::memcpy(prefixField, header + 345, 155);
            std::string prefix(prefixField);
            
            if (!prefix.empty()) {
                name = prefix + "/" + name;
            }
        }
        
        if (name.empty()) break;
        
        // Clean up the path
        name.erase(name.find_last_not_of(" \t\n\r\f\v\0") + 1);
        
#if !JUCE_WINDOWS
        mode_t mode = static_cast<mode_t>(
            std::strtoul(reinterpret_cast<const char*>(header + 100), nullptr, 8)
        );
        bool executable = (mode & 0100) || (mode & 0010) || (mode & 0001);
#endif
        
        // Get file size (octal)
        size_t fileSize = std::strtoull(reinterpret_cast<const char*>(header + 124), nullptr, 8);
        
        // Determine type
        char typeFlag = header[156];
        
        // Handle GNU tar long link entries
        if (typeFlag == 'L') {
            // This is a @@LongLink entry - read the long filename
            size_t fileOffset = offset + 512;
            if (fileOffset + fileSize <= size) {
                longLinkName.assign(reinterpret_cast<const char*>(data + fileOffset), fileSize);
                // Remove null terminator if present
                if (!longLinkName.empty() && longLinkName.back() == '\0') {
                    longLinkName.pop_back();
                }
            }
            // Skip this entry and continue
            size_t totalEntrySize = 512 + ((fileSize + 511) & ~511);
            offset += totalEntrySize;
            continue;
        }
        
        // Handle PaxHeaders (POSIX extended headers)
        if (typeFlag == 'x' || name.find("PaxHeaders.") == 0) {
            // Skip extended header entries for now
            // (Full implementation would parse the extended attributes)
            size_t totalEntrySize = 512 + ((fileSize + 511) & ~511);
            offset += totalEntrySize;
            continue;
        }
        
        File outFile = destRoot.getChildFile(String(name));
        
        if (typeFlag == '5') {
            outFile.createDirectory();
#if !JUCE_WINDOWS
            outFile.setExecutePermission(executable);
#endif
        } else if (typeFlag == '0' || typeFlag == '\0') {
            outFile.getParentDirectory().createDirectory();
            std::ofstream out(outFile.getFullPathName().toRawUTF8(), std::ios::binary);
            size_t fileOffset = offset + 512;
            out.write(reinterpret_cast<const char*>(data + fileOffset), fileSize);
            
            if (!out.good()) {
                out.close();
                outFile.deleteFile(); // cleanup partial file
                return false;
            }
            out.close();
#if !JUCE_WINDOWS
            outFile.setExecutePermission(executable);
#endif
        }
        
        size_t totalEntrySize = 512 + ((fileSize + 511) & ~511); // pad to next 512
        offset += totalEntrySize;
    }
    return true;
}

static bool extractTarXz(uint8_t const* data, int dataSize, const File& destRoot, int expectedSize = 0)
{
    HeapArray<uint8_t> decompressedData;
    if(expectedSize > 0) {
        decompressedData.reserve(expectedSize);
    }
    // First decompress the XZ data
    if (!extractXz(data, dataSize, decompressedData)) {
        return false;
    }
    
    // Then extract the TAR archive
    return extractTar(decompressedData.data(), decompressedData.size(), destRoot);
}

};
