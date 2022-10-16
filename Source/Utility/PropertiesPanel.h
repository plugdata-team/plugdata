#pragma once
#include "DraggableNumber.h"

struct PropertiesPanel : public PropertyPanel {
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRect(getLocalBounds().withHeight(getTotalContentHeight()));
    }

    struct Property : public PropertyComponent {
        int idx;
        bool hideLabel = false;

        Property(String const& propertyName, int i)
            : PropertyComponent(propertyName, 23)
            , idx(i)
        {
        }

        void setHideLabel(bool labelHidden)
        {
            hideLabel = labelHidden;
            repaint();
            resized();
        }

        void paint(Graphics& g) override
        {
            if (hideLabel)
                return;

            auto bg = idx & 1 ? findColour(PlugDataColour::panelBackgroundOffsetColourId) : findColour(PlugDataColour::panelBackgroundColourId);

            g.fillAll(bg);
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
        }

        void refresh() override {};
    };

    struct ComboComponent : public Property {
        ComboComponent(String const& propertyName, Value& value, int idx, std::vector<String> options)
            : Property(propertyName, idx)
        {
            for (int n = 0; n < options.size(); n++) {
                comboBox.addItem(options[n], n + 1);
            }

            comboBox.setName("inspector:combo");
            comboBox.getSelectedIdAsValue().referTo(value);

            addAndMakeVisible(comboBox);
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

    private:
        ComboBox comboBox;
    };

    // Combobox entry that displays the font name with that font
    struct FontEntry : public PopupMenu::CustomComponent {
        String fontName;
        FontEntry(String name)
            : fontName(name)
        {
        }

        void paint(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::panelTextColourId));

            auto font = Font(fontName, 15, Font::plain);
            g.setFont(font);
            g.drawText(fontName, getLocalBounds().reduced(2), Justification::left);
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 150;
            idealHeight = 23;
        }
    };

    struct FontComponent : public Property {
        Value fontValue;
        StringArray options = Font::findAllTypefaceNames();

        FontComponent(String const& propertyName, Value& value, int idx)
            : Property(propertyName, idx)
        {
            options.addIfNotAlreadyThere("Inter");

            for (int n = 0; n < options.size(); n++) {
                comboBox.getRootMenu()->addCustomItem(n + 1, std::make_unique<FontEntry>(options[n]), nullptr, options[n]);
            }

            comboBox.setText(value.toString());
            comboBox.setName("inspector:combo");
            fontValue.referTo(value);

            comboBox.onChange = [this]() {
                fontValue.setValue(options[comboBox.getSelectedItemIndex()]);
            };

            addAndMakeVisible(comboBox);
        }

        void setFont(String fontName)
        {
            fontValue.setValue(fontValue);
            comboBox.setText(fontName);
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

    private:
        ComboBox comboBox;
    };

    struct BoolComponent : public Property {
        BoolComponent(String const& propertyName, Value& value, int idx, std::vector<String> options)
            : Property(propertyName, idx)
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
            toggleButton.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

    private:
        TextButton toggleButton;
    };

    struct ColourComponent : public Property
        , public ChangeListener {
        ColourComponent(String const& propertyName, Value& value, int idx)
            : Property(propertyName, idx)
            , currentColour(value)
        {
            String strValue = currentColour.toString();
            if (strValue.length() > 2) {
                button.setButtonText(String("#") + strValue.substring(2).toUpperCase());
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            updateColour();

            button.onClick = [this]() {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));
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
            button.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

    private:
        TextButton button;
        Value& currentColour;
    };

    struct RangeComponent : public Property {
        Value& property;

        DraggableNumber minLabel, maxLabel;

        float min, max;

        RangeComponent(String propertyName, Value& value, int idx)
            : Property(propertyName, idx)
            , property(value)
            , minLabel(false)
            , maxLabel(false)
        {
            min = value.getValue().getArray()->getReference(0);
            max = value.getValue().getArray()->getReference(1);

            addAndMakeVisible(minLabel);
            minLabel.setEditableOnClick(true);
            minLabel.addMouseListener(this, true);
            minLabel.setText(String(min), dontSendNotification);

            addAndMakeVisible(maxLabel);
            maxLabel.setEditableOnClick(true);
            maxLabel.addMouseListener(this, true);
            maxLabel.setText(String(max), dontSendNotification);

            auto setMinimum = [this](float value) {
                min = value;
                Array<var> arr = { min, max };
                //maxLabel.setMinimum(min + 1e-5f);
                property = var(arr);
            };

            auto setMaximum = [this](float value) {
                max = value;
                Array<var> arr = { min, max };
                //minLabel.setMaximum(max - 1e-5f);
                property = var(arr);
            };

            minLabel.valueChanged = setMinimum;
            minLabel.onTextChange = [this, setMinimum]() {
                setMinimum(minLabel.getText().getFloatValue());
            };

            maxLabel.valueChanged = setMaximum;
            maxLabel.onTextChange = [this, setMaximum]() {
                setMaximum(maxLabel.getText().getFloatValue());
            };
        }

        void resized() override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            maxLabel.setBounds(bounds.removeFromRight(bounds.getWidth() / (2 - hideLabel)));
            minLabel.setBounds(bounds);
        }
    };

    template<typename T>
    struct EditableComponent : public Property {
        std::unique_ptr<Label> label;
        Value& property;

        EditableComponent(String propertyName, Value& value, int idx)
            : Property(propertyName, idx)
            , property(value)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                label = std::make_unique<DraggableNumber>(std::is_integral<T>::value);

                dynamic_cast<DraggableNumber*>(label.get())->valueChanged = [this](float value) {
                    property = value;
                };
            } else {
                label = std::make_unique<Label>();
            }

            addAndMakeVisible(label.get());
            label->setEditable(true, false);
            label->getTextValue().referTo(property);
            label->addMouseListener(this, true);

            label->setFont(Font(14));

            label->onEditorShow = [this]() {
                auto* editor = label->getCurrentTextEditor();
                
                if constexpr (std::is_floating_point<T>::value) {
                    editor->setInputRestrictions(0, "0123456789.-");
                } else if constexpr (std::is_integral<T>::value) {
                    editor->setInputRestrictions(0, "0123456789-");
                }
            };
        }

        void resized() override
        {
            label->setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }
    };
};
