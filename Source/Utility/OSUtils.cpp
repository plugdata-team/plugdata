/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#if !defined(__APPLE__)
#    include <raw_keyboard_input/raw_keyboard_input.cpp>
#endif

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_gui_basics/juce_gui_basics.h>
#include "OSUtils.h"

#if defined(__APPLE__)
#    define HAS_STD_FILESYSTEM 0
#elif defined(__unix__)
#    if defined(__cpp_lib_filesystem) || defined(__cpp_lib_experimental_filesystem)
#        define HAS_STD_FILESYSTEM 1
#    else
#        define HAS_STD_FILESYSTEM 0
#    endif
#elif defined(_WIN32) || defined(_WIN64)
#    define HAS_STD_FILESYSTEM 1
#endif

#if HAS_STD_FILESYSTEM
#    if defined(__cpp_lib_filesystem)
#        include <filesystem>
#    elif defined(__cpp_lib_experimental_filesystem)
#        include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#    endif
#else

#    include "../Libraries/cpath/cpath.h"

#endif

#if defined(_WIN32) || defined(_WIN64)

#    define REPARSE_MOUNTPOINT_HEADER_SIZE 8

#    define _WIN32_WINNT 0x0500 // Windows 2000 or later
#    define WIN32_LEAN_AND_MEAN
#    define WIN32_NO_STATUS

#    include <windows.h>
#    include <WINIOCTL.H>
#    include <shlobj.h>
#    include <ShellAPI.h>

#    include <stdio.h>
#    include <filesystem>

void OSUtils::createJunction(std::string from, std::string to)
{

    typedef struct {
        DWORD ReparseTag;
        DWORD ReparseDataLength;
        WORD Reserved;
        WORD ReparseTargetLength;
        WORD ReparseTargetMaximumLength;
        WORD Reserved1;
        WCHAR ReparseTarget[1];
    } REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

    auto szJunction = (LPCTSTR)from.c_str();
    auto szPath = (LPCTSTR)to.c_str();

    BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
    char szTarget[MAX_PATH] = "\\??\\";

    strcat(szTarget, szPath);
    strcat(szTarget, "\\");

    if (!::CreateDirectory(szJunction, nullptr))
        throw ::GetLastError();

    // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
    HANDLE hToken = nullptr;
    TOKEN_PRIVILEGES tp;
    try {
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
            throw ::GetLastError();
        if (!::LookupPrivilegeValue(nullptr, SE_RESTORE_NAME, &tp.Privileges[0].Luid))
            throw ::GetLastError();
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw ::GetLastError();
    } catch (DWORD) {
    } // Ignore errors
    if (hToken)
        ::CloseHandle(hToken);

    HANDLE hDir = ::CreateFile(szJunction, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (hDir == INVALID_HANDLE_VALUE)
        throw ::GetLastError();

    memset(buf, 0, sizeof(buf));
    ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = ::MultiByteToWideChar(CP_ACP, 0, szTarget, -1, ReparseBuffer.ReparseTarget, MAX_PATH);
    ReparseBuffer.ReparseTargetMaximumLength = (len--) * sizeof(WCHAR);
    ReparseBuffer.ReparseTargetLength = len * sizeof(WCHAR);
    ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;

    DWORD dwRet;
    if (!::DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer, ReparseBuffer.ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE, nullptr, 0, &dwRet, nullptr)) {
        DWORD dr = ::GetLastError();
        ::CloseHandle(hDir);
        ::RemoveDirectory(szJunction);
        throw dr;
    }

    ::CloseHandle(hDir);
}

void OSUtils::createHardLink(std::string from, std::string to)
{
    std::filesystem::create_hard_link(from, to);
}

// Function to run a command as admin on Windows
// It should spawn a dialog, asking for permissions
bool OSUtils::runAsAdmin(std::string command, std::string parameters, void* hWndPtr)
{

    HWND hWnd = (HWND)hWndPtr;
    auto lpFile = (LPCTSTR)command.c_str();
    auto lpParameters = (LPCTSTR)parameters.c_str();

    BOOL retval;
    SHELLEXECUTEINFO sei;
    ZeroMemory(&sei, sizeof(sei));

    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.hwnd = hWnd;
    sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NO_CONSOLE;
    sei.lpVerb = TEXT("runas");
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.nShow = SW_SHOWNORMAL;
    retval = ShellExecuteEx(&sei);

    return (bool)retval;
}

OSUtils::KeyboardLayout OSUtils::getKeyboardLayout()
{
    TCHAR buff[KL_NAMELENGTH];
    bool result = GetKeyboardLayoutNameA(buff);

    if (buff == "French" || buff == "Belgian French" || buff == "Belgian (Comma)" || buff == "Belgian (Period)") {
        return AZERTY;
    }

    return QWERTY;
}

#endif // Windows

// Selects Linux and BSD
#if defined(__unix__) && !defined(__APPLE__)

bool OSUtils::isX11WindowMaximised(void* handle)
{
    enum window_state_t {
        WINDOW_STATE_NONE = 0,
        WINDOW_STATE_MODAL = (1 << 0),
        WINDOW_STATE_STICKY = (1 << 1),
        WINDOW_STATE_MAXIMIZED_VERT = (1 << 2),
        WINDOW_STATE_MAXIMIZED_HORZ = (1 << 3),
        WINDOW_STATE_MAXIMIZED = (WINDOW_STATE_MAXIMIZED_VERT | WINDOW_STATE_MAXIMIZED_HORZ),
        WINDOW_STATE_SHADED = (1 << 4),
        WINDOW_STATE_SKIP_TASKBAR = (1 << 5),
        WINDOW_STATE_SKIP_PAGER = (1 << 6),
        WINDOW_STATE_HIDDEN = (1 << 7),
        WINDOW_STATE_FULLSCREEN = (1 << 8),
        WINDOW_STATE_ABOVE = (1 << 9),
        WINDOW_STATE_BELOW = (1 << 10),
        WINDOW_STATE_DEMANDS_ATTENTION = (1 << 11),
        WINDOW_STATE_FOCUSED = (1 << 12),
        WINDOW_STATE_SIZE = 13,
    };

    /* state names */
    static char const* WINDOW_STATE_NAMES[] = {
        "_NET_WM_STATE_MODAL",
        "_NET_WM_STATE_STICKY",
        "_NET_WM_STATE_MAXIMIZED_VERT",
        "_NET_WM_STATE_MAXIMIZED_HORZ",
        "_NET_WM_STATE_SHADED",
        "_NET_WM_STATE_SKIP_TASKBAR",
        "_NET_WM_STATE_SKIP_PAGER",
        "_NET_WM_STATE_HIDDEN",
        "_NET_WM_STATE_FULLSCREEN",
        "_NET_WM_STATE_ABOVE",
        "_NET_WM_STATE_BELOW",
        "_NET_WM_STATE_DEMANDS_ATTENTION",
        "_NET_WM_STATE_FOCUSED"
    };

    auto* display = juce::XWindowSystem::getInstance()->getDisplay();

    juce::XWindowSystemUtilities::ScopedXLock xLock;

    Atom net_wm_state;
    Atom net_wm_states[WINDOW_STATE_SIZE];

    auto window = (Window)handle;

    if (!display)
        return false;

    net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);

    for (int i = 0; i < WINDOW_STATE_SIZE; i++) {
        net_wm_states[i] = XInternAtom(display, WINDOW_STATE_NAMES[i], False);
    }

    long max_length = 1024;
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after, i, num_states = 0;
    Atom* states = nullptr;
    window_state_t state = WINDOW_STATE_NONE;

    if (XGetWindowProperty(display, window, net_wm_state, 0l, max_length, False, 4,
            &actual_type, &actual_format, &num_states, &bytes_after, (unsigned char**)&states)
        == Success) {

        // for every state we get from the server
        for (i = 0; i < num_states; ++i) {
            // for every (known) state
            for (int n = 0; n < WINDOW_STATE_SIZE; ++n) {
                // test the state at index i
                if (states[i] == net_wm_states[n]) {
                    state = static_cast<window_state_t>(static_cast<int>(state) | (1 << n));
                    break;
                }
            }
        }
        XFree(states);
    }

    return state & WINDOW_STATE_MAXIMIZED;
}

void OSUtils::maximiseX11Window(void* handle, bool shouldBeMaximised)
{
    juce::XWindowSystem::getInstance()->setMaximised((::Window)handle, shouldBeMaximised);
}

OSUtils::KeyboardLayout OSUtils::getKeyboardLayout()
{
    char* line = NULL;
    size_t size = 0;
    ssize_t len;
    KeyboardLayout result = QWERTY;
    FILE* in;

    in = popen("LANG=C LC_ALL=C setxkbmap -print", "rb");
    if (!in)
        return QWERTY;

    while (1) {
        len = getline(&line, &size, in);
        if (strstr(line, "aliases(qwerty)"))
            result = QWERTY;
        if (strstr(line, "aliases(azerty)"))
            result = AZERTY;
    }

    free(line);
    pclose(in);

    return result;
}
#endif // Linux/BSD

// Use std::filesystem directory iterator if available
// On old versions of GCC and macos <10.15, std::filesystem is not available
#if HAS_STD_FILESYSTEM

juce::Array<juce::File> OSUtils::iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles, int maximum)
{
    juce::Array<juce::File> result;

    if (recursive) {
        try {
            for (auto const& dirEntry : std::filesystem::recursive_directory_iterator(directory.getFullPathName().toStdString())) {
                auto isDir = dirEntry.is_directory();
                if ((isDir && !onlyFiles) || !isDir) {
                    result.add(juce::File(dirEntry.path().string()));
                }

                if (maximum > 0 && result.size() >= maximum)
                    break;
            }
        } catch (std::filesystem::filesystem_error e) {
            std::cerr << "Error while iterating over directory: " << e.path1() << std::endl;
        }
    } else {
        try {
            for (auto const& dirEntry : std::filesystem::directory_iterator(directory.getFullPathName().toStdString())) {
                auto isDir = dirEntry.is_directory();
                if ((isDir && !onlyFiles) || !isDir) {
                    result.add(juce::File(dirEntry.path().string()));
                }

                if (maximum > 0 && result.size() >= maximum)
                    break;
            }
        } catch (std::filesystem::filesystem_error e) {
            std::cerr << "Error while iterating over directory: " << e.path1() << std::endl;
        }
    }

    return result;
}

// Otherwise use cpath
#else

static juce::Array<juce::File> iterateDirectoryRecurse(cpath::Dir&& dir, bool recursive, bool onlyFiles, int maximum)
{
    juce::Array<juce::File> result;

    while (cpath::Opt<cpath::File, cpath::Error::Type> file = dir.GetNextFile()) {
        auto isDir = file->IsDir();

        if (isDir && recursive && !file->IsSpecialHardLink()) {
            result.addArray(iterateDirectoryRecurse(file->ToDir().GetRaw(), recursive, onlyFiles, maximum));
        }
        if ((isDir && !onlyFiles) || !isDir) {
            result.add(juce::File(juce::String::fromUTF8(file->GetPath().GetRawPath()->buf)));
        }

        if (maximum > 0 && result.size() >= maximum)
            break;
    }

    dir.Close();

    return result;
}

juce::Array<juce::File> OSUtils::iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles, int maximum)
{
    auto pathName = directory.getFullPathName();
    auto dir = cpath::Dir(pathName.toRawUTF8());
    return iterateDirectoryRecurse(std::move(dir), recursive, onlyFiles, maximum);
}

#endif

// needs to be in OSutils because it needs <windows.h>
unsigned int OSUtils::keycodeToHID(unsigned int scancode)
{

#if defined(_WIN32)
    static unsigned char const KEYCODE_TO_HID[256] = {
        0, 41, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46, 42, 43, 20, 26, 8, 21, 23, 28, 24, 12, 18, 19,
        47, 48, 158, 224, 4, 22, 7, 9, 10, 11, 13, 14, 15, 51, 52, 53, 225, 49, 29, 27, 6, 25, 5, 17, 16, 54,
        55, 56, 229, 0, 226, 0, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 72, 71, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 68, 69, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 228, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 230, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 74, 82, 75, 0, 80, 0, 79, 0, 77, 81, 78, 73, 76, 0, 0, 0, 0, 0, 0, 0, 227, 231,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    static HKL layout;

    scancode = MapVirtualKeyEx(scancode, MAPVK_VK_TO_VSC, layout);
    if (scancode >= 256)
        return (0);
    return (KEYCODE_TO_HID[scancode]);

#elif defined(__APPLE__)
    static unsigned char const KEYCODE_TO_HID[128] = {
        4, 22, 7, 9, 11, 10, 29, 27, 6, 25, 0, 5, 20, 26, 8, 21, 28, 23, 30, 31, 32, 33, 35, 34, 46, 38, 36,
        45, 37, 39, 48, 18, 24, 47, 12, 19, 158, 15, 13, 52, 14, 51, 49, 54, 56, 17, 16, 55, 43, 44, 53, 42,
        0, 41, 231, 227, 225, 57, 226, 224, 229, 230, 228, 0, 108, 220, 0, 85, 0, 87, 0, 216, 0, 0, 127,
        84, 88, 0, 86, 109, 110, 103, 98, 89, 90, 91, 92, 93, 94, 95, 111, 96, 97, 0, 0, 0, 62, 63, 64, 60,
        65, 66, 0, 68, 0, 104, 107, 105, 0, 67, 0, 69, 0, 106, 117, 74, 75, 76, 61, 77, 59, 78, 58, 80, 79,
        81, 82, 0
    };

    if (scancode >= 128)
        return 0;
    return KEYCODE_TO_HID[scancode];
#else

    static unsigned char const KEYCODE_TO_HID[256] = {
        0, 41, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 45, 46, 42, 43, 20, 26, 8, 21, 23, 28, 24, 12, 18, 19,
        47, 48, 158, 224, 4, 22, 7, 9, 10, 11, 13, 14, 15, 51, 52, 53, 225, 49, 29, 27, 6, 25, 5, 17, 16, 54,
        55, 56, 229, 85, 226, 44, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 83, 71, 95, 96, 97, 86, 92,
        93, 94, 87, 89, 90, 91, 98, 99, 0, 0, 100, 68, 69, 0, 0, 0, 0, 0, 0, 0, 88, 228, 84, 154, 230, 0, 74,
        82, 75, 80, 79, 77, 81, 78, 73, 76, 0, 0, 0, 0, 0, 103, 0, 72, 0, 0, 0, 0, 0, 227, 231, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 118, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // xorg differs from kernel values
    scancode -= 8;
    if (scancode >= 256)
        return 0;
    return KEYCODE_TO_HID[scancode];
#endif
}
