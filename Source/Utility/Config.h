#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace juce {
class AudioDeviceManager;
}
using namespace juce;

struct ProjectInfo {

    static char const* projectName;
    static bool isStandalone;

    static inline char const* companyName = "plugdata";
    static inline char const* versionString = PLUGDATA_VERSION;
    static inline int const versionNumber = 0x800;

    static AudioDeviceManager* getDeviceManager();

    static bool canUseSemiTransparentWindows();
};

template<typename T>
inline T getValue(Value const& v)
{
    return static_cast<T>(v.getValue());
}
