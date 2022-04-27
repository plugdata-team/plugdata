
struct Subpatch : public GUIComponent
{
    Subpatch(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), subpatch(gui.getPatch())
    {
    }
    
    void updateValue() override
    {
        // Pd sometimes sets the isgraph flag too late...
        // In that case we tell the box to create the gui
        if (static_cast<t_canvas*>(gui.getPointer())->gl_isgraph)
        {
            box->setType(box->currentText, true);
        }
    };
    
    ~Subpatch()
    {
        closeOpenedSubpatchers();
    }
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    bool noGui() override
    {
        return true;
    }
    
private:
    pd::Patch subpatch;
};
