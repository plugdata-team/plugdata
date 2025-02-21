#pragma once
#include <readerwriterqueue.h>
#include "Dialogs/Dialogs.h"
#include "Components/BouncingViewport.h"

class Autosave final : public Timer
    , public AsyncUpdater
    , public Value::Listener {

    static inline File const autoSaveFile = ProjectInfo::appDataDir.getChildFile(".autosave");
    static inline ValueTree autoSaveTree = ValueTree("Autosave");
    Value autosaveInterval;
    Value autosaveEnabled;

    PluginProcessor* pd;
    moodycamel::ReaderWriterQueue<std::pair<String, String>> autoSaveQueue;

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

    // Call this whenever we load a file
    static void checkForMoreRecentAutosave(File& patchPath, PluginEditor* editor, std::function<void(File, File)> callback)
    {
        auto lastAutoSavedPatch = autoSaveTree.getChildWithProperty("Path", patchPath.getFullPathName());
        auto const autoSavedTime = static_cast<int64>(lastAutoSavedPatch.getProperty("LastModified"));
        auto const fileChangedTime = patchPath.getLastModificationTime().toMilliseconds();
        if (lastAutoSavedPatch.isValid() && autoSavedTime > fileChangedTime) {
            auto const timeDescription = RelativeTime((autoSavedTime - fileChangedTime) / 1000.0f).getApproximateDescription();

            Dialogs::showMultiChoiceDialog(
                &editor->openedDialog, editor, "Restore autosave?\n (last autosave is " + timeDescription + " newer)", [lastAutoSavedPatch, &patchPath, callback, editor](int const dontUseAutosaved) {
                    if (!dontUseAutosaved) {
                        MemoryOutputStream ostream;
                        Base64::convertFromBase64(ostream, lastAutoSavedPatch.getProperty("Patch").toString());
                        auto const autosavedPatch = String::fromUTF8(static_cast<char const*>(ostream.getData()), ostream.getDataSize());
                        
                        glob_forcefilename(editor->pd->generateSymbol(patchPath.getFileName().toRawUTF8()), editor->pd->generateSymbol(patchPath.getParentDirectory().getFullPathName().replaceCharacter('\\', '/').toRawUTF8()));
                        auto patchFile = File::createTempFile(".pd");
                        patchFile.replaceWithText(autosavedPatch);
                        callback(patchFile, patchPath);
                    }
                    else {
                        callback(patchPath, patchPath);
                    }

                },
                { "Yes", "No" });
        } else {
            callback(patchPath, patchPath);
        }
    }

private:
    void updateAutosaveInterval()
    {
        auto const interval = jlimit(1, 60, getValue<int>(autosaveInterval));
        startTimer(1000 * 60 * interval);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(autosaveInterval)) {
            updateAutosaveInterval();
        }
    }

    void timerCallback() override
    {
        if (!getValue<bool>(autosaveEnabled))
            return;

        auto patches = pd->patches;
        pd->enqueueFunctionAsync([_this = WeakReference(this), patches] {
            if (_this) {
                _this->pd->lockAudioThread();
                _this->save(patches);
                _this->pd->unlockAudioThread();
            }
        });
    }

    void save(SmallArray<pd::Patch::Ptr, 16> const& patches)
    {
        for (auto const& patch : pd->patches) {
            auto const* patchPtr = patch->getPointer().get();
            if (!patchPtr || !patchPtr->gl_dirty)
                continue;

            // Check if patch is a root canvas
            for (auto const* x = pd_getcanvaslist(); x; x = x->gl_next) {
                if (x == patchPtr) {

                    auto patchFile = patch->getPatchFile();

                    // Simple way to filter out plugdata default patches which we don't want to save.
                    if (!isInternalPatch(patchFile) && !patch->openInPluginMode) {
                        autoSaveQueue.enqueue({ patchFile.getFullPathName(), patch->getCanvasContent() });
                    }

                    triggerAsyncUpdate();
                    break;
                }
            }
        }
    }

    static bool isInternalPatch(File const& patch)
    {
        auto const pathName = patch.getFullPathName().replace("\\", "/");
        return pathName.contains("Documents/plugdata/Abstractions") || pathName.contains("Documents/plugdata/Documentation") || pathName.contains("Documents/plugdata/Extra") || patch.getParentDirectory() == File::getSpecialLocation(File::tempDirectory);
    }

    void handleAsyncUpdate() override
    {
        std::pair<String, String> pathAndContent;
        while (autoSaveQueue.try_dequeue(pathAndContent)) {
            auto& [path, content] = pathAndContent;

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

        autoSaveFile.replaceWithText("");
        FileOutputStream ostream(autoSaveFile);
        autoSaveTree.writeToStream(ostream);
    }

    friend class AutosaveHistoryComponent;
    JUCE_DECLARE_WEAK_REFERENCEABLE(Autosave);
};

class AutosaveHistoryComponent final : public Component {
    struct AutoSaveHistory final : public Component {
        AutoSaveHistory(PluginEditor* editor, ValueTree autoSaveTree)
        {
            patchPath = autoSaveTree.getProperty("Path").toString();
            patch = autoSaveTree.getProperty("Patch").toString();

            addAndMakeVisible(openPatch);

            auto const backgroundColour = findColour(PlugDataColour::panelForegroundColourId);
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
            openPatch.setBounds(getLocalBounds().removeFromRight(140).reduced(24, 20).translated(0, 4));
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(16, 3).withTrimmedTop(8);

            Path shadowPath;
            shadowPath.addRoundedRectangle(bounds.reduced(3).toFloat(), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(hash("autosave"), g, shadowPath, Colour(0, 0, 0).withAlpha(0.4f), 7, { 0, 1 });

            g.setColour(findColour(PlugDataColour::panelForegroundColourId));
            g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.drawRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius, 1.0f);

            Fonts::drawIcon(g, Icons::File, bounds.removeFromLeft(32).withTrimmedLeft(10), findColour(PlugDataColour::panelTextColourId), 20);

            auto const patchName = patchPath.fromLastOccurrenceOf("/", false, false);
            Fonts::drawStyledText(g, patchName, bounds.removeFromTop(24).withTrimmedLeft(14), findColour(PlugDataColour::panelTextColourId), Semibold, 15);

            g.setFont(Fonts::getDefaultFont().withHeight(14.0f));
            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.drawText(patchPath, bounds.removeFromTop(24).withTrimmedLeft(14), Justification::centredLeft);
        }

        String patchPath;
        String patch;
        TextButton openPatch = TextButton("Open");
    };

    struct ContentComponent final : public Component {
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
        auto bounds = getLocalBounds();
        auto const titlebarBounds = bounds.removeFromTop(40).toFloat();

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(toolbarPath);

        Path backgroundPath;
        backgroundPath.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillPath(backgroundPath);

        Fonts::drawStyledText(g, "Autosave History", titlebarBounds, findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
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
