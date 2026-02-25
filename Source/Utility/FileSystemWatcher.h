/*==============================================================================

Copyright 2018 by Roland Rabien
For more information visit www.rabiensoftware.com

==============================================================================*/

#pragma once
#include "Containers.h"

#if JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD || JUCE_IOS

/**

    Watches a folder in the file system for changes.

    Listener callbcks will be called every time a file is
    created, modified, deleted or renamed in the watched
    folder.

    FileSystemWatcher will also recursively watch all subfolders on
    macOS and windows and will not on Linux.

 */
class FileSystemWatcher {
public:
    FileSystemWatcher();
    ~FileSystemWatcher();

    void addFolder(File const& folder);
    void removeFolder(File const& folder);
    void removeAllFolders();

    enum FileSystemEvent {
        fileCreated,
        fileDeleted,
        fileUpdated,
        fileRenamedOldName,
        fileRenamedNewName
    };

    /** Receives callbacks from the FileSystemWatcher when a file changes */
    class Listener : public AsyncUpdater {
    public:
        ~Listener() override = default;

        // group changes together
        void handleAsyncUpdate() override
        {
            filesystemChanged();
        }

        /* Called for each file that has changed and how it has changed. Use this callback
           if you need to reload a file when it's contents change */
        virtual void fileChanged(File const f, FileSystemEvent)
        {
            triggerAsyncUpdate();
        }

        virtual void filesystemChanged() { }
    };
    

    void addListener (Listener* newListener)
    {
        listeners.add (newListener);
    }

    void removeListener (Listener* listener)
    {
        listeners.remove (listener);
    }
    
    static void addGlobalIgnorePath(File const& pathToIgnore)
    {
        pathsToIgnore.add_unique(pathToIgnore);
    }
    
    static void removeGlobalIgnorePath(File const& pathToIgnore)
    {
        pathsToIgnore.remove_one(pathToIgnore);
    }
    
private:
    class Impl;

    void fileChanged(File const& f, FileSystemEvent fsEvent)
    {
        // By default, don't respond to hidden files (which would be .settings and .autosave)
        // If you want that to respond to hidden file changes, override this
        if (f.isHidden() || f.getFileName().startsWith("."))
            return;
        
        for(auto const& pathToIgnore : pathsToIgnore)
        {
            if(f.isAChildOf(pathToIgnore) || f == pathToIgnore) {
                return;
            }
        }
        
        listeners.call (&FileSystemWatcher::Listener::fileChanged, f, fsEvent);
    }

    ListenerList<Listener> listeners;
    OwnedArray<Impl> watched;
    static inline SmallArray<File> pathsToIgnore;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSystemWatcher)
};

#endif
