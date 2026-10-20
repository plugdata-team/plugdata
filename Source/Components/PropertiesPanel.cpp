/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

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
}

PropertiesPanel::SectionComponent::~SectionComponent()
{
    propertyComponents.clear();
}

void PropertiesPanel::SectionComponent::paint(Graphics& g)
{
    auto const& colours = getThemeColours(*this);

    auto [x, width] = parent.getContentXAndWidth();

    auto titleX = x;
    if (parent.titleAlignment == AlignWithPropertyName) {
        titleX += 11;
    }

    auto const title = getName();
    auto const titleHeight = title.isEmpty() ? 0 : parent.titleHeight;

    if (titleHeight != 0) {
        Fonts::drawStyledText(g, title, titleX, 0, width - 4, titleHeight, colours.panelTextColour, Semibold, 14.5f);
    }

    auto const propertyBounds = Rectangle<int>(x, titleHeight + 8.0f, width, getHeight() - (titleHeight + 16.0f));

    if (parent.drawShadowAndOutline) {
        StackShadow::drawShadowForRect(g, propertyBounds, Corners::largeCornerRadius, StackShadow::Card);
    }

    g.setColour(findColour(parent.panelColour));
    g.fillRoundedRectangle(propertyBounds.toFloat(), Corners::largeCornerRadius);

    if (parent.drawShadowAndOutline) {
        g.setColour(findColour(parent.separatorColour));
        g.drawRoundedRectangle(propertyBounds.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    if (!propertyComponents.isEmpty() && !extraHeaderNames.isEmpty()) {
        auto propertyBounds = Rectangle<int>(x + width / 2, 0, width / 2, parent.titleHeight);
        auto const extraHeaderWidth = propertyBounds.getWidth() / static_cast<float>(extraHeaderNames.size());

        for (auto& extraHeader : extraHeaderNames) {
            auto const colour = colours.panelTextColour.withAlpha(0.75f);
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

    g.setColour(getThemeColours(*this).toolbarOutlineColour);

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

PropertiesPanel::ComboComponent::ComboComponent(String const& propertyName, Value const& value, StringArray const& options)
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

class FontEntry final : public PopupMenu::CustomComponent {
    String fontName;

public:
    explicit FontEntry(String name)
        : fontName(std::move(name))
    {
    }

    void paint(Graphics& g) override
    {
        auto const font = Font(FontOptions(fontName, 15, Font::plain));
        g.setFont(font);
        g.setColour(getThemeColours(*this).panelTextColour);
        g.drawText(fontName, getLocalBounds().reduced(2), Justification::centredLeft);
    }

    void getIdealSize(int& idealWidth, int& idealHeight) override
    {
        idealWidth = 150;
        idealHeight = 23;
    }
};

PropertiesPanel::FontComponent::FontComponent(String const& propertyName, Value const& value, File const& extraFontsDir)
    : PropertiesPanelProperty(propertyName)
{
    StringArray extraFontOptions;

    if (extraFontsDir.isDirectory() && !extraFontsDir.isRoot()) {
        auto const patchFonts = Fonts::getFontsInFolder(extraFontsDir);
        for (int n = 0; n < patchFonts.size(); n++) {
            extraFontOptions.addIfNotAlreadyThere(patchFonts[n].getFileNameWithoutExtension());
        }
    }
    extraFontOptions.addIfNotAlreadyThere("Inter");

    auto const offset = extraFontOptions.size();
    extraFontOptions.addArray(options);

    for (int n = 0; n < extraFontOptions.size(); n++) {
        if (n == offset)
            comboBox.getRootMenu()->addSeparator();

        comboBox.getRootMenu()->addCustomItem(n + 1, std::make_unique<FontEntry>(extraFontOptions[n]), nullptr, extraFontOptions[n]);
    }

    comboBox.setText(value.toString(), dontSendNotification);
    comboBox.getProperties().set("Style", "Inspector");
    fontValue.referTo(value);
    fontValue.addListener(this);

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

    setLookAndFeel(nullptr);

    addAndMakeVisible(comboBox);

    lookAndFeelChanged();
}

PropertiesPanel::FontComponent::~FontComponent()
{
    fontValue.removeListener(this);
}

void PropertiesPanel::FontComponent::valueChanged(Value& value)
{
    comboBox.setText(value.toString(), dontSendNotification);
}

PropertiesPanelProperty* PropertiesPanel::FontComponent::createCopy()
{
    return new FontComponent(getName(), fontValue);
}

void PropertiesPanel::FontComponent::lookAndFeelChanged()
{
    comboBox.setColour(ComboBox::textColourId, isFontMissing ? Colours::red : getThemeColours(*this).panelTextColour);
}

void PropertiesPanel::FontComponent::setFont(String const& fontName)
{
    comboBox.setText(fontName);
}

void PropertiesPanel::FontComponent::resized()
{
    comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / (2 - hideLabel)));
}

PropertiesPanel::BoolComponent::BoolComponent(String const& propertyName, Value const& value, StringArray options)
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
    if (textOptions.size() == 2) {
        auto const off = textOptions[0].toLowerCase();
        auto const on = textOptions[1].toLowerCase();
        isOnOffPair = (off == "no" && on == "yes") || (off == "off" && on == "on");
    }

    toggleStateValue.addListener(this);
    setLookAndFeel(nullptr);
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

// Shared look of the segmented bool control, also used by the colour field
static void drawControlTrack(Graphics& g, PlugDataColours const& colours, Rectangle<float> const bounds)
{
    g.setColour(colours.panelTextColour.withAlpha(0.07f));
    g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius);
}

static void drawControlThumb(Graphics& g, PlugDataColours const& colours, Rectangle<float> const bounds, Colour const fill, float const outlineAlpha = 0.12f)
{
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius - 2.0f);
    g.setColour(colours.panelTextColour.withAlpha(outlineAlpha));
    g.drawRoundedRectangle(bounds, Corners::defaultCornerRadius - 2.0f, 0.5f);
}

static void drawControlHover(Graphics& g, PlugDataColours const& colours, Rectangle<float> const bounds)
{
    g.setColour(colours.panelTextColour.withAlpha(0.05f));
    g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius - 2.0f);
}

// Drawn as a two-segment control, with the selected option on a raised thumb
void PropertiesPanel::BoolComponent::paint(Graphics& g)
{
    auto const& colours = getThemeColours(*this);
    auto const selectedIndex = static_cast<int>(getValue<bool>(toggleStateValue));
    bool const isOver = isMouseOver();

    if (!isEnabled())
        g.beginTransparencyLayer(0.5f);

    auto const track = getControlBounds();
    drawControlTrack(g, colours, track);

    auto segments = track.reduced(2.0f);
    std::array<Rectangle<float>, 2> const segmentBounds = { segments.removeFromLeft(segments.getWidth() / 2.0f), segments };
    auto const hoveredIndex = isOver ? static_cast<int>(getMouseXYRelative().x >= track.getCentreX()) : -1;

    for (int i = 0; i < 2; i++) {
        auto const& segment = segmentBounds[i];
        bool const isSelected = i == selectedIndex;

        // An enabled on/off property is drawn in the selection colour, like active buttons in the toolbar
        bool const isActive = isSelected && isOnOffPair && i == 1;
        auto const accent = colours.objectSelectedOutlineColour;

        bool const isLightTheme = colours.panelTextColour.getPerceivedBrightness() < 0.5f;
        auto controlThumbColour = isLightTheme ? Colours::white : colours.panelTextColour.withAlpha(0.2f);

        if (isActive)
            drawControlThumb(g, colours, segment, accent);
        else if (isSelected)
            drawControlThumb(g, colours, segment, controlThumbColour);
        else if (i == hoveredIndex)
            drawControlHover(g, colours, segment);

        auto textColour = colours.panelTextColour.withAlpha(isSelected ? 1.0f : 0.55f);
        if (isActive)
            textColour = accent.getPerceivedBrightness() > 0.7f ? Colours::black : Colours::white;

        Fonts::drawFittedText(g, textOptions[i], segment.toNearestInt().reduced(2, 0), textColour, 1, 0.8f, 13.0f, Justification::centred);
    }

    if (!isEnabled())
        g.endTransparencyLayer();

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

void PropertiesPanel::BoolComponent::mouseMove(MouseEvent const& e)
{
    // The hovered segment follows the mouse
    repaint();
}

void PropertiesPanel::BoolComponent::mouseUp(MouseEvent const& e)
{
    if (!isRealClickEvent(e))
        return;

    // Select the clicked segment
    bool const currentState = getValue<bool>(toggleStateValue);
    bool const newState = e.position.x >= getControlBounds().getCentreX();

    if (newState != currentState)
        toggleStateValue.setValue(newState);

    repaint();
}

void PropertiesPanel::BoolComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(toggleStateValue))
        repaint();
}

PropertiesPanel::ColourComponent::ColourComponent(String const& propertyName, Value const& value, bool canHaveAlpha)
    : PropertiesPanelProperty(propertyName)
    , allowAlpha(canHaveAlpha)
{
    currentColour.referTo(value);
    setWantsKeyboardFocus(true);

    currentColour.addListener(this);

    addAndMakeVisible(hexValueEditor);
    hexValueEditor.setJustificationType(Justification::centred);
    hexValueEditor.setInterceptsMouseClicks(false, true);
    hexValueEditor.setFont(Fonts::getTabularNumbersFont().withHeight(13.0f));

    hexValueEditor.onEditorShow = [this] {
        auto* editor = hexValueEditor.getCurrentTextEditor();
        // Centred text also gets shifted by the TextEditor's left indent, so move a pixel of border to the right
        editor->setBorder({ 0, 0, 4, 2 });
        editor->setJustification(Justification::centred);
        editor->setInputRestrictions(allowAlpha ? 9 : 7, "#0123456789ABCDEFabcdef");
    };

    hexValueEditor.onEditorHide = [this] {
        if (allowAlpha) {
            colour = hexValueEditor.getText().substring(1).toLowerCase();
        } else {
            colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
        }
        currentColour.setValue(colourToVar(Colour::fromString(colour.toString())));
    };

    hexValueEditor.onTextChange = [this] {
        if (allowAlpha) {
            colour = hexValueEditor.getText().substring(1).toLowerCase();
        } else {
            colour = String("ff") + hexValueEditor.getText().substring(1).toLowerCase();
        }
    };

    updateHexValue();

    setLookAndFeel(nullptr);
}

PropertiesPanel::ColourComponent::~ColourComponent()
{
    currentColour.removeListener(this);
}

PropertiesPanelProperty* PropertiesPanel::ColourComponent::createCopy()
{
    return new ColourComponent(getName(), currentColour, allowAlpha);
}

void PropertiesPanel::ColourComponent::updateHexValue()
{
    hexValueEditor.setText(String("#") + getValue<Colour>(currentColour).toString().substring(2).toUpperCase(), dontSendNotification);
}

// Square on the left of the field, inset like the thumb of the segmented control
Rectangle<float> PropertiesPanel::ColourComponent::getSwatchBounds() const
{
    auto const field = getControlBounds().reduced(2.0f);
    return field.withWidth(field.getHeight());
}

void PropertiesPanel::ColourComponent::resized()
{
    auto const field = getControlBounds();
    auto const textX = getSwatchBounds().getRight() + 1.0f;
    hexValueEditor.setBounds(field.withLeft(textX).withTrimmedRight(2.0f).toNearestInt().withY(0).withHeight(getHeight()));
}

void PropertiesPanel::ColourComponent::paint(Graphics& g)
{
    auto const& colours = getThemeColours(*this);
    auto const colour = getValue<Colour>(currentColour);
    auto const swatchBounds = getSwatchBounds();

    if (!isEnabled())
        g.beginTransparencyLayer(0.5f);

    drawControlTrack(g, colours, getControlBounds());

    if (allowAlpha && !colour.isOpaque()) {
        Path swatchShape;
        swatchShape.addRoundedRectangle(swatchBounds, Corners::defaultCornerRadius - 2.0f);

        Graphics::ScopedSaveState saveState(g);
        g.reduceClipRegion(swatchShape);
        g.fillCheckerBoard(swatchBounds, 4.5f, 4.5f, Colours::white, Colours::lightgrey);
    }

    // Stronger outline than the segmented thumb, so light swatches stay visible on light themes and dark ones on dark themes
    drawControlThumb(g, colours, swatchBounds, colour, isMouseOverSwatch ? 0.45f : 0.2f);

    if (!isEnabled())
        g.endTransparencyLayer();

    PropertiesPanelProperty::paint(g);
}

void PropertiesPanel::ColourComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(currentColour)) {
        updateHexValue();
        repaint();
    }
}

void PropertiesPanel::ColourComponent::mouseDown(MouseEvent const& e)
{
    if (hexValueEditor.isBeingEdited() && e.getNumberOfClicks() > 1)
        return;

    auto const field = getControlBounds();
    if (!field.contains(e.position))
        return;

    // The swatch opens the picker, the hex code is edited in place
    if (e.position.x < hexValueEditor.getX()) {
        auto const pickerBounds = (field.toNearestInt() + getScreenPosition()).expanded(5);

        ColourPicker::getInstance()->show(findParentComponentOfClass<PluginEditor>(), getTopLevelComponent(), false, getValue<Colour>(currentColour), pickerBounds, [_this = SafePointer(this)](Colour const c) {
            if (!_this)
                return;

            _this->currentColour = colourToVar(c);
            _this->repaint();
        });
    } else {
        hexValueEditor.showEditor();
    }
}

void PropertiesPanel::ColourComponent::mouseMove(MouseEvent const& e)
{
    auto const overSwatch = getControlBounds().contains(e.position) && e.position.x < hexValueEditor.getX();
    if (overSwatch != isMouseOverSwatch) {
        isMouseOverSwatch = overSwatch;
        repaint();
    }
}

void PropertiesPanel::ColourComponent::mouseExit(MouseEvent const& e)
{
    isMouseOverSwatch = false;
    repaint();
}

PropertiesPanel::RangeComponent::RangeComponent(String const& propertyName, Value const& value, bool const integerMode)
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

    for (auto* label : { &minLabel, &maxLabel }) {
        label->onEditorShow = [label] {
            label->getCurrentTextEditor()->setBorder(getInlineEditorBorder());
        };
    }
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
        if (minLabel.getValue() != min) {
            minLabel.setText(String(min), dontSendNotification);
        }
        if (maxLabel.getValue() != max) {
            maxLabel.setText(String(max), dontSendNotification);
        }
    }
}

template class PropertiesPanel::EditableComponent<String>;
template class PropertiesPanel::EditableComponent<int>;
template class PropertiesPanel::EditableComponent<float>;

PropertiesPanel::FilePathComponent::FilePathComponent(String const& propertyName, Value const& value)
    : PropertiesPanelProperty(propertyName)
    , property(value)
{
    label.setEditable(true, false);
    label.getTextValue().referTo(property);
    label.addMouseListener(this, true);
    label.setFont(Font(FontOptions(14)));

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

void PropertiesPanel::FilePathComponent::resized()
{
    auto labelBounds = getLocalBounds().removeFromRight(getWidth() / 2);
    browseButton.setBounds(labelBounds.removeFromRight(getHeight()));
    label.setBounds(labelBounds);
}

PropertiesPanel::DirectoryPathComponent::DirectoryPathComponent(String const& propertyName, Value const& value)
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
    if (path.length() > 46) {
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

    g.setColour(getThemeColours(*this).panelTextColour.withAlpha(0.8f));
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
    auto const& colours = getThemeColours(*this);

    auto const bounds = getLocalBounds();
    auto textBounds = bounds;
    auto const iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

    auto const colour = colours.panelTextColour;
    if (mouseIsOver) {
        g.setColour(colours.panelActiveBackgroundColour);

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
    messageWhenEmpty = "(no inspector selection)";

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
void PropertiesPanel::addSection(String const& sectionTitle, PropertiesArray const& newProperties, int const indexToInsertAt, int const extraPaddingBetweenComponents)
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
        g.setColour(getThemeColours(*this).panelTextColour.withAlpha(0.5f));
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
    auto const& colours = getThemeColours(*this);

    g.setColour(colours.panelBackgroundColour);
    g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

    auto const titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();

    Path p;
    p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

    g.setColour(colours.toolbarBackgroundColour);
    g.fillPath(p);

    g.setColour(colours.toolbarOutlineColour);
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
