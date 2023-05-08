/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_gui_basics/juce_gui_basics.h>

#include <juce_core/juce_core.h>
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
    auto* display = juce::XWindowSystem::getInstance()->getDisplay();
    auto& atoms = juce::XWindowSystem::getInstance()->getAtoms();
    
    juce::XWindowSystemUtilities::ScopedXLock xLock;
    juce::XWindowSystemUtilities::GetXProperty prop (display, (::Window)handle, atoms.state, 0, 64, false, atoms.state);

    if (prop.success && prop.actualType == atoms.state
        && prop.actualFormat == 32 && prop.numItems > 0)
    {
        unsigned long state;
        memcpy (&state, prop.data, sizeof (unsigned long));

        return state == IconicState;
    }

    return false;
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

juce::Array<juce::File> OSUtils::iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles)
{
    juce::Array<juce::File> result;

    if (recursive) {
        for (auto const& dirEntry : std::filesystem::recursive_directory_iterator(directory.getFullPathName().toStdString())) {
            auto isDir = dirEntry.is_directory();
            if ((isDir && !onlyFiles) || !isDir) {
                result.add(juce::File(dirEntry.path().string()));
            }
        }
    } else {
        for (auto const& dirEntry : std::filesystem::directory_iterator(directory.getFullPathName().toStdString())) {
            auto isDir = dirEntry.is_directory();
            if ((isDir && !onlyFiles) || !isDir) {
                result.add(juce::File(dirEntry.path().string()));
            }
        }
    }

    return result;
}

// Otherwise use cpath
#else

static juce::Array<juce::File> iterateDirectoryRecurse(cpath::Dir&& dir, bool recursive, bool onlyFiles)
{
    juce::Array<juce::File> result;

    while (cpath::Opt<cpath::File, cpath::Error::Type> file = dir.GetNextFile()) {
        auto isDir = file->IsDir();

        if (isDir && recursive && !file->IsSpecialHardLink()) {
            result.addArray(iterateDirectoryRecurse(std::move(file->ToDir().GetRaw()), recursive, onlyFiles));
        }
        if ((isDir && !onlyFiles) || !isDir) {
            result.add(juce::File(juce::String(file->GetPath().GetRawPath()->buf)));
        }
    }

    dir.Close();

    return result;
}

juce::Array<juce::File> OSUtils::iterateDirectory(juce::File const& directory, bool recursive, bool onlyFiles)
{
    auto pathName = directory.getFullPathName();
    auto dir = cpath::Dir(pathName.toRawUTF8());
    return iterateDirectoryRecurse(std::move(dir), recursive, onlyFiles);
}

#endif
