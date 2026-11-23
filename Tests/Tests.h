#include <juce_gui_basics/juce_gui_basics.h>
#include <random>

#include "Utility/Config.h"
#include "Utility/OSUtils.h"
#include "Sidebar/Sidebar.h" // So we can read and clear the console
#include "Objects/ObjectBase.h" // So we can interact with object GUIs
#include "PluginEditor.h"

namespace TestHelpers {

// Delivers a synthetic mouseDown/mouseUp pair to a component
inline void simulateClick(Component* c)
{
    bool allowsMouseClicks, allowsMouseClicksOnChildren;
    c->getInterceptsMouseClicks(allowsMouseClicks, allowsMouseClicksOnChildren);
    if (!allowsMouseClicks)
        return;

    auto const centre = c->getLocalBounds().getCentre().toFloat();
    MouseEvent e(Desktop::getInstance().getMainMouseSource(),
                 centre, ModifierKeys::leftButtonModifier,
                 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                 Time::getCurrentTime(), centre, Time::getCurrentTime(), 1, false);
    c->mouseDown(e);
    c->mouseUp(e);
}

inline void collectComponents(Component* c, HeapArray<Component::SafePointer<Component>>& targets)
{
    if (!c)
        return;
    for (auto* child : c->getChildren()) {
        if (!child->isShowing())
            continue;
        targets.add(child);
        collectComponents(child, targets);
    }
}

// Clicks every visible, enabled component below root (collected up-front, so
// components spawned by the clicks themselves are not clicked)
inline void clickThrough(Component* root)
{
    HeapArray<Component::SafePointer<Component>> targets;
    collectComponents(root, targets);
    for (auto& target : targets) {
        if (auto* c = target.getComponent()) {
            if (c->isShowing() && c->isEnabled())
                simulateClick(c);
        }
    }
}

inline Button* findButtonWithText(Component* root, String const& text)
{
    if (!root)
        return nullptr;
    for (auto* child : root->getChildren()) {
        if (auto* button = dynamic_cast<Button*>(child)) {
            if (button->getButtonText() == text)
                return button;
        }
        if (auto* found = findButtonWithText(child, text))
            return found;
    }
    return nullptr;
}

inline Label* findLabelContaining(Component* root, String const& text)
{
    if (!root)
        return nullptr;
    for (auto* child : root->getChildren()) {
        if (auto* label = dynamic_cast<Label*>(child)) {
            if (label->getText().contains(text))
                return label;
        }
        if (auto* found = findLabelContaining(child, text))
            return found;
    }
    return nullptr;
}

template<typename ComponentType>
inline ComponentType* findChildOfType(Component* root)
{
    if (!root)
        return nullptr;
    for (auto* child : root->getChildren()) {
        if (auto* typed = dynamic_cast<ComponentType*>(child))
            return typed;
        if (auto* found = findChildOfType<ComponentType>(child))
            return found;
    }
    return nullptr;
}

} // namespace TestHelpers

class PlugDataUnitTest : public UnitTest, public AsyncUpdater
{
protected:
    PlugDataUnitTest(PluginEditor* editor, String const& name) : UnitTest(name), editor(editor)
    {
    }

    virtual void perform() = 0;

    void signalDone(bool result)
    {
        testResult = result;
        testDone.signal();
    }

private:

    void runTest() final
    {
        rng = getRandom();
        triggerAsyncUpdate();
        testDone.wait();
        expect(testResult, "Test failed");
    }

    void handleAsyncUpdate() final
    {
        perform();
    }

    WaitableEvent testDone;

protected:
    Random rng;
    bool testResult;
    PluginEditor* editor;
};
