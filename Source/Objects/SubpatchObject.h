
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
