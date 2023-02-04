#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include "OSUtils.h"

int getStyleMask(bool nativeTitlebar) {
    
    unsigned int style = NSWindowStyleMaskTitled;
    
    style |= NSWindowStyleMaskMiniaturizable;
    style |= NSWindowStyleMaskClosable;
    style |= NSWindowStyleMaskResizable;
    
    if(!nativeTitlebar) {
        style |= NSFullSizeContentViewWindowMask;
    }

    return style;
}

void enableInsetTitlebarButtons(void* nativeHandle, bool enable) {
    
    NSView* view = static_cast<NSView*>(nativeHandle);
    
    if(!view) return;
    
    NSWindow* window = view.window;
    
    if(enable) {
        window.titlebarAppearsTransparent = true;
        window.titleVisibility = NSWindowTitleHidden;
        window.styleMask = getStyleMask(false);
    }
    else {
        window.titlebarAppearsTransparent = false;
        window.titleVisibility = NSWindowTitleVisible;
        window.styleMask = getStyleMask(true);
    }

    [window update];
}
