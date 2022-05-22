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


String Object::getText(void* obj)
{
    bool hasText = pd_class(&pd_checkobject(static_cast<t_pd*>(obj))->te_g.g_pd) == text_class;
    if (obj && hasText)
    {
        char* text = nullptr;
        int size = 0;
        
        libpd_get_object_text(pd_checkobject(static_cast<t_pd*>(obj)), &text, &size);
        if (text && size)
        {
            std::string txt(text, size);
            freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
            return String(txt);
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
    
    return {x, y, textObj->te_width, h};
}



void Object::setBounds(Rectangle<int> bounds)
{
    auto* textObj = static_cast<t_text*>(ptr);

    if (bounds.getWidth() != textObj->te_width)
    {
        addUndoableAction();
        textObj->te_width = bounds.getWidth();
    }
    
    libpd_moveobj(patch->getPointer(), static_cast<t_gobj*>(getPointer()), bounds.getX(), bounds.getY());
}


void Object::addUndoableAction()
{
    // Used for size changes, could also be used for properties
    auto* obj = static_cast<t_gobj*>(getPointer());
    auto* cnv = static_cast<t_canvas*>(patch->getPointer());
    libpd_undo_apply(cnv, obj);
}

void Object::toFront()
{
    auto glist_getindex = [](t_glist* x, t_gobj* y)
    {
        t_gobj* y2;
        int indx;
        for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next) indx++;
        return (indx);
    };

    auto glist_nth = [](t_glist* x, int n) -> t_gobj*
    {
        t_gobj* y;
        int indx;
        for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
            if (indx == n) return (y);

        jassertfalse;
        return nullptr;
    };

    auto* cnv = static_cast<t_canvas*>(patch->getPointer());
    t_gobj* y = static_cast<t_gobj*>(getPointer());

    t_gobj *y_prev = nullptr, *y_next = nullptr;

    /* if there is an object before ours (in other words our index is > 0) */
    if (int idx = glist_getindex(cnv, y)) y_prev = glist_nth(cnv, idx - 1);

    /* if there is an object after ours */
    if (y->g_next) y_next = y->g_next;

    t_gobj* y_end = glist_nth(cnv, glist_getindex(cnv, 0) - 1);

    y_end->g_next = y;
    y->g_next = NULL;

    /* now fix links in the hole made in the list due to moving of the oldy
     * (we know there is oldy_next as y_end != oldy in canvas_done_popup)
     */
    if (y_prev) /* there is indeed more before the oldy position */
        y_prev->g_next = y_next;
    else
        cnv->gl_list = y_next;
}

}  // namespace pd
