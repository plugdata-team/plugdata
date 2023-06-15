/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/OSUtils.h"

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
#include <x_libpd_mod_utils.h>

void ofelia_setup();

}

#include "../Libraries/plugdata-ofelia/Source/Objects/ofxOfeliaMessageManager.h"



namespace pd {

class Ofelia : public Thread
{
public:
    static inline File ofeliaExecutable = File();
    
    Ofelia() : Thread("Ofelia Thread")
    {
        startThread();
    }
    
    ~Ofelia()
    {
        shouldQuit = true;
        ofeliaProcess.kill();
        waitForThreadToExit(-1);
    }
    
private:
    
    void run() override
    {
        while(!shouldQuit) {
            
            auto ofeliaExecutable = findOfeliaExecutable();
            
            if(!ofeliaExecutable.existsAsFile())
            {
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 3000);
                continue;
            }
            
            ofeliaExecutable.setExecutePermission(true);
            
            if(!ofeliaInitialised) {
                sys_lock();
                pd_globallock();
                set_class_prefix(gensym("ofelia"));
                ofelia_setup();
                
                set_class_prefix(nullptr);
                pd_globalunlock();
                sys_unlock();
                
                ofeliaInitialised = true;
            }
            
            // Initialise threading system for ofelia
            auto* messageManager = ofxOfeliaMessageManager::initialise(canvas_class);
            
            int uniquePortNumber = Random().nextInt({20000, 50000});
            
            ofeliaProcess.start(ofeliaExecutable.getFullPathName() + " " + String(uniquePortNumber));
            
            bool success = messageManager->bind(uniquePortNumber);
#if JUCE_DEBUG
            
            auto err = ofeliaProcess.readAllProcessOutput();
            std::cerr << err << std::endl;
            
            // When debugging ofelia, it will falsly report that the process has finished
            // Instead we wait forever
            while(!shouldQuit)
            {
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 3000);
            }
#else
            ofeliaProcess.waitForProcessToFinish(-1);
            ofeliaProcess.kill(); // just to be sure
#endif
        }
    }
    
    
    File findOfeliaExecutable()
    {
        if(ofeliaExecutable.existsAsFile())
        {
            return ofeliaExecutable;
        }
        
        char* p[1024];
        int numItems;
        libpd_get_search_paths(p, &numItems);
        auto paths = StringArray(p, numItems);
        
        for (auto& dir : paths) {
            for (const auto& file : OSUtils::iterateDirectory(dir, true, true)) {
                if (file.getFileNameWithoutExtension() == "ofelia") {
                    return ofeliaExecutable = file;
                }
            }
        }
        
        return ofeliaExecutable = File();
    }

    static inline std::atomic<bool> ofeliaInitialised = false;
    std::atomic<bool> shouldQuit = false;
    ChildProcess ofeliaProcess;
};

}
