/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "DraggableNumber.h"
#include "BouncingViewport.h"
#include "SearchEditor.h"
#include "PluginEditor.h"

class SwatchComponent;
class PropertiesPanelProperty : public PropertyComponent {

protected:
    bool hideLabel : 1 = false;
    bool roundTopCorner : 1 = false;
    bool roundBottomCorner : 1 = false;

public:
    explicit PropertiesPanelProperty(String const& propertyName)
        : PropertyComponent(propertyName, 32)
    {
    }

    virtual PropertiesPanelProperty* createCopy() { return nullptr; }

    void setHideLabel(bool const labelHidden)
    {
        hideLabel = labelHidden;
        repaint();
        resized();
    }

    void paint(Graphics& g) override
    {
        if (!hideLabel) {
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.85f, *this);
        }
    }

    virtual void setRoundedCorners(bool const roundTop, bool const roundBottom)
    {
        roundTopCorner = roundTop;
        roundBottomCorner = roundBottom;
        repaint();
    }

    void refresh() override { }
};

using PropertiesArray = Array<PropertiesPanelProperty*>;

class PropertiesPanel : public Component {
public:
    enum TitleAlignment {
        AlignWithSection,
        AlignWithPropertyName,
    };

private:
    class SectionComponent final : public Component {
        public:
        SectionComponent(PropertiesPanel& propertiesPanel, String const& sectionTitle,
            PropertiesArray const& newProperties, int extraPadding);

        ~SectionComponent() override;

        void paint(Graphics& g) override;

        void setExtraHeaderNames(StringArray headerNames);

        void paintOverChildren(Graphics& g) override;

        void resized() override;

        void lookAndFeelChanged() override;

        int getPreferredHeight() const;

        void mouseUp(MouseEvent const& e) override;

        PropertiesPanel& parent;
        OwnedArray<PropertiesPanelProperty> propertyComponents;
        StringArray extraHeaderNames;
        int padding;
        std::unique_ptr<melatonin::DropShadow> dropShadow;
        JUCE_DECLARE_NON_COPYABLE(SectionComponent)
    };

    class PropertyHolderComponent final : public Component {
    public:
        PropertyHolderComponent() = default;

        void paint(Graphics&) override { }

        void updateLayout(int width, int viewHeight);

        void insertSection(int indexToInsertAt, SectionComponent* newSection);

        SectionComponent* getSectionWithNonEmptyName(int targetIndex) const noexcept;

        OwnedArray<SectionComponent> sections;

        JUCE_DECLARE_NON_COPYABLE(PropertyHolderComponent)
    };

public:
    class ComboComponent : public PropertiesPanelProperty {
        public:
        ComboComponent(String const& propertyName, Value const& value, StringArray const& options);

        ComboComponent(String const& propertyName, StringArray const& options);

        void resized() override;

        PropertiesPanelProperty* createCopy() override;

        StringArray items;
        ComboBox comboBox;
    };

    class FontComponent final : public PropertiesPanelProperty {
        public:
        Value fontValue;
        StringArray options = Font::findAllTypefaceNames();
        bool isFontMissing = false;

        FontComponent(String const& propertyName, Value const& value, File const& extraFontsDir = File());

        PropertiesPanelProperty* createCopy() override;

        void lookAndFeelChanged() override;

        void setFont(String const& fontName);

        void resized() override;

    private:
        ComboBox comboBox;
    };

    class BoolComponent : public PropertiesPanelProperty
        , public Value::Listener {
        public:
        BoolComponent(String const& propertyName, Value const& value, StringArray options);

        // Also allow creating it without passing in a Value, makes it easier to derive from this class for custom bool components
        BoolComponent(String const& propertyName, StringArray options);

        // Allow creation without an attached juce::Value, but with an initial value
        // We need this constructor sometimes to prevent feedback caused by the initial value being set after the listener is attached
        BoolComponent(String const& propertyName, bool initialValue, StringArray options);

        void init();

        ~BoolComponent() override;

        void lookAndFeelChanged() override;

        PropertiesPanelProperty* createCopy() override;

        bool hitTest(int x, int y) override;

        void paint(Graphics& g) override;

        void mouseEnter(MouseEvent const& e) override;
        void mouseExit(MouseEvent const& e) override;
        void mouseUp(MouseEvent const& e) override;

        void valueChanged(Value& v) override;

    protected:
        StringArray textOptions;
        Value toggleStateValue;
    };

    class InspectorColourComponent final : public PropertiesPanelProperty
        , public Value::Listener {
    public:
        InspectorColourComponent(String const& propertyName, Value const& value);

        ~InspectorColourComponent() override;

        PropertiesPanelProperty* createCopy() override;

        void updateHexValue();

        void resized() override;

        void paint(Graphics& g) override;

        void valueChanged(Value& v) override;

        void mouseDown(MouseEvent const& e) override;
        void mouseEnter(MouseEvent const& e) override;
        void mouseExit(MouseEvent const& e) override;

    private:
        Value currentColour;
        Value colour;
        Label hexValueEditor;
        bool isMouseOver = false;
    };

    class ColourComponent final : public PropertiesPanelProperty
        , public Value::Listener {
    public:
        ColourComponent(String const& propertyName, Value& value);

        ~ColourComponent() override;

        void lookAndFeelChanged() override;

        PropertiesPanelProperty* createCopy() override;

        void updateHexValue();

        void resized() override;

        void valueChanged(Value& v) override;

    private:
        std::unique_ptr<SwatchComponent> swatchComponent;
        Value currentColour;
        Value colour;
        TextEditor hexValueEditor;
    };

    class RangeComponent final : public PropertiesPanelProperty
        , public Value::Listener {
        Value property;

        DraggableNumber minLabel, maxLabel;

        float min, max;
    public:
        RangeComponent(String const& propertyName, Value const& value, bool integerMode);

        ~RangeComponent() override;

        PropertiesPanelProperty* createCopy() override;

        DraggableNumber& getMinimumComponent();

        DraggableNumber& getMaximumComponent();

        void setIntegerMode(bool integerMode);

        void resized() override;

        void valueChanged(Value& v) override;
    };

    class FilePathComponent final : public PropertiesPanelProperty {
        Label label;
        SmallIconButton browseButton = SmallIconButton(Icons::File);
        Value property;
    public:
        FilePathComponent(String const& propertyName, Value const& value);

        PropertiesPanelProperty* createCopy() override;

        void paint(Graphics& g) override;

        void resized() override;
    };

    class DirectoryPathComponent final : public PropertiesPanelProperty
        , public Value::Listener {
        String label;
        SmallIconButton browseButton = SmallIconButton(Icons::Folder);
        Value property;
    public:
        DirectoryPathComponent(String const& propertyName, Value const& value);

        void valueChanged(Value& v) override;

        void setPath(String path);

        PropertiesPanelProperty* createCopy() override;

        void paint(Graphics& g) override;

        void resized() override;
    };

    class ActionComponent final : public PropertiesPanelProperty {
        bool mouseIsOver : 1 = false;
        bool roundTop : 1, roundBottom : 1;

    public:
        ActionComponent(std::function<void()> callback, String iconToShow, String const& textToShow, bool roundOnTop = false, bool roundOnBottom = false);

        PropertiesPanelProperty* createCopy() override;

        void paint(Graphics& g) override;

        void mouseEnter(MouseEvent const& e) override;
        void mouseExit(MouseEvent const& e) override;
        void mouseUp(MouseEvent const& e) override;

        std::function<void()> onClick = [] { };
        String icon;
    };

    template<typename T>
    class EditableComponent final : public PropertiesPanelProperty
        , public Value::Listener {
        Value property;
        String allowedCharacters = "";
        double min, max;
        bool limit;

    public:
        std::unique_ptr<Component> label;

            EditableComponent(String const& propertyName, Value const& value, bool clip = false, double minimum = 0.0, double maximum = 1<<30, std::function<void(bool)> onInteractionFn = nullptr)
            : PropertiesPanelProperty(propertyName)
            , property(value)
            , min(minimum)
            , max(maximum)
            , limit(clip)
        {
            if constexpr (std::is_arithmetic_v<T>) {
                auto* draggableNumber = new DraggableNumber(std::is_integral_v<T>);
                label = std::unique_ptr<DraggableNumber>(draggableNumber);

                // By setting the text before attaching the value, we can prevent an unnesssary/harmful call to ValueChanged
                draggableNumber->setText(property.toString(), dontSendNotification);
                draggableNumber->setFont(draggableNumber->getFont().withHeight(14.5f));
                draggableNumber->setEditableOnClick(true);

                if(clip)
                {
                    draggableNumber->setMinimum(minimum);
                    draggableNumber->setMaximum(maximum);
                }

                if (onInteractionFn)
                    draggableNumber->onInteraction = onInteractionFn;
                
                draggableNumber->setPrecision(3);

                draggableNumber->onValueChange = [this](double const newValue) {
                    if (limit) {
                        property = std::clamp<T>(newValue, min, max);
                    } else {
                        property = static_cast<T>(newValue);
                    }
                };

                draggableNumber->onReturnKey = [this](double const newValue) {
                    if (limit) {
                        property = std::clamp<T>(newValue, min, max);
                    } else {
                        property = static_cast<T>(newValue);
                    }
                };

                property.addListener(this);
                draggableNumber->onEditorShow = [draggableNumber] {
                    auto* editor = draggableNumber->getCurrentTextEditor();
                    editor->setBorder(BorderSize<int>(2, 1, 4, 1));
                    editor->setJustification(Justification::centredLeft);
                };
            } else {
                auto* labelComp = new Label();
                label = std::unique_ptr<Label>(labelComp);
                labelComp->setEditable(true, false);
                labelComp->getTextValue().referTo(property);
                labelComp->setFont(Font(14.5f));

                labelComp->onEditorShow = [this, labelComp] {
                    auto* editor = labelComp->getCurrentTextEditor();
                    editor->setBorder(BorderSize<int>(2, 1, 4, 1));
                    editor->setJustification(Justification::centredLeft);

                    if (allowedCharacters.isNotEmpty()) {
                        editor->setInputRestrictions(0, allowedCharacters);
                    }
                };

                labelComp->onEditorHide = [this] {
                    // synchronise the value to the canvas when updated
                    if (auto* pluginEditor = findParentComponentOfClass<PluginEditor>()) {
                        if (auto const cnv = pluginEditor->getCurrentCanvas())
                            cnv->synchronise();
                    }
                };
            }

            addAndMakeVisible(label.get());

            label->addMouseListener(this, true);
        }

        void valueChanged(Value& v) override
        {
            if constexpr (std::is_arithmetic_v<T>) {
                auto const draggableNumber = reinterpret_cast<DraggableNumber*>(label.get());
                auto const value = getValue<T>(v);
                if (value != static_cast<T>(draggableNumber->getValue())) {
                    draggableNumber->setValue(value, dontSendNotification);
                }
            }
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new EditableComponent(getName(), property);
        }

        void setInputRestrictions(String const& newAllowedCharacters)
        {
            allowedCharacters = newAllowedCharacters;
        }

        void resized() override
        {
            label->setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

        void editableOnClick(bool const editable)
        {
            dynamic_cast<DraggableNumber*>(label.get())->setEditableOnClick(editable);
        }
    };

    template<typename T>
    class MultiPropertyComponent final : public PropertiesPanelProperty {

        OwnedArray<T> properties;
        SmallArray<Value*> propertyValues;
        StringArray propertyOptions;
    public:
        MultiPropertyComponent(String const& propertyName, SmallArray<Value*> values)
            : PropertiesPanelProperty(propertyName)
            , propertyValues(values)
        {
            for (int i = 0; i < propertyValues.size(); i++) {
                auto* property = properties.add(new T(propertyName, *values[i]));
                property->setHideLabel(true);
                addAndMakeVisible(property);
            }

            setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
        }

        MultiPropertyComponent(String const& propertyName, SmallArray<Value*> values, StringArray options)
            : PropertiesPanelProperty(propertyName)
            , propertyValues(values)
            , propertyOptions(options)
        {
            for (int i = 0; i < propertyValues.size(); i++) {
                auto* property = properties.add(new T(propertyName, *values[i], options));
                property->setHideLabel(true);
                addAndMakeVisible(property);
            }
        }

        PropertiesPanelProperty* createCopy() override
        {
            if constexpr (std::is_same_v<T, BoolComponent> || std::is_same_v<T, ComboComponent>) {
                return new MultiPropertyComponent(getName(), propertyValues, propertyOptions);
            } else {
                return new MultiPropertyComponent(getName(), propertyValues);
            }
        }

        void lookAndFeelChanged() override
        {
            for (auto& property : properties) {
                property->setColour(ComboBox::textColourId, findColour(PlugDataColour::panelTextColourId));
            }
        }

        void setRoundedCorners(bool roundTop, bool roundBottom) override
        {
            properties.getLast()->setRoundedCorners(roundTop, roundBottom);
        }

        void resized() override
        {
            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));

            int const itemWidth = b.getWidth() / propertyValues.size();
            for (int i = 0; i < propertyValues.size(); i++) {
                properties[i]->setBounds(b.removeFromLeft(itemWidth));
            }
        }

        void paintOverChildren(Graphics& g) override
        {
            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            int const itemWidth = b.getWidth() / propertyValues.size();

            for (int i = 0; i < propertyValues.size(); i++) {
                auto propertyBounds = b.removeFromLeft(itemWidth);
                g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
                g.drawVerticalLine(propertyBounds.getX(), 0, getHeight());
            }
        }
    };

    PropertiesPanel();

    /** Destructor. */
    ~PropertiesPanel() override;

    // Deletes all property components from the panel
    void clear() const;

    // Adds a set of properties to the panel
    void addSection(String const& sectionTitle, PropertiesArray const& newProperties, int indexToInsertAt = -1, int extraPaddingBetweenComponents = 0);

    // Returns true if the panel contains no properties
    bool isEmpty() const;

    void setContentWidth(int newContentWidth);

    Component* getSectionByName(String const& name) const noexcept;

    std::pair<int, int> getContentXAndWidth();

    // Returns a list of all the names of sections in the panel
    StringArray getSectionNames() const;

    // Returns the PropertiesPanel's internal Viewport.
    Viewport& getViewport() noexcept { return viewport; }

    void paint(Graphics& g) override;

    void setTitleAlignment(TitleAlignment newTitleAlignment);

    void setPanelColour(int newPanelColourId);

    void setSeparatorColour(int newSeparatorColourId);

    void setDrawShadowAndOutline(bool shouldDrawShadowAndOutline);

    void setTitleHeight(int newTitleHeight);

    // Sets extra section header text
    // All lines passed in here will be divided equally across the non-label area of the property
    // Useful for naming rows when using a MultiPropertyComponent
    void setExtraHeaderNames(int sectionIndex, StringArray headerNames) const;

    void resized() override;

    void updatePropHolderLayout() const;

    TitleAlignment titleAlignment = AlignWithSection;
    int panelColour;
    int separatorColour;
    bool drawShadowAndOutline = true;
    int titleHeight = 28;
    int contentWidth = 600;
    BouncingViewport viewport;
    PropertyHolderComponent* propertyHolderComponent;
    String messageWhenEmpty;

    friend class PropertiesSearchPanel;
};

class PropertiesSearchPanel final : public Component {

public:
    explicit PropertiesSearchPanel(SmallArray<PropertiesPanel*> const& searchedPanels);

    void resized() override;

    void updateResults();

    void paint(Graphics& g) override;

    void startSearching();
    void stopSearching();

    SearchEditor input;
    PropertiesPanel resultsPanel;
    SmallArray<PropertiesPanel*> panelsToSearch;
};

extern template class PropertiesPanel::EditableComponent<String>;
extern template class PropertiesPanel::EditableComponent<int>;
extern template class PropertiesPanel::EditableComponent<float>;
