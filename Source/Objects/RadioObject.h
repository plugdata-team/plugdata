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
        selected = getValue();

        if (selected > ::getValue<int>(max)) {
            selected = std::min<int>(::getValue<int>(max) - 1, selected);
        }

        if (auto radio = ptr.get<t_radio>()) {
            isVertical = radio->x_orientation;
            sizeProperty = isVertical ? radio->x_gui.x_w : radio->x_gui.x_h;
        }

        numItems = getMaximum();
        max = numItems;

        iemHelper.update();

        onConstrainerCreate = [this]() {
            updateAspectRatio();
        };
    }

    bool hideInlets() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
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

    float getValue()
    {
        return ptr.get<t_radio>()->x_on;
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        float size = (isVertical ? static_cast<float>(getHeight()) / numItems : static_cast<float>(getWidth()) / numItems);

        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));

        for (int i = 1; i < numItems; i++) {
            if (isVertical) {
                g.drawLine(0, i * size, size, i * size);
            } else {
                g.drawLine(i * size, 0, i * size, size);
            }
        }

        g.setColour(iemHelper.getForegroundColour());

        float selectionX = isVertical ? 0 : selected * size;
        float selectionY = isVertical ? selected * size : 0;

        auto selectionBounds = Rectangle<float>(selectionX, selectionY, size, size);
        
        g.fillRoundedRectangle(selectionBounds.reduced(jmin<int>(size * 0.25f, 5)), Corners::objectCornerRadius / 2.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }
    
    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f).toFloat();
        bool isSelected = object->isSelected() && !cnv->isGraph;
        auto selectedOutlineColour = convertColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(object->findColour(PlugDataColour::objectOutlineColourId));
 
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(iemHelper.getBackgroundColour()), isSelected ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        float size = (isVertical ? static_cast<float>(getHeight()) / numItems : static_cast<float>(getWidth()) / numItems);

        nvgStrokeColor(nvg, convertColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour)));
        nvgStrokeWidth(nvg, 1.0f);

        for (int i = 1; i < numItems; i++) {
            if (isVertical) {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, 0, i * size);
                nvgLineTo(nvg, size, i * size);
                nvgStroke(nvg);
            } else {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, i * size, 0);
                nvgLineTo(nvg, i * size, size);
                nvgStroke(nvg);
            }
        }

        nvgFillColor(nvg, convertColour(iemHelper.getForegroundColour()));

        float selectionX = isVertical ? 0 : selected * size;
        float selectionY = isVertical ? selected * size : 0;
        auto selectionBounds = Rectangle<float>(selectionX, selectionY, size, size).reduced(jmin<int>(size * 0.25f, 5));

        nvgBeginPath(nvg);
        nvgRoundedRect(nvg, selectionBounds.getX(), selectionBounds.getY(), selectionBounds.getWidth(), selectionBounds.getHeight(), Corners::objectCornerRadius / 2.0f);
        nvgFill(nvg);
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
        return ptr.get<t_radio>()->x_number;
    }

    void setMaximum(float maxValue)
    {
        if (selected >= maxValue) {
            selected = maxValue - 1;
        }

        ptr.get<t_radio>()->x_number = maxValue;

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
