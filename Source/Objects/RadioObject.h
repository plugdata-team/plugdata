
struct RadioObject final : public IEMObject {

    bool alreadyToggled = false;
    bool isVertical;

    RadioObject(bool vertical, void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
        isVertical = vertical;

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

        if (isVertical) {
            auto* dial = static_cast<t_vdial*>(ptr);

            dial->x_gui.x_w = getWidth();
            dial->x_gui.x_h = getHeight() / dial->x_number;
        } else {
            auto* dial = static_cast<t_hdial*>(ptr);
            dial->x_gui.x_w = getWidth() / dial->x_number;
            dial->x_gui.x_h = getHeight();
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
        return isVertical ? (static_cast<t_vdial*>(ptr))->x_on : (static_cast<t_hdial*>(ptr))->x_on;
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

        if (isVertical) {
            auto* dial = static_cast<t_vdial*>(ptr);
            bounds.setSize(dial->x_gui.x_w, dial->x_gui.x_h * dial->x_number);
        } else {
            auto* dial = static_cast<t_hdial*>(ptr);
            bounds.setSize(dial->x_gui.x_w * dial->x_number, dial->x_gui.x_h);
        }

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    
    void paint(Graphics& g) override
    {
        g.setColour(getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
        
        
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
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
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
        return isVertical ? (static_cast<t_vdial*>(ptr))->x_number : (static_cast<t_hdial*>(ptr))->x_number;
    }

    void setMaximum(float value)
    {
        if(getValueOriginal() >= value) {
            setValueOriginal(value - 1);
        }
        if (isVertical) {
            static_cast<t_vdial*>(ptr)->x_number = value;
        } else {
            static_cast<t_hdial*>(ptr)->x_number = value;
        }
        
        resized();
    }
};
