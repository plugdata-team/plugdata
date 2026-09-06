/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"
#include <juce_data_structures/juce_data_structures.h>
#include "Utility/Icons.h"

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,
    toolbarHoverColourId,
    toolbarOutlineColourId,
    activeTabBackgroundColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    presentationBackgroundColourId,

    guiObjectBackgroundColourId,
    guiObjectInternalOutlineColourId,
    textObjectBackgroundColourId,

    objectOutlineColourId,
    objectSelectedOutlineColourId,
    commentTextColourId,
    outlineColourId,

    ioletAreaColourId,
    ioletOutlineColourId,

    dataColourId,
    connectionColourId,
    signalColourId,
    gemColourId,

    dialogBackgroundColourId,

    sidebarBackgroundColourId,
    sidebarTextColourId,
    sidebarActiveBackgroundColourId,

    panelBackgroundColourId,
    panelForegroundColourId,
    panelTextColourId,
    panelActiveBackgroundColourId,

    popupMenuBackgroundColourId,
    popupMenuActiveBackgroundColourId,
    popupMenuTextColourId,

    scrollbarThumbColourId,
    graphAreaColourId,
    gridLineColourId,
    caretColourId,

    /* iteration hack */
    numberOfColours
};

enum CommandIDs {
    NewProject = 1,
    OpenProject,
    SaveProject,
    SaveProjectAs,
    CloseTab,
    Undo,
    Redo,

    Lock,
    ConnectionStyle,
    ConnectionPathfind,
    PanDragKey,
    ZoomIn,
    ZoomOut,
    ZoomNormal,
    ZoomToFitAll,
    GoToOrigin,
    Copy,
    Paste,
    Cut,
    Delete,
    Duplicate,
    Encapsulate,
    Triggerize,
    Tidy,
    CreateConnection,
    RemoveConnections,
    SelectAll,
    ShowBrowser,
    ToggleLeftSidebar,
    ToggleRightSidebar,
    Search,
    NextTab,
    PreviousTab,
    ToggleSnapping,
    ClearConsole,
    ShowSettings,
    ShowReference,
    ShowHelp,
    OpenObjectBrowser,
    ToggleDSP,
    TogglePresentationMode,
    TogglePluginMode,
    Compile,
    NumItems // <-- the total number of items in this enum
};

enum ObjectIDs {
    NewObject = 100,
    NewComment,
    NewBang,
    NewMessage,
    NewToggle,
    NewNumbox,
    NewVerticalSlider,
    NewHorizontalSlider,
    NewVerticalRadio,
    NewHorizontalRadio,
    NewFloatAtom,
    NewSymbolAtom,
    NewListAtom,
    NewArray,
    NewGraphOnParent,
    NewCanvas,
    NewVUMeter,
    NumObjects,
    OtherObject
};

UnorderedMap<ObjectIDs, String> const objectNames {
    { NewObject, "" },
    { NewComment, "comment" },
    { NewBang, "bng" },
    { NewMessage, "msg" },
    { NewToggle, "tgl" },
    { NewNumbox, "nbx" },
    { NewVerticalSlider, "vsl" },
    { NewHorizontalSlider, "hsl" },
    { NewVerticalRadio, "vradio" },
    { NewHorizontalRadio, "hradio" },
    { NewFloatAtom, "floatbox" },
    { NewSymbolAtom, "symbolbox" },
    { NewListAtom, "listbox" },
    { NewGraphOnParent, "graph" },
    { NewCanvas, "cnv" },
    { NewVUMeter, "vu" },
    { NewArray, "garray" },
};

struct Corners {
    static constexpr float windowCornerRadius = 12.0f;
    static constexpr float largeCornerRadius = 8.0f;
    static constexpr float defaultCornerRadius = 6.0f;
    static constexpr float resizeHanleCornerRadius = 2.75f;
};

enum Overlay {
    None = 0,
    Origin = 1 << 0,
    Border = 1 << 1,
    Index = 1 << 2,
    Coordinate = 1 << 3,
    ActivationState = 1 << 4,
    ConnectionActivity = 1 << 5,
    Order = 1 << 6,
    Direction = 1 << 7,
    Behind = 1 << 8
};

enum Align {
    Left = 0,
    Right,
    HCentre,
    HDistribute,
    Top,
    Bottom,
    VCentre,
    VDistribute
};

namespace PlatformStrings {
inline String getBrowserTip()
{
#if JUCE_MAC
    return "Reveal in Finder";
#elif JUCE_WINDOWS
    return "Reveal in Explorer";
#else
    return "Reveal in file browser";
#endif
}
}
