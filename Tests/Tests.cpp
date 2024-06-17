#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/OSUtils.h"
#include "Sidebar/Sidebar.h" // So we can read and clear the console
#include "Objects/ObjectBase.h" // So we can interact with object GUIs
#include "PluginEditor.h"

String loggedErrors;

void openHelpfilesRecursively(TabComponent& tabbar, std::vector<File>& helpFiles)
{
    static int numProcessed = 0;
    
    if(helpFiles.empty())
    {
        std::cout << "TEST COMPLETED SUCCESFULLY" << std::endl;
        ProjectInfo::appDataDir.getChildFile("console-errors.md").replaceWithText(loggedErrors);
        return;
    }

    auto helpFile = helpFiles.back();
    helpFiles.pop_back();

    std::cout << "STARTED PROCESSING: " << numProcessed++ << " " << helpFile.getFullPathName() << std::endl;

    auto* cnv = tabbar.openPatch(URL(helpFile));

    auto* pd = cnv->pd;
    auto* editor = cnv->editor;

    // Evil test that deletes the patch instantly after being created, leaving dangling pointers everywhere
    // plugdata should be able to handle that!
#define TEST_PATCH_DETACHED 0
#if TEST_PATCH_DETACHED

    std::vector<pd::WeakReference> openedPatches;
    // Close all patches
    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
        openedPatches.push_back(pd::WeakReference(cnv, pd));
    }

    pd->patches.clear();
    for(auto patch : openedPatches)
    {
        if(auto cnv = patch.get<t_glist*>()) {
            libpd_closefile(cnv.get());
        }
    }
#endif

    /*
    // Click everything
    cnv->locked.setValue(true);
    for(auto* object : cnv->objects)
    {
        auto mms = Desktop::getInstance().getMainMouseSource();
        auto pos = object->gui->getLocalBounds().getCentre().toFloat();
        auto fakeEvent = MouseEvent (mms, pos, ModifierKeys::leftButtonModifier, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, object->gui.get(), object->gui.get(), Time::getCurrentTime(), pos, Time::getCurrentTime(), 1, false);

        object->gui->mouseDown(fakeEvent);
        object->gui->mouseUp(fakeEvent);
    }

    // Go into all the subpatches that were opened by clicking, and click everything again
    for(auto* subcanvas : tabbar.getCanvases())
    {
        if(subcanvas == cnv) continue;
        subcanvas->locked.setValue(true);
        for(auto* object : subcanvas->objects)
        {
            auto mms = Desktop::getInstance().getMainMouseSource();
            auto pos = object->gui->getLocalBounds().getCentre().toFloat();
            auto fakeEvent = MouseEvent (mms, pos, ModifierKeys::leftButtonModifier, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, object->gui.get(), object->gui.get(), Time::getCurrentTime(), pos, Time::getCurrentTime(), 1, false);

            object->gui->mouseDown(fakeEvent);
            object->gui->mouseUp(fakeEvent);
        }
    } */

    Timer::callAfterDelay(30, [pd, editor, helpFile, &helpFiles, tabbar = &tabbar]() mutable {
        StringArray errors;
        auto messages = pd->getConsoleMessages();
        for(auto& [ptr, message, type, length, repeats] : messages)
        {
            if(type == 1)
            {
                errors.add(message);
            }
        }

        if(!errors.isEmpty())
        {
            loggedErrors += "\n\n\n" + helpFile.getFullPathName() + "\n--------------------------------------------------------------------------\n";
            for(auto& error : errors)
            {
                loggedErrors += error + "\n";
            }
        }
        editor->sidebar->clearConsole();

        while(auto* cnv = tabbar->getCurrentCanvas()) { // TODO: why is this faster than closeAllTabs?()
            tabbar->closeTab(cnv);
        }
        openHelpfilesRecursively(*tabbar, helpFiles);
    });
}

void runTests(PluginEditor* editor)
{
    static std::vector<File> allHelpfiles = {};
    // Open every helpfile, this will make sure it initialises and closes every object at least once (but probasbly a whole bunch of times in different contexts)
    // Run with AddressSanitizer, UBSanitizer or ThreadSanitizer to find all memory, UB and threading problems
    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            allHelpfiles.push_back(file);
        }
    }

    auto& tabbar = editor->getTabComponent();

    //allHelpfiles.erase(allHelpfiles.end() - 662, allHelpfiles.end());

    openHelpfilesRecursively(tabbar, allHelpfiles);
}
