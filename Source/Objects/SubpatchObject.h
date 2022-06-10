
struct SubpatchObject final : public TextBase, public Value::Listener
{
    SubpatchObject(void* obj, Box* box) : TextBase(obj, box), subpatch({ ptr, cnv->pd })
    {
        isGraphChild = false;
        hideNameAndArgs = static_cast<bool>(subpatch.getPointer()->gl_hidetext);

        isGraphChild.addListener(this);
        hideNameAndArgs.addListener(this);
    }

    void updateValue() override
    {
        // Change from subpatch to graph
        if(static_cast<t_canvas*>(ptr)->gl_isgraph)
        {
            cnv->setSelected(box, false);
            box->cnv->main.sidebar.hideParameters();
            box->setType(currentText, ptr);
        }
    };

    void mouseDown(const MouseEvent& e) override
    {
        //  If locked and it's a left click
        if((box->locked == var(true) || box->commandLocked == var(true)) && ! ModifierKeys::getCurrentModifiers().isRightButtonDown())
        {
            box->openSubpatch();

            return;
        }
        else
        {
            TextBase::mouseDown(e);
        }
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
        if(v.refersToSameSourceAs(isGraphChild))
        {
            subpatch.getPointer()->gl_isgraph = static_cast<bool>(isGraphChild.getValue());
            updateValue();
        }
        else if(v.refersToSameSourceAs(hideNameAndArgs))
        {
            subpatch.getPointer()->gl_hidetext = static_cast<bool>(hideNameAndArgs.getValue());
            repaint();
        }
    }

protected:
    pd::Patch subpatch;
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
};
