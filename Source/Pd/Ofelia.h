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

class Ofelia : public Timer
{
public:
    static inline File ofeliaExecutable = File();
    
    Ofelia()
    {
        startProcess();
        startTimer(3000);
    }
    
    ~Ofelia()
    {
        ofeliaProcess.kill();
    }
    
private:
    void startProcess()
    {
        if(ofeliaProcess.isRunning()) return;
        
        auto ofeliaExecutable = findOfeliaExecutable();
        if(ofeliaExecutable.existsAsFile()) {
            
            int uniquePortNumber = Random().nextInt({20000, 50000});
            
            ofeliaProcess.start(ofeliaExecutable.getFullPathName() + " " + String(uniquePortNumber));
            
            // Initialise threading system for ofelia
            ofxOfeliaMessageManager::initialise(uniquePortNumber);
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
    
    void timerCallback() override
    {
        findOfeliaExecutable();
        
        if(!ofeliaProcess.isRunning() && ofeliaExecutable.existsAsFile())
        {
            startProcess();
        }

    }

    ChildProcess ofeliaProcess;
};

}

extern bool ofxOfeliaExecutableFound();
