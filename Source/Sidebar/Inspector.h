/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/PropertiesPanel.h"

class Inspector final : public Component {
    // The value shown for a property of several selected objects. While their values differ, the first write is
    // passed on even if it's the value already shown (the first object's), so committing it reaches every object.
    // A plain juce::Value drops a write that changes nothing.
    class MixedValueSource final : public Value::ValueSource {
    public:
        explicit MixedValueSource(var const& initialValue)
            : value(initialValue)
        {
        }

        var getValue() const override { return value; }

        void setValue(var const& newValue) override
        {
            if (!mixed && newValue.equalsWithSameType(value))
                return;

            mixed = false;
            value = newValue;
            sendChangeMessage(false);
        }

        // Only once the field exists: fields write back the value they're given while they're set up
        void setMixed(bool const isMixed) { mixed = isMixed; }

    private:
        var value;
        bool mixed = false;
    };

    class PropertyRedirector final : public Value::Listener {
    public:
        explicit PropertyRedirector(Inspector* parent)
            : inspector(parent)
        {
        }

        Value* addProperty(Value* controllerValue, SmallArray<Value*> const& attachedValues, var const& shownWhenMixed, bool& isMixed)
        {
            auto* property = properties.add(new Property(this, controllerValue, attachedValues, shownWhenMixed));
            isMixed = property->mixed;
            return &property->baseValue;
        }

        // Call once the field for the last added property is built
        void fieldCreated()
        {
            if (auto* property = properties.getLast())
                property->source->setMixed(property->mixed);
        }

        void clearProperties()
        {
            properties.clear();
        }

    private:
        struct Property {
            Property(PropertyRedirector* parent, Value* controllerValue, SmallArray<Value*> const& attachedValues, var const& shownWhenMixed)
                : redirector(parent)
                , values(attachedValues)
            {
                values.add(controllerValue);

                // Do the selected objects differ? Then the field shows "(mixed)", with either nothing behind it
                // (shownWhenMixed), so that whatever is picked or typed is a change, or the first object's value,
                // which a dragged number starts from
                auto shown = controllerValue->getValue();
                for (auto const* value : values) {
                    if (value->getValue() != shown) {
                        mixed = true;
                        break;
                    }
                }
                if (mixed && !shownWhenMixed.isVoid())
                    shown = shownWhenMixed;

                source = new MixedValueSource(shown);
                baseValue.referTo(Value(source));
                baseValue.addListener(redirector);
            }

            ~Property()
            {
                baseValue.removeListener(redirector);
            }

            PropertyRedirector* redirector;
            Value baseValue;
            SmallArray<Value*> values;
            MixedValueSource* source = nullptr; // owned by baseValue
            bool mixed = false;
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
        panel.setTitleHeight(18);
        panel.setTitleAlignment(PropertiesPanel::AlignWithPropertyName);
        panel.setDrawShadowAndOutline(false);
        addAndMakeVisible(panel);

        setOpaque(true);
        lookAndFeelChanged();
    }

    void lookAndFeelChanged() override
    {
        panel.setSeparatorColour(PlugDataColour::sidebarBackgroundColourId);
        panel.setPanelColour(PlugDataColour::sidebarActiveBackgroundColourId);
    }

    void visibilityChanged() override
    {
        showParameters();
    }

    void paint(Graphics& g) override
    {
        g.fillAll(getThemeColours(*this).sidebarBackgroundColour);
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().withTrimmedTop(2));
        resetButton.setTopLeftPosition(getLocalBounds().withTrimmedRight(23).getRight(), 0);

        panel.setContentWidth(getWidth() - 16);
    }

    PropertiesPanelProperty* createPanel(int const type, String const& name, Value const* value, StringArray const& options, bool const clip, double const min, double const max, std::function<void(bool)> const& onInteractionFn = nullptr)
    {
        switch (type) {
        case tString:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        case tFloat: {
            return new PropertiesPanel::EditableComponent<float>(name, *value, clip, min, max);
        }
        case tInt: {
            return new PropertiesPanel::EditableComponent<int>(name, *value, clip, min, max, onInteractionFn);
        }
        case tColour:
            return new PropertiesPanel::ColourComponent(name, *value, false);
        case tColourAlpha:
            return new PropertiesPanel::ColourComponent(name, *value, true);
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

    void updateEmptyState()
    {
        bool const empty = panel.isEmpty();
        panel.setVisible(!empty);
        repaint();
    }

    bool isEmpty() const
    {
        return properties.empty();
    }

    bool loadParameters(SmallArray<ObjectParameters, 6>& objectParameters)
    {
        // A cached list can outlive the object its Value pointers belong to
        for (auto const& parameters : objectParameters) {
            if (!parameters.isValid()) {
                objectParameters.clear();
                break;
            }
        }

        properties = objectParameters;

        panel.clear();
        redirector.clearProperties();

        if (objectParameters.empty()) {
            updateEmptyState();
            return false;
        }

        auto parameterIsInAllObjects = [&objectParameters](ObjectParameter& param, SmallArray<Value*>& values) {
            auto& [name1, type1, category1, value1, options1, defaultVal1, customComponent1, onInteractionFn1, clip1, min1, max1] = param;

            if (objectParameters.size() > 1 && (name1 == "Size" || name1 == "Position" || name1 == "Height")) {
                return false;
            }

            bool isInAllObjects = true;
            for (auto& parameters : objectParameters) {
                bool hasParameter = false;
                for (auto& [name2, type2, category2, value2, options2, defaultVal2, customComponent2, onInteractionFn2, clip2, min2, max2] : parameters.getParameters()) {
                    if (name1 == name2 && type1 == type2 && category1.name == category2.name) {
                        values.add(value2);
                        hasParameter = true;
                        break;
                    }
                }

                isInAllObjects = isInAllObjects && hasParameter;
            }

            return isInAllObjects;
        };

        auto params = objectParameters[0].getParameters();

        std::set<int> categories;
        for (auto const& param : params)
            categories.emplace(param.category.order);

        for (auto& c : categories) {
            PropertiesArray panels;
            String categoryName;
            for (auto& parameter : params) {
                auto& [name, type, category, value, options, defaultVal, customComponentFn, onInteractionFn, clip, min, max] = parameter;
                if (category.order != c)
                    continue;

                if (customComponentFn && objectParameters.size() == 1) {
                    if (auto* customComponent = customComponentFn()) {
                        panel.addSection("", { customComponent });
                    } else {
                        continue;
                    }
                } else if (customComponentFn) {
                    continue;
                } else {
                    SmallArray<Value*> otherValues;
                    if (!parameterIsInAllObjects(parameter, otherValues))
                        continue;
                    if (objectParameters.size() == 1) {
                        auto newPanel = createPanel(type, name, value, options, clip, min, max, onInteractionFn);
                        newPanel->setPreferredHeight(32);
                        panels.add(newPanel);
                    } else {
                        // While the objects differ, a choice, a text or a font starts out empty; other fields keep the
                        // first object's value behind "(mixed)"
                        auto const shownWhenMixed = type == tCombo ? var(0) : type == tString || type == tFont ? var(String()) : var();
                        bool mixed = false;
                        auto const* redirectedProperty = redirector.addProperty(value, otherValues, shownWhenMixed, mixed);
                        auto newPanel = createPanel(type, name, redirectedProperty, options, clip, min, max);
                        if (mixed)
                            newPanel->showMixed();
                        redirector.fieldCreated();
                        newPanel->setPreferredHeight(32);
                        panels.add(newPanel);
                    }
                }
                categoryName = category.name;
            }
            if (!panels.isEmpty())
                panel.addSection(categoryName, panels);
        }
        updateEmptyState();

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
                propertiesList.resetAll(*this);
            }
        };

        return std::unique_ptr<TextButton>(resetButton);
    }
};
