
struct RadioObject : public IEMObject
{
    int lastState = 0;

    Value numButtons = Value(8.0f);

    bool isVertical;

    RadioObject(bool vertical, void* obj, Box* parent) : IEMObject(obj, parent)
    {
        isVertical = vertical;

        numButtons = getMaximum();

        initialise();
        updateRange();

        numButtons.addListener(this);

        int selected = getValueOriginal();

        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
    }

    void resized() override
    {
        int size = isVertical ? getWidth() : getHeight();
        int minSize = 12;
        size = std::max(size, minSize);

        for (int i = 0; i < radioButtons.size(); i++)
        {
            if (isVertical)
                radioButtons[i]->setBounds(0, i * size, size, size);
            else
                radioButtons[i]->setBounds(i * size, 0, size, size);
        }

        // Fix aspect ratio
        if (isVertical)
        {
            box->setSize(std::max(box->getWidth(), minSize + Box::doubleMargin), size * radioButtons.size() + Box::doubleMargin);
        }
        else
        {
            box->setSize(size * radioButtons.size() + Box::doubleMargin, std::max(box->getHeight(), minSize + Box::doubleMargin));
        }

        if (isVertical)
        {
            auto* dial = static_cast<t_vdial*>(ptr);

            dial->x_gui.x_w = getWidth();
            dial->x_gui.x_h = getHeight() / dial->x_number;
        }
        else
        {
            auto* dial = static_cast<t_hdial*>(ptr);
            dial->x_gui.x_w = getWidth() / dial->x_number;
            dial->x_gui.x_h = getHeight();
        }
    }

    float getValue() override
    {
        return isVertical ? (static_cast<t_vdial*>(ptr))->x_on : (static_cast<t_hdial*>(ptr))->x_on;
    }

    void update() override
    {
        int selected = getValueOriginal();

        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
    }

    void updateBounds() override
    {
        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        Rectangle<int> bounds;
        if (isVertical)
        {
            auto* dial = static_cast<t_vdial*>(ptr);
            bounds = {x, y, dial->x_gui.x_w, dial->x_gui.x_h * dial->x_number};
        }
        else
        {
            auto* dial = static_cast<t_hdial*>(ptr);
            bounds = {x, y, dial->x_gui.x_w * dial->x_number, dial->x_gui.x_h};
        }

        box->setBounds(bounds.expanded(Box::margin));
    }

    // To make it trigger on mousedown instead of mouseup
    void mouseDown(const MouseEvent& e) override
    {
        for (int i = 0; i < numButtons; i++)
        {
            if (radioButtons[i]->getBounds().contains(e.getEventRelativeTo(this).getPosition()))
            {
                radioButtons[i]->triggerClick();
            }
        }
    }

    void updateRange()
    {
        radioButtons.clear();
        
       

        for (int i = 0; i < numButtons; i++)
        {
            radioButtons.add(new TextButton);
            radioButtons[i]->setConnectedEdges(12);
            radioButtons[i]->setRadioGroupId(1001);
            radioButtons[i]->setClickingTogglesState(true);
            radioButtons[i]->setName("radiobutton");
            radioButtons[i]->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(radioButtons[i]);

            radioButtons[i]->onClick = [this, i]() mutable
            {
                if (!radioButtons[i]->getToggleState()) return;

                lastState = i;

                startEdition();
                setValueOriginal(i);
                stopEdition();
            };
        }

        int idx = getValueOriginal();
        radioButtons[idx]->setToggleState(true, dontSendNotification);
        
        if(getWidth() != 0 && getHeight() != 0) {
            resized();
        }
    }

    OwnedArray<TextButton> radioButtons;

    ObjectParameters defineParameters() override
    {
        return {{"Options", tInt, cGeneral, &numButtons, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(numButtons))
        {
            setMaximum(static_cast<int>(numButtons.getValue()));
            updateRange();
        }
        else
        {
            IEMObject::valueChanged(value);
        }
    }

    float getMaximum()
    {
        return isVertical ? (static_cast<t_vdial*>(ptr))->x_number : (static_cast<t_hdial*>(ptr))->x_number;
    }

    void setMaximum(float value)
    {
        if (isVertical)
        {
            static_cast<t_vdial*>(ptr)->x_number = value;
        }
        else
        {
            static_cast<t_hdial*>(ptr)->x_number = value;
        }
    }
};
