#pragma once
#include "Keyboard.h"

#include <Windows.h>

namespace juce { class Component; }

class WindowsKeyboard : public Keyboard {
public:
  explicit WindowsKeyboard(juce::Component* parent);
  ~WindowsKeyboard();
  
private:
	static LRESULT CALLBACK keyHandler(int keyCode, WPARAM w, LPARAM l);
  HHOOK hook;
  
};
