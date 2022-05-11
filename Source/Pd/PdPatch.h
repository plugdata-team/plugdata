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
    Patch(void* ptr, Instance* instance, File currentFile = File());

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

    std::unique_ptr<Object> createObject(const String& name, int x, int y);
    void removeObject(Object* obj);
    std::unique_ptr<Object> renameObject(Object* obj, const String& name);

    void moveObjects(const std::vector<Object*>&, int x, int y);

    void finishRemove();
    void removeSelection();

    void selectObject(Object*);
    void deselectAll();

    bool isDirty();

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

    bool isDirty() const;

    void savePatch(const File& location);
    void savePatch();

    File getCurrentFile() const
    {
        return currentFile;
    }
    void setCurrentFile(File newFile)
    {
        currentFile = newFile;
    }

    bool canConnect(Object* src, int nout, Object* sink, int nin);
    bool createConnection(Object* src, int nout, Object* sink, int nin);
    void removeConnection(Object* src, int nout, Object* sink, int nin);

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

    static t_object* checkObject(Object* obj) noexcept;

    void keyPress(int keycode, int shift);

    String getTitle() const;
    void setTitle(const String& title);

    std::vector<t_template*> getTemplates() const;

    Instance* instance = nullptr;

   private:
    File currentFile;

    void* ptr = nullptr;

    // Initialisation parameters for GUI objects
    // Taken from pd save files, this will make sure that it directly initialises objects with the right parameters
    static inline const std::map<String, String> guiDefaults = {{"tgl", "23 0 empty empty empty 17 7 0 10 bgColour fgColour lblColour 0 1"},
                                                                {"hsl", "128 15 0 127 0 0 empty empty empty -2 -8 0 10 bgColour lnColour lblColour 0 1"},
                                                                {"vsl", "15 128 0 127 0 0 empty empty empty 0 -9 0 10 bgColour lnColour lblColour 0 1"},
                                                                {"bng", "23 250 50 0 empty empty empty 17 7 0 10 bgColour fgColour lblColour"},
                                                                {"nbx", "4 19 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 bgColour lnColour lblColour 0 256"},
                                                                {"hradio", "18 1 0 8 empty empty empty 0 -8 0 10 bgColour fgColour lblColour 0"},
                                                                {"vradio", "18 1 0 8 empty empty empty 0 -8 0 10 bgColour fgColour lblColour 0"},
                                                                {"cnv", "15 100 60 empty empty empty 20 12 0 14 #8b949e lblColour"},
                                                                {"vu", "15 120 empty empty -1 -8 0 10 bgColour lblColour 1 0"},
                                                                {"floatatom", "5 -3.40282e+38 3.40282e+38 0 empty - - 12"}};

    friend class Instance;
    friend class Gui;
    friend class Object;
};
}  // namespace pd
