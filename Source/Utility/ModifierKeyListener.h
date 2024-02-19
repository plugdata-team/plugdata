#pragma once

#include "Config.h"

/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// A class that does more reliable modifier listening than JUCE has by default
// Create one broadcaster class and attach listeners to that

struct ModifierKeyListener {
    virtual void shiftKeyChanged(bool isHeld) { ignoreUnused(isHeld); }
    virtual void commandKeyChanged(bool isHeld) { ignoreUnused(isHeld); }
    virtual void altKeyChanged(bool isHeld) { ignoreUnused(isHeld); }
    virtual void ctrlKeyChanged(bool isHeld) { ignoreUnused(isHeld); }

    virtual void spaceKeyChanged(bool isHeld) { ignoreUnused(isHeld); }
    virtual void middleMouseChanged(bool isHeld) { ignoreUnused(isHeld); }

    JUCE_DECLARE_WEAK_REFERENCEABLE(ModifierKeyListener)
};

class ModifierKeyBroadcaster : private Timer {
public:
    ModifierKeyBroadcaster()
    {
        startTimer(50);
    }

    void addModifierKeyListener(ModifierKeyListener* listener)
    {
        listeners.add(listener);
    }

    void removeModifierKeyListener(ModifierKeyListener* listener)
    {
        listeners.removeAllInstancesOf(listener);
    }

    void setModifierKeys(ModifierKeys const& mods)
    {
        // Handle mod down
        if (mods.isShiftDown() && !shiftWasDown) {
            callListeners(Shift, true);
            shiftWasDown = true;
        }
        if (mods.isCommandDown() && !commandWasDown) {
            callListeners(Command, true);
            commandWasDown = true;
        }
        if (mods.isAltDown() && !altWasDown) {
            callListeners(Alt, true);
            altWasDown = true;
        }
        if (mods.isCtrlDown() && !ctrlWasDown) {
            callListeners(Ctrl, true);
            ctrlWasDown = true;
        }
        if (mods.isMiddleButtonDown() && !middleMouseWasDown) {
            callListeners(MiddleMouse, true);
            middleMouseWasDown = true;
        }
        if (KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey) && !spaceWasDown) {
            callListeners(Space, true);
            spaceWasDown = true;
        }

        // Handle mod up
        if (!mods.isShiftDown() && shiftWasDown) {
            callListeners(Shift, false);
            shiftWasDown = false;
        }
        if (!mods.isCommandDown() && commandWasDown) {
            callListeners(Command, false);
            commandWasDown = false;
        }
        if (!mods.isAltDown() && altWasDown) {
            callListeners(Alt, false);
            altWasDown = false;
        }
        if (!mods.isCtrlDown() && ctrlWasDown) {
            callListeners(Ctrl, false);
            ctrlWasDown = false;
        }
        if (!mods.isMiddleButtonDown() && middleMouseWasDown) {
            callListeners(MiddleMouse, false);
            middleMouseWasDown = false;
        }
        if (!KeyPress::isKeyCurrentlyDown(KeyPress::spaceKey) && spaceWasDown) {
            callListeners(Space, false);
            spaceWasDown = false;
        }
    }

private:
    enum Modifier {
        Shift,
        Command,
        Alt,
        Ctrl,
        Space,
        MiddleMouse
    };

    void callListeners(Modifier mod, bool down)
    {
        for (auto const& listener : listeners) {
            if (!listener)
                continue;
            switch (mod) {
            case Shift: {
                listener->shiftKeyChanged(down);
                break;
            }
            case Command: {
                listener->commandKeyChanged(down);
                break;
            }
            case Alt: {
                listener->altKeyChanged(down);
                break;
            }
            case Ctrl: {
                listener->ctrlKeyChanged(down);
                break;
            }
            case Space: {
                listener->spaceKeyChanged(down);
                break;
            }
            case MiddleMouse: {
                listener->middleMouseChanged(down);
                break;
            }
            }
        }
    }

    void timerCallback() override
    {
        // If a window that's not coming from our app is top-level, ignore
        // key commands
        if (ProjectInfo::isStandalone && !isActiveWindow()) {
            return;
        }

        auto mods = ModifierKeys::getCurrentModifiersRealtime();
        setModifierKeys(mods);
    }

    virtual bool isActiveWindow() { return true; }

    bool shiftWasDown = false;
    bool commandWasDown = false;
    bool altWasDown = false;
    bool ctrlWasDown = false;
    bool spaceWasDown = false;
    bool middleMouseWasDown = false;

    Array<WeakReference<ModifierKeyListener>> listeners;
};
