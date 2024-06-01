#include "Keyboard.h"

std::set<juce::WeakReference<Keyboard>> Keyboard::thisses;

Keyboard::Keyboard(juce::Component* initialParent) : parent(initialParent)
{
  thisses.emplace(juce::WeakReference(this));
  startTimer(1);
}

Keyboard::~Keyboard()
{
  thisses.erase(this);
}

bool Keyboard::processKeyEvent(int keyCode, bool isKeyDown)
{
  auto focusedPeer = getFocusedPeer();

  if (focusedPeer == nullptr)
    return false;

  for (auto t : thisses) {
      if(t) {
        if (t->peer == focusedPeer || (t->auxPeer != nullptr && t->auxPeer == focusedPeer)) {
        if (isKeyDown)
            t->addPressedKey(keyCode);
        else
            t->removePressedKey(keyCode);
        }
      }
  }

  return true;
}

juce::ComponentPeer* Keyboard::getFocusedPeer()
{
  for (int i = 0; i < juce::ComponentPeer::getNumPeers(); i++) {
    if (juce::ComponentPeer::getPeer(i)->isFocused()) {
      return juce::ComponentPeer::getPeer(i);
    }
  }
  return nullptr;
}

void Keyboard::timerCallback()
{
    if (peer == nullptr)
    {
        auto _peer = parent->getPeer();
        if (_peer != nullptr) {
            peer = _peer;
        }
    }

    if (auxParent != nullptr && auxPeer == nullptr)
    {
        auto _auxPeer = auxParent->getPeer();
        if (_auxPeer != nullptr) {
            auxPeer = _auxPeer;
        }
    }

    if (peer != nullptr && (auxParent == nullptr || auxPeer != nullptr))
    {
        stopTimer();
    }
}

bool Keyboard::isKeyDown(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);
  return pressedKeys.count(keyCode) == 1;
}

void Keyboard::addPressedKey(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);
  pressedKeys.emplace(keyCode);
  if (onKeyDownFn) onKeyDownFn(keyCode);
}

void Keyboard::removePressedKey(int keyCode)
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);

  if (pressedKeys.count(keyCode) == 1)
    pressedKeys.erase(keyCode);

  if (onKeyUpFn) onKeyUpFn(keyCode);
}

void Keyboard::allKeysUp()
{
  std::lock_guard<std::recursive_mutex> lock(pressedKeysMutex);

  for (auto keyCode : pressedKeys)
    onKeyUpFn(keyCode);

  pressedKeys.clear();
}

void Keyboard::setAuxParent(juce::Component* newAuxParent)
{
    auxParent = newAuxParent;

    if (auxParent == nullptr)
    {
        auxPeer = nullptr;
    }
    else
    {
        startTimer(1);
    }
}
