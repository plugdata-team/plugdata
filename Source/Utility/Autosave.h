#pragma once
#include <readerwriterqueue.h>
#include "Dialogs/Dialogs.h"

class Autosave : public Timer, public AsyncUpdater, public Value::Listener {
    
    File autoSaveFile = ProjectInfo::appDataDir.getChildFile(".autosave");
    ValueTree autoSaveTree;
    Value autosaveInterval;
    Value autosaveEnabled;
    
    PluginProcessor* pd;
    moodycamel::ReaderWriterQueue<std::pair<String, String>> autoSaveQueue;
    
public:
    Autosave(PluginProcessor* procesor) : pd(procesor)
    {
        if(!autoSaveFile.existsAsFile())  {
            autoSaveFile.create();
            autoSaveTree = ValueTree("Autosave");
        }
        {
            FileInputStream istream(autoSaveFile);
            autoSaveTree = ValueTree::readFromStream(istream);
            // In case something went wrong
            if(!autoSaveTree.isValid()) autoSaveTree = ValueTree("Autosave");
        }
        
        autosaveEnabled.referTo(SettingsFile::getInstance()->getPropertyAsValue("autosave_enabled"));
        
        // autosave timer trigger
        autosaveInterval.referTo(SettingsFile::getInstance()->getPropertyAsValue("autosave_interval"));
        autosaveInterval.addListener(this);
        startTimer(1000 * getValue<int>(autosaveInterval));
    }
    
    // Call this whenever we load a file
    void checkForMoreRecentAutosave(File& patchPath, std::function<void()> callback)
    {
        auto* editor = dynamic_cast<PluginEditor*>(pd->getActiveEditor());
        if(!editor) return;
        
        auto lastAutoSavedPatch = autoSaveTree.getChildWithProperty("Path", patchPath.getFullPathName());
        auto autoSavedTime = static_cast<int64>(lastAutoSavedPatch.getProperty("LastModified"));
        auto fileChangedTime = patchPath.getLastModificationTime().toMilliseconds();
        if(lastAutoSavedPatch.isValid() && autoSavedTime > fileChangedTime)
        {
            Dialogs::showOkayCancelDialog(&editor->openedDialog, editor, "Restore autosave?", [lastAutoSavedPatch, patchPath, callback](bool useAutosaved){
                if(useAutosaved) {
                    MemoryOutputStream ostream;
                    Base64::convertFromBase64(ostream, lastAutoSavedPatch.getProperty("Patch").toString());
                    auto autosavedPatch = String::fromUTF8((const char*)ostream.getData(), ostream.getDataSize());
                    patchPath.replaceWithText(autosavedPatch);
                    // TODO: instead of replacing, it would be better to load it as a string, (but also with the correct patch path)
                }
                
                callback();
            });
        }
        else {
            callback();
        }
    }
    
private:
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(autosaveInterval)) {
            auto interval = getValue<int>(autosaveInterval);
            startTimer(1000 * interval);
        }
    }

    void timerCallback() override
    {
        if(!getValue<bool>(autosaveEnabled)) return;
        
        pd->enqueueFunctionAsync([this](){
            save();
        });
    }

    void save()
    {
        for(auto& patch : pd->patches)
        {
            // Check if patch is a root canvas
            bool isRootCanvas = false;
            for (auto* x = pd_getcanvaslist(); x; x = x->gl_next)
            {
                if(x == patch->getPointer().get())
                {
                    isRootCanvas = true;
                    break;
                }
            }
            if(!isRootCanvas) continue;
            
            auto patchFile = patch->getPatchFile();
            auto fullPath = patchFile.getFullPathName();
            
            // Simple way to filter out plugdata default patches which we don't want to save.
            if (!fullPath.contains("Documents/plugdata/") && !fullPath.contains("Documents\\plugdata\\") && patchFile.getParentDirectory() != File::getSpecialLocation(File::tempDirectory)) {
                autoSaveQueue.enqueue({ fullPath, patch->getCanvasContent() });
            }
        }
        
        triggerAsyncUpdate();
    }
    
    void handleAsyncUpdate() override
    {
        std::pair<String, String> pathAndContent;
        while(autoSaveQueue.try_dequeue(pathAndContent))
        {
            auto& [path, content] = pathAndContent;
            
            // Make sure we get current time in the correct format used by the OS for file modification time
            auto tempFile = File::createTempFile("temp_time_test");
            tempFile.create();
            auto time = tempFile.getCreationTime().toMilliseconds();
            tempFile.deleteFile();
            
            auto existingPatch = autoSaveTree.getChildWithProperty("Path", path);
            
            if(existingPatch.isValid())
            {
                existingPatch.setProperty("Patch", Base64::toBase64(content), nullptr);
                existingPatch.setProperty("LastModified", (int64)time, nullptr);
            }
            else {
                ValueTree newAutoSave = ValueTree("Save");
                newAutoSave.setProperty("Path", path, nullptr);
                newAutoSave.setProperty("Patch", Base64::toBase64(content), nullptr);
                newAutoSave.setProperty("LastModified", (int64)time, nullptr);
                autoSaveTree.appendChild(newAutoSave, nullptr);
            }
        }
        
        autoSaveFile.replaceWithText("");
        FileOutputStream ostream(autoSaveFile);
        autoSaveTree.writeToStream(ostream);
    }
};
