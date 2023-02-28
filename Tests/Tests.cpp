#include <catch2/catch_all.hpp>

// Workaround for naming issue on windows
#include <juce_graphics/juce_graphics.h>
#define Rectangle juce::Rectangle

#include <PluginProcessor.h>


#include <juce_core/system/juce_TargetPlatform.h>
#include <Standalone/PlugDataApp.cpp>

#if JUCE_MAC
extern void stopLoop();
#endif

extern juce::JUCEApplicationBase* juce_CreateApplication();

#define StartApplication juce::JUCEApplicationBase::createInstance = &::juce_CreateApplication; \
                         juce::ScopedJuceInitialiser_GUI gui; \
                         PlugDataApp app; \
                         app.initialise(""); \
                         auto editor = dynamic_cast<PluginEditor*>(app.getWindow()->getContentComponent()->getChildComponent(0))

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
    
    CHECK_THAT(editor->pd->getName().toStdString(),
               Catch::Matchers::Equals("PlugData"));
    
    StopApplicationAfter(500);

}

TEST_CASE("Create and delete objects", "[name]")
{
    StartApplication;
    
    MessageManager::callAsync([=](){
        
       
        auto* obj1 = editor->getCurrentCanvas()->patch.createObject("metro 200", 200, 500);
        auto* obj2 = editor->getCurrentCanvas()->patch.createObject("tgl", 300, 700);
        
        editor->getCurrentCanvas()->synchronise();
        
        REQUIRE(editor->getCurrentCanvas()->objects.getFirst()->getObjectBounds().getPosition() == Point<int>(200, 500));
        
        REQUIRE(editor->getCurrentCanvas()->objects[0]->getPointer() == obj1);
        REQUIRE(editor->getCurrentCanvas()->objects[1]->getPointer() == obj2);
        
        editor->getCurrentCanvas()->patch.removeObject(obj1);
        editor->getCurrentCanvas()->patch.removeObject(obj2);
        
        editor->getCurrentCanvas()->synchronise();
        
        REQUIRE(editor->getCurrentCanvas()->objects.size() == 0);
    
    });
    
    StopApplicationAfter(1500);
}
