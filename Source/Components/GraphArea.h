/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Graph bounds component
class GraphArea : public Component, public Value::Listener {
    ComponentBoundsConstrainer constrainer;
    ResizableBorderComponent resizer;
    Canvas* canvas;
    Rectangle<float> topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;

public:
    explicit GraphArea(Canvas* parent)
        : resizer(this, &constrainer)
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

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::graphAreaColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(3.5f), Corners::objectCornerRadius, 1.0f);

        g.saveState();
        
        // Make a rounded rectangle hole path:
        // We do this by creating a large rectangle path with inverted winding
        // and adding the inner rounded rectangle path
        // this creates one path that has a hole in the middle
        Path outerArea;
        outerArea.addRectangle(getLocalBounds());
        outerArea.setUsingNonZeroWinding(false);
        Path innerArea;
        
        auto innerRect = getLocalBounds().toFloat().reduced(3.5f);
        innerArea.addRoundedRectangle(innerRect, Corners::objectCornerRadius);
        outerArea.addPath(innerArea);

        // use the path with a hole in it to exclude the inner rounded rect from painting
        g.reduceClipRegion(outerArea);

        g.fillRoundedRectangle(topLeftCorner, Corners::objectCornerRadius);
        g.fillRoundedRectangle(topRightCorner, Corners::objectCornerRadius);
        g.fillRoundedRectangle(bottomLeftCorner, Corners::objectCornerRadius);
        g.fillRoundedRectangle(bottomRightCorner, Corners::objectCornerRadius);
        
        g.restoreState();
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
