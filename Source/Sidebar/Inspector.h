/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/PropertiesPanel.h"

class Inspector final : public Component {
    // A property of several selected objects. It wraps the property's usual component: an edit made there is
    // copied to every selected object. While the objects' values differ, the row says "(mixed)" instead, and the
    // first edit reaches all of them, even if it's the value the first object already had.
    class MixedPropertyComponent final : public PropertiesPanelProperty
        , public Value::Listener
        , public TextEditor::Listener
        , public ComponentListener {

        // What the wrapped component edits. A juce::Value drops a write of the value it already holds. While the
        // objects differ, such a write is passed on if the user typed it: number and colour fields also write
        // their unchanged value when they're only clicked and left again.
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
                sendChangeMessage(false);
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

            // A choice or a font the objects don't share starts out as none, so that any pick is a change. Other
            // properties keep the first object's value under "(mixed)", for a drag or an edit to start from.
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
            property.reset(); // now, while this still hears its children being deleted
        }

        void paint(Graphics& g) override
        {
            if (!isShowingMixed())
                return; // the wrapped component draws the whole row

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
        // "(mixed)" stands in for the component until it's used: pressed, or its text edited
        bool isShowingMixed() const
        {
            return mixed && !mouseIsDown && textEditor == nullptr && !toggleRevealed;
        }

        void update()
        {
            source->passUnchanged = mixed && typed;

            // The hidden component still gets the mouse, so one press shows it and starts a drag or an edit.
            // A toggle would act on that press without having been seen, so it's only shown by it.
            if (type == tBool)
                property->setVisible(!isShowingMixed());
            else
                property->setAlpha(isShowingMixed() ? 0.0f : 1.0f);

            repaint();
        }

        // The wrapped component changed the shared value: copy it to every selected object
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

        // Mouse events of this row, and of the wrapped component and its children
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

            // A toggle only reports a click on its other side: while the objects differ, either side applies
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

        // Labels and number boxes add a TextEditor as a child while their text is being edited
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

        void componentBeingDeleted(Component& component) override
        {
            if (&component != textEditor)
                return;

            textEditor = nullptr;

            // Once the component has dealt with the edit itself
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (!_this)
                    return;

                // A text field only reports text that differs from before
                if (_this->mixed && _this->typed && _this->type == tString)
                    _this->sharedValue.setValue(_this->typedText);

                _this->typed = false;
                _this->update();
            });
        }

        Inspector& inspector;
        ParameterType const type;
        SmallArray<Value*> values; // the property's value in each selected object
        SharedValue* source;       // owned by sharedValue
        Value sharedValue;
        std::unique_ptr<PropertiesPanelProperty> property;

        bool mixed = false;          // the objects' values differ, and nothing has been set yet
        bool mouseIsDown = false;    // on the wrapped component
        bool toggleRevealed = false; // a toggle, shown by a click on "(mixed)"
        bool typed = false;          // into the open text editor
        String typedText;
        TextEditor* textEditor = nullptr;
    };

    PropertiesPanel panel;
    TextButton resetButton;
    SmallArray<ObjectParameters, 6> properties;
    Value lastChangedValue; // by a MixedPropertyComponent: a change of another one starts a new undo step

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
        };

        return std::unique_ptr<TextButton>(resetButton);
    }
};
