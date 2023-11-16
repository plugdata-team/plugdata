#include "WindowsKeyboard.h"

#include <functional>

union KeyInfo
{
	// LPARAM
	LPARAM lParam;

	struct {
		unsigned nRepeatCount : 16;
		unsigned nScanCode : 8;
		unsigned nExtended : 1;
		unsigned nReserved : 4;
		unsigned nContext : 1;
		unsigned nPrev : 1;
		unsigned nTrans : 1;
	};
};

LRESULT CALLBACK WindowsKeyboard::keyHandler(int keyCode, WPARAM w, LPARAM l) {
	KeyInfo i;
	i.lParam = l;
	
  if (!Keyboard::processKeyEvent(w, i.nTrans == 0))
  		return CallNextHookEx(nullptr, keyCode, w, l);
	
  return 0;
}

WindowsKeyboard::WindowsKeyboard(juce::Component* parent) : Keyboard(parent)
{
	hook = SetWindowsHookA(WH_KEYBOARD, (HOOKPROC) keyHandler);
}

WindowsKeyboard::~WindowsKeyboard()
{
  UnhookWindowsHookEx(hook);
}
