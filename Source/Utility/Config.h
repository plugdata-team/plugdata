#pragma once

namespace juce {
class AudioDeviceManager;
}
using namespace juce;

struct ProjectInfo {

    static char const* projectName;
    static bool isStandalone;

    static inline char const* companyName = "plugdata";
    static inline char const* versionString = PLUGDATA_VERSION;
    static inline int const versionNumber = 0x700;

    static AudioDeviceManager* getDeviceManager();
};
