/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/OSUtils.h"

#include <m_pd.h>
#include <x_libpd_mod_utils.h>

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
        ofeliaProcess.kill();
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
            
            int uniquePortNumber = Random().nextInt({20000, 50000});
            
            ofeliaProcess.start(ofeliaExecutable.getFullPathName() + " " + String(uniquePortNumber));
            
            // Initialise threading system for ofelia
            ofxOfeliaMessageManager::initialise(uniquePortNumber);
            
#if JUCE_DEBUG
            // When debugging ofelia, it will falsly report that the process has finished
            // Instead we wait forever
            std::promise<void>().get_future().wait();
#else
            ofeliaProcess.waitForProcessToFinish(-1);
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


    std::atomic<bool> shouldQuit = false;
    ChildProcess ofeliaProcess;
};

}
