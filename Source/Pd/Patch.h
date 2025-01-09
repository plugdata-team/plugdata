/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Pd/Interface.h"
#include "WeakReference.h"

namespace pd {

using Connections = HeapArray<std::tuple<t_outconnect*, int, t_object*, int, t_object*>>;
class Instance;

// The Pd patch.
// Wrapper around a Pd patch. The lifetime of the internal patch
// is not guaranteed by the class.
// Has reference counting, because both the Canvas and PluginProcessor hold a reference to it
// That makes it tricky to clean up from the setStateInformation function, which may be called from any thread
class Patch final : public ReferenceCountedObject {
public:
    using Ptr = ReferenceCountedObjectPtr<Patch>;

    Patch(pd::WeakReference ptr, Instance* instance, bool ownsPatch, File currentFile = File());

    ~Patch() override;

    // The compare equal operator.
    bool operator==(Patch const& other) const
    {
        return getRawPointer() == other.getRawPointer();
    }

    // Gets the bounds of the patch.
    Rectangle<int> getBounds() const;
    Rectangle<int> getGraphBounds() const;

    t_gobj* createObject(int x, int y, String const& name);
    t_gobj* renameObject(t_object* obj, String const& name);

    void moveObjects(SmallArray<t_gobj*> const&, int x, int y);

    void moveObjectTo(t_gobj* object, int x, int y);

    void finishRemove();
    void removeObjects(SmallArray<t_gobj*> const& objects);

    void deselectAll();

    bool isSubpatch() const;

    void setVisible(bool shouldVis);

    static String translatePatchAsString(String const& clipboardContent, Point<int> position);

    t_glist* getRoot() const;

    void copy(SmallArray<t_gobj*> const& objects);
    void paste(Point<int> position);
    void duplicate(SmallArray<t_gobj*> const& objects, t_outconnect* connection);

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

    void updateUndoRedoState(SmallString undoName, SmallString redoName);

    bool hasConnection(t_object* src, int nout, t_object* sink, int nin) const;
    bool canConnect(t_object* src, int nout, t_object* sink, int nin) const;
    void createConnection(t_object* src, int nout, t_object* sink, int nin);
    t_outconnect* createAndReturnConnection(t_object* src, int nout, t_object* sink, int nin);
    void removeConnection(t_object* src, int nout, t_object* sink, int nin, t_symbol* connectionPath);
    t_outconnect* setConnctionPath(t_object* src, int nout, t_object* sink, int nin, t_symbol* oldConnectionPath, t_symbol* newConnectionPath);

    Connections getConnections() const;

    WeakReference::Ptr<t_canvas> getPointer() const
    {
        return ptr.get<t_canvas>();
    }

    t_canvas* getRawPointer() const
    {
        return ptr.getRaw<t_canvas>();
    }

    t_canvas* getUncheckedPointer() const
    {
        return ptr.getRawUnchecked<t_canvas>();
    }

    // Gets the objects of the patch.
    HeapArray<pd::WeakReference> getObjects();

    String getCanvasContent() const;

    static void reloadPatch(File const& changedPatch, t_glist* except);

    void updateTitle(SmallString const& newTitle, bool dirty);
    void updateTitle();

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

    SmallString lastUndoSequence;
    SmallString lastRedoSequence;

    int untitledPatchNum = 0;

private:
    bool canPatchUndo : 1;
    bool canPatchRedo : 1;
    bool isPatchDirty : 1;
    SmallString title;

    File currentFile;
    URL currentURL; // We hold a URL to the patch as well, which is needed for file IO on iOS

    WeakReference ptr;

    friend class Instance;
    friend class Object;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Patch)
};
} // namespace pd
