/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/ColourPicker.h"

class PdTildeObject final : public TextBase {
public:
    
    bool pdLocationWasSet = false;
    std::unique_ptr<FileChooser> openChooser;
    
    PdTildeObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        if(!e.mods.isLeftButtonDown()) return;
        
        if (getValue<bool>(object->locked)) {
            openPd();
        }
    }

    void openPd()
    {
        
        auto openFunc = [this](FileChooser const& f) {
            auto pdDir = f.getResult();
            
#if JUCE_MAC
            if(pdDir.hasFileExtension("app"))
            {
                pdDir = pdDir.getChildFile("Contents").getChildFile("Resources");
            }
#endif
            
            if(pdDir.isDirectory())
            {
                if(auto pdTilde = ptr.get<t_fake_pd_tilde>())
                {
                    auto pdPath = pdDir.getFullPathName();
                    auto schedPath = pdDir.getChildFile("extra").getChildFile("pd~").getFullPathName();;
                    pdTilde->x_pddir = gensym(pdPath.toRawUTF8());
                    pdTilde->x_schedlibdir = gensym(schedPath.toRawUTF8());
                }
                
                if(auto pdTilde = ptr.get<t_fake_pd_tilde>())
                {
                    pd->sendDirectMessage(pdTilde.get(), "pd~", { "start" });
                }
            }
        };
            
        if(!pdLocationWasSet) {
            
            openChooser = std::make_unique<FileChooser>("Locate pd folder", File::getSpecialLocation(File::SpecialLocationType::globalApplicationsDirectory), "", SettingsFile::getInstance()->wantsNativeDialog());
            
            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectDirectories, openFunc);
        }
        else {
            if(auto pdTilde = ptr.get<t_fake_pd_tilde>())
            {
                pd->sendDirectMessage(pdTilde.get(), "pd~", { "start" });
            }
        }
    }
};
