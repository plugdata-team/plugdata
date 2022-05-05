/*==============================================================================

Copyright 2018 by Roland Rabien
For more information visit www.rabiensoftware.com

==============================================================================*/

#pragma once

#if defined JUCE_MAC || defined JUCE_WINDOWS || defined JUCE_LINUX

/**

    Watches a folder in the file system for changes.

    Listener callbcks will be called every time a file is
    created, modified, deleted or renamed in the watched
    folder.

    FileSystemWatcher will also recursively watch all subfolders on
    macOS and windows and will not on Linux.

 */
class FileSystemWatcher
{
public:
    //==============================================================================
    FileSystemWatcher();
    ~FileSystemWatcher();

    //==============================================================================
    /** Adds a folder to be watched */
    void addFolder (const File& folder);

    /** Removes a folder from being watched */
    void removeFolder (const File& folder);

    /** Removes all folders from being watched */
    void removeAllFolders();

    /** Gets a list of folders being watched */
    Array<File> getWatchedFolders ();

    /** A set of events that can happen to a file.
        When a file is renamed it will appear as the
        original filename being deleted and the new
        filename being created
    */
    enum FileSystemEvent
    {
        fileCreated,
        fileDeleted,
        fileUpdated,
        fileRenamedOldName,
        fileRenamedNewName
    };

    //==============================================================================
    /** Receives callbacks from the FileSystemWatcher when a file changes */
    class Listener : public Timer
    {
    public:
        virtual ~Listener() = default;
        
        virtual void changeCallback() = 0;
        
        
        // group changes together
        void timerCallback() {
            changeCallback();
            stopTimer();
        }
        /* Called when any file in the listened to folder changes with the name of
           the folder that has changed. For example, use this for a file browser that
           needs to refresh any time a file changes */
        void folderChanged (const File) {
            startTimer(200);
        }

        /* Called for each file that has changed and how it has changed. Use this callback
           if you need to reload a file when it's contents change */
        void fileChanged (const File, FileSystemEvent) {
            startTimer(200);
        }
    };

    /** Registers a listener to be told when things happen to the text.
     @see removeListener
     */
    void addListener (Listener* newListener);

    /** Deregisters a listener.
     @see addListener
     */
    void removeListener (Listener* listener);

private:
    class Impl;

    void folderChanged (const File& folder);
    void fileChanged (const File& file, FileSystemEvent fsEvent);

    ListenerList<Listener> listeners;

    OwnedArray<Impl> watched;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileSystemWatcher)
};

#endif
