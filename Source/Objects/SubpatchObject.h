/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SubpatchObject final : public TextBase {

    pd::Patch::Ptr subpatch;
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));

    bool locked = false;

public:
    SubpatchObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch(new pd::Patch(ptr, cnv->pd, false))
    {
        object->hvccMode.addListener(this);

        if (getValue<bool>(object->hvccMode)) {
            checkHvccCompatibility(subpatch.get());
        }
    }

    ~SubpatchObject()
    {
        object->hvccMode.removeListener(this);
        closeOpenedSubpatchers();
    }

    void update() override
    {
        isGraphChild = static_cast<bool>(subpatch->getPointer()->gl_isgraph);
        hideNameAndArgs = static_cast<bool>(subpatch->getPointer()->gl_hidetext);

        updateValue();
    }

    void updateValue()
    {
        // Change from subpatch to graph
        if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            cnv->setSelected(object, false);
            object->cnv->editor->sidebar->hideParameters();
            object->setType(objectText, ptr);
        }
    };

    void mouseDown(MouseEvent const& e) override
    {
        if (locked && click()) {
            return;
        }

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

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    ObjectParameters getParameters() override
    {
        return {
            makeParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" } ),
            makeParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" } )
        };
    };

    void checkGraphState()
    {
        if (!ptr)
            return;

        pd->setThis();

        int isGraph = getValue<bool>(isGraphChild);
        int hideText = getValue<bool>(hideNameAndArgs);

        canvas_setgraph(static_cast<t_glist*>(ptr), isGraph + 2 * hideText, 0);
        repaint();

        MessageManager::callAsync([this, _this = SafePointer(this)]() {
            if (!_this)
                return;

            // Change from subpatch to graph
            if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
                cnv->setSelected(object, false);
                object->cnv->editor->sidebar->hideParameters();
                object->setType(getText(), ptr);
                return;
            }
        });
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(isGraphChild) || v.refersToSameSourceAs(hideNameAndArgs)) {
            checkGraphState();
        } else if (v.refersToSameSourceAs(object->hvccMode)) {
            if (getValue<bool>(v)) {
                checkHvccCompatibility(subpatch.get());
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

    static void checkHvccCompatibility(pd::Patch::Ptr patch, String prefix = "")
    {
        auto* instance = patch->instance;

        for (auto* object : patch->getObjects()) {
            const String name = libpd_get_object_class_name(object);

            if (name == "canvas" || name == "graph") {
                pd::Patch::Ptr patch = new pd::Patch(object, instance, false);

                char* text = nullptr;
                int size = 0;
                libpd_get_object_text(object, &text, &size);

                checkHvccCompatibility(patch, prefix + String::fromUTF8(text) + " -> ");
                freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));

            } else if (!Object::hvccObjects.contains(name)) {
                instance->logWarning(String("Warning: object \"" + prefix + name + "\" is not supported in Compiled Mode"));
            }
        }
    }
};
