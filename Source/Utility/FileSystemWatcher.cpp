/*==============================================================================

Copyright 2018 by Roland Rabien
For more information visit www.rabiensoftware.com

==============================================================================*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

using namespace juce;
#include "FileSystemWatcher.h"

#ifdef _WIN32
#    include <Windows.h>
#    include <ctime>
#else
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <fcntl.h>
#    include <semaphore.h>
#    include <ctime>
#endif

#if defined(__linux__) || JUCE_BSD
#    include <sys/inotify.h>
#    include <limits.h>
#    include <unistd.h>
#    include <sys/stat.h>
#    include <sys/time.h>
#    include <poll.h>
#endif

#ifdef JUCE_LINUX
#    define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

class FileSystemWatcher::Impl : public Thread
    , private AsyncUpdater {
public:
    struct Event {
        Event() { }
        Event(Event const& other) = default;
        Event(Event&& other) = default;

        File file;
        FileSystemEvent fsEvent;

        bool operator==(Event const& other) const
        {
            return file == other.file && fsEvent == other.fsEvent;
        }
    };

    Impl(FileSystemWatcher& o, File f)
        : Thread("FileSystemWatcher::Impl")
        , owner(o)
        , folder(f)
    {
        fd = inotify_init();

        wd = inotify_add_watch(fd,
            folder.getFullPathName().toRawUTF8(),
            IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_TO | IN_MOVED_FROM);

        startThread();
    }

    ~Impl()
    {
        shouldQuit = true;
        signalThreadShouldExit();
        inotify_rm_watch(fd, wd);
        close(fd);

        waitForThreadToExit(1000);
    }

    void run() override
    {
        char buf[BUF_LEN];
        const struct inotify_event* iNotifyEvent;
        char* ptr;

        while (!shouldQuit) {
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLIN;

            // Poll with 100ms timeout
            int pollResult = poll(&pfd, 1, 100);

            if (threadShouldExit())
                break; // Check exit condition regularly

            if (pollResult > 0 && (pfd.revents & POLLIN)) {
                int numRead = read(fd, buf, BUF_LEN);
                if (numRead <= 0 || threadShouldExit())
                    break;

                for (ptr = buf; ptr < buf + numRead; ptr += sizeof(struct inotify_event) + iNotifyEvent->len) {
                    iNotifyEvent = (const struct inotify_event*)ptr;
                    Event e;
                    e.file = File { folder.getFullPathName() + '/' + iNotifyEvent->name };
                    if (iNotifyEvent->mask & IN_CREATE)
                        e.fsEvent = FileSystemEvent::fileCreated;
                    else if (iNotifyEvent->mask & IN_CLOSE_WRITE)
                        e.fsEvent = FileSystemEvent::fileUpdated;
                    else if (iNotifyEvent->mask & IN_MOVED_FROM)
                        e.fsEvent = FileSystemEvent::fileRenamedOldName;
                    else if (iNotifyEvent->mask & IN_MOVED_TO)
                        e.fsEvent = FileSystemEvent::fileRenamedNewName;
                    else if (iNotifyEvent->mask & IN_DELETE)
                        e.fsEvent = FileSystemEvent::fileDeleted;
                    ScopedLock sl(lock);
                    bool duplicateEvent = false;
                    for (auto existing : events) {
                        if (e == existing) {
                            duplicateEvent = true;
                            break;
                        }
                    }
                    if (!duplicateEvent)
                        events.add(std::move(e));
                }
                ScopedLock sl(lock);
                if (events.size() > 0)
                    triggerAsyncUpdate();
            }
        }
    }

    void handleAsyncUpdate() override
    {
        if (shouldQuit)
            return;

        ScopedLock sl(lock);

        for (auto& e : events)
            owner.fileChanged(e.file, e.fsEvent);

        events.clear();
    }

    std::atomic<bool> shouldQuit = false;
    FileSystemWatcher& owner;
    File folder;

    CriticalSection lock;
    SmallArray<Event> events;

    int fd;
    int wd;
};

#elif JUCE_WINDOWS
class FileSystemWatcher::Impl : private AsyncUpdater
    , private Thread {
public:
    struct Event {
        File file;
        FileSystemEvent fsEvent;

        bool operator==(Event const& other) const
        {
            return file == other.file && fsEvent == other.fsEvent;
        }
    };

    Impl(FileSystemWatcher& o, File f)
        : Thread("FileSystemWatcher::Impl")
        , owner(o)
        , folder(f)
    {
        WCHAR path[_MAX_PATH] = { 0 };
        wcsncpy_s(path, folder.getFullPathName().toWideCharPointer(), _MAX_PATH - 1);

        folderHandle = CreateFileW(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

        if (folderHandle != INVALID_HANDLE_VALUE)
            startThread();
    }

    ~Impl() override
    {
        if (isThreadRunning()) {
            signalThreadShouldExit();

            CancelIoEx(folderHandle, nullptr);

            stopThread(1000);
        }

        CloseHandle(folderHandle);
    }

    void run() override
    {
        int const heapSize = 16 * 1024;
        uint8_t buffer[heapSize];

        DWORD bytesOut = 0;

        while (!threadShouldExit()) {
            memset(buffer, 0, heapSize);
            BOOL success = ReadDirectoryChangesW(folderHandle, buffer, heapSize, true,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                &bytesOut, nullptr, nullptr);

            if (success && bytesOut > 0) {
                ScopedLock sl(lock);

                uint8_t* rawData = buffer;
                while (true) {
                    FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)rawData;

                    Event e;
                    e.file = folder.getChildFile(String(fni->FileName, fni->FileNameLength / sizeof(wchar_t)));

                    switch (fni->Action) {
                    case FILE_ACTION_ADDED:
                        e.fsEvent = fileCreated;
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        e.fsEvent = fileRenamedNewName;
                        break;
                    case FILE_ACTION_MODIFIED:
                        e.fsEvent = fileUpdated;
                        break;
                    case FILE_ACTION_REMOVED:
                        e.fsEvent = fileDeleted;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        e.fsEvent = fileRenamedOldName;
                        break;
                    }

                    bool duplicateEvent = false;
                    for (auto existing : events) {
                        if (e == existing) {
                            duplicateEvent = true;
                            break;
                        }
                    }

                    if (!duplicateEvent)
                        events.add(e);

                    if (fni->NextEntryOffset > 0)
                        rawData += fni->NextEntryOffset;
                    else
                        break;
                }

                if (events.size() > 0)
                    triggerAsyncUpdate();
            }
        }
    }

    void handleAsyncUpdate() override
    {
        ScopedLock sl(lock);

        for (auto e : events)
            owner.fileChanged(e.file, e.fsEvent);

        events.clear();
    }

    FileSystemWatcher& owner;
    File const folder;

    CriticalSection lock;
    SmallArray<Event> events;

    HANDLE folderHandle;
};

// Dummy implementation for OS where we don't support this yet
#elif JUCE_BSD
class FileSystemWatcher::Impl {
public:
    Impl(FileSystemWatcher& o, File f)
        : owner(o)
        , folder(f)
    {
    }

    ~Impl()
    {
    }

    FileSystemWatcher& owner;
    File const folder;
};
#endif

#if defined JUCE_WINDOWS || defined JUCE_LINUX || defined JUCE_BSD
FileSystemWatcher::FileSystemWatcher()
{
}

FileSystemWatcher::~FileSystemWatcher()
{
}

void FileSystemWatcher::addFolder(File const& folder)
{
    SmallArray<File> allFolders;
    for (auto w : watched)
        allFolders.add(w->folder);

    if (!allFolders.contains(folder))
        watched.add(new Impl(*this, folder));
}

void FileSystemWatcher::removeFolder(File const& folder)
{
    for (int i = watched.size(); --i >= 0;) {
        if (watched[i]->folder == folder) {
            watched.remove(i);
            break;
        }
    }
}

void FileSystemWatcher::removeAllFolders()
{
    watched.clear();
}
#endif
