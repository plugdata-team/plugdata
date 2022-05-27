/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdPatch.h"

#include "PdInstance.h"
#include "PdStorage.h"
#include "../Objects/GUIObject.h"

extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>

#include "g_undo.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_multi.h"

    struct _instanceeditor
    {
        t_binbuf* copy_binbuf;
        char* canvas_textcopybuf;
        int canvas_textcopybufsize;
        t_undofn canvas_undo_fn;      /* current undo function if any */
        int canvas_undo_whatnext;     /* whether we can now UNDO or REDO */
        void* canvas_undo_buf;        /* data private to the undo function */
        t_canvas* canvas_undo_canvas; /* which canvas we can undo on */
        const char* canvas_undo_name;
        int canvas_undo_already_set_move;
        double canvas_upclicktime;
        int canvas_upx, canvas_upy;
        int canvas_find_index, canvas_find_wholeword;
        t_binbuf* canvas_findbuf;
        int paste_onset;
        t_canvas* paste_canvas;
        t_glist* canvas_last_glist;
        int canvas_last_glist_x, canvas_last_glist_y;
        t_canvas* canvas_cursorcanvaswas;
        unsigned int canvas_cursorwas;
    };

    static void canvas_bind(t_canvas* x)
    {
        if (strcmp(x->gl_name->s_name, "Pd")) pd_bind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    }

    static void canvas_unbind(t_canvas* x)
    {
        if (strcmp(x->gl_name->s_name, "Pd")) pd_unbind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    }

    void canvas_map(t_canvas* x, t_floatarg f);

    void canvas_declare(t_canvas* x, t_symbol* s, int argc, t_atom* argv);
}

namespace pd
{

Patch::Patch(void* patchPtr, Instance* parentInstance, File patchFile) : ptr(patchPtr), instance(parentInstance), currentFile(patchFile)
{
    if (auto* cnv = getPointer())
    {
        instance->getCallbackLock()->enter();

        setCurrent();
        setZoom(1);

        instance->getCallbackLock()->exit();
    }
}

Rectangle<int> Patch::getBounds() const noexcept
{
    if (ptr)
    {
        t_canvas* cnv = getPointer();

        if (cnv->gl_isgraph)
        {
            cnv->gl_pixwidth = std::max(50, cnv->gl_pixwidth);
            cnv->gl_pixheight = std::max(50, cnv->gl_pixheight);
            
            return {cnv->gl_xmargin, cnv->gl_ymargin, cnv->gl_pixwidth, cnv->gl_pixheight};
        }
    }
    return {0, 0, 0, 0};
}

void Patch::close()
{
    libpd_closefile(ptr);
}

bool Patch::isDirty()
{
    return getPointer()->gl_dirty;
}

void Patch::savePatch(const File& location)
{
    String fullPathname = location.getParentDirectory().getFullPathName();
    String filename = location.getFileName();

    auto* dir = gensym(fullPathname.toRawUTF8());
    auto* file = gensym(filename.toRawUTF8());
    libpd_savetofile(getPointer(), file, dir);

    setTitle(filename);

    canvas_dirty(getPointer(), 0);
    currentFile = location;
}

void Patch::savePatch()
{
    String fullPathname = currentFile.getParentDirectory().getFullPathName();
    String filename = currentFile.getFileName();

    auto* dir = gensym(fullPathname.toRawUTF8());
    auto* file = gensym(filename.toRawUTF8());

    libpd_savetofile(getPointer(), file, dir);

    setTitle(filename);

    canvas_dirty(getPointer(), 0);
}

void Patch::setCurrent(bool lock)
{
    instance->setThis();  // important for canvas_getcurrent

    if (!ptr) return;

    if (lock) instance->getCallbackLock()->enter();

    auto* cnv = canvas_getcurrent();

    if (cnv)
    {
        canvas_unsetcurrent(cnv);
    }

    canvas_setcurrent(getPointer());
    canvas_vis(getPointer(), 1.);
    canvas_map(getPointer(), 1.);

    t_atom argv[1];
    SETFLOAT(argv, 1);
    pd_typedmess((t_pd*)getPointer(), gensym("pop"), 1, argv);

    if (lock) instance->getCallbackLock()->exit();
}

int Patch::getIndex(void* obj)
{
    int i = 0;
    auto* cnv = getPointer();

    for (t_gobj* y = cnv->gl_list; y; y = y->g_next)
    {
        if (Storage::isInfoParent(y)) continue;

        if (obj == y)
        {
            return i;
        }

        i++;
    }

    return -1;
}

Connections Patch::getConnections() const
{
    Connections connections;

    t_outconnect* oc;
    t_linetraverser t;
    auto* x = getPointer();

    // Get connections from pd
    linetraverser_start(&t, x);

    while ((oc = linetraverser_next(&t)))
    {
        connections.push_back({t.tr_inno, t.tr_ob, t.tr_outno, t.tr_ob2});
    }

    return connections;
}

std::vector<void*> Patch::getObjects(bool onlyGui) noexcept
{
    if (ptr)
    {
        std::vector<void*> objects;
        t_canvas const* cnv = getPointer();

        for (t_gobj* y = cnv->gl_list; y; y = y->g_next)
        {
            if (Storage::isInfoParent(y)) continue;

            if (onlyGui && y->g_pd->c_gobj)
            {
                objects.push_back(static_cast<void*>(y));
            }
            else if (!onlyGui)
            {
                objects.push_back(static_cast<void*>(y));
            }
        }

        return objects;
    }
    return {};
}

void* Patch::createGraphOnParent(int x, int y)
{
    t_pd* pdobject = nullptr;
    std::atomic<bool> done = false;

    instance->enqueueFunction(
        [this, x, y, &pdobject, &done]() mutable
        {
            setCurrent();
            pdobject = libpd_creategraphonparent(getPointer(), x, y);
            done = true;
        });

    while (!done)
    {
        instance->waitForStateUpdate();
    }

    assert(pdobject);

    return pdobject;
}

void* Patch::createGraph(const String& name, int size, int x, int y)
{
    t_pd* pdobject = nullptr;
    std::atomic<bool> done = false;

    instance->enqueueFunction(
        [this, name, size, x, y, &pdobject, &done]() mutable
        {
            setCurrent();
            pdobject = libpd_creategraph(getPointer(), name.toRawUTF8(), size, x, y);
            done = true;
        });

    while (!done)
    {
        instance->waitForStateUpdate();
    }

    assert(pdobject);

    return pdobject;
}

void* Patch::createObject(const String& name, int x, int y)
{
    if (!ptr) return nullptr;

    StringArray tokens;
    tokens.addTokens(name, false);

    // See if we have preset parameters for this object
    // These parameters are designed to make the experience in plugdata better
    // Mostly larger GUI objects and a different colour scheme
    if (guiDefaults.find(tokens[0]) != guiDefaults.end())
    {
        auto preset = guiDefaults.at(tokens[0]);

        auto bg = instance->getBackgroundColour().toString().substring(2);
        auto fg = instance->getForegroundColour().toString().substring(2);
        auto lbl = instance->getTextColour().toString().substring(2);
        auto ln = instance->getOutlineColour().toString().substring(2);

        preset = preset.replace("bgColour", "#" + bg);
        preset = preset.replace("fgColour", "#" + fg);
        preset = preset.replace("lblColour", "#" + lbl);
        preset = preset.replace("lnColour", "#" + ln);

        tokens.addTokens(preset, false);
    }

    if (tokens[0] == "graph" && tokens.size() == 3)
    {
        return createGraph(tokens[1], tokens[2].getIntValue(), x, y);
    }
    else if (tokens[0] == "graph")
    {
        return createGraphOnParent(x, y);
    }

    t_symbol* typesymbol = gensym("obj");

    if (tokens[0] == "msg")
    {
        typesymbol = gensym("msg");
        tokens.remove(0);
    }
    if (tokens[0] == "comment")
    {
        typesymbol = gensym("text");
        tokens.remove(0);
    }
    if (tokens[0] == "floatatom")
    {
        typesymbol = gensym("floatatom");
        tokens.remove(0);
    }
    if (tokens[0] == "listbox")
    {
        typesymbol = gensym("listbox");
        tokens.remove(0);
    }
    if (tokens[0] == "symbolatom")
    {
        typesymbol = gensym("symbolatom");
        tokens.remove(0);
    }
    if (tokens[0] == "+")
    {
        tokens.set(0, "\\+");
    }

    int argc = tokens.size() + 2;

    auto argv = std::vector<t_atom>(argc);

    // Set position
    SETFLOAT(argv.data(), static_cast<float>(x));
    SETFLOAT(argv.data() + 1, static_cast<float>(y));

    for (int i = 0; i < tokens.size(); i++)
    {
        auto& tok = tokens[i];
        if (tokens[i].containsOnly("0123456789e.-+") && tokens[i] != "-")
        {
            SETFLOAT(argv.data() + i + 2, tokens[i].getFloatValue());
        }
        else
        {
            SETSYMBOL(argv.data() + i + 2, gensym(tokens[i].toRawUTF8()));
        }
    }

    t_pd* pdobject = nullptr;
    std::atomic<bool> done = false;

    instance->enqueueFunction(
        [this, argc, argv, typesymbol, &pdobject, &done]() mutable
        {
            setCurrent();
            pdobject = libpd_createobj(getPointer(), typesymbol, argc, argv.data());
            done = true;
        });

    while (!done)
    {
        instance->waitForStateUpdate();
    }

    assert(pdobject);
    return pdobject;
}

static int glist_getindex(t_glist* x, t_gobj* y)
{
    t_gobj* y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next) indx++;
    return (indx);
}

void* Patch::renameObject(void* obj, const String& name)
{
    if (!obj || !ptr) return nullptr;

    // Cant use the queue for this...
    setCurrent(true);

    auto type = name.upToFirstOccurrenceOf(" ", false, false);
    String newName = name;
    // Also apply default style when renaming
    if (guiDefaults.find(type) != guiDefaults.end())
    {
        auto preset = guiDefaults.at(type);

        auto bg = instance->getBackgroundColour().toString().substring(2);
        auto fg = instance->getForegroundColour().toString().substring(2);
        auto lbl = instance->getTextColour().toString().substring(2);
        auto ln = instance->getOutlineColour().toString().substring(2);

        preset = preset.replace("bgColour", "#" + bg);
        preset = preset.replace("fgColour", "#" + fg);
        preset = preset.replace("lblColour", "#" + lbl);
        preset = preset.replace("lnColour", "#" + ln);

        newName += " " + preset;
    }

    instance->enqueueFunction([this, obj, newName]() mutable { libpd_renameobj(getPointer(), &checkObject(obj)->te_g, newName.toRawUTF8(), newName.getNumBytesAsUTF8()); });

    instance->waitForStateUpdate();

    setCurrent(true);

    return libpd_newest(getPointer());
}

void Patch::copy()
{
    instance->enqueueFunction(
        [this]()
        {
            int size;
            const char* text = libpd_copy(getPointer(), &size);
            auto copied = String(CharPointer_UTF8(text), size);
            MessageManager::callAsync([copied]() mutable { SystemClipboard::copyTextToClipboard(copied); });
        });
}

void Patch::paste()
{
    auto text = SystemClipboard::getTextFromClipboard();

    instance->enqueueFunction([this, text]() mutable { libpd_paste(getPointer(), text.toRawUTF8()); });
}

void Patch::duplicate()
{
    instance->enqueueFunction(
        [this]()
        {
            setCurrent();
            libpd_duplicate(getPointer());
        });
}

void Patch::selectObject(void* obj)
{
    instance->enqueueFunction(
        [this, obj]()
        {
            auto* checked = &checkObject(obj)->te_g;
            if (!glist_isselected(getPointer(), checked))
            {
                glist_select(getPointer(), checked);
            }
        });
}

void Patch::deselectAll()
{
    instance->enqueueFunction(
        [this]()
        {
            glist_noselect(getPointer());
            EDITOR->canvas_undo_already_set_move = 0;
        });
}

void Patch::removeObject(void* obj)
{
    if (!obj || !ptr) return;

    instance->enqueueFunction(
        [this, obj]()
        {
            setCurrent();
            libpd_removeobj(getPointer(), &checkObject(obj)->te_g);
        });
}

bool Patch::hasConnection(void* src, int nout, void* sink, int nin)
{
    bool hasConnection = false;
    bool hasReturned = false;

    instance->enqueueFunction(
        [this, &hasConnection, &hasReturned, src, nout, sink, nin]() mutable
        {
            hasConnection = libpd_hasconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
            hasReturned = true;
        });

    while (!hasReturned)
    {
        instance->waitForStateUpdate();
    }

    return hasConnection;
}

bool Patch::canConnect(void* src, int nout, void* sink, int nin)
{
    bool canConnect = false;

    instance->enqueueFunction([this, &canConnect, src, nout, sink, nin]() mutable { canConnect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin); });

    instance->waitForStateUpdate();

    return canConnect;
}

bool Patch::createConnection(void* src, int nout, void* sink, int nin)
{
    if (!src || !sink || !ptr) return false;

    bool canConnect = false;

    instance->enqueueFunction(
        [this, &canConnect, src, nout, sink, nin]() mutable
        {
            canConnect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);

            if (!canConnect) return;

            setCurrent();

            libpd_createconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
        });

    instance->waitForStateUpdate();

    return canConnect;
}

void Patch::removeConnection(void* src, int nout, void* sink, int nin)
{
    if (!src || !sink || !ptr) return;

    instance->enqueueFunction(
        [this, src, nout, sink, nin]() mutable
        {
            setCurrent();

            libpd_removeconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
        });
}

void Patch::moveObjects(const std::vector<void*>& objects, int dx, int dy)
{
    // if(!obj || !ptr) return;

    instance->enqueueFunction(
        [this, objects, dx, dy]() mutable
        {
            setCurrent();

            glist_noselect(getPointer());

            for (auto* obj : objects)
            {
                if (!obj) continue;

                glist_select(getPointer(), &checkObject(obj)->te_g);
            }

            libpd_moveselection(getPointer(), dx, dy);

            glist_noselect(getPointer());
            EDITOR->canvas_undo_already_set_move = 0;
            setCurrent();
        });
}

void Patch::finishRemove()
{
    instance->enqueueFunction(
        [this]() mutable
        {
            setCurrent();
            libpd_finishremove(getPointer());
        });
}

void Patch::removeSelection()
{
    instance->enqueueFunction(
        [this]() mutable
        {
            setCurrent();

            libpd_removeselection(getPointer());
        });
}

void Patch::undo()
{
    instance->enqueueFunction(
        [this]()
        {
            setCurrent();
            glist_noselect(getPointer());
            EDITOR->canvas_undo_already_set_move = 0;

            libpd_undo(getPointer());

            setCurrent();
        });
}

void Patch::redo()
{
    instance->enqueueFunction(
        [this]()
        {
            setCurrent();
            glist_noselect(getPointer());
            EDITOR->canvas_undo_already_set_move = 0;

            libpd_redo(getPointer());

            setCurrent();
        });
}

void Patch::setZoom(int newZoom)
{
    t_atom arg;
    SETFLOAT(&arg, newZoom);

    pd_typedmess(static_cast<t_pd*>(ptr), gensym("zoom"), 2, &arg);
}

t_object* Patch::checkObject(void* obj) noexcept
{
    return pd_checkobject(static_cast<t_pd*>(obj));
}

void Patch::keyPress(int keycode, int shift)
{
    t_atom args[3];

    SETFLOAT(args, 1);
    SETFLOAT(args + 1, keycode);
    SETFLOAT(args + 2, shift);

    pd_typedmess(static_cast<t_pd*>(ptr), gensym("key"), 3, args);
}

String Patch::getTitle() const
{
    return {getPointer()->gl_name->s_name};
}

void Patch::setTitle(const String& title)
{
    if (!getPointer()) return;

    canvas_unbind(getPointer());
    getPointer()->gl_name = gensym(title.toRawUTF8());
    canvas_bind(getPointer());
    instance->titleChanged();
}

}  // namespace pd
