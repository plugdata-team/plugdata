/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdObject.h"

#include "PdInstance.h"
extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <s_stuff.h>

#include "x_libpd_extra_utils.h"
    extern t_class* text_class;
}

struct _outconnect
{
    struct _outconnect* oc_next;
    t_pd* oc_to;
};

struct _outlet
{
    t_object* o_owner;
    struct _outlet* o_next;
    t_outconnect* o_connections;
    t_symbol* o_sym;
};

namespace pd
{

Object::Object(void* objectPtr, Patch* parentPatch, Instance* parentInstance) noexcept : ptr(objectPtr), patch(parentPatch), instance(parentInstance)
{
}

bool Object::operator==(Object const& other) const noexcept
{
    return ptr == other.ptr;
}

bool Object::operator!=(Object const& other) const noexcept
{
    return ptr != other.ptr;
}

std::string Object::getText()
{
    if (ptr && patch->checkObject(this))
    {
        char* text = nullptr;
        int size = 0;
        instance->setThis();

        libpd_get_object_text(ptr, &text, &size);
        if (text && size)
        {
            std::string txt(text, size);
            freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
            return txt;
        }
    }
    return {};
}

std::string Object::getName() const
{
    if (ptr)
    {
        char const* name = libpd_get_object_class_name(ptr);
        if (name)
        {
            return {name};
        }
    }
    return {};
}

Rectangle<int> Object::getBounds() const noexcept
{
    int x = 0, y = 0, w = 0, h = 0;
    instance->setThis();

    // If it's a text object, we need to handle the resizable width, which pd saves in amount of text characters
    auto* textObj = static_cast<t_text*>(ptr);

    libpd_get_object_bounds(patch->getPointer(), ptr, &x, &y, &w, &h);

    w = textObj->te_width * glist_fontwidth(patch->getPointer());

    return {x, y, w, h};
}

//! @brief The name of the help file
Patch Object::getHelp() const
{
    static File appDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
    static File helpDir = appDir.getChildFile("Documentation/5.reference/");

    auto* pdclass = pd_class(static_cast<t_pd*>(ptr));
    auto* name = class_gethelpname(pdclass);
    auto* dir = class_gethelpdir(pdclass);

    char realname[MAXPDSTRING];
    /* make up a silly "dir" if none is supplied */

    const String& fullPath = helpDir.getFullPathName();
    const char* usedir = (*dir ? dir : fullPath.toRawUTF8());

    /* 1. "objectname-help.pd" */
    strncpy(realname, name, MAXPDSTRING - 10);
    realname[MAXPDSTRING - 10] = 0;
    if (strlen(realname) > 3 && !strcmp(realname + strlen(realname) - 3, ".pd")) realname[strlen(realname) - 3] = 0;
    strcat(realname, "-help.pd");

    if (File(usedir).getChildFile(realname).existsAsFile())
    {
        sys_lock();
        auto* pdPatch = glob_evalfile(nullptr, gensym(realname), gensym(usedir));
        sys_unlock();
        return {pdPatch, instance};
    }

    /* 2. "help-objectname.pd" */
    strcpy(realname, "help-");
    strncat(realname, name, MAXPDSTRING - 10);
    realname[MAXPDSTRING - 1] = 0;

    if (File(dir).getChildFile(realname).existsAsFile())
    {
        sys_lock();
        auto* pdPatch = glob_evalfile(nullptr, gensym(realname), gensym(usedir));
        sys_unlock();
        return {pdPatch, instance};
    }

    return {nullptr, nullptr};
}

void Object::setBounds(Rectangle<int> bounds)
{
    auto* textObj = static_cast<t_text*>(ptr);
    short newWidth = std::max<short>(3, bounds.getWidth() / glist_fontwidth(patch->getPointer()));

    if (newWidth != textObj->te_width)
    {
        addUndoableAction();
        textObj->te_width = newWidth;

        libpd_moveobj(patch->getPointer(), static_cast<t_gobj*>(getPointer()), bounds.getX(), bounds.getY());
    }
}

int Object::getNumInlets() noexcept
{
    if (auto* checked = patch->checkObject(this))
    {
        return libpd_ninlets(checked);
    }

    return 0;
}
int Object::getNumOutlets() noexcept
{
    if (auto* checked = patch->checkObject(this))
    {
        return libpd_noutlets(checked);
    }

    return 0;
}

bool Object::isSignalInlet(int idx) noexcept
{
    if (auto* checked = patch->checkObject(this))
    {
        return libpd_issignalinlet(checked, idx);
    }

    return false;
}
bool Object::isSignalOutlet(int idx) noexcept
{
    if (auto* checked = patch->checkObject(this))
    {
        return libpd_issignaloutlet(checked, idx);
    }

    return false;
}

void Object::addUndoableAction()
{
    // Used for size changes, could also be used for properties
    auto* obj = static_cast<t_gobj*>(getPointer());
    auto* cnv = static_cast<t_canvas*>(patch->getPointer());
    libpd_undo_apply(cnv, obj);
}

void Object::toFront() {
    
    auto glist_getindex = [](t_glist* x, t_gobj* y){
        t_gobj* y2;
        int indx;
        for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next) indx++;
        return (indx);
    };
    
    auto glist_nth = [](t_glist *x, int n) -> t_gobj* {
        t_gobj *y;
        int indx;
        for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
            if (indx == n)
                return (y);
       
        jassertfalse;
        return nullptr;
    };

    auto* cnv = static_cast<t_canvas*>(patch->getPointer());
    t_gobj* y = static_cast<t_gobj*>(getPointer());
    
    t_gobj *y_prev, *y_next;
    t_gobj *y_begin = cnv->gl_list;
    
        /* if there is an object before ours (in other words our index is > 0) */
    if (int idx = glist_getindex(cnv, y))
        y_prev = glist_nth(cnv, idx - 1);

        /* if there is an object after ours */
    if (y->g_next)
        y_next = y->g_next;

    t_gobj *y_end = glist_nth(cnv, glist_getindex(cnv, 0) - 1);

    y_end->g_next = y;
    y->g_next = NULL;

        /* now fix links in the hole made in the list due to moving of the oldy
         * (we know there is oldy_next as y_end != oldy in canvas_done_popup)
         */
    if (y_prev) /* there is indeed more before the oldy position */
        y_prev->g_next = y_next;
    else cnv->gl_list = y_next;
}

}  // namespace pd
