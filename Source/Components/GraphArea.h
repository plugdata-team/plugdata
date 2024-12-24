/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

// Graph bounds component
class GraphArea final : public Component
    , public NVGComponent
    , public Value::Listener
    , public ModifierKeyListener {
    ComponentBoundsConstrainer constrainer;
    ResizableBorderComponent resizer;
    Canvas* canvas;
    ComponentDragger dragger;
    Rectangle<float> topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;
    bool shiftDown = false;

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
        canvas->editor->addModifierKeyListener(this);
    }

    ~GraphArea() override
    {
        canvas->locked.removeListener(this);
    }

    void shiftKeyChanged(bool const isHeld) override
    {
        resizer.setVisible(!isHeld);
        setMouseCursor(isHeld ? MouseCursor::UpDownLeftRightResizeCursor : MouseCursor::NormalCursor);
        shiftDown = isHeld;
    }

    void valueChanged(Value& v) override
    {
        setVisible(!getValue<bool>(v));
    }

    void render(NVGcontext* nvg) override
    {
        auto lineBounds = getLocalBounds().toFloat().reduced(4.0f);

        nvgDrawRoundedRect(nvg, lineBounds.getX(), lineBounds.getY(), lineBounds.getWidth(), lineBounds.getHeight(), nvgRGBA(0, 0, 0, 0), canvas->graphAreaCol, Corners::objectCornerRadius);

        auto& resizeHandleImage = canvas->resizeHandleImage;
        int angle = 360;

        auto getVert = [lineBounds](int const index) -> Point<float> {
            switch (index) {
            case 0:
                return lineBounds.getTopLeft();
            case 1:
                return lineBounds.getBottomLeft();
            case 2:
                return lineBounds.getBottomRight();
            case 3:
                return lineBounds.getTopRight();
            default:
                return {};
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
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, 0, 0, 9, 9, 0, resizeHandleImage.getImageId(), canvas->graphAreaCol));
            nvgFill(nvg);
            angle -= 90;
        }
    }

    bool hitTest(int const x, int const y) override
    {
        return (topLeftCorner.contains(x, y) || topRightCorner.contains(x, y) || bottomLeftCorner.contains(x, y) || bottomRightCorner.contains(x, y)) && !getLocalBounds().reduced(4).contains(x, y);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (shiftDown) {
            dragger.startDraggingComponent(this, e);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (shiftDown) {
            dragger.dragComponent(this, e, nullptr);
        }
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

    void moved() override
    {
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
        auto const patchBounds = canvas->patch.getGraphBounds().expanded(4.0f);
        auto const width = patchBounds.getWidth() + 1;
        auto const height = patchBounds.getHeight() + 1;
        setBounds(patchBounds.translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y).withWidth(width).withHeight(height));
    }
};
