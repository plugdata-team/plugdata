/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class CanvasObject final : public ObjectBase {

    bool locked;
    Value sizeProperty = SynchronousValue();

    IEMHelper iemHelper;

public:
    CanvasObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        object->setColour(PlugDataColour::outlineColourId, Colours::transparentBlack);
        locked = getValue<bool>(object->locked);

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColour("Canvas color", cGeneral, &iemHelper.secondaryColour, PlugDataColour::guiObjectInternalOutlineColour);
        iemHelper.addIemParameters(objectParameters, false, true, 20, 12, 14);
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto canvasObj = ptr.get<t_my_canvas>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(canvasObj->x_vis_w), var(canvasObj->x_vis_h) });
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

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
    }

    void update() override
    {
        if (auto cnvObj = ptr.get<t_my_canvas>()) {
            sizeProperty = Array<var> { var(cnvObj->x_vis_w), var(cnvObj->x_vis_h) };
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

    bool canReceiveMouseEvent(int x, int y) override
    {
        if (auto iemgui = ptr.get<t_iemgui>()) {
            return !locked && Rectangle<int>(iemgui->x_w, iemgui->x_h).contains(x - Object::margin, y - Object::margin);
        }

        return false;
    }

    void lock(bool isLocked) override
    {
        locked = isLocked;
    }

    void setPdBounds(Rectangle<int> b) override
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
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, canvas.cast<t_gobj>(), &x, &y, &w, &h);
            auto const pdSize = getPDSize(ptr.get<t_my_canvas>().get());

            return Rectangle<int>(x, y, pdSize.getWidth(), pdSize.getHeight());
        }

        return {};
    }

    void render(NVGcontext* nvg) override
    {
        Colour bgcolour = Colour::fromString(iemHelper.secondaryColour.toString());
        auto b = getLocalBounds().toFloat();

        nvgFillColor(nvg, convertColour(bgcolour));
        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
        nvgFill(nvg);

        if (!locked) {
            Rectangle<float> draggableRect;
            if (auto iemgui = ptr.get<t_iemgui>()) {
                draggableRect = Rectangle<float>(iemgui->x_w, iemgui->x_h);
            } else {
                return;
            }

            nvgStrokeColor(nvg, convertColour(object->isSelected() ? cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId) : bgcolour.contrasting(0.75f)));
            nvgStrokeWidth(nvg, 1.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, draggableRect.getX() + 1.0f, draggableRect.getY() + 1.0f, draggableRect.getWidth() - 2.0f, draggableRect.getHeight() - 2.0f, Corners::objectCornerRadius);
            nvgStroke(nvg);
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto cnvObj = ptr.get<t_my_canvas>()) {
                cnvObj->x_vis_w = width;
                cnvObj->x_vis_h = height;
            }

            object->updateBounds();
        } else {
            iemHelper.valueChanged(v);
        }
    }
};
