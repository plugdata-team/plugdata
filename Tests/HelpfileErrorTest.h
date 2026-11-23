class HelpFileErrorTest : public PlugDataUnitTest
{
public:
    HelpFileErrorTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Helpfile Error Test")
    {
    }

private:
    void perform() override
    {
        HeapArray<File> allHelpfiles;
        loggedErrors.clear();
        auto const filter = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_HELPFILE_FILTER", {}), ",", "");

        // Open every helpfile, this will make sure it initialises and closes every object at least once (but probasbly a whole bunch of times in different contexts)
        // Run with AddressSanitizer, UBSanitizer or ThreadSanitizer to find all memory, UB and threading problems
        for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation"), true, true))
        {
            if(file.hasFileExtension(".pd"))
            {
                allHelpfiles.add(file);
            }
        }

        if (!filter.isEmpty()) {
            allHelpfiles.remove_if([&filter](File const& file) {
                return std::none_of(filter.begin(), filter.end(), [&file](String const& token) {
                    return token.trim().isNotEmpty() && file.getFullPathName().contains(token.trim());
                });
            });
        }

        allHelpfiles.sort();
        openHelpfilesRecursively(editor->getTabComponent(), allHelpfiles);
    }

    void openHelpfilesRecursively(TabComponent& tabbar, HeapArray<File>& helpFiles)
    {
        if(helpFiles.empty())
        {
            ProjectInfo::appDataDir.getChildFile("console-errors.md").replaceWithText(loggedErrors);
            beginTest("Console error summary");
            if (loggedErrors.isNotEmpty())
                logMessage(loggedErrors);
            expect(loggedErrors.isEmpty(), "Help patches reported console errors (listed above)");
            signalDone(true);
            return;
        }

        auto helpFile = helpFiles.back();
        helpFiles.remove_at(helpFiles.size() - 1);

        beginTest(String(helpFiles.size()) + " -> " + helpFile.getFullPathName());

        editor->pd->flushConsoleMessages();
        editor->getSidebarForPanel(Sidebar::ConsolePanel)->clearConsole();
        tabbar.openPatch(URL(helpFile));
        auto* cnv = tabbar.getCurrentCanvas();
        auto* pd = cnv->pd;
        auto* editor = cnv->editor;

        // Click everything
        cnv->locked.setValue(true);
        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();
        
        editor->pd->flushMessageQueue();
        pd->flushConsoleMessages();

        StringArray errors;
        for(auto& [ptr, message, type, length, repeats] : pd->getConsoleMessages())
        {
            if(type == 1) errors.add(message);
        }

        if(!errors.isEmpty())
        {
            loggedErrors += "\n\n\n" + helpFile.getFullPathName() + "\n--------------------------------------------------------------------------\n";
            for(auto& error : errors)
            {
                loggedErrors += error + "\n";
            }
        }
        while(auto* cnv = tabbar.getCurrentCanvas()) {
            tabbar.closeTab(cnv);
        }
        MessageManager::callAsync([this, &tabbar, helpFiles]() mutable {
            openHelpfilesRecursively(tabbar, helpFiles);
        });
        
        editor->getSidebarForPanel(Sidebar::ConsolePanel)->clearConsole();
    }

    
    String loggedErrors;
};
