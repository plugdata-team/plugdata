/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/MidiDeviceManager.h"

class MidiObject final : public TextBase {
public:
    
    const int allChannelsAndDevices = 99999;
    bool midiInput;
    
    MidiObject(void* ptr, Object* object, bool isInput)
    : TextBase(ptr, object), midiInput(isInput)
    {
    }
    
    void setChannel(int channel)
    {
        if(channel == allChannelsAndDevices)
        {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false));
        }
        else {
            object->setType(getText().upToFirstOccurrenceOf(" ", false, false) + " " + String(channel));
        }
    }
    
    
    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && (e.mods.isPopupMenu() || e.getNumberOfClicks() >= 2)) {
            
            PopupMenu popupMenu;
            
            auto text = StringArray::fromTokens(getText(), false);
            auto currentPort = text.size() > 1 ? text[1].getIntValue() : 0;
            
            if(ProjectInfo::isStandalone)
            {
                auto* midiDeviceManager =  ProjectInfo::getMidiDeviceManager();
                
                if(midiInput)
                {
                    int port = 0;
                    for(auto input : midiDeviceManager->getInputDevices()) {
                        PopupMenu subMenu;
                        for(int ch = 1; ch < 16; ch++)
                        {
                            int portNumber = ch + (port << 4);
                            subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber  == currentPort);
                        }
                        
                        popupMenu.addSubMenu(input.name, subMenu, midiDeviceManager->isMidiDeviceEnabled(midiInput, input.identifier));
                        port++;
                    }
                }
                else {
                    //popupMenu.addItem(allChannelsAndDevices, "Receive from all devices");
                    
                    int port = 0;
                    for(auto output : midiDeviceManager->getOutputDevices()) {
                        PopupMenu subMenu;
                        for(int ch = 1; ch < 16; ch++)
                        {
                            int portNumber = ch + (port << 4);
                            subMenu.addItem(portNumber, "Channel " + String(ch), true, portNumber  == currentPort);
                        }
  
                        popupMenu.addSubMenu(output.name, subMenu, midiDeviceManager->isMidiDeviceEnabled(midiInput, output.identifier));
                        port++;
                    }
                }
            }
            else {
                //popupMenu.addItem(allChannelsAndDevices, "Receive from all channels");
                
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
