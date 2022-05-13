/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Components/DraggableNumber.h"

struct Inspector : public PropertyPanel
{
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(getLocalBounds().withHeight(getTotalContentHeight()));
    }

    void loadParameters(ObjectParameters& params)
    {
        StringArray names = {"General", "Appearance", "Label", "Extra"};

        clear();

        auto createPanel = [](int type, const String& name, Value* value, int idx, std::vector<String>& options) -> PropertyComponent*
        {
            switch (type)
            {
                case tString:
                    return new EditableComponent<String>(name, *value, idx);
                case tFloat:
                    return new EditableComponent<float>(name, *value, idx);
                case tInt:
                    return new EditableComponent<int>(name, *value, idx);
                case tColour:
                    return new ColourComponent(name, *value, idx);
                case tBool:
                    return new BoolComponent(name, *value, idx, options);
                case tCombo:
                    return new ComboComponent(name, *value, idx, options);
                default:
                    return new EditableComponent<String>(name, *value, idx);
            }
        };

        for (int i = 0; i < 4; i++)
        {
            Array<PropertyComponent*> panels;

            int idx = 0;
            for (auto& [name, type, category, value, options] : params)
            {
                if (static_cast<int>(category) == i)
                {
                    panels.add(createPanel(type, name, value, idx, options));
                    idx++;
                }
            }
            if (!panels.isEmpty())
            {
                addSection(names[i], panels);
            }
        }
    }

    struct InspectorProperty : public PropertyComponent
    {
        int idx;

        InspectorProperty(const String& propertyName, int i) : PropertyComponent(propertyName, 23), idx(i)
        {
        }

        void paint(Graphics& g) override
        {
            auto bg = idx & 1 ? findColour(PlugDataColour::toolbarColourId) : findColour(PlugDataColour::canvasColourId);

            g.fillAll(bg);
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
        }

        void refresh() override{};
    };

    struct ComboComponent : public InspectorProperty
    {
        ComboComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            for (int n = 0; n < options.size(); n++)
            {
                comboBox.addItem(options[n], n + 1);
            }

            comboBox.setName("inspector:combo");
            comboBox.getSelectedIdAsValue().referTo(value);

            addAndMakeVisible(comboBox);
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        ComboBox comboBox;
    };

    struct BoolComponent : public InspectorProperty
    {
        BoolComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setConnectedEdges(12);

            toggleButton.getToggleStateValue().referTo(value);
            toggleButton.setButtonText(static_cast<bool>(value.getValue()) ? options[1] : options[0]);

            toggleButton.setName("inspector:toggle");

            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this, value, options]() mutable { toggleButton.setButtonText(toggleButton.getToggleState() ? options[1] : options[0]); };
        }

        void resized() override
        {
            toggleButton.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        TextButton toggleButton;
    };

    struct ColourComponent : public InspectorProperty, public ChangeListener
    {
        ColourComponent(const String& propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), currentColour(value)
        {
            String strValue = currentColour.toString();
            if (strValue.length() > 2)
            {
                button.setButtonText(String("#") + strValue.substring(2).toUpperCase());
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            updateColour();

            button.onClick = [this]()
            {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(PlugDataColour::toolbarColourId));

                colourSelector->setCurrentColour(Colour::fromString(currentColour.toString()));

                CallOutBox::launchAsynchronously(std::move(colourSelector), button.getScreenBounds(), nullptr);
            };
        }

        void updateColour()
        {
            auto colour = Colour::fromString(currentColour.toString());

            button.setColour(TextButton::buttonColourId, colour);
            button.setColour(TextButton::buttonOnColourId, colour.brighter());

            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;

            // make sure text is readable
            button.setColour(TextButton::textColourOffId, textColour);
            button.setColour(TextButton::textColourOnId, textColour);

            button.setButtonText(String("#") + currentColour.toString().substring(2).toUpperCase());
        }

        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            auto* cs = dynamic_cast<ColourSelector*>(source);

            auto colour = cs->getCurrentColour();
            currentColour = colour.toString();

            updateColour();
        }

        ~ColourComponent() override = default;

        void resized() override
        {
            button.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        TextButton button;
        Value& currentColour;
    };

    template <typename T>
    struct EditableComponent : public InspectorProperty
    {
        Label label;
        Value& property;
        std::unique_ptr<DraggableNumber> dragger;

        float dragValue = 0.0f;
        bool shift = false;
        int decimalDrag = 0;

        Point<float> lastDragPos;

        EditableComponent(String propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), property(value)
        {
            addAndMakeVisible(label);
            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);
            
            if constexpr (std::is_arithmetic<T>::value)  {
                dragger = std::make_unique<DraggableNumber>(label);
                
                dragger->valueChanged = [this](float value){
                    property = value;
                };
            };
            
            label.setFont(Font(14));

            label.onEditorShow = [this]()
            {
                auto* editor = label.getCurrentTextEditor();

                if constexpr (std::is_floating_point<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789.-");
                }
                else if constexpr (std::is_integral<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789-");
                }
            };
            

        }

        void resized() override
        {
            label.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
    };
};
