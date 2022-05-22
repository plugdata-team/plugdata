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

#include <memory>
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
#include "CommentObject.h"
#include "FloatAtomObject.h"
#include "SymbolAtomObject.h"
#include "DrawableTemplate.h"

ObjectBase::ObjectBase(void* obj, Box* parent) : ptr(obj), box(parent), cnv(box->cnv) {};

String ObjectBase::getText()
{
    char* text = nullptr;
    int size = 0;
    cnv->pd->setThis();

    libpd_get_object_text(ptr, &text, &size);
    if (text && size)
    {
        std::string txt(text, size);
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
        return String(txt);
    }
    
    return "";
}

GUIObject::GUIObject(void* obj, Box* parent) : ObjectBase(obj, parent), processor(*parent->cnv->pd), edited(false), type(getType(obj))
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
// TODO: Why not in constructor?
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

void GUIObject::paint(Graphics& g)
{
    getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(PlugDataColour::textColourId));


    // make sure text is readable
    getLookAndFeel().setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));
    getLookAndFeel().setColour(TextEditor::textColourId, box->findColour(PlugDataColour::textColourId));
    
    g.setColour(box->findColour(PlugDataColour::toolbarColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
}


ObjectParameters GUIObject::defineParameters()
{
    return {};
};

ObjectParameters GUIObject::getParameters()
{
    return defineParameters();
}


pd::Patch* GUIObject::getPatch()
{
    return nullptr;
}

Canvas* GUIObject::getCanvas()
{
    return nullptr;
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
                if(!thisPtr) return;
                
                float const v = thisPtr->getValue();
                if(thisPtr->value != v) {
                    MessageManager::callAsync(
                        [thisPtr, v]() mutable
                        {
                            if(thisPtr) {
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

    if (recursiveResize)
    {
        recursiveResize = false;
        return;  // break out of recursion: but doesn't protect against async recursion!!
    }
    else
    {
        recursiveResize = true;
        checkBoxBounds();
        recursiveResize = false;
    }
}


Type GUIObject::getType()
{
    return type;
}

void GUIObject::setValue(float value) noexcept
{
    cnv->pd->enqueueDirectMessages(ptr, value);
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void GUIObject::closeOpenedSubpatchers()
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
        tabbar->setTabBarDepth(30);
    }
    else
    {
        tabbar->getTabbedButtonBar().setVisible(false);
        tabbar->setTabBarDepth(1);
    }
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


Type GUIObject::getType(void* ptr) noexcept
{
    const String name = libpd_get_object_class_name(ptr);
    if (name == "bng")
    {
        return Type::Bang;
    }
    if (name == "hsl")
    {
        return Type::HorizontalSlider;
    }
    if (name == "vsl")
    {
        return Type::VerticalSlider;
    }
    if (name == "tgl")
    {
        return Type::Toggle;
    }
    if (name == "nbx")
    {
        return Type::Number;
    }
    if (name == "vradio")
    {
        return Type::VerticalRadio;
    }
    if (name == "hradio")
    {
        return Type::HorizontalRadio;
    }
    if (name == "cnv")
    {
        return Type::Panel;
    }
    if (name == "vu")
    {
        return Type::VuMeter;
    }
    if (name == "text")
    {
        auto* textObj = static_cast<t_text*>(ptr);
        if (textObj->te_type == T_OBJECT)
        {
            return Type::Invalid;
        }
        else
        {
            return Type::Comment;
        }
    }
    // Check size to prevent confusing it with else/message
    if (name == "message" && static_cast<t_gobj*>(ptr)->g_pd->c_size == sizeof(t_message))
    {
        return Type::Message;
    }
    else if (name == "pad")
    {
        return Type::Mousepad;
    }
    else if (name == "mouse")
    {
        return Type::Mouse;
    }
    else if (name == "keyboard")
    {
        return Type::Keyboard;
    }
    else if (name == "pic")
    {
        return Type::Picture;
    }

    else if (name == "gatom")
    {
        if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_FLOAT)
            return Type::AtomNumber;
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_SYMBOL)
            return Type::AtomSymbol;
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_NULL)
            return Type::AtomList;
    }
    else if (name == "canvas" || name == "graph")
    {
        if (static_cast<t_canvas*>(ptr)->gl_list)
        {
            t_class* c = static_cast<t_canvas*>(ptr)->gl_list->g_pd;
            if (c && c->c_name && (std::string(c->c_name->s_name) == std::string("array")))
            {
                return Type::Array;
            }
            else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
            {
                return Type::GraphOnParent;
            }
            else
            {  // abstraction or subpatch
                return Type::Subpatch;
            }
        }
        else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
        {
            return Type::GraphOnParent;
        }
        else
        {
            return Type::Subpatch;
        }
    }
    else if (name == "clone")
    {
        return Type::Clone;
    }
    else if (name == "pd")
    {
        return Type::Subpatch;
    }
    if (name == "scalar")
    {
        return Type::Scalar;
    }

    return Type::Text;
}



ObjectBase* GUIObject::createGui(void* ptr, Box* parent)
{
    auto type = getType(ptr);
    
    if (type == Type::Text)
    {
        return new TextObject(ptr, parent);
    }
    if (type == Type::Bang)
    {
        return new BangObject(ptr, parent);
    }
    if (type == Type::Toggle)
    {
        return new ToggleObject(ptr, parent);
    }
    if (type == Type::HorizontalSlider)
    {
        return new SliderObject(false, ptr, parent);
    }
    if (type == Type::VerticalSlider)
    {
        return new SliderObject(true, ptr, parent);
    }
    if (type == Type::HorizontalRadio)
    {
        return new RadioObject(false, ptr, parent);
    }
    if (type == Type::VerticalRadio)
    {
        return new RadioObject(true, ptr, parent);
    }
    if (type == Type::Message)
    {
        return new MessageObject(ptr, parent);
    }
    if (type == Type::Number)
    {
        return new NumberObject(ptr, parent);
    }
    if (type == Type::AtomList)
    {
        return new ListObject(ptr, parent);
    }
    if (type == Type::Array)
    {
        return new ArrayObject(ptr, parent);
    }
    if (type == Type::GraphOnParent)
    {
        return new GraphOnParent(ptr, parent);
    }
    if (type == Type::Subpatch || type == Type::Clone)
    {
        return new SubpatchObject(ptr, parent);
    }
    if (type == Type::VuMeter)
    {
        return new VUMeterObject(ptr, parent);
    }
    if (type == Type::Panel)
    {
        return new CanvasObject(ptr, parent);
    }
    if (type == Type::Comment)
    {
        return new CommentObject(ptr, parent);
    }
    if (type == Type::AtomNumber)
    {
        return new FloatAtomObject(ptr, parent);
    }
    if (type == Type::AtomSymbol)
    {
        return new SymbolAtomObject(ptr, parent);
    }
    if (type == Type::Mousepad)
    {
        return new MousePadObject(ptr, parent);
    }
    if (type == Type::Mouse)
    {
        return new MouseObject(ptr, parent);
    }
    if (type == Type::Keyboard)
    {
        return new KeyboardObject(ptr, parent);
    }
    if (type == Type::Picture)
    {
        return new PictureObject(ptr, parent);
    }
    if (type == Type::Scalar)
    {
        return new ScalarObject(ptr, parent);
    }

    
    return nullptr;
}
