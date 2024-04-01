/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Graph bounds component
class GraphArea : public Component, public NVGComponent, public Value::Listener {
    ComponentBoundsConstrainer constrainer;
    ResizableBorderComponent resizer;
    Canvas* canvas;
    Rectangle<float> topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;

public:
    explicit GraphArea(Canvas* parent)
        : NVGComponent(this)
        , resizer(this, &constrainer)
        , canvas(parent)
    {
        addAndMakeVisible(resizer);
        updateBounds();
        
        constrainer.setMinimumSize(12, 12);

        resizer.setBorderThickness(BorderSize<int>(4, 4, 4, 4));
        resizer.addMouseListener(this, false);
        canvas->locked.addListener(this);
        valueChanged(canvas->locked);
    }

    ~GraphArea() override
    {
        canvas->locked.removeListener(this);
    }

    void valueChanged(Value& v) override
    {
        setVisible(!getValue<bool>(v));
    }
    
    Array<Rectangle<float>> getCorners() const
    {
        auto rect = getLocalBounds().toFloat().reduced(3.5f);
        float const offset = 2.0f;

        Array<Rectangle<float>> corners = { Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
            Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset) };

        return corners;
    }
    
    void render(NVGcontext* nvg) override
    {
        auto lineBounds = getLocalBounds().toFloat().reduced(4.0f);
        auto graphAreaColour = convertColour(findColour(PlugDataColour::graphAreaColourId));

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, lineBounds.getX(), lineBounds.getY(), lineBounds.getWidth(), lineBounds.getHeight(), Corners::objectCornerRadius);
        nvgStrokeColor(nvg, graphAreaColour);
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    
        
        auto drawCorner = [](NVGcontext* nvg, int x, int y, int angle){
            nvgSave(nvg);
            
            nvgTranslate(nvg, x, y);
            nvgRotate(nvg, degreesToRadians<float>(angle));
            
            // (Calculated from svg)
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, 3.51f, 9.004f);
            nvgLineTo(nvg, 2.251f, 9.004f);
            nvgBezierTo(nvg, 0.0f, 9.004f, 0.0f, 7.996f, 0.0f, 2.251f);

            nvgBezierTo(nvg, 0.0f, 1.009f, 1.008f, 0.0f, 2.251f, 0.0f);

            nvgLineTo(nvg, 6.753f, 0.0f);
            nvgBezierTo(nvg, 7.995f, 0.0f, 9.004f, 1.009f, 9.004f, 2.251f);
            
            nvgLineTo(nvg, 9.004f, 3.511f);
            nvgLineTo(nvg, 6.239f, 3.511f);
            nvgBezierTo(nvg, 4.733f, 3.511f, 3.51f, 4.734f, 3.51f, 6.24f);
            nvgClosePath(nvg);

            nvgFill(nvg);
            nvgRestore(nvg);
        };
        
        nvgFillColor(nvg, graphAreaColour);
        drawCorner(nvg, 1, 1, 0);
        drawCorner(nvg, getWidth() - 1, 1, 90);
        drawCorner(nvg, getWidth() - 1, getHeight() - 1, 180);
        drawCorner(nvg, 1, getHeight() - 1, 270);
    }

    bool hitTest(int x, int y) override
    {
        return (topLeftCorner.contains(x, y) || topRightCorner.contains(x, y) || bottomLeftCorner.contains(x, y)|| bottomRightCorner.contains(x, y)) && !getLocalBounds().reduced(4).contains(x, y);
    }
    
    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }
    
    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }
    
    void mouseMove(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        applyBounds();
        repaint();
    }

    void resized() override
    {
        topLeftCorner = getLocalBounds().toFloat().removeFromTop(9).removeFromLeft(9).translated(0.5f, 0.5f);
        topRightCorner = getLocalBounds().toFloat().removeFromTop(9).removeFromRight(9).translated(-0.5f, 0.5f);
        bottomLeftCorner = getLocalBounds().toFloat().removeFromBottom(9).removeFromLeft(9).translated(0.5f, -0.5f);
        bottomRightCorner = getLocalBounds().toFloat().removeFromBottom(9).removeFromRight(9).translated(-0.5f, -0.5f);
        
        resizer.setBounds(getLocalBounds());
        repaint();
    }

    void applyBounds()
    {
        if (auto cnv = canvas->patch.getPointer()) {
            cnv->gl_pixwidth = getWidth() - 8;
            cnv->gl_pixheight = getHeight() - 8;

            cnv->gl_xmargin = getX() - canvas->canvasOrigin.x + 4;
            cnv->gl_ymargin = getY() - canvas->canvasOrigin.y + 4;
        }
        
        canvas->updateDrawables();
    }

    void updateBounds()
    {
        setBounds(canvas->patch.getBounds().expanded(4).translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y));
    }
};
