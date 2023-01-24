/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "DraggableNumber.h"

class PropertiesPanel : public PropertyPanel {

public:
    class Property : public PropertyComponent {

    protected:
        bool hideLabel = false;

    public:
        Property(String const& propertyName)
            : PropertyComponent(propertyName, 23)
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
            if (!hideLabel) {
                getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
            }
        }

        void refresh() override {};
    };

    struct ComboComponent : public Property {
        ComboComponent(String const& propertyName, Value& value, std::vector<String> options)
            : Property(propertyName)
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
            auto font = Font(fontName, 15, Font::plain);
            g.setFont(font);
            PlugDataLook::drawText(g, fontName, getLocalBounds().reduced(2), Justification::centredLeft, findColour(PlugDataColour::panelTextColourId));
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

        FontComponent(String const& propertyName, Value& value)
            : Property(propertyName)
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

    template<typename T>
    struct MultiPropertyComponent : public Property {

        OwnedArray<T> properties;
        int numProperties;

        MultiPropertyComponent(String const& propertyName, Array<Value*> values)
            : Property(propertyName)
            , numProperties(values.size())
        {
            for (int i = 0; i < numProperties; i++) {
                auto* property = properties.add(new T(propertyName, *values[i]));
                property->setHideLabel(true);
                addAndMakeVisible(property);
            }
        }

        MultiPropertyComponent(String const& propertyName, Array<Value*> values, std::vector<String> options)
            : Property(propertyName)
            , numProperties(values.size())
        {
            for (int i = 0; i < numProperties; i++) {
                auto* property = properties.add(new T(propertyName, *values[i], options));
                property->setHideLabel(true);
                addAndMakeVisible(property);
            }
        }

        void resized() override
        {

            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));

            int itemWidth = b.getWidth() / numProperties;
            for (int i = 0; i < numProperties; i++) {
                properties[i]->setBounds(b.removeFromLeft(itemWidth));
            }
        }
    };

    struct BoolComponent : public Property {
        BoolComponent(String const& propertyName, Value& value, std::vector<String> options)
            : Property(propertyName)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setConnectedEdges(12);

            toggleButton.getToggleStateValue().referTo(value);
            toggleButton.setButtonText(static_cast<bool>(value.getValue()) ? options[1] : options[0]);

            toggleButton.setName("inspector:toggle");

            addAndMakeVisible(toggleButton);

            toggleButton.onStateChange = [this, value, options]() mutable {
                toggleButton.setButtonText(toggleButton.getToggleState() ? options[1] : options[0]);
            };
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
        ColourComponent(String const& propertyName, Value& value)
            : Property(propertyName)
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

        RangeComponent(String propertyName, Value& value)
            : Property(propertyName)
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
                // maxLabel.setMinimum(min + 1e-5f);
                property = var(arr);
            };

            auto setMaximum = [this](float value) {
                max = value;
                Array<var> arr = { min, max };
                // minLabel.setMaximum(max - 1e-5f);
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

        EditableComponent(String propertyName, Value& value)
            : Property(propertyName)
            , property(value)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                auto* draggableNumber = new DraggableNumber(std::is_integral<T>::value);
                label = std::unique_ptr<DraggableNumber>(draggableNumber);

                dynamic_cast<DraggableNumber*>(label.get())->valueChanged = [this](float value) {
                    property = value;
                };
                
                draggableNumber->setEditableOnClick(true);
            } else {
                label = std::make_unique<Label>();
                label->setEditable(true, false);
            }

            addAndMakeVisible(label.get());
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

    struct FilePathComponent : public Property {
        Label label;
        TextButton browseButton = TextButton(Icons::File);
        Value& property;

        std::unique_ptr<FileChooser> saveChooser;

        FilePathComponent(String propertyName, Value& value)
            : Property(propertyName)
            , property(value)
        {

            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);
            label.setFont(Font(14));

            browseButton.setName("statusbar::browse");

            addAndMakeVisible(label);
            addAndMakeVisible(browseButton);

            browseButton.onClick = [this]() {
                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;

                saveChooser = std::make_unique<FileChooser>("Choose a location...", File::getSpecialLocation(File::userHomeDirectory), "", false);

                saveChooser->launchAsync(folderChooserFlags,
                    [this](FileChooser const& fileChooser) {
                        const auto file = fileChooser.getResult();
                        if (file.getParentDirectory().exists()) {
                            label.setText(file.getFullPathName(), sendNotification);
                        }
                    });
            };
        }

        void paint(Graphics& g) override
        {

            Property::paint(g);

            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRect(getLocalBounds().removeFromRight(getHeight()));
        }

        void resized() override
        {
            auto labelBounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            label.setBounds(labelBounds);
            browseButton.setBounds(labelBounds.removeFromRight(getHeight()));
        }
    };
};
