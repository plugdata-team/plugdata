/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class SubpatchObject final : public TextBase
    , public SettingsFileListener {

    pd::Patch::Ptr subpatch;
    Value isGraphChild = SynchronousValue(var(false));

public:
    SubpatchObject(pd::WeakReference obj, Object* object)
        : TextBase(obj, object)
        , subpatch(new pd::Patch(obj, cnv->pd, false))
    {
        objectParameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" });

        // There is a possibility that a donecanvasdialog message is sent inbetween the initialisation in pd and the initialisation of the plugdata object, making it possible to miss this message. This especially tends to happen if the messagebox is connected to a loadbang.
        // By running another update call asynchrounously, we can still respond to the new state
        MessageManager::callAsync([_this = SafePointer(this)] {
            if (_this) {
                _this->update();
                _this->propertyChanged(_this->isGraphChild);
            }
        });

        setRepaintsOnMouseActivity(true);
    }

    ~SubpatchObject() override
    {
        closeOpenedSubpatchers();
    }

    void settingsChanged(String const& name, var const& value) override
    {
        if (name == "hvcc_mode") {
            if (value) {
                checkHvccCompatibility(getText(), subpatch.get());
            }
        }
    }

    void render(NVGcontext* nvg) override
    {
        TextBase::render(nvg);
    }

    void update() override
    {
        // Change from subpatch to graph
        if (auto canvas = ptr.get<t_canvas>()) {
            isGraphChild = static_cast<bool>(canvas->gl_isgraph);
        } else {
            return;
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (isLocked && click(e.getPosition(), e.mods.isShiftDown(), e.mods.isAltDown())) {
            return;
        }

        //  If locked and it's a left click
        if (isLocked && !e.mods.isRightButtonDown()) {
            openSubpatch();
            return;
        }
        TextBase::mouseDown(e);
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            // forward the value change to the text object
            TextBase::propertyChanged(v);
        } else if (v.refersToSameSourceAs(isGraphChild)) {
            int const isGraph = getValue<bool>(isGraphChild);
            if (auto glist = ptr.get<t_glist>()) {
                canvas_setgraph(glist.get(), isGraph + 2 * glist->gl_hidetext, 0);
            }

            if (isGraph) {
                MessageManager::callAsync([this, _this = SafePointer(this)] {
                    if (!_this)
                        return;

                    _this->cnv->setSelected(object, false);
                    _this->object->editor->sidebar->hideParameters();
                    _this->object->setType(_this->getText(), ptr);
                });
            }
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("donecanvasdialog"):
        case hash("coords"): {
            update();
            break;
        }
        default:
            break;
        }
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open", [_this = SafePointer(this)] { if(_this) _this->openSubpatch(); });
    }

    bool showParametersWhenSelected() override
    {
        return cnv->isGraph;
    }

    static void checkHvccCompatibility(String const& objectText, pd::Patch::Ptr patch, String const& prefix = "")
    {
        auto* instance = patch->instance;

        if (objectText.startsWith("pd @hv_obj")) {
            return;
        }

        for (auto object : patch->getObjects()) {
            if (auto ptr = object.get<t_pd>()) {
                String const type = pd::Interface::getObjectClassName(ptr.get());

                if (type == "canvas" || type == "graph") {
                    pd::Patch::Ptr const subpatch = new pd::Patch(object, instance, false);

                    char* text = nullptr;
                    int size = 0;
                    pd::Interface::getObjectText(&ptr.cast<t_canvas>()->gl_obj, &text, &size);
                    auto objName = String::fromUTF8(text, size);

                    checkHvccCompatibility(objName, subpatch, prefix + objName + " -> ");
                    freebytes(text, static_cast<size_t>(size) * sizeof(char));

                } else if (!HeavyCompatibleObjects::getAllCompatibleObjects().contains(type)) {
                    instance->logWarning(String("Warning: object \"" + prefix + type + "\" is not supported in Compiled Mode"));
                }
            }
        }
    }
};
