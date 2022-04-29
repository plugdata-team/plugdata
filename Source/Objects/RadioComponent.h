
struct RadioComponent : public GUIComponent
{
    int lastState = 0;

    Value numButtons = Value(8.0f);

    bool isVertical;

    RadioComponent(bool vertical, const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        isVertical = vertical;

        numButtons = gui.getMaximum();

        initialise(newObject);
        updateRange();

        numButtons.addListener(this);

        int selected = getValueOriginal();

        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
    }

    template <typename T>
    T roundMultiple(T value, T multiple)
    {
        if (multiple == 0) return value;
        return static_cast<T>(std::round(static_cast<double>(value) / static_cast<double>(multiple)) * static_cast<double>(multiple));
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
    }

    void update() override
    {
        int selected = getValueOriginal();

        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
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
        numButtons = gui.getMaximum();

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

        resized();
        // box->updateBounds(false);
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
            gui.setMaximum(static_cast<int>(numButtons.getValue()));
            updateRange();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};
