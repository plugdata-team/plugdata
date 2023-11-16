#pragma once

namespace juce { class Component; }

class Keyboard;
class KeyboardFactory {
public:
  static Keyboard* instance(juce::Component* parent);
};
