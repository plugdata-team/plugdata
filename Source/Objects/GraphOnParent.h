
struct GraphOnParent : public GUIComponent
{
    bool isLocked = false;
    
public:
    // Graph On Parent
    GraphOnParent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), subpatch(gui.getPatch())
    {
        setInterceptsMouseClicks(box->locked == var(false), true);
        
        updateCanvas();
        
        initialise(newObject);
        
        addMouseListener(this, true);
        
        resized();
    }
    
    void checkBoxBounds() override {
        // Apply size limits
        int w = jlimit(25, maxSize, box->getWidth());
        int h = jlimit(25, maxSize, box->getHeight());
        
        if(w != box->getWidth() || h != box->getHeight()) {
            box->setSize(w, h);
        }
    }
    
    
    ~GraphOnParent() override
    {
        closeOpenedSubpatchers();
    }
    
    void resized() override
    {
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!isLocked)
        {
            box->mouseDown(e.getEventRelativeTo(box));
        }
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if (!isLocked)
        {
            box->mouseDrag(e.getEventRelativeTo(box));
        }
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if (!isLocked)
        {
            box->mouseUp(e.getEventRelativeTo(box));
        }
    }
    
    void updateCanvas()
    {
        if (!canvas)
        {
            canvas = std::make_unique<Canvas>(box->cnv->main, subpatch, this);
            
            // Make sure that the graph doesn't become the current canvas
            box->cnv->patch.setCurrent(true);
            box->cnv->main.updateCommandStatus();
        }
        
        auto b = getPatch()->getBounds();
        // canvas->checkBounds();
        canvas->setBounds(-b.getX(), -b.getY(), b.getWidth() + b.getX(), b.getHeight() + b.getY());
        canvas->setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
    }
    
    void updateValue() override
    {
        updateCanvas();
        
        if (!canvas) return;
        
        for (auto& box : canvas->boxes)
        {
            if (box->graphics)
            {
                box->graphics->updateValue();
            }
        }
    }
    
    // override to make transparent
    void paint(Graphics& g) override
    {
    }
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    Canvas* getCanvas() override
    {
        return canvas.get();
    }
    
private:
    pd::Patch subpatch;
    std::unique_ptr<Canvas> canvas;
};
