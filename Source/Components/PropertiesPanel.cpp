/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Constants.h"
#include "Buttons.h"

#include "PropertiesPanel.h"
#include "ColourPicker.h"
#include "Dialogs/Dialogs.h"

PropertiesPanel::SectionComponent::SectionComponent(PropertiesPanel& propertiesPanel, String const& sectionTitle,
                                                    PropertiesArray const& newProperties, int const extraPadding)
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

PropertiesPanel::SectionComponent::~SectionComponent()
{
    propertyComponents.clear();
}

void PropertiesPanel::SectionComponent::paint(Graphics& g)
{
    auto [x, width] = parent.getContentXAndWidth();
    
    auto titleX = x;
    if (parent.titleAlignment == AlignWithPropertyName) {
        titleX += 11;
    }
    
    auto const title = getName();
    auto const titleHeight = title.isEmpty() ? 0 : parent.titleHeight;
    
    if (titleHeight != 0) {
        Fonts::drawStyledText(g, title, titleX, 0, width - 4, titleHeight, findColour(PlugDataColour::panelTextColourId), Semibold, 14.5f);
    }
    
    auto const propertyBounds = Rectangle<float>(x, titleHeight + 8.0f, width, getHeight() - (titleHeight + 16.0f));
    
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
        auto const extraHeaderWidth = propertyBounds.getWidth() / static_cast<float>(extraHeaderNames.size());
        
        for (auto& extraHeader : extraHeaderNames) {
            auto const colour = findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f);
            Fonts::drawText(g, extraHeader, propertyBounds.removeFromLeft(extraHeaderWidth), colour, 15, Justification::centred);
        }
    }
}

void PropertiesPanel::SectionComponent::setExtraHeaderNames(StringArray headerNames)
{
    extraHeaderNames = std::move(headerNames);
    repaint();
}

void PropertiesPanel::SectionComponent::paintOverChildren(Graphics& g)
{
    auto [x, width] = parent.getContentXAndWidth();
    
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    
    for (int i = 0; i < propertyComponents.size() - 1; i++) {
        auto const y = propertyComponents[i]->getBottom() + padding;
        g.drawHorizontalLine(y, x + 10, x + width - 10);
    }
}

void PropertiesPanel::SectionComponent::resized()
{
    auto const title = getName();
    auto y = title.isNotEmpty() ? parent.titleHeight + 8 : 0;
    auto [x, width] = parent.getContentXAndWidth();
    
    for (auto* propertyComponent : propertyComponents) {
        propertyComponent->setBounds(x, y, width, propertyComponent->getPreferredHeight());
        y = propertyComponent->getBottom() + padding;
    }
}

void PropertiesPanel::SectionComponent::lookAndFeelChanged()
{
    resized();
    repaint();
}

int PropertiesPanel::SectionComponent::getPreferredHeight() const
{
    auto const title = getName();
    auto y = title.isNotEmpty() ? parent.titleHeight : 0;
    
    auto const numComponents = propertyComponents.size();
    
    if (numComponents > 0) {
        for (auto const* propertyComponent : propertyComponents)
            y += propertyComponent->getPreferredHeight();
        
        y += (numComponents - 1) * padding;
    }
    
    return y + (title.isNotEmpty() ? 16 : 0);
}

void PropertiesPanel::SectionComponent::mouseUp(MouseEvent const& e)
{
    if (e.getMouseDownX() < parent.titleHeight
        && e.x < parent.titleHeight
        && e.getNumberOfClicks() != 2)
        mouseDoubleClick(e);
}

void PropertiesPanel::PropertyHolderComponent::updateLayout(int const width, int const viewHeight)
{
    auto y = 4;
    
    for (auto* section : sections) {
        section->setBounds(0, y, width, section->getPreferredHeight());
        y = section->getBottom();
    }
    
    setSize(width, std::max(viewHeight, y));
    repaint();
}

void PropertiesPanel::PropertyHolderComponent::insertSection(int const indexToInsertAt, SectionComponent* newSection)
{
    sections.insert(indexToInsertAt, newSection);
    addAndMakeVisible(newSection, 0);
}

PropertiesPanel::SectionComponent* PropertiesPanel::PropertyHolderComponent::getSectionWithNonEmptyName(int const targetIndex) const noexcept
{
    auto index = 0;
    for (auto* section : sections) {
        if (section->getName().isNotEmpty())
            if (index++ == targetIndex)
                return section;
    }
    
    return nullptr;
}


PropertiesPanel::ComboComponent::ComboComponent(String const& propertyName, Value& value, StringArray const& options)
: PropertiesPanelProperty(propertyName)
, items(options)
{
    comboBox.addItemList(options, 1);
    comboBox.getProperties().set("Style", "Inspector");
    comboBox.getSelectedIdAsValue().referTo(value);
    
    addAndMakeVisible(comboBox);
}

PropertiesPanel::ComboComponent::ComboComponent(String const& propertyName, StringArray const& options)
: PropertiesPanelProperty(propertyName)
, items(options)
{
    comboBox.addItemList(options, 1);
    comboBox.getProperties().set("Style", "Inspector");
    addAndMakeVisible(comboBox);
}

void PropertiesPanel::ComboComponent::resized()
{
    comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
}

PropertiesPanelProperty* PropertiesPanel::ComboComponent::createCopy()
{
    return new ComboComponent(getName(), comboBox.getSelectedIdAsValue(), items);
}


struct FontEntry final : public PopupMenu::CustomComponent {
    String fontName;
    explicit FontEntry(String name)
        : fontName(std::move(name))
    {
    }

    void paint(Graphics& g) override
    {
        auto const font = Font(fontName, 15, Font::plain);
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

PropertiesPanel::FontComponent::FontComponent(String const& propertyName, Value& value, File const& extraFontsDir)
: PropertiesPanelProperty(propertyName)
{
    StringArray extraFontOptions;
    
    if (extraFontsDir.isDirectory() && !extraFontsDir.isRoot()) {
        auto const patchFonts = Fonts::getFontsInFolder(extraFontsDir);
        for (int n = 0; n < patchFonts.size(); n++) {
            extraFontOptions.addIfNotAlreadyThere(patchFonts[n].getFileNameWithoutExtension());
        }
    }
#if JUCE_WINDOWS
    extraFontOptions.addIfNotAlreadyThere("Inter Regular");
#else
    extraFontOptions.addIfNotAlreadyThere("Inter");
#endif
    
    auto const offset = extraFontOptions.size();
    extraFontOptions.addArray(options);
    
    for (int n = 0; n < extraFontOptions.size(); n++) {
        if (n == offset)
            comboBox.getRootMenu()->addSeparator();
        
        comboBox.getRootMenu()->addCustomItem(n + 1, std::make_unique<FontEntry>(extraFontOptions[n]), nullptr, extraFontOptions[n]);
    }
    
    comboBox.setText(value.toString());
    comboBox.getProperties().set("Style", "Inspector");
    fontValue.referTo(value);
    
    comboBox.onChange = [this, extraFontOptions, propertyName] {
        auto fontName = extraFontOptions[comboBox.getSelectedItemIndex()];
        
        if (fontName.isEmpty()) {
            isFontMissing = true;
            fontName = fontValue.toString();
            PropertiesPanelProperty::setName(propertyName + " (missing)");
        } else {
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

PropertiesPanelProperty* PropertiesPanel::FontComponent::createCopy()
{
    return new FontComponent(getName(), fontValue);
}

void PropertiesPanel::FontComponent::lookAndFeelChanged()
{
    comboBox.setColour(ComboBox::textColourId, isFontMissing ? Colours::red : findColour(PlugDataColour::panelTextColourId));
}

void PropertiesPanel::FontComponent::setFont(String const& fontName)
{
    comboBox.setText(fontName);
}

void PropertiesPanel::FontComponent::resized()
{
    comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
}

PropertiesPanel::BoolComponent::BoolComponent(String const& propertyName, Value& value, StringArray options)
: PropertiesPanelProperty(propertyName)
, textOptions(std::move(options))
, toggleStateValue(value)
{
    init();
}

// Also allow creating it without passing in a Value, makes it easier to derive from this class for custom bool components
PropertiesPanel::BoolComponent::BoolComponent(String const& propertyName, StringArray options)
: PropertiesPanelProperty(propertyName)
, textOptions(std::move(options))
{
    init();
}

// Allow creation without an attached juce::Value, but with an initial value
// We need this constructor sometimes to prevent feedback caused by the initial value being set after the listener is attached
PropertiesPanel::BoolComponent::BoolComponent(String const& propertyName, bool const initialValue, StringArray options)
: PropertiesPanelProperty(propertyName)
, textOptions(std::move(options))
{
    toggleStateValue = initialValue;
    init();
}

PropertiesPanel::BoolComponent::~BoolComponent()
{
    toggleStateValue.removeListener(this);
}

void PropertiesPanel::BoolComponent::init()
{
    toggleStateValue.addListener(this);
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
    lookAndFeelChanged();
}



void PropertiesPanel::BoolComponent::lookAndFeelChanged()
{
    repaint();
}

PropertiesPanelProperty* PropertiesPanel::BoolComponent::createCopy()
{
    return new BoolComponent(getName(), toggleStateValue, textOptions);
}

bool PropertiesPanel::BoolComponent::hitTest(int const x, int const y)
{
    if (!isEnabled())
        return false;
    
    auto const bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
    return bounds.contains(x, y);
}

void PropertiesPanel::BoolComponent::paint(Graphics& g)
{
    bool const isDown = getValue<bool>(toggleStateValue);
    bool const isOver = isMouseOver();
    
    auto const bounds = getLocalBounds().toFloat().removeFromRight(getWidth() / (2.0f - hideLabel));
    auto const buttonBounds = bounds.reduced(4);
    
    if (isDown || isOver) {
        // Add some alpha to make it look good on any background...
        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId).contrasting(isOver ? 0.125f : 0.2f).withAlpha(0.25f));
        g.fillRoundedRectangle(buttonBounds, Corners::defaultCornerRadius);
    }
    auto textColour = findColour(PlugDataColour::panelTextColourId);
    
    if (!isEnabled()) {
        textColour = findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f);
    }
    Fonts::drawText(g, textOptions[isDown], bounds, textColour, 14.5f, Justification::centred);
    
    // Paint label
    PropertiesPanelProperty::paint(g);
}

void PropertiesPanel::BoolComponent::mouseEnter(MouseEvent const& e)
{
    repaint();
}

void PropertiesPanel::BoolComponent::mouseExit(MouseEvent const& e)
{
    repaint();
}

void PropertiesPanel::BoolComponent::mouseUp(MouseEvent const& e)
{
    toggleStateValue.setValue(!getValue<bool>(toggleStateValue));
    repaint();
}

void PropertiesPanel::BoolComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(toggleStateValue))
        repaint();
}

PropertiesPanel::InspectorColourComponent::InspectorColourComponent(String const& propertyName, Value& value)
: PropertiesPanelProperty(propertyName)
{
    
    currentColour.referTo(value);
    setWantsKeyboardFocus(true);
    
    currentColour.addListener(this);
    
    addAndMakeVisible(hexValueEditor);
    hexValueEditor.setJustificationType(Justification::centred);
    hexValueEditor.setInterceptsMouseClicks(false, true);
    hexValueEditor.setFont(Fonts::getCurrentFont().withHeight(13.5f));
    
    hexValueEditor.onEditorShow = [this] {
        auto* editor = hexValueEditor.getCurrentTextEditor();
        editor->setBorder(BorderSize<int>(0, 0, 4, 1));
        editor->setJustification(Justification::centred);
        editor->setInputRestrictions(7, "#0123456789ABCDEFabcdef");
        editor->applyColourToAllText(Colour::fromString(currentColour.toString()).contrasting(0.95f));
    };
    
    hexValueEditor.onEditorHide = [this] {
        colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
        currentColour.setValue(colour);
    };
    
    hexValueEditor.onTextChange = [this] {
        colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
    };
    
    updateHexValue();
    
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
    
    repaint();
}

PropertiesPanel::InspectorColourComponent::~InspectorColourComponent()
{
    currentColour.removeListener(this);
}

PropertiesPanelProperty* PropertiesPanel::InspectorColourComponent::createCopy()
{
    return new InspectorColourComponent(getName(), currentColour);
}

void PropertiesPanel::InspectorColourComponent::updateHexValue()
{
    hexValueEditor.setColour(Label::textColourId, Colour::fromString(currentColour.toString()).contrasting(0.95f));
    hexValueEditor.setText(String("#") + currentColour.toString().substring(2).toUpperCase(), dontSendNotification);
}

void PropertiesPanel::InspectorColourComponent::resized()
{
    auto const bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
    if (isMouseOver) {
        hexValueEditor.setBounds(bounds.reduced(1).withTrimmedRight(24));
    } else {
        hexValueEditor.setBounds(bounds.reduced(1));
    }
}

void PropertiesPanel::InspectorColourComponent::paint(Graphics& g)
{
    auto const colour = Colour::fromString(currentColour.toString());
    auto const hoverColour = isMouseOver ? colour.brighter(0.4f) : colour;
    
    auto swatchBounds = getLocalBounds().removeFromRight(getWidth() / 2).toFloat().reduced(4.5f);
    g.setColour(hoverColour);
    g.fillRoundedRectangle(swatchBounds, Corners::defaultCornerRadius);
    g.setColour(colour.darker(0.15f));
    g.drawRoundedRectangle(swatchBounds, Corners::defaultCornerRadius, 0.8f);
    
    if (isMouseOver) {
        g.setColour(hoverColour.contrasting(0.85f));
        g.setFont(Fonts::getIconFont().withHeight(11.5f));
        g.drawText(Icons::Eyedropper, swatchBounds.removeFromRight(24), Justification::centred);
        
        g.setColour(colour.darker(0.15f));
        g.drawLine(getWidth() - 28, 4, getWidth() - 28, getHeight() - 4);
    }
    
    PropertiesPanelProperty::paint(g);
}

void PropertiesPanel::InspectorColourComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(currentColour)) {
        updateHexValue();
        repaint();
    }
}

void PropertiesPanel::InspectorColourComponent::mouseDown(MouseEvent const& e)
{
    if (hexValueEditor.isBeingEdited() && e.getNumberOfClicks() > 1)
        return;
    
    if (e.x > getWidth() - 28) {
        auto const pickerBounds = getScreenBounds().withTrimmedLeft(getWidth() / 2).expanded(5);
        
        ColourPicker::getInstance().show(findParentComponentOfClass<PluginEditor>(), getTopLevelComponent(), false, Colour::fromString(currentColour.toString()), pickerBounds, [_this = SafePointer(this)](Colour const c) {
            if (!_this)
                return;
            
            _this->currentColour = c.toString();
            _this->repaint();
        });
    } else {
        hexValueEditor.showEditor();
    }
}

void PropertiesPanel::InspectorColourComponent::mouseEnter(MouseEvent const& e)
{
    isMouseOver = true;
    resized();
    repaint();
}

void PropertiesPanel::InspectorColourComponent::mouseExit(MouseEvent const& e)
{
    isMouseOver = false;
    resized();
    repaint();
}

class SwatchComponent final : public Component {
    
public:
    explicit SwatchComponent(Value const& colour)
    {
        colourValue.referTo(colour);
    }
    
    void paint(Graphics& g)
    {
        auto const colour = Colour::fromString(colourValue.toString());
        
        g.setColour(isMouseOver() ? colour.brighter(0.4f) : colour);
        g.fillEllipse(getLocalBounds().reduced(1).toFloat());
        g.setColour(colour.darker(0.2f));
        g.drawEllipse(getLocalBounds().reduced(1).toFloat(), 0.8f);
    }
    
    void mouseEnter(MouseEvent const& e)
    {
        repaint();
    }
    
    void mouseExit(MouseEvent const& e)
    {
        repaint();
    }
    
    void mouseDown(MouseEvent const& e)
    {
        auto const pickerBounds = getScreenBounds().expanded(5);
        ColourPicker::getInstance().show(findParentComponentOfClass<PluginEditor>(), getTopLevelComponent(), false, Colour::fromString(colourValue.toString()), pickerBounds, [_this = SafePointer(this)](Colour const c) {
            if (!_this)
                return;
            
            _this->colourValue = c.toString();
            _this->repaint();
        });
    }
    
    Value colourValue;
};



PropertiesPanel::ColourComponent::ColourComponent(String const& propertyName, Value& value)
: PropertiesPanelProperty(propertyName)
, swatchComponent(std::make_unique<SwatchComponent>(value))
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
    
    hexValueEditor.onReturnKey = [this] {
        grabKeyboardFocus();
    };
    
    hexValueEditor.onTextChange = [this] {
        colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
    };
    
    hexValueEditor.onFocusLost = [this] {
        currentColour.setValue(colour);
    };
    
    addAndMakeVisible(*swatchComponent);
    updateHexValue();
    
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());
    
    repaint();
}

PropertiesPanel::ColourComponent::~ColourComponent()
{
    currentColour.removeListener(this);
}

void PropertiesPanel::ColourComponent::lookAndFeelChanged()
{
    // TextEditor is special, setColour() will only change newly typed text colour
    hexValueEditor.applyColourToAllText(findColour(PlugDataColour::panelTextColourId));
}

PropertiesPanelProperty* PropertiesPanel::ColourComponent::createCopy()
{
    return new ColourComponent(getName(), currentColour);
}

void PropertiesPanel::ColourComponent::updateHexValue()
{
    hexValueEditor.setText(String("#") + currentColour.toString().substring(2).toUpperCase());
}

void PropertiesPanel::ColourComponent::resized()
{
    auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
    auto const colourSwatchBounds = bounds.removeFromLeft(getHeight()).reduced(4).translated(12, 0);
    
    swatchComponent->setBounds(colourSwatchBounds);
    hexValueEditor.setBounds(bounds.translated(0, -3));
}

void PropertiesPanel::ColourComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(currentColour)) {
        updateHexValue();
        repaint();
    }
}

PropertiesPanel::RangeComponent::RangeComponent(String const& propertyName, Value& value, bool const integerMode)
: PropertiesPanelProperty(propertyName), property(value), minLabel(integerMode), maxLabel(integerMode)
{
    property.addListener(this);
    
    min = value.getValue().getArray()->getReference(0);
    max = value.getValue().getArray()->getReference(1);
    
    addAndMakeVisible(minLabel);
    minLabel.setEditableOnClick(true);
    minLabel.addMouseListener(this, true);
    minLabel.setText(String(min), dontSendNotification);
    minLabel.setPrecision(3);
    
    addAndMakeVisible(maxLabel);
    maxLabel.setEditableOnClick(true);
    maxLabel.addMouseListener(this, true);
    maxLabel.setText(String(max), dontSendNotification);
    maxLabel.setPrecision(3);
    
    auto setMinimum = [this](float const value) {
        min = value;
        VarArray const arr = { min, max };
        // maxLabel.setMinimum(min + 1e-5f);
        property = var(arr);
    };
    
    auto setMaximum = [this](float const value) {
        max = value;
        VarArray const arr = { min, max };
        // minLabel.setMaximum(max - 1e-5f);
        property = var(arr);
    };
    
    minLabel.onValueChange = setMinimum;
    minLabel.onReturnKey = setMinimum;
    maxLabel.onValueChange = setMaximum;
    maxLabel.onReturnKey = setMaximum;
}

PropertiesPanel::RangeComponent::~RangeComponent()
{
    property.removeListener(this);
}

PropertiesPanelProperty* PropertiesPanel::RangeComponent::createCopy()
{
    return new RangeComponent(getName(), property, false);
}

DraggableNumber& PropertiesPanel::RangeComponent::getMinimumComponent()
{
    return minLabel;
}

DraggableNumber& PropertiesPanel::RangeComponent::getMaximumComponent()
{
    return maxLabel;
}

void PropertiesPanel::RangeComponent::setIntegerMode(bool const integerMode)
{
    minLabel.setDragMode(integerMode ? DraggableNumber::Integer : DraggableNumber::Regular);
    maxLabel.setDragMode(integerMode ? DraggableNumber::Integer : DraggableNumber::Regular);
}

void PropertiesPanel::RangeComponent::resized()
{
    auto bounds = getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel));
    maxLabel.setBounds(bounds.removeFromRight(bounds.getWidth() / (2 - hideLabel)));
    minLabel.setBounds(bounds);
}

void PropertiesPanel::RangeComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(property)) {
        min = v.getValue().getArray()->getReference(0);
        max = v.getValue().getArray()->getReference(1);
        if(minLabel.getValue() != min)
        {
            minLabel.setText(String(min), dontSendNotification);
        }
        if(maxLabel.getValue() != max)
        {
            maxLabel.setText(String(max), dontSendNotification);
        }
    }
}

template class PropertiesPanel::EditableComponent<String>;
template class PropertiesPanel::EditableComponent<int>;
template class PropertiesPanel::EditableComponent<float>;

PropertiesPanel::FilePathComponent::FilePathComponent(String const& propertyName, Value& value)
: PropertiesPanelProperty(propertyName)
, property(value)
{
    label.setEditable(true, false);
    label.getTextValue().referTo(property);
    label.addMouseListener(this, true);
    label.setFont(Font(14));
    
    addAndMakeVisible(label);
    addAndMakeVisible(browseButton);
    
    browseButton.onClick = [this] {
        Dialogs::showSaveDialog([this](URL const& url) {
            auto const result = url.getLocalFile();
            if (result.getParentDirectory().exists()) {
                label.setText(result.getFullPathName(), sendNotification);
            }
        },
                                "", "", getTopLevelComponent());
    };
}

PropertiesPanelProperty* PropertiesPanel::FilePathComponent::createCopy()
{
    return new FilePathComponent(getName(), property);
}

void PropertiesPanel::FilePathComponent::paint(Graphics& g)
{
    PropertiesPanelProperty::paint(g);
    
    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.fillRect(getLocalBounds().removeFromRight(getHeight()));
}

void PropertiesPanel::FilePathComponent::resized()
{
    auto labelBounds = getLocalBounds().removeFromRight(getWidth() / 2);
    label.setBounds(labelBounds);
    browseButton.setBounds(labelBounds.removeFromRight(getHeight()));
}

PropertiesPanel::DirectoryPathComponent::DirectoryPathComponent(String const& propertyName, Value& value)
: PropertiesPanelProperty(propertyName)
, property(value)
{
    setPath(value.toString());
    addAndMakeVisible(browseButton);
    property.addListener(this);
    
    browseButton.onClick = [this] {
        Dialogs::showOpenDialog([this](URL const& url) {
            auto const result = url.getLocalFile();
            if (result.exists()) {
                setPath(result.getFullPathName());
            }
        },
                                false, true, "", "", getTopLevelComponent());
    };
}

void PropertiesPanel::DirectoryPathComponent::valueChanged(Value& v)
{
    label = v.toString();
    repaint();
}

void PropertiesPanel::DirectoryPathComponent::setPath(String path)
{
    property = path;
    if(path.length() > 46)
    {
        path = "..." + path.substring(path.length() - 46, path.length()).fromFirstOccurrenceOf("/", true, false);
    }
    label = path;
    repaint();
}

PropertiesPanelProperty* PropertiesPanel::DirectoryPathComponent::createCopy()
{
    return new DirectoryPathComponent(getName(), property);
}

void PropertiesPanel::DirectoryPathComponent::paint(Graphics& g)
{
    PropertiesPanelProperty::paint(g);
    
    g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.8f));
    g.setFont(Fonts::getDefaultFont().withHeight(14));
    g.drawText(label, 90, 2, getWidth() - 120, getHeight() - 4, Justification::centredLeft);
}

void PropertiesPanel::DirectoryPathComponent::resized()
{
    browseButton.setBounds(getLocalBounds().removeFromRight(getHeight()));
}

PropertiesPanel::ActionComponent::ActionComponent(std::function<void()> callback, String iconToShow, String const& textToShow, bool const roundOnTop, bool const roundOnBottom)
    : PropertiesPanelProperty(textToShow)
    , roundTop(roundOnTop)
    , roundBottom(roundOnBottom)
    , onClick(std::move(callback))
    , icon(std::move(iconToShow))
{
    setHideLabel(true);
}

PropertiesPanelProperty* PropertiesPanel::ActionComponent::createCopy()
{
    return new ActionComponent(onClick, icon, getName(), roundTop, roundBottom);
}

void PropertiesPanel::ActionComponent::paint(Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto textBounds = bounds;
    auto const iconBounds = textBounds.removeFromLeft(textBounds.getHeight());
    
    auto const colour = findColour(PlugDataColour::panelTextColourId);
    if (mouseIsOver) {
        g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
        
        Path p;
        p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, roundBottom, roundBottom);
        g.fillPath(p);
    }
    
    Fonts::drawIcon(g, icon, iconBounds, colour, 12);
    Fonts::drawText(g, getName(), textBounds, colour, 15);
}

void PropertiesPanel::ActionComponent::mouseEnter(MouseEvent const& e)
{
    mouseIsOver = true;
    repaint();
}

void PropertiesPanel::ActionComponent::mouseExit(MouseEvent const& e)
{
    mouseIsOver = false;
    repaint();
}

void PropertiesPanel::ActionComponent::mouseUp(MouseEvent const& e)
{
    onClick();
}

PropertiesPanel::PropertiesPanel()
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
PropertiesPanel::~PropertiesPanel()
{
    viewport.removeMouseListener(this);
    clear();
}

// Deletes all property components from the panel
void PropertiesPanel::clear() const
{
    if (!isEmpty()) {
        propertyHolderComponent->sections.clear();
        updatePropHolderLayout();
    }
}

// Adds a set of properties to the panel
void PropertiesPanel::addSection(String const& sectionTitle, PropertiesArray const& newProperties, int const indexToInsertAt, int const extraPaddingBetweenComponents )
{
    if (isEmpty())
        repaint();
    
    propertyHolderComponent->insertSection(indexToInsertAt, new SectionComponent(*this, sectionTitle, newProperties, extraPaddingBetweenComponents));
    updatePropHolderLayout();
}

// Returns true if the panel contains no properties
bool PropertiesPanel::isEmpty() const
{
    return propertyHolderComponent->sections.size() == 0;
}

void PropertiesPanel::setContentWidth(int const newContentWidth)
{
    contentWidth = newContentWidth;
    resized();
    repaint();
}

Component* PropertiesPanel::getSectionByName(String const& name) const noexcept
{
    if (propertyHolderComponent) {
        for (auto* section : propertyHolderComponent->sections) {
            if (section->getName() == name)
                return section;
        }
    }
    
    return nullptr;
}

std::pair<int, int> PropertiesPanel::getContentXAndWidth()
{
    auto marginWidth = (getWidth() - contentWidth) / 2;
    return { marginWidth, contentWidth };
}

// Returns a list of all the names of sections in the panel
StringArray PropertiesPanel::getSectionNames() const
{
    StringArray s;
    
    for (auto const* section : propertyHolderComponent->sections) {
        if (section->getName().isNotEmpty())
            s.add(section->getName());
    }
    
    return s;
}

void PropertiesPanel::paint(Graphics& g)
{
    if (isEmpty()) {
        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f));
        g.setFont(14.0f);
        g.drawText(messageWhenEmpty, getLocalBounds().withHeight(30),
                   Justification::centred, true);
    }
}

void PropertiesPanel::setTitleAlignment(TitleAlignment const newTitleAlignment)
{
    titleAlignment = newTitleAlignment;
}

void PropertiesPanel::setPanelColour(int const newPanelColourId)
{
    panelColour = newPanelColourId;
}

void PropertiesPanel::setSeparatorColour(int const newSeparatorColourId)
{
    separatorColour = newSeparatorColourId;
}

void PropertiesPanel::setDrawShadowAndOutline(bool const shouldDrawShadowAndOutline)
{
    drawShadowAndOutline = shouldDrawShadowAndOutline;
}

void PropertiesPanel::setTitleHeight(int const newTitleHeight)
{
    titleHeight = newTitleHeight;
}

// Sets extra section header text
// All lines passed in here will be divided equally across the non-label area of the property
// Useful for naming rows when using a MultiPropertyComponent
void PropertiesPanel::setExtraHeaderNames(int const sectionIndex, StringArray headerNames) const
{
    if (auto* s = propertyHolderComponent->getSectionWithNonEmptyName(sectionIndex)) {
        s->setExtraHeaderNames(std::move(headerNames));
    }
}

void PropertiesPanel::resized()
{
    viewport.setBounds(getLocalBounds().withTrimmedTop(1));
    updatePropHolderLayout();
}

void PropertiesPanel::updatePropHolderLayout() const
{
    auto const maxWidth = viewport.getMaximumVisibleWidth();
    auto const maxHeight = viewport.getMaximumVisibleHeight();
    propertyHolderComponent->updateLayout(maxWidth, maxHeight);
    
    auto const newMaxWidth = viewport.getMaximumVisibleWidth();
    if (maxWidth != newMaxWidth) {
        // need to do this twice because of vertical scrollbar changing the size, etc.
        propertyHolderComponent->updateLayout(newMaxWidth, maxHeight);
    }
}


PropertiesSearchPanel::PropertiesSearchPanel(SmallArray<PropertiesPanel*> const& searchedPanels)
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
    input.onTextChange = [this] {
        updateResults();
    };
}

void PropertiesSearchPanel::resized()
{
    input.setBounds(getLocalBounds().removeFromTop(42).reduced(48, 6).translated(-2, -1));
    resultsPanel.setBounds(getLocalBounds().withTrimmedTop(40));
}

void PropertiesSearchPanel::updateResults()
{
    resultsPanel.clear();
    
    auto const query = input.getText().toLowerCase();
    if (query.isEmpty())
        return;
    
    for (auto const* propertiesPanel : panelsToSearch) {
        for (auto* section : propertiesPanel->propertyHolderComponent->sections) {
            PropertiesArray properties;
            auto sectionTitle = section->getName();
            
            for (auto* property : section->propertyComponents) {
                if (property->getName().toLowerCase().contains(query) || sectionTitle.toLowerCase().contains(query)) {
                    if (auto* propertyCopy = property->createCopy())
                        properties.add(propertyCopy);
                }
            }
            if (!properties.isEmpty()) {
                resultsPanel.addSection(sectionTitle, properties);
            }
        }
    }
}

void PropertiesSearchPanel::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);
    
    auto const titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();
    
    Path p;
    p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
    
    g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
    g.fillPath(p);
    
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawHorizontalLine(40, 0.0f, getWidth());
}

void PropertiesSearchPanel::startSearching()
{
    setVisible(true);
    input.grabKeyboardFocus();
}

void PropertiesSearchPanel::stopSearching()
{
    setVisible(false);
    input.setText("");
}
