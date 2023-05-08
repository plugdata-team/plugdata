/*
MIT License

Copyright (c) 2019 Braedon Wooding

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CPATH_H
#define CPATH_H

#ifdef __cplusplus
extern "C" {
#endif

/*
    Note this library also works with C++ via a C++ provided binding
    which is available for free just by including this on a C++ compiler
    or with __cplusplus defined

    If you want:
    - Unicode just add a #define CPATH_UNICODE or UNICODE or _UNICODE
    - Custom Allocators just #define:
        - CPATH_MALLOC and CPATH_FREE (make sure to define both)
*/

/*
    Note on why this library offers no guarantees on reentrant systems
    or no TOUTOC (and other race conditions):
    - Often supporting this requires breaking a ton of compatability
        and often the behaviour of each of the individual commands differs way
        too much to make it consistent.  Even when fully following it we can't
        guarantee that you won't just misuse it and still cause the race conditions
    - The majority (99%) of cases simply just don't care about it how often
        are your files being written over...
    - We have extra safety on things like opening directories and recursive
        solutions to make sure that things like that don't happen.
    - How do you want to handle it properly?  You may want to just keep going
        or you may want some way to unwind what you previously did, or just take
        a snapshot of the system... It is too varied for us to offer a generalised
        way.
    - The majority of libraries don't offer it or they offer it to an extremely
        limited number of commands with the others not having it, in my opinion
        this is worse than just not offering it.  Atleast we are consistent with
        our guarantees
    - File Systems are a mess and already are basically a gigantic global mess
        trying to guarantee any sort of safety is not only a complexity mess but
        also can give the wrong idea.
    - You should be able to detect hard failures due to the change in a folder
        and simply just re-call.  Of course this could happen repeatedly but
        come on... In reality it will occur once in a blue moon.
    - Reloading files is easy it is just cpathLoadFiles
*/

/*
    @TODO:
    - Windows has lifted their max path so we could use the new functions
        I don't think all functions have an alternative and it could be more messy
        still look into it eventually.
*/

// custom allocators
#if !defined CPATH_MALLOC && !defined CPATH_FREE
#define CPATH_MALLOC(size) malloc(size)
#define CPATH_FREE(ptr) free(ptr)
#elif (defined CPATH_MALLOC && !defined CPATH_FREE) || \
            (defined CPATH_FREE && !defined CPATH_MALLOC)
#error "Can't define only free or only malloc have to define both or neither"
#endif

// Support unicode
#if defined CPATH_UNICODE || (!defined UNICODE && defined _UNICODE)
#define UNICODE
#endif

#if defined CPATH_UNICODE || (defined UNICODE && !defined _UNICODE)
#define _UNICODE
#endif

#if !defined CPATH_UNICODE && (defined UNICODE || defined _UNICODE)
#define CPATH_UNICODE
#endif

#ifdef _MSC_VER
#define _CPATH_FUNC_ static __inline
#elif !defined __STDC_VERSION__ || __STDC_VERSION__ < 199901L
#define _CPATH_FUNC_ static __inline__
#else
#define _CPATH_FUNC_ static inline
#endif

/* == #includes == */

#if defined _MSC_VER || defined __MINGW32__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef _MSC_VER
#include <tchar.h>
// Ignore all warnings to do with std functions not being 100% safe
// they are fine
#pragma warning(push)
#pragma warning (disable : 4996)
#else
#include <dirent.h>
#include <stddef.h>
#include <libgen.h>
#include <sys/stat.h>
#endif

// NOTE: This has to appear before defined BSD
//       since sys/param.h defines BSD given environment
#if defined __unix__ || (defined __APPLE__ && defined __MACH__)
#include <sys/param.h>
#endif

#if defined __linux__ || defined BSD
#include <limits.h>
#endif

#ifdef __MINGW32__
#include <tchar.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>

// Linux has a max of 255 (+1 for \0) I couldn't find a max on windows
// But since 260 > 256 it is a reasonable value that should be crossplatform
#define CPATH_MAX_FILENAME_LEN (256)

/* Windows Unicode Support */
#if defined _MSC_VER || defined __MINGW32__
#define CPATH_STR(str) _TEXT(str)
#define cpath_str_length _tcslen
#define cpath_str_copy _tcscpy
#define cpath_strn_copy _tcsncpy
#define cpath_str_find_last_char _tcsrchr
#define cpath_str_compare _tcscmp
#define cpath_str_compare_safe _tcsncmp
#define cpath_fopen _tfopen
#define cpath_sprintf _stprintf
#define cpath_vsnprintf _vsntprintf
#else
#define CPATH_STR(str) str
#define cpath_str_length strlen
#define cpath_str_copy strcpy
#define cpath_strn_copy strncpy
#define cpath_str_find_last_char strrchr
#define cpath_str_compare strcmp
#define cpath_str_compare_safe strncmp
#define cpath_fopen fopen
#define cpath_sprintf sprintf
#define cpath_vsnprintf vsnprintf
#endif

#if defined _MSC_VER || defined __MINGW32__
#define CPATH_MAX_PATH_LEN MAX_PATH
#elif defined __linux__ || defined BSD

// @TODO: Do we add one? for \0
// I have found conflicting info
// we definitely don't for windows
#ifdef PATH_MAX
#define CPATH_MAX_PATH_LEN PATH_MAX
#endif

#endif

// Found this value online in a few other libraries
// so I'll just continue this tradition/standard despite not knowing why
// @A: This seems to be the max on Linux so I suppose its a reasonable value
#ifndef CPATH_MAX_PATH_LEN
#define CPATH_MAX_PATH_LEN (4096)
#endif

// on windows we need extra space for the \* mask that is required
// yes this is very weird...  Maybe we can just chuck it onto the max path?
// I don't think we can because it seems that it is independent of the max path
// i.e. if you can have a max path of 256 characters this includes the extra
//      in this case we could just subtract it from the max path
//      but then again we may not always want to apply the mask (and we don't)
//      so that isn't a great solution either...
// @TODO: Figure this stuff out.
#ifdef _MSC_VER
#define CPATH_PATH_EXTRA_CHARS (2)
#else
#define CPATH_PATH_EXTRA_CHARS (0)
#endif

/*
     NOTE: This isn't the maximum number of drives (that would be 26)
                 This is the length of a drive prepath i.e. D:\ which is clearly 3

                 No such max exists on other systems to the best of my knowledge

        By defining it like (static_assert(0, msg), 1) we make sure that we don't
        have a compiler syntax error in it's typical usage we prefer having
        the error from having the static assert rather than the error for having
        an incorrect token.
        @TODO: Do we actually need this???
*/
#if defined _MSC_VER
#define FILE_IS(f, flag) !!(f.dwFileAttributes & FILE_ATTRIBUTE_##flag)
#define FILE_IS_NOT(f, flag) !(f.dwFileAttributes & FILE_ATTRIBUTE_##flag)
#endif

#if defined _MSC_VER || defined __MINGW32__
#define CPATH_MAX_DRIVE_LEN (3)
#elif __cpp_static_assert
#define CPATH_MAX_DRIVE_LEN \
        (static_assert(0, "You shouldn't use max drive length on non windows"), 1)
#elif __STDC_VERSION__ >= 201112L
#define CPATH_MAX_DRIVE_LEN \
        (_Static_assert(0, "You shouldn't use max drive length on non windows"), 1)
#else
#define CPATH_MAX_DRIVE_LEN \
        ("You shouldn't use max drive length on non windows", )
#endif

#if defined _WIN32
    #include <direct.h>
    #define _cpath_getcwd _getcwd
#else
    #include <unistd.h> // for getcwd()
    #define _cpath_getcwd getcwd
#endif

#if !defined _MSC_VER
#if defined __MINGW32__ && defined _UNICODE
#define _cpath_opendir _wopendir
#define _cpath_readdir _wreaddir
#define _cpath_closedir _wclosedir
#else
#define _cpath_opendir opendir
#define _cpath_readdir readdir
#define _cpath_closedir closedir
#endif
#endif

#if defined CPATH_FORCE_CONVERSION_SYSTEM
#if defined _MSC_VER || defined __MINGW32__
#define CPATH_SEP CPATH_STR('\\')
#define CPATH_OTHER_SEP CPATH_STR('/')
#else
#define CPATH_SEP CPATH_STR('/')
#define CPATH_OTHER_SEP CPATH_STR('\\')
#endif
#else
#define CPATH_SEP CPATH_STR('/')
#define CPATH_OTHER_SEP CPATH_STR('\\')
#endif

#if !defined CPATH_NO_CPP_BINDINGS && defined __cplusplus
    namespace cpath { namespace internals {
#endif

// @NOTE: gdb/lldb can't print our strings
//        for some reason they don't understand the typedefs
//        however they will understand a #define cause they'll
//        just see it as a char symbol.
// @BUG: @TODO: Make it so it can be a typedef...
//              Currently I just have decided to make it default to this
//              Worse case we introduce a #define
#if defined _MSC_VER || defined __MINGW32__
// todo
typedef int cpath_offset_t;
typedef int cpath_time_t;
typedef TCHAR cpath_char_t;
#else
typedef char cpath_char_t;
typedef off_t cpath_offset_t;
typedef time_t cpath_time_t;
#endif

#if !defined _MSC_VER
#if defined __MINGW32__ && defined _UNICODE
typedef _WDIR cpath_dirdata_t;
typedef struct _wdirent cpath_dirent_t;
#else
typedef DIR cpath_dirdata_t;
typedef struct dirent cpath_dirent_t;
#endif
#endif

typedef cpath_char_t *cpath_str;
typedef int(*cpath_err_handler)();
typedef int(*cpath_cmp)(const void*, const void*);
typedef struct cpath_t {
    cpath_char_t buf[CPATH_MAX_PATH_LEN];
    size_t len;
} cpath;

typedef struct cpath_file_t {
    int isDir;
    int isReg;
    int isSym;

#if !defined _MSC_VER
#if defined __MINGW32__
        struct _stat stat;
#else
        struct stat stat;
#endif
#endif

    int statLoaded;

    cpath path;
    cpath_char_t name[CPATH_MAX_FILENAME_LEN];
    cpath_str extension;
} cpath_file;

typedef struct cpath_dir_t {
    cpath_file *files;
    size_t size;

#ifdef _MSC_VER
    HANDLE handle;
    WIN32_FIND_DATA findData;
#else
    cpath_dirdata_t *dir;
    cpath_dirent_t *dirent;
#endif

    // This is set whenever you do an emplace
    // This allows you to revert an emplace
    struct cpath_dir_t *parent;

    int hasNext;

    cpath path;
} cpath_dir;

// This allows a forced type
typedef uint8_t CPathByteRep;
enum CPathByteRep_ {
    BYTE_REP_JEDEC          = 0,  // KB = 1024, ...
    BYTE_REP_DECIMAL        = 1,  // 1000 interval segments, B, kB, MB, GB, ...
    BYTE_REP_IEC            = 2,  // KiB = 1024, ...
    BYTE_REP_DECIMAL_UPPER  = 3,  // 1000 interval segments but B, KB, MB, GB, ...
    BYTE_REP_DECIMAL_LOWER  = 4,  // 1000 interval segments but b, kb, mb, gb, ...

    // representations
    BYTE_REP_LONG           = 0b10000000, // Represent as words i.e. kibibytes
    BYTE_REP_BYTE_WORD      = 0b01000000, // Just B as Byte (only bytes)
};

typedef void(*cpath_traverse_it)(
    cpath_file *file, cpath_dir *parent, int depth,
    void *data
);

/* == Declarations == */

/* == Path == */

/*
    A clear path is always just '.'

    When constructing a path it will always ignore extra / or \ at the end
    and multiple in a row.

    i.e. C://a\b\\c\ is just C:/a/b/c/

    If you wish to force conversion to the system specific one
    (/ on posix \ on windows) then just add
    #define CPATH_FORCE_CONVERSION_SYSTEM
    i.e. on Windows it will be C:\a\b\c on Posix it will be C:/a/b/c
*/

/*
    Does a strcpy but converts all separators and ignores multiple in a row
    Returns how many characters were copied.
*/
_CPATH_FUNC_
size_t cpathStrCpyConv(cpath_str dest, size_t len, const cpath_char_t *src);

/*
    Trim all trailing / or \ from a path
*/
_CPATH_FUNC_
void cpathTrim(cpath *path);

/*
    Construct a path from a utf8 string
*/
_CPATH_FUNC_
cpath cpathFromUtf8(const char *str);

/*
    Copy a path
*/
_CPATH_FUNC_
void cpathCopy(cpath *out, const cpath *in);

/*
    Construct a path from a system typed string.
*/
_CPATH_FUNC_
int cpathFromStr(cpath *out, const cpath_char_t *str);

/*
    Concatenate a path with a literal
*/
#define CPATH_CONCAT_LIT(path, other) cpathConcatStr(path, CPATH_STR(other))

/*
    Appends to a path with a literal, wont add a /
*/
#define CPATH_APPEND_LIT(path, other) cpathAppendStr(path, CPATH_STR(other))

/*
    Append to the cpath buffer.
    NOTE: Won't add a /
    There is no concat version since it would be too inefficient
    and thus encourage bad habits.  Just add a / everytime you call this
*/
_CPATH_FUNC_
void cpathAppendSprintf(cpath *out, const cpath_char_t *fmt, ...);

/*
    Appends to the path (not adding /)
*/
_CPATH_FUNC_
int cpathAppendStrn(cpath *out, const cpath_char_t *other, size_t len);

/*
    Appends to the path (not adding /)
*/
_CPATH_FUNC_
int cpathAppendStr(cpath *out, const cpath_char_t *other);

/*
    Appends to the path (not adding /)
*/
_CPATH_FUNC_
int cpathAppend(cpath *out, const cpath *other);

/*
    Concatenate a path (adds / for you) from a system typed string
*/
_CPATH_FUNC_
int cpathConcatStrn(cpath *out, const cpath_char_t *other, size_t len);

/*
    Concatenate a path (adds / for you) from a system typed string
*/
_CPATH_FUNC_
int cpathConcatStr(cpath *out, const cpath_char_t *other);

/*
    Concat a path with another.
*/
_CPATH_FUNC_
int cpathConcat(cpath *out, const cpath *other);

/*
    Does the path exist.
*/
_CPATH_FUNC_
int cpathExists(const cpath *path);

/*
    Attempts to canonicalise without system calls
    Will fail in cases where it can't predict the path
    i.e. C:/a/../b will be just b
*/
_CPATH_FUNC_
int cpathCanonicaliseNoSysCall(cpath *out, cpath *path);

/*
    Resolves the path.  Involves system calls.
*/
_CPATH_FUNC_
int cpathCanonicalise(cpath *out, cpath *path);

/*
    Tries to canonicalise without any system calls
    then resorts to the system call version if it fails
*/
_CPATH_FUNC_
int cpathCanonicaliseAvoidSysCall(cpath *out, cpath *path);

/*
    Clears a given path.
*/
_CPATH_FUNC_
void cpathClear(cpath *path);

/*
    Iterate through every component.  Doesn't include the \
    i.e. \usr\local\bin\myfile.so will be '' 'usr' 'local' 'bin' 'myfile.so'

    If you wish to iterate from the beginning set index to 0.  Else set index
    to whatever index you wish to iterate from.  It'll restore the previous
    segments as it goes meaning on completion the path will be fully restored.

    NOTE: Modifies the path buffer so you shouldn't use the path for anything
                Until this returns NULL (indicating no more path segments)
                OR you use cpathItRefRestore() note that on the last path segment
                it obviously won't have to add an '\0' so it'll also work if you bail
                out then.  It is hard to detect that though.

    You can strdup it yourself or do a memcpy if you care about editing it
*/
_CPATH_FUNC_
const cpath_char_t *cpathItRef(cpath *path, int *index);

/*
    Restores the path to a usable state.
    ONLY needs to be called if the path is wanted to be usable and you haven't
             iterated through all the references.
    If the index isn't valid it'll cycle through the path fixing it up.
    This is just for the sake of it, it'll make things easier and less prone
    to breaking.

    Do note that this will update index so that you can use it again with
    it ref to get the resumed behaviour you would expect (effectively just a ++)
*/
_CPATH_FUNC_
void cpathItRefRestore(cpath *path, int *index);

/*
    Go up a directory effectively going back by one /
    i.e. C:/D/E/F/g.c => C:/D/E/F and so on...
    Returns true if it succeeded
    Could possibly involve a canonicalise if the path is a symlink
    i.e. ./ has to be resolved and so does ~/
*/
_CPATH_FUNC_
int cpathUpDir(cpath *path);

/*
    Converts all separators to the given separator
    Note: will only convert / and \
*/
_CPATH_FUNC_
void cpathConvertSepCustom(cpath *path, cpath_char_t sep);

/*
    Converts all seperators to the default one.
*/
_CPATH_FUNC_
void cpathConvertSep(cpath *path) { cpathConvertSepCustom(path, CPATH_SEP); }

/* == File System == */

/*
    Get the current working directory allocating the space
*/
_CPATH_FUNC_
cpath_char_t *cpathGetCwdAlloc();

/*
    Get the current working directory passing in a buffer
*/
_CPATH_FUNC_
cpath_char_t *cpathGetCwdBuf(cpath_char_t *buf, size_t size);

/*
    Write the cwd to a path buffer.
*/
_CPATH_FUNC_
void cpathWriteCwd(cpath *path);

/*
    Get the cwd as a path.
*/
_CPATH_FUNC_
cpath cpathGetCwd();

/*
    Opens a directory placing information into the given directory buffer.
*/
_CPATH_FUNC_
int cpathOpenDir(cpath_dir *dir, const cpath *path);

/*
    Restarts the given directory iterator.
*/
_CPATH_FUNC_
int cpathRestartDir(cpath_dir *dir);

/*
    Clear directory data.
    If closing parents then will recurse through all previous emplaces.
*/
_CPATH_FUNC_
void cpathCloseDir(cpath_dir *dir);

/*
    Move to the next file in the iterator.
*/
_CPATH_FUNC_
int cpathMoveNextFile(cpath_dir *dir);

/*
    Load stat.  Sets statLoaded.
*/
_CPATH_FUNC_
int cpathGetFileInfo(cpath_file *file);

/*
    Load file flags such as isDir, isReg, isSym
    Attempts to not use stat since that is slow
*/
_CPATH_FUNC_
int cpathLoadFlags(cpath_dir *dir, cpath_file *file, void *data);

/*
    Peeks the next file but doesn't move the iterator along.
*/
_CPATH_FUNC_
int cpathPeekNextFile(cpath_dir *dir, cpath_file *file);

/*
    Get the next file inside the directory, acts like an iterator.
    i.e. while (cpathGetNextFile(...)) can use dir->hasNext to verify
    that there are indeed a file but it is safe!

    File may be null if you don't wish to actually get a copy
*/
_CPATH_FUNC_
int cpathGetNextFile(cpath_dir *dir, cpath_file *file);

/*
    Is the file . or ..
*/
_CPATH_FUNC_
int cpathFileIsSpecialHardLink(const cpath_file *file);

/*
    Preload all files in a directory.  Will be called automatically in some
    function calls such as cpathGetFile()
*/
_CPATH_FUNC_
int cpathLoadAllFiles(cpath_dir *dir);

/*
    More of a helper function, checks if we have space for n
    and will preload any files if required.
*/
_CPATH_FUNC_
int cpathCheckGetN(cpath_dir *dir, size_t n);

/*
    Get the nth file inside the directory, this is independent of the iterator
    above.

    Note calling this will require the reading of the entire directory.
             if you want to prevent all these allocations then I recommend using
             getNextFile(...)
*/
_CPATH_FUNC_
int cpathGetFile(cpath_dir *dir, cpath_file *file, size_t n);

/*
    Get a const reference to a file, this is more performant than GetFile
    but you can't modify anything about the object.
*/
_CPATH_FUNC_
int cpathGetFileConst(cpath_dir *dir, const cpath_file **file, size_t n);

/*
    Opens the next sub directory into this
    Saves the old directory into the parent if given saveDir
*/
_CPATH_FUNC_
int cpathOpenSubFileEmplace(cpath_dir *dir, const cpath_file *file, int saveDir);

/*
    Opens the n'th sub directory into this directory
    Saves the old directory into the parent if given saveDir
*/
_CPATH_FUNC_
int cpathOpenSubDirEmplace(cpath_dir *dir, size_t n, int saveDir);

/*
    Opens the n'th sub directory into other given directory
*/
_CPATH_FUNC_
int cpathOpenSubDir(cpath_dir *out, cpath_dir *dir, size_t n);

/*
    Opens the next sub directory into other given directory
*/
_CPATH_FUNC_
int cpathOpenNextSubDir(cpath_dir *out, cpath_dir *dir);

/*
    Peeks the sub directory.
*/
_CPATH_FUNC_
int cpathOpenNextSubDir(cpath_dir *out, cpath_dir *dir);

/*
    Opens the next sub directory into this
    Saves the old directory into the parent if given saveDir
*/
_CPATH_FUNC_
int cpathOpenNextSubDirEmplace(cpath_dir *dir, int saveDir);

/*
    Peeks the sub directory
*/
_CPATH_FUNC_
int cpathOpenCurrentSubDirEmplace(cpath_dir *dir, int saveDir);

/*
    Revert an emplace and go back to the parent.
    Note: This can occur multiple times.
    Returns true if it went back to the parent.

    This form is for a pointer variant where the storage
    of the first directory is external.
*/
_CPATH_FUNC_
int cpathRevertEmplace(cpath_dir **dir);

/*
    Revert an emplace and go back to the parent.
    Note: This can occur multiple times.
    Returns true if it went back to the parent.

    This form is for a directory that is stored
    internally to the function.
*/
_CPATH_FUNC_
int cpathRevertEmplaceCopy(cpath_dir *dir);

/*
    Opens the given path as a file.
*/
_CPATH_FUNC_
int cpathOpenFile(cpath_file *file, const cpath *path);

/*
    Converts a given file to a directory.
    Note: Fails to convert if the file isn't a directory.
*/
_CPATH_FUNC_
int cpathFileToDir(cpath_dir *dir, const cpath_file *file);

/*
    Get the extension for a file.
    Note: You can't access a file's extension BEFORE you call this
                It will be null prior.
    Returns NULL if no extension or if it is a directory.
*/
_CPATH_FUNC_
cpath_str cpathGetExtension(cpath_file *file);

/*
    Get the time of last access
*/
_CPATH_FUNC_
cpath_time_t cpathGetLastAccess(cpath_file *file);

/*
    Get the time of last modification
*/
_CPATH_FUNC_
cpath_time_t cpathGetLastModification(cpath_file *file);

/*
    Get the file size in bytes of this file.
*/
_CPATH_FUNC_
cpath_offset_t cpathGetFileSize(cpath_file *file);

/*
    Standard compare function for files
*/
_CPATH_FUNC_
void cpathSort(cpath_dir *dir, cpath_cmp cmp);

/*
    Get the file size in decimal form
*/
_CPATH_FUNC_
double cpathGetFileSizeDec(cpath_file *file, int interval);

/*
    Get the file size prefix
*/
_CPATH_FUNC_
const cpath_char_t *cpathGetFileSizeSuffix(cpath_file *file, CPathByteRep rep);

/*
    Create the given directory
*/
_CPATH_FUNC_
int cpathMkdir(const cpath *path);

/*
    Open a given file
*/
_CPATH_FUNC_
FILE *cpathOpen(const cpath *path, const cpath_char_t *mode);

/* == Definitions == */

/* == Path == */

_CPATH_FUNC_
size_t cpathStrCpyConv(cpath_str dest, size_t len, const cpath_char_t *src) {
    int lastWasSep = 0;
    cpath_str startDest = dest;
    for (size_t i = 0; i < len + 1; i++) {
        int isSep = src[i] == CPATH_OTHER_SEP || src[i] == CPATH_SEP;

        if (isSep && !lastWasSep) {
            *dest++ = CPATH_SEP;
        } else if (!isSep || !lastWasSep) {
            *dest++ = src[i];
        }
        lastWasSep = isSep;
    }
    return dest - startDest - 1;
}

_CPATH_FUNC_
void cpathTrim(cpath *path) {
    /* trim all the terminating / and \ */
    /* We don't want to trim // into empty string
         We will trim it to just /
     */
    while (path->len > 1 && (path->buf[path->len - 1] == CPATH_SEP ||
                 path->buf[path->len - 1] == CPATH_OTHER_SEP)) {
        path->len--;
    }
    path->buf[path->len] = CPATH_STR('\0');
}

_CPATH_FUNC_
cpath cpathFromUtf8(const char *str) {
    cpath path;
    if (str[0] == CPATH_STR('\0')) {
        // empty string which is just '.'
        path.len = 1;
        path.buf[0] = CPATH_STR('.');
        path.buf[1] = CPATH_STR('\0');
        return path;
    }

    path.len = 0;
    path.buf[0] = CPATH_STR('\0');
    size_t len = cpath_str_length(str);
    // NOTE: max path len includes the \0 where as str length doesn't!
    if (len >= CPATH_MAX_PATH_LEN) {
        errno = ENAMETOOLONG;
        return path;
    }
#if defined CPATH_UNICODE && defined _MSC_VER
    mbstowcs_s(&path.len, path.buf, len + 1, str, CPATH_MAX_PATH_LEN);
    cpathConvertSep(&path);
#else
    path.len = cpathStrCpyConv(path.buf, len, str);
#endif
    cpathTrim(&path);
    return path;
}

_CPATH_FUNC_
void cpathCopy(cpath *out, const cpath *in) {
    cpath_str_copy(out->buf, in->buf);
    out->len = in->len;
}

_CPATH_FUNC_
int cpathFromStr(cpath *out, const cpath_char_t *str) {
    size_t len = cpath_str_length(str);
    if (len >= CPATH_MAX_PATH_LEN) return 0;
    if (len == 0) {
        out->len = 1;
        out->buf[0] = CPATH_STR('.');
        out->buf[1] = CPATH_STR('\0');
        return 1;
    }

    out->len = cpathStrCpyConv(out->buf, len, str);
    cpathTrim(out);
    return 1;
}

_CPATH_FUNC_
void cpathAppendSprintf(cpath *out, const cpath_char_t *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    out->len += cpath_vsnprintf(out->buf + out->len,
                                CPATH_MAX_PATH_LEN - out->len, fmt, list);
    va_end(list);
}

_CPATH_FUNC_
int cpathAppendStrn(cpath *out, const cpath_char_t *other, size_t len) {
    if (len + out->len >= CPATH_MAX_PATH_LEN) {
        // path too long, >= cause max path includes CPATH_STR('\0')
        errno = ENAMETOOLONG;
        return 0;
    }

    // This is more efficient than a strcat since it doesn't have to get
    // the length again it also handles all the conversions well and doesn't
    // have to retraverse the out->buf which we know is fine.
    out->len += cpathStrCpyConv(out->buf + out->len, len, other);
    cpathTrim(out);
    return 1;
}

_CPATH_FUNC_
int cpathAppendStr(cpath *out, const cpath_char_t *other) {
    return cpathAppendStrn(out, other, cpath_str_length(other));
}

_CPATH_FUNC_
int cpathAppend(cpath *out, const cpath *other) {
    return cpathAppendStrn(out, other->buf, other->len);
}

_CPATH_FUNC_
int cpathConcatStrn(cpath *out, const cpath_char_t *str, size_t len) {
    if (len + out->len >= CPATH_MAX_PATH_LEN) {
        // path too long, >= cause max path includes CPATH_STR('\0')
        errno = ENAMETOOLONG;
        return 0;
    }

    if (str[0] != CPATH_SEP && out->buf[out->len - 1] != CPATH_SEP &&
            str[0] != CPATH_OTHER_SEP && out->buf[out->len - 1] != CPATH_OTHER_SEP) {
        out->buf[out->len++] = CPATH_SEP;
        out->buf[out->len] = CPATH_STR('\0');
    }

    // This is more efficient than a strcat since it doesn't have to get
    // the length again it also handles all the conversions well and doesn't
    // have to retraverse the out->buf which we know is fine.
    out->len += cpathStrCpyConv(out->buf + out->len, len, str);
    cpathTrim(out);
    return 1;
}

_CPATH_FUNC_
int cpathConcatStr(cpath *out, const cpath_char_t *other) {
    return cpathConcatStrn(out, other, cpath_str_length(other));
}

_CPATH_FUNC_
int cpathConcat(cpath *out, const cpath *other) {
    return cpathConcatStrn(out, other->buf, other->len);
}

_CPATH_FUNC_
int cpathExists(const cpath *path) {
#if defined _MSC_VER || defined __MINGW32__
    DWORD res = GetFileAttributes(path->buf);
    return res != INVALID_FILE_ATTRIBUTES;
#else
    /*
        We can either try to use stat or just access, access is more efficient
        when not checking permissions.

        struct stat tmp;
        return stat(path->buf, &tmp) == 0;
    */
    return access(path->buf, F_OK) != -1;
#endif
}

_CPATH_FUNC_
int cpathCanonicaliseNoSysCall(cpath *out, cpath *path) {
    /*
        NOTE: This should work even if out == path
                    It just has been written that way.
    */
    if (path == NULL) {
        errno = EINVAL;
        return 0;
    }

    out->len = 0;
    cpath_char_t *chr = &path->buf[0];

    while (*chr != CPATH_STR('\0')) {
        if (*chr == CPATH_STR('.')) {
            if (chr[1] == CPATH_STR('.')) {
                if (out->len == 0 || (out->len == 1 && 
                        (out->buf[0] == CPATH_SEP || out->buf[0] == CPATH_OTHER_SEP))) {
                    // no directory to go back based on string alone
                    errno = ENOENT;
                    return 0;
                }
                // remove last directory ignoring the last / since that is part of ..
                out->len--;
                while (out->len > 0 && out->buf[out->len - 1] != CPATH_SEP &&
                             out->buf[out->len - 1] != CPATH_OTHER_SEP) {
                    out->len--;
                }
                // skip twice
                chr++;
                chr++;
            } else if (chr[1] == CPATH_SEP || chr[1] == CPATH_STR('\0') ||
                                 chr[1] == CPATH_OTHER_SEP) {
                // skip
                chr++;
            } else {
                out->buf[out->len++] = *chr;
            }
        } else {
            out->buf[out->len++] = *chr;
        }
        chr++;
    }

    out->buf[out->len] = CPATH_STR('\0');
    cpathTrim(out);
    if (out->len == 0) {
        out->buf[0] = CPATH_STR('.');
        out->buf[1] = CPATH_STR('\0');
        out->len = 1;
    }

    return 1;
}

_CPATH_FUNC_
int cpathCanonicalise(cpath *out, cpath *path) {
    cpath tmp;
    if (out == path) {
        // support non-restrict method
        cpathCopy(&tmp, path);
        path = &tmp;
    }

#if defined _MSC_VER
    DWORD size = GetFullPathName(path->buf, CPATH_MAX_PATH_LEN, out->buf, NULL);
    if (size == 0) {
        // figure out errno
        return 0;
    } else {
        path->len = size;
        return 1;
    }
#else
    char *res = realpath(path->buf, out->buf);
    if (res != NULL) {
        out->len = cpath_str_length(out->buf);
    }
    return res != NULL;
#endif
}

_CPATH_FUNC_
int cpathCanonicaliseAvoidSysCall(cpath *out, cpath *path) {
    if (!cpathCanonicaliseNoSysCall(out, path)) {
        return cpathCanonicalise(out, path);
    }
    return 1;
}

_CPATH_FUNC_
void cpathClear(cpath *path) {
    path->buf[0] = CPATH_STR('.');
    path->buf[1] = CPATH_STR('\0');
    path->len = 1;
}

_CPATH_FUNC_
const cpath_char_t *cpathItRef(cpath *path, int *index) {
    if (path == NULL || index == NULL || *index >= path->len || *index < 0) {
        return NULL;
    }

    if (path->buf[*index] == CPATH_STR('\0')) {
        path->buf[*index] = CPATH_SEP;
        (*index)++;
    }
    int old_pos = *index;

    // find next '/'
    while (*index < path->len && path->buf[*index] != CPATH_SEP &&
                 path->buf[*index] != CPATH_OTHER_SEP) {
        (*index)++;
    }

    path->buf[*index] = '\0';

    return &path->buf[old_pos];
}

_CPATH_FUNC_
void cpathItRefRestore(cpath *path, int *index) {
    if (path == NULL) return;

    if (index == NULL || *index < 0 || *index >= path->len ||
            path->buf[*index] != CPATH_STR('\0')) {
        // we have to loop
        for (int i = 0; i < path->len; i++) {
            if (path->buf[i] == CPATH_STR('\0')) path->buf[i] = CPATH_SEP;
        }
        // set index to the length since we have no real way to determine
        // where abouts it its (that is if it isn't null)
        if (index != NULL) *index = path->len;
    } else {
        path->buf[*index] = CPATH_SEP;
        (*index)++;
    }
}

_CPATH_FUNC_
int cpathUpDir(cpath *path) {
    if (path->len == 0) {
        // shouldn't have a path with this length
        errno = EINVAL;
        return 0;
    }

    // We have to find the last component and strip it
    int oldLen = path->len;
    while (path->len > 1 && path->buf[path->len - 1] != CPATH_SEP &&
                 path->buf[path->len - 1] != CPATH_SEP) {
        path->len--;
    }
    if (path->len == 1) {
        // we failed to quickly go up a dir
        // so we will have to restore len and try another method
        // this is better than the avoid method because
        // this will allow you to go up a directory if the path == PATH_MAX
        path->len = oldLen;
        if (!CPATH_CONCAT_LIT(path, "..")) {
            return 0;
        }
        return cpathCanonicalise(path, path);
    }

    // we have found a '/' so we just set it to '\0'
    path->buf[path->len - 1] = '\0';
    path->len--;
    return 1;
}

_CPATH_FUNC_
void cpathConvertSepCustom(cpath *path, cpath_char_t sep) {
    int lastSep = 0;
    cpath_str cur = path->buf;
    for (int i = 0; i < path->len + 1; i++) {
        int isSep = path->buf[i] == CPATH_SEP || path->buf[i] == CPATH_OTHER_SEP;
        if (isSep && !lastSep) {
            *cur++ = CPATH_SEP;
        } else if (!isSep || !lastSep) {
            *cur++ = path->buf[i];
        }
        lastSep = isSep;
    }
}

/* == File System == */

/*
    Get the current working directory allocating the space
*/
_CPATH_FUNC_
cpath_char_t *cpathGetCwdAlloc() {
    cpath_char_t *buf;
    buf = (cpath_char_t*)CPATH_MALLOC(sizeof(cpath_char_t) * CPATH_MAX_PATH_LEN);
    return cpathGetCwdBuf(buf, CPATH_MAX_PATH_LEN);
}

_CPATH_FUNC_
cpath_char_t *cpathGetCwdBuf(cpath_char_t *buf, size_t size) {
    return _cpath_getcwd(buf, size);
}

_CPATH_FUNC_
void cpathWriteCwd(cpath *path) {
    cpathGetCwdBuf(path->buf, CPATH_MAX_PATH_LEN);
    path->len = cpath_str_length(path->buf);
    cpathTrim(path);
}

_CPATH_FUNC_
cpath cpathGetCwd() {
    cpath path;
    cpathWriteCwd(&path);
    cpathTrim(&path);
    return path;
}

_CPATH_FUNC_
int cpathOpenDir(cpath_dir *dir, const cpath *path) {
    if (dir == NULL || path == NULL || path->len == 0) {
        // empty strings are invalid arguments
        errno = EINVAL;
        return 0;
    }

    if (path->len + CPATH_PATH_EXTRA_CHARS >= CPATH_MAX_PATH_LEN) {
        errno = ENAMETOOLONG;
        return 0;
    }

    dir->files = NULL;

#if defined _MSC_VER
    dir->handle = INVALID_HANDLE_VALUE;
#else
    dir->dir = NULL;
#endif
    dir->path.buf[path->len] = CPATH_STR('\0');
    dir->parent = NULL;
    dir->files = NULL;
#if defined _MSC_VER
    dir->handle = INVALID_HANDLE_VALUE;
#else
    dir->dir = NULL;
    dir->dirent = NULL;
#endif

    cpathCopy(&dir->path, path);
    return cpathRestartDir(dir);
}

_CPATH_FUNC_
int cpathRestartDir(cpath_dir *dir) {
    // @TODO: I think there is a faster way if the handles exist
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    dir->hasNext = 1;
    dir->size = -1;
    if (dir->files != NULL) CPATH_FREE(dir->files);
    dir->files = NULL;

#if defined _MSC_VER
    if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
    dir->handle = INVALID_HANDLE_VALUE;
#else
    if (dir->dir != NULL) _cpath_closedir(dir->dir);
    dir->dir = NULL;
    dir->dirent = NULL;
#endif

    // Ignore parent, just restart this dir

#if defined _MSC_VER
    cpath_char_t pathBuf[CPATH_MAX_PATH_LEN];
    cpath_str_copy(pathBuf, dir->path.buf);
    strcat(pathBuf, CPATH_STR("\\*"));

#if (defined WINAPI_FAMILY) && (WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP)
    dir->handle = FindFirstFileEx(pathBuf, FindExInfoStandard, &dir->findData,
                                                                FindExSearchNameMatch, NULL, 0);
#else
    dir->handle = FindFirstFile(pathBuf, &dir->findData);
#endif

    if (dir->handle == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        // free associate memory and exit
        cpathCloseDir(dir);
        return 0;
    }

#else

    dir->dir = _cpath_opendir(dir->path.buf);
    if (dir->dir == NULL) {
        cpathCloseDir(dir);
        return 0;
    }
    dir->dirent = _cpath_readdir(dir->dir);
    // empty directory
    if (dir->dirent == NULL) dir->hasNext = 0;

#endif

    return 1;
}

_CPATH_FUNC_
void cpathCloseDir(cpath_dir *dir) {
    if (dir == NULL) return;

    dir->hasNext = 1;
    dir->size = -1;
    if (dir->files != NULL) CPATH_FREE(dir->files);
    dir->files = NULL;

#if defined _MSC_VER
    if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
    dir->handle = INVALID_HANDLE_VALUE;
#else
    if (dir->dir != NULL) _cpath_closedir(dir->dir);
    dir->dir = NULL;
    dir->dirent = NULL;
#endif

    cpathClear(&dir->path);
    if (dir->parent != NULL) {
        cpathCloseDir(dir->parent);
        CPATH_FREE(dir->parent);
        dir->parent = NULL;
    }
}

_CPATH_FUNC_
int cpathMoveNextFile(cpath_dir *dir) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }
    if (!dir->hasNext) {
        return 0;
    }

#if defined _MSC_VER
    if (FindNextFile(dir->handle, &dir->findData) == 0) {
        dir->hasNext = 0;
        if (GetLastError() != ERROR_SUCCESS &&
                GetLastError() != ERROR_NO_MORE_FILES) {
            cpathCloseDir(dir);
            errno = EIO;
            return 0;
        }
    }
#else
    dir->dirent = _cpath_readdir(dir->dir);
    if (dir->dirent == NULL) {
        dir->hasNext = 0;
    }
#endif

    return 1;
}

_CPATH_FUNC_
int cpathGetFileInfo(cpath_file *file) {
    if (file->statLoaded) {
        return 1;
    }
#if !defined _MSC_VER
#if defined __MINGW32__
    if (_tstat(file->path.buf, &file->stat) == -1) {
        return 0;
    }
#elif defined _BSD_SOURCE || defined _DEFAULT_SOURCE	\
            || (defined _XOPEN_SOURCE && _XOPEN_SOURCE >= 500)	\
            || (defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 200112L)
    if (lstat(file->path.buf, &file->stat) == -1) {
        return 0;
    }
#else
    if (stat(file->path.buf, &file->stat) == -1) {
        return 0;
    }
#endif
#endif
    file->statLoaded = 1;
    return 1;
}

_CPATH_FUNC_
int cpathLoadFlags(cpath_dir *dir, cpath_file *file, void *data) {
#if defined _MSC_VER
    WIN32_FIND_DATA find = *((WIN32_FIND_DATA*) data);
    file->isDir = FILE_IS(find, DIRECTORY);
    if (FILE_IS(find, NORMAL)) {
        file->isReg = 1;
    } else if (FILE_IS_NOT(find, DEVICE) && FILE_IS_NOT(find, DIRECTORY) &&
                         FILE_IS_NOT(find, ENCRYPTED) && FILE_IS_NOT(find, OFFLINE) &&
#ifdef FILE_ATTRIBUTE_INTEGRITY_STREAM
                         FILE_IS_NOT(find, INTEGRITY_STREAM) &&
#endif
#ifdef FILE_ATTRIBUTE_NO_SCRUB_DATA
                         FILE_IS_NOT(find, NO_SCRUB_DATA) &&
#endif
                         FILE_IS_NOT(find, TEMPORARY)) {
        file->isReg = 1;
    } else {
        file->isReg = 0;
    }
    file->isSym = FILE_IS(find, REPARSE_POINT);
    file->extension = NULL;
#else
    if (dir->dirent == NULL || dir->dirent->d_type == DT_UNKNOWN) {
        if (!cpathGetFileInfo(file)) {
            return 0;
        }

        file->isDir = S_ISDIR(file->stat.st_mode);
        file->isReg = S_ISREG(file->stat.st_mode);
        file->isSym = S_ISLNK(file->stat.st_mode);
    } else {
        file->isDir = dir->dirent->d_type == DT_DIR;
        file->isReg = dir->dirent->d_type == DT_REG;
        file->isSym = dir->dirent->d_type == DT_LNK;
        file->statLoaded = 0;
    }
#endif
    return 1;
}

_CPATH_FUNC_
int cpathPeekNextFile(cpath_dir *dir, cpath_file *file) {
    if (file == NULL || dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    file->statLoaded = 0;
    // load current file into file
    const cpath_char_t *filename;
    size_t filenameLen;
#if defined _MSC_VER
    if (dir->handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    filename = dir->findData.cFileName;
    filenameLen = cpath_str_length(filename);
#else
    if (dir->dirent == NULL) {
        return 0;
    }
    filename = dir->dirent->d_name;
    // TODO: On MACOS there is a d_namlen but not on linux
    filenameLen = strlen(dir->dirent->d_name);
#endif
    size_t totalLen = dir->path.len + filenameLen;
    if (totalLen + 1 + CPATH_PATH_EXTRA_CHARS >= CPATH_MAX_PATH_LEN ||
            filenameLen >= CPATH_MAX_FILENAME_LEN) {
        errno = ENAMETOOLONG;
        return 0;
    }

    cpath_str_copy(file->name, filename);
    cpathCopy(&file->path, &dir->path);
    if (!CPATH_CONCAT_LIT(&file->path, "/") ||
            !cpathConcatStr(&file->path, filename)) {
        return 0;
    }
#ifndef CPATH_NO_AUTOLOAD_EXT
    cpathGetExtension(file);
#endif
#if defined _MSC_VER
    if (!cpathLoadFlags(dir, file, (WIN32_FIND_DATA*)(&dir->findData))) return 0;
#else
    if (!cpathLoadFlags(dir, file, NULL)) return 0;
#endif
#ifdef CPATH_AUTOLOAD_STAT
    if (!cpathGetFileInfo(file)) return 0;
#endif
    return 1;
}

_CPATH_FUNC_
int cpathGetNextFile(cpath_dir *dir, cpath_file *file) {
    if (file != NULL) {
        if (!cpathPeekNextFile(dir, file)) {
            return 0;
        }
    }

    errno = 0;
    // @TODO: Bug I think
    //        I think we want to remove the condition that dir->hasNext
    //        Sicne cpath checks that.
    if (dir->hasNext && !cpathMoveNextFile(dir) && errno != 0) {
        return 0;
    }

    return 1;
}

_CPATH_FUNC_
int cpathFileIsSpecialHardLink(const cpath_file *file) {
    return file->name[0] == CPATH_STR('.') && (file->name[1] == CPATH_STR('\0') ||
                 (file->name[1] == CPATH_STR('.') && file->name[2] == CPATH_STR('\0')));
}

_CPATH_FUNC_
int cpathLoadAllFiles(cpath_dir *dir) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (dir->files != NULL) CPATH_FREE(dir->files);

    // @Ugly:
    /*
        The problem is that we could go through all files to get a count
        This is kinda ugly but is linear and ensures we don't make allocations
        The other method is to use a 'vector' however the copies are really
        expensive since we have huge objects which is why I chose the count method
    */
    size_t count = 0;
    errno = 0;
    while (cpathMoveNextFile(dir)) {
        count++;
    }

    // we can have 0 files
    if (count == 0) {
        dir->size = 0;
        return 1;
    }

    // loading failed for some reason or we failed to restart the dir iterator
    if (errno != 0 || !cpathRestartDir(dir)) {
        // we won't close the directory though!
        // we'll just pretend we have no files!
        dir->size = 0;
        return 0;
    }

    dir->size = count;
    dir->files = (cpath_file*) CPATH_MALLOC(sizeof(cpath_file) * count);
    if (dir->files == NULL) {
        // we won't close the directory just error out
        dir->size = 0;
        return 0;
    }

    errno = 0;
    int i = 0;
    // also make sure that we don't overflow the array
    // because size requirements changed i.e. race condition
    // we could fix this by resizing but that's super expensive
    // maybe this is time for a linked list or just to dynamically allocate
    // all file nodes and waste the extra 8 bytes per (and cache locality)
    while (i < dir->size && cpathGetNextFile(dir, &dir->files[i])) {
        i++;
    }

    if (i < dir->size) {
        // we stopped early due to error I'm still not going to crazily error out
        // because it may just be race condition and I'll be a bit lazy
        // @TODO: check if it was just race or if a read failed
        dir->size = i;
    }

    return 1;
}

_CPATH_FUNC_
int cpathCheckGetN(cpath_dir *dir, size_t n) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (dir->size == -1) {
        // we haven't loaded
        cpathLoadAllFiles(dir);
    }

    if (n >= dir->size) {
        errno = ENOENT;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathGetFile(cpath_dir *dir, cpath_file *file, size_t n) {
    if (file == NULL) {
        errno = EINVAL;
        return 0;
    }
    if (!cpathCheckGetN(dir, n)) return 0;

    memcpy(file, &dir->files[n], sizeof(cpath_file));
    return 1;
}

_CPATH_FUNC_
int cpathGetFileConst(cpath_dir *dir, const cpath_file **file, size_t n) {
    if (dir == NULL || file == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (dir->size == -1) {
        // we haven't loaded
        cpathLoadAllFiles(dir);
    }

    if (n >= dir->size) {
        errno = ENOENT;
        return 0;
    }

    *file = &dir->files[n];
    return 1;
}

_CPATH_FUNC_
int cpathOpenSubFileEmplace(cpath_dir *dir, const cpath_file *file,
                                                        int saveDir) {
    cpath_dir *saved = NULL;
    if (saveDir) {
        // save the old one
        saved = (cpath_dir*)CPATH_MALLOC(sizeof(cpath_dir));
        if (saved != NULL) memcpy(saved, dir, sizeof(cpath_dir));
    }

    if (!cpathFileToDir(dir, file)) {
        if (saved != NULL) CPATH_FREE(saved);
        return 0;
    }

    dir->parent = saved;

    return 1;
}

_CPATH_FUNC_
int cpathOpenSubDirEmplace(cpath_dir *dir, size_t n, int saveDir) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }
    if (!cpathCheckGetN(dir, n)) return 0;

    const cpath_file *file;
    if (!cpathGetFileConst(dir, &file, n) ||
            !file->isDir || !cpathOpenSubFileEmplace(dir, file, saveDir)) {
        if (!file->isDir) errno = EINVAL;
        return 0;
    }

    return 1;
}

_CPATH_FUNC_
int cpathOpenSubDir(cpath_dir *out, cpath_dir *dir, size_t n) {
    if (dir == NULL || out == NULL) {
        errno = EINVAL;
        return 0;
    }
    if (!cpathCheckGetN(dir, n)) return 0;

    const cpath_file *file;
    if (!cpathGetFileConst(dir, &file, n) ||
            !file->isDir || !cpathFileToDir(out, file)) {
        if (!file->isDir) errno = EINVAL;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathOpenNextSubDir(cpath_dir *out, cpath_dir *dir) {
    if (dir == NULL || out == NULL) {
        errno = EINVAL;
        return 0;
    }

    cpath_file file;
    if (!cpathGetNextFile(dir, &file) ||
            !file.isDir || !cpathFileToDir(out, &file)) {
        if (!file.isDir) errno = EINVAL;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathOpenCurrentSubDir(cpath_dir *out, cpath_dir *dir) {
    if (dir == NULL || out == NULL) {
        errno = EINVAL;
        return 0;
    }

    cpath_file file;
    if (!cpathPeekNextFile(dir, &file) ||
            !file.isDir || !cpathFileToDir(out, &file)) {
        if (!file.isDir) errno = EINVAL;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathOpenNextSubDirEmplace(cpath_dir *dir, int saveDir) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    cpath_file file;
    if (!cpathGetNextFile(dir, &file) ||
            !file.isDir || !cpathOpenSubFileEmplace(dir, &file, saveDir)) {
        if (!file.isDir) errno = EINVAL;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathOpenCurrentSubDirEmplace(cpath_dir *dir, int saveDir) {
    if (dir == NULL) {
        errno = EINVAL;
        return 0;
    }

    cpath_file file;
    if (!cpathPeekNextFile(dir, &file) ||
            !file.isDir || !cpathOpenSubFileEmplace(dir, &file, saveDir)) {
        if (!file.isDir) errno = EINVAL;
        return 0;
    }
    return 1;
}

_CPATH_FUNC_
int cpathRevertEmplace(cpath_dir **dir) {
    if (dir == NULL || *dir == NULL) return 0;
    cpath_dir *tmp = (*dir)->parent;
    (*dir)->parent = NULL;
    cpathCloseDir(*dir);
    *dir = tmp;
    return *dir != NULL;
}

_CPATH_FUNC_
int cpathRevertEmplaceCopy(cpath_dir *dir) {
    if (dir == NULL) return 0;
    cpath_dir *tmp = dir->parent;
    dir->parent = NULL;
    cpathCloseDir(dir);
    if (tmp != NULL) {
        memcpy(dir, tmp, sizeof(cpath_dir));
    }
    return tmp != NULL;
}

_CPATH_FUNC_
int cpathOpenFile(cpath_file *file, const cpath *path) {
    // We want to efficiently open this file so unlike most libraries
    // we won't use a directory search we will just find the given file
    // or directly use stuff like dirname/basename!
    cpath_dir dir;
    int found = 0;

    if (file == NULL || path == NULL || path->len == 0) {
        errno = EINVAL;
        return 0;
    }
    if (path->len >= CPATH_MAX_PATH_LEN) {
        errno = ENAMETOOLONG;
        return 0;
    }

    void *data;
    void *handle = NULL;

    cpathCopy(&file->path, path);

#if defined _MSC_VER
    WIN32_FIND_DATA findData;

#if (defined WINAPI_FAMILY) && (WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP)
    handle = FindFirstFileEx(path->buf, FindExInfoStandard, &findData,
                                                                FindExSearchNameMatch, NULL, 0);
#else
    handle = FindFirstFile(path->buf, &findData);
#endif

    if (handle == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return 0;
    }

    data = &findData;
    cpath_str_copy(file->name, findData.cFileName);
#else
    // copy the name then strip to just basename
    // this is very ewwwww, honestly we probably would be better
    // to just do this ourselves since ugh
    cpath_strn_copy(file->name, path->buf, path->len);
    char *tmp = basename(file->name);
    // some systems allocate it seems.. ugh
    if (tmp != file->name) {
        cpath_str_copy(file->name, tmp);
        CPATH_FREE(tmp);
    }
    // @TODO: make sure tmp isn't too long
#endif

    file->statLoaded = 0;
    int res = cpathLoadFlags(&dir, file, data);
#if defined _MSC_VER
    FindClose((HANDLE)handle);
#else
    dir.dirent = NULL;
#endif

    return res;
}

_CPATH_FUNC_
int cpathFileToDir(cpath_dir *dir, const cpath_file *file) {
    if (dir == NULL || file == NULL || !file->isDir) {
        errno = EINVAL;
        return 0;
    }
    return cpathOpenDir(dir, &file->path);
}

_CPATH_FUNC_
cpath_str cpathGetExtension(cpath_file *file) {
    if (file->extension != NULL) return file->extension;

    cpath_char_t *dot = cpath_str_find_last_char(file->name, CPATH_STR('.'));
    if (dot != NULL) {
        // extension
        file->extension = dot + 1;
    } else {
        // no extension so set to CPATH_STR('\0')
        file->extension = &file->name[cpath_str_length(file->name)];
    }
    return file->extension;
}

_CPATH_FUNC_
cpath_time_t cpathGetLastAccess(cpath_file *file) {
    if (!file->statLoaded) cpathGetFileInfo(file);

#if defined _MSC_VER || defined __MINGW32__
    // Idk todo
#else
    return file->stat.st_atime;
#endif
}

_CPATH_FUNC_
cpath_time_t cpathGetLastModification(cpath_file *file) {
    if (!file->statLoaded) cpathGetFileInfo(file);

#if defined _MSC_VER || defined __MINGW32__
    // Idk todo
#else
    return file->stat.st_mtime;
#endif
}

_CPATH_FUNC_
cpath_offset_t cpathGetFileSize(cpath_file *file) {
    if (!file->statLoaded) cpathGetFileInfo(file);

#if defined _MSC_VER || defined __MINGW32__
    // Idk todo
#else
    return file->stat.st_size;
#endif
}

_CPATH_FUNC_
void cpathSort(cpath_dir *dir, cpath_cmp cmp) {
    if (dir->files == NULL) {
        if (dir->size == -1) {
            // bad, this means you try to sorted a directory before you preloaded
            // in this case we'll just preload the files.  This is just to be nice
            cpathLoadAllFiles(dir);
        }

        // this is fine just means no files in directory
        if (dir->size == 0) {
            return;
        }
    }
    qsort(dir->files, dir->size, sizeof(struct cpath_dir_t), cmp);
}

static const cpath_char_t *prefixTableDecimal[] = {
    CPATH_STR("kB"), CPATH_STR("MB"), CPATH_STR("GB"), CPATH_STR("TB"),
    CPATH_STR("PB"), CPATH_STR("EB"), CPATH_STR("ZB"), CPATH_STR("YB"),
};
static const cpath_char_t *prefixTableDecimalUpper[] = {
    CPATH_STR("KB"), CPATH_STR("MB"), CPATH_STR("GB"), CPATH_STR("TB"),
    CPATH_STR("PB"), CPATH_STR("EB"), CPATH_STR("ZB"), CPATH_STR("YB"),
};
static const cpath_char_t *prefixTableDecimalLower[] = {
    CPATH_STR("kb"), CPATH_STR("mb"), CPATH_STR("gb"), CPATH_STR("tb"),
    CPATH_STR("pb"), CPATH_STR("eb"), CPATH_STR("zb"), CPATH_STR("yb"),
};
static const cpath_char_t *prefixTableIEC[] = {
    CPATH_STR("KiB"), CPATH_STR("MiB"), CPATH_STR("GiB"), CPATH_STR("TiB"),
    CPATH_STR("PiB"), CPATH_STR("EiB"), CPATH_STR("ZiB"), CPATH_STR("YiB"),
};
// identical to decimal upper but kept separate for the sake of readability
static const cpath_char_t *prefixTableJEDEC[] = {
    CPATH_STR("KB"), CPATH_STR("MB"), CPATH_STR("GB"), CPATH_STR("TB"),
    CPATH_STR("PB"), CPATH_STR("EB"), CPATH_STR("ZB"), CPATH_STR("YB"),
};

_CPATH_FUNC_
double cpathGetFileSizeDec(cpath_file *file, int intervalSize) {
    double size = cpathGetFileSize(file);
    int steps = 0;
    while (size >= intervalSize / 2 && steps < 8) {
        size /= intervalSize;
        steps++;
    }
    return size;
}

_CPATH_FUNC_
const cpath_char_t *cpathGetFileSizeSuffix(cpath_file *file, CPathByteRep rep) {
    cpath_offset_t size = cpathGetFileSize(file);
    int word = (rep & BYTE_REP_LONG) == BYTE_REP_LONG;
    int byte_word = (rep & BYTE_REP_BYTE_WORD) == BYTE_REP_BYTE_WORD;
    // disable both them to make comparing easier
    rep &= ~BYTE_REP_LONG;
    rep &= ~BYTE_REP_BYTE_WORD;
    int interval = rep == BYTE_REP_IEC || rep == BYTE_REP_JEDEC ? 1024 : 1000;

    if (size < interval / 2) {
        // then we just have a byte case
        if (word || byte_word) {
            return rep != BYTE_REP_DECIMAL_LOWER ? CPATH_STR("Bytes")
                                                                                     : CPATH_STR("bytes");
        } else {
            return rep != BYTE_REP_DECIMAL_LOWER ? CPATH_STR("B")
                                                                                     : CPATH_STR("b");
        }
    }

    int steps = 0;
    double size_flt = size;
    while (size_flt >= interval / 2 && steps < 8) {
        size_flt /= (double)interval;
        steps++;
    }

    switch (rep) {
        case BYTE_REP_IEC:            return prefixTableIEC[steps - 1];
        case BYTE_REP_JEDEC:          return prefixTableJEDEC[steps - 1];
        case BYTE_REP_DECIMAL:        return prefixTableDecimal[steps - 1];
        case BYTE_REP_DECIMAL_LOWER:  return prefixTableDecimalLower[steps - 1];
        case BYTE_REP_DECIMAL_UPPER:  return prefixTableDecimalUpper[steps - 1];
        default: return NULL;
    }
}

_CPATH_FUNC_
void cpath_traverse(
    cpath_dir *dir, int depth, int visit_subdirs, cpath_err_handler err,
    cpath_traverse_it it, void *data
) {
    // currently implemented recursively
    if (dir == NULL) {
        errno = EINVAL;
        if (err != NULL) err();
        return;
    }
    cpath_file file;
    while (cpathGetNextFile(dir, &file)) {
        if (it != NULL) {
            it(&file, dir, depth, data);
        }
        if (file.isDir && visit_subdirs && !cpathFileIsSpecialHardLink(&file)) {
            cpath_dir tmp;
            if (!cpathFileToDir(&tmp, &file)) {
                if (err) err();
                continue;
            }
            cpath_traverse(&tmp, depth + 1, visit_subdirs, err, it, data);
            cpathCloseDir(&tmp);
        }
    }
}

_CPATH_FUNC_
int cpathMkdir(const cpath *path) {
#if defined _MSC_VER || __MINGW32__
    return CreateDirectory(path->buf, NULL);
#else
    return mkdir(path->buf, 0700) == 0;
#endif
}

_CPATH_FUNC_
FILE *cpathOpen(const cpath *path, const cpath_char_t *mode) {
    return cpath_fopen(path->buf, mode);
}

#endif
#ifdef __cplusplus
}
#ifndef CPATH_NO_CPP_BINDINGS
    } }

// cpp bindings
namespace cpath {
// using is a C++11 extension, we want to remain pretty
// compatible to all versions
typedef internals::cpath_time_t     Time;
typedef internals::cpath_offset_t   Offset;
typedef internals::cpath_char_t     RawChar;
typedef internals::cpath            RawPath;
typedef internals::cpath_dir        RawDir;
typedef internals::cpath_file       RawFile;
typedef internals::CPathByteRep     ByteRep;

struct File;
struct Path;

typedef void(*TraversalIt)(
    struct File &file, struct Dir &parent, int depth, void *data
);

typedef void(*ErrorHandler)();

/*
    This has to be kept to date with the other representation
    Kinda disgusting but seems to be the best way to ensure encapsulation
*/
const ByteRep BYTE_REP_JEDEC          = internals::BYTE_REP_JEDEC;
const ByteRep BYTE_REP_DECIMAL        = internals::BYTE_REP_DECIMAL;
const ByteRep BYTE_REP_IEC            = internals::BYTE_REP_IEC;
const ByteRep BYTE_REP_DECIMAL_UPPER  = internals::BYTE_REP_DECIMAL_UPPER;
const ByteRep BYTE_REP_DECIMAL_LOWER  = internals::BYTE_REP_DECIMAL_LOWER;
const ByteRep BYTE_REP_LONG           = internals::BYTE_REP_LONG;
const ByteRep BYTE_REP_BYTE_WORD      = internals::BYTE_REP_BYTE_WORD;

#if !defined _MSC_VER
#if defined __MINGW32__
typedef struct _stat RawStat;
#else
typedef struct stat RawStat;
#endif
#endif

template<typename T, typename Err>
struct Opt {
private:
#if __cplusplus <= 199711L
    struct Data {
#else
    union Data {
#endif
    T raw;
    Err err;

    Data(T raw) : raw(raw) {}
    Data(Err err) : err(err) {}
} data;
    bool ok;

public:
    Opt(T raw) : data(raw), ok(true) {}
    Opt(Err err) : data(err), ok(false) {}
    Opt() : data(Err()), ok(false) {}

    bool IsOk() const {
        return ok;
    }

    operator bool() const {
        return ok;
    }

    Err GetErr() const {
        return data.err;
    }

    T GetRaw() const {
        return data.raw;
    }

    T *operator*() {
        if (!ok) return NULL;
        return &data.raw;
    }

    T *operator->() {
        if (!ok) return NULL;
        return &data.raw;
    }
};

struct Error {
    enum Type {
        UNKNOWN,
        INVALID_ARGUMENTS,
        NAME_TOO_LONG,
        NO_SUCH_FILE,
        IO_ERROR,
    };

    static Type FromErrno() {
        switch (errno) {
            case EINVAL: return INVALID_ARGUMENTS;
            case ENAMETOOLONG: return NAME_TOO_LONG;
            case ENOENT: return NO_SUCH_FILE;
            case EIO: return IO_ERROR;
            default: return UNKNOWN;
        }
    }

    static void WriteToErrno(Type type) {
        switch (type) {
            case INVALID_ARGUMENTS: errno = EINVAL;
            case NAME_TOO_LONG: errno = ENAMETOOLONG;
            case NO_SUCH_FILE: errno = ENOENT;
            case IO_ERROR: errno = IO_ERROR;
            default: errno = 0;
        }
    }
};

struct Path {
private:
    RawPath path;

public:
    inline Path(const char *str) : path(internals::cpathFromUtf8(str)) { }

#if CPATH_UNICODE
    inline Path(RawChar *str) {
        internals::cpathFromStr(&path, str);
    }
#endif

    inline Path() {
        path.len = 1;
        path.buf[0] = CPATH_STR('.');
        path.buf[1] = CPATH_STR('\0');
    }

    inline Path(RawPath path) : path(path) {}

    static inline Path GetCwd() {
        return Path(internals::cpathGetCwd());
    }

    inline bool Exists() const {
        return internals::cpathExists(&path);
    }

    inline const RawChar *GetBuffer() const {
        return path.buf;
    }

    inline const RawPath *GetRawPath() const {
        return &path;
    }

    inline unsigned long Size() const {
        return path.len;
    }

    inline friend bool operator==(const Path &p1, const Path &p2) {
        if (p1.path.len != p2.path.len) return false;
        return !cpath_str_compare_safe(p1.path.buf, p2.path.buf, p1.path.len);
    }

    inline friend bool operator!=(const Path &p1, const Path &p2) {
        return !(p1 == p2);
    }

    inline Path &operator/=(const Path &other) {
        internals::cpathConcat(&path, &other.path);
        return *this;
    }

    inline Path &operator/=(const RawChar *str) {
        internals::cpathConcatStr(&path, str);
        return *this;
    }

#if CPATH_UNICODE
    inline Path &operator/=(const char *str) {
        internals::cpathConcatStr(&path, str);
        return *this;
    }
#endif

    inline friend Path operator/(Path lhs, const Path &rhs) {
        lhs /= rhs;
        return lhs;
    }

    inline friend Path operator/(Path lhs, const char *&rhs) {
        lhs /= rhs;
        return lhs;
    }

#if CPATH_UNICODE
    inline friend Path operator/(Path lhs, const RawChar *&rhs) {
        lhs /= rhs;
        return lhs;
    }
#endif
};

struct File {
private:
    RawFile file;
    bool hasArg;

public:
    inline bool LoadFileInfo() {
        return internals::cpathGetFileInfo(&file);
    }

    inline bool LoadFlags(struct Dir &dir, void *data);

    inline RawFile *GetRawFile() {
        return &file;
    }

    inline const RawFile *GetRawFileConst() const {
        return &file;
    }

    inline File(RawFile file) : file(file), hasArg(true) {}
    inline File() : hasArg(false) {}

    inline static Opt<File, Error::Type> OpenFile(const Path &path) {
        RawFile file;
        if (!internals::cpathOpenFile(&file, path.GetRawPath())) {
            return Error::FromErrno();
        } else {
            return File(file);
        }
    }

    inline Opt<Dir, Error::Type> ToDir() const;

    inline bool IsSpecialHardLink() const {
        if (!hasArg) {
            errno = EINVAL;
            return false;
        }
        return internals::cpathFileIsSpecialHardLink(&file);
    }

    inline bool IsDir() const {
        if (!hasArg) {
            errno = EINVAL;
            return false;
        }
        return file.isDir;
    }

    inline bool IsReg() const {
        if (!hasArg) {
            errno = EINVAL;
            return false;
        }
        return file.isReg;
    }

    inline bool IsSym() const {
        if (!hasArg) {
            errno = EINVAL;
            return false;
        }
        return file.isSym;
    }

    inline const RawChar *Extension() {
        if (internals::cpathGetExtension(&file)) {
            return file.extension;
        } else {
            return NULL;
        }
    }

    inline Time GetLastAccess() {
        return internals::cpathGetLastAccess(&file);
    }

    inline Time GetLastModification() {
        return internals::cpathGetLastModification(&file);
    }

    inline Time GetFileSize() {
        return internals::cpathGetFileSize(&file);
    }

    inline double GetFileSizeDec(int intervalSize) {
        return internals::cpathGetFileSizeDec(&file, intervalSize);
    }

    inline const RawChar *GetFileSizeSuffix(ByteRep rep) {
        return internals::cpathGetFileSizeSuffix(&file, rep);
    }

    inline Path GetPath() const {
        return cpath::Path(file.path);
    }

    inline const RawChar *Name() {
        return file.name;
    }

#if !defined _MSC_VER
#if defined __MINGW32__
    inline RawStat Stat() {
        if (!file.statLoaded) internals::cpathGetFileInfo(&file);
        return file.stat;
    }
#else
    inline RawStat Stat() {
        if (!file.statLoaded) internals::cpathGetFileInfo(&file);
        return file.stat;
    }
#endif
#endif
};

struct Dir {
private:
    RawDir dir;
    bool loadedFiles;

public:
    inline Dir(const Path &path) {
        loadedFiles = false;
        internals::cpathOpenDir(&dir, path.GetRawPath());
    }
    inline Dir(RawDir dir) : dir(dir) {
        loadedFiles = false;
    }
    inline Dir() {
        loadedFiles = false;
        internals::cpathOpenDir(&dir, Path().GetRawPath());
    }

    inline static Opt<Dir, Error::Type> Open(const File &file) {
        RawDir dir;
        internals::cpathFileToDir(&dir, file.GetRawFileConst());
        return Dir(dir);
    }

    inline void Traverse(TraversalIt it, ErrorHandler err, int visitSubDir,
                                             int depth, void *data) {
        while (Opt<File, Error::Type> file = GetNextFile()) {
            if (file && it) it(**file, *this, depth, data);
            if (!file) {
                Error::WriteToErrno(file.GetErr());
                if (err) err();
                continue;
            }

            if (file->IsDir() && !file->IsSpecialHardLink()) {
                Opt<Dir, Error::Type> dir = file->ToDir();
                if (!dir) {
                    Error::WriteToErrno(dir.GetErr());
                    if (err) err();
                    continue;
                }
                dir->Traverse(it, err, visitSubDir, depth + 1, data);
                dir->Close();
            }
        }
    }

    inline bool OpenEmplace(const Path &path) {
        internals::cpathCloseDir(&dir);
        loadedFiles = false;
        return internals::cpathOpenDir(&dir, path.GetRawPath());
    }

    inline void Close() {
        return internals::cpathCloseDir(&dir);
    }

    inline void Sort(internals::cpath_cmp cmp) {
        loadedFiles = true; // will load files as required
        internals::cpathSort(&dir, cmp);
    }

    inline bool MoveNext() {
        return internals::cpathMoveNextFile(&dir);
    }

    inline RawDir *GetRawDir() {
        return &dir;
    }

    inline Opt<File, Error::Type> PeekNextFile() {
        RawFile file;
        if (!internals::cpathPeekNextFile(&dir, &file)) {
            return Error::FromErrno();
        } else {
            return File(file);
        }
    }

    inline Opt<File, Error::Type> GetNextFile() {
        RawFile file;
        if (!internals::cpathGetNextFile(&dir, &file)) {
            return Error::FromErrno();
        } else {
            return File(file);
        }
    }

    inline bool LoadFiles() {
        loadedFiles = true;
        return internals::cpathLoadAllFiles(&dir);
    }

    inline unsigned long Size() {
        if (!loadedFiles && !LoadFiles()) return 0;
        return dir.size;
    }

    inline Opt<File, Error::Type> GetFile(unsigned long n) {
        RawFile file;
        if (!internals::cpathGetFile(&dir, &file, n)) {
            return Error::FromErrno();
        } else {
            return File(file);
        }
    }

    inline bool OpenSubFileEmplace(const File &file, bool saveDir) {
        loadedFiles = false;
        return internals::cpathOpenSubFileEmplace(&dir, file.GetRawFileConst(), saveDir);
    }

    inline bool OpenSubDirEmplace(unsigned int n, bool saveDir) {
        loadedFiles = false;
        return internals::cpathOpenSubDirEmplace(&dir, n, saveDir);
    }

    inline Opt<Dir, Error::Type> OpenSubDir(unsigned int n) {
        RawDir raw;
        if (!internals::cpathOpenSubDir(&raw, &dir, n)) {
            return Error::FromErrno();
        } else {
            return Dir(raw);
        }
    }

    inline Dir OpenNextSubDir() {
        RawDir raw;
        internals::cpathOpenNextSubDir(&raw, &dir);
        return Dir(raw);
    }

    inline Dir OpenCurrentSubDir() {
        RawDir raw;
        internals::cpathOpenCurrentSubDir(&raw, &dir);
        return Dir(raw);
    }

    inline bool OpenNextSubDirEmplace(bool saveDir) {
        loadedFiles = false;
        return internals::cpathOpenNextSubDirEmplace(&dir, saveDir);
    }

    inline bool OpenCurrentSubDirEmplace(bool saveDir) {
        loadedFiles = false;
        return internals::cpathOpenCurrentSubDirEmplace(&dir, saveDir);
    }

    inline bool RevertEmplace() {
        return internals::cpathRevertEmplaceCopy(&dir);
    }
};

bool File::LoadFlags(struct Dir &dir, void *data) {
    return internals::cpathLoadFlags(dir.GetRawDir(), &file, data);
}

Opt<Dir, Error::Type> File::ToDir() const {
    return Dir::Open(*this);
}

}

#endif
#endif /* CPath */
       
