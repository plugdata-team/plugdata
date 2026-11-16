#pragma once
#include <readerwriterqueue.h>
#include "Dialogs/Dialogs.h"
#include "Components/BouncingViewport.h"

class Autosave final : public MultiTimer
    , public Value::Listener {

    enum {
        SaveTimer,
        CollectTimer
    };

    // What the audio thread hands over for a dirty patch. Turning it into text and storing it happens on the message thread
    struct SavedPatch {
        t_binbuf* content;
        t_symbol* dir;
        t_symbol* name;
    };

    static inline auto const autoSaveFile = ProjectInfo::appDataDir.getChildFile(".autosave");
    static inline auto autoSaveTree = ValueTree("Autosave");
    Value autosaveInterval;
    Value autosaveEnabled;

    PluginProcessor* pd;
    String const tempDirectory = File::getSpecialLocation(File::tempDirectory).getFullPathName().replace("\\", "/");
    moodycamel::ReaderWriterQueue<SavedPatch> autoSaveQueue = moodycamel::ReaderWriterQueue<SavedPatch>(64);
    std::atomic<bool> savePending = false;

public:
    explicit Autosave(PluginProcessor* procesor)
        : pd(procesor)
    {
        if (!autoSaveFile.existsAsFile()) {
            autoSaveFile.create();
        } else {
            FileInputStream istream(autoSaveFile);
            autoSaveTree = ValueTree::readFromStream(istream);
            // In case something went wrong
            if (!autoSaveTree.isValid())
                autoSaveTree = ValueTree("Autosave");
        }

        autosaveEnabled.referTo(SettingsFile::getInstance()->getPropertyAsValue("autosave_enabled"));

        // autosave timer trigger
        autosaveInterval.referTo(SettingsFile::getInstance()->getPropertyAsValue("autosave_interval"));
        autosaveInterval.addListener(this);
        updateAutosaveInterval();
    }

    ~Autosave() override
    {
        pd->setThis();
        SavedPatch saved;
        while (autoSaveQueue.try_dequeue(saved))
            binbuf_free(saved.content);
    }

    // Call this whenever we load a file
    static void checkForMoreRecentAutosave(URL const& patchUrl, PluginEditor* editor, std::function<void(URL const&, URL const&)> callback)
    {
        auto patchPath = patchUrl.getLocalFile();
        auto lastAutoSavedPatch = autoSaveTree.getChildWithProperty("Path", patchPath.getFullPathName());
        auto const autoSavedTime = static_cast<int64>(lastAutoSavedPatch.getProperty("LastModified"));
        auto const fileChangedTime = patchPath.getLastModificationTime().toMilliseconds();
        if (lastAutoSavedPatch.isValid() && autoSavedTime > fileChangedTime) {
            auto const timeDescription = RelativeTime((autoSavedTime - fileChangedTime) / 1000.0f).getApproximateDescription();

            Dialogs::showMultiChoiceDialog(
                &editor->openedDialog, editor, "Restore autosave?\n (last autosave is " + timeDescription + " newer)", [lastAutoSavedPatch, patchUrl, patchPath, callback, editor](int const dontUseAutosaved) {
                    if (!dontUseAutosaved) {
                        MemoryOutputStream ostream;
                        Base64::convertFromBase64(ostream, lastAutoSavedPatch.getProperty("Patch").toString());
                        auto const autosavedPatch = String::fromUTF8(static_cast<char const*>(ostream.getData()), ostream.getDataSize());

                        glob_forcefilename(editor->pd->generateSymbol(patchPath.getFileName().toRawUTF8()), editor->pd->generateSymbol(patchPath.getParentDirectory().getFullPathName().replaceCharacter('\\', '/').toRawUTF8()));
                        auto const patchFile = File::createTempFile(".pd");
                        patchFile.replaceWithText(autosavedPatch);
                        callback(URL(patchFile), patchUrl);
                    } else {
                        callback(URL(patchPath), patchUrl);
                    }
                },
                { "Yes", "No" });
        } else {
            callback(patchUrl, patchUrl);
        }
    }

private:
    void updateAutosaveInterval()
    {
        auto const interval = jlimit(1, 60, getValue<int>(autosaveInterval));
        startTimer(SaveTimer, 1000 * 60 * interval);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(autosaveInterval)) {
            updateAutosaveInterval();
        }
    }

    void timerCallback(int const timerID) override
    {
        if (timerID == CollectTimer) {
            collect();
            return;
        }

        if (!getValue<bool>(autosaveEnabled))
            return;

        savePending = true;
        pd->enqueueFunctionAsync([_this = WeakReference(this)] {
            if (_this) {
                _this->save();
            }
        });

        // The audio thread can't wake us up without posting a message, so we look for its result ourselves
        startTimer(CollectTimer, 100);
    }

    // Runs on the audio thread: only do what needs the patch here, and don't allocate for the hand-off
    void save()
    {
        for (auto const& patch : pd->patches) {
            auto* cnv = patch->getRawPointer();

            // Only dirty top-level patches
            if (!cnv || cnv->gl_owner || !cnv->gl_dirty || patch->openInPluginMode)
                continue;

            auto* dir = canvas_getdir(cnv);
            if (isInternalPatch(dir->s_name))
                continue;

            auto* content = pd::Interface::getCanvasBinbuf(cnv);
            if (!autoSaveQueue.try_enqueue(SavedPatch { content, dir, cnv->gl_name }))
                binbuf_free(content);
        }

        savePending = false;
    }

    // Simple way to filter out plugdata default patches which we don't want to save.
    bool isInternalPatch(char const* dir) const
    {
        return std::strstr(dir, "Documents/plugdata/Abstractions") || std::strstr(dir, "Documents/plugdata/Documentation") || std::strstr(dir, "Documents/plugdata/Extra") || tempDirectory.equalsIgnoreCase(dir);
    }

    void collect()
    {
        // Read this before emptying the queue, otherwise we could stop while the last patches are still on their way
        bool const finished = !savePending;

        pd->setThis();

        bool changed = false;
        SavedPatch saved;
        while (autoSaveQueue.try_dequeue(saved)) {
            auto const path = File(String::fromUTF8(saved.dir->s_name)).getChildFile(String::fromUTF8(saved.name->s_name)).getFullPathName();
            auto const content = pd::Interface::getBinbufText(saved.content);
            changed = true;

            // Make sure we get current time in the correct format used by the OS for file modification time
            auto tempFile = File::createTempFile("temp_time_test");
            tempFile.create();
            auto const time = tempFile.getCreationTime().toMilliseconds();
            tempFile.deleteFile();

            auto existingPatch = autoSaveTree.getChildWithProperty("Path", path);

            if (existingPatch.isValid()) {
                existingPatch.setProperty("Patch", Base64::toBase64(content), nullptr);
                existingPatch.setProperty("LastModified", time, nullptr);
            } else {
                auto newAutoSave = ValueTree("Save");
                newAutoSave.setProperty("Path", path, nullptr);
                newAutoSave.setProperty("Patch", Base64::toBase64(content), nullptr);
                newAutoSave.setProperty("LastModified", time, nullptr);
                autoSaveTree.addChild(newAutoSave, 0, nullptr);

                if (autoSaveTree.getNumChildren() > 15) {
                    int64 oldestTime = std::numeric_limits<int64>::max();
                    int oldestIdx = -1;
                    int currentIdx = 0;
                    for (auto autoSave : autoSaveTree) {
                        auto const modifiedTime = static_cast<int64>(autoSave.getProperty("LastModified"));
                        if (modifiedTime < oldestTime) {
                            oldestTime = modifiedTime;
                            oldestIdx = currentIdx;
                        }
                        currentIdx++;
                    }
                    if (oldestIdx >= 0) {
                        autoSaveTree.removeChild(oldestIdx, nullptr);
                    }
                }
            }
        }

        if (changed) {
            autoSaveFile.replaceWithText("");
            FileOutputStream ostream(autoSaveFile);
            autoSaveTree.writeToStream(ostream);
        }

        if (finished)
            stopTimer(CollectTimer);
    }

    friend class AutosaveHistoryComponent;
    JUCE_DECLARE_WEAK_REFERENCEABLE(Autosave);
};

class AutosaveHistoryComponent final : public Component {
    class AutoSaveHistory final : public Component {
    public:
        AutoSaveHistory(PluginEditor* editor, ValueTree autoSaveTree)
        {
            patchPath = autoSaveTree.getProperty("Path").toString();
            patch = autoSaveTree.getProperty("Patch").toString();

            addAndMakeVisible(openPatch);

            auto const backgroundColour = getThemeColours(*this).panelForegroundColour;
            openPatch.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
            openPatch.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
            openPatch.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            openPatch.onClick = [this, editor] {
                MemoryOutputStream ostream;
                Base64::convertFromBase64(ostream, patch);
                auto const patch = editor->pd->loadPatch(String::fromUTF8(static_cast<char const*>(ostream.getData()), ostream.getDataSize()));
                patch->setTitle(patchPath.fromLastOccurrenceOf("/", false, false));
                patch->setCurrentFile(URL(patchPath));
                editor->getTabComponent().triggerAsyncUpdate();

                MessageManager::callAsync([editor] {
                    // Close the whole chain of dialogs
                    // do it async so the stack can unwind normally
                    editor->openedDialog.reset(nullptr);
                });
            };
        }

        void resized() override
        {
            openPatch.setBounds(getLocalBounds().removeFromRight(140).reduced(24, 20).translated(0, -1));
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            // Leaves 8px below the card for its shadow
            auto bounds = getLocalBounds().reduced(16, 0).withTrimmedTop(6).withTrimmedBottom(8);
            StackShadow::drawShadowForRect(g, bounds, Corners::defaultCornerRadius, StackShadow::Card);

            g.setColour(colours.panelForegroundColour);
            g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);

            g.setColour(colours.toolbarOutlineColour);
            g.drawRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius, 1.0f);

            Fonts::drawIcon(g, Icons::File, bounds.removeFromLeft(32).withTrimmedLeft(10), colours.panelTextColour, 20);

            auto const patchName = patchPath.fromLastOccurrenceOf("/", false, false);
            Fonts::drawStyledText(g, patchName, bounds.removeFromTop(24).withTrimmedLeft(14), colours.panelTextColour, Semibold, 15);

            g.setFont(Fonts::getDefaultFont().withHeight(14.0f));
            g.setColour(colours.panelTextColour);
            g.drawText(patchPath, bounds.removeFromTop(24).withTrimmedLeft(14), Justification::centredLeft);
        }

        String patchPath;
        String patch;
        TextButton openPatch = TextButton("Open");
    };

    class ContentComponent final : public Component {
    public:
        explicit ContentComponent(PluginEditor* editor)
        {
            for (auto const child : Autosave::autoSaveTree) {
                addAndMakeVisible(histories.add(new AutoSaveHistory(editor, child)));
            }

            setSize(getWidth(), Autosave::autoSaveTree.getNumChildren() * 64 + 24);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();

            for (auto* history : histories) {
                history->setBounds(bounds.removeFromTop(64));
            }
        }

        OwnedArray<AutoSaveHistory> histories;
    };

public:
    explicit AutosaveHistoryComponent(PluginEditor* editor)
        : contentComponent(editor)
    {
        backButton.onClick = [this] {
            setVisible(false);
        };
        addAndMakeVisible(backButton);

        contentComponent.setVisible(true);
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport);
    }

private:
    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        auto bounds = getLocalBounds();
        auto const titlebarBounds = bounds.removeFromTop(40).toFloat();

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(colours.toolbarBackgroundColour);
        g.fillPath(toolbarPath);

        Path backgroundPath;
        backgroundPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);
        g.setColour(colours.panelBackgroundColour);
        g.fillPath(backgroundPath);

        Fonts::drawStyledText(g, "Autosave History", titlebarBounds, colours.panelTextColour, Semibold, 15, Justification::centred);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(getThemeColours(*this).toolbarOutlineColour);
        g.drawLine(0, 40, getWidth(), 40);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto toolbarBounds = bounds.removeFromTop(40);

        backButton.setBounds(toolbarBounds.removeFromLeft(40).translated(2, 0));
        contentComponent.setSize(bounds.getWidth(), std::max(contentComponent.getHeight(), bounds.getHeight()));
        viewport.setBounds(bounds);
    }

    BouncingViewport viewport;
    ContentComponent contentComponent;
    MainToolbarButton backButton = MainToolbarButton(Icons::Back);
};
