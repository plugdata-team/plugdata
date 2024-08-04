
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "OSUtils.h"

#import <Metal/Metal.h>

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
    
    auto* view = static_cast<NSView*>(nativeHandle);
    
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
    
    [[NSNotificationCenter defaultCenter] removeObserver:static_cast<ScrollEventObserver*>(observer)];
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
    [view addGestureRecognizer:pinchGesture];
    
    UILongPressGestureRecognizer* longPressGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(longPressEventOccurred:)];
    [view addGestureRecognizer:longPressGesture];
    
    UIPanGestureRecognizer* panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(scrollEventOccurred:)];
    [panGesture setMaximumNumberOfTouches: 2];
    [panGesture setMinimumNumberOfTouches: 2];
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


juce::BorderSize<int> OSUtils::getSafeAreaInsets()
{    
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    if (@available(iOS 11.0, *)) {
        UIEdgeInsets insets = window.safeAreaInsets;
        return juce::BorderSize<int>(insets.top, insets.left, insets.bottom, insets.right);
    }

    // Fallback for older iOS versions or devices without safeAreaInsets
    return juce::BorderSize<int>();
}

bool OSUtils::isIPad()
{
    return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad;
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
                                                                      callback(7);
                                                                                                                    }];

                                                                  UIAlertAction *subAction2 = [UIAlertAction actionWithTitle:@"Second Theme (dark)"
                                                                                                                      style:UIAlertActionStyleDefault
                                                                                                                    handler:^(UIAlertAction * _Nonnull action) {
                                                                      callback(8);
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



