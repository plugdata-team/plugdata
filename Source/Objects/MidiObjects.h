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
        if (channel == 1) {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false));
        } else {
            if (midiInput)
                channel -= 16;
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
        }
    }

    void setChannelAndCC(int channel, int cc)
    {
        if (channel == 1) {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(cc));
        } else {
            if (midiInput)
                channel -= 16;
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && e.getNumberOfClicks() >= 2) {

            PopupMenu popupMenu;

            auto text = StringArray::fromTokens(getText(), false);
            auto currentPort = text.size() > 1 ? text[1].getIntValue() : 0;
            auto currentCC = text.size() > 2 ? text[2].getIntValue() : 0;

            popupMenu.addItem(1, "All devices by channel", true, currentPort == 0);

            if (ProjectInfo::isStandalone) {
                auto* midiDeviceManager = ProjectInfo::getMidiDeviceManager();

                if (midiInput) {
                    int port = 1;
                    for (auto const& input : midiDeviceManager->getInputDevices()) {
                        PopupMenu subMenu;
                        for (int ch = 1; ch < 17; ch++) {
                            int portNumber = ch + (port << 4);

                            if (isCtl) {
                                subMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(portNumber, portNumber == currentPort, currentCC), true);
                                // Call function to append CC submenu!
                            } else {
                                subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber == currentPort);
                            }
                        }

                        popupMenu.addSubMenu(input.name, subMenu, midiDeviceManager->isMidiDeviceEnabled(midiInput, input.identifier));
                        port++;
                    }
                } else {
                    int port = 1;
                    for (auto const& output : midiDeviceManager->getOutputDevices()) {
                        PopupMenu subMenu;
                        for (int ch = 1; ch < 17; ch++) {
                            int portNumber = ch + (port << 4);
                            if (isCtl) {
                                subMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(portNumber, portNumber == currentPort, currentCC), true);
                            } else {
                                subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber == currentPort);
                            }
                        }

                        popupMenu.addSubMenu(output.name, subMenu, midiDeviceManager->isMidiDeviceEnabled(midiInput, output.identifier));
                        port++;
                    }

                    // Add MIDI output option for internal synth
                    // This will automatically get chosen if the midi output port number is out of range
                    PopupMenu subMenu;
                    for (int ch = 1; ch < 17; ch++) {
                        int portNumber = ch + (port << 4);
                        if (isCtl) {
                            subMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(portNumber, portNumber == currentPort, currentCC), true);
                            // Call function to append CC submenu!
                        } else {
                            subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber == currentPort);
                        }
                    }

                    auto internalSynthEnabled = SettingsFile::getInstance()->getProperty<bool>("internal_synth");
                    popupMenu.addSubMenu("Internal GM Synth", subMenu, internalSynthEnabled);
                }
            } else {
                for (int ch = 1; ch < 17; ch++) {
                    if (isCtl) {
                        popupMenu.addSubMenu("Channel " + String(ch), getCCSubmenu(ch, currentPort == ch, currentCC), true);
                        // Call function to append CC submenu!
                    } else {
                        popupMenu.addItem(ch, "Channel " + String(ch), true, currentPort == ch);
                    }
                }
            }

            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(80).withMaximumNumColumns(1).withTargetComponent(this), ModalCallbackFunction::create([this](int itemID) {
                if (itemID == 0)
                    return;

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
