#pragma once
#include "Keyboard.h"

namespace juce { class Component; }

class MacOsKeyboard : public Keyboard {
public:
  explicit MacOsKeyboard(juce::Component* parent);
  ~MacOsKeyboard();

private:
  void installMonitor();
  void removeMonitor();
  
  void* keyDownMonitor = nullptr;
  void* keyUpMonitor = nullptr;
  void* modifierChangedMonitor = nullptr;
};
