/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "GUIObject.h"

extern "C" {
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <g_all_guis.h>
#include <g_undo.h>
}

#include "Object.h"
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
#include "ButtonObject.h"
#include "RadioObject.h"
#include "SliderObject.h"
#include "ArrayObject.h"
#include "GraphOnParent.h"
#include "KeyboardObject.h"
#include "KeyObject.h"
#include "MousePadObject.h"
#include "NumberObject.h"
#include "NumboxTildeObject.h"
#include "CanvasObject.h"
#include "PictureObject.h"
#include "VUMeterObject.h"
#include "ListObject.h"
#include "SubpatchObject.h"
#include "CloneObject.h"
#include "CommentObject.h"
#include "CycloneCommentObject.h"
#include "FloatAtomObject.h"
#include "SymbolAtomObject.h"
#include "ScalarObject.h"
#include "TextDefineObject.h"
#include "CanvasListenerObjects.h"
#include "ScopeObject.h"
#include "FunctionObject.h"

ObjectBase::ObjectBase(void* obj, Object* parent)
    : ptr(obj)
    , object(parent)
    , cnv(object->cnv)
    , pd(object->cnv->pd)
{
}

String ObjectBase::getText()
{
    if (!cnv->patch.checkObject(ptr))
        return "";

    char* text = nullptr;
    int size = 0;
    cnv->pd->setThis();

    libpd_get_object_text(ptr, &text, &size);
    if (text && size) {

        auto txt = String::fromUTF8(text, size);
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
        return txt;
    }

    return "";
}

String ObjectBase::getType() const
{
    ScopedLock lock(*pd->getCallbackLock());
    
    if (ptr) {
        if (pd_class(static_cast<t_pd*>(ptr)) == canvas_class && canvas_isabstraction((t_canvas*)ptr)) {
            char namebuf[MAXPDSTRING];
            t_object* ob = (t_object*)ptr;
            int ac = binbuf_getnatom(ob->te_binbuf);
            t_atom* av = binbuf_getvec(ob->te_binbuf);
            if (ac < 1)
                return String();
            atom_string(av, namebuf, MAXPDSTRING);

            return String::fromUTF8(namebuf).fromLastOccurrenceOf("/", false, false);
        }
        if(String(libpd_get_object_class_name(ptr)) == "text" && static_cast<t_text*>(ptr)->te_type == T_OBJECT)
        {
            return String("invalid");
        }
        if (auto* name = libpd_get_object_class_name(ptr)) {
            return String(name);
        }
    }
    
    sys_unlock();

    return {};
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void ObjectBase::closeOpenedSubpatchers()
{
    auto& main = object->cnv->main;
    auto* tabbar = &main.tabbar;

    if (!tabbar)
        return;

    for (int n = 0; n < tabbar->getNumTabs(); n++) {
        auto* cnv = main.getCanvas(n);
        if (cnv && cnv->patch == *getPatch()) {
            auto* deleted_patch = &cnv->patch;
            main.canvases.removeObject(cnv);
            tabbar->removeTab(n);
            main.pd.patches.removeObject(deleted_patch, false);

            break;
        }
    }
}

void ObjectBase::openSubpatch()
{
    auto* subpatch = getPatch();

    if (!subpatch)
        return;

    auto* glist = subpatch->getPointer();

    if (!glist)
        return;

    auto abstraction = canvas_isabstraction(glist);
    File path;

    if (abstraction) {
        path = File(String::fromUTF8(canvas_getdir(subpatch->getPointer())->s_name) + "/" + String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
    }

    for (int n = 0; n < cnv->main.tabbar.getNumTabs(); n++) {
        auto* tabCanvas = cnv->main.getCanvas(n);
        if (tabCanvas->patch == *subpatch) {
            cnv->main.tabbar.setCurrentTabIndex(n);
            return;
        }
    }

    auto* newPatch = cnv->main.pd.patches.add(new pd::Patch(*subpatch));
    auto* newCanvas = cnv->main.canvases.add(new Canvas(cnv->main, *newPatch, nullptr));

    newPatch->setCurrentFile(path);

    cnv->main.addTab(newCanvas);
    newCanvas->checkBounds();
}

void ObjectBase::moveToFront()
{
    auto glist_getindex = [](t_glist* x, t_gobj* y) {
        t_gobj* y2;
        int indx;
        for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
            indx++;
        return (indx);
    };

    auto glist_nth = [](t_glist* x, int n) -> t_gobj* {
        t_gobj* y;
        int indx;
        for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
            if (indx == n)
                return (y);

        jassertfalse;
        return nullptr;
    };

    auto* canvas = static_cast<t_canvas*>(cnv->patch.getPointer());
    t_gobj* y = static_cast<t_gobj*>(ptr);

    t_gobj *y_prev = nullptr, *y_next = nullptr;

    /* if there is an object before ours (in other words our index is > 0) */
    if (int idx = glist_getindex(canvas, y))
        y_prev = glist_nth(canvas, idx - 1);

    /* if there is an object after ours */
    if (y->g_next)
        y_next = y->g_next;

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

void ObjectBase::moveToBack()
{
    auto glist_getindex = [](t_glist* x, t_gobj* y) {
        t_gobj* y2;
        int indx;
        for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
            indx++;
        return (indx);
    };

    auto glist_nth = [](t_glist* x, int n) -> t_gobj* {
        t_gobj* y;
        int indx;
        for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
            if (indx == n)
                return (y);

        jassertfalse;
        return nullptr;
    };

    auto* canvas = static_cast<t_canvas*>(cnv->patch.getPointer());
    t_gobj* y = static_cast<t_gobj*>(ptr);

    t_gobj *y_prev = nullptr, *y_next = nullptr;

    /* if there is an object before ours (in other words our index is > 0) */
    if (int idx = glist_getindex(canvas, y))
        y_prev = glist_nth(canvas, idx - 1);

    /* if there is an object after ours */
    if (y->g_next)
        y_next = y->g_next;

    t_gobj* y_start = canvas->gl_list;

    canvas->gl_list = y;
    y->g_next = y_start;
    
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
    getLookAndFeel().setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
    getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
    getLookAndFeel().setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));

    g.setColour(object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

    bool selected = cnv->isSelected(object) && !cnv->isGraph;
    auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);
    
    g.setColour(outlineColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 3.0f, 1.0f);
}

NonPatchable::NonPatchable(void* obj, Object* parent)
    : ObjectBase(obj, parent)
{
    // Make object invisible
    object->setVisible(false);
}

NonPatchable::~NonPatchable()
{
}

struct Lambda {
    template<typename Tret, typename T>
    static Tret lambda_ptr_exec(void* data) {
        return (Tret) (*(T*)fn<T>())(data);
    }

    template<typename Tret = void, typename Tfp = Tret(*)(void*), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, T>;
    }

    template<typename T>
    static void* fn(void* new_fn = nullptr) {
        static void* fn;
        if (new_fn != nullptr)
            fn = new_fn;
        return fn;
    }
};



GUIObject::GUIObject(void* obj, Object* parent)
    : ObjectBase(obj, parent)
    , processor(*parent->cnv->pd)
    , edited(false)
{
    object->addComponentListener(this);
    updateLabel(); // TODO: fix virtual call from constructor

    setWantsKeyboardFocus(true);

    setLookAndFeel(dynamic_cast<PlugDataLook*>(&LookAndFeel::getDefaultLookAndFeel())->getPdLook());

    MessageManager::callAsync([_this = SafePointer<GUIObject>(this)] {
        if (_this) {
            _this->updateParameters();
        }
    });
    
    // TODO: enable this for v0.6.3
    //pd->registerMessageListener(ptr, this);
}

GUIObject::~GUIObject()
{
    //pd->unregisterMessageListener(ptr, this);
    object->removeComponentListener(this);
    auto* lnf = &getLookAndFeel();
    setLookAndFeel(nullptr);
    delete lnf;
}

void GUIObject::updateParameters()
{
    getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(Label::textWhenEditingColourId));
    getLookAndFeel().setColour(Label::textColourId, object->findColour(Label::textColourId));

    auto params = getParameters();
    for (auto& [name, type, cat, value, list] : params) {
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

float GUIObject::getValueOriginal() const
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

float GUIObject::getValueScaled() const
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

void GUIObject::startEdition()
{
    edited = true;
    processor.enqueueMessages("gui", "mouse", { 1.f });

    value = getValue();
}

void GUIObject::stopEdition()
{
    edited = false;
    processor.enqueueMessages("gui", "mouse", { 0.f });
}

void GUIObject::updateValue()
{
    if (!edited) {
        pd->enqueueFunction(
            [_this = SafePointer(this)]() {
                if (!_this)
                    return;

                float const v = _this->getValue();
                if (_this->value != v) {
                    MessageManager::callAsync(
                        [_this, v]() mutable {
                            if (_this) {
                                _this->value = v;
                                _this->update();
                            }
                        });
                }
            });
    }
}

void GUIObject::componentMovedOrResized(Component& component, bool moved, bool resized)
{
    updateLabel();

    if (!resized)
        return;

    checkBounds();
}

void GUIObject::setValue(float value)
{
    cnv->pd->enqueueDirectMessages(ptr, value);
}

ObjectBase* GUIObject::createGui(void* ptr, Object* parent)
{
    const String name = libpd_get_object_class_name(ptr);
    if (name == "bng") {
        return new BangObject(ptr, parent);
    }
    if (name == "button") {
        return new ButtonObject(ptr, parent);
    }
    if (name == "hsl") {
        return new SliderObject(false, ptr, parent);
    }
    if (name == "vsl") {
        return new SliderObject(true, ptr, parent);
    }
    if (name == "tgl") {
        return new ToggleObject(ptr, parent);
    }
    if (name == "nbx") {
        return new NumberObject(ptr, parent);
    }
    if (name == "numbox~") {
        return new NumboxTildeObject(ptr, parent);
    }
    if (name == "vradio") {
        return new RadioObject(true, ptr, parent);
    }
    if (name == "hradio") {
        return new RadioObject(false, ptr, parent);
    }
    if (name == "cnv") {
        return new CanvasObject(ptr, parent);
    }
    if (name == "vu") {
        return new VUMeterObject(ptr, parent);
    }
    if (name == "text") {
        auto* textObj = static_cast<t_text*>(ptr);
        if (textObj->te_type == T_OBJECT) {
            return new TextObject(ptr, parent, false);
        } else {
            return new CommentObject(ptr, parent);
        }
    }
    if (name == "comment") {
        return new CycloneCommentObject(ptr, parent);
    }
    // Check size to prevent confusing it with else/message
    if (name == "message" && static_cast<t_gobj*>(ptr)->g_pd->c_size == sizeof(t_message)) {
        return new MessageObject(ptr, parent);
    } else if (name == "pad") {
        return new MousePadObject(ptr, parent);
    } else if (name == "mouse") {
        return new MouseObject(ptr, parent);
    } else if (name == "keyboard") {
        return new KeyboardObject(ptr, parent);
    } else if (name == "pic") {
        return new PictureObject(ptr, parent);
    } else if (name == "text define") {
        return new TextDefineObject(ptr, parent);
    } else if (name == "gatom") {
        if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_FLOAT)
            return new FloatAtomObject(ptr, parent);
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_SYMBOL)
            return new SymbolAtomObject(ptr, parent);
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_NULL)
            return new ListObject(ptr, parent);
    } else if (name == "canvas" || name == "graph") {
        if (static_cast<t_canvas*>(ptr)->gl_list) {
            t_class* c = static_cast<t_canvas*>(ptr)->gl_list->g_pd;
            if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                return new ArrayObject(ptr, parent);
            } else if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
                return new GraphOnParent(ptr, parent);
            } else { // abstraction or subpatch
                return new SubpatchObject(ptr, parent);
            }
        } else if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            return new GraphOnParent(ptr, parent);
        } else {
            return new SubpatchObject(ptr, parent);
        }
    } else if (name == "array define") {
        return new ArrayDefineObject(ptr, parent);
    } else if (name == "clone") {
        return new CloneObject(ptr, parent);
    } else if (name == "pd") {
        return new SubpatchObject(ptr, parent);
    } else if (name == "scalar") {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (gobj->g_pd == scalar_class) {
            return new ScalarObject(ptr, parent);
        }
    } else if (name == "key") {
        return new KeyObject(ptr, parent, KeyObject::Key);
    } else if (name == "keyname") {
        return new KeyObject(ptr, parent, KeyObject::KeyName);
    }
    else if (name == "keyup") {
        return new KeyObject(ptr, parent, KeyObject::KeyUp);
    }
    // ELSE's [oscope~] and cyclone [scope~] are basically the same object
    else if (name == "oscope~" || name == "scope~") {
        return new ScopeObject(ptr, parent);
    }
    else if (name == "function") {
        return new FunctionObject(ptr, parent);
    }
    else if (name == "canvas.active") {
        return new CanvasActiveObject(ptr, parent);
    }
    else if (name == "canvas.mouse") {
        return new CanvasMouseObject(ptr, parent);
    }
    else if (name == "canvas.vis") {
        return new CanvasVisibleObject(ptr, parent);
    }
    else if (name == "canvas.zoom") {
        return new CanvasZoomObject(ptr, parent);
    }
    else if (name == "canvas.edit") {
        return new CanvasEditObject(ptr, parent);
    }
    else if (!pd_checkobject(static_cast<t_pd*>(ptr))) {
        // Object is not a patcher object but something else
        return new NonPatchable(ptr, parent);
    }
    

    return new TextObject(ptr, parent);
}
