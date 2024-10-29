/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/PropertiesPanel.h"

class Inspector : public Component {
    class PropertyRedirector : public Value::Listener {
        public:
        PropertyRedirector(Inspector* parent) : inspector(parent)
        {
        }

        Value* addProperty(Value* controllerValue, Array<Value*> attachedValues)
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
            Property(PropertyRedirector* parent, Value* controllerValue, Array<Value*> attachedValues)
                : redirector(parent), values(attachedValues)
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
            Array<Value*> values;
        };
        
        
        void valueChanged(Value& v) override
        {
            pd::Patch* currentPatch = nullptr;
            if(auto* editor = inspector->findParentComponentOfClass<PluginEditor>()) {
                if(auto* cnv = editor->getCurrentCanvas()) {
                    currentPatch = &cnv->patch;
                }
            }
            
            for(auto* property : properties)
            {
                if(property->baseValue.refersToSameSourceAs(v))
                {
                    bool isInsideUndoSequence = false;
                    if(currentPatch && !lastChangedValue.refersToSameSourceAs(v))
                    {
                        currentPatch->startUndoSequence("properties");
                        lastChangedValue.referTo(v);
                        isInsideUndoSequence = true;
                    }
                    
                    for (auto* value : property->values) {
                        value->setValue(v.getValue());
                    }
                    
                    
                    if(isInsideUndoSequence)
                    {
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
    SmallVector<ObjectParameters> properties;
    PropertyRedirector redirector;

public:
    Inspector() : redirector(this)
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
        panel.setBounds(getLocalBounds());
        resetButton.setTopLeftPosition(getLocalBounds().withTrimmedRight(23).getRight(), 0);

        panel.setContentWidth(getWidth() - 16);
    }

    static PropertiesPanelProperty* createPanel(int type, String const& name, Value* value, StringArray& options, std::function<void(bool)> onInteractionFn = nullptr)
    {
        switch (type) {
        case tString:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        case tFloat:
            return new PropertiesPanel::EditableComponent<float>(name, *value);
        case tInt:
            return new PropertiesPanel::EditableComponent<int>(name, *value, 0.0f, 0.0f, onInteractionFn);
        case tColour:
            return new PropertiesPanel::ColourComponent(name, *value);
        case tBool:
            return new PropertiesPanel::BoolComponent(name, *value, options);
        case tCombo:
            return new PropertiesPanel::ComboComponent(name, *value, options);
        case tRangeFloat:
            return new PropertiesPanel::RangeComponent(name, *value, false);
        case tRangeInt:
            return new PropertiesPanel::RangeComponent(name, *value, true);
        case tFont:
            return new PropertiesPanel::FontComponent(name, *value);
        default:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        }
    }

    void showParameters()
    {
        loadParameters(properties);
    }

    void loadParameters(SmallVector<ObjectParameters>& objectParameters)
    {
        properties = objectParameters;

        StringArray names = { "Dimensions", "General", "Appearance", "Label", "Extra" };

        panel.clear();

        auto parameterIsInAllObjects = [&objectParameters](ObjectParameter& param, Array<Value*>& values) {
            auto& [name1, type1, category1, value1, options1, defaultVal1, customComponent1, onInteractionFn1] = param;

            if (objectParameters.size() > 1 && (name1 == "Size" || name1 == "Position" || name1 == "Height")) {
                return false;
            }

            bool isInAllObjects = true;
            for (auto& parameters : objectParameters) {
                bool hasParameter = false;
                for (auto& [name2, type2, category2, value2, options2, defaultVal2, customComponent2, onInteractionFn2] : parameters.getParameters()) {
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
            Array<PropertiesPanelProperty*> panels;
            for (auto& parameter : objectParameters[0].getParameters()) {
                auto& [name, type, category, value, options, defaultVal, customComponentFn, onInteractionFn] = parameter;

                if (customComponentFn && objectParameters.size() == 1 && static_cast<int>(category) == i) {
                    if (auto* customComponent = customComponentFn()) {
                        panel.addSection("", { customComponent });
                    } else {
                        continue;
                    }
                } else if (customComponentFn) {
                    continue;
                } else if (static_cast<int>(category) == i) {

                    Array<Value*> otherValues;
                    if (!parameterIsInAllObjects(parameter, otherValues))
                        continue;

                    else if (objectParameters.size() == 1) {
                        auto newPanel = createPanel(type, name, value, options, onInteractionFn);
                        newPanel->setPreferredHeight(26);
                        panels.add(newPanel);
                    } else {
                        auto* redirectedProperty = redirector.addProperty(value, otherValues);
                        auto newPanel = createPanel(type, name, redirectedProperty, options);
                        newPanel->setPreferredHeight(26);
                        panels.add(newPanel);
                    }
                }
            }
            if (!panels.isEmpty()) {
                panel.addSection(names[i], panels);
            }
        }
    }

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* resetButton = new SmallIconButton(Icons::Reset);
        resetButton->setTooltip("Reset to default");
        resetButton->setSize(23, 23);
        resetButton->onClick = [this]() {
            for (auto& propertiesList : properties) {
                propertiesList.resetAll();
            }
        };

        return std::unique_ptr<TextButton>(resetButton);
    }
};
