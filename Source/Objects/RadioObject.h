/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <JuceHeader.h>

class RadioObject final : public ObjectBase {

    bool alreadyToggled = false;
    bool isVertical;
    int numItems = 0;

    int selected;

    IEMHelper iemHelper;

    Value max = Value(0.0f);

public:
    RadioObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        isVertical = static_cast<t_radio*>(ptr)->x_orientation;

        max = getMaximum();
        max.addListener(this);

        selected = getValue();

        valueChanged(max);

        if (selected > static_cast<int>(max.getValue())) {
            selected = std::min<int>(static_cast<int>(max.getValue()) - 1, selected);
        }
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

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();

        // radio stores it's height and width as a square to allow changing orientation via message: "orientation 0/1"
        b = isVertical ? b.withHeight(b.getWidth()) : b.withWidth(b.getHeight());
        iemHelper.applyBounds(b);
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

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("set"): {
            selected = std::clamp<float>(atoms[0].getFloat(), 0.0f, numItems - 1);
            repaint();
            break;
        }
        case hash("orientation"): {
            if (atoms.size() >= 1) {
                isVertical = static_cast<bool>(atoms[0].getFloat());
                updateBounds();
                updateAspectRatio();
            }
            break;
        }
        case hash("number"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(max, static_cast<int>(atoms[0].getFloat()));
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
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
        return static_cast<t_radio*>(ptr)->x_on;
    }

    void updateBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        auto* radio = static_cast<t_radio*>(ptr);

        if (isVertical) {
            bounds.setSize(radio->x_gui.x_w, radio->x_gui.x_w * numItems);
        } else {
            bounds.setSize(radio->x_gui.x_h * numItems, radio->x_gui.x_h);
        }

        pd->unlockAudioThread();

        object->setObjectBounds(bounds);
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        int size = (isVertical ? getHeight() / numItems : getHeight());
        // int minSize = 12;
        // size = std::max(size, minSize);

        g.setColour(object->findColour(PlugDataColour::objectOutlineColourId));

        for (int i = 1; i < numItems; i++) {
            if (isVertical) {
                g.drawLine(0, i * size, size, i * size);
            } else {
                g.drawLine(i * size, 0, i * size, size);
            }
        }

        g.setColour(iemHelper.getForegroundColour());

        int currentValue = selected;
        int selectionX = isVertical ? 0 : currentValue * size;
        int selectionY = isVertical ? currentValue * size : 0;

        auto selectionBounds = Rectangle<int>(selectionX, selectionY, size, size);
        g.fillRoundedRectangle(selectionBounds.reduced(5).toFloat(), PlugDataLook::objectCornerRadius / 2.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = { { "Options", tInt, cGeneral, &max, {} } };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    void updateAspectRatio()
    {
        float verticalLength = ((object->getWidth() - Object::doubleMargin) * numItems) + Object::doubleMargin;
        float horizontalLength = ((object->getHeight() - Object::doubleMargin) * numItems) + Object::doubleMargin;

        auto minLongSide = object->minimumSize * numItems;
        auto minShortSide = object->minimumSize;
        if (isVertical) {
            object->setSize(object->getWidth(), verticalLength);
            object->constrainer->setMinimumSize(minShortSide, minLongSide);
        } else {
            object->setSize(horizontalLength, object->getHeight());
            object->constrainer->setMinimumSize(minLongSide, minShortSide);
        }
        object->constrainer->setFixedAspectRatio(isVertical ? 1.0f / numItems : static_cast<float>(numItems) / 1.0f);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(max)) {
            if (static_cast<int>(max.getValue()) != numItems) {
                limitValueMin(value, 1);
                numItems = static_cast<int>(max.getValue());
                updateAspectRatio();
                setMaximum(numItems);
            }
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getMaximum()
    {
        return static_cast<t_radio*>(ptr)->x_number;
    }

    void setMaximum(float maxValue)
    {
        if (selected >= maxValue) {
            selected = maxValue - 1;
        }

        static_cast<t_radio*>(ptr)->x_number = maxValue;

        resized();
    }
};
