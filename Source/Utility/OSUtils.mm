
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


@interface NSView (FrameViewMethodSwizzling)
- (NSPoint)FrameView__closeButtonOrigin;
- (CGFloat)FrameView__titlebarHeight;
@end

@implementation NSView (FrameViewMethodSwizzling)

- (NSPoint)FrameView__closeButtonOrigin {
    auto* win = static_cast<NSWindow*>(self.window);
    auto isFullscreen = win.styleMask & NSWindowStyleMaskFullScreen;
    auto isPopup = win.level == NSPopUpMenuWindowLevel;
    auto isInset = win.titleVisibility == NSWindowTitleVisible;
    if(isPopup || isFullscreen || isInset)
        return [self FrameView__closeButtonOrigin];
    return {15, self.bounds.size.height - 28};
}
- (CGFloat)FrameView__titlebarHeight {
    auto* win = static_cast<NSWindow*>(self.window);
    auto isFullscreen = win.styleMask & NSWindowStyleMaskFullScreen;
    auto isPopup = win.level == NSPopUpMenuWindowLevel;
    auto isInset = win.titleVisibility == NSWindowTitleVisible;
    if(isPopup || isFullscreen || isInset)
        return [self FrameView__titlebarHeight];
    return 34;
}
@end

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

void OSUtils::setWindowMovable(void* nativeHandle, bool canMove) {
    auto* view = static_cast<NSView*>(nativeHandle);
    
    if(!view) return;
    
    NSWindow* window = view.window;
    if(window)
    {
        [window setMovable: canMove];
    }
}

void OSUtils::enableInsetTitlebarButtons(void* nativeHandle, bool enable) {
    auto* view = static_cast<NSView*>(nativeHandle);
    if(!view) return;
    
    NSWindow* window = view.window;
    if(!window) return;
    
    // Swaps out the implementation of one Obj-c instance method with another
    auto swizzleMethods = [](Class aClass, SEL orgMethod, SEL posedMethod) {
        @try {
            Method original = nil;
            Method posed = nil;

            original = class_getInstanceMethod(aClass, orgMethod);
            posed = class_getInstanceMethod(aClass, posedMethod);
            
            if (!original || !posed) return;

            method_exchangeImplementations(original, posed);
        } @catch (NSException * _exn) {
        }
    };
    
    Class frameViewClass = [[[window contentView] superview] class];
    if(frameViewClass != nil) {
        auto oldOriginMethod = enable ? @selector(_closeButtonOrigin) : @selector(FrameView__closeButtonOrigin);
        auto newOriginMethod = enable ? @selector(FrameView__closeButtonOrigin) : @selector(_closeButtonOrigin);
        
        static IMP our_closeButtonOrigin = class_getMethodImplementation([NSView class], newOriginMethod);
        IMP _closeButtonOrigin = class_getMethodImplementation(frameViewClass, oldOriginMethod);
        if (_closeButtonOrigin && _closeButtonOrigin != our_closeButtonOrigin) {
            swizzleMethods(frameViewClass,
                           oldOriginMethod,
                           newOriginMethod);
        }
        
        auto oldTitlebarHeightMethod = enable ? @selector(_titlebarHeight) : @selector(FrameView__titlebarHeight);
        auto newTitlebarHeightMethod = enable ? @selector(FrameView__titlebarHeight) : @selector(_titlebarHeight);
        
        static IMP our_titlebarHeight = class_getMethodImplementation([NSView class], newTitlebarHeightMethod);
        IMP _titlebarHeight = class_getMethodImplementation([view class], oldTitlebarHeightMethod);
        if (_titlebarHeight && _titlebarHeight != our_titlebarHeight) {
            swizzleMethods(frameViewClass,
                           oldTitlebarHeightMethod,
                           newTitlebarHeightMethod);
        }
    }
    
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
    
    NSView* frameView = window.contentView.superview;
    if ([frameView respondsToSelector:@selector(_tileTitlebarAndRedisplay:)]) {
      [frameView _tileTitlebarAndRedisplay:NO];
    }
    
    [window update];
}

void OSUtils::hideTitlebarButtons(void* view, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton)
{
    auto* nsView = (NSView*)view;
    NSWindow* nsWindow = [nsView window];
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
    bool* isScrollingFlag;
}

- (instancetype)initWithScrollingFlag:(bool*)scrollingFlag {
    self = [super init];
    if (self) {
        isScrollingFlag = scrollingFlag;
    }
    return self;
}


- (void)scrollEventOccurred:(NSEvent*)event {
    if (event.phase == NSEventPhaseBegan) {
        *isScrollingFlag = true;
    } else if (event.phase == NSEventPhaseEnded || event.phase == NSEventPhaseCancelled) {
        *isScrollingFlag = false;
    }
}

@end


OSUtils::ScrollTracker::ScrollTracker()
{
    // Create the ScrollEventObserver instance
    observer = [[ScrollEventObserver alloc] initWithScrollingFlag:&scrolling];
    
    NSEventMask scrollEventMask = NSEventMaskScrollWheel;
            [NSEvent addLocalMonitorForEventsMatchingMask:scrollEventMask handler:^NSEvent* (NSEvent* event) {
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

void* OSUtils::MTLCreateView(void* parent, int x, int y, int width, int height)
{
    // Create child view
    NSView *childView = [[NSView alloc] initWithFrame:NSMakeRect(x, y, width, height)];
    auto* parentView = reinterpret_cast<NSView*>(parent);
    
    // Add child view as a subview of parent view
    [parentView addSubview:childView];
    
    return childView;
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

@interface ScrollEventObserver : NSObject
- (instancetype)initWithComponentPeer:(juce::ComponentPeer*)peer scrollState:(bool*)scrollState;
@end

@implementation ScrollEventObserver {
    juce::ComponentPeer* peer;
    UIView<CALayerDelegate>* view;
    juce::Point<float> lastPosition;
    double lastScale;
    bool* isScrolling;
    bool* allowOneFingerScroll;
    
    // Inertia variables
    juce::Point<float> velocity;
    juce::Point<float> lastMousePosition;
    int lastNumberOfTouches;
    std::unique_ptr<juce::TimedCallback> inertiaTimer;
}

- (instancetype)initWithComponentPeer:(juce::ComponentPeer*)componentPeer scrollState:(bool*)scrollState allowsOneFingerScroll:(bool*)allowsOneFingerScroll {
    self = [super init];
    
    peer = componentPeer;
    view = (UIView<CALayerDelegate>*)peer->getNativeHandle();
    isScrolling = scrollState;
    allowOneFingerScroll = allowsOneFingerScroll;
    lastScale = 1.0;
    velocity = {0.0f, 0.0f};
    lastMousePosition = {0.0f, 0.0f};
    
    inertiaTimer = std::make_unique<juce::TimedCallback>([self]() {
        [self updateInertia];
    });
    
    UIPinchGestureRecognizer* pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(pinchEventOccurred:)];
    [view addGestureRecognizer:pinchGesture];
    
    UILongPressGestureRecognizer* longPressGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPressEventOccurred:)];
    [view addGestureRecognizer:longPressGesture];
    
    UIPanGestureRecognizer* panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(scrollEventOccurred:)];
    [panGesture setMaximumNumberOfTouches: 2];
    [panGesture setMinimumNumberOfTouches: 1];
    [view addGestureRecognizer:panGesture];
    
    UITapGestureRecognizer* tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapEventOccurred:)];
    [tapGesture setCancelsTouchesInView: NO]; // Don't interfere with other gestures
    [view addGestureRecognizer:tapGesture];
    return self;
}

extern "C"
{
    void pd_error(const void *object, const char *fmt, ...);
}

- (void)longPressEventOccurred:(UILongPressGestureRecognizer*)gesture {
    
    juce::ModifierKeys::currentModifiers = juce::ModifierKeys::currentModifiers.withoutMouseButtons().withFlags (juce::ModifierKeys::rightButtonModifier);
    
    const auto eventPosition = [gesture locationOfTouch:0 inView:view];
    auto pos = juce::Point<float>(eventPosition.x, eventPosition.y);
    const auto time = (juce::Time::currentTimeMillis() - juce::Time::getMillisecondCounter()) + (juce::int64) ([[NSProcessInfo processInfo] systemUptime] * 1000.0);
    
    peer->handleMouseEvent(juce::MouseInputSource::InputSourceType::touch, pos, juce::ModifierKeys::currentModifiers,
                           juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation, time, {}, 0);
}

- (void)pinchEventOccurred:(UIPinchGestureRecognizer*)gesture {
    
    if(gesture.numberOfTouches != 2) return;
    
    const auto time = (juce::Time::currentTimeMillis() - juce::Time::getMillisecondCounter())
    + (juce::int64) ([[NSProcessInfo processInfo] systemUptime] * 1000.0);
    
    if(gesture.state == UIGestureRecognizerStateBegan || gesture.state == UIGestureRecognizerStateEnded)
    {
        lastScale = 1.0;
        return;
    }
    
    const auto point1 = [gesture locationOfTouch:0 inView:nil];
    const auto point2 = [gesture locationOfTouch:1 inView:nil];
    const auto pinchCenter = juce::Point<float>((point1.x + point2.x) / 2.0f, (point1.y + point2.y) / 2.0f);
    
    peer->handleMagnifyGesture (juce::MouseInputSource::InputSourceType::touch, pinchCenter, time, (gesture.scale - lastScale) + 1.0, 0);
    lastScale = gesture.scale;
}

- (void)scrollEventOccurred:(UIPanGestureRecognizer*)gesture {
    
    if(gesture.numberOfTouches == 1 && !*allowOneFingerScroll)
    {
        [gesture setCancelsTouchesInView: false];
        inertiaTimer->stopTimer();
        return;
    }
        
    const auto offset = [gesture translationInView: view];
    const auto scale = 1.0f / 256.0f;
    
    if(gesture.state == UIGestureRecognizerStateBegan)
    {
        *isScrolling = true;
        lastPosition = {0.0f, 0.0f};
        velocity = {0.0f, 0.0f};
        inertiaTimer->stopTimer();
        [gesture setCancelsTouchesInView: true];
        return;
    }
    
    if(gesture.state == UIGestureRecognizerStateEnded)
    {
        *isScrolling = false;
        
        // Get velocity and start inertia
        CGPoint gestureVelocity = [gesture velocityInView: view];
        velocity = {(float)gestureVelocity.x, (float)gestureVelocity.y};
        
        const float minVelocity = 100.0f;
        if (lastNumberOfTouches == 1 && *allowOneFingerScroll && (std::abs(velocity.x) > minVelocity || std::abs(velocity.y) > minVelocity))
        {
            inertiaTimer->startTimerHz(60); // 60 FPS
        }
        
        *allowOneFingerScroll = false;
        lastPosition = {0.0f, 0.0f};
        [gesture setCancelsTouchesInView: false];
        return;
    }
    
    juce::MouseWheelDetails details;
    details.deltaX = scale * (float) (offset.x - lastPosition.x);
    details.deltaY = scale * (float) (offset.y - lastPosition.y);
    details.isReversed = false;
    details.isSmooth = true;
    details.isInertial = false;
    
    lastPosition = juce::Point<float>(offset.x, offset.y);
    lastNumberOfTouches = gesture.numberOfTouches;
    
    const auto eventMousePosition = [gesture locationInView: view];
    const auto reconstructedMousePosition = juce::Point<float>(eventMousePosition.x, eventMousePosition.y) - juce::Point<float>(offset.x, offset.y);
    lastMousePosition = reconstructedMousePosition;
    
    const auto time = (juce::Time::currentTimeMillis() - juce::Time::getMillisecondCounter())
    + (juce::int64) ([[NSProcessInfo processInfo] systemUptime] * 1000.0);
        
    peer->handleMouseWheel(juce::MouseInputSource::InputSourceType::touch, reconstructedMousePosition, time, details);
}

- (void)tapEventOccurred:(UITapGestureRecognizer*)gesture {
    // Cancel any ongoing inertia scrolling
    inertiaTimer->stopTimer();
    velocity = {0.0f, 0.0f};
    *allowOneFingerScroll = false;
}

- (void)updateInertia
{
    const float decelerationRate = 0.95f;
    const float scale = 1.0f / 256.0f;
    const float dt = 1.0f / 60.0f; // Frame time at 60 FPS
    
    velocity.x *= decelerationRate;
    velocity.y *= decelerationRate;
    
    const float minVelocity = 10.0f;
    if (std::abs(velocity.x) < minVelocity && std::abs(velocity.y) < minVelocity)
    {
        inertiaTimer->stopTimer();
        return;
    }
    
    juce::MouseWheelDetails details;
    details.deltaX = scale * velocity.x * dt;
    details.deltaY = scale * velocity.y * dt;
    details.isReversed = false;
    details.isSmooth = true;
    details.isInertial = true;
    
    const auto time = (juce::Time::currentTimeMillis() - juce::Time::getMillisecondCounter())
    + (juce::int64) ([[NSProcessInfo processInfo] systemUptime] * 1000.0);

    peer->handleMouseWheel(juce::MouseInputSource::InputSourceType::touch, lastMousePosition, time, details);
}
@end

OSUtils::ScrollTracker::ScrollTracker(juce::ComponentPeer* peer)
{
    // Create the ScrollEventObserver instance
    observer = [[ScrollEventObserver alloc] initWithComponentPeer:peer scrollState: &scrolling allowsOneFingerScroll: &allowOneFingerScroll];
}

OSUtils::ScrollTracker::~ScrollTracker()
{
    auto* ob = static_cast<ScrollEventObserver*>(observer);
    [[NSNotificationCenter defaultCenter] removeObserver:ob];
    [ob dealloc];
}


juce::BorderSize<int> OSUtils::getSafeAreaInsets()
{    
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    if (@available(iOS 11.0, *)) {
        UIEdgeInsets insets = window.safeAreaInsets;
        return juce::BorderSize<int>(insets.top + (isIPad() ? 26 : 0), insets.left, insets.bottom  - std::max<int>(0, insets.bottom - (isIPad() ? 20 : 0)), insets.right);
    }
    
    // Fallback for older iOS versions or devices without safeAreaInsets
    return juce::BorderSize<int>();
}

bool OSUtils::isIPad()
{
    return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad;
}

void OSUtils::showMobileChoiceMenu(juce::ComponentPeer* peer, juce::StringArray options, std::function<void(int)> callback)
{
    auto* view = (UIView*) peer->getNativeHandle();
    if (view == nil)
        return;

    // Find the parent view controller
    UIViewController* viewController = nil;
    UIResponder* responder = view;
    while (responder != nil)
    {
        if ([responder isKindOfClass:[UIViewController class]])
        {
            viewController = (UIViewController*) responder;
            break;
        }
        responder = [responder nextResponder];
    }

    if (viewController == nil)
        return;

    UIAlertController* alertController =
        [UIAlertController alertControllerWithTitle:nil
                                            message:nil
                                     preferredStyle:UIAlertControllerStyleActionSheet];

    // Add option actions
    for (int i = 0; i < options.size(); ++i)
    {
        NSString* option = [[NSString alloc] initWithUTF8String:options[i].toRawUTF8()];
        UIAlertAction* action = [UIAlertAction actionWithTitle:option
                                     style:UIAlertActionStyleDefault
                                   handler:^(UIAlertAction*)
                                   {
                                        callback(i);
                                   }];

        [alertController addAction:action];
    }

    // Cancel button
    UIAlertAction* cancel = [UIAlertAction actionWithTitle:@"Cancel"
                                 style:UIAlertActionStyleCancel
                               handler:^(UIAlertAction*)
                               {
                                    callback(-1);
                               }];

    [alertController addAction:cancel];

    if (isIPad())
    {
        alertController.preferredContentSize = view.frame.size;

        if (auto* popoverController = alertController.popoverPresentationController)
        {
            popoverController.sourceView = view;
            popoverController.sourceRect = CGRectMake (35.0f, 1.0f, 50.0f, 50.0f);
            popoverController.canOverlapSourceViewRect = YES;
        }
    }


    // Present the alert controller using the found view controller
    [viewController presentViewController:alertController animated:YES completion:nil];
}

void OSUtils::showMobileMainMenu(juce::ComponentPeer* peer, std::function<void(int)> callback)
{
    auto* view = (UIView<CALayerDelegate>*)peer->getNativeHandle();
    
    // Find the parent view controller
    UIViewController *viewController = nil;
    UIResponder *responder = view;
    while (responder) {
        if ([responder isKindOfClass:[UIViewController class]]) {
            viewController = (UIViewController *)responder;
            break;
        }
        responder = [responder nextResponder];
    }

    if (viewController) {
        // Create an alert controller
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Main Menu"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleActionSheet];
        
        UIAlertAction *themeAction = [UIAlertAction actionWithTitle:@"Select Theme..."
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            // Create a second UIAlertController for the submenu
            UIAlertController *submenu = [UIAlertController alertControllerWithTitle:@"Themes"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleActionSheet];
            
            // Add actions for the submenu
            UIAlertAction *subAction1 = [UIAlertAction actionWithTitle:@"First Theme (Light)"
                                                                 style:UIAlertActionStyleDefault
                                                               handler:^(UIAlertAction * _Nonnull action) {
                callback(8);
            }];
            
            UIAlertAction *subAction2 = [UIAlertAction actionWithTitle:@"Second Theme (dark)"
                                                                 style:UIAlertActionStyleDefault
                                                               handler:^(UIAlertAction * _Nonnull action) {
                callback(9);
            }];
            
            UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                                   style:UIAlertActionStyleCancel
                                                                 handler:^(UIAlertAction * _Nonnull action) {
                callback(-1);
            }];
            
            if (isIPad())
            {
                
                submenu.preferredContentSize = view.frame.size;
                
                if (auto* popoverController = submenu.popoverPresentationController)
                {
                    popoverController.sourceView = view;
                    popoverController.sourceRect = CGRectMake (35.0f, 1.0f, 50.0f, 50.0f);
                    popoverController.canOverlapSourceViewRect = YES;
                }
            }
            
            [submenu addAction:subAction1];
            [submenu addAction:subAction2];
            [submenu addAction:cancelAction];
            
            
            // Present the submenu
            [viewController presentViewController:submenu animated:YES completion:nil];
        }];

        
        UIAlertAction *newPatchAction = [UIAlertAction actionWithTitle:@"New Patch"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(1);
                                                            }];
        UIAlertAction *openPatchAction = [UIAlertAction actionWithTitle:@"Open Patch"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(2);
                                            }];
        
        UIAlertAction *openPatchFolderAction = [UIAlertAction actionWithTitle:@"Open Folder"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(3);
                                            }];
        
        UIAlertAction *savePatchAction = [UIAlertAction actionWithTitle:@"Save Patch"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(4);
                                                            }];
        
        UIAlertAction *savePatchAsAction = [UIAlertAction actionWithTitle:@"Save Patch As"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(5);
                                                            }];
        UIAlertAction *settingsAction = [UIAlertAction actionWithTitle:@"Settings"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(6);
                                                            }];
        UIAlertAction *aboutAction = [UIAlertAction actionWithTitle:@"About"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(7);
                                                            }];
        
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                               style:UIAlertActionStyleCancel
                                                             handler:^(UIAlertAction * _Nonnull action) {
            callback(-1);
                                                             }];

        [alertController addAction:newPatchAction];
        [alertController addAction:openPatchAction];
        [alertController addAction:openPatchFolderAction];
        [alertController addAction:savePatchAction];
        [alertController addAction:savePatchAsAction];
        [alertController addAction:themeAction];
        [alertController addAction:settingsAction];
        [alertController addAction:aboutAction];
        [alertController addAction:cancelAction];
        
        if (isIPad())
        {
            alertController.preferredContentSize = view.frame.size;

            if (auto* popoverController = alertController.popoverPresentationController)
            {
                popoverController.sourceView = view;
                popoverController.sourceRect = CGRectMake (35.0f, 1.0f, 50.0f, 50.0f);
                popoverController.canOverlapSourceViewRect = YES;
            }
        }

    
        // Present the alert controller using the found view controller
        [viewController presentViewController:alertController animated:YES completion:nil];
    }
    else {
        NSLog(@"Failed to find a UIViewController to present the UIAlertController.");
    }
}

void OSUtils::showMobileCanvasMenu(juce::ComponentPeer* peer, std::function<void(int)> callback)
{
    auto* view = (UIView<CALayerDelegate>*)peer->getNativeHandle();
    
    // Find the parent view controller
    UIViewController *viewController = nil;
    UIResponder *responder = view;
    while (responder) {
        if ([responder isKindOfClass:[UIViewController class]]) {
            viewController = (UIViewController *)responder;
            break;
        }
        responder = [responder nextResponder];
    }

    if (viewController) {
        // Create an alert controller
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Select an Option"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleActionSheet];
    

        UIAlertAction *cutAction = [UIAlertAction actionWithTitle:@"Cut"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(1);
                                                            }];
        UIAlertAction *copyAction = [UIAlertAction actionWithTitle:@"Copy"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(2);
                                            }];
        
        UIAlertAction *pasteAction = [UIAlertAction actionWithTitle:@"Paste"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(3);
                                                            }];
        
        UIAlertAction *duplicateAction = [UIAlertAction actionWithTitle:@"Duplicate"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(4);
                                            }];
        UIAlertAction *encapsulateAction = [UIAlertAction actionWithTitle:@"Encapsulate"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(5);
                                            }];
        UIAlertAction *tidyConnectionAction = [UIAlertAction actionWithTitle:@"Tidy Connection"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(6);
                                            }];
        
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                              style:UIAlertActionStyleCancel
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(-1);
                                                            }];

        
        if (isIPad())
        {
            alertController.preferredContentSize = view.frame.size;

            if (auto* popoverController = alertController.popoverPresentationController)
            {
                auto peerBounds = peer->getBounds().toFloat();
                
                popoverController.sourceView = view;
                popoverController.sourceRect = CGRectMake (peerBounds.getCentreX() - 10.0f, peerBounds.getBottom() - 120.0f, 20.0f, 5.0f);
                popoverController.canOverlapSourceViewRect = YES;
            }
        }
        
        [alertController addAction:cutAction];
        [alertController addAction:copyAction];
        [alertController addAction:pasteAction];
        [alertController addAction:duplicateAction];
        [alertController addAction:encapsulateAction];
        [alertController addAction:tidyConnectionAction];
        [alertController addAction:cancelAction];
        // Present the alert controller using the found view controller
        [viewController presentViewController:alertController animated:YES completion:nil];
    }
    else {
        NSLog(@"Failed to find a UIViewController to present the UIAlertController.");
    }
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

void OSUtils::MTLDeleteView(void* view)
{
    UIView *viewToRemove = reinterpret_cast<UIView*>(view);
    [viewToRemove removeFromSuperview];
    [viewToRemove release];
}

#endif



