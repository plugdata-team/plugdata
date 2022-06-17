#include <AppKit/AppKit.h>

// Workaround for restarting the message loop on macos
void stopLoop()
{
    [NSEvent stopPeriodicEvents];
}
