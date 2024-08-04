/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class RadioObject final : public ObjectBase {

    bool alreadyToggled = false;
    bool isVertical;
    int numItems = 0;

    int selected;

    int hoverIdx = -1;
    bool mouseHover = false;

    IEMHelper iemHelper;

    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

public:
    RadioObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamInt("Options", cGeneral, &max, 8);
        iemHelper.addIemParameters(objectParameters);

        if (auto radio = ptr.get<t_radio>()) {
            isVertical = radio->x_orientation;
            sizeProperty = isVertical ? radio->x_gui.x_w : radio->x_gui.x_h;
        }
    }

    void update() override
    {
        numItems = getMaximum();
        max = numItems;

        selected = jlimit(0, numItems - 1, static_cast<int>(getValue()));

        if (auto radio = ptr.get<t_radio>()) {
            isVertical = radio->x_orientation;
            sizeProperty = isVertical ? radio->x_gui.x_w : radio->x_gui.x_h;
        }

        iemHelper.update();

        onConstrainerCreate = [this]() {
            updateAspectRatio();
        };
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

    void setPdBounds(Rectangle<int> b) override
    {
        // radio stores it's height and width as a square to allow changing orientation via message: "orientation 0/1"
        b = isVertical ? b.withHeight(b.getWidth()) : b.withWidth(b.getHeight());
        iemHelper.setPdBounds(b);
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto radio = ptr.get<t_radio>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, radio.cast<t_gobj>(), &x, &y, &w, &h);
            auto width = !isVertical ? (radio->x_gui.x_h * numItems) + 1 : (radio->x_gui.x_w + 1);
            auto height = isVertical ? (radio->x_gui.x_w * numItems) + 1 : (radio->x_gui.x_h + 1);

            return { x, y, width, height };
        }

        return {};
    }

    void toggleObject(Point<int> position) override
    {
        if (alreadyToggled) {
            alreadyToggled = false;
        }

        float pos = isVertical ? position.y : position.x;
        float div = isVertical ? getHeight() : getWidth();

        int idx = std::clamp<int>((pos / div) * numItems, 0, numItems - 1);

        if (idx != static_cast<int>(selected)) {
            startEdition();
            sendFloatValue(idx);
            stopEdition();
            repaint();
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            selected = std::clamp<float>(atoms[0].getFloat(), 0.0f, numItems - 1);
            repaint();
            break;
        }
        case hash("orientation"): {
            if (numAtoms >= 1) {
                isVertical = static_cast<bool>(atoms[0].getFloat());
                object->updateBounds();
                updateAspectRatio();
            }
            break;
        }
        case hash("number"): {
            if (numAtoms >= 1)
                max = getMaximum();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            break;
        }
        }
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
    }

    void mouseMove(MouseEvent const& e) override
    {
        float pos = isVertical ? e.y : e.x;
        float div = isVertical ? getHeight() : getWidth();

        hoverIdx = (pos / div) * numItems;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        float pos = isVertical ? e.y : e.x;
        float div = isVertical ? getHeight() : getWidth();

        int idx = (pos / div) * numItems;

        alreadyToggled = true;
        startEdition();
        sendFloatValue(idx);
        stopEdition();

        repaint();
    }
    
    void mouseEnter(MouseEvent const& e) override
    {
        mouseHover = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        mouseHover = false;
        repaint();
    }

    float getValue()
    {
        if(auto radio = ptr.get<t_radio>()) return radio->x_on;
        
        return 0.0f;
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        bool isSelected = object->isSelected() && !cnv->isGraph;
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(iemHelper.getBackgroundColour()), isSelected ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        float size = (isVertical ? static_cast<float>(getHeight()) / numItems : static_cast<float>(getWidth()) / numItems);

        nvgStrokeColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour)));
        nvgStrokeWidth(nvg, 1.0f);

        for (int i = 1; i < numItems; i++) {
            if (isVertical) {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, 1, i * size);
                nvgLineTo(nvg, size - 0.5, i * size);
                nvgStroke(nvg);
            } else {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, i * size, 1);
                nvgLineTo(nvg, i * size, size - 0.5);
                nvgStroke(nvg);
            }
        }

        auto bgColour = ::getValue<Colour>(iemHelper.secondaryColour);

        if (mouseHover) {
            auto hoverColour = bgColour.contrasting(bgColour.getBrightness() > 0.5f ? 0.03f : 0.05f);
            float hoverX = isVertical ? 0 : hoverIdx * size;
            float hoverY = isVertical ? hoverIdx * size : 0;
            auto hoverBounds = Rectangle<float>(hoverX, hoverY, size, size).reduced(jmin<int>(size * 0.25f, 5));
            nvgFillColor(nvg, convertColour(hoverColour));
            nvgFillRoundedRect(nvg, hoverBounds.getX(), hoverBounds.getY(), hoverBounds.getWidth(), hoverBounds.getHeight(), Corners::objectCornerRadius / 2.0f);
        }

        float selectionX = isVertical ? 0 : selected * size;
        float selectionY = isVertical ? selected * size : 0;
        auto selectionBounds = Rectangle<float>(selectionX, selectionY, size, size).reduced(jmin<int>(size * 0.25f, 5));

        nvgFillColor(nvg, convertColour(::getValue<Colour>(iemHelper.primaryColour)));
        nvgFillRoundedRect(nvg, selectionBounds.getX(), selectionBounds.getY(), selectionBounds.getWidth(), selectionBounds.getHeight(), Corners::objectCornerRadius / 2.0f);
    }

    void updateAspectRatio()
    {
        auto b = getPdBounds();
        float verticalLength = (b.getWidth() * numItems) + Object::doubleMargin;
        float horizontalLength = (b.getHeight() * numItems) + Object::doubleMargin;

        auto minLongSide = object->minimumSize * numItems;
        auto minShortSide = object->minimumSize;
        if (isVertical) {
            object->setSize(b.getWidth() + Object::doubleMargin, verticalLength);
            constrainer->setMinimumSize(minShortSide, minLongSide);
        } else {
            object->setSize(horizontalLength, b.getHeight() + Object::doubleMargin);
            constrainer->setMinimumSize(minLongSide, minShortSide);
        }
        constrainer->setFixedAspectRatio(isVertical ? 1.0f / numItems : static_cast<float>(numItems) / 1.0f);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto size = std::max(::getValue<int>(sizeProperty), isVertical ? constrainer->getMinimumWidth() : constrainer->getMinimumHeight());
            setParameterExcludingListener(sizeProperty, size);

            if (auto radio = ptr.get<t_radio>()) {
                if (isVertical) {
                    radio->x_gui.x_w = size;
                    radio->x_gui.x_h = size;
                } else {
                    radio->x_gui.x_h = size;
                    radio->x_gui.x_w = size;
                }
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(max)) {
            if (::getValue<int>(max) != numItems) {
                limitValueMin(value, 1);
                numItems = ::getValue<int>(max);
                updateAspectRatio();
                setMaximum(numItems);
            }
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getMaximum()
    {
        if(auto radio = ptr.get<t_radio>())
        {
            return radio->x_number;
        }
        
        return 0.0f;
    }

    void setMaximum(float maxValue)
    {
        if (selected >= maxValue) {
            selected = maxValue - 1;
        }

        if(auto radio = ptr.get<t_radio>())
        {
            radio->x_number = maxValue;
        }

        resized();
    }

    void updateSizeProperty() override
    {
        if (auto radio = ptr.get<t_radio>()) {
            auto size = isVertical ? object->getWidth() : object->getHeight();
            size -= (Object::doubleMargin + 1);

            radio->x_gui.x_w = size;
            radio->x_gui.x_h = size;

            setParameterExcludingListener(sizeProperty, isVertical ? var(radio->x_gui.x_w) : var(radio->x_gui.x_h));
        }
    }
};
