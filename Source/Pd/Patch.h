/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

extern "C" {
#include "Pd/Interface.h"
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

    Patch(pd::WeakReference ptr, Instance* instance, bool ownsPatch, File currentFile = File());

    ~Patch();

    // The compare equal operator.
    bool operator==(Patch const& other) const
    {
        return getPointer().get() == other.getPointer().get();
    }

    // Gets the bounds of the patch.
    Rectangle<int> getBounds() const;

    t_gobj* createObject(int x, int y, String const& name);
    t_gobj* renameObject(t_object* obj, String const& name);

    void moveObjects(std::vector<t_gobj*> const&, int x, int y);

    void moveObjectTo(t_gobj* object, int x, int y);

    void finishRemove();
    void removeObjects(std::vector<t_gobj*> const& objects);

    void deselectAll();

    bool isSubpatch();

    void setVisible(bool shouldVis);

    static String translatePatchAsString(String const& clipboardContent, Point<int> position);

    t_glist* getRoot();

    void copy(std::vector<t_gobj*> const& objects);
    void paste(Point<int> position);
    void duplicate(std::vector<t_gobj*> const& objects, t_outconnect* connection);

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
    bool canUndo() const;
    bool canRedo() const;

    void savePatch(URL const& location);
    void savePatch();

    File getCurrentFile() const;
    File getPatchFile() const;

    void setCurrentFile(URL const& newFile);

    void updateUndoRedoState();

    bool objectWasDeleted(t_gobj* ptr) const;

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
    
    t_canvas* getUncheckedPointer() const
    {
        return ptr.getRawUnchecked<t_canvas>();
    }

    // Gets the objects of the patch.
    std::vector<pd::WeakReference> getObjects();

    String getCanvasContent();

    static void reloadPatch(File const& changedPatch, t_glist* except);

    String getTitle() const;
    void setTitle(String const& title);
    void setUntitled();

    Instance* instance = nullptr;
    bool closePatchOnDelete;

    bool openInPluginMode = false;
    int splitViewIndex = 0;
    int windowIndex = 0;

    Point<int> lastViewportPosition = { 1, 1 };
    float lastViewportScale;

    String lastUndoSequence;
    String lastRedoSequence;

    int untitledPatchNum = 0;

    void updateUndoRedoString();

private:
    std::atomic<bool> canPatchUndo;
    std::atomic<bool> canPatchRedo;
    std::atomic<bool> isPatchDirty;

    File currentFile;
    URL currentURL; // We hold a URL to the patch as well, which is needed for file IO on iOS

    WeakReference ptr;

    friend class Instance;
    friend class Object;

    int undoQueueSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Patch)
};
} // namespace pd
