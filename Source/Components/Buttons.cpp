/*
 // Copyright (c) 2023 Alexander Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../PluginEditor.h"
#include "Buttons.h"

String MainToolbarButton::getTooltip()
{
    auto setTooltip = TextButton::getTooltip();
    if (auto* editor = dynamic_cast<PluginEditor*>(getParentComponent())) {
        if (auto const* cnv = editor->getCurrentCanvas()) {
            if (isUndo) {
                setTooltip = "Undo";
                if (cnv->patch.canUndo() && cnv->patch.lastUndoSequence != "")
                    setTooltip += ": " + cnv->patch.lastUndoSequence.toString();
            } else if (isRedo) {
                setTooltip = "Redo";
                if (cnv->patch.canRedo() && cnv->patch.lastRedoSequence != "")
                    setTooltip += ": " + cnv->patch.lastRedoSequence.toString();
            }
        }
    }
    return setTooltip;
}
