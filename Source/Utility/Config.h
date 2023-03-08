#pragma once
#define JUCE_DISPLAY_SPLASH_SCREEN 0

// uncomment to display repaint areas
// #define JUCE_ENABLE_REPAINT_DEBUGGING 1

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



