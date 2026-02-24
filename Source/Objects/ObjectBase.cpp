/*
 // Copyright (c) 2021-2025 Timothy Schoen and Pierre Guillot
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

#if _MSC_VER
__declspec(dllimport) void canvas_setgraph(t_glist* x, int flag, int nogoprect);
__declspec(dllimport) void canvas_click(t_canvas* x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt);
#else
void canvas_setgraph(t_glist* x, int flag, int nogoprect);
void canvas_click(t_canvas* x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt);
#endif
}

#include "AllGuis.h"
#include "Object.h"
#include "Iolet.h"
#include "Canvas.h"
#include "Components/SuggestionComponent.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "TabComponent.h"
#include "Pd/Patch.h"
#include "Sidebar/Sidebar.h"
#include "Utility/CachedTextRender.h"
#include "Utility/CachedStringWidth.h"

#include "Dialogs/Dialogs.h"

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
#include "GraphOnParent.h"
#include "ArrayObject.h"
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
#include "FloatAtomObject.h"
#include "SymbolAtomObject.h"
#include "ScalarObject.h"
#include "ScopeObject.h"
#include "FunctionObject.h"
#include "BicoeffObject.h"
#include "NoteObject.h"
#include "ColourPickerObject.h"
#include "MidiObjects.h"
#include "OpenFileObject.h"
#include "PdTildeObject.h"
#include "PopMenu.h"
#include "LuaObject.h"
#include "DropzoneObject.h"

// Class for non-patchable objects
class NonPatchable final : public ObjectBase {

public:
    NonPatchable(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        parent->setVisible(false);
    }

    Rectangle<int> getPdBounds() override { return { 0, 0, 1, 1 }; }
    void setPdBounds(Rectangle<int> newBounds) override { }
};

ObjectBase::ObjectSizeListener::ObjectSizeListener(Object* obj)
    : object(obj)
{
    lastChange = 0;
}

void ObjectBase::ObjectSizeListener::componentMovedOrResized(Component& component, bool moved, bool const resized)
{
    dynamic_cast<Object&>(component).gui->objectMovedOrResized(resized);
}

void ObjectBase::ObjectSizeListener::valueChanged(Value& v)
{
    if (auto obj = object->gui->ptr.get<t_gobj>()) {
        auto* patch = object->cnv->patch.getRawPointer();

        auto const x = static_cast<float>(v.getValue().getArray()->getReference(0));
        auto const y = static_cast<float>(v.getValue().getArray()->getReference(1));

        if (Time::getMillisecondCounter() - lastChange > 6000) {
            pd::Interface::undoApply(patch, obj.get());
        }

        lastChange = Time::getMillisecondCounter();

        pd::Interface::moveObject(patch, obj.get(), x, y);
        object->updateBounds();
    }
}

ObjectBase::PropertyListener::PropertyListener(ObjectBase* p)
{
    lastChange = Time::getMillisecondCounter();
    parent = p;
    noCallback = false;
}

void ObjectBase::PropertyListener::setNoCallback(bool const skipCallback)
{
    noCallback = skipCallback;
}

void ObjectBase::PropertyListener::valueChanged(Value& v)
{
    if (noCallback)
        return;

    if (!v.refersToSameSourceAs(lastValue) || Time::getMillisecondCounter() - lastChange > 6000) {
        onChange();
        lastValue.referTo(v);
    }

    lastChange = Time::getMillisecondCounter();
    parent->propertyChanged(v);
}

ObjectBase::ObjectBase(pd::WeakReference obj, Object* parent)
    : NVGComponent(this)
    , ptr(obj)
    , object(parent)
    , cnv(parent->cnv)
    , pd(parent->cnv->pd)
    , propertyListener(this)
    , objectSizeListener(parent)
{
    setType();

    // Perform async, so that we don't get a size change callback for initial creation
    MessageManager::callAsync([_this = SafePointer(this)] {
        if (!_this)
            return;
        _this->object->addComponentListener(&_this->objectSizeListener);
        _this->updateLabel();

        auto const objectBounds = _this->object->getObjectBounds();
        _this->positionParameter = VarArray { var(objectBounds.getX()), var(objectBounds.getY()) };
        _this->objectParameters.addParamPosition(&_this->positionParameter);
        _this->positionParameter.addListener(&_this->objectSizeListener);
    });

    setWantsKeyboardFocus(true);

    propertyListener.onChange = [_this = SafePointer(this)] {
        if (!_this)
            return;

        if (auto obj = _this->ptr.get<t_gobj>()) {
            auto* canvas = _this->cnv->patch.getRawPointer();
            pd::Interface::undoApply(canvas, obj.get());
            canvas_dirty(canvas, 1);
        }
    };
}

ObjectBase::~ObjectBase()
{
    pd->unregisterMessageListener(this);
    object->removeComponentListener(&objectSizeListener);
}

void ObjectBase::initialise()
{
    updateProperties();
    
    constrainer = createConstrainer();
    onConstrainerCreate();

    pd->registerMessageListener(ptr.getRawUnchecked<void>(), this);

    for (auto const& param : objectParameters.getParameters()) {
        if (param.valuePtr) {
            param.valuePtr->addListener(&propertyListener);
        }
    }
}

void ObjectBase::objectMovedOrResized(bool const resized)
{
    auto const objectBounds = object->getObjectBounds();

    setParameterExcludingListener(positionParameter, VarArray { var(objectBounds.getX()), var(objectBounds.getY()) }, &objectSizeListener);

    if (resized)
        updateSizeProperty();

    updateLabel();
}

String ObjectBase::getText()
{
    char* text = nullptr;
    int size = 0;

    if (auto obj = ptr.get<t_gobj>()) {

        if (!pd::Interface::checkObject(obj.get()))
            return "";

        pd::Interface::getObjectText(obj.cast<t_object>(), &text, &size);
    }

    if (text && size) {

        auto txt = String::fromUTF8(text, size);
        freebytes(text, static_cast<size_t>(size) * sizeof(char));
        return txt;
    }

    return "";
}

bool ObjectBase::checkHvccCompatibility()
{
    auto type = getType();

    if (type == "msg") // Prevent mixing up pd message and else/message
    {
        if (auto* objectPtr = ptr.getRaw<t_gobj>()) {
            auto const origin = pd::Library::getObjectOrigin(objectPtr);
            if (origin == "ELSE") {
                pd->logWarning(String("Warning: object message is not supported in Compiled Mode").toRawUTF8());
                return false;
            }
        }
        return true;
    } else if (HeavyCompatibleObjects::isCompatible(type)) {
        return true;
    } else {
        pd->logWarning(String("Warning: object \"" + getType() + "\" is not supported in Compiled Mode").toRawUTF8());
        return false;
    }
}

String ObjectBase::getTypeWithOriginPrefix() const
{
    auto type = getType();
    if (type.contains("/"))
        return type;

    if (auto* objectPtr = ptr.getRaw<t_gobj>()) {
        auto const origin = pd::Library::getObjectOrigin(objectPtr);

        if (origin == "ELSE" && type == "msg") {
            return "ELSE/message";
        }

        if (origin.isEmpty())
            return type;

        return origin + "/" + type;
    }
    return {};
}

String ObjectBase::getType() const
{
    return type;
}

void ObjectBase::setType()
{
    auto getObjectType = [this]() -> String {
        if (auto obj = ptr.get<t_pd>()) {
            // Check if it's an abstraction or subpatch
            if (pd_class(obj.get()) == canvas_class && canvas_isabstraction(obj.cast<t_glist>())) {
                char namebuf[MAXPDSTRING];
                auto const* ob = obj.cast<t_object>();
                int const ac = binbuf_getnatom(ob->te_binbuf);
                t_atom const* av = binbuf_getvec(ob->te_binbuf);
                if (ac < 1)
                    return {};
                atom_string(av, namebuf, MAXPDSTRING);

                return String::fromUTF8(namebuf).fromLastOccurrenceOf("/", false, false);
            }

            auto* className = pd::Interface::getObjectClassName(obj.get());
            if (!className)
                return {};

            // Deal with different text objects
            switch (hash(className)) {
            case hash("message"):
                return "msg";
            case hash("text"):
                if (obj.cast<t_text>()->te_type == T_OBJECT)
                    return "invalid";
                if (obj.cast<t_text>()->te_type == T_TEXT)
                    return "comment";
                if (obj.cast<t_text>()->te_type == T_MESSAGE)
                    return "msg";
                break;
            // Deal with atoms
            case hash("gatom"):
                if (obj.cast<t_fake_gatom>()->a_flavor == A_FLOAT)
                    return "floatbox";
                if (obj.cast<t_fake_gatom>()->a_flavor == A_SYMBOL)
                    return "symbolbox";
                if (obj.cast<t_fake_gatom>()->a_flavor == A_NULL)
                    return "listbox";
                break;
            case hash("hsl"):
                if (obj.cast<t_slider>()->x_orientation)
                    return "vsl";
                else
                    return "hsl";
            case hash("hradio"):
                if (obj.cast<t_radio>()->x_orientation)
                    return "vradio";
                else
                    return "hradio";
            default:
                break;
            }

            return String::fromUTF8(className);
        }
        return {};
    };

    type = getObjectType();
}

// Gets position from pd and applies it to Object
Rectangle<int> ObjectBase::getSelectableBounds()
{
    return object->getBounds().reduced(Object::margin) - cnv->canvasOrigin;
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void ObjectBase::closeOpenedSubpatchers()
{
    for (auto* editor : pd->getEditors()) {
        for (auto* canvas : editor->getCanvases()) {
            auto const* patch = getPatch().get();
            if (patch && canvas && canvas->patch == *patch) {

                canvas->editor->getTabComponent().closeTab(canvas);
                break;
            }
        }
    }
}

bool ObjectBase::click(Point<int> const position, bool const shift, bool const alt)
{
    if (auto obj = ptr.get<t_text>()) {

        t_text* x = obj.get();
        if (x->te_type == T_OBJECT) {
            t_symbol* clicksym = gensym("click");
            auto const click_func = zgetfn(&x->te_pd, clicksym);

            // Check if a click function has been registered, and if it's not the default canvas click function (in which case we want to handle it manually)
            if (click_func && reinterpret_cast<void*>(click_func) != reinterpret_cast<void*>(canvas_click)) {
                pd_vmess(&x->te_pd, clicksym, "fffff",
                    static_cast<double>(position.x), static_cast<double>(position.y),
                    static_cast<double>(shift), static_cast<double>(0), static_cast<double>(alt));

                return true;
            } else
                return false;
        }
    }

    return false;
}

void ObjectBase::openSubpatch()
{
    auto const subpatch = getPatch();

    if (!subpatch)
        return;

    auto const* glist = subpatch->getPointer().get();

    if (!glist)
        return;

    auto const abstraction = canvas_isabstraction(glist);
    File path;

    if (abstraction) {
        path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
    }

    cnv->editor->getTabComponent().openPatch(subpatch);

    if (path.getFullPathName().isNotEmpty()) {
        subpatch->setCurrentFile(URL(path));
    }
}

void ObjectBase::moveToFront()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getRawPointer();
        pd::Interface::toFront(patch, obj.get());
    }
}

void ObjectBase::moveForward()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getRawPointer();
        pd::Interface::moveForward(patch, obj.get());
    }
}

void ObjectBase::moveBackward()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getRawPointer();
        if (!patch)
            return;

        pd::Interface::moveBackward(patch, obj.get());
    }
}

void ObjectBase::moveToBack()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getRawPointer();
        if (!patch)
            return;

        pd::Interface::toBack(patch, obj.get());
    }
}

void ObjectBase::paint(Graphics& g)
{
    g.setColour(PlugDataColours::guiObjectBackgroundColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

    bool const selected = object->isSelected() && !cnv->isGraph;
    auto const outlineColour = selected ? PlugDataColours::objectSelectedOutlineColour : PlugDataColours::objectOutlineColour;

    g.setColour(outlineColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
}

float ObjectBase::getImageScale()
{
    auto const* topLevel = cnv;
    if (!hideInGraph()) { // No need to do this if we can't be visible in a graph anyway!
        while (auto const* nextCnv = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCnv;
        }
    }
    if (topLevel->editor->pluginMode) {
        auto const scale = std::sqrt(std::abs(topLevel->getTransform().getDeterminant()));
        return object->editor->getRenderScale() * scale;
    }

    return object->editor->getRenderScale() * getValue<float>(topLevel->zoomScale);
}

ObjectParameters ObjectBase::getParameters()
{
    return objectParameters;
}

bool ObjectBase::showParametersWhenSelected()
{
    return true;
}

void ObjectBase::startEdition()
{
    if (edited)
        return;

    edited = true;
    if (auto lockedPtr = ptr.get<void>()) {
        pd->sendMessage("gui", "mouse", { 1.f });
    }
}

void ObjectBase::stopEdition()
{
    if (!edited)
        return;

    edited = false;
    if (auto lockedPtr = ptr.get<void>()) {
        pd->sendMessage("gui", "mouse", { 0.f });
    }
}

void ObjectBase::sendFloatValue(float const newValue)
{
    t_atom atom;
    SETFLOAT(&atom, newValue);

    if (auto obj = ptr.get<t_pd>()) {
        pd_typedmess(obj.get(), cnv->patch.instance->generateSymbol("set"), 1, &atom);
        pd_bang(obj.get());
    }
}


ObjectBase* ObjectBase::createGui(pd::WeakReference ptr, Object* parent)
{
    parent->cnv->pd->setThis();

    // This will ensure the object is still valid at this point, and also locks the audio thread to make sure it will remain valid
    hash32 name;
    bool isNonPatchable = false;
    if (auto checked = ptr.get<t_gobj>()) {
        name = hash(pd::Interface::getObjectClassName(checked.cast<t_pd>()));
        isNonPatchable = !pd::Interface::checkObject(checked.get());
    } else {
        return nullptr;
    }

    if (parent->cnv->pd->isLuaClass(name)) {
        if (auto checked = ptr.get<t_gobj>()) {
            if (checked.cast<t_pdlua>()->has_gui) {
                return new LuaObject(ptr, parent);
            } else {
                return new LuaTextObject(ptr, parent);
            }
        }
    }
    // check if object is a patcher object, or something else
    if (isNonPatchable && name != hash("scalar")) {
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
            unsigned int type;
            if (auto checked = ptr.get<t_text>()) {
                type = checked->te_type;
            } else {
                break;
            }
            if (type == T_OBJECT) {
                return new TextObject(ptr, parent, false);
            } else {
                return new CommentObject(ptr, parent);
            }
        }
            // Check if message type text object to prevent confusing it with else/message
        case hash("message"): {
            if (auto checked = ptr.get<t_gobj>()) {
                if (pd::Interface::isTextObject(checked.get()) && checked.cast<t_text>()->te_type == T_MESSAGE) {
                    return new MessageObject(ptr, parent);
                }
            }
            break;
        }
        case hash("pad"):
            return new MousePadObject(ptr, parent);
        case hash("keyboard"):
            return new KeyboardObject(ptr, parent);
        case hash("pic"):
            return new PictureObject(ptr, parent);
        case hash("gatom"): {
            if (auto checked = ptr.get<t_gobj>()) {
                if (checked.cast<t_fake_gatom>()->a_flavor == A_FLOAT) {
                    return new FloatAtomObject(ptr, parent);
                } else if (checked.cast<t_fake_gatom>()->a_flavor == A_SYMBOL) {
                    return new SymbolAtomObject(ptr, parent);
                } else if (checked.cast<t_fake_gatom>()->a_flavor == A_NULL) {
                    return new ListObject(ptr, parent);
                }
            }
            break;
        }
        case hash("canvas"):
        case hash("graph"): {
            if (auto checked = ptr.get<t_gobj>()) {
                auto const* canvas = checked.cast<t_canvas>();
                if (checked.cast<t_canvas>()->gl_list) {
                    t_class* c = canvas->gl_list->g_pd;
                    if (c && c->c_name && String::fromUTF8(c->c_name->s_name) == "array") {
                        return new ArrayObject(ptr, parent);
                    } else if (canvas->gl_isgraph) {
                        return new GraphOnParent(ptr, parent);
                    } else { // abstraction or subpatch
                        return new SubpatchObject(ptr, parent);
                    }
                } else if (canvas->gl_isgraph) {
                    return new GraphOnParent(ptr, parent);
                } else {
                    return new SubpatchObject(ptr, parent);
                }
            }
        }
        case hash("array define"):
            return new ArrayDefineObject(ptr, parent);
        case hash("clone"):
            return new CloneObject(ptr, parent);
        case hash("pd"):
            return new SubpatchObject(ptr, parent);
        case hash("pd~"):
            return new PdTildeObject(ptr, parent);
        case hash("scalar"): {
            if (auto checked = ptr.get<t_gobj>()) {
                if (checked->g_pd == scalar_class) {
                    return new ScalarObject(ptr, parent);
                }
            }
            break;
        }
        case hash("colors"):
            return new ColourPickerObject(ptr, parent);
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
        case hash("popmenu"):
            return new PopMenu(ptr, parent);
            // case hash("dropzone"):
            //     return new DropzoneObject(ptr, parent);
        case hash("openfile"): {
            if (auto checked = ptr.get<t_gobj>()) {
                char* text;
                int size;
                pd::Interface::getObjectText(checked.cast<t_text>(), &text, &size);
                auto const objText = String::fromUTF8(text, size);
                if (objText.contains("openfile -h")) {
                    return new OpenFileObject(ptr, parent);
                } else {
                    return new TextObject(ptr, parent);
                }
            }
        }
        case hash("noteout"):
        case hash("pgmout"):
        case hash("bendout"): {
            return new MidiObject(ptr, parent, false, false);
        }
        case hash("notein"):
        case hash("pgmin"):
        case hash("bendin"): {
            return new MidiObject(ptr, parent, true, false);
        }
        case hash("ctlout"): {
            return new MidiObject(ptr, parent, false, true);
        }
        case hash("ctlin"): {
            return new MidiObject(ptr, parent, true, true);
        }
        case hash("pdlua"): {
            return new LuaTextObject(ptr, parent);
        }
        default:
            break;
        }
    }
    return new TextObject(ptr, parent);
}

void ObjectBase::updateProperties()
{
    propertyListener.setNoCallback(true);
    update();
    propertyListener.setNoCallback(false);
}

void ObjectBase::getMenuOptions(PopupMenu& menu)
{
    if (auto obj = ptr.get<t_pd>()) {
        if (zgetfn(obj.get(), pd->generateSymbol("menu-open")) != nullptr) {
            menu.addItem("Open", [_this = SafePointer(this)] {
                if (!_this)
                    return;
                if (auto obj = _this->ptr.get<t_pd>()) {
                    _this->pd->sendDirectMessage(obj.get(), "menu-open", {});
                }
            });
        } else {
            menu.addItem(-1, "Open", false);
        }
    } else {
        menu.addItem(-1, "Open", false);
    }
}

bool ObjectBase::hideInGraph()
{
    return false;
}

void ObjectBase::lock(bool const isLocked)
{
    setInterceptsMouseClicks(isLocked, isLocked);
}

String ObjectBase::getBinbufSymbol(int const argIndex) const
{
    if (auto obj = ptr.get<t_object>()) {
        auto const* binbuf = obj->te_binbuf;
        int const numAtoms = binbuf_getnatom(binbuf);
        if (argIndex < numAtoms) {
            StackArray<char, 80> buf;
            atom_string(binbuf_getvec(binbuf) + argIndex, buf.data(), 80);
            return String::fromUTF8(buf.data());
        }
    }

    return {};
}

pd::Patch::Ptr ObjectBase::getPatch()
{
    return nullptr;
}

bool ObjectBase::canReceiveMouseEvent(int x, int y)
{
    return true;
}

void ObjectBase::receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms)
{
    object->triggerOverlayActiveState();

    auto const symHash = hash(symbol->s_name);
    switch (symHash) {
    case hash("size"):
    case hash("delta"):
    case hash("pos"):
    case hash("dim"):
    case hash("width"):
    case hash("height"): {
        MessageManager::callAsync([_this = SafePointer(this)] {
            if (_this)
                _this->object->updateBounds();
        });
        break;
    }
    case hash("_activity"):
        return;
    default:
        break;
    }

    receiveObjectMessage(symHash, atoms);
}

void ObjectBase::setParameterExcludingListener(Value& parameter, var const& value)
{
    propertyListener.setNoCallback(true);

    auto oldValue = parameter.getValue();
    parameter.setValue(value);

    propertyListener.setNoCallback(false);
}

void ObjectBase::setParameterExcludingListener(Value& parameter, var const& value, Value::Listener* otherListener)
{
    propertyListener.setNoCallback(true);
    parameter.removeListener(otherListener);

    auto oldValue = parameter.getValue();
    parameter.setValue(value);

    parameter.addListener(otherListener);
    propertyListener.setNoCallback(false);
}

ObjectLabel* ObjectBase::getLabel(int const index)
{
    if (index < labels.size())
        return labels[index];
    return nullptr;
}

bool ObjectBase::isBeingEdited()
{
    return edited;
}

ComponentBoundsConstrainer* ObjectBase::getConstrainer() const
{
    return constrainer.get();
}

ObjectBase::ResizeDirection ObjectBase::getAllowedResizeDirections() const
{
    return Any;
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
            bool const isStretchingTop,
            bool const isStretchingLeft,
            bool const isStretchingBottom,
            bool const isStretchingRight) override
        {
            // we remove the margin from the resizing object
            BorderSize<int> const border(Object::margin);
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
            auto const isStretchingWidth = isStretchingLeft || isStretchingRight;
            auto const isStretchingHeight = isStretchingBottom || isStretchingTop;

            if (getFixedAspectRatio() != 0.0f && isStretchingWidth ^ isStretchingHeight) {

                bounds = isStretchingHeight ? bounds.withX(old.getX()) : bounds.withY(old.getY());
            }
        }
    };

    return std::make_unique<ObjectBoundsConstrainer>();
}

bool ObjectBase::recurseHvccCompatibility(String const& objectText, pd::Patch::Ptr patch, String const& prefix)
{
    auto const instance = patch->instance;

    if (objectText.startsWith("pd @hv_obj") || HeavyCompatibleObjects::isCompatible(objectText)) {
        return true;
    }

    bool compatible = true;

    for (auto object : patch->getObjects()) {
        if (auto ptr = object.get<t_pd>()) {
            String const type = pd::Interface::getObjectClassName(ptr.get());

            if (type == "canvas" || type == "graph") {
                pd::Patch::Ptr const subpatch = new pd::Patch(object, instance, false);

                if (subpatch->isSubpatch()) {
                    char* text = nullptr;
                    int size = 0;
                    pd::Interface::getObjectText(&ptr.cast<t_canvas>()->gl_obj, &text, &size);
                    auto objName = String::fromUTF8(text, size);

                    compatible = recurseHvccCompatibility(objName, subpatch, prefix + objName + " -> ") && compatible;
                    freebytes(text, static_cast<size_t>(size) * sizeof(char));
                } else if (!HeavyCompatibleObjects::isCompatible(type)) {
                    compatible = false;
                    instance->logWarning(String("Warning: object \"" + prefix + type + "\" is not supported in Compiled Mode"));
                }
            } else if (!HeavyCompatibleObjects::isCompatible(type)) {
                compatible = false;
                instance->logWarning(String("Warning: object \"" + prefix + type + "\" is not supported in Compiled Mode"));
            }
        }
    }

    return compatible;
}
