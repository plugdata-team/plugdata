#include "../Components/DraggableNumber.h"

struct NumberComponent : public GUIComponent
{    
    DraggableNumber dragger;

    NumberComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject), dragger(input)
    {
        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            if (!gui.isAtom())
            {
                editor->setBorder({0, 10, 0, 0});
            }

            if (editor != nullptr)
            {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]()
        {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };

        if (!gui.isAtom())
        {
            input.setBorderSize({1, 15, 1, 1});
        }

        addAndMakeVisible(input);

        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);

        initialise(newObject);

        input.setEditable(true, false);

        addMouseListener(this, true);
        
        dragger.dragStart = [this](){
            startEdition();
        };
        
        dragger.valueChanged = [this](float value){
            setValueOriginal(value);
        };
        
        dragger.dragEnd = [this](){
            stopEdition();
        };
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(30, maxSize, box->getWidth());
        int h = jlimit(Box::height - 12, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }


    void update() override
    {
        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);
    }

    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tFloat, cGeneral, &min, {}}, {"Maximum", tFloat, cGeneral, &max, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
        else if (gui.isAtom() && value.refersToSameSourceAs(labelHeight))
        {
            updateLabel();
            box->updateBounds(false);  // update box size based on new font
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);

        if (gui.isAtom()) return;

        const int indent = 9;

        const Rectangle<int> iconBounds = getLocalBounds().withWidth(indent - 4).withHeight(getHeight() - 8).translated(4, 4);

        Path corner;

        corner.addTriangle(iconBounds.getTopLeft().toFloat(), iconBounds.getTopRight().toFloat() + Point<float>(0, (iconBounds.getHeight() / 2.)), iconBounds.getBottomLeft().toFloat());

        
        g.setColour(Colour(gui.getForegroundColour()).interpolatedWith(box->findColour(PlugDataColour::toolbarColourId), 0.5f));
        g.fillPath(corner);
    }
};
