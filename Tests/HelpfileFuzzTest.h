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

        // Open every helpfile, this will make sure it initialises and closes every object at least once (but probasbly a whole bunch of times in different contexts)
        // Run with AddressSanitizer, UBSanitizer or ThreadSanitizer to find all memory, UB and threading problems
        for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation"), true, true))
        {
            if(file.hasFileExtension(".pd"))
            {
                allHelpfiles.add(file);
            }
        }

        allHelpfiles.sort();
        //allHelpfiles.remove_range(220, allHelpfiles.size()-1);

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

        auto* cnv = tabbar.openPatch(URL(helpFile));
        auto* editor = cnv->editor;

        // Click everything
        cnv->locked.setValue(true);
        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();

        HeapArray<Object*> objects;
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

    static void clickNextObject(Component::SafePointer<Canvas> cnv, HeapArray<Object*>& objects, std::function<void()> done)
    {
        if(!objects.size() || !cnv) {
            done();
            return;
        }

        auto* obj = objects.back();
        objects.remove_at(objects.size() - 1);

        simulateClick(obj, cnv.getComponent());

        cnv->pd->volume->store(0.0f);

        if(objects.empty())
        {
            Timer::callAfterDelay(10, [done](){
                done();
            });
            return;
        }

        if(sys_trylock())
        Timer::callAfterDelay(2, [cnv, objects, done]() mutable {
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
            const_cast<ModifierKeys&>(e.mods) = ModifierKeys::noModifiers;
            c->mouseUp(e);
        }
        if(allowsMouseClicks || allowsMouseClicksOnChildren) {
            for(auto* child : c->getChildren())
            {
                simulateClick(child, nullptr);
            }
        }
    }

    //static constexpr  bool recursive = false; TODO: implement this
};
