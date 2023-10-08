#include "Config.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Standalone/PlugDataWindow.h"

#if PLUGDATA_FX
char const* ProjectInfo::projectName = "plugdata-fx";
#elif PLUGDATA_MIDI
char const* ProjectInfo::projectName = "plugdata-midi";
#else
char const* ProjectInfo::projectName = "plugdata";
#endif

#if PLUGDATA_STANDALONE
bool ProjectInfo::isStandalone = true;
#else
bool ProjectInfo::isStandalone = false;
#endif

MidiDeviceManager* ProjectInfo::getMidiDeviceManager()
{
#if PLUGDATA_STANDALONE
    if (auto* standalone = StandalonePluginHolder::getInstance())
        return &standalone->player.midiDeviceManager;

    return nullptr;
#else
    return nullptr;
#endif
}

juce::AudioDeviceManager* ProjectInfo::getDeviceManager()
{
#if PLUGDATA_STANDALONE
    if (auto* standalone = StandalonePluginHolder::getInstance())
        return &standalone->deviceManager;

    return nullptr;
#else
    return nullptr;
#endif
}

PlugDataWindow* ProjectInfo::createNewWindow(PluginEditor* editor)
{
#if PLUGDATA_STANDALONE
    return new PlugDataWindow(editor);
#else
    return nullptr;
#endif
}

bool ProjectInfo::canUseSemiTransparentWindows()
{
#if !JUCE_MAC || PLUGDATA_STANDALONE
    return Desktop::canUseSemiTransparentWindows();
#else

    // Apple's plugin hosts will show an ugly pink edge below transparent edges
    // This is a "security feature" and will probably not be removed anytime soon

    auto hostType = PluginHostType();
    if (hostType.isLogic() || hostType.isGarageBand() || hostType.isMainStage()) {
        return false;
    }

    return Desktop::canUseSemiTransparentWindows();
#endif
}
