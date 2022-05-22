
struct GraphOnParent : public GUIObject
{
    bool isLocked = false;

   public:
    // Graph On Parent
    GraphOnParent(void* obj, Box* box) : GUIObject(obj, box), subpatch({ptr, cnv->pd})
    {
        
        setInterceptsMouseClicks(box->locked == var(false), true);

        updateCanvas();

        initialise();

        addMouseListener(this, true);

        resized();
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(25, maxSize, box->getWidth());
        int h = jlimit(25, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }
    
    void updateBounds() override {
        box->setBounds(getBounds().expanded(Box::margin));
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
        GUIObject::mouseDown(e);
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
            canvas = std::make_unique<Canvas>(cnv->main, subpatch, this);

            // Make sure that the graph doesn't become the current canvas
            cnv->patch.setCurrent(true);
            cnv->main.updateCommandStatus();
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
