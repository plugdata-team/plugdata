#import <Foundation/Foundation.h>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

using namespace juce;
#include "FileSystemWatcher.h"
#include "Containers.h"

#if JUCE_MAC
class FileSystemWatcher::Impl
{
public:
    Impl (FileSystemWatcher& o, File f) : owner (o), folder (f)
    {
        NSString* newPath = [NSString stringWithUTF8String:folder.getFullPathName().toRawUTF8()];

        paths = [[NSArray arrayWithObject:newPath] retain];
        context.version         = 0L;
        context.info            = this;
        context.retain          = nil;
        context.release         = nil;
        context.copyDescription = nil;

        stream = FSEventStreamCreate (kCFAllocatorDefault, callback, &context, (CFArrayRef)paths, kFSEventStreamEventIdSinceNow, 0.05,
                                      kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents);
        if (stream)
        {
            FSEventStreamScheduleWithRunLoop (stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
            FSEventStreamStart (stream);
        }

    }

    ~Impl()
    {
        if (stream)
        {
            FSEventStreamStop (stream);
            FSEventStreamUnscheduleFromRunLoop (stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
            FSEventStreamInvalidate (stream);
            FSEventStreamRelease (stream);
        }
    }

    static void callback (ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths,
                          const FSEventStreamEventFlags* eventFlags, const FSEventStreamEventId* eventIds)
    {
        ignoreUnused (streamRef, numEvents, eventIds, eventPaths, eventFlags);

        Impl* impl = (Impl*)clientCallBackInfo;

        char** files = (char**)eventPaths;

        for (int i = 0; i < int (numEvents); i++)
        {
            char* file = files[i];
            FSEventStreamEventFlags evt = eventFlags[i];

            File path = String::fromUTF8 (file);
            if (evt & kFSEventStreamEventFlagItemModified)
                impl->owner.fileChanged (path, FileSystemEvent::fileUpdated);
            else if (evt & kFSEventStreamEventFlagItemRemoved)
                impl->owner.fileChanged (path, FileSystemEvent::fileDeleted);
            else if (evt & kFSEventStreamEventFlagItemRenamed)
                impl->owner.fileChanged (path, path.exists() ? FileSystemEvent::fileRenamedNewName : FileSystemEvent::fileRenamedOldName);
            else if (evt & kFSEventStreamEventFlagItemCreated)
                impl->owner.fileChanged (path, FileSystemEvent::fileCreated);
        }
    }

    FileSystemWatcher& owner;
    const File folder;

    NSArray* paths;
    FSEventStreamRef stream;
    struct FSEventStreamContext context;
};

#elif JUCE_IOS

class FileSystemWatcher::Impl
{
public:
    Impl (FileSystemWatcher& o, File f) : owner (o), folder (f), fileDescriptor(-1), dispatchSource(nullptr)
    {
        NSString* path = [NSString stringWithUTF8String:folder.getFullPathName().toRawUTF8()];

        // Open the directory for monitoring
        fileDescriptor = open([path UTF8String], O_EVTONLY);

        if (fileDescriptor == -1)
        {
            DBG("Failed to open file descriptor for: " + folder.getFullPathName());
            return;
        }

        // Create dispatch source for monitoring
        dispatchSource = dispatch_source_create(
            DISPATCH_SOURCE_TYPE_VNODE,
            fileDescriptor,
            DISPATCH_VNODE_WRITE | DISPATCH_VNODE_DELETE | DISPATCH_VNODE_RENAME | DISPATCH_VNODE_EXTEND,
            dispatch_get_main_queue()
        );

        if (!dispatchSource)
        {
            close(fileDescriptor);
            fileDescriptor = -1;
            return;
        }

        // Capture 'this' for the event handler
        Impl* implPtr = this;

        dispatch_source_set_event_handler(dispatchSource, ^{
            unsigned long flags = dispatch_source_get_data(dispatchSource);

            if (flags & DISPATCH_VNODE_WRITE)
            {
                implPtr->owner.fileChanged(implPtr->folder, FileSystemEvent::fileUpdated);
            }
            if (flags & DISPATCH_VNODE_DELETE)
            {
                implPtr->owner.fileChanged(implPtr->folder, FileSystemEvent::fileDeleted);
            }
            if (flags & DISPATCH_VNODE_RENAME)
            {
                implPtr->owner.fileChanged(implPtr->folder, FileSystemEvent::fileRenamedNewName);
            }
            if (flags & DISPATCH_VNODE_EXTEND)
            {
                implPtr->owner.fileChanged(implPtr->folder, FileSystemEvent::fileCreated);
            }
        });

        dispatch_source_set_cancel_handler(dispatchSource, ^{
            if (implPtr->fileDescriptor != -1)
            {
                close(implPtr->fileDescriptor);
                implPtr->fileDescriptor = -1;
            }
        });

        dispatch_resume(dispatchSource);
    }

    ~Impl()
    {
        if (dispatchSource)
        {
            dispatch_source_cancel(dispatchSource);
            dispatch_release(dispatchSource);
            dispatchSource = nullptr;
        }

        if (fileDescriptor != -1)
        {
            close(fileDescriptor);
            fileDescriptor = -1;
        }
    }

    FileSystemWatcher& owner;
    const File folder;
    int fileDescriptor;
    dispatch_source_t dispatchSource;
};

#endif

FileSystemWatcher::FileSystemWatcher()
{
}

FileSystemWatcher::~FileSystemWatcher()
{
}

void FileSystemWatcher::addFolder (const File& folder)
{
    SmallArray<File> allFolders;
    for (auto w : watched)
        allFolders.add (w->folder);

    if ( !allFolders.contains (folder))
        watched.add (new Impl (*this, folder));
}

void FileSystemWatcher::removeFolder (const File& folder)
{
    for (int i = watched.size(); --i >= 0;)
    {
        if (watched[i]->folder == folder)
        {
            watched.remove (i);
            break;
        }
    }
}

void FileSystemWatcher::removeAllFolders()
{
    watched.clear();
}

void FileSystemWatcher::addListener (Listener* newListener)
{
    listeners.add (newListener);
}

void FileSystemWatcher::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

void FileSystemWatcher::fileChanged (const File& file, FileSystemEvent fsEvent)
{
    if(file.getFileName().endsWith(".autosave")) return;

    listeners.call (&FileSystemWatcher::Listener::fileChanged, file, fsEvent);
}
