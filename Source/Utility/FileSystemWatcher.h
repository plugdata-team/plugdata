/*==============================================================================

Copyright 2018 by Roland Rabien
For more information visit www.rabiensoftware.com

==============================================================================*/

#pragma once

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

    /** Adds a folder to be watched */
    void addFolder(File const& folder);

    /** Removes a folder from being watched */
    void removeFolder(File const& folder);

    /** Removes all folders from being watched */
    void removeAllFolders();

    /** Gets a list of folders being watched */
    Array<File> getWatchedFolders();

    /** A set of events that can happen to a file.
        When a file is renamed it will appear as the
        original filename being deleted and the new
        filename being created
    */
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
        virtual ~Listener() = default;

        // group changes together
        void handleAsyncUpdate() override
        {
            filesystemChanged();
        }

        /* Called for each file that has changed and how it has changed. Use this callback
           if you need to reload a file when it's contents change */
        virtual void fileChanged(File const f, FileSystemEvent)
        {
            // By default, don't respond to hidden files (which would be .settings and .autosave)
            // If you want that to respond to hidden file changes, override this
            if (f.isHidden() || f.getFileName().startsWith("."))
                return;

            triggerAsyncUpdate();
        }

        virtual void filesystemChanged() {};
    };

    /** Registers a listener to be told when things happen to the text.
     @see removeListener
     */
    void addListener(Listener* newListener);

    /** Deregisters a listener.
     @see addListener
     */
    void removeListener(Listener* listener);

private:
    class Impl;

    void fileChanged(File const& file, FileSystemEvent fsEvent);

    ListenerList<Listener> listeners;

    OwnedArray<Impl> watched;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSystemWatcher)
};

#endif
