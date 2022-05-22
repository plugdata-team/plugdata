

extern "C"
{
    t_glist* clone_get_instance(t_gobj*, int);
    int clone_get_n(t_gobj*);
}

struct SubpatchObject : public TextObject
{
    SubpatchObject(void* obj, Box* box) : TextObject(obj, box), subpatch({ptr, cnv->pd})
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
        // closeOpenedSubpatchers();
    }

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    void updateBounds() override
    {
        box->setBounds(getBounds().expanded(Box::margin));
    }

   protected:
    pd::Patch subpatch;
};

struct CloneObject : public SubpatchObject
{
    CloneObject(void* obj, Box* box) : SubpatchObject(obj, box)
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (clone_get_n(gobj) > 0)
        {
            subpatch = {clone_get_instance(gobj, 0), cnv->pd};
        }
    }

    void updateValue() override
    {
    }
};
