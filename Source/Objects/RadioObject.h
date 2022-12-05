/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct RadioObject final : public IEMObject {

    bool alreadyToggled = false;
    bool isVertical;

    RadioObject(void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
        isVertical = static_cast<t_radio*>(obj)->x_orientation;

        max = getMaximum();
        max.addListener(this);

        int selected = getValueOriginal();
        if (selected > static_cast<int>(max.getValue())) {
            setValueOriginal(std::min<int>(static_cast<int>(max.getValue()) - 1, selected));
        }
    }

    void resized() override
    {
        int size = (isVertical ? getWidth() : getHeight());
        int minSize = 12;
        size = std::max(size, minSize);
        
        int numItems = static_cast<int>(max.getValue());

        // Fix aspect ratio
        if (isVertical) {
            object->setSize(std::max(object->getWidth(), minSize + Object::doubleMargin), size * numItems + Object::doubleMargin);
        } else {
            object->setSize(size * numItems + Object::doubleMargin, std::max(object->getHeight(), minSize + Object::doubleMargin));
        }
    }
    
    void applyBounds() override
    {
        auto* radio = static_cast<t_radio*>(ptr);
        
        if (isVertical) {
            radio->x_gui.x_w = getWidth();
            radio->x_gui.x_h = getHeight() / radio->x_number;
        } else {
            radio->x_gui.x_w = getWidth() / radio->x_number;
            radio->x_gui.x_h = getHeight();
        }
    }

    void toggleObject(Point<int> position) override
    {
        if(alreadyToggled)  {
            alreadyToggled = false;
        }
        
        float pos = isVertical ? position.y : position.x;
        float div = isVertical ? getHeight() : getWidth();
        int numItems = static_cast<int>(max.getValue());
        
        int idx = std::clamp<int>((pos / div) * numItems, 0, numItems - 1);
        
        if(idx != static_cast<int>(getValueOriginal())) {
            startEdition();
            setValueOriginal(idx);
            stopEdition();
            repaint();
        }
    }
    
    void receiveObjectMessage(const String& symbol, std::vector<pd::Atom>& atoms) override {
        
        if(symbol == "orientation" && atoms.size() >= 1) {
            isVertical = static_cast<bool>(atoms[0].getFloat());
            updateBounds();
        }
        else if(symbol == "number" && atoms.size() >= 1) {
            setParameterExcludingListener(max, static_cast<int>(atoms[0].getFloat()));
        }
        else {
            IEMObject::receiveObjectMessage(symbol, atoms);
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
        int numItems = static_cast<int>(max.getValue());
        
        int idx = (pos / div) * numItems;
        
        alreadyToggled = true;
        startEdition();
        setValueOriginal(idx);
        stopEdition();
        
        repaint();
    }

    float getValue() override
    {
        return static_cast<t_radio*>(ptr)->x_on;
    }

    void update() override
    {
        repaint();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        auto* radio = static_cast<t_radio*>(ptr);
        
        if (isVertical) {
            bounds.setSize(radio->x_gui.x_w, radio->x_gui.x_h);
        } else {
            bounds.setSize(radio->x_gui.x_w, radio->x_gui.x_h);
        }

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    
    void paint(Graphics& g) override
    {
        g.setColour(getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Constants::objectCornerRadius);
        
        int size = (isVertical ? getWidth() : getHeight());
        int minSize = 12;
        size = std::max(size, minSize);
        
        g.setColour(object->findColour(PlugDataColour::objectOutlineColourId));
        
        for (int i = 1; i < static_cast<int>(max.getValue()); i++) {
            if (isVertical)
            {
                g.drawLine(0, i * size, size, i * size);
            }
            else
            {
                g.drawLine(i * size, 0, i * size, size);
            }
        }
        
        g.setColour(getForegroundColour());
        
        int currentValue = getValueOriginal();
        int selectionX = isVertical ? 0 : currentValue * size;
        int selectionY = isVertical ? currentValue * size : 0;

        auto selectionBounds = Rectangle<int>(selectionX, selectionY, size, size);
        g.fillRect(selectionBounds.reduced(5));
    }

    void paintOverChildren(Graphics& g) override
    {

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Constants::objectCornerRadius, 1.0f);
    }

    ObjectParameters defineParameters() override
    {
        return { { "Options", tInt, cGeneral, &max, {} } };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(max)) {
            setMaximum(limitValueMin(value, 1));
            repaint();
        } else {
            IEMObject::valueChanged(value);
        }
    }

    float getMaximum()
    {
        return static_cast<t_radio*>(ptr)->x_number;
    }

    void setMaximum(float value)
    {
        if(getValueOriginal() >= value) {
            setValueOriginal(value - 1);
        }
        
        static_cast<t_radio*>(ptr)->x_number = value;
        
        resized();
    }
};
