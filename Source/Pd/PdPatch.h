/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

#include "PdGui.h"
#include "x_libpd_mod_utils.h"

namespace pd
{

using Connections = std::vector<std::tuple<int, t_object*, int, t_object*>>;
class Instance;

//! @brief The Pd patch.
//! @details The class is a wrapper around a Pd patch. The lifetime of the internal patch\n
//! is not guaranteed by the class.
//! @see Instance, Object, Gui
class Patch
{
   public:
    Patch(void* ptr, Instance* instance) noexcept;

    Patch(const File& toOpen, Instance* instance) noexcept;

    //! @brief The default constructor.
    Patch() = default;

    //! @brief The compare equal operator.
    bool operator==(Patch const& other) const noexcept
    {
        return getPointer() == other.getPointer();
    }

    void close();

    //! @brief Gets the bounds of the patch.
    Rectangle<int> getBounds() const noexcept;

    std::unique_ptr<Object> createGraph(const String& name, int size, int x, int y);
    std::unique_ptr<Object> createGraphOnParent(int x, int y);

    std::unique_ptr<Object> createObject(const String& name, int x, int y, bool undoable = true);
    void removeObject(Object* obj);
    std::unique_ptr<Object> renameObject(Object* obj, const String& name);

    void moveObjects(const std::vector<Object*>&, int x, int y);

    void finishRemove();
    void removeSelection();

    void selectObject(Object*);
    void deselectAll();

    void setZoom(int zoom);

    void copy();
    void paste();
    void duplicate();

    void undo();
    void redo();

    enum GroupUndoType
    {
        Remove = 0,
        Move
    };

    void setCurrent(bool lock = false);

    bool canConnect(Object* src, int nout, Object* sink, int nin);
    bool createConnection(Object* src, int nout, Object* sink, int nin);
    void removeConnection(Object* src, int nout, Object* sink, int nin);

    static int applyZoom(int toZoom);
    static int applyUnzoom(int toUnzoom);

    Connections getConnections() const;

    t_canvas* getPointer() const
    {
        return static_cast<t_canvas*>(ptr);
    }

    //! @brief Gets the objects of the patch.
    std::vector<Object> getObjects(bool onlyGui = false) noexcept;

    String getCanvasContent()
    {
        if (!ptr) return {};
        char* buf;
        int bufsize;
        libpd_getcontent(static_cast<t_canvas*>(ptr), &buf, &bufsize);
        return {buf, static_cast<size_t>(bufsize)};
    }

    int getIndex(void* obj);

    t_gobj* getInfoObject();
    void setExtraInfoId(const String& oldId, const String& newId);

    void storeExtraInfo();

    void updateExtraInfo();
    MemoryBlock getExtraInfo(const String& id) const;
    void setExtraInfo(const String& id, MemoryBlock& info);

    ValueTree extraInfo = ValueTree("PlugDataInfo");

    static t_object* checkObject(Object* obj) noexcept;

    void keyPress(int keycode, int shift);

    String getTitle() const;
    void setTitle(const String& title);

    std::vector<t_template*> getTemplates() const;

    static inline float zoom = 1.3f;

   private:
    void* ptr = nullptr;
    Instance* instance = nullptr;

    t_gobj* infoObject = nullptr;

    // Initialisation parameters for GUI objects
    // Taken from pd save files, this will make sure that it directly initialises objects with the right parameters, which is important for correct undo/redo
    static inline const std::map<String, String> guiDefaults = {
        {"tgl", "15 0 empty empty empty 17 7 0 10 #171717 #42a2c8 #ffffff 0 1"},
        {"hsl", "128 15 0 127 0 0 empty empty empty -2 -8 0 10 #171717 #42a2c8 #ffffff 0 1"},
        {"vsl", "15 128 0 127 0 0 empty empty empty 0 -9 0 10 #171717 #42a2c8 #ffffff 0 1"},
        {"bng", "15 250 50 0 empty empty empty 17 7 0 10 #171717 #42a2c8 #ffffff"},
        {"nbx", "3 14 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 #171717 #42a2c8 #ffffff 0 256"},
        {"hradio", "15 1 0 8 empty empty empty 0 -8 0 10 #171717 #42a2c8 #ffffff 0"},
        {"vradio", "15 1 0 8 empty empty empty 0 -8 0 10 #171717 #42a2c8 #ffffff 0"},
        {"cnv", "15 100 60 empty empty empty 20 12 0 14 #171717 #404040"},

    };

    friend class Instance;
    friend class Gui;
    friend class Object;
};
}  // namespace pd
