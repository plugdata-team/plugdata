/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class CanvasObject final : public ObjectBase {

    Value sizeProperty = SynchronousValue();
    Value hitAreaSize = SynchronousValue();
    Rectangle<float> hitArea;
    bool hideHitArea = false;
    IEMHelper iemHelper;

    Colour bgColour;
    NVGcolor bgCol;
    NVGcolor selectionAreaCol;

public:
    CanvasObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        object->setColour(PlugDataColour::outlineColourId, Colours::transparentBlack);

        iemHelper.iemColourChangedCallback = [this] {
            bgColour = Colour::fromString(iemHelper.secondaryColour.toString());
            bgCol = convertColour(bgColour);
            selectionAreaCol = convertColour(bgColour.contrasting(0.75f));
        };

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamInt("Active area size", ParameterCategory::cDimensions, &hitAreaSize, 15);
        objectParameters.addParamColour("Background", cGeneral, &iemHelper.secondaryColour, PlugDataColour::guiObjectInternalOutlineColour);
        iemHelper.addIemParameters(objectParameters, false, true, 20, 12, 14);
        setRepaintsOnMouseActivity(true);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto canvasObj = ptr.get<t_my_canvas>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(canvasObj->x_vis_w), var(canvasObj->x_vis_h) });
        }
    }

    bool inletIsSymbol() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(labels);
    }

    void updateHitArea()
    {
        // resize the hit area if the size of the canvas is smaller than the hit area
        if (auto iemgui = ptr.get<t_iemgui>()) {
            hitArea = Rectangle<float>(iemgui->x_w, iemgui->x_h).withPosition(1, 1);
        }
        if (hitArea.getWidth() > getWidth() - 2 || hitArea.getHeight() > getHeight() - 2) {
            auto const shortestLength = jmin(getWidth(), getHeight()) - 2;
            hitArea = Rectangle<float>(1, 1, shortestLength, shortestLength);
        }
        if (getWidth() < 4 || getHeight() < 4) {
            hitArea = getLocalBounds().toFloat();
            hideHitArea = true;
        } else {
            hideHitArea = false;
        }

        repaint();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("size"):
            updateHitArea();
        default:
            iemHelper.receiveObjectMessage(symbol, atoms);
        }
    }

    void update() override
    {
        if (auto cnvObj = ptr.get<t_my_canvas>()) {
            sizeProperty = VarArray { var(cnvObj->x_vis_w), var(cnvObj->x_vis_h) };
        }

        if (auto iemgui = ptr.get<t_iemgui>()) {
            hitAreaSize = iemgui->x_w;
        }

        iemHelper.update();
    }

    Rectangle<int> getSelectableBounds() override
    {
        if (auto cnvObj = ptr.get<t_my_canvas>()) {
            return { cnvObj->x_gui.x_obj.te_xpix, cnvObj->x_gui.x_obj.te_ypix, cnvObj->x_gui.x_w, cnvObj->x_gui.x_h };
        }

        return {};
    }

    void resized() override
    {
        updateHitArea();

        ObjectBase::resized();
    }

    // So we get mouseEnter/Exit notifications for the hitArea
    bool hitTest(int const x, int const y) override
    {
        if (hitArea.contains(x, y)) {
            return true;
        }

        return false;
    }

    bool canReceiveMouseEvent(int const x, int const y) override
    {
        if (hitArea.contains(x - Object::margin, y - Object::margin)) {
            return true;
        }

        return false;
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto cnvObj = ptr.get<t_my_canvas>()) {
            cnvObj->x_gui.x_obj.te_xpix = b.getX();
            cnvObj->x_gui.x_obj.te_ypix = b.getY();
            cnvObj->x_vis_w = b.getWidth() - 1;
            cnvObj->x_vis_h = b.getHeight() - 1;
        }
    }

    static Rectangle<int> getPDSize(t_my_canvas* cnvObj)
    {
        return Rectangle<int>(0, 0, cnvObj->x_vis_w + 1, cnvObj->x_vis_h + 1);
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto canvas = ptr.get<t_my_canvas>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, canvas.cast<t_gobj>(), &x, &y, &w, &h);
            auto const pdSize = getPDSize(ptr.get<t_my_canvas>().get());

            return Rectangle<int>(x, y, pdSize.getWidth(), pdSize.getHeight());
        }

        return {};
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), bgCol, bgCol, Corners::objectCornerRadius);

        if (!cnv->isGraph && !getValue<bool>(object->locked) && !getValue<bool>(object->commandLocked) && !hideHitArea) {
            auto const selectionRectColour = object->isSelected() || isMouseOver() ? cnv->selectedOutlineCol : selectionAreaCol;
            nvgDrawRoundedRect(nvg, hitArea.getX(), hitArea.getY(), hitArea.getWidth(), hitArea.getHeight(), nvgRGBA(0, 0, 0, 0), selectionRectColour, Corners::objectCornerRadius);
        }
    }

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto cnvObj = ptr.get<t_my_canvas>()) {
                cnvObj->x_vis_w = width;
                cnvObj->x_vis_h = height;
            }

            object->updateBounds();
        }
        if (v.refersToSameSourceAs(hitAreaSize)) {
            auto size = getValue<int>(hitAreaSize);
            hitAreaSize = size = jmax(4, size);
            if (auto iemgui = ptr.get<t_iemgui>()) {
                iemgui->x_w = size;
                iemgui->x_h = size;
            }
            updateHitArea();
        } else {
            iemHelper.valueChanged(v);
        }
    }
};
