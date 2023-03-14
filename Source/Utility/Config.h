#pragma once

namespace juce
{
    class AudioDeviceManager;
}
using namespace juce;

struct ProjectInfo {

    static const char* projectName;
    static bool isStandalone;

    static inline const char* companyName = "plugdata";
    static inline const char* versionString = PLUGDATA_VERSION;
    static inline const int versionNumber = 0x700;
    
    static AudioDeviceManager* getDeviceManager();
};



