/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/ColourPicker.h"

class MidiObject final : public TextBase {
public:
    
    bool midiInput;
    MidiObject(void* ptr, Object* object, bool isInput)
    : TextBase(ptr, object), midiInput(isInput)
    {
    }
    
    bool isDeviceEnabled(const MidiDeviceInfo& info)
    {
        if(ProjectInfo::isStandalone) {
            if (midiInput) {
                return ProjectInfo::getDeviceManager()->isMidiInputDeviceEnabled(info.identifier);
            } else {
                for (auto* midiOut : pd->midiOutputs) {
                    if (midiOut->getIdentifier() == info.identifier) {
                        return true;
                    }
                }
                return false;
            }
        }
        
        return true;
    }
    void setChannel(int channel)
    {
        object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
    }
    
    
    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && e.mods.isPopupMenu()) {
            
            // TODO: add "all channels" option?
            
            PopupMenu popupMenu;
            
           
            auto text = StringArray::fromTokens(getText(), false);
            auto currentPort = text.size() > 1 ? text[1].getIntValue() : 0;
            
            if(ProjectInfo::isStandalone)
            {
                if(midiInput)
                {
                    int port = 0;
                    
                    for(auto input : MidiInput::getAvailableDevices()) {
                        PopupMenu subMenu;
                        for(int ch = 1; ch < 16; ch++)
                        {
                            int portNumber = ch + (port << 4);
                            subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber  == currentPort);
                        }
                        
                        popupMenu.addSubMenu(input.name, subMenu, isDeviceEnabled(input));
                        port++;
                    }
                }
                else {
                    int port = 0;
                    for(auto output : MidiOutput::getAvailableDevices()) {
                        PopupMenu subMenu;
                        for(int ch = 1; ch < 16; ch++)
                        {
                            int portNumber = ch + (port << 4);
                            subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber  == currentPort);
                        }
                        
                        popupMenu.addSubMenu(output.name, subMenu, isDeviceEnabled(output));
                        port++;
                    }
                }
            }
            else {
                for(int ch = 1; ch < 16; ch++)
                {
                    popupMenu.addItem(ch, "Channel " + String(ch), true, ch == currentPort);
                }
            }
            
            popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(80).withMaximumNumColumns(1).withParentComponent(cnv).withTargetComponent(this), ModalCallbackFunction::create([this](int channel){
                if(channel == 0) return;
                
                setChannel(channel);
            }));
            
        }
        
    }
};
