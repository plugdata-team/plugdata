/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

#include "PdInstance.h"

extern "C" {
#include "x_libpd_mod_utils.h"
}

namespace pd {

class Storage {
    t_glist* parentPatch = nullptr;
    Instance* instance = nullptr;

    t_gobj* infoObject = nullptr;
    t_glist* infoParent = nullptr;

public:
    Storage(t_glist* patch, Instance* inst);

    Storage() = delete;

    void setInfoId(String const& oldId, String const& newId);
    void confirmIds();

    bool hasInfo(String const& id) const;
    void storeInfo();
    void loadInfoFromPatch();

    void undoIfNeeded();
    void redoIfNeeded();

    void createUndoAction();

    static bool isInfoParent(t_gobj* obj);
    static bool isInfoParent(t_glist* glist);

    String getInfo(String const& id, String const& property) const;
    void setInfo(String const& id, String const& property, String const& info, bool undoable = true);

private:
    void createObject();

    UndoManager undoManager;

    ValueTree extraInfo = ValueTree("PlugDataInfo");

    friend class Instance;
    friend class Patch;
};
} // namespace pd
