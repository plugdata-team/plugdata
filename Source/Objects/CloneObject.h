
extern "C"
{
    t_glist* clone_get_instance(t_gobj*, int);
    int clone_get_n(t_gobj*);
}

struct CloneObject final : public TextBase
{
    CloneObject(void* obj, Box* box) : TextBase(obj, box), subpatch({nullptr, nullptr})
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (clone_get_n(gobj) > 0)
        {
            subpatch = {clone_get_instance(gobj, 0), cnv->pd};
        }
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

    ~CloneObject()
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
