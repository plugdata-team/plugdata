#pragma once
#include "Keyboard.h"

#include <X11/Xlib.h>

#include <atomic>
#include <thread>

namespace juce { class Component; }

class LinuxKeyboard : public Keyboard {
public:
  explicit LinuxKeyboard(juce::Component* parent);
  ~LinuxKeyboard();

  void timerCallback() override;

private:
    char prev_keymap[32] = {};
    std::thread* eventLoop;
    Display* display = nullptr;
    std::atomic<bool> running { false };
};
