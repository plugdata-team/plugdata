#include "MacOsKeyboard.h"
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

MacOsKeyboard::MacOsKeyboard(juce::Component* parent) : Keyboard(parent)
{
  installMonitor();
}

MacOsKeyboard::~MacOsKeyboard()
{
  removeMonitor();
}

void MacOsKeyboard::installMonitor()
{  
  keyDownMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                         handler:^NSEvent*(NSEvent* event) {
    Keyboard::processKeyEvent([event keyCode], true);
    return event;
  }];
  
  keyUpMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp
                                                       handler:^NSEvent*(NSEvent* event) {
    Keyboard::processKeyEvent([event keyCode], false);
    return event;
  }];
  
  modifierChangedMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskFlagsChanged
                                                                 handler:^NSEvent*(NSEvent* event) {
    auto code = [event keyCode];
    
    if (code == 0x3A || code == 0x3D) {
      if ([event modifierFlags] & NSEventModifierFlagOption) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    else if (code == 0x38 || code == 0x3C) {
      if ([event modifierFlags] & NSEventModifierFlagShift) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    else if (code == 0x3B || code == 0x3E) {
      if ([event modifierFlags] & NSEventModifierFlagControl) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    else if (code == 0x36 || code == 0x37) {
      if ([event modifierFlags] & NSEventModifierFlagCommand) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    else if (code == 0x3F) {
      if ([event modifierFlags] & NSEventModifierFlagFunction) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    else if (code == 0x39) {
      if ([event modifierFlags] & NSEventModifierFlagCapsLock) {
        Keyboard::processKeyEvent(code, true);
      } else {
        Keyboard::processKeyEvent(code, false);
      }
    }
    
    return event;
  }];
  
}

void MacOsKeyboard::removeMonitor()
{
  if (keyDownMonitor != nullptr) {
    [NSEvent removeMonitor:(id)keyDownMonitor];
    keyDownMonitor = nullptr;
  }
  if (keyUpMonitor != nullptr) {
    [NSEvent removeMonitor:(id)keyUpMonitor];
    keyUpMonitor =  nullptr;
  }
  if (modifierChangedMonitor != nullptr) {
    [NSEvent removeMonitor:(id)modifierChangedMonitor];
    modifierChangedMonitor = nullptr;
  }
}
