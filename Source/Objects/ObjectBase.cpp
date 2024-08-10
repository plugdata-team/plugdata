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
#include "TextDefineObject.h"
#include "ScopeObject.h"
#include "FunctionObject.h"
#include "BicoeffObject.h"
#include "NoteObject.h"
#include "ColourPickerObject.h"
#include "MidiObjects.h"
#include "OpenFileObject.h"
#include "PdTildeObject.h"
#include "LuaObject.h"

// Class for non-patchable objects
class NonPatchable : public ObjectBase {

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
}

void ObjectBase::ObjectSizeListener::componentMovedOrResized(Component& component, bool moved, bool resized)
{
    dynamic_cast<Object&>(component).gui->objectMovedOrResized(resized);
}

void ObjectBase::ObjectSizeListener::valueChanged(Value& v)
{
    if (auto obj = object->gui->ptr.get<t_gobj>()) {
        auto* patch = object->cnv->patch.getPointer().get();
        if (!patch)
            return;

        auto x = static_cast<float>(v.getValue().getArray()->getReference(0));
        auto y = static_cast<float>(v.getValue().getArray()->getReference(1));

        pd::Interface::moveObject(patch, obj.get(), x, y);
        object->updateBounds();
    }
}

ObjectBase::PropertyUndoListener::PropertyUndoListener()
{
    lastChange = Time::getMillisecondCounter();
}

void ObjectBase::PropertyUndoListener::valueChanged(Value& v)
{
    if (Time::getMillisecondCounter() - lastChange > 400) {
        onChange();
    }

    lastChange = Time::getMillisecondCounter();
}

ObjectBase::ObjectBase(pd::WeakReference obj, Object* parent)
    : NVGComponent(this)
    , ptr(obj)
    , object(parent)
    , cnv(parent->cnv)
    , pd(parent->cnv->pd)
    , objectSizeListener(parent)
{
    // Perform async, so that we don't get a size change callback for initial creation
    MessageManager::callAsync([_this = SafePointer(this)](){
        if(!_this) return;
        _this->object->addComponentListener(&_this->objectSizeListener);
        _this->updateLabel();
    });
    
    setWantsKeyboardFocus(true);

    setLookAndFeel(new PlugDataLook());

    auto objectBounds = object->getObjectBounds();
    positionParameter = Array<var> { var(objectBounds.getX()), var(objectBounds.getY()) };

    objectParameters.addParamPosition(&positionParameter);
    positionParameter.addListener(&objectSizeListener);

    propertyUndoListener.onChange = [_this = SafePointer(this)]() {
        if (!_this)
            return;

        if (auto obj = _this->ptr.get<t_gobj>()) {
            auto* canvas = _this->cnv->patch.getPointer().get();
            if (!canvas)
                return;

            pd::Interface::undoApply(canvas, obj.get());
        }
    };
}

ObjectBase::~ObjectBase()
{
    pd->unregisterMessageListener(ptr.getRawUnchecked<void>(), this);
    object->removeComponentListener(&objectSizeListener);

    auto* lnf = &getLookAndFeel();
    setLookAndFeel(nullptr);
    delete lnf;
}

void ObjectBase::initialise()
{
    update();
    constrainer = createConstrainer();
    onConstrainerCreate();

    pd->registerMessageListener(ptr.getRawUnchecked<void>(), this);

    for (auto& [name, type, cat, value, list, valueDefault, customComponent, onInteractionFn] : objectParameters.getParameters()) {
        if (value) {
            value->addListener(this);
            value->addListener(&propertyUndoListener);
        }
    }
}

void ObjectBase::objectMovedOrResized(bool resized)
{
    auto objectBounds = object->getObjectBounds();

    setParameterExcludingListener(positionParameter, Array<var> { var(objectBounds.getX()), var(objectBounds.getY()) }, &objectSizeListener);

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
        freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
        return txt;
    }

    return "";
}

String ObjectBase::getTypeWithOriginPrefix() const
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto type = getType();
        if (type.contains("/"))
            return type;

        auto origin = pd::Library::getObjectOrigin(obj.get());

        if (origin.isEmpty())
            return type;
        
        return origin + "/" + type;
    }

    return {};
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
        default:
            break;
        }

        return String::fromUTF8(className);
    }

    return {};
}

// Make sure the object can't be triggered if that palette is in drag mode
bool ObjectBase::hitTest(int x, int y)
{
    return Component::hitTest(x, y);
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
    auto* editor = object->editor;

    for (auto* canvas : editor->getCanvases()) {
        auto* patch = getPatch().get();
        if (patch && canvas && canvas->patch == *patch) {

            canvas->editor->getTabComponent().closeTab(canvas);
            break;
        }
    }
}

bool ObjectBase::click(Point<int> position, bool shift, bool alt)
{
    if (auto obj = ptr.get<t_text>()) {

        t_text* x = obj.get();
        if (x->te_type == T_OBJECT) {
            t_symbol* clicksym = gensym("click");
            auto click_func = zgetfn(&x->te_pd, clicksym);

            // Check if a click function has been registered, and if it's not the default canvas click function (in which case we want to handle it manually)
            if (click_func && reinterpret_cast<void*>(click_func) != reinterpret_cast<void*>(canvas_click)) {
                pd_vmess(&x->te_pd, clicksym, "fffff",
                    (double)position.x, (double)position.y,
                    (double)shift, (double)0, (double)alt);

                return true;
            } else
                return false;
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
    for (auto* cnv : cnv->editor->getCanvases()) {
        if (cnv->patch == *subpatch) {
            cnv->editor->getTabComponent().showTab(cnv, cnv->patch.splitViewIndex);
            return;
        }
    }
    
    cnv->editor->getTabComponent().setActiveSplit(cnv);
    subpatch->splitViewIndex = cnv->patch.splitViewIndex;
    cnv->editor->getTabComponent().openPatch(subpatch);

    if (path.getFullPathName().isNotEmpty()) {
        subpatch->setCurrentFile(URL(path));
    }
}

void ObjectBase::moveToFront()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        pd::Interface::toFront(patch, obj.get());
    }
}

void ObjectBase::moveForward()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        pd::Interface::moveForward(patch, obj.get());
    }
}

void ObjectBase::moveBackward()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        pd::Interface::moveBackward(patch, obj.get());
    }
}

void ObjectBase::moveToBack()
{
    if (auto obj = ptr.get<t_gobj>()) {
        auto* patch = cnv->patch.getPointer().get();
        if (!patch)
            return;

        pd::Interface::toBack(patch, obj.get());
    }
}

void ObjectBase::render(NVGcontext* nvg)
{
    imageRenderer.renderJUCEComponent(nvg, *this, getImageScale());
}

void ObjectBase::paint(Graphics& g)
{
    g.setColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

    bool selected = object->isSelected() && !cnv->isGraph;
    auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

    g.setColour(outlineColour);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
}

float ObjectBase::getImageScale()
{
    Canvas* topLevel = cnv;
    if(!hideInGraph()) { // No need to do this if we can't be visible in a graph anyway!
        while (auto* nextCnv = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCnv;
        }
    }
    return topLevel->isZooming ? topLevel->getRenderScale() * 2.0f : topLevel->getRenderScale() * std::max(1.0f, getValue<float>(topLevel->zoomScale));
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

void ObjectBase::sendFloatValue(float newValue)
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
    if (auto checked = ptr.get<t_gobj>()) {
        auto const name = hash(pd::Interface::getObjectClassName(checked.cast<t_pd>()));

        if (parent->cnv->pd->isLuaClass(name)) {
            if (checked.cast<t_pdlua>()->has_gui) {
                return new LuaObject(ptr, parent);
            } else {
                return new LuaTextObject(ptr, parent);
            }
        }
        // check if object is a patcher object, or something else
        if (!pd::Interface::checkObject(checked.get()) && name != hash("scalar")) {
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
                if (checked.cast<t_text>()->te_type == T_OBJECT) {
                    return new TextObject(ptr, parent, false);
                } else {
                    return new CommentObject(ptr, parent);
                }
            }
            // Check if message type text object to prevent confusing it with else/message
            case hash("message"): {
                if (pd::Interface::isTextObject(checked.get()) && checked.cast<t_text>()->te_type == T_MESSAGE) {
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
                if (checked.cast<t_fake_gatom>()->a_flavor == A_FLOAT) {
                    return new FloatAtomObject(ptr, parent);
                } else if (checked.cast<t_fake_gatom>()->a_flavor == A_SYMBOL) {
                    return new SymbolAtomObject(ptr, parent);
                } else if (checked.cast<t_fake_gatom>()->a_flavor == A_NULL) {
                    return new ListObject(ptr, parent);
                }
                break;
            }
            case hash("canvas"):
            case hash("graph"): {
                auto* canvas = checked.cast<t_canvas>();
                if (checked.cast<t_canvas>()->gl_list) {
                    t_class* c = canvas->gl_list->g_pd;
                    if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
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
            case hash("array define"):
                return new ArrayDefineObject(ptr, parent);
            case hash("clone"):
                return new CloneObject(ptr, parent);
            case hash("pd"):
                return new SubpatchObject(ptr, parent);
            case hash("pd~"):
                return new PdTildeObject(ptr, parent);
            case hash("scalar"): {
                if (checked->g_pd == scalar_class) {
                    return new ScalarObject(ptr, parent);
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
            case hash("openfile"): {
                char* text;
                int size;
                pd::Interface::getObjectText(checked.cast<t_text>(), &text, &size);
                auto objText = String::fromUTF8(text, size);
                bool hyperlink = objText.contains("openfile -h");
                if (hyperlink) {
                    return new OpenFileObject(ptr, parent);
                } else {
                    return new TextObject(ptr, parent);
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

String ObjectBase::getBinbufSymbol(int argIndex)
{
    if (auto obj = ptr.get<t_object>()) {
        auto* binbuf = obj->te_binbuf;
        int numAtoms = binbuf_getnatom(binbuf);
        if (argIndex < numAtoms) {
            char buf[80];
            atom_string(binbuf_getvec(binbuf) + argIndex, buf, 80);
            return String::fromUTF8(buf);
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

void ObjectBase::receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms)
{
    object->triggerOverlayActiveState();

    auto symHash = hash(symbol->s_name);

    switch (symHash) {
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
        break;
    }
    default:
        break;
    }

    receiveObjectMessage(symHash, atoms, numAtoms);
}

void ObjectBase::setParameterExcludingListener(Value& parameter, var const& value)
{
    parameter.removeListener(&propertyUndoListener);

    setValueExcludingListener(parameter, value, this);

    parameter.addListener(&propertyUndoListener);
}

void ObjectBase::setParameterExcludingListener(Value& parameter, var const& value, Value::Listener* otherListener)
{
    parameter.removeListener(&propertyUndoListener);
    parameter.removeListener(otherListener);

    setValueExcludingListener(parameter, value, this);

    parameter.addListener(otherListener);
    parameter.addListener(&propertyUndoListener);
}

ObjectLabel* ObjectBase::getLabel()
{
    if (labels)
        return labels->getObjectLabel();
    return nullptr;
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
