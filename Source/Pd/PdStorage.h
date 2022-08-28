/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

extern "C" {
#include "x_libpd_mod_utils.h"
}

class Canvas;


namespace pd {

class Instance;

class Storage {

public:

    static void setContent(t_glist* patch, String content);
    static ValueTree getContent(t_glist* patch);

    static void undoIfNeeded(Canvas* cnv);
    static void redoIfNeeded(Canvas* cnv);

    static void createUndoAction(Canvas* cnv);

    static String getInfo(Canvas* cnv, String const& id, String const& property);
    static void setInfo(Canvas* cnv, String const& id, String const& property, String const& info, bool undoable = true);
    
    static std::pair<ValueTree, UndoManager*> getInfoForPatch(void* patch);

private:
    static inline std::unordered_map<void*, std::pair<ValueTree, std::unique_ptr<UndoManager>>> canvasInfo = std::unordered_map<void*, std::pair<ValueTree, std::unique_ptr<UndoManager>>>();
};
} // namespace pd
