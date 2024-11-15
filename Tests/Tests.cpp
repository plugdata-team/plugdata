#include <juce_gui_basics/juce_gui_basics.h>
#include <random>

#include "Utility/Config.h"
#include "Utility/OSUtils.h"
#include "Sidebar/Sidebar.h" // So we can read and clear the console
#include "Objects/ObjectBase.h" // So we can interact with object GUIs
#include "PluginEditor.h"

String loggedErrors;

void clickNextObject(Component::SafePointer<Canvas> cnv, std::vector<Object*>& objects, std::function<void()> done)
{
    if(!objects.size() || !cnv) {
        done();
        return;
    }
    
    auto* peer = cnv->editor->getTopLevelComponent()->getPeer();
    auto* obj = objects.back();
    objects.pop_back();
    
    auto& peerComponent = peer->getComponent();
    auto pos = peerComponent.getLocalPoint(nullptr, obj->getScreenBounds().getCentre().toFloat());
    auto viewportBounds = peerComponent.getLocalArea(nullptr, cnv->viewport->getScreenBounds().toFloat()).reduced(8);
    
    if(viewportBounds.contains(pos)) {
        peer->handleMouseEvent(MouseInputSource::InputSourceType::mouse, pos, ModifierKeys::leftButtonModifier, 0.0f, 0.0f, Time::getMillisecondCounter());
        peer->handleMouseEvent(MouseInputSource::InputSourceType::mouse, pos, ModifierKeys::noModifiers, 0.0f, 0.0f, Time::getMillisecondCounter());
    }
    
    cnv->pd->volume->store(0.0f);
    
    if(objects.empty())
    {
        Timer::callAfterDelay(140, [done](){
            done();
        });
        return;
    }
    
    Timer::callAfterDelay(20, [cnv, objects, done]() mutable {
        if(cnv) {
            clickNextObject(cnv, objects, done);
        }
        else {
            done();
        }
    });
}

t_symbol* generateRandomString(size_t length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    std::default_random_engine generator(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
    for (size_t i = 0; i < length; ++i)
    {
        result += charset[dist(generator)];
    }
    return gensym(result.c_str());
}

// Generate a random float
float generateRandomFloat()
{
    return static_cast<float>(rand()) / RAND_MAX * 1000.0f;
}

// Generate a random pd::Atom based on type
pd::Atom generateAtomForType(const std::string& type)
{
    if (type == "<float>")
    {
        return pd::Atom(generateRandomFloat());
    }
    else if (type == "<symbol>")
    {
        return pd::Atom(generateRandomString(10));
    }
    else if (type == "<int>")
    {
        return pd::Atom(static_cast<int>(rand() % 1000)); // Random integer
    }
    else if (type == "<bool>")
    {
        return pd::Atom(static_cast<int>(rand() % 2)); // Random boolean (as 0 or 1)
    }
    else
    {
        // Default to a random string for unrecognized types
        return pd::Atom(generateRandomString(10));
    }
}

void fuzzObject(Component::SafePointer<Canvas> cnv, std::vector<Object*>& objects)
{
    for (auto& object : objects)
    {
        auto info = cnv->pd->objectLibrary->getObjectInfo(object->getType());
        auto methods = info.getChildWithName("methods");

        for (auto method : methods)
        {
            auto methodName = method.getProperty("type").toString().toStdString();
            auto description = method.getProperty("description").toString();
            std::cout << "Fuzzing method: " << methodName << " (" << description << ")" << std::endl;

            auto nameWithoutArgs = String(methodName).upToFirstOccurrenceOf("<", false, false).trim();
            // Extract expected argument types from the method notation
            std::vector<std::string> expectedArgTypes;
            size_t pos = 0;
            while ((pos = methodName.find('<', pos)) != std::string::npos)
            {
                size_t end = methodName.find('>', pos + 1);
                if (end != std::string::npos)
                {
                    expectedArgTypes.push_back(methodName.substr(pos, end - pos + 1));
                    pos = end + 1;
                }
            }

            // Generate a vector of pd::Atom arguments based on expected types
            SmallArray<pd::Atom> args;
            for (const auto& argType : expectedArgTypes)
            {
                args.push_back(generateAtomForType(argType));
            }

            // Log the generated arguments for debugging
            std::cout << "Generated arguments: ";
            for (const auto& arg : args)
            {
                if (arg.isFloat())
                    std::cout << arg.getFloat() << " ";
                else if (arg.isSymbol())
                    std::cout << arg.getSymbol() << " ";
                else
                    std::cout << "<unknown> ";
            }
            std::cout << std::endl;

            // Send the fuzzed arguments to the object
            cnv->editor->pd->sendDirectMessage(object->getPointer(), SmallString(nameWithoutArgs), std::move(args));
        }
    }
}

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

    auto* peer = editor->getTopLevelComponent()->getPeer();
    // Click everything
    cnv->locked.setValue(true);
    editor->pd->volume->store(0.0f);
    
    std::vector<Object*> objects;
    for(auto* obj : cnv->objects)
    {
        objects.push_back(obj);
    }
    
    fuzzObject(cnv, objects);
    
    /*
    clickNextObject(cnv, objects, [&tabbar, &helpFiles](){
        while(auto* cnv = tabbar.getCurrentCanvas()) { // TODO: why is this faster than closeAllTabs?()
            tabbar.closeTab(cnv);
        }
        openHelpfilesRecursively(tabbar, helpFiles);
    }); */
    
    // Go into all the subpatches that were opened by clicking, and click everything again
    /*
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

    Timer::callAfterDelay(200, [pd, editor, helpFile, &helpFiles, tabbar = Component::SafePointer(&tabbar)]() mutable {
        /*
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
        } */
        //editor->sidebar->clearConsole();

        editor->pd->volume->store(0.0f);
        
        if(tabbar) {
            while(auto* cnv = tabbar->getCurrentCanvas()) { // TODO: why is this faster than closeAllTabs?()
                tabbar->closeTab(cnv);
            }
            openHelpfilesRecursively(*tabbar, helpFiles);
        }
    });
}

void helpPatchToImage(TabComponent& tabbar, File file, String lib)
{
    auto outputFileDir = ProjectInfo::appDataDir.getChildFile("HelpFileImages");
    Image helpFileImage;

    auto* cnv = tabbar.openPatch(URL(file));
    tabbar.handleUpdateNowIfNeeded();

    cnv->jumpToOrigin();

    cnv->editor->nvgSurface.invalidateAll();
    cnv->editor->nvgSurface.render();
    cnv->editor->nvgSurface.renderFrameToImage(helpFileImage, cnv->patch.getBounds().withZeroOrigin());

    auto outputDir = outputFileDir.getChildFile(lib);
    outputDir.createDirectory();

    auto outputFile = outputDir.getChildFile(file.getFileNameWithoutExtension().upToLastOccurrenceOf("-help", false, false) + ".png");

    PNGImageFormat imageFormat;
    FileOutputStream ostream(outputFile);
    imageFormat.writeImageToStream(helpFileImage, ostream);

    tabbar.closeTab(cnv);
}

void exportHelpFileImages(TabComponent& tabbar, File outputFileDir)
{
    if(!outputFileDir.isDirectory()) outputFileDir.createDirectory();

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/9.else"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "else");
        }
    }

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/10.cyclone"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "cyclone");
        }
    }

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/5.reference"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "vanilla");
        }
    }
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
    
    editor->getTopLevelComponent()->getPeer()->setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea, false);

    allHelpfiles.erase(allHelpfiles.end() - 563, allHelpfiles.end());

    //exportHelpFileImages(tabbar, File("/Users/timschoen/Projecten/plugdata/Tests/Help"));
    openHelpfilesRecursively(tabbar, allHelpfiles);
}
