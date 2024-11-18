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
            //ProjectInfo::appDataDir.getChildFile("console-errors.md").replaceWithText(loggedErrors);
            signalDone();
            return;
        }

        auto helpFile = helpFiles.back();
        helpFiles.remove_at(helpFiles.size() - 1);

        beginTest(String(helpFiles.size()) + " -> " + helpFile.getFullPathName());

        auto* cnv = tabbar.openPatch(URL(helpFile));
        auto* pd = cnv->pd;
        auto* editor = cnv->editor;

        // Evil test that deletes the patch instantly after being created, leaving dangling pointers everywhere
        // plugdata should be able to handle that!
        if(detachCanvas) {
            HeapArray<pd::WeakReference> openedPatches;
            // Close all patches
            for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
                openedPatches.add(pd::WeakReference(cnv, pd));
            }

            pd->patches.clear();
            for(auto patch : openedPatches)
            {
                if(auto cnv = patch.get<t_glist*>()) {
                    libpd_closefile(cnv.get());
                }
            }
        }

        // Click everything
        cnv->locked.setValue(true);
        editor->pd->volume->store(0.0f);

        HeapArray<Object*> objects;
        for(auto* obj : cnv->objects)
        {
            objects.add(obj);
        }

        // Will simulate a click on all objects
        clickNextObject(cnv, objects, [this, &tabbar, &helpFiles](){
            while(auto* cnv = tabbar.getCurrentCanvas()) { // TODO: why is this faster than closeAllTabs?()
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
        if(simulateRealClicks)
        {
            auto* peer = cnv->editor->getTopLevelComponent()->getPeer();
            auto& peerComponent = peer->getComponent();
            auto pos = peerComponent.getLocalPoint(nullptr, c->getScreenBounds().getCentre().toFloat());
            auto viewportBounds = peerComponent.getLocalArea(nullptr, cnv->viewport->getScreenBounds().toFloat()).reduced(8);

            if(viewportBounds.contains(pos)) {
                peer->handleMouseEvent(MouseInputSource::InputSourceType::mouse, pos, ModifierKeys::leftButtonModifier, 0.0f, 0.0f, Time::getMillisecondCounter());
                peer->handleMouseEvent(MouseInputSource::InputSourceType::mouse, pos, ModifierKeys::noModifiers, 0.0f, 0.0f, Time::getMillisecondCounter());
            }
        }
        else {
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
    }

    static constexpr bool detachCanvas = false;
    //static constexpr  bool recursive = false; TODO: implement this
    static constexpr bool simulateRealClicks = false;
};
