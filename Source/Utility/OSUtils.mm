
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "OSUtils.h"

#import <Metal/Metal.h>
#include <objc/runtime.h>

#if JUCE_MAC
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <Carbon/Carbon.h>
#import <string>
#include <raw_keyboard_input/raw_keyboard_input.mm>


static char insetTitlebarKey;
static IMP originalCloseButtonOrigin = nullptr;
static IMP originalTitlebarHeight = nullptr;

static bool usesInsetTitlebar(NSView* frameView)
{
    NSWindow* window = frameView.window;
    if (!window || (window.styleMask & NSWindowStyleMaskFullScreen) || window.level == NSPopUpMenuWindowLevel)
        return false;

    return [objc_getAssociatedObject(window, &insetTitlebarKey) boolValue];
}

static NSPoint insetCloseButtonOrigin(NSView* frameView, SEL selector)
{
    if (usesInsetTitlebar(frameView))
        return { 15, frameView.bounds.size.height - 26 };

    return reinterpret_cast<NSPoint (*)(id, SEL)>(originalCloseButtonOrigin)(frameView, selector);
}

static CGFloat insetTitlebarHeight(NSView* frameView, SEL selector)
{
    if (usesInsetTitlebar(frameView))
        return 34;

    return reinterpret_cast<CGFloat (*)(id, SEL)>(originalTitlebarHeight)(frameView, selector);
}

// These are private AppKit methods: if one is gone or its signature changed, keep the native layout
static IMP replaceFrameViewMethod(Class frameViewClass, SEL selector, char const* expectedReturnType, IMP replacement)
{
    Method method = class_getInstanceMethod(frameViewClass, selector);
    if (!method || method_getNumberOfArguments(method) != 2)
        return nullptr;

    char* returnType = method_copyReturnType(method);
    bool const matches = strcmp(returnType, expectedReturnType) == 0;
    free(returnType);

    return matches ? method_setImplementation(method, replacement) : nullptr;
}

// Installed once and never swapped back; the per-window flag decides which windows get the inset layout
static void installInsetTitlebarOverrides(Class frameViewClass)
{
    if (!originalCloseButtonOrigin)
        originalCloseButtonOrigin = replaceFrameViewMethod(frameViewClass, @selector(_closeButtonOrigin), @encode(NSPoint), reinterpret_cast<IMP>(insetCloseButtonOrigin));
    if (!originalTitlebarHeight)
        originalTitlebarHeight = replaceFrameViewMethod(frameViewClass, @selector(_titlebarHeight), @encode(CGFloat), reinterpret_cast<IMP>(insetTitlebarHeight));
}

int getStyleMask(bool nativeTitlebar) {
    
    unsigned int style = NSWindowStyleMaskTitled;
    
    style |= NSWindowStyleMaskMiniaturizable;
    style |= NSWindowStyleMaskClosable;
    style |= NSWindowStyleMaskResizable;
    
    if(!nativeTitlebar) {
        style |= NSWindowStyleMaskFullSizeContentView;
    }

    return style;
}

void OSUtils::setWindowMovable(juce::ComponentPeer* peer, bool canMove) {
    auto* view = static_cast<NSView*>(peer->getNativeHandle());
    
    if(!view) return;
    
    NSWindow* window = view.window;
    if(window)
    {
        [window setMovable: canMove];
    }
}

@interface NSWindow (Private)
- (void)_tileTitlebarAndRedisplay:(BOOL)redisplay;
@end

static void applyInsetTitlebar(NSWindow* window, bool enable)
{
    if (window.styleMask & NSWindowStyleMaskFullScreen) {
        // Take the window from the notification: the peer can be deleted before fullscreen is exited
        __block id token = [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowDidExitFullScreenNotification
                        object:window
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification* notification) {
                        [[NSNotificationCenter defaultCenter] removeObserver:token];
                        applyInsetTitlebar(static_cast<NSWindow*>(notification.object), enable);
                    }];
        return;
    }

    objc_setAssociatedObject(window, &insetTitlebarKey, @(enable), OBJC_ASSOCIATION_RETAIN_NONATOMIC);

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
    
    // Non-opaque window with a dropshadow has a negative impact on rendering performance
    window.opaque = TRUE;

    // Fixes shadow rendering bug by ensuring there is always something visible to draw a shadow behind
    window.backgroundColor = [NSColor windowBackgroundColor];
    
    // Only after the styleMask change: JUCE creates the window borderless, without these methods
    NSView* frameView = window.contentView.superview;
    installInsetTitlebarOverrides([frameView class]);

    if ([frameView respondsToSelector:@selector(_tileTitlebarAndRedisplay:)]) {
      [(id)frameView _tileTitlebarAndRedisplay:NO];
    }
    
    [window update];
}

void OSUtils::enableInsetTitlebarButtons(juce::ComponentPeer* peer, bool enable) {
    auto* view = static_cast<NSView*>(peer->getNativeHandle());
    if(!view) return;

    if (NSWindow* window = view.window)
        applyInsetTitlebar(window, enable);
}

void OSUtils::hideTitlebarButtons(juce::ComponentPeer* peer, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton)
{
    auto* view = static_cast<NSView*>(peer->getNativeHandle());
    if(!view) return;
    
    NSWindow* nsWindow = [view window];
    NSButton *minimizeButton = [nsWindow standardWindowButton:NSWindowMiniaturizeButton];
    NSButton *maximizeButton = [nsWindow standardWindowButton:NSWindowZoomButton];
    NSButton *closeButton = [nsWindow standardWindowButton:NSWindowCloseButton];

        [minimizeButton setHidden: hideMinimiseButton ? YES : NO];
        [minimizeButton setEnabled: hideMinimiseButton ? NO : YES];
        [maximizeButton setHidden: hideMaximiseButton ? YES : NO];
        [maximizeButton setEnabled: hideMaximiseButton ? NO : YES];
        [closeButton setHidden: hideCloseButton ? YES : NO];
        [closeButton setEnabled: hideCloseButton ? NO : YES];
}

OSUtils::KeyboardLayout OSUtils::getKeyboardLayout()
{
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    auto *s = (__bridge NSString *)(TISGetInputSourceProperty(source, kTISPropertyInputSourceID));
    auto const* locale = [[s substringWithRange:NSMakeRange(20, [s length]-20)] UTF8String];
    
    if(!strcmp(locale, "Belgian") || !strcmp(locale, "French") || !strcmp(locale, "ABC-AZERTY")) {
        return AZERTY;
    }
    
    return QWERTY;
}

@interface ScrollEventObserver : NSObject
- (instancetype)initWithScrollingFlag:(bool*)scrollingFlag;
@end

@implementation ScrollEventObserver {
    bool* isGesturingFlag;
}

- (instancetype)initWithScrollingFlag:(bool*)gesturingFlag {
    self = [super init];
    if (self) {
        isGesturingFlag = gesturingFlag;
    }
    return self;
}


- (void)scrollEventOccurred:(NSEvent*)event {
    if (event.phase == NSEventPhaseBegan) {
        *isGesturingFlag = true;
    } else if (event.phase == NSEventPhaseEnded || event.phase == NSEventPhaseCancelled) {
        *isGesturingFlag = false;
    }
}

@end


OSUtils::ScrollTracker::ScrollTracker()
{
    // Create the ScrollEventObserver instance
    observer = [[ScrollEventObserver alloc] initWithScrollingFlag:&gesturing];
    
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel handler:^NSEvent* (NSEvent* event) {
        [(ScrollEventObserver*)observer scrollEventOccurred:event];
        return event;
    }];
    
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify handler:^NSEvent* (NSEvent* event) {
        [(ScrollEventObserver*)observer scrollEventOccurred:event];
        return event;
    }];
}

OSUtils::ScrollTracker::~ScrollTracker()
{
    // Remove the observer when no longer needed
    auto* ob = static_cast<ScrollEventObserver*>(observer);
    [[NSNotificationCenter defaultCenter] removeObserver:ob];
    [ob dealloc];
}

float OSUtils::MTLGetPixelScale(void* view) {
    auto* nsView = reinterpret_cast<NSView*>(view);
    
    CGFloat scale = 1.0;
        if ([nsView respondsToSelector:@selector(convertRectToBacking:)]) {
            NSRect backingRect = [nsView convertRectToBacking:[nsView bounds]];
            scale = backingRect.size.width / [nsView bounds].size.width;
        } else {
            // Fallback for macOS versions that do not support backing scale
            NSWindow *window = [nsView window];
            if (window) {
                scale = [window backingScaleFactor];
            }
        }
        return scale;
}

@interface MTLCustomView : NSView
@end

@implementation MTLCustomView
- (NSView *)hitTest:(NSPoint)point {
    return nil;
}
@end

void* OSUtils::MTLCreateView(void* parent, int x, int y, int width, int height)
{
    NSView *childView = [[MTLCustomView alloc] initWithFrame:NSMakeRect(x, y, width, height)];

    CAMetalLayer* metalLayer = [CAMetalLayer new];
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = NO;
    metalLayer.presentsWithTransaction = NO;
    metalLayer.displaySyncEnabled = YES;
    childView.layer = metalLayer;
    childView.wantsLayer = YES;

    auto* parentView = reinterpret_cast<NSView*>(parent);
    [parentView addSubview:childView];

    // Now that the view is in a window, match the layer's point<->pixel scale to
    // the backing scale so the drawable is presented at the correct on-screen size.
    CGFloat const scale = childView.window ? childView.window.backingScaleFactor : 2.0;
    metalLayer.contentsScale = scale;
    metalLayer.drawableSize = CGSizeMake(width * scale, height * scale);

    return childView;
}

void* OSUtils::MTLGetLayer(void* view)
{
    return reinterpret_cast<NSView*>(view).layer;
}

void OSUtils::MTLDeleteView(void* view)
{
    auto* viewToRemove = reinterpret_cast<NSView*>(view);
    [viewToRemove removeFromSuperview];
    [viewToRemove release];
}

void OSUtils::MTLSetVisible(void* view, bool shouldBeVisible)
{
    auto* viewToShow = reinterpret_cast<NSView*>(view);
    
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [viewToShow setHidden:!shouldBeVisible];
    [CATransaction commit];
}


#endif

#if JUCE_IOS
#import <UIKit/UIKit.h>

void OSUtils::MTLSetVisible(void* view, bool shouldBeVisible)
{
    auto* viewToShow = reinterpret_cast<UIView*>(view);
    [viewToShow setHidden:!shouldBeVisible];
}


OSUtils::KeyboardLayout OSUtils::getKeyboardLayout()
{
    // This is only for keyboard shortcuts, so it doens't really matter much on iOS
    return QWERTY;
}

extern "C"
{
    void pd_error(const void *object, const char *fmt, ...);
}

juce::BorderSize<int> OSUtils::getSafeAreaInsets()
{    
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    if (@available(iOS 11.0, *)) {
        UIEdgeInsets insets = window.safeAreaInsets;
        return juce::BorderSize<int>(insets.top + (isIPad() ? 18 : 0), insets.left, insets.bottom  - std::max<int>(0, insets.bottom - (isIPad() ? 20 : 0)), insets.right);
    }
    
    // Fallback for older iOS versions or devices without safeAreaInsets
    return juce::BorderSize<int>();
}

bool OSUtils::isIPad()
{
    return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad;
}

void OSUtils::showiOSNativeMenu(juce::ComponentPeer* peer, juce::String const& title, SmallArray<TouchPopupMenuItem> const& items, SmallArray<SmallArray<TouchPopupMenuItem>> const& sub, juce::Point<int> screenPosition)
{
    static std::function<void(UIViewController*, UIView*, juce::ComponentPeer*, NSString*, SmallArray<TouchPopupMenuItem>, SmallArray<SmallArray<TouchPopupMenuItem>>, juce::Point<int>)> presentMenu;
    
    presentMenu = [](UIViewController* viewController, UIView* view, juce::ComponentPeer* peer,
                                                 NSString* title,
                                                 SmallArray<TouchPopupMenuItem> items,
                                                 SmallArray<SmallArray<TouchPopupMenuItem>> subMenus, juce::Point<int> pos)
     {
         UIAlertController* alertController = [UIAlertController alertControllerWithTitle:title
                                                                                  message:nil
                                                                           preferredStyle:UIAlertControllerStyleActionSheet];
         for (auto item : items)
         {
             auto* nsTitle = (NSString* _Nonnull)[NSString stringWithUTF8String:item.title.toUTF8()];

             if (item.subMenuIndex >= 0)
             {
                 auto subItems = subMenus[item.subMenuIndex];
                 UIAlertAction* action = [UIAlertAction actionWithTitle:nsTitle
                                                                  style:UIAlertActionStyleDefault
                                                                handler:^(UIAlertAction*) {
                     presentMenu(viewController, view, peer, nsTitle, subItems, subMenus, pos);
                 }];
                 action.enabled = item.active;
                 [action setValue:[nsTitle stringByAppendingString:@"  ›"] forKey:@"title"];
                 [alertController addAction:action];
             }
             else
             {
                 UIAlertAction* action = [UIAlertAction actionWithTitle:nsTitle
                                                                  style:UIAlertActionStyleDefault
                                                                handler:^(UIAlertAction*) {
                     if (item.callback)
                         item.callback();
                 }];
                 action.enabled = item.active;
                 [alertController addAction:action];
             }
         }
         UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                                style:UIAlertActionStyleCancel
                                                              handler:nil];
         [alertController addAction:cancelAction];

         if (OSUtils::isIPad())
         {
             alertController.preferredContentSize = view.frame.size;
             if (auto* popoverController = alertController.popoverPresentationController)
             {
                 popoverController.sourceView = view;
                 popoverController.sourceRect = CGRectMake(pos.x, pos.y + 5.f, 20.0f, 5.0f);
                 popoverController.canOverlapSourceViewRect = YES;
             }
         }

         [viewController presentViewController:alertController animated:YES completion:nil];
     };
    
    auto* view = (UIView<CALayerDelegate>*)peer->getNativeHandle();
    UIViewController* viewController = nil;
    UIResponder* responder = view;
    while (responder) {
        if ([responder isKindOfClass:[UIViewController class]]) {
            viewController = (UIViewController*)responder;
            break;
        }
        responder = [responder nextResponder];
    }

    auto* nsTitle = (NSString* _Nonnull)[NSString stringWithUTF8String:title.toUTF8()];
    
    auto globalScale = juce::Desktop::getInstance().getGlobalScaleFactor();
    if (viewController)
        presentMenu(viewController, view, peer, nsTitle, items, sub, screenPosition * globalScale);
}

// The method implementation that will be added to JuceAppStartupDelegate
BOOL openURLImplementation(id self, SEL _cmd, UIApplication* app, NSURL* url, NSDictionary* options)
{
    if (url && url.isFileURL)
    {
        NSString *filePath = [url path];
        juce::String juceFilePath = juce::String::fromUTF8([filePath UTF8String]);
        [url startAccessingSecurityScopedResource];
        
        if (juce::JUCEApplicationBase::getInstance())
        {
            juce::MessageManager::callAsync([juceFilePath]()
            {
                if (auto* app = juce::JUCEApplicationBase::getInstance())
                {
                    app->anotherInstanceStarted(juceFilePath.quoted());
                }
            });
        }
        
        return YES;
    }
    
    return NO;
}

// Function to add the method to the existing delegate class
bool OSUtils::addOpenURLMethodToDelegate()
{
    Class delegateClass = objc_getClass("JuceAppStartupDelegate");
    if (delegateClass)
    {
        // Add the openURL:options: method
        SEL selector = @selector(application:openURL:options:);
        const char* types = "B@:@@@"; // Returns BOOL, takes id, SEL, UIApplication*, NSURL*, NSDictionary*
        
        class_addMethod(delegateClass, selector, (IMP)openURLImplementation, types);
        return true;
    }
    
    return false;
}

@interface NVGMetalView : UIView

@property (nonatomic, strong) CAMetalLayer *metalLayer;

@end

@implementation NVGMetalView

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self commonInit];
    }
    return self;
}

// Always return NO to pass through touch events
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    return NO;
}

- (void)commonInit {
    self.metalLayer = (CAMetalLayer *)self.layer;
    [self.metalLayer setPresentsWithTransaction:FALSE];
    [self.metalLayer setFramebufferOnly:FALSE];
}

@end

float OSUtils::MTLGetPixelScale(void* view) {
    return reinterpret_cast<UIView*>(view).window.screen.scale;
}

void* OSUtils::MTLCreateView(void* parent, int x, int y, int width, int height)
{
    // Create child view
    NVGMetalView *childView = [[NVGMetalView alloc] initWithFrame:CGRectMake(x, y, width, height)];
    UIView *parentView = reinterpret_cast<UIView*>(parent);
    
    // Add child view as a subview of parent view
    [parentView addSubview:childView];

    return childView;
}

void* OSUtils::MTLGetLayer(void* view)
{
    return reinterpret_cast<UIView*>(view).layer;
}

void OSUtils::MTLDeleteView(void* view)
{
    UIView *viewToRemove = reinterpret_cast<UIView*>(view);
    [viewToRemove removeFromSuperview];
    [viewToRemove release];
}

#endif



