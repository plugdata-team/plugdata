/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/PropertiesPanel.h"

class Inspector final : public Component {

    // Property of several selected objects. Shows "(mixed)" while their values differ, an edit applies to all
    class MixedPropertyComponent final : public PropertiesPanelProperty
        , public Value::Listener
        , public TextEditor::Listener
        , public ComponentListener {

        // Lets an unchanged value through when the user typed it, so it still reaches the other objects.
        struct SharedValue final : public Value::ValueSource {
            explicit SharedValue(var const& initialValue)
                : value(initialValue)
            {
            }

            var getValue() const override { return value; }

            void setValue(var const& newValue) override
            {
                if (newValue == value && !passUnchanged)
                    return;

                value = newValue;
                // Synchronous, because a canvas click rebuilds the rows before a message would arrive
                sendChangeMessage(true);
            }

            var value;
            bool passUnchanged = false;
        };

    public:
        MixedPropertyComponent(Inspector& parent, ObjectParameter const& parameter, SmallArray<Value*> const& objectValues)
            : PropertiesPanelProperty(parameter.name)
            , inspector(parent)
            , type(parameter.type)
            , values(objectValues)
        {
            auto shown = parameter.valuePtr->getValue();
            for (auto const* value : values) {
                if (value->getValue() != shown) {
                    mixed = true;
                    break;
                }
            }

            // A mixed choice or font starts out empty, so that any pick is a change
            if (mixed && type == tCombo)
                shown = 0;
            else if (mixed && type == tFont)
                shown = String();

            source = new SharedValue(shown);
            sharedValue.referTo(Value(source));
            sharedValue.addListener(this);

            property.reset(inspector.createPanel(type, parameter.name, &sharedValue, parameter.options, parameter.clip, parameter.min, parameter.max));
            addAndMakeVisible(property.get());
            property->addMouseListener(this, true);
            watchForTextEditors(*property);

            update();
        }

        ~MixedPropertyComponent() override
        {
            sharedValue.removeListener(this);
            property.reset(); // Its children notify us while they're deleted
        }

        void paint(Graphics& g) override
        {
            if (!isShowingMixed())
                return;

            PropertiesPanelProperty::paint(g);

            g.setFont(Font(FontOptions(14.5f)));
            g.setColour(getThemeColours(*this).panelTextColour.withAlpha(0.55f));
            g.drawText("(mixed)", getLocalBounds().removeFromRight(getWidth() / 2).withTrimmedLeft(5), Justification::centredLeft);
        }

        void resized() override
        {
            property->setBounds(getLocalBounds());
        }

        void setRoundedCorners(bool const roundTop, bool const roundBottom) override
        {
            property->setRoundedCorners(roundTop, roundBottom);
        }

    private:
        bool isShowingMixed() const
        {
            return mixed && !mouseIsDown && textEditor == nullptr && !toggleRevealed;
        }

        void update()
        {
            source->passUnchanged = mixed && typed;

            // At alpha 0 it still gets the mouse, so one press reveals it and starts the drag or edit.
            // A toggle would already switch on that press, so it's hidden and the first click only reveals it
            if (type == tBool)
                property->setVisible(!isShowingMixed());
            else
                property->setAlpha(isShowingMixed() ? 0.0f : 1.0f);

            repaint();
        }

        void valueChanged(Value& v) override
        {
            mixed = false;
            update();

            pd::Patch* currentPatch = nullptr;
            if (auto* editor = inspector.findParentComponentOfClass<PluginEditor>()) {
                if (auto const* cnv = editor->getCurrentCanvas()) {
                    currentPatch = &cnv->patch;
                }
            }

            bool isInsideUndoSequence = false;
            if (currentPatch && !inspector.lastChangedValue.refersToSameSourceAs(v)) {
                currentPatch->startUndoSequence("properties");
                inspector.lastChangedValue.referTo(v);
                isInsideUndoSequence = true;
            }

            for (auto* value : values) {
                value->setValue(v.getValue());
            }

            if (isInsideUndoSequence && currentPatch) {
                currentPatch->endUndoSequence("properties");
            }
        }

        // Also called for the wrapped component and its children
        void mouseDown(MouseEvent const& e) override
        {
            if (!mixed)
                return;

            if (e.eventComponent == this)
                toggleRevealed = type == tBool;
            else
                mouseIsDown = true;

            update();
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!mixed || e.eventComponent == this)
                return;

            mouseIsDown = false;

            // A toggle only writes when its other side is clicked, here either side applies
            if (type == tBool && isRealClickEvent(e)) {
                source->passUnchanged = true;
                sharedValue.setValue(sharedValue.getValue());
            }

            update();
        }

        void mouseExit(MouseEvent const& e) override
        {
            if (toggleRevealed && !e.source.isTouch() && !isMouseOver(true)) {
                toggleRevealed = false;
                update();
            }
        }

        // Labels and number boxes add a TextEditor child while editing
        void watchForTextEditors(Component& component)
        {
            component.addComponentListener(this);
            for (auto* child : component.getChildren())
                watchForTextEditors(*child);
        }

        void componentChildrenChanged(Component& component) override
        {
            for (auto* child : component.getChildren()) {
                if (auto* editor = dynamic_cast<TextEditor*>(child); editor && editor != textEditor) {
                    textEditor = editor;
                    typed = false;
                    editor->addListener(this);
                    editor->addComponentListener(this);
                    update();
                }
            }
        }

        void textEditorTextChanged(TextEditor& editor) override
        {
            typed = true;
            typedText = editor.getText();
            update();
        }

        // Called before the label's listener, which discards the edit
        void textEditorEscapeKeyPressed(TextEditor&) override
        {
            typed = false;
            update();
        }

        void componentBeingDeleted(Component& component) override
        {
            if (&component != textEditor)
                return;

            textEditor = nullptr;

            // A label only writes text that differs, and has done so by now
            if (mixed && typed && type == tString)
                sharedValue.setValue(typedText);

            // A number box writes its value after deleting its editor
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    _this->typed = false;
                    _this->update();
                }
            });
        }

        Inspector& inspector;
        ParameterType const type;
        SmallArray<Value*> values;
        SharedValue* source; // Owned by sharedValue
        Value sharedValue;
        std::unique_ptr<PropertiesPanelProperty> property;

        bool mixed = false;
        bool mouseIsDown = false;
        bool toggleRevealed = false;
        bool typed = false;
        String typedText;
        TextEditor* textEditor = nullptr;
    };

    PropertiesPanel panel;
    TextButton resetButton;
    SmallArray<ObjectParameters, 6> properties;
    Value lastChangedValue; // Editing another property starts a new undo step

public:
    Inspector()
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
                        auto newPanel = new MixedPropertyComponent(*this, parameter, otherValues);
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
            showParameters();
        };

        return std::unique_ptr<TextButton>(resetButton);
    }
};
