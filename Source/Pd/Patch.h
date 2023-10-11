/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

extern "C" {
#include "Pd/Interface.h" //  TODO: we only need t_object
}

#include "WeakReference.h"

namespace pd {

using Connections = std::vector<std::tuple<t_outconnect*, int, t_object*, int, t_object*>>;
class Instance;

// The Pd patch.
// Wrapper around a Pd patch. The lifetime of the internal patch
// is not guaranteed by the class.
// Has reference counting, because both the Canvas and PluginProcessor hold a reference to it
// That makes it tricky to clean up from the setStateInformation function, which may be called from any thread
class Patch : public ReferenceCountedObject {
public:
    using Ptr = ReferenceCountedObjectPtr<Patch>;

    Patch(t_canvas* ptr, Instance* instance, bool ownsPatch, File currentFile = File());

    ~Patch();

    // The compare equal operator.
    bool operator==(Patch const& other) const
    {
        return getPointer().get() == other.getPointer().get();
    }

    // Gets the bounds of the patch.
    Rectangle<int> getBounds() const;

    t_gobj* createObject(int x, int y, String const& name);
    void removeObject(t_gobj* obj);
    t_gobj* renameObject(t_object* obj, String const& name);

    void moveObjects(std::vector<t_gobj*> const&, int x, int y);

    void moveObjectTo(t_gobj* object, int x, int y);

    void finishRemove();
    void removeObjects(std::vector<t_gobj*> const& objects);

    void deselectAll();

    bool isSubpatch();
    bool isAbstraction();

    void setVisible(bool shouldVis);

    static String translatePatchAsString(String const& clipboardContent, Point<int> position);

    t_glist* getRoot();

    void copy(std::vector<t_gobj*> const& objects);
    void paste(Point<int> position);
    void duplicate(std::vector<t_gobj*> const& objects);

    void startUndoSequence(String const& name);
    void endUndoSequence(String const& name);

    void undo();
    void redo();

    enum GroupUndoType {
        Remove = 0,
        Move
    };

    void setCurrent();

    bool isDirty() const;

    void savePatch(File const& location);
    void savePatch();

    File getCurrentFile() const;
    File getPatchFile() const;

    void setCurrentFile(File newFile);

    bool objectWasDeleted(t_gobj* ptr) const;
    bool connectionWasDeleted(t_outconnect* ptr) const;

    bool hasConnection(t_object* src, int nout, t_object* sink, int nin);
    bool canConnect(t_object* src, int nout, t_object* sink, int nin);
    void createConnection(t_object* src, int nout, t_object* sink, int nin);
    t_outconnect* createAndReturnConnection(t_object* src, int nout, t_object* sink, int nin);
    void removeConnection(t_object* src, int nout, t_object* sink, int nin, t_symbol* connectionPath);
    t_outconnect* setConnctionPath(t_object* src, int nout, t_object* sink, int nin, t_symbol* oldConnectionPath, t_symbol* newConnectionPath);

    Connections getConnections() const;

    WeakReference::Ptr<t_canvas> getPointer() const
    {
        return ptr.get<t_canvas>();
    }

    // Gets the objects of the patch.
    std::vector<t_gobj*> getObjects();

    String getCanvasContent();

    static void reloadPatch(File const& changedPatch, t_glist* except);

    String getTitle() const;
    void setTitle(String const& title);

    Instance* instance = nullptr;
    bool closePatchOnDelete;
    bool openInPluginMode = false;
    int splitViewIndex = 0;

    int untitledPatchNum = 0;

private:
    File currentFile;

    WeakReference ptr;

    // Initialisation parameters for GUI objects
    // Taken from pd save files, this will make sure that it directly initialises objects with the right parameters
    static inline const std::map<String, String> guiDefaults = {
        { "tgl", "25 0 empty empty empty 17 7 0 10 bgColour fgColour lblColour 0 1" },
        { "hsl", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 bgColour fgColour lblColour 0 1" },
        { "hslider", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 bgColour fgColour lblColour 0 1" },
        { "vsl", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 bgColour fgColour lblColour 0 1" },
        { "vslider", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 bgColour fgColour lblColour 0 1" },
        { "bng", "25 250 50 0 empty empty empty 17 7 0 10 bgColour fgColour lblColour" },
        { "nbx", "4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 bgColour lblColour lblColour 0 256" },
        { "hradio", "20 1 0 8 empty empty empty 0 -8 0 10 bgColour fgColour lblColour 0" },
        { "vradio", "20 1 0 8 empty empty empty 0 -8 0 10 bgColour fgColour lblColour 0" },
        { "cnv", "15 100 60 empty empty empty 20 12 0 14 lnColour lblColour" },
        { "vu", "20 120 empty empty -1 -8 0 10 bgColour lblColour 1 0" },
        { "floatatom", "5 0 0 0 empty - - 12" },
        { "symbolatom", "5 0 0 0 empty - - 12" },
        { "listbox", "9 0 0 0 empty - - 0" },
        { "numbox~", "4 15 100 bgColour fgColour 10 0 0 0" },
        { "button", "25 25 bgColour_rgb fgColour_rgb" },
        { "oscope~", "130 130 256 3 128 -1 1 0 0 0 0 fgColour_rgb bgColour_rgb lnColour_rgb 0 empty" },
        { "scope~", "130 130 256 3 128 -1 1 0 0 0 0 fgColour_rgb bgColour_rgb lnColour_rgb 0 empty" },
        { "function", "200 100 empty empty 0 1 bgColour_rgb lblColour_rgb 0 0 0 0 0 1000 0" },
        { "messbox", "180 60 bgColour_rgb lblColour_rgb 0 12" },
        { "note", "0 14 Inter empty 0 lblColour_rgb 0 bgColour_rgb 0 0 note" },
        { "knob", "50 0 127 0 0 empty empty bgColour lnColour fgColour 1 0 0 0 1 270 0 0" }
    };

    friend class Instance;
    friend class Gui;
    friend class Object;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Patch)
};
} // namespace pd
