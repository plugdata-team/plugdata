/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

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
#    include <string>
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

KeyboardLayout OSUtils::getKeyboardLayout()
{
    TCHAR buff[KL_NAMELENGTH];
    bool result = GetKeyboardLayoutNameA(buff);

    if(buff == "French" || buff == "Belgian French" || buff == "Belgian (Comma)" || buff == "Belgian (Period)") {
        return AZERTY;
    }
        
    return QWERTY;
}

#endif // Windows

// Selects Linux and BSD
#if defined(__unix__) && !defined(__APPLE__)
extern "C" {
#    include <X11/Xlib.h>
#    include <X11/Xatom.h>
}


bool OSUtils::isMaximised(void* handle)
{
    typedef enum {
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
    } window_state_t;

    typedef struct {

        Display* dpy;
        Window id;

        struct {
            Atom NET_WM_STATE;
            Atom NET_WM_STATES[WINDOW_STATE_SIZE];
        } atoms;

    } window_t;

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
    
    window_t win;
    auto window = (Window)handle;
    auto* display = XOpenDisplay(nullptr);

    win.id = window;
    win.dpy = display;

    win.atoms.NET_WM_STATE = XInternAtom(win.dpy, "_NET_WM_STATE", False);

    for (int i = 0; i < WINDOW_STATE_SIZE; ++i) {
        win.atoms.NET_WM_STATES[i] = XInternAtom(win.dpy, WINDOW_STATE_NAMES[i], False);
    }

    long max_length = 1024;
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after, i, num_states = 0;
    Atom* states = nullptr;
    window_state_t state = WINDOW_STATE_NONE;

    if (XGetWindowProperty(win.dpy,
            win.id,
            win.atoms.NET_WM_STATE,
            0l,
            max_length,
            False,
            XA_ATOM,
            &actual_type,
            &actual_format,
            &num_states,
            &bytes_after,
            (unsigned char**)&states)
        == Success) {
        // for every state we get from the server
        for (i = 0; i < num_states; ++i) {

            // for every (known) state
            for (int n = 0; n < WINDOW_STATE_SIZE; ++n) {

                // test the state at index i
                if (states[i] == win.atoms.NET_WM_STATES[n]) {

                    state = static_cast<window_state_t>(static_cast<int>(state) | (1 << n));
                    break;
                }
            }
        }

        XFree(states);
    }

    return state & WINDOW_STATE_MAXIMIZED;
}

void OSUtils::maximiseLinuxWindow(void* handle)
{
    auto win = (Window)handle;
    auto* display = XOpenDisplay(nullptr);

    XEvent ev;
    ev.xclient.window = win;
    ev.xclient.type = ClientMessage;
    ev.xclient.format = 32;
    ev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
    ev.xclient.data.l[0] = 2;
    ev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    ev.xclient.data.l[2] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    ev.xclient.data.l[3] = 1;

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);

    XCloseDisplay(display);
}

KeyboardLayout OSUtils::getKeyboardLayout()
{
    char   *line = NULL;
    size_t  size = 0;
    ssize_t len;
    KeyboardLayout result = QWERTY;
    FILE *in;
    
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
