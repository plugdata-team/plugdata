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

#if PLUGDATA_FX
bool ProjectInfo::isFx = true;
#else
bool ProjectInfo::isFx = false;
#endif

MidiDeviceManager* ProjectInfo::getMidiDeviceManager()
{
#if PLUGDATA_STANDALONE
    if (auto* standalone = getStandalonePluginHolder())
        return &standalone->player.midiDeviceManager;

    return nullptr;
#else
    return nullptr;
#endif
}

juce::AudioDeviceManager* ProjectInfo::getDeviceManager()
{
#if PLUGDATA_STANDALONE
    if (auto* standalone = getStandalonePluginHolder())
        return &standalone->deviceManager;

    return nullptr;
#else
    return nullptr;
#endif
}

PlugDataWindow* ProjectInfo::createNewWindow(PluginEditor* editor)
{
#if PLUGDATA_STANDALONE
    return new PlugDataWindow(reinterpret_cast<AudioProcessorEditor*>(editor));
#else
    ignoreUnused(editor);
    return nullptr;
#endif
}

void ProjectInfo::closeWindow(PlugDataWindow* window)
{
#if PLUGDATA_STANDALONE
    delete window;
#else
    ignoreUnused(window);
#endif
}

bool ProjectInfo::canUseSemiTransparentWindows()
{
#if JUCE_IOS
    return isStandalone;
#endif
#if !JUCE_MAC || PLUGDATA_STANDALONE
    return Desktop::canUseSemiTransparentWindows();
#else
    return Desktop::canUseSemiTransparentWindows();
#endif
}

bool ProjectInfo::isMidiEffect() noexcept
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

StandalonePluginHolder* ProjectInfo::getStandalonePluginHolder()
{
#if PLUGDATA_STANDALONE
    return StandalonePluginHolder::getInstance();
#else
    return nullptr;
#endif
}
