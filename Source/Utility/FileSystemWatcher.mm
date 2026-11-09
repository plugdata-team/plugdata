#import <Foundation/Foundation.h>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

using namespace juce;
#include "FileSystemWatcher.h"

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
    Impl (FileSystemWatcher& o, File f) : owner (o), folder (f)
    {
        const int fileDescriptor = open(folder.getFullPathName().toRawUTF8(), O_EVTONLY);

        if (fileDescriptor == -1)
        {
            DBG("Failed to open file descriptor for: " + folder.getFullPathName());
            return;
        }

        dispatchSource = dispatch_source_create(
            DISPATCH_SOURCE_TYPE_VNODE,
            fileDescriptor,
            DISPATCH_VNODE_WRITE | DISPATCH_VNODE_DELETE | DISPATCH_VNODE_RENAME | DISPATCH_VNODE_EXTEND,
            dispatch_get_main_queue()
        );

        if (!dispatchSource)
        {
            close(fileDescriptor);
            return;
        }

        Impl* implPtr = this;
        auto aliveFlag = alive;

        dispatch_source_set_event_handler(dispatchSource, ^{
            if (!aliveFlag->load())
                return;

            unsigned long flags = dispatch_source_get_data(implPtr->dispatchSource);

            auto notify = [&](unsigned long flag, FileSystemEvent event) {
                if ((flags & flag) && aliveFlag->load())
                    implPtr->owner.fileChanged(implPtr->folder, event);
            };

            notify(DISPATCH_VNODE_WRITE, FileSystemEvent::fileUpdated);
            notify(DISPATCH_VNODE_DELETE, FileSystemEvent::fileDeleted);
            notify(DISPATCH_VNODE_RENAME, FileSystemEvent::fileRenamedNewName);
            notify(DISPATCH_VNODE_EXTEND, FileSystemEvent::fileCreated);
        });

        dispatch_source_set_cancel_handler(dispatchSource, ^{
            close(fileDescriptor);
        });

        dispatch_resume(dispatchSource);
    }

    ~Impl()
    {
        alive->store(false);

        if (dispatchSource)
        {
            dispatch_source_cancel(dispatchSource);
            dispatch_release(dispatchSource);
        }
    }

    FileSystemWatcher& owner;
    const File folder;
    dispatch_source_t dispatchSource = nullptr;
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
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
