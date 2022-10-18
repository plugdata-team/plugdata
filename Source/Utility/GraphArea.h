// Graph bounds component
struct GraphArea : public Component
    , public ComponentDragger {
    ResizableCornerComponent resizer;
    Canvas* canvas;

    explicit GraphArea(Canvas* parent)
        : resizer(this, nullptr)
        , canvas(parent)
    {
        addAndMakeVisible(resizer);
        updateBounds();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), 2.0f, 4.0f);
    }

    bool hitTest(int x, int y) override
    {
        return !getLocalBounds().reduced(8).contains(Point<int> { x, y });
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (canvas->locked == var(false)) {
            setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        startDraggingComponent(this, e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        dragComponent(this, e, nullptr);
    }

    void mouseUp(MouseEvent const& e) override
    {
        setPdBounds();
        repaint();
    }

    void resized() override
    {
        int handleSize = 20;

        setPdBounds();
        resizer.setBounds(getWidth() - handleSize, getHeight() - handleSize, handleSize, handleSize);

        canvas->updateDrawables();

        repaint();
    }

    void setPdBounds()
    {
        t_canvas* cnv = canvas->patch.getPointer();
        // TODO: make this thread safe
        if (cnv) {
            cnv->gl_pixwidth = getWidth();
            cnv->gl_pixheight = getHeight();

            cnv->gl_xmargin = getX() - canvas->canvasOrigin.x;
            cnv->gl_ymargin = getY() - canvas->canvasOrigin.y;
        }
    }

    void updateBounds()
    {
        setBounds(canvas->patch.getBounds().translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y));
    }
};
