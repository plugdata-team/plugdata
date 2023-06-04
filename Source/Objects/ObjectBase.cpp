/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "ObjectBase.h"

extern "C" {

#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <g_all_guis.h>
#include <g_undo.h>

void canvas_setgraph(t_glist* x, int flag, int nogoprect);
}

#include "AllGuis.h"
#include "Object.h"
#include "Iolet.h"
#include "Canvas.h"
#include "Tabbar.h"
#include "SuggestionComponent.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Pd/Patch.h"
#include "Sidebar/Sidebar.h"

#include "IEMHelper.h"
#include "AtomHelper.h"

#include "TextObject.h"
#include "ToggleObject.h"
#include "MessageObject.h"
#include "BangObject.h"
#include "ButtonObject.h"
#include "RadioObject.h"
#include "SliderObject.h"
#include "KnobObject.h"
#include "ArrayObject.h"
#include "GraphOnParent.h"
#include "KeyboardObject.h"
#include "MessboxObject.h"
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
#include "ScopeObject.h"
#include "FunctionObject.h"
#include "BicoeffObject.h"
#include "NoteObject.h"
#include "ColourPickerObject.h"

// Class for non-patchable objects
class NonPatchable : public ObjectBase {

public:
    NonPatchable(void* obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        parent->setVisible(false);
    }

    Rectangle<int> getPdBounds() override { return { 0, 0, 1, 1 }; };
    void setPdBounds(Rectangle<int> newBounds) override {};
};

void ObjectLabel::ObjectListener::componentMovedOrResized(Component& component, bool moved, bool resized)
{

    dynamic_cast<Object&>(component).gui->updateLabel();
}

ObjectBase::ObjectBase(void* obj, Object* parent)
    : ptr(obj, parent->cnv->pd)
    , object(parent)
    , cnv(parent->cnv)
    , pd(parent->cnv->pd)
{
    pd->registerMessageListener(ptr.getRawUnchecked<void>(), this);

    setWantsKeyboardFocus(true);

    setLookAndFeel(new PlugDataLook());

    MessageManager::callAsync([_this = SafePointer(this)] {
        if (_this) {
            _this->updateLabel();
            _this->constrainer = _this->createConstrainer();
            _this->onConstrainerCreate();

            for (auto& [name, type, cat, value, list, valueDefault] : _this->objectParameters.getParameters()) {
                value->addListener(_this.getComponent());
            }
        }
    });
}

ObjectBase::~ObjectBase()
{
    pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);

    auto* lnf = &getLookAndFeel();
    setLookAndFeel(nullptr);
    delete lnf;
}

String ObjectBase::getText()
{
    char* text = nullptr;
    int size = 0;

    if (auto obj = ptr.get<t_gobj>()) {
        if (!cnv->patch.checkObject(obj.get()))
            return "";

        libpd_get_object_text(obj.get(), &text, &size);
    }

    if (text && size) {

        auto txt = String::fromUTF8(text, size);
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
        return txt;
    }

    return "";
}

String ObjectBase::getType() const
{
    if (auto obj = ptr.get<t_pd>()) {
        // Check if it's an abstraction or subpatch
        if (pd_class(obj.get()) == canvas_class && canvas_isabstraction(obj.cast<t_glist>())) {
            char namebuf[MAXPDSTRING];
            auto* ob = obj.cast<t_object>();
            int ac = binbuf_getnatom(ob->te_binbuf);
            t_atom* av = binbuf_getvec(ob->te_binbuf);
            if (ac < 1)
                return {};
            atom_string(av, namebuf, MAXPDSTRING);

            return String::fromUTF8(namebuf).fromLastOccurrenceOf("/", false, false);
        }
        // Deal with different text objects
        switch (hash(libpd_get_object_class_name(obj.get()))) {
        case hash("text"):
            if (ptr.get<t_text>()->te_type == T_OBJECT)
                return "invalid";
            if (ptr.get<t_text>()->te_type == T_TEXT)
                return "comment";
            if (ptr.get<t_text>()->te_type == T_MESSAGE)
                return "message";
            break;
        // Deal with atoms
        case hash("gatom"):
            if (ptr.get<t_fake_gatom>()->a_flavor == A_FLOAT)
                return "floatbox";
            if (ptr.get<t_fake_gatom>()->a_flavor == A_SYMBOL)
                return "symbolbox";
            if (ptr.get<t_fake_gatom>()->a_flavor == A_NULL)
                return "listbox";
            break;
        default:
            break;
        }
        // Get class name for all other objects
        if (auto* name = libpd_get_object_class_name(obj.get())) {
            return String::fromUTF8(name);
        }
    }

    return {};
}

// Make sure the object can't be triggered if that palette is in drag mode
bool ObjectBase::hitTest(int x, int y)
{
    return Component::hitTest(x, y);
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void ObjectBase::closeOpenedSubpatchers()
{
    auto* editor = object->cnv->editor;

    for (auto* canvas : editor->canvases) {
        auto* patch = getPatch().get();
        if (patch && canvas && canvas->patch == *patch) {

            canvas->editor->closeTab(canvas);
            break;
        }
    }
}

bool ObjectBase::click()
{
    if (auto obj = ptr.get<t_object>()) {
        if (libpd_has_click_function(obj.get())) {
            pd->sendDirectMessage(obj.get(), "click", {});
            return true;
        }
    }

    return false;
}

void ObjectBase::openSubpatch()
{
    auto subpatch = getPatch();

    if (!subpatch)
        return;

    auto* glist = subpatch->getPointer().get();

    if (!glist)
        return;

    auto abstraction = canvas_isabstraction(glist);
    File path;

    if (abstraction) {
        path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
    }

    // Check if subpatch is already opened
    for (auto* cnv : cnv->editor->canvases) {
        if (cnv->patch == *subpatch) {
            auto* tabbar = cnv->getTabbar();
            tabbar->setCurrentTabIndex(cnv->getTabIndex());
            return;
        }
    }

    cnv->editor->pd->patches.add(subpatch);
    auto newPatch = cnv->editor->pd->patches.getLast();
    auto* newCanvas = cnv->editor->canvases.add(new Canvas(cnv->editor, *newPatch, nullptr));

    newPatch->setCurrentFile(path);

    cnv->editor->addTab(newCanvas);
}

void ObjectBase::moveToFront()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        libpd_tofront(patch, obj.get());
    }
}

void ObjectBase::moveToBack()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        libpd_toback(patch, obj.get());
    }
}

void ObjectBase::paint(Graphics& g)
{
    g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

    bool selected = object->isSelected() && !cnv->isGraph;
    auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

    g.setColour(outlineColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
}

ObjectParameters ObjectBase::getParameters()
{
    return objectParameters;
}

void ObjectBase::startEdition()
{
    if (edited)
        return;

    edited = true;
    pd->sendMessage("gui", "mouse", { 1.f });
}

void ObjectBase::stopEdition()
{
    if (!edited)
        return;

    edited = false;
    pd->sendMessage("gui", "mouse", { 0.f });
}

void ObjectBase::sendFloatValue(float newValue)
{
    t_atom atom;
    SETFLOAT(&atom, newValue);

    if (auto obj = ptr.get<t_pd>()) {
        pd_typedmess(obj.get(), cnv->patch.instance->generateSymbol("set"), 1, &atom);
        pd_bang(obj.get());
    }
}

ObjectBase* ObjectBase::createGui(void* ptr, Object* parent)
{
    parent->cnv->pd->setThis();

    auto const name = hash(libpd_get_object_class_name(ptr));

    // check if object is a patcher object, or something else
    if (!pd_checkobject(static_cast<t_pd*>(ptr)) && name != hash("scalar")) {
        return new NonPatchable(ptr, parent);
    } else {
        switch (name) {
        case hash("bng"):
            return new BangObject(ptr, parent);
        case hash("button"):
            return new ButtonObject(ptr, parent);
        case hash("hsl"):
        case hash("vsl"):
        case hash("slider"):
            return new SliderObject(ptr, parent);
        case hash("tgl"):
            return new ToggleObject(ptr, parent);
        case hash("nbx"):
            return new NumberObject(ptr, parent);
        case hash("numbox~"):
            return new NumboxTildeObject(ptr, parent);
        case hash("vradio"):
        case hash("hradio"):
            return new RadioObject(ptr, parent);
        case hash("cnv"):
            return new CanvasObject(ptr, parent);
        case hash("vu"):
            return new VUMeterObject(ptr, parent);
        case hash("text"): {
            auto* textObj = static_cast<t_text*>(ptr);
            if (textObj->te_type == T_OBJECT) {
                return new TextObject(ptr, parent, false);
            } else {
                return new CommentObject(ptr, parent);
            }
        }
        case hash("comment"):
            return new CycloneCommentObject(ptr, parent);
        // Check if message type text object to prevent confusing it with else/message
        case hash("message"): {
            if (libpd_is_text_object(ptr) && static_cast<t_text*>(ptr)->te_type == T_MESSAGE) {
                return new MessageObject(ptr, parent);
            }
            break;
        }
        case hash("pad"):
            return new MousePadObject(ptr, parent);
        case hash("keyboard"):
            return new KeyboardObject(ptr, parent);
        case hash("pic"):
            return new PictureObject(ptr, parent);
        case hash("text define"):
            return new TextDefineObject(ptr, parent);
        case hash("textfile"):
        case hash("qlist"):
            return new TextFileObject(ptr, parent);
        case hash("gatom"): {
            if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_FLOAT) {
                return new FloatAtomObject(ptr, parent);
            } else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_SYMBOL) {
                return new SymbolAtomObject(ptr, parent);
            } else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_NULL) {
                return new ListObject(ptr, parent);
            }
            break;
        }
        case hash("canvas"):
        case hash("graph"): {
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
        }
        case hash("array define"):
            return new ArrayDefineObject(ptr, parent);
        case hash("clone"):
            return new CloneObject(ptr, parent);
        case hash("pd"):
            return new SubpatchObject(ptr, parent);
        case hash("scalar"): {
            auto* gobj = static_cast<t_gobj*>(ptr);
            if (gobj->g_pd == scalar_class) {
                return new ScalarObject(ptr, parent);
            }
            break;
        }
        case hash("colors"):
            return new ColourPickerObject(ptr, parent);
        case hash("oscope~"):
            return new OscopeObject(ptr, parent);
        case hash("scope~"):
            return new ScopeObject(ptr, parent);
        case hash("function"):
            return new FunctionObject(ptr, parent);
        case hash("bicoeff"):
            return new BicoeffObject(ptr, parent);
        case hash("messbox"):
            return new MessboxObject(ptr, parent);
        case hash("note"):
            return new NoteObject(ptr, parent);
        case hash("knob"):
            return new KnobObject(ptr, parent);
        default:
            break;
        }
    }
    return new TextObject(ptr, parent);
}

bool ObjectBase::canOpenFromMenu()
{
    if (auto obj = ptr.get<t_pd>()) {
        return zgetfn(obj.get(), pd->generateSymbol("menu-open")) != nullptr;
    }

    return false;
}

void ObjectBase::openFromMenu()
{
    if (auto obj = ptr.get<t_pd>()) {
        pd->sendDirectMessage(obj.get(), "menu-open", {});
    }
}

bool ObjectBase::hideInGraph()
{
    return false;
}

void ObjectBase::lock(bool isLocked)
{
    setInterceptsMouseClicks(isLocked, isLocked);
}

Canvas* ObjectBase::getCanvas()
{
    return nullptr;
}

pd::Patch::Ptr ObjectBase::getPatch()
{
    return nullptr;
}

bool ObjectBase::canReceiveMouseEvent(int x, int y)
{
    return true;
}

void ObjectBase::receiveMessage(String const& symbol, int argc, t_atom* argv)
{
    auto sym = hash(symbol);

    switch (sym) {
    case hash("size"):
    case hash("delta"):
    case hash("pos"):
    case hash("dim"):
    case hash("width"):
    case hash("height"): {
        MessageManager::callAsync([_this = SafePointer(this)]() {
            if (_this)
                _this->object->updateBounds();
        });
        return;
    }
    default:
        break;
    }

    auto messages = getAllMessages();
    if (std::find(messages.begin(), messages.end(), hash("anything")) != messages.end() || std::find(messages.begin(), messages.end(), sym) != messages.end()) {

        auto atoms = pd::Atom::fromAtoms(argc, argv);

        MessageManager::callAsync([_this = SafePointer(this), symbol, atoms]() mutable {
            if (_this)
                _this->receiveObjectMessage(symbol, atoms);
        });
    }
}

void ObjectBase::setParameterExcludingListener(Value& parameter, var const& value)
{
    parameter.removeListener(this);
    parameter.setValue(value);
    parameter.addListener(this);
}

ObjectLabel* ObjectBase::getLabel()
{
    return label.get();
}
bool ObjectBase::isBeingEdited()
{
    return edited;
}

ComponentBoundsConstrainer* ObjectBase::getConstrainer()
{
    return constrainer.get();
}

std::unique_ptr<ComponentBoundsConstrainer> ObjectBase::createConstrainer()
{
    class ObjectBoundsConstrainer : public ComponentBoundsConstrainer {
    public:
        ObjectBoundsConstrainer()
        {
            setMinimumSize(Object::minimumSize, Object::minimumSize);
        }
        /*
         * Custom version of checkBounds that takes into consideration
         * the padding around plugdata node objects when resizing
         * to allow the aspect ratio to be interpreted correctly.
         * Otherwise resizing objects with an aspect ratio will
         * resize the object size **including** margins, and not follow the
         * actual size of the visible object
         */
        void checkBounds(Rectangle<int>& bounds,
            Rectangle<int> const& old,
            Rectangle<int> const& limits,
            bool isStretchingTop,
            bool isStretchingLeft,
            bool isStretchingBottom,
            bool isStretchingRight) override
        {
            // we remove the margin from the resizing object
            BorderSize<int> border(Object::margin);
            border.subtractFrom(bounds);

            // we also have to remove the margin from the old object, but don't alter the old object
            ComponentBoundsConstrainer::checkBounds(bounds, border.subtractedFrom(old), limits, isStretchingTop,
                isStretchingLeft,
                isStretchingBottom,
                isStretchingRight);

            // put back the margins
            border.addTo(bounds);

            // If we're stretching in only one direction, make sure to keep the position on the other axis the same.
            // This prevents ice-skating when the canvas is zoomed in
            auto isStretchingWidth = isStretchingLeft || isStretchingRight;
            auto isStretchingHeight = isStretchingBottom || isStretchingTop;

            if (getFixedAspectRatio() != 0.0f && (isStretchingWidth ^ isStretchingHeight)) {

                bounds = isStretchingHeight ? bounds.withX(old.getX()) : bounds.withY(old.getY());
            }
        }
    };

    return std::make_unique<ObjectBoundsConstrainer>();
}
