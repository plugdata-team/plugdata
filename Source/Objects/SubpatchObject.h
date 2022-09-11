
struct SubpatchObject final : public TextBase
    , public Value::Listener {
    SubpatchObject(void* obj, Object* object)
        : TextBase(obj, object)
        , subpatch({ ptr, cnv->pd })
    {
        isGraphChild = false;
        hideNameAndArgs = static_cast<bool>(subpatch.getPointer()->gl_hidetext);

        isGraphChild.addListener(this);
        hideNameAndArgs.addListener(this);
    }

    void updateValue() override
    {
        // Change from subpatch to graph
        if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            cnv->setSelected(object, false);
            object->cnv->main.sidebar.hideParameters();
            object->setType(currentText, ptr);
        }
    };

    void mouseDown(MouseEvent const& e) override
    {
        
        //  If locked and it's a left click
        if (locked && !ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
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
        setInterceptsMouseClicks(isLocked, isLocked);
        locked = isLocked;
    }

    ~SubpatchObject()
    {
        closeOpenedSubpatchers();
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

protected:
    pd::Patch subpatch;
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
        
    bool locked = false;
};
