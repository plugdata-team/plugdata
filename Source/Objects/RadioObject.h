
struct RadioObject final : public IEMObject
{
    int lastState = 0;

    bool isVertical;

    RadioObject(bool vertical, void* obj, Box* parent) : IEMObject(obj, parent)
    {
        isVertical = vertical;

        max = getMaximum();

        initialise();
        updateRange();

        max.addListener(this);

        int selected = getValueOriginal();

        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
    }

    void resized() override
    {
        int size = (isVertical ? getWidth() : getHeight());
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
            box->setObjectBounds({x, y, dial->x_gui.x_w, dial->x_gui.x_h * dial->x_number});
        }
        else
        {
            auto* dial = static_cast<t_hdial*>(ptr);
            box->setObjectBounds({x, y, dial->x_gui.x_w * dial->x_number, dial->x_gui.x_h});
        }
    }

    void updateRange()
    {
        radioButtons.clear();

        for (int i = 0; i < max; i++)
        {
            radioButtons.add(new TextButton);
            radioButtons[i]->setConnectedEdges(12);
            radioButtons[i]->setRadioGroupId(1001);
            radioButtons[i]->setClickingTogglesState(true);
            radioButtons[i]->setName("radiobutton");
            radioButtons[i]->setInterceptsMouseClicks(true, false);
            radioButtons[i]->setTriggeredOnMouseDown(true);
            
            radioButtons[i]->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            radioButtons[i]->setColour(TextButton::buttonColourId, Colours::transparentBlack);
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

        if (getWidth() != 0 && getHeight() != 0)
        {
            resized();
        }
    }
    
    void paintOverChildren(Graphics& g) override
    {
        bool skipped = false;
        for(auto& button : radioButtons)
        {
            if(!skipped) {
                skipped = true;
                continue;
            }
            g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
            if(isVertical)
            {
                g.drawLine({button->getBounds().getTopLeft().toFloat(), button->getBounds().getTopRight().toFloat()}, 1.0f);
            }
            else {
                g.drawLine({button->getBounds().getTopLeft().toFloat(), button->getBounds().getBottomLeft().toFloat()}, 1.0f);
            }
            
        }

        auto outlineColour = box->findColour(cnv->isSelected(box) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }
    
    OwnedArray<TextButton> radioButtons;

    ObjectParameters defineParameters() override
    {
        return {{"Options", tInt, cGeneral, &max, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(max))
        {
            setMaximum(static_cast<int>(max.getValue()));
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
