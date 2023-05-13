/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "DraggableNumber.h"
#include "ColourPicker.h"

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

            comboBox.getProperties().set("Style", "Inspector");
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
            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.drawText(fontName, getLocalBounds().reduced(2), Justification::centredLeft);
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
            comboBox.getProperties().set("Style", "Inspector");
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

    struct BoolComponent : public Property, public Value::Listener {
        BoolComponent(String const& propertyName, Value& value, std::vector<String> options)
            : Property(propertyName)
            , textOptions(options)
            , toggleStateValue(value)
        {
            toggleStateValue.addListener(this);
        }

        ~BoolComponent()
        {
            toggleStateValue.removeListener(this);
        }

        bool hitTest(int x, int y) override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            return bounds.contains(x, y);
        }

        void paint(Graphics& g) override
        {
            bool isDown = getValue<bool>(toggleStateValue);
            bool isHovered = isMouseOver();

            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));

            if (isDown || isHovered) {
                // Add some alpha to make it look good on any background...
                g.setColour(findColour(TextButton::buttonColourId).withAlpha(isDown ? 0.9f : 0.7f));
                g.fillRect(bounds);
            }

            auto textColour = isDown ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);
            Fonts::drawText(g, textOptions[isDown], bounds, textColour, 14.0f, Justification::centred);

            // Paint label
            Property::paint(g);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            toggleStateValue = !getValue<bool>(toggleStateValue);
            repaint();
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(toggleStateValue))
                repaint();
        }

    private:
        std::vector<String> textOptions;
        Value toggleStateValue;
    };

    struct ColourComponent : public Property, public Value::Listener {
        ColourComponent(String const& propertyName, Value& value)
            : Property(propertyName)
            , currentColour(value)
        {
            currentColour.addListener(this);
            repaint();
        }

        ~ColourComponent() override
        {
            currentColour.removeListener(this);
        }

        void paint(Graphics& g) override
        {
            auto colour = Colour::fromString(currentColour.toString());
            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;

            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));

            g.setColour(isMouseOver() ? colour.brighter(0.4f) : colour);
            g.fillRect(bounds);

            Fonts::drawText(g, String("#") + currentColour.toString().substring(2).toUpperCase(), bounds, textColour, 14.0f, Justification::centred);

            // Paint label
            Property::paint(g);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            ColourPicker::show(getTopLevelComponent(), false, Colour::fromString(currentColour.toString()), getScreenBounds(), [_this = SafePointer(this)](Colour c) {
                if (!_this)
                    return;

                _this->currentColour = c.toString();
                _this->repaint();
            });
        }

        bool hitTest(int x, int y) override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            return bounds.contains(x, y);
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(currentColour))
                repaint();
        }

    private:
        Value currentColour;
    };

    struct RangeComponent : public Property {
        Value property;

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
        Value property;

        EditableComponent(String propertyName, Value& value)
            : Property(propertyName)
            , property(value)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                auto* draggableNumber = new DraggableNumber(std::is_integral<T>::value);
                label = std::unique_ptr<DraggableNumber>(draggableNumber);

                draggableNumber->getTextValue().referTo(property);
                draggableNumber->setFont(Font(14));

                draggableNumber->valueChanged = [this](float value) {
                    property = value;
                };

                draggableNumber->setEditableOnClick(true);

                draggableNumber->onEditorShow = [this, draggableNumber]() {
                    auto* editor = draggableNumber->getCurrentTextEditor();

                    if constexpr (std::is_floating_point<T>::value) {
                        editor->setInputRestrictions(0, "0123456789.-");
                    } else if constexpr (std::is_integral<T>::value) {
                        editor->setInputRestrictions(0, "0123456789-");
                    }
                };
            } else {
                label = std::make_unique<Label>();
                label->setEditable(true, false);
                label->getTextValue().referTo(property);
                label->setFont(Font(14));
            }

            addAndMakeVisible(label.get());

            label->addMouseListener(this, true);
        }

        void resized() override
        {
            label->setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }
    };

    struct FilePathComponent : public Property {
        Label label;
        TextButton browseButton = TextButton(Icons::File);
        Value property;

        std::unique_ptr<FileChooser> saveChooser;

        FilePathComponent(String propertyName, Value& value)
            : Property(propertyName)
            , property(value)
        {

            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);
            label.setFont(Font(14));

            browseButton.getProperties().set("Style", "SmallIcon");

            addAndMakeVisible(label);
            addAndMakeVisible(browseButton);

            browseButton.onClick = [this]() {
                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles;

                // TODO: Shouldn't this be an open dialog ?!
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
