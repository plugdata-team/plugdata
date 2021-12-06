/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   fontaudio_ttf;
    const int            fontaudio_ttfSize = 36948;

    extern const char*   plugd_logo_png;
    const int            plugd_logo_pngSize = 12479;

    extern const char*   forkawesomewebfont_ttf;
    const int            forkawesomewebfont_ttfSize = 188756;

    extern const char*   Abstractions_zip;
    const int            Abstractions_zipSize = 370945;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 4;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
