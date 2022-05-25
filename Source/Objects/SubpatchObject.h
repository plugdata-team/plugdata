
struct SubpatchObject final : public TextBase
{
    SubpatchObject(void* obj, Box* box) : TextBase(obj, box), subpatch({ptr, cnv->pd})
    {
    }

    void updateValue() override
    {
        // Pd sometimes sets the isgraph flag too late...
        // In that case we tell the box to create the gui
        if (static_cast<t_canvas*>(ptr)->gl_isgraph)
        {
            box->setType(currentText, ptr);
        }
    };
    
    void mouseDown(const MouseEvent& e) override
    {
        //  If locked and it's a left click
        if ((box->locked == var(true) || box->commandLocked == var(true)) && !ModifierKeys::getCurrentModifiers().isRightButtonDown())
        {
            box->openSubpatch();
        
            return;
        }
        else {
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

   protected:
    pd::Patch subpatch;
};
