/*
 // Copyright (c) 2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


#pragma once
#include <fstream>
#include <xz/src/liblzma/api/lzma.h>

#if (defined(__cpp_lib_filesystem) || __has_include(<filesystem>)) && (!defined(__APPLE__) || __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 101500)
#    include <filesystem>
namespace fs = std::filesystem;
#elif defined(__cpp_lib_experimental_filesystem)
#    include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#    include <ghc_filesystem/include/ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

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

    static std::pair<std::string, std::string> parsePaxPathAndLink(const uint8_t* data, size_t size) {
        std::string path, linkpath;
        std::string paxData(reinterpret_cast<const char*>(data), size);
        std::istringstream stream(paxData);
        std::string line;
        
        while (std::getline(stream, line, '\n')) {
            if (line.empty()) continue;
            
            // Find the first space to separate length from key=value
            size_t spacePos = line.find(' ');
            if (spacePos == std::string::npos) continue;
            
            // Extract the key=value part
            std::string keyValue = line.substr(spacePos + 1);
            
            // Find the '=' separator
            size_t equalPos = keyValue.find('=');
            if (equalPos == std::string::npos) continue;
            
            std::string key = keyValue.substr(0, equalPos);
            std::string value = keyValue.substr(equalPos + 1);
            
            if (key == "path") {
                path = value;
            } else if (key == "linkpath") {
                linkpath = value;
            }
        }
        
        return {path, linkpath};
    }

    static bool extractTar(const uint8_t* data, size_t size, const juce::File& destRoot)
    {
        // Convert destination root to std::filesystem::path
        std::filesystem::path destPath(destRoot.getFullPathName().toStdString());
        
        size_t offset = 0;
        std::string longLinkName; // For GNU tar @@LongLink entries
        std::string paxPath, paxLinkPath; // For Pax extended headers
        
        while (offset + 512 <= size) {
            const uint8_t* header = data + offset;
            if (header[0] == '\0') break; // End of archive
            
            // Get file name - handle both name and prefix fields for long paths
            std::string name;
            
            if (!longLinkName.empty()) {
                // Use the long name from previous @@LongLink entry
                name = longLinkName;
                longLinkName.clear();
            } else if (!paxPath.empty()) {
                // Use the path from previous Pax header
                name = paxPath;
                paxPath.clear();
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
            
            // Get file permissions
            std::filesystem::perms permissions = std::filesystem::perms::none;
            mode_t mode = static_cast<mode_t>(
                std::strtoul(reinterpret_cast<const char*>(header + 100), nullptr, 8)
            );
            
            // Convert mode to std::filesystem::perms
            if (mode & 0400) permissions |= std::filesystem::perms::owner_read;
            if (mode & 0200) permissions |= std::filesystem::perms::owner_write;
            if (mode & 0100) permissions |= std::filesystem::perms::owner_exec;
            if (mode & 0040) permissions |= std::filesystem::perms::group_read;
            if (mode & 0020) permissions |= std::filesystem::perms::group_write;
            if (mode & 0010) permissions |= std::filesystem::perms::group_exec;
            if (mode & 0004) permissions |= std::filesystem::perms::others_read;
            if (mode & 0002) permissions |= std::filesystem::perms::others_write;
            if (mode & 0001) permissions |= std::filesystem::perms::others_exec;
            
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
                // Parse path and linkpath from Pax header
                size_t fileOffset = offset + 512;
                if (fileOffset + fileSize <= size) {
                    auto [path, linkpath] = parsePaxPathAndLink(data + fileOffset, fileSize);
                    paxPath = path;
                    paxLinkPath = linkpath;
                }
                // Skip this entry and continue
                size_t totalEntrySize = 512 + ((fileSize + 511) & ~511);
                offset += totalEntrySize;
                continue;
            }
            
            std::filesystem::path outPath = destPath / name;
            
            try {
                if (typeFlag == '5') {
                    // Directory
                    std::filesystem::create_directories(outPath);
                    std::filesystem::permissions(outPath, permissions);
                } else if (typeFlag == '0' || typeFlag == '\0') {
                    // Regular file
                    std::filesystem::create_directories(outPath.parent_path());
                    
                    std::ofstream out(outPath, std::ios::binary);
                    if (!out) {
                        return false;
                    }
                    
                    size_t fileOffset = offset + 512;
                    out.write(reinterpret_cast<const char*>(data + fileOffset), fileSize);
                    
                    if (!out.good()) {
                        out.close();
                        std::filesystem::remove(outPath); // cleanup partial file
                        return false;
                    }
                    out.close();
                    
                    std::filesystem::permissions(outPath, permissions);
                }
            } catch (const std::filesystem::filesystem_error& e) {
                // Handle filesystem errors
                return false;
            }
            
            // Clear pax linkpath after use (path is already cleared above)
            paxLinkPath.clear();
            
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
