/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <melatonin_blur/melatonin_blur.h>

#include <utility>

#include "DraggableNumber.h"
#include "ColourPicker.h"
#include "BouncingViewport.h"
#include "SearchEditor.h"
#include "Dialogs/Dialogs.h"
#include "PluginEditor.h"

class PropertiesPanelProperty : public PropertyComponent {

protected:
    bool hideLabel = false;
    bool roundTopCorner = false;
    bool roundBottomCorner = false;

public:
    explicit PropertiesPanelProperty(String const& propertyName)
        : PropertyComponent(propertyName, 32)
    {
    }

    virtual PropertiesPanelProperty* createCopy() { return nullptr; }

    void setHideLabel(bool labelHidden)
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

    virtual void setRoundedCorners(bool roundTop, bool roundBottom)
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
    struct SectionComponent : public Component {

        SectionComponent(PropertiesPanel& propertiesPanel, String const& sectionTitle,
            PropertiesArray const& newProperties, int extraPadding)
            : Component(sectionTitle)
            , parent(propertiesPanel)
            , padding(extraPadding)

        {
            lookAndFeelChanged();

            propertyComponents.addArray(newProperties);

            for (auto* propertyComponent : propertyComponents) {
                addAndMakeVisible(propertyComponent);
                propertyComponent->refresh();
            }

            if (propertyComponents.size() == 1) {
                propertyComponents[0]->setRoundedCorners(true, true);
            } else if (propertyComponents.size() > 1) {
                propertyComponents.getFirst()->setRoundedCorners(true, false);
                propertyComponents.getLast()->setRoundedCorners(false, true);
            }

            if (parent.drawShadowAndOutline) {
                dropShadow = std::make_unique<melatonin::DropShadow>();
                dropShadow->setColor(Colour(0, 0, 0).withAlpha(0.4f));
                dropShadow->setRadius(7);
                dropShadow->setSpread(0);
            }
        }

        ~SectionComponent() override
        {
            propertyComponents.clear();
        }

        void paint(Graphics& g) override
        {
            auto [x, width] = parent.getContentXAndWidth();

            auto titleX = x;
            if (parent.titleAlignment == AlignWithPropertyName) {
                titleX += 11;
            }

            auto title = getName();
            auto titleHeight = title.isEmpty() ? 0 : parent.titleHeight;

            if (titleHeight != 0) {
                Fonts::drawStyledText(g, title, titleX, 0, width - 4, titleHeight, findColour(PlugDataColour::panelTextColourId), Semibold, 14.5f);
            }

            auto propertyBounds = Rectangle<float>(x, titleHeight + 8.0f, width, getHeight() - (titleHeight + 16.0f));

            if (parent.drawShadowAndOutline) {
                Path p;
                p.addRoundedRectangle(propertyBounds.reduced(3.0f), Corners::largeCornerRadius);
                dropShadow->render(g, p);
            }

            g.setColour(findColour(parent.panelColour));
            g.fillRoundedRectangle(propertyBounds, Corners::largeCornerRadius);

            if (parent.drawShadowAndOutline) {
                g.setColour(findColour(parent.separatorColour));
                g.drawRoundedRectangle(propertyBounds, Corners::largeCornerRadius, 1.0f);
            }

            if (!propertyComponents.isEmpty() && !extraHeaderNames.isEmpty()) {
                auto propertyBounds = Rectangle<int>(x + width / 2, 0, width / 2, parent.titleHeight);
                auto extraHeaderWidth = propertyBounds.getWidth() / static_cast<float>(extraHeaderNames.size());

                for (auto& extraHeader : extraHeaderNames) {
                    auto colour = findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f);
                    Fonts::drawText(g, extraHeader, propertyBounds.removeFromLeft(extraHeaderWidth), colour, 15, Justification::centred);
                }
            }
        }

        void setExtraHeaderNames(StringArray headerNames)
        {
            extraHeaderNames = std::move(headerNames);
            repaint();
        }

        void paintOverChildren(Graphics& g) override
        {
            auto [x, width] = parent.getContentXAndWidth();

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));

            for (int i = 0; i < propertyComponents.size() - 1; i++) {
                auto y = propertyComponents[i]->getBottom() + padding;
                g.drawHorizontalLine(y, x + 10, (x + width) - 10);
            }
        }

        void resized() override
        {
            auto title = getName();
            auto y = title.isNotEmpty() ? parent.titleHeight + 8 : 0;
            auto [x, width] = parent.getContentXAndWidth();

            for (auto* propertyComponent : propertyComponents) {
                propertyComponent->setBounds(x, y, width, propertyComponent->getPreferredHeight());
                y = propertyComponent->getBottom() + padding;
            }
        }

        void lookAndFeelChanged() override
        {
            resized();
            repaint();
        }

        int getPreferredHeight() const
        {
            auto title = getName();
            auto y = title.isNotEmpty() ? parent.titleHeight : 0;

            auto numComponents = propertyComponents.size();

            if (numComponents > 0) {
                for (auto* propertyComponent : propertyComponents)
                    y += propertyComponent->getPreferredHeight();

                y += (numComponents - 1) * padding;
            }

            return y + (title.isNotEmpty() ? 16 : 0);
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (e.getMouseDownX() < parent.titleHeight
                && e.x < parent.titleHeight
                && e.getNumberOfClicks() != 2)
                mouseDoubleClick(e);
        }

        PropertiesPanel& parent;
        OwnedArray<PropertiesPanelProperty> propertyComponents;
        StringArray extraHeaderNames;
        int padding;
        std::unique_ptr<melatonin::DropShadow> dropShadow;
        JUCE_DECLARE_NON_COPYABLE(SectionComponent)
    };

    struct PropertyHolderComponent : public Component {
        PropertyHolderComponent() = default;

        void paint(Graphics&) override { }

        void updateLayout(int width, int viewHeight)
        {
            auto y = 4;

            for (auto* section : sections) {
                section->setBounds(0, y, width, section->getPreferredHeight());
                y = section->getBottom();
            }

            setSize(width, std::max(viewHeight, y));
            repaint();
        }

        void insertSection(int indexToInsertAt, SectionComponent* newSection)
        {
            sections.insert(indexToInsertAt, newSection);
            addAndMakeVisible(newSection, 0);
        }

        SectionComponent* getSectionWithNonEmptyName(int targetIndex) const noexcept
        {
            auto index = 0;
            for (auto* section : sections) {
                if (section->getName().isNotEmpty())
                    if (index++ == targetIndex)
                        return section;
            }

            return nullptr;
        }

        OwnedArray<SectionComponent> sections;

        JUCE_DECLARE_NON_COPYABLE(PropertyHolderComponent)
    };

public:
    struct ComboComponent : public PropertiesPanelProperty {
        ComboComponent(String const& propertyName, Value& value, StringArray const& options)
            : PropertiesPanelProperty(propertyName)
            , items(options)
        {
            comboBox.addItemList(options, 1);
            comboBox.getProperties().set("Style", "Inspector");
            comboBox.getSelectedIdAsValue().referTo(value);

            addAndMakeVisible(comboBox);
        }

        ComboComponent(String const& propertyName, StringArray const& options)
            : PropertiesPanelProperty(propertyName)
            , items(options)
        {
            comboBox.addItemList(options, 1);
            comboBox.getProperties().set("Style", "Inspector");
            addAndMakeVisible(comboBox);
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new ComboComponent(getName(), comboBox.getSelectedIdAsValue(), items);
        }

        StringArray items;
        ComboBox comboBox;
    };

    // Combobox entry that displays the font name with that font
    struct FontEntry : public PopupMenu::CustomComponent {
        String fontName;
        explicit FontEntry(String name)
            : fontName(std::move(name))
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

    struct FontComponent : public PropertiesPanelProperty {
        Value fontValue;
        StringArray options = Font::findAllTypefaceNames();
        bool isFontMissing = false;

        FontComponent(String const& propertyName, Value& value, File const& extraFontsDir = File())
            : PropertiesPanelProperty(propertyName)
        {
            StringArray extraFontOptions;

            if(extraFontsDir.isDirectory() && !extraFontsDir.isRoot()) {
                auto patchFonts = Fonts::getFontsInFolder(extraFontsDir);
                for (int n = 0; n < patchFonts.size(); n++) {
                    extraFontOptions.addIfNotAlreadyThere(patchFonts[n].getFileNameWithoutExtension());
                }
            }
#if JUCE_WINDOWS
            extraFontOptions.addIfNotAlreadyThere("Inter Regular");
#else
            extraFontOptions.addIfNotAlreadyThere("Inter");
#endif

            auto offset = extraFontOptions.size();
            extraFontOptions.addArray(options);

            for (int n = 0; n < extraFontOptions.size(); n++) {
                if (n == offset)
                    comboBox.getRootMenu()->addSeparator();

                comboBox.getRootMenu()->addCustomItem(n + 1, std::make_unique<FontEntry>(extraFontOptions[n]), nullptr, extraFontOptions[n]);

            }

            comboBox.setText(value.toString());
            comboBox.getProperties().set("Style", "Inspector");
            fontValue.referTo(value);

            comboBox.onChange = [this, extraFontOptions, propertyName]() {
                auto fontName = extraFontOptions[comboBox.getSelectedItemIndex()];

                if (fontName.isEmpty()) {
                    isFontMissing = true;
                    fontName = fontValue.toString();
                    PropertiesPanelProperty::setName(propertyName + " (missing)");
                }
                else {
                    isFontMissing = false;
                    PropertiesPanelProperty::setName(propertyName);
                }

                lookAndFeelChanged();
                getParentComponent()->repaint();
                fontValue.setValue(fontName);
            };

            setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

            addAndMakeVisible(comboBox);

            lookAndFeelChanged();
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new FontComponent(getName(), fontValue);
        }

        void lookAndFeelChanged() override
        {
            comboBox.setColour(ComboBox::textColourId, isFontMissing ? Colours::red : findColour(PlugDataColour::panelTextColourId));
        }

        void setFont(String const& fontName)
        {
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
    struct MultiPropertyComponent : public PropertiesPanelProperty {

        OwnedArray<T> properties;
        SmallArray<Value*> propertyValues;
        StringArray propertyOptions;

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
                return new MultiPropertyComponent<T>(getName(), propertyValues, propertyOptions);
            } else {
                return new MultiPropertyComponent<T>(getName(), propertyValues);
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

            int itemWidth = b.getWidth() / propertyValues.size();
            for (int i = 0; i < propertyValues.size(); i++) {
                properties[i]->setBounds(b.removeFromLeft(itemWidth));
            }
        }

        void paintOverChildren(Graphics& g) override
        {
            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            int itemWidth = b.getWidth() / propertyValues.size();

            for (int i = 0; i < propertyValues.size(); i++) {
                auto propertyBounds = b.removeFromLeft(itemWidth);
                g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
                g.drawVerticalLine(propertyBounds.getX(), 0, getHeight());
            }
        }
    };

    struct BoolComponent : public PropertiesPanelProperty
        , public Value::Listener
    {
        BoolComponent(String const& propertyName, Value& value, StringArray options)
            : PropertiesPanelProperty(propertyName)
            , textOptions(std::move(options))
            , toggleStateValue(value)
        {
            init();
        }

        // Also allow creating it without passing in a Value, makes it easier to derive from this class for custom bool components
        BoolComponent(String const& propertyName, StringArray options)
            : PropertiesPanelProperty(propertyName)
            , textOptions(std::move(options))
        {
            init();
        }

        // Allow creation without an attached juce::Value, but with an initial value
        // We need this constructor sometimes to prevent feedback caused by the initial value being set after the listener is attached
        BoolComponent(String const& propertyName, bool initialValue, StringArray options)
            : PropertiesPanelProperty(propertyName)
            , textOptions(std::move(options))
        {
            toggleStateValue = initialValue;
            init();
        }

        void init()
        {
            toggleStateValue.addListener(this);
            setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
            lookAndFeelChanged();
        }

        ~BoolComponent()
        {
            toggleStateValue.removeListener(this);
        }

        void lookAndFeelChanged() override
        {
            repaint();
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new BoolComponent(getName(), toggleStateValue, textOptions);
        }

        bool hitTest(int x, int y) override
        {
            if (!isEnabled())
                return false;

            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            return bounds.contains(x, y);
        }

        void paint(Graphics& g) override
        {
            bool isDown = getValue<bool>(toggleStateValue);
            bool isOver = isMouseOver();
            
            auto bounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2.0f - hideLabel));
            auto buttonBounds = bounds.reduced(4);

            if(isDown || isOver) {
                // Add some alpha to make it look good on any background...
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId).contrasting(isOver ? 0.125f : 0.2f).withAlpha(0.25f));
                g.fillRoundedRectangle(buttonBounds, Corners::defaultCornerRadius);
            }
            auto textColour = findColour(PlugDataColour::panelTextColourId);

            if (!isEnabled()) {
                textColour = findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f);
            }
            Fonts::drawText(g, textOptions[isDown], bounds, textColour, 14.0f, Justification::centred);

            // Paint label
            PropertiesPanelProperty::paint(g);
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
            toggleStateValue.setValue(!getValue<bool>(toggleStateValue));
            repaint();
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(toggleStateValue))
                repaint();
        }

    protected:
        StringArray textOptions;
        Value toggleStateValue;
    };

    struct InspectorColourComponent : public PropertiesPanelProperty
        , public Value::Listener {

        struct SwatchComponent : public Component {
            explicit SwatchComponent(Value const& colour)
            {
                colourValue.referTo(colour);
            }

            void paint(Graphics& g) override
            {
                auto colour = Colour::fromString(colourValue.toString());
                
                g.setColour(isMouseOver() ? colour.brighter(0.4f) : colour);
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::defaultCornerRadius);
                g.setColour(colour.darker(0.15f));
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::defaultCornerRadius, 0.8f);
            }

            void mouseEnter(MouseEvent const& e) override
            {
                repaint();
            }

            void mouseExit(MouseEvent const& e) override
            {
                repaint();
            }

            Value colourValue;
        };

        InspectorColourComponent(String const& propertyName, Value& value)
            : PropertiesPanelProperty(propertyName)
            , swatchComponent(value)
        {

            currentColour.referTo(value);
            setWantsKeyboardFocus(true);

            currentColour.addListener(this);

            swatchComponent.setInterceptsMouseClicks(false, false);
            addAndMakeVisible(swatchComponent);
            
            addAndMakeVisible(hexValueEditor);
            hexValueEditor.setJustificationType(Justification::centred);
            hexValueEditor.setInterceptsMouseClicks(false, true);
            hexValueEditor.setFont(Fonts::getCurrentFont().withHeight(13.5f));
            
            hexValueEditor.onEditorShow = [this](){
                auto* editor = hexValueEditor.getCurrentTextEditor();
                editor->setBorder(BorderSize<int>(0, 0, 4, 1));
                editor->setJustification(Justification::centred);
                editor->setInputRestrictions(7, "#0123456789ABCDEFabcdef");
                editor->applyColourToAllText(Colour::fromString(currentColour.toString()).contrasting(0.95f));
            };
            
            hexValueEditor.onEditorHide = [this]() {
                colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
                currentColour.setValue(colour);
            };

            hexValueEditor.onTextChange = [this]() {
                colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
            };

            updateHexValue();

            setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

            repaint();
        }

        ~InspectorColourComponent() override
        {
            currentColour.removeListener(this);
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new InspectorColourComponent(getName(), currentColour);
        }

        void updateHexValue()
        {
            hexValueEditor.setColour(Label::textColourId, Colour::fromString(currentColour.toString()).contrasting(0.95f));
            hexValueEditor.setText(String("#") + currentColour.toString().substring(2).toUpperCase(), dontSendNotification);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            swatchComponent.setBounds(bounds.reduced(4));
            hexValueEditor.setBounds(bounds.reduced(1));
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(currentColour)) {
                updateHexValue();
                repaint();
            }
        }
            
        void mouseDown(MouseEvent const& e) override
        {
            if(hexValueEditor.isBeingEdited() && e.getNumberOfClicks() > 1) return;
            
            Timer::callAfterDelay(250, [_this = SafePointer(this)](){
                if(!_this || _this->hexValueEditor.isBeingEdited() || ColourPicker::getInstance().isShowing()) return;
                
                auto pickerBounds = _this->getScreenBounds().expanded(5);
                
                ColourPicker::getInstance().show(_this->findParentComponentOfClass<PluginEditor>(), _this->getTopLevelComponent(), false, Colour::fromString(_this->currentColour.toString()), pickerBounds, [_this](Colour c) {
                    if (!_this)
                        return;

                    _this->currentColour = c.toString();
                    _this->repaint();
                });
            });
        }
        
        void mouseDoubleClick (MouseEvent const& e) override
        {
            if(hexValueEditor.isBeingEdited() || ColourPicker::getInstance().isShowing()) return;
            hexValueEditor.showEditor();
        }

    private:
        SwatchComponent swatchComponent;
        Value currentColour;
        Value colour;
        Label hexValueEditor;
    };

    struct ColourComponent : public PropertiesPanelProperty
        , public Value::Listener {

        struct SwatchComponent : public Component {

            explicit SwatchComponent(Value const& colour)
            {
                colourValue.referTo(colour);
            }

            void paint(Graphics& g) override
            {
                auto colour = Colour::fromString(colourValue.toString());

                g.setColour(isMouseOver() ? colour.brighter(0.4f) : colour);
                g.fillEllipse(getLocalBounds().reduced(1).toFloat());
                g.setColour(colour.darker(0.2f));
                g.drawEllipse(getLocalBounds().reduced(1).toFloat(), 0.8f);
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
                auto pickerBounds = getScreenBounds().expanded(5);
                ColourPicker::getInstance().show(findParentComponentOfClass<PluginEditor>(), getTopLevelComponent(), false, Colour::fromString(colourValue.toString()), pickerBounds, [_this = SafePointer(this)](Colour c) {
                    if (!_this)
                        return;

                    _this->colourValue = c.toString();
                    _this->repaint();
                });
            }

            Value colourValue;
        };

        ColourComponent(String const& propertyName, Value& value)
            : PropertiesPanelProperty(propertyName)
            , swatchComponent(value)
        {

            currentColour.referTo(value);
            currentColour.addListener(this);
            setWantsKeyboardFocus(false);
            
            addAndMakeVisible(hexValueEditor);
            hexValueEditor.getProperties().set("NoOutline", true);
            hexValueEditor.getProperties().set("NoBackground", true);
            hexValueEditor.setInputRestrictions(7, "#0123456789ABCDEFabcdef");
            hexValueEditor.setColour(outlineColourId, Colour());
            hexValueEditor.setJustification(Justification::centred);

            hexValueEditor.onReturnKey = [this]() {
                grabKeyboardFocus();
            };

            hexValueEditor.onTextChange = [this]() {
                colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
            };

            hexValueEditor.onFocusLost = [this]() {
                currentColour.setValue(colour);
            };

            addAndMakeVisible(swatchComponent);
            updateHexValue();

            setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

            repaint();
        }

        ~ColourComponent() override
        {
            currentColour.removeListener(this);
        }

        void lookAndFeelChanged() override
        {
            // TextEditor is special, setColour() will only change newly typed text colour
            hexValueEditor.applyColourToAllText(findColour(PlugDataColour::panelTextColourId));
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new ColourComponent(getName(), currentColour);
        }

        void updateHexValue()
        {
            hexValueEditor.setText(String("#") + currentColour.toString().substring(2).toUpperCase());
        }

        void resized() override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            auto colourSwatchBounds = bounds.removeFromLeft(getHeight()).reduced(4).translated(12, 0);

            swatchComponent.setBounds(colourSwatchBounds);
            hexValueEditor.setBounds(bounds.translated(0, -3));
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(currentColour)) {
                updateHexValue();
                repaint();
            }
        }

    private:
        SwatchComponent swatchComponent;
        Value currentColour;
        Value colour;
        TextEditor hexValueEditor;
    };

    struct RangeComponent : public PropertiesPanelProperty
        , public Value::Listener {
        Value property;

        DraggableNumber minLabel, maxLabel;

        float min, max;

        RangeComponent(String const& propertyName, Value& value, bool integerMode)
            : PropertiesPanelProperty(propertyName)
            , property(value)
            , minLabel(integerMode)
            , maxLabel(integerMode)
        {
            property.addListener(this);

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
                VarArray arr = { min, max };
                // maxLabel.setMinimum(min + 1e-5f);
                property = var(arr);
            };

            auto setMaximum = [this](float value) {
                max = value;
                VarArray arr = { min, max };
                // minLabel.setMaximum(max - 1e-5f);
                property = var(arr);
            };

            minLabel.onValueChange = setMinimum;
            minLabel.onTextChange = [this, setMinimum]() {
                setMinimum(minLabel.getText().getFloatValue());
            };

            maxLabel.onValueChange = setMaximum;
            maxLabel.onTextChange = [this, setMaximum]() {
                setMaximum(maxLabel.getText().getFloatValue());
            };
        }

        ~RangeComponent() override
        {
            property.removeListener(this);
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new RangeComponent(getName(), property, false);
        }

        DraggableNumber& getMinimumComponent()
        {
            return minLabel;
        }

        DraggableNumber& getMaximumComponent()
        {
            return maxLabel;
        }

        void setIntegerMode(bool integerMode)
        {
            minLabel.setDragMode(integerMode ? DraggableNumber::Integer : DraggableNumber::Regular);
            maxLabel.setDragMode(integerMode ? DraggableNumber::Integer : DraggableNumber::Regular);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            maxLabel.setBounds(bounds.removeFromRight(bounds.getWidth() / (2 - hideLabel)));
            minLabel.setBounds(bounds);
        }

        void valueChanged(Value& v) override
        {
            if (v.refersToSameSourceAs(property)) {
                min = v.getValue().getArray()->getReference(0);
                max = v.getValue().getArray()->getReference(1);
                minLabel.setText(String(min), dontSendNotification);
                maxLabel.setText(String(max), dontSendNotification);
            }
        }
    };

    template<typename T>
    struct EditableComponent : public PropertiesPanelProperty {
        std::unique_ptr<Label> label;
        Value property;
        String allowedCharacters = "";

        EditableComponent(String propertyName, Value& value, double minimum = 0.0, double maximum = 0.0, std::function<void(bool)> onInteractionFn = nullptr)
            : PropertiesPanelProperty(propertyName)
            , property(value)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                auto* draggableNumber = new DraggableNumber(std::is_integral<T>::value);
                label = std::unique_ptr<DraggableNumber>(draggableNumber);

                // By setting the text before attaching the value, we can prevent an unnesssary/harmful call to ValueChanged
                draggableNumber->setText(property.toString(), dontSendNotification);
                draggableNumber->getTextValue().referTo(property);
                draggableNumber->setFont(draggableNumber->getFont().withHeight(14));
                draggableNumber->setEditableOnClick(true);
                if (minimum != 0.0f)
                    draggableNumber->setMinimum(minimum);
                if (maximum != 0.0f)
                    draggableNumber->setMaximum(maximum);

                if (onInteractionFn)
                    draggableNumber->onInteraction = onInteractionFn;

                draggableNumber->onEditorShow = [draggableNumber]() {
                    auto* editor = draggableNumber->getCurrentTextEditor();
                    editor->setBorder(BorderSize<int>(2, 1, 4, 1));
                    editor->setJustification(Justification::centredLeft);
                    if constexpr (std::is_floating_point<T>::value) {
                        //editor->setInputRestrictions(0, "0123456789.-");
                    } else if constexpr (std::is_integral<T>::value) {
                        //editor->setInputRestrictions(0, "0123456789-");
                    }
                };
            } else {
                label = std::make_unique<Label>();
                label->setEditable(true, false);
                label->getTextValue().referTo(property);
                label->setFont(Font(14));

                label->onEditorShow = [this]() {
                    auto* editor = label->getCurrentTextEditor();
                    editor->setBorder(BorderSize<int>(2, 1, 4, 1));
                    editor->setJustification(Justification::centredLeft);

                    if (allowedCharacters.isNotEmpty()) {
                        editor->setInputRestrictions(0, allowedCharacters);
                    }
                };

                label->onEditorHide = [this]() {
                    // synchronise the value to the canvas when updated
                    if (PluginEditor* pluginEditor = findParentComponentOfClass<PluginEditor>()) {
                        if (auto cnv = pluginEditor->getCurrentCanvas())
                            cnv->synchronise();
                    }
                };
            }

            addAndMakeVisible(label.get());

            label->addMouseListener(this, true);
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new EditableComponent<T>(getName(), property);
        }

        void setInputRestrictions(String const& newAllowedCharacters)
        {
            allowedCharacters = newAllowedCharacters;
        }

        void setRangeMin(float minimum)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                dynamic_cast<DraggableNumber*>(label.get())->setMinimum(minimum);
            }
        }

        void setRangeMax(float maximum)
        {
            if constexpr (std::is_arithmetic<T>::value) {
                dynamic_cast<DraggableNumber*>(label.get())->setMaximum(maximum);
            }
        }

        void resized() override
        {
            label->setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
        }

        void editableOnClick(bool editable)
        {
            dynamic_cast<DraggableNumber*>(label.get())->setEditableOnClick(editable);
        }
    };

    struct FilePathComponent : public PropertiesPanelProperty {
        Label label;
        SmallIconButton browseButton = SmallIconButton(Icons::File);
        Value property;

        FilePathComponent(String const& propertyName, Value& value)
            : PropertiesPanelProperty(propertyName)
            , property(value)
        {
            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);
            label.setFont(Font(14));

            addAndMakeVisible(label);
            addAndMakeVisible(browseButton);

            browseButton.onClick = [this]() {
                Dialogs::showSaveDialog([this](URL url) {
                    auto result = url.getLocalFile();
                    if (result.getParentDirectory().exists()) {
                        label.setText(result.getFullPathName(), sendNotification);
                    }
                },
                    "", "", getTopLevelComponent());
            };
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new FilePathComponent(getName(), property);
        }

        void paint(Graphics& g) override
        {
            PropertiesPanelProperty::paint(g);

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

    class ActionComponent : public PropertiesPanelProperty {

        bool mouseIsOver = false;
        bool roundTop, roundBottom;

    public:
        ActionComponent(std::function<void()> callback, String iconToShow, String const& textToShow, bool roundOnTop = false, bool roundOnBottom = false)
            : PropertiesPanelProperty(textToShow)
            , roundTop(roundOnTop)
            , roundBottom(roundOnBottom)
            , onClick(std::move(callback))
            , icon(std::move(iconToShow))
        {
            setHideLabel(true);
        }

        PropertiesPanelProperty* createCopy() override
        {
            return new ActionComponent(onClick, icon, getName(), roundTop, roundBottom);
        }

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds();
            auto textBounds = bounds;
            auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto colour = findColour(PlugDataColour::panelTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));

                Path p;
                p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, roundBottom, roundBottom);
                g.fillPath(p);
            }

            Fonts::drawIcon(g, icon, iconBounds, colour, 12);
            Fonts::drawText(g, getName(), textBounds, colour, 15);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            mouseIsOver = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            mouseIsOver = false;
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            onClick();
        }

        std::function<void()> onClick = []() { };
        String icon;
    };

    PropertiesPanel()
    {
        messageWhenEmpty = "(nothing settable)";

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(propertyHolderComponent = new PropertyHolderComponent());
        viewport.setFocusContainerType(FocusContainerType::focusContainer);

        viewport.addMouseListener(this, true);

        panelColour = PlugDataColour::panelForegroundColourId;
        separatorColour = PlugDataColour::toolbarOutlineColourId;
    }

    /** Destructor. */
    ~PropertiesPanel() override
    {
        viewport.removeMouseListener(this);
        clear();
    }

    // Deletes all property components from the panel
    void clear() const
    {
        if (!isEmpty()) {
            propertyHolderComponent->sections.clear();
            updatePropHolderLayout();
        }
    }

    // Adds a set of properties to the panel
    void addSection(String const& sectionTitle, PropertiesArray const& newProperties, int indexToInsertAt = -1, int extraPaddingBetweenComponents = 0)
    {
        if (isEmpty())
            repaint();

        propertyHolderComponent->insertSection(indexToInsertAt, new SectionComponent(*this, sectionTitle, newProperties, extraPaddingBetweenComponents));
        updatePropHolderLayout();
    }

    // Returns true if the panel contains no properties
    bool isEmpty() const
    {
        return propertyHolderComponent->sections.size() == 0;
    }

    void setContentWidth(int newContentWidth)
    {
        contentWidth = newContentWidth;
        resized();
        repaint();
    }

    Component* getSectionByName(String const& name) const noexcept
    {
        if (propertyHolderComponent) {
            for (auto* section : propertyHolderComponent->sections) {
                if (section->getName() == name)
                    return section;
            }
        }

        return nullptr;
    }

    std::pair<int, int> getContentXAndWidth()
    {
        auto marginWidth = (getWidth() - contentWidth) / 2;
        return { marginWidth, contentWidth };
    }

    // Returns a list of all the names of sections in the panel
    StringArray getSectionNames() const
    {
        StringArray s;

        for (auto* section : propertyHolderComponent->sections) {
            if (section->getName().isNotEmpty())
                s.add(section->getName());
        }

        return s;
    }

    // Returns the PropertiesPanel's internal Viewport.
    Viewport& getViewport() noexcept { return viewport; }

    void paint(Graphics& g) override
    {
        if (isEmpty()) {
            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f));
            g.setFont(14.0f);
            g.drawText(messageWhenEmpty, getLocalBounds().withHeight(30),
                Justification::centred, true);
        }
    }

    void setTitleAlignment(TitleAlignment newTitleAlignment)
    {
        titleAlignment = newTitleAlignment;
    }

    void setPanelColour(int newPanelColourId)
    {
        panelColour = newPanelColourId;
    }
    
    void setPanelAlpha(float panelTransparency)
    {
        panelAlpha = panelTransparency;
    }

    void setSeparatorColour(int newSeparatorColourId)
    {
        separatorColour = newSeparatorColourId;
    }

    void setDrawShadowAndOutline(bool shouldDrawShadowAndOutline)
    {
        drawShadowAndOutline = shouldDrawShadowAndOutline;
    }

    void setTitleHeight(int newTitleHeight)
    {
        titleHeight = newTitleHeight;
    }

    // Sets extra section header text
    // All lines passed in here will be divided equally across the non-label area of the property
    // Useful for naming rows when using a MultiPropertyComponent
    void setExtraHeaderNames(int sectionIndex, StringArray headerNames) const
    {
        if (auto* s = propertyHolderComponent->getSectionWithNonEmptyName(sectionIndex)) {
            s->setExtraHeaderNames(std::move(headerNames));
        }
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds().withTrimmedTop(1));
        updatePropHolderLayout();
    }

    void updatePropHolderLayout() const
    {
        auto maxWidth = viewport.getMaximumVisibleWidth();
        auto maxHeight = viewport.getMaximumVisibleHeight();
        propertyHolderComponent->updateLayout(maxWidth, maxHeight);

        auto newMaxWidth = viewport.getMaximumVisibleWidth();
        if (maxWidth != newMaxWidth) {
            // need to do this twice because of vertical scrollbar changing the size, etc.
            propertyHolderComponent->updateLayout(newMaxWidth, maxHeight);
        }
    }

    TitleAlignment titleAlignment = AlignWithSection;
    int panelColour;
    int separatorColour;
    float panelAlpha = 1.0f;
    bool drawShadowAndOutline = true;
    int titleHeight = 28;
    int contentWidth = 600;
    BouncingViewport viewport;
    PropertyHolderComponent* propertyHolderComponent;
    String messageWhenEmpty;

    friend class PropertiesSearchPanel;
};

class PropertiesSearchPanel : public Component {

public:
    PropertiesSearchPanel(SmallArray<PropertiesPanel*> searchedPanels)
        : panelsToSearch(searchedPanels)
    {
        addAndMakeVisible(resultsPanel);
        resultsPanel.messageWhenEmpty = "";

        addAndMakeVisible(input);
        input.setTextToShowWhenEmpty("Type to search for settings", findColour(TextEditor::textColourId).withAlpha(0.5f));
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        input.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        input.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        input.setJustification(Justification::centredLeft);
        input.setBorder({ 0, 3, 5, 1 });
        input.onTextChange = [this]() {
            updateResults();
        };
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().removeFromTop(42).reduced(48, 6).translated(-2, -1));
        resultsPanel.setBounds(getLocalBounds().withTrimmedTop(40));
    }

    void updateResults()
    {
        resultsPanel.clear();

        auto query = input.getText().toLowerCase();
        if (query.isEmpty())
            return;

        for (auto* propertiesPanel : panelsToSearch) {
            for (auto* section : propertiesPanel->propertyHolderComponent->sections) {
                PropertiesArray properties;
                auto sectionTitle = section->getName();

                for (auto* property : section->propertyComponents) {
                    if (property->getName().toLowerCase().contains(query) || sectionTitle.toLowerCase().contains(query)) {
                        auto* propertyCopy = property->createCopy();
                        if (propertyCopy)
                            properties.add(propertyCopy);
                    }
                }
                if (!properties.isEmpty()) {
                    resultsPanel.addSection(sectionTitle, properties);
                }
            }
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(40, 0.0f, getWidth());
    }

    void startSearching()
    {
        setVisible(true);
        input.grabKeyboardFocus();
    }

    void stopSearching()
    {
        setVisible(false);
        input.setText("");
    }

    SearchEditor input;
    PropertiesPanel resultsPanel;
    SmallArray<PropertiesPanel*> panelsToSearch;
};
