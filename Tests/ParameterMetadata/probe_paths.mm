// Test-host-only filesystem location isolation. Never linked into the plug-in.
// dyld applies these replacements to other images in this process, including
// the unmodified plug-in; calls within this image resolve to the originals.
#import <Foundation/Foundation.h>
#include <cstdlib>
#include <cstring>

static NSString* testHomeDirectory()
{
    if (const char* path = getenv("PLUGDATA_PARAMETER_TEST_HOME"))
        return [NSString stringWithUTF8String:path];
    return NSHomeDirectory();
}

static NSString* testTemporaryDirectory()
{
    if (const char* path = getenv("PLUGDATA_PARAMETER_TEST_TMPDIR"))
        return [NSString stringWithUTF8String:path];
    return NSTemporaryDirectory();
}

static char* testGetenv(const char* name)
{
    if (std::strcmp(name, "TMPDIR") == 0)
        if (char* path = getenv("PLUGDATA_PARAMETER_TEST_TMPDIR"))
            return path;
    return getenv(name);
}

#define TEST_INTERPOSE(replacement, original)                                                      \
    __attribute__((used)) static const struct {                                                    \
        const void* replace;                                                                       \
        const void* with;                                                                          \
    } interpose_##original __attribute__((section("__DATA,__interpose")))                          \
    = { reinterpret_cast<const void*>(&replacement), reinterpret_cast<const void*>(&original) }

TEST_INTERPOSE(testHomeDirectory, NSHomeDirectory);
TEST_INTERPOSE(testTemporaryDirectory, NSTemporaryDirectory);
TEST_INTERPOSE(testGetenv, getenv);
