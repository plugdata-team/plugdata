/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Graph bounds component
class GraphArea : public Component
    , public NVGComponent
    , public Value::Listener {
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

    void render(NVGcontext* nvg) override
    {
        auto lineBounds = getLocalBounds().toFloat().reduced(4.0f);
        auto graphAreaColour = convertColour(findColour(PlugDataColour::graphAreaColourId));

        nvgDrawRoundedRect(nvg, lineBounds.getX(), lineBounds.getY(), lineBounds.getWidth(), lineBounds.getHeight(), nvgRGBAf(0, 0, 0, 0), graphAreaColour, Corners::objectCornerRadius);

        auto &resizeHandleImage = canvas->resizeGOPHandleImage;
        int angle = 360;

        auto getVert = [lineBounds](int index) -> Point<float> {
            switch(index){
                case 0:
                    return lineBounds.getTopLeft();
                case 1:
                    return lineBounds.getBottomLeft();
                case 2:
                    return lineBounds.getBottomRight();
                case 3:
                    return lineBounds.getTopRight();
                default:
                    return { };
            }
        };

        for (int i = 0; i < 4; i++) {
            NVGScopedState scopedState(nvg);
            // Rotate around centre
            nvgTranslate(nvg, getVert(i).x, getVert(i).y);
            nvgRotate(nvg, degreesToRadians<float>(angle));
            nvgTranslate(nvg, -3.0f, -3.0f);

            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, 9, 9);
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, 9, 9, 0, resizeHandleImage.getImageId(), 1));
            nvgFill(nvg);
            angle -= 90;
        }
    }

    bool hitTest(int x, int y) override
    {
        return (topLeftCorner.contains(x, y) || topRightCorner.contains(x, y) || bottomLeftCorner.contains(x, y) || bottomRightCorner.contains(x, y)) && !getLocalBounds().reduced(4).contains(x, y);
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

        // Update parent canvas directly (needed if open in splitview)
        applyBounds();
        canvas->synchroniseAllCanvases();
    }

    void applyBounds()
    {
        if (auto cnv = canvas->patch.getPointer()) {
            cnv->gl_pixwidth = getWidth() - 8 - 1;
            cnv->gl_pixheight = getHeight() - 8 - 1;

            cnv->gl_xmargin = getX() - canvas->canvasOrigin.x + 4;
            cnv->gl_ymargin = getY() - canvas->canvasOrigin.y + 4;
        }

        canvas->updateDrawables();
    }

    void updateBounds()
    {
        auto patchBounds = canvas->patch.getBounds().expanded(4.0f);
        auto width = patchBounds.getWidth() + 1;
        auto height = patchBounds.getHeight() + 1;
        setBounds(patchBounds.translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y).withWidth(width).withHeight(height));
    }
};
