/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/MidiDeviceManager.h"

class MidiObject final : public TextBase {
public:
    bool midiInput;
    bool isCtl;

    MidiObject(pd::WeakReference ptr, Object* object, bool isInput, bool isCC)
        : TextBase(ptr, object)
        , midiInput(isInput)
        , isCtl(isCC)
    {
    }

    void setChannel(int channel)
    {
        if (channel == 0) {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false));
        } else {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
        }
    }

    void setChannelAndCC(int channel, int cc)
    {
        if (channel == 0) {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(cc));
        } else {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
        }
    }

    PopupMenu getPopupMenu()
    {
        PopupMenu popupMenu;

        auto text = StringArray::fromTokens(getText(), false);
        auto currentPort = text.size() > 1 ? text[1].getIntValue() : 0;
        auto currentCC = text.size() > 2 ? text[2].getIntValue() : 0;

        if (midiInput) {
            popupMenu.addItem(1, "All input devices", true, currentPort == 0);
        }

        auto& midiDeviceManager = pd->getMidiDeviceManager();
        if (midiInput) {
            for (int port = 0; port < 8; port++) {
                PopupMenu subMenu;
                for (int ch = 1; ch < 17; ch++) {
                    int portNumber = (ch + (port << 4)) + 1;

                    if (isCtl) {
                        subMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(portNumber, portNumber == currentPort, currentCC), true);
                    } else {
                        subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber == currentPort);
                    }
                }
                popupMenu.addSubMenu(midiDeviceManager.getPortDescription(true, port), subMenu, true);
            }
        } else {
            for (int port = 0; port < 8; port++) {
                PopupMenu subMenu;
                for (int ch = 1; ch < 17; ch++) {
                    int portNumber = (ch + (port << 4)) + 1;
                    if (isCtl) {
                        subMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(portNumber, portNumber == currentPort, currentCC), true);
                    } else {
                        subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber == currentPort);
                    }
                }
                popupMenu.addSubMenu(midiDeviceManager.getPortDescription(false, port), subMenu, true);
            }
        }

        return popupMenu;
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem(-1, "Open", false);
        menu.addSubMenu("MIDI device", getPopupMenu());
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && e.getNumberOfClicks() >= 2) {
            auto popupMenu = getPopupMenu();
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(80).withMaximumNumColumns(1).withTargetComponent(this), ModalCallbackFunction::create([this](int itemID) {
                if (itemID == 0)
                    return;

                itemID -= 1;

                if (isCtl) {
                    auto channelDevice = itemID & 0x1FF;
                    auto ccValue = (itemID >> 9) & 0x7F;
                    setChannelAndCC(channelDevice, ccValue);
                } else {
                    setChannel(itemID);
                }
            }));
        }
    }

    PopupMenu getCCSubmenu(int channelAndDevice, bool channelSelected, int currentCC)
    {
        PopupMenu submenu;
        for (int cc = 0; cc < 127; cc++) {
            submenu.addItem((channelAndDevice & 0x1FF) + ((cc & 0x7F) << 9), "CC " + String(cc), true, channelSelected && currentCC == cc);
        }

        return submenu;
    }
};
