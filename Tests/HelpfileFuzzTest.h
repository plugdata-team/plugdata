#if defined(__SANITIZE_ADDRESS__)
#    define PLUGDATA_HELPFILE_FUZZ_ASAN 1
#elif defined(__has_feature)
#    if __has_feature(address_sanitizer)
#        define PLUGDATA_HELPFILE_FUZZ_ASAN 1
#    endif
#endif

class HelpFileFuzzTest : public PlugDataUnitTest
{
public:
    HelpFileFuzzTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Helpfile Fuzz Test")
    {
    }

private:
    void perform() override
    {
        static HeapArray<File> allHelpfiles = {};

        // Set PLUGDATA_HELPFILE_FILTER to comma-separated parts of the paths you want to test
        auto const filter = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_HELPFILE_FILTER", {}), ",", "");

        // Open every helpfile, this will make sure it initialises and closes every object at least once (but probasbly a whole bunch of times in different contexts)
        // Run with AddressSanitizer, UBSanitizer or ThreadSanitizer to find all memory, UB and threading problems
        for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation"), true, true))
            
        {

#ifdef PLUGDATA_HELPFILE_FUZZ_ASAN
            // The pdlua benchmark example keeps the audio thread busy for many minutes under ASan
            if(file.getFileName() == "luajit.pd" && filter.isEmpty()) continue;
#endif
            
            if(file.hasFileExtension(".pd"))
            {
                allHelpfiles.add(file);
            }
        }

        // If you want to test a single helpfile:
        //allHelpfiles.clear();
        //allHelpfiles.emplace_back("/Users/timothy/Documents/plugdata/Documentation/...");

        if (!filter.isEmpty()) {
            allHelpfiles.remove_if([&filter](File const& file) {
                for (auto const& token : filter) {
                    if (file.getFullPathName().contains(token))
                        return false;
                }
                return true;
            });
        }

        allHelpfiles.sort();

        openHelpfilesRecursively(editor->getTabComponent(), allHelpfiles);
    }

    void openHelpfilesRecursively(TabComponent& tabbar, HeapArray<File>& helpFiles)
    {
        if(helpFiles.empty())
        {
            signalDone(true);
            return;
        }

        auto helpFile = helpFiles.back();
        helpFiles.remove_at(helpFiles.size() - 1);

        beginTest(String(helpFiles.size()) + " -> " + helpFile.getFullPathName());

        tabbar.openPatch(URL(helpFile));
        auto* cnv = tabbar.getCurrentCanvas();
        auto* editor = cnv->editor;

        // Click everything
        cnv->locked.setValue(true);
        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();

        // Clicks can delete objects (dynamic patching), so hold on to them with SafePointers
        HeapArray<Component::SafePointer<Object>> objects;
        for(auto* obj : cnv->objects)
        {
            objects.add(obj);
        }

        // Will simulate a click on all objects
        clickNextObject(cnv, objects, [this, &tabbar, &helpFiles](){
            while(auto* cnv = tabbar.getCurrentCanvas()) {
                tabbar.closeTab(cnv);
            }
            openHelpfilesRecursively(tabbar, helpFiles);
        });
    }

    static void clickNextObject(Component::SafePointer<Canvas> cnv, HeapArray<Component::SafePointer<Object>>& objects, std::function<void()> done)
    {
        if(!objects.size() || !cnv) {
            done();
            return;
        }

        auto obj = objects.back();
        objects.remove_at(objects.size() - 1);

        if(obj)
            simulateClick(obj.getComponent(), cnv.getComponent());

        cnv->pd->volume->store(0.0f);

        if(objects.empty())
        {
            Timer::callAfterDelay(3, [done](){
                done();
            });
            return;
        }

        Timer::callAfterDelay(1, [cnv, objects, done]() mutable {
            if(cnv) {
                clickNextObject(cnv, objects, done);
            }
            else {
                done();
            }
        });
    }

    static void simulateClick(Component* c, Canvas* cnv)
    {
        bool allowsMouseClicks, allowsMouseClicksOnChildren;
        c->getInterceptsMouseClicks(allowsMouseClicks, allowsMouseClicksOnChildren);

        if(allowsMouseClicks) {
            MouseEvent e(Desktop::getInstance().getMainMouseSource(),
                         c->getLocalBounds().getCentre().toFloat(), ModifierKeys::leftButtonModifier,
                         0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                         Time::getCurrentTime(), c->getLocalBounds().getCentre().toFloat(), Time::getCurrentTime(), 1, false);
            c->mouseDown(e);
            c->mouseUp(e);
        }
        if(allowsMouseClicks || allowsMouseClicksOnChildren) {
            for(auto* child : c->getChildren())
            {
                simulateClick(child, nullptr);
            }
        }
    }

    //static constexpr  bool recursive = false;
};
