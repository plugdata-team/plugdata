#include <PluginProcessor.h>
#include <catch2/catch_all.hpp>

#include <juce_core/system/juce_TargetPlatform.h>
#include <Standalone/PlugDataApp.cpp>

extern void stopLoop();
extern juce::JUCEApplicationBase* juce_CreateApplication();

#define StartApplication juce::JUCEApplicationBase::createInstance = &::juce_CreateApplication; \
                         juce::ScopedJuceInitialiser_GUI gui; \
                         PlugDataApp app; \
                         app.initialise(""); \
                         auto editor = dynamic_cast<PlugDataPluginEditor*>(app.getWindow()->getContentComponent()->getChildComponent(0))

// On Mac, restarting the message manager causes a problem with the NSEvent loop
// I've fixed this with some obj-c++ code
#if JUCE_MAC
#define StopApplicationAfter(MS)     Timer::callAfterDelay(MS, [&app](){ \
                                app.quit(); \
                            }); \
                            MessageManager::getInstance()->runDispatchLoop(); \
                            stopLoop();
#else
#define StopApplicationAfter(MS)     Timer::callAfterDelay(MS, [&app](){ \
                                app.quit(); \
                            }); \
                            MessageManager::getInstance()->runDispatchLoop();
#endif


TEST_CASE("Plugin instance name", "[name]")
{
    StartApplication;
    
    CHECK_THAT(editor->pd.getName().toStdString(),
               Catch::Matchers::Equals("PlugData"));
    
    

    
    StopApplicationAfter(500);
}

TEST_CASE("Create Object", "[name]")
{
    StartApplication;
    
    editor->getCurrentCanvas()->patch.createObject("metro 200", 200, 500);
    editor->getCurrentCanvas()->patch.createObject("tgl", 300, 700);
    
    StopApplicationAfter(500);
}
