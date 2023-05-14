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

class PropertiesPanel : public Component {
    
public:
    class Property : public PropertyComponent {
        
    protected:
        bool hideLabel = false;
        bool roundTopCorner = false;
        bool roundBottomCorner = false;
        
    public:
        Property(String const& propertyName)
        : PropertyComponent(propertyName, 32)
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
                getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.85f, *this);
            }
        }
        
        virtual void setRoundedCorners(bool roundTop, bool roundBottom)
        {
            roundTopCorner = roundTop;
            roundBottomCorner = roundBottom;
            repaint();
        }
        
        void refresh() override {};
    };
    
private:
    struct SectionComponent  : public Component
    {
        SectionComponent (const String& sectionTitle,
                          const Array<Property*>& newProperties, int extraPadding)
            : Component (sectionTitle),
              padding (extraPadding)
        {
            lookAndFeelChanged();

            propertyComps.addArray (newProperties);

            for (auto* propertyComponent : propertyComps)
            {
                addAndMakeVisible (propertyComponent);
                propertyComponent->refresh();
            }
            
            if(propertyComps.size() == 1)
            {
                propertyComps[0]->setRoundedCorners(true, true);
            }
            else if (propertyComps.size() > 1){
                propertyComps.getFirst()->setRoundedCorners(true, false);
                propertyComps.getLast()->setRoundedCorners(false, true);
            }
        }

        ~SectionComponent() override
        {
            propertyComps.clear();
        }

        void paint(Graphics& g) override
        {
            auto marginWidth = getWidth() - 600;
            int x = marginWidth / 2;
            int width = getWidth() - marginWidth;
            
            Fonts::drawStyledText(g, getName(), x + 8, 0, width - 4, titleHeight, findColour(PropertyComponent::labelTextColourId), Semibold, 15.0f);
            
            auto propertyBounds = Rectangle<float>(x, titleHeight + 8.0f, width, getHeight() - (titleHeight + 16.0f));
            
            Path p;
            p.addRoundedRectangle(propertyBounds.reduced(3.0f), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 6, {0, 1});
            
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId).brighter(0.25f));
            g.fillRoundedRectangle(propertyBounds, Corners::largeCornerRadius);
            
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
            g.drawRoundedRectangle(propertyBounds, Corners::largeCornerRadius, 1.0f);
            
            if(!propertyComps.isEmpty() && !extraHeaderNames.isEmpty())
            {
                auto propertyBounds = Rectangle<int>(x + width / 2 , 0, width / 2, titleHeight);
                auto extraHeaderWidth = propertyBounds.getWidth() / static_cast<float>(extraHeaderNames.size());
                
                for(auto& extraHeader : extraHeaderNames)
                {
                    auto colour = findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f);
                    Fonts::drawText(g, extraHeader, propertyBounds.removeFromLeft(extraHeaderWidth), colour, 15, Justification::centred);
                }
            }
        }
        
        void setExtraHeaderNames(StringArray headerNames)
        {
            extraHeaderNames = headerNames;
            repaint();
        }
        
        void paintOverChildren(Graphics& g) override
        {
            auto y = titleHeight + 8;
            auto marginWidth = getWidth() - 600;
            int x = marginWidth / 2;
            int width = getWidth() - marginWidth;
            
            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
            
            for (int i = 0; i < propertyComps.size() - 1; i++)
            {
                y = propertyComps[i]->getBottom() + padding;
                g.drawHorizontalLine(y, x, x + width);
            }
        }

        void resized() override
        {
            auto y = titleHeight + 8;
            auto marginWidth = getWidth() - 600;
            int x = marginWidth / 2;
            int newWidth = getWidth() - marginWidth;
            
            for (auto* propertyComponent : propertyComps)
            {
                propertyComponent->setBounds (x, y, newWidth, propertyComponent->getPreferredHeight());
                y = propertyComponent->getBottom() + padding;
            }
        }

        void lookAndFeelChanged() override
        {
            titleHeight = getLookAndFeel().getPropertyPanelSectionHeaderHeight (getName());
            resized();
            repaint();
        }

        int getPreferredHeight() const
        {
            auto y = titleHeight;

            auto numComponents = propertyComps.size();

            if (numComponents > 0)
            {
                for (auto* propertyComponent : propertyComps)
                    y += propertyComponent->getPreferredHeight();

                y += (numComponents - 1) * padding;
            }

            return y + 16;
        }
        
        void refreshAll() const
        {
            for (auto* propertyComponent : propertyComps)
                propertyComponent->refresh();
        }

        void mouseUp(const MouseEvent& e) override
        {
            if (e.getMouseDownX() < titleHeight
                  && e.x < titleHeight
                  && e.getNumberOfClicks() != 2)
                mouseDoubleClick (e);
        }

        OwnedArray<Property> propertyComps;
        StringArray extraHeaderNames;
        int titleHeight;
        int padding;

        JUCE_DECLARE_NON_COPYABLE (SectionComponent)
    };

    //==============================================================================
    struct PropertyHolderComponent  : public Component
    {
        PropertyHolderComponent() {}

        void paint (Graphics&) override {}

        void updateLayout (int width)
        {
            auto y = 4;

            for (auto* section : sections)
            {
                section->setBounds (0, y, width, section->getPreferredHeight());
                y = section->getBottom();
            }

            setSize (width, y);
            repaint();
        }

        void refreshAll() const
        {
            for (auto* section : sections)
                section->refreshAll();
        }

        void insertSection (int indexToInsertAt, SectionComponent* newSection)
        {
            sections.insert (indexToInsertAt, newSection);
            addAndMakeVisible (newSection, 0);
        }

        SectionComponent* getSectionWithNonEmptyName (int targetIndex) const noexcept
        {
            auto index = 0;
            for (auto* section : sections)
            {
                if (section->getName().isNotEmpty())
                    if (index++ == targetIndex)
                        return section;
            }

            return nullptr;
        }

        OwnedArray<SectionComponent> sections;

        JUCE_DECLARE_NON_COPYABLE (PropertyHolderComponent)
    };
    
public:
    
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
        
        void setRoundedCorners(bool roundTop, bool roundBottom) override
        {
            properties.getLast()->setRoundedCorners(roundTop, roundBottom);
        }
        
        void resized() override
        {
            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            
            int itemWidth = b.getWidth() / numProperties;
            for (int i = 0; i < numProperties; i++) {
                properties[i]->setBounds(b.removeFromLeft(itemWidth));
            }
        }
        
        void paintOverChildren(Graphics& g) override
        {
            auto b = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
            int itemWidth = b.getWidth() / numProperties;
            
            for(int i = 0; i < numProperties; i++)
            {
                auto propertyBounds = b.removeFromLeft(itemWidth);
                g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
                g.drawVerticalLine(propertyBounds.getX(), 0, getHeight());
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
            
            auto bounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2 - hideLabel));
            
            if (isDown || isHovered) {
                Path p;
                p.addRoundedRectangle (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, false, roundTopCorner, false, roundBottomCorner);
                
                // Add some alpha to make it look good on any background...
                g.setColour(findColour(TextButton::buttonColourId).withAlpha(isDown ? 0.9f : 0.7f));
                g.fillPath(p);
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
            
            auto bounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2 - hideLabel));
            
            Path p;
            p.addRoundedRectangle (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, false, roundTopCorner, false, roundBottomCorner);
            
            g.setColour(isMouseOver() ? colour.brighter(0.4f) : colour);
            g.fillPath(p);

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

    struct RangeComponent : public Property, public Value::Listener {
        Value property;
        
        DraggableNumber minLabel, maxLabel;
        
        float min, max;
        
        RangeComponent(String propertyName, Value& value)
        : Property(propertyName)
        , property(value)
        , minLabel(false)
        , maxLabel(false)
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

        ~RangeComponent()
        {
            property.removeListener(this);
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
    
    class ActionComponent : public Property {

        bool mouseIsOver = false;
        bool roundTop, roundBottom;
        
    public:
        
        ActionComponent(std::function<void()> callback, String iconToShow, String textToShow, bool roundOnTop = false, bool roundOnBottom = false) : Property(textToShow), icon(iconToShow), roundTop(roundOnTop),  roundBottom(roundOnBottom), onClick(callback)
        {
            setHideLabel(true);
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
                p.addRoundedRectangle (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, true, true);
                g.fillPath(p);

                colour = findColour(PlugDataColour::panelActiveTextColourId);
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
        
        std::function<void()> onClick = []() {};
        String icon;
    };

    
    
    PropertiesPanel()
    {
        init();
    }
    
    /** Destructor. */
    ~PropertiesPanel() override
    {
        clear();
    }
    
    void init()
    {
        messageWhenEmpty = "(nothing selected)";
        
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (propertyHolderComponent = new PropertyHolderComponent());
        viewport.setFocusContainerType (FocusContainerType::keyboardFocusContainer);
    }
    
    //==============================================================================
    /** Deletes all property components from the panel. */
    void clear() {
        if (!isEmpty())
        {
            propertyHolderComponent->sections.clear();
            updatePropHolderLayout();
        }
    }
    
    // Adds a set of properties to the panel.
    void addProperties (const Array<Property*>& newProperties,
                        int extraPaddingBetweenComponents = 0)
    {
        if (isEmpty())
            repaint();
        
        propertyHolderComponent->insertSection (-1, new SectionComponent ({}, newProperties, extraPaddingBetweenComponents));
        updatePropHolderLayout();
        
    }
    
    /** Adds a set of properties to the panel.
     
     These properties are added under a section heading with a plus/minus button that
     allows it to be opened and closed. If indexToInsertAt is < 0 then it will be added
     at the end of the list, or before the given index if the index is non-zero.
     
     The components in the list will be owned by this object and will be automatically
     deleted later on when no longer needed.
     
     To add properties without them being in a section, use addProperties().
     */
    void addSection (const String& sectionTitle,
                     const Array<Property*>& newProperties,
                     int indexToInsertAt = -1,
                     int extraPaddingBetweenComponents = 0)
    {
        jassert (sectionTitle.isNotEmpty());
        
        if (isEmpty())
            repaint();
        
        propertyHolderComponent->insertSection (indexToInsertAt, new SectionComponent (sectionTitle,
                                                                                       newProperties,
                                                                                       extraPaddingBetweenComponents));
        
        updatePropHolderLayout();
    }
    
    /** Calls the refresh() method of all PropertyComponents in the panel */
    void refreshAll() const
    {
        propertyHolderComponent->refreshAll();
    }
    
    /** Returns true if the panel contains no properties. */
    bool isEmpty() const
    {
        return propertyHolderComponent->sections.size() == 0;
    }
    
    /** Returns the height that the panel needs in order to display all of its content
     without scrolling.
     */
    int getTotalContentHeight() const
    {
        return propertyHolderComponent->getHeight();
    }
    
    //==============================================================================
    /** Returns a list of all the names of sections in the panel.
     These are the sections that have been added with addSection().
     */
    StringArray getSectionNames() const
    {
        StringArray s;
        
        for (auto* section : propertyHolderComponent->sections)
        {
            if (section->getName().isNotEmpty())
                s.add (section->getName());
        }
        
        return s;
    }
   
    void removeSection (int sectionIndex)
    {
        if (auto* s = propertyHolderComponent->getSectionWithNonEmptyName (sectionIndex))
        {
            propertyHolderComponent->sections.removeObject (s);
            updatePropHolderLayout();
        }
    }
    
    //==============================================================================
    /** Sets a message to be displayed when there are no properties in the panel.
     The default message is "nothing selected".
     */
    void setMessageWhenEmpty (const String& newMessage);
    
    /** Returns the message that is displayed when there are no properties.
     */
    const String& getMessageWhenEmpty() const noexcept;
    
    //==============================================================================
    /** Returns the PropertyPanel's internal Viewport. */
    Viewport& getViewport() noexcept        { return viewport; }
    
    //==============================================================================
    void paint (Graphics& g) override
    {
        if (isEmpty())
        {
            g.setColour (Colours::black.withAlpha (0.5f));
            g.setFont (14.0f);
            g.drawText (messageWhenEmpty, getLocalBounds().withHeight (30),
                        Justification::centred, true);
        }
    }
    
    void setExtraHeaderNames(int sectionIndex, StringArray headerNames)
    {        
        if (auto* s = propertyHolderComponent->getSectionWithNonEmptyName (sectionIndex))
        {
            s->setExtraHeaderNames(headerNames);
        }
    }
    
    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        updatePropHolderLayout();
    }
    
    void updatePropHolderLayout() const
    {
        auto maxWidth = viewport.getMaximumVisibleWidth();
        propertyHolderComponent->updateLayout (maxWidth);
        
        auto newMaxWidth = viewport.getMaximumVisibleWidth();
        if (maxWidth != newMaxWidth)
        {
            // need to do this twice because of scrollbars changing the size, etc.
            propertyHolderComponent->updateLayout (newMaxWidth);
        }
    }
    
    
    Viewport viewport;
    PropertyHolderComponent* propertyHolderComponent;
    String messageWhenEmpty;
};
