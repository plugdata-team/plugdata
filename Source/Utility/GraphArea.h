/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Graph bounds component
class GraphArea : public Component
    , public ComponentDragger {
    ResizableCornerComponent resizer;
    Canvas* canvas;

public:
    explicit GraphArea(Canvas* parent)
        : resizer(this, nullptr)
        , canvas(parent)
    {
        addAndMakeVisible(resizer);
        updateBounds();
        setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::resizeableCornerColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), Corners::objectCornerRadius, Corners::smallCornerRadius);
    }

    bool hitTest(int x, int y) override
    {
        return !getLocalBounds().reduced(8).contains(Point<int>(x, y));
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

        canvas->pd->enqueueFunction([cnv, _this = SafePointer(this)]() {
            if (_this && cnv) {
                cnv->gl_pixwidth = _this->getWidth();
                cnv->gl_pixheight = _this->getHeight();

                cnv->gl_xmargin = _this->getX() - _this->canvas->canvasOrigin.x;
                cnv->gl_ymargin = _this->getY() - _this->canvas->canvasOrigin.y;
            }
        });
    }

    void updateBounds()
    {
        setBounds(canvas->patch.getBounds().translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y));
    }
};
