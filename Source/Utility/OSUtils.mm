
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "OSUtils.h"

#if JUCE_MAC
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#import <string>
#include <raw_keyboard_input/raw_keyboard_input.mm>

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

void OSUtils::enableInsetTitlebarButtons(void* nativeHandle, bool enable) {
    
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

void OSUtils::HideTitlebarButtons(void* view, bool hideMinimiseButton, bool hideMaximiseButton, bool hideCloseButton)
{
	NSView* nsView = (NSView*)view;
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
    NSString *s = (__bridge NSString *)(TISGetInputSourceProperty(source, kTISPropertyInputSourceID));
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
    
    [[NSNotificationCenter defaultCenter] removeObserver:static_cast<ScrollEventObserver*>(observer)];
}
#endif

#if JUCE_IOS
#import <UIKit/UIKit.h>

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
}

- (instancetype)initWithComponentPeer:(juce::ComponentPeer*)componentPeer scrollState:(bool*)scrollState {
    self = [super init];
    
    peer = componentPeer;
    view = (UIView<CALayerDelegate>*)peer->getNativeHandle();
    isScrolling = scrollState;
    lastScale = 1.0;

    UIPinchGestureRecognizer* pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(pinchEventOccurred:)];
    //[pinchGesture setCancelsTouchesInView: true];
    //[pinchGesture setDelaysTouchesBegan: true];
    [view addGestureRecognizer:pinchGesture];
    
    UILongPressGestureRecognizer* longPressGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPressEventOccurred:)];
    //[longPressGesture setCancelsTouchesInView: true];
    //[longPressGesture setDelaysTouchesBegan: true];
    [view addGestureRecognizer:longPressGesture];
    
    UIPanGestureRecognizer* panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(scrollEventOccurred:)];
    [panGesture setMaximumNumberOfTouches: 2];
    [panGesture setMinimumNumberOfTouches: 2];
    //[panGesture setCancelsTouchesInView: true];
    //[panGesture setDelaysTouchesBegan: true];
    [view addGestureRecognizer:panGesture];
    
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
    
    const auto offset = [gesture translationInView: view];
    const auto scale = 1.0f / 256.0f;
    
    if(gesture.state == UIGestureRecognizerStateBegan)
    {
        *isScrolling = true;
        lastPosition = {0.0f, 0.0f};
        [gesture setCancelsTouchesInView: true];
        return;
    }
    if(gesture.state == UIGestureRecognizerStateEnded)
    {
        *isScrolling = false;
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

    const auto eventMousePosition = [gesture locationInView: view];
    const auto reconstructedMousePosition =  juce::Point<float>(eventMousePosition.x, eventMousePosition.y) -  juce::Point<float>(offset.x, offset.y);
    const auto time = (juce::Time::currentTimeMillis() - juce::Time::getMillisecondCounter())
    + (juce::int64) ([[NSProcessInfo processInfo] systemUptime] * 1000.0);
        
    peer->handleMouseWheel (juce::MouseInputSource::InputSourceType::touch, reconstructedMousePosition, time, details);
}

@end

OSUtils::ScrollTracker::ScrollTracker(juce::ComponentPeer* peer)
{
    // Create the ScrollEventObserver instance
    observer = [[ScrollEventObserver alloc] initWithComponentPeer:peer scrollState: &scrolling];
}

OSUtils::ScrollTracker::~ScrollTracker()
{
    // TODO: clean up!
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
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Select an Option"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleActionSheet];
    
        UIAlertAction *themeAction = [UIAlertAction actionWithTitle:@"Select Theme..."
                                                                style:UIAlertActionStyleDefault
                                                              handler:^(UIAlertAction * _Nonnull action) {
                                                                  // Create a second UIAlertController for the submenu
                                                                  UIAlertController *submenu = [UIAlertController alertControllerWithTitle:@"Submenu Options"
                                                                                                                                  message:nil
                                                                                                                           preferredStyle:UIAlertControllerStyleActionSheet];

                                                                  // Add actions for the submenu
                                                                  UIAlertAction *subAction1 = [UIAlertAction actionWithTitle:@"First Theme (Light)"
                                                                                                                      style:UIAlertActionStyleDefault
                                                                                                                    handler:^(UIAlertAction * _Nonnull action) {
                                                                      callback(7);
                                                                                                                    }];

                                                                  UIAlertAction *subAction2 = [UIAlertAction actionWithTitle:@"Second Theme (dark)"
                                                                                                                      style:UIAlertActionStyleDefault
                                                                                                                    handler:^(UIAlertAction * _Nonnull action) {
                                                                      callback(8);
                                                                                                                    }];

                                                                UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                                                                                        style:UIAlertActionStyleDefault
                                                                                                                      handler:^(UIAlertAction * _Nonnull action) {
                                                                    callback(-1);
                                                                                                                      }];
            
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
        
        UIAlertAction *savePatchAction = [UIAlertAction actionWithTitle:@"Save Patch"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(3);
                                                            }];
        
        UIAlertAction *savePatchAsAction = [UIAlertAction actionWithTitle:@"Save Patch As"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(4);
                                                            }];
        UIAlertAction *settingsAction = [UIAlertAction actionWithTitle:@"Settings"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(5);
                                                            }];
        UIAlertAction *aboutAction = [UIAlertAction actionWithTitle:@"About"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            callback(6);
                                                            }];
        
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                               style:UIAlertActionStyleCancel
                                                             handler:^(UIAlertAction * _Nonnull action) {
            callback(-1);
                                                             }];

        [alertController addAction:newPatchAction];
        [alertController addAction:openPatchAction];
        [alertController addAction:savePatchAction];
        [alertController addAction:savePatchAsAction];
        [alertController addAction:themeAction];
        [alertController addAction:settingsAction];
        [alertController addAction:aboutAction];
        [alertController addAction:cancelAction];
    
        // Present the alert controller using the found view controller
        [viewController presentViewController:alertController animated:YES completion:nil];
    }
    else {
        NSLog(@"Failed to find a UIViewController to present the UIAlertController.");
    }
}

void OSUtils::showMobileCanvasMenu(juce::ComponentPeer* peer)
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
    
       
        UIAlertAction *openAction = [UIAlertAction actionWithTitle:@"Open"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            //callback(1);
                                                            }];
        UIAlertAction *encapsulateAction = [UIAlertAction actionWithTitle:@"Encapsulate"
                                              style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * _Nonnull action) {
            //callback(2);
                                            }];
        
        UIAlertAction *propertiesAction = [UIAlertAction actionWithTitle:@"Properties"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            //callback(3);
                                                            }];
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel"
                                                              style:UIAlertActionStyleDefault
                                                            handler:^(UIAlertAction * _Nonnull action) {
            //callback(3);
                                                            }];

        [alertController addAction:openAction];
        [alertController addAction:encapsulateAction];
        [alertController addAction:propertiesAction];
        [alertController addAction:cancelAction];

        // Present the alert controller using the found view controller
        [viewController presentViewController:alertController animated:YES completion:nil];
    }
    else {
        NSLog(@"Failed to find a UIViewController to present the UIAlertController.");
    }
}


#endif
