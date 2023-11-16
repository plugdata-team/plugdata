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
#include "Pd/Interface.h"

void ofelia_setup();
}

#include "../Libraries/plugdata-ofelia/Source/Objects/ofxOfeliaMessageManager.h"

namespace pd {

class Ofelia : public Thread {
public:
    static inline File ofeliaExecutable = File();

    Ofelia(t_pdinstance* pdthis)
        : Thread("Ofelia Thread")
        , pdinstance(pdthis)
    {
        setup();
        startThread();
    }

    ~Ofelia()
    {
        shouldQuit = true;
        ofeliaProcess.kill();
        stopThread(-1);
    }

private:
    void setup()
    {
        auto ofeliaExecutable = findOfeliaExecutable();
        if (!ofeliaInitialised && ofeliaExecutable.existsAsFile()) {
            ofeliaInitialised = true;
            sys_lock();
            pd_globallock();
            set_class_prefix(gensym("ofelia"));
            ofelia_setup();

            set_class_prefix(nullptr);
            pd_globalunlock();
            sys_unlock();
        }
    }

    void run() override
    {
        while (!shouldQuit) {
            auto ofeliaExecutable = findOfeliaExecutable();

            if (!ofeliaExecutable.existsAsFile()) {
                for (int i = 0; i < 10; i++) {
                    Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 500);
                    if (shouldQuit)
                        break;
                }
                continue;
            }

            ofeliaExecutable.setExecutePermission(true);

            libpd_set_instance(pdinstance);

            setup();

            // Initialise threading system for ofelia
            auto* messageManager = ofxOfeliaMessageManager::initialise();

            int uniquePortNumber = Random().nextInt({ 20000, 50000 });

            ofeliaProcess.start(ofeliaExecutable.getFullPathName() + " " + String(uniquePortNumber));

            bool success = messageManager->bind(uniquePortNumber);

            if (!success) {
                ofeliaProcess.kill();
                continue;
            }

#if JUCE_DEBUG

            auto err = ofeliaProcess.readAllProcessOutput();
            std::cerr << err << std::endl;

            // When debugging ofelia, it will falsly report that the process has finished
            // Instead we wait forever
            while (!shouldQuit) {
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
        if (ofeliaExecutable.existsAsFile()) {
            return ofeliaExecutable;
        }

        libpd_set_instance(pdinstance);

        char* p[1024];
        int numItems;
        pd::Interface::getSearchPaths(p, &numItems);
        auto paths = StringArray(p, numItems);

        for (auto& dir : paths) {
            auto ofeliaDir = File(dir).getChildFile("ofelia");

            if (!ofeliaDir.isDirectory())
                continue;

            for (auto const& file : OSUtils::iterateDirectory(ofeliaDir, false, true)) {
                if (file.getFileName() == "ofelia" || file.getFileName() == "ofelia.exe") {
                    return ofeliaExecutable = file;
                }
            }
        }

        return ofeliaExecutable = File();
    }

    t_pdinstance* pdinstance;
    std::atomic<bool> ofeliaInitialised = false;
    std::atomic<bool> shouldQuit = false;
    ChildProcess ofeliaProcess;
};

}
