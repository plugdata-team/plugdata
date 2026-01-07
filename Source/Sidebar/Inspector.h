/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/PropertiesPanel.h"

class Inspector final : public Component {
    class PropertyRedirector final : public Value::Listener {
    public:
        explicit PropertyRedirector(Inspector* parent)
            : inspector(parent)
        {
        }

        Value* addProperty(Value* controllerValue, SmallArray<Value*> const& attachedValues)
        {
            auto* property = properties.add(new Property(this, controllerValue, attachedValues));
            return &property->baseValue;
        }

        void clearProperties()
        {
            properties.clear();
        }

    private:
        struct Property {
            Property(PropertyRedirector* parent, Value* controllerValue, SmallArray<Value*> const& attachedValues)
                : redirector(parent)
                , values(attachedValues)
            {
                values.add(controllerValue);
                baseValue.setValue(controllerValue->getValue());
                baseValue.addListener(redirector);
            }

            ~Property()
            {
                baseValue.removeListener(redirector);
            }

            PropertyRedirector* redirector;
            Value baseValue;
            SmallArray<Value*> values;
        };

        void valueChanged(Value& v) override
        {
            pd::Patch* currentPatch = nullptr;
            if (auto* editor = inspector->findParentComponentOfClass<PluginEditor>()) {
                if (auto const* cnv = editor->getCurrentCanvas()) {
                    currentPatch = &cnv->patch;
                }
            }

            for (auto* property : properties) {
                if (property->baseValue.refersToSameSourceAs(v)) {
                    bool isInsideUndoSequence = false;
                    if (currentPatch && !lastChangedValue.refersToSameSourceAs(v)) {
                        currentPatch->startUndoSequence("properties");
                        lastChangedValue.referTo(v);
                        isInsideUndoSequence = true;
                    }

                    for (auto* value : property->values) {
                        value->setValue(v.getValue());
                    }

                    if (isInsideUndoSequence && currentPatch) {
                        currentPatch->endUndoSequence("properties");
                    }
                    break;
                }
            }
        }

        Value lastChangedValue;
        OwnedArray<Property> properties;
        Inspector* inspector;
    };

    PropertiesPanel panel;
    TextButton resetButton;
    SmallArray<ObjectParameters, 6> properties;
    PropertyRedirector redirector;

public:
    Inspector()
        : redirector(this)
    {
        panel.setTitleHeight(20);
        panel.setTitleAlignment(PropertiesPanel::AlignWithPropertyName);
        panel.setDrawShadowAndOutline(false);
        addAndMakeVisible(panel);
        lookAndFeelChanged();
    }

    void lookAndFeelChanged() override
    {
        panel.setSeparatorColour(PlugDataColour::sidebarBackgroundColourId);
        panel.setPanelColour(PlugDataColour::sidebarActiveBackgroundColourId);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().withTrimmedTop(2));
        resetButton.setTopLeftPosition(getLocalBounds().withTrimmedRight(23).getRight(), 0);

        panel.setContentWidth(getWidth() - 16);
    }

    PropertiesPanelProperty* createPanel(int const type, String const& name, Value* value, StringArray const& options, bool const clip, double const min, double const max, std::function<void(bool)> const& onInteractionFn = nullptr)
    {
        switch (type) {
        case tString:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        case tFloat: {
            auto* c = new PropertiesPanel::EditableComponent<float>(name, *value);
            if (clip) {
                c->setRangeMin(min);
                c->setRangeMax(max);
            }
            return c;
        }
        case tInt: {
            auto* c = new PropertiesPanel::EditableComponent<int>(name, *value);
            if (clip) {
                c->setRangeMin(min);
                c->setRangeMax(max);
            }
            return c;
        }
        case tColour:
            return new PropertiesPanel::InspectorColourComponent(name, *value);
        case tBool:
            return new PropertiesPanel::BoolComponent(name, *value, options);
        case tCombo:
            return new PropertiesPanel::ComboComponent(name, *value, options);
        case tRangeFloat:
            return new PropertiesPanel::RangeComponent(name, *value, false);
        case tRangeInt:
            return new PropertiesPanel::RangeComponent(name, *value, true);
        case tFont: {
            if (auto* editor = findParentComponentOfClass<PluginEditor>()) {
                if (auto const* cnv = editor->getCurrentCanvas()) {
                    return new PropertiesPanel::FontComponent(name, *value, cnv->patch.getCurrentFile().getParentDirectory());
                }
            }
            return new PropertiesPanel::FontComponent(name, *value);
        }
        default:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        }
    }

    void showParameters()
    {
        loadParameters(properties);
    }

    bool isEmpty()
    {
        return properties.empty();
    }

    bool loadParameters(SmallArray<ObjectParameters, 6>& objectParameters)
    {
        properties = objectParameters;

        StringArray const names = { "Dimensions", "General", "Appearance", "Label", "Extra" };

        panel.clear();

        if (objectParameters.empty())
            return false;

        auto parameterIsInAllObjects = [&objectParameters](ObjectParameter& param, SmallArray<Value*>& values) {
            auto& [name1, type1, category1, value1, options1, defaultVal1, customComponent1, onInteractionFn1, clip1, min1, max1] = param;

            if (objectParameters.size() > 1 && (name1 == "Size" || name1 == "Position" || name1 == "Height")) {
                return false;
            }

            bool isInAllObjects = true;
            for (auto& parameters : objectParameters) {
                bool hasParameter = false;
                for (auto& [name2, type2, category2, value2, options2, defaultVal2, customComponent2, onInteractionFn2, clip2, min2, max2] : parameters.getParameters()) {
                    if (name1 == name2 && type1 == type2 && category1 == category2) {
                        values.add(value2);
                        hasParameter = true;
                        break;
                    }
                }

                isInAllObjects = isInAllObjects && hasParameter;
            }

            return isInAllObjects;
        };

        redirector.clearProperties();

        for (int i = 0; i < 4; i++) {
            PropertiesArray panels;
            for (auto& parameter : objectParameters[0].getParameters()) {
                auto& [name, type, category, value, options, defaultVal, customComponentFn, onInteractionFn, clip, min, max] = parameter;

                if (customComponentFn && objectParameters.size() == 1 && static_cast<int>(category) == i) {
                    if (auto* customComponent = customComponentFn()) {
                        panel.addSection("", { customComponent });
                    } else {
                        continue;
                    }
                } else if (customComponentFn) {
                    continue;
                } else if (static_cast<int>(category) == i) {

                    SmallArray<Value*> otherValues;
                    if (!parameterIsInAllObjects(parameter, otherValues))
                        continue;
                    if (objectParameters.size() == 1) {
                        auto newPanel = createPanel(type, name, value, options, clip, min, max, onInteractionFn);
                        newPanel->setPreferredHeight(30);
                        panels.add(newPanel);
                    } else {
                        auto* redirectedProperty = redirector.addProperty(value, otherValues);
                        auto newPanel = createPanel(type, name, redirectedProperty, options, clip, min, max);
                        newPanel->setPreferredHeight(30);
                        panels.add(newPanel);
                    }
                }
            }
            if (!panels.isEmpty()) {
                panel.addSection(names[i], panels);
            }
        }
        if (panel.isEmpty())
            return false;

        return true;
    }

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* resetButton = new SmallIconButton(Icons::Reset);
        resetButton->setTooltip("Reset to default");
        resetButton->setSize(23, 23);
        resetButton->onClick = [this] {
            for (auto& propertiesList : properties) {
                propertiesList.resetAll();
            }
        };

        return std::unique_ptr<TextButton>(resetButton);
    }
};
