/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "GUIObject.h"

extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <g_all_guis.h>
}

#include "Box.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Pd/PdPatch.h"

#include "IEMObject.h"
#include "AtomObject.h"

#include "TextObject.h"
#include "ToggleObject.h"
#include "MessageObject.h"
#include "MouseObject.h"
#include "BangObject.h"
#include "RadioObject.h"
#include "SliderObject.h"
#include "ArrayObject.h"
#include "GraphOnParent.h"
#include "KeyboardObject.h"
#include "MousePadObject.h"
#include "NumberObject.h"
#include "CanvasObject.h"
#include "PictureObject.h"
#include "VUMeterObject.h"
#include "ListObject.h"
#include "SubpatchObject.h"
#include "CloneObject.h"
#include "CommentObject.h"
#include "FloatAtomObject.h"
#include "SymbolAtomObject.h"
#include "ScalarObject.h"

ObjectBase::ObjectBase(void* obj, Box* parent) : ptr(obj), box(parent), cnv(box->cnv){};

String ObjectBase::getText()
{
    if (!cnv->patch.checkObject(ptr)) return "";

    char* text = nullptr;
    int size = 0;
    cnv->pd->setThis();

    libpd_get_object_text(ptr, &text, &size);
    if (text && size)
    {
        String txt(text, size);
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
        return txt;
    }

    return "";
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void ObjectBase::closeOpenedSubpatchers()
{
    auto& main = box->cnv->main;
    auto* tabbar = &main.tabbar;

    if (!tabbar) return;

    for (int n = 0; n < tabbar->getNumTabs(); n++)
    {
        auto* cnv = main.getCanvas(n);
        if (cnv && cnv->patch == *getPatch())
        {
            auto* deleted_patch = &cnv->patch;
            main.canvases.removeObject(cnv);
            tabbar->removeTab(n);
            main.pd.patches.removeObject(deleted_patch, false);
        }
    }

    if (tabbar->getNumTabs() > 1)
    {
        tabbar->getTabbedButtonBar().setVisible(true);
        tabbar->setTabBarDepth(29);
        main.resized();
    }
    else
    {
        tabbar->getTabbedButtonBar().setVisible(false);
        tabbar->setTabBarDepth(1);
        main.resized();
    }
}

void ObjectBase::moveToFront()
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

    auto* canvas = static_cast<t_canvas*>(cnv->patch.getPointer());
    t_gobj* y = static_cast<t_gobj*>(ptr);

    t_gobj *y_prev = nullptr, *y_next = nullptr;

    /* if there is an object before ours (in other words our index is > 0) */
    if (int idx = glist_getindex(canvas, y)) y_prev = glist_nth(canvas, idx - 1);

    /* if there is an object after ours */
    if (y->g_next) y_next = y->g_next;

    t_gobj* y_end = glist_nth(canvas, glist_getindex(canvas, 0) - 1);

    y_end->g_next = y;
    y->g_next = NULL;

    /* now fix links in the hole made in the list due to moving of the oldy
     * (we know there is oldy_next as y_end != oldy in canvas_done_popup)
     */
    if (y_prev) /* there is indeed more before the oldy position */
        y_prev->g_next = y_next;
    else
        canvas->gl_list = y_next;
}

void ObjectBase::paint(Graphics& g)
{
    // make sure text is readable
    // TODO: move this to places where it's relevant
    getLookAndFeel().setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));
    getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(PlugDataColour::textColourId));
    getLookAndFeel().setColour(TextEditor::textColourId, box->findColour(PlugDataColour::textColourId));

    g.setColour(box->findColour(PlugDataColour::toolbarColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

    auto outlineColour = box->findColour(cnv->isSelected(box) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);

    g.setColour(outlineColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
}

NonPatchable::NonPatchable(void* obj, Box* parent) : ObjectBase(obj, parent)
{
    // Make object invisible
    box->setVisible(false);
}

NonPatchable::~NonPatchable()
{
}

GUIObject::GUIObject(void* obj, Box* parent) : ObjectBase(obj, parent), processor(*parent->cnv->pd), edited(false)
{
    box->addComponentListener(this);
    updateLabel();

    setWantsKeyboardFocus(true);

    setLookAndFeel(dynamic_cast<PlugDataLook*>(&LookAndFeel::getDefaultLookAndFeel())->getPdLook());
}

GUIObject::~GUIObject()
{
    box->removeComponentListener(this);
    auto* lnf = &getLookAndFeel();
    setLookAndFeel(nullptr);
    delete lnf;
}

void GUIObject::mouseDown(const MouseEvent& e)
{
    if (box->commandLocked == var(true))
    {
        auto& sidebar = box->cnv->main.sidebar;
        inspectorWasVisible = !sidebar.isShowingConsole();
        sidebar.hideParameters();
    }
}

void GUIObject::mouseUp(const MouseEvent& e)
{
    if (box->commandLocked == var(true) && inspectorWasVisible)
    {
        box->cnv->main.sidebar.showParameters();
    }
}

void GUIObject::initialise()
{
    getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(Label::textWhenEditingColourId));
    getLookAndFeel().setColour(Label::textColourId, box->findColour(Label::textColourId));

    auto params = getParameters();
    for (auto& [name, type, cat, value, list] : params)
    {
        value->addListener(this);

        // Push current parameters to pd
        valueChanged(*value);
    }

    repaint();
}

ObjectParameters GUIObject::defineParameters()
{
    return {};
};

ObjectParameters GUIObject::getParameters()
{
    return defineParameters();
}

float GUIObject::getValueOriginal() const noexcept
{
    return value;
}

void GUIObject::setValueOriginal(float v)
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());

    value = (minimum < maximum) ? std::max(std::min(v, maximum), minimum) : std::max(std::min(v, minimum), maximum);

    setValue(value);
}

float GUIObject::getValueScaled() const noexcept
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());

    return (minimum < maximum) ? (value - minimum) / (maximum - minimum) : 1.f - (value - maximum) / (minimum - maximum);
}

void GUIObject::setValueScaled(float v)
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());

    value = (minimum < maximum) ? std::max(std::min(v, 1.f), 0.f) * (maximum - minimum) + minimum : (1.f - std::max(std::min(v, 1.f), 0.f)) * (minimum - maximum) + maximum;
    setValue(value);
}

void GUIObject::startEdition() noexcept
{
    edited = true;
    processor.enqueueMessages("gui", "mouse", {1.f});

    value = getValue();
}

void GUIObject::stopEdition() noexcept
{
    edited = false;
    processor.enqueueMessages("gui", "mouse", {0.f});
}

void GUIObject::updateValue()
{
    if (!edited)
    {
        auto thisPtr = SafePointer<GUIObject>(this);
        box->cnv->pd->enqueueFunction(
            [thisPtr]()
            {
                if (!thisPtr) return;

                float const v = thisPtr->getValue();
                if (thisPtr->value != v)
                {
                    MessageManager::callAsync(
                        [thisPtr, v]() mutable
                        {
                            if (thisPtr)
                            {
                                thisPtr->value = v;
                                thisPtr->update();
                            }
                        });
                }
            });
    }
}

void GUIObject::componentMovedOrResized(Component& component, bool moved, bool resized)
{
    updateLabel();

    if (!resized) return;

    checkBounds();
}

void GUIObject::setValue(float value) noexcept
{
    cnv->pd->enqueueDirectMessages(ptr, value);
}

String GUIObject::getName() const
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

ObjectBase* GUIObject::createGui(void* ptr, Box* parent)
{
    const String name = libpd_get_object_class_name(ptr);
    if (name == "bng")
    {
        return new BangObject(ptr, parent);
    }
    if (name == "hsl")
    {
        return new SliderObject(false, ptr, parent);
    }
    if (name == "vsl")
    {
        return new SliderObject(true, ptr, parent);
    }
    if (name == "tgl")
    {
        return new ToggleObject(ptr, parent);
    }
    if (name == "nbx")
    {
        return new NumberObject(ptr, parent);
    }
    if (name == "vradio")
    {
        return new RadioObject(true, ptr, parent);
    }
    if (name == "hradio")
    {
        return new RadioObject(false, ptr, parent);
    }
    if (name == "cnv")
    {
        return new CanvasObject(ptr, parent);
    }
    if (name == "vu")
    {
        return new VUMeterObject(ptr, parent);
    }
    if (name == "text")
    {
        auto* textObj = static_cast<t_text*>(ptr);
        if (textObj->te_type == T_OBJECT)
        {
            return new TextObject(ptr, parent, false);
        }
        else
        {
            return new CommentObject(ptr, parent);
        }
    }
    // Check size to prevent confusing it with else/message
    if (name == "message" && static_cast<t_gobj*>(ptr)->g_pd->c_size == sizeof(t_message))
    {
        return new MessageObject(ptr, parent);
    }
    else if (name == "pad")
    {
        return new MousePadObject(ptr, parent);
    }
    else if (name == "mouse")
    {
        return new MouseObject(ptr, parent);
    }
    else if (name == "keyboard")
    {
        return new KeyboardObject(ptr, parent);
    }
    else if (name == "pic")
    {
        return new PictureObject(ptr, parent);
    }

    else if (name == "gatom")
    {
        if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_FLOAT)
            return new FloatAtomObject(ptr, parent);
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_SYMBOL)
            return new SymbolAtomObject(ptr, parent);
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_NULL)
            return new ListObject(ptr, parent);
    }
    else if (name == "canvas" || name == "graph")
    {
        if (static_cast<t_canvas*>(ptr)->gl_list)
        {
            t_class* c = static_cast<t_canvas*>(ptr)->gl_list->g_pd;
            if (c && c->c_name && (String(c->c_name->s_name) == "array"))
            {
                return new ArrayObject(ptr, parent);
            }
            else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
            {
                return new GraphOnParent(ptr, parent);
            }
            else
            {  // abstraction or subpatch
                return new SubpatchObject(ptr, parent);
            }
        }
        else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
        {
            return new GraphOnParent(ptr, parent);
        }
        else
        {
            return new SubpatchObject(ptr, parent);
        }
    }
    else if (name == "clone")
    {
        return new CloneObject(ptr, parent);
    }
    else if (name == "pd")
    {
        return new SubpatchObject(ptr, parent);
    }
    else if (name == "scalar")
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (gobj->g_pd == scalar_class)
        {
            return new ScalarObject(ptr, parent);
        }
    }
    else if (!pd_checkobject(static_cast<t_pd*>(ptr)))
    {
        // Object is not a patcher object but something else
        return new NonPatchable(ptr, parent);
    }

    return new TextObject(ptr, parent);
}
