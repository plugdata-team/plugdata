#include "Keyboard.h"
#include <UIKit/UIKit.h>

@interface JuceUIViewController
@end

@interface JuceUIViewController (KeyboardCatching)
- (void) pressesBegan:(NSSet*)presses withEvent:(UIPressesEvent *)event;
- (void) pressesEnded:(NSSet*)presses withEvent:(UIPressesEvent *)event;
@end

@implementation JuceUIViewController (KeyboardCatching)

- (void) pressesBegan:(NSSet*)presses withEvent:(UIPressesEvent *)event {
  UIPress* p = [presses anyObject];
  Keyboard::processKeyEvent((int)p.key.keyCode, true);
}

- (void) pressesEnded:(NSSet*)presses withEvent:(UIPressesEvent *)event {
  UIPress* p = [presses anyObject];
  Keyboard::processKeyEvent((int)p.key.keyCode, false);
}

@end
