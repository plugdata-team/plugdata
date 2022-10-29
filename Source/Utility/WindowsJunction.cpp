#if defined (_WIN32) || defined (_WIN64)

#define REPARSE_MOUNTPOINT_HEADER_SIZE   8

#define _WIN32_WINNT		0x0500		// Windows 2000 or later
#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS

#include <windows.h>
#include <WINIOCTL.H>
#include <shlobj.h>

#include <stdio.h>
#include <string>
#include <filesystem>

typedef struct {
    DWORD ReparseTag;
    DWORD ReparseDataLength;
    WORD Reserved;
    WORD ReparseTargetLength;
    WORD ReparseTargetMaximumLength;
    WORD Reserved1;
    WCHAR ReparseTarget[1];
} REPARSE_MOUNTPOINT_DATA_BUFFER, * PREPARSE_MOUNTPOINT_DATA_BUFFER;

void createJunction(std::string from, std::string to) {
    
    auto szJunction = (LPCTSTR)from.c_str();
    auto szPath = (LPCTSTR)to.c_str();
    
    BYTE buf[sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) + MAX_PATH * sizeof(WCHAR)];
    REPARSE_MOUNTPOINT_DATA_BUFFER& ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER&)buf;
    char szTarget[MAX_PATH] = "\\??\\";
    
    strcat(szTarget, szPath);
    strcat(szTarget, "\\");
    
    if (!::CreateDirectory(szJunction, NULL)) throw ::GetLastError();
    
    // Obtain SE_RESTORE_NAME privilege (required for opening a directory)
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    try {
        if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) throw ::GetLastError();
        if (!::LookupPrivilegeValue(NULL, SE_RESTORE_NAME, &tp.Privileges[0].Luid))  throw ::GetLastError();
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))  throw ::GetLastError();
    }
    catch (DWORD) { }   // Ignore errors
    if (hToken) ::CloseHandle(hToken);
    
    HANDLE hDir = ::CreateFile(szJunction, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hDir == INVALID_HANDLE_VALUE) throw ::GetLastError();
    
    memset(buf, 0, sizeof(buf));
    ReparseBuffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    int len = ::MultiByteToWideChar(CP_ACP, 0, szTarget, -1, ReparseBuffer.ReparseTarget, MAX_PATH);
    ReparseBuffer.ReparseTargetMaximumLength = (len--) * sizeof(WCHAR);
    ReparseBuffer.ReparseTargetLength = len * sizeof(WCHAR);
    ReparseBuffer.ReparseDataLength = ReparseBuffer.ReparseTargetLength + 12;
    
    DWORD dwRet;
    if (!::DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, &ReparseBuffer, ReparseBuffer.ReparseDataLength+REPARSE_MOUNTPOINT_HEADER_SIZE, NULL, 0, &dwRet, NULL)) {
        DWORD dr = ::GetLastError();
        ::CloseHandle(hDir);
        ::RemoveDirectory(szJunction);
        throw dr;
    }
    
    ::CloseHandle(hDir);
}

void createHardLink(std::string from, std::string to) {
    std::filesystem::create_hard_link(from, to);
}

#endif
