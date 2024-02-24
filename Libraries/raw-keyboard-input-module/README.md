# juce-raw-keyboard-input-module
Raw keyboard input module for JUCE on Linux, Windows, macOS and iOS

## Usage

```
#include <raw_keyboard_input/src/Keyboard.h>

MyComponent::MyComponent() // extends juce::Component
{
  // We need access to the ComponentPeer in order to distinguish
  // between multiple instances in a DAW, so we pass the holder of
  // the Keyboard instance to the factory method. The holder can be
  // any component in your component tree.
  
  keyboard = KeyboardFactory::instance(this);
  
  // Install callbacks
  
  keyboard->onKeyDownFn = [&](int keyCode){ keyEvent(juce::KeyEvent(keyCode, true)); };
  keyboard->onKeyUpFn = [&](int keyCode){ keyEvent(juce::KeyEvent(keyCode, false)); };

  // or call this in a juce::Timer instead
  
  keyboard->isKeyDown(keyCode);
}
```

On iOS you have to make sure that any components in your tree that
return `true` for `getWantsKeyboardFocus()` and that implement `keyPressed`, should
return `false` in that implementation for iOS for any key presses that you want
your JUCE project to handle via raw keyboard input.
