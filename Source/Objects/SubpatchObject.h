/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SubpatchObject final : public TextBase {

    pd::Patch subpatch;
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));

    bool locked = false;

public:
    SubpatchObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch({ ptr, cnv->pd })
    {
        isGraphChild = false;
        hideNameAndArgs = static_cast<bool>(subpatch.getPointer()->gl_hidetext);

        isGraphChild.addListener(this);
        hideNameAndArgs.addListener(this);

        object->hvccMode.addListener(this);

        if (static_cast<bool>(object->hvccMode.getValue())) {
            checkHvccCompatibility(subpatch);
        }
    }

    ~SubpatchObject()
    {
        object->hvccMode.removeListener(this);
        closeOpenedSubpatchers();
    }

    void updateValue()
    {
        // Change from subpatch to graph
        if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            cnv->setSelected(object, false);
            object->cnv->editor->sidebar.hideParameters();
            object->setType(objectText, ptr);
        }
    };

    void mouseDown(MouseEvent const& e) override
    {
        //  If locked and it's a left click
        if (locked && !e.mods.isRightButtonDown() && !object->attachedToMouse) {
            openSubpatch();

            return;
        } else {
            TextBase::mouseDown(e);
        }
    }

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    ObjectParameters getParameters() override
    {
        return { { "Is graph", tBool, cGeneral, &isGraphChild, { "No", "Yes" } }, { "Hide name and arguments", tBool, cGeneral, &hideNameAndArgs, { "No", "Yes" } } };
    };

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(isGraphChild)) {
            subpatch.getPointer()->gl_isgraph = static_cast<bool>(isGraphChild.getValue());
            updateValue();
        } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
            subpatch.getPointer()->gl_hidetext = static_cast<bool>(hideNameAndArgs.getValue());
            repaint();
        } else if (v.refersToSameSourceAs(object->hvccMode)) {
            if (static_cast<bool>(v.getValue())) {
                checkHvccCompatibility(subpatch);
            }
        }
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

    static void checkHvccCompatibility(pd::Patch& patch, String prefix = "")
    {

        auto* instance = patch.instance;

        for (auto* object : patch.getObjects()) {
            const String name = libpd_get_object_class_name(object);

            if (name == "canvas" || name == "graph") {
                auto patch = pd::Patch(object, instance);

                char* text = nullptr;
                int size = 0;
                libpd_get_object_text(object, &text, &size);

                checkHvccCompatibility(patch, prefix + String(text) + " -> ");
                freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
                
            } else if (!Object::hvccObjects.contains(name)) {
                instance->logWarning(String("Warning: object \"" + prefix + name + "\" is not supported in Compiled Mode").toRawUTF8());
            }
        }
    }
};
