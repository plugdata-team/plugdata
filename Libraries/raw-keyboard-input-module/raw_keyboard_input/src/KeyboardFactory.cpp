#include "KeyboardFactory.h"
#include "Keyboard.h"
#if defined (__APPLE__)
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#include "MacOsKeyboard.h"
#endif
#elif defined (_WIN32)
#include "WindowsKeyboard.h"
#elif defined (__linux__)
#include "LinuxKeyboard.h"
#endif

Keyboard* KeyboardFactory::instance(juce::Component* parent)
{
#if defined (__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
  return new Keyboard(parent);
#else
  return new MacOsKeyboard(parent);
#endif
#elif defined (_WIN32)
  return new WindowsKeyboard(parent);
#elif defined (__linux__)
  return new LinuxKeyboard(parent);
#else
  return new Keyboard(parent);
#endif
}
