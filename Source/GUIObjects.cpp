/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "GUIObjects.h"
#include "Box.h"
#include "Canvas.h"
#include "Connection.h"
#include "Edge.h"
#include "PluginEditor.h"

#include <g_canvas.h>
#include <m_imp.h>
#include <m_pd.h>

GUIComponent::GUIComponent(pd::Gui pdGui, Box* parent)
    : box(parent)
    , processor(parent->cnv->main.pd)
    , gui(pdGui)
    , edited(false)
    , guiLook(box->cnv->main.resources.get())
      
{
    // if(!box->pdObject) return;
    value = gui.getValue();
    min = gui.getMinimum();
    max = gui.getMaximum();
    
    updateLabel();
    
    

    sendSymbol = gui.getSendSymbol();
    receiveSymbol = gui.getReceiveSymbol();

    setWantsKeyboardFocus(true);
    
    setLookAndFeel(&guiLook);
}

GUIComponent::~GUIComponent()
{
    box->removeComponentListener(this);
    setLookAndFeel(nullptr);
}

void GUIComponent::initParameters()
{
    if(gui.getType() == pd::Type::Number) {
        auto color = Colour::fromString(secondaryColour);
        secondaryColour = color.toString();
        setBackground(color);
    }
    
    if (!gui.isIEM())
        return;

    primaryColour = Colour(gui.getForegroundColor()).toString();
    secondaryColour = Colour(gui.getBackgroundColor()).toString();
    if (primaryColour == "ff000000")
        primaryColour = MainLook::highlightColour.toString();
    if (secondaryColour == "fffcfcfc")
        secondaryColour = MainLook::firstBackground.toString();
    setForeground(Colour::fromString(primaryColour));
    setBackground(Colour::fromString(secondaryColour));
    

}


void GUIComponent::setForeground(Colour colour)
{
    getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
    getLookAndFeel().setColour(Slider::thumbColourId, colour);

    // TODO: these functions aren't working correctly yet...

    String colourStr = colour.toString();
    libpd_iemgui_set_foreground_color(gui.getPointer(), colourStr.toRawUTF8());
}

void GUIComponent::setBackground(Colour colour)
{
    getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
    getLookAndFeel().setColour(TextButton::buttonColourId, colour);
    getLookAndFeel().setColour(Slider::backgroundColourId, colour.brighter(0.14f));

    String colourStr = colour.toString();
    libpd_iemgui_set_background_color(gui.getPointer(), colourStr.toRawUTF8());
}

GUIComponent* GUIComponent::createGui(String name, Box* parent)
{
    auto* gui_ptr = dynamic_cast<pd::Gui*>(parent->pdObject.get());

    if (!gui_ptr)
        return nullptr;

    auto& gui = *gui_ptr;

    if (gui.getType() == pd::Type::Bang) {
        return new BangComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Toggle) {
        return new ToggleComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::HorizontalSlider) {
        return new SliderComponent(false, gui, parent);
    }
    if (gui.getType() == pd::Type::VerticalSlider) {
        return new SliderComponent(true, gui, parent);
    }
    if (gui.getType() == pd::Type::HorizontalRadio) {
        return new RadioComponent(false, gui, parent);
    }
    if (gui.getType() == pd::Type::VerticalRadio) {
        return new RadioComponent(true, gui, parent);
    }
    if (gui.getType() == pd::Type::Message) {
        return new MessageComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Number) {
        return new NumboxComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::AtomList) {
        return new ListComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Array) {
        return new ArrayComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::GraphOnParent) {
        return new GraphOnParent(gui, parent);
    }
    if (gui.getType() == pd::Type::Subpatch) {
        return new Subpatch(gui, parent);
    }
    if (gui.getType() == pd::Type::VuMeter) {
        return new VUMeter(gui, parent);
    }
    if (gui.getType() == pd::Type::Panel) {
        return new PanelComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Comment) {
        return new CommentComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::AtomNumber) {
        return new NumboxComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::AtomSymbol) {
        return new MessageComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Mousepad) {
        return new MousePad(gui, parent);
    }
    if (gui.getType() == pd::Type::Mouse) {
        return new MouseComponent(gui, parent);
    }
    if (gui.getType() == pd::Type::Keyboard) {
        return new KeyboardComponent(gui, parent);
    }

    return nullptr;
}

float GUIComponent::getValueOriginal() const noexcept
{
    return value;
}

void GUIComponent::setValueOriginal(float v, bool sendNotification)
{
    ScopedLock lock(*box->cnv->main.pd.getCallbackLock());

    value = (min < max) ? std::max(std::min(v, max), min) : std::max(std::min(v, min), max);
    if (sendNotification)
        gui.setValue(value);
}

float GUIComponent::getValueScaled() const noexcept
{

    return (min < max) ? (value - min) / (max - min) : 1.f - (value - max) / (min - max);
}

void GUIComponent::setValueScaled(float v)
{
    ScopedLock lock(*box->cnv->main.pd.getCallbackLock());

    value = (min < max) ? std::max(std::min(v, 1.f), 0.f) * (max - min) + min
                        : (1.f - std::max(std::min(v, 1.f), 0.f)) * (min - max) + max;
    gui.setValue(value);
}

void GUIComponent::startEdition() noexcept
{
    edited = true;
    processor.enqueueMessages(stringGui, stringMouse, { 1.f });

    ScopedLock lock(*box->cnv->main.pd.getCallbackLock());
    value = gui.getValue();
}

void GUIComponent::stopEdition() noexcept
{
    edited = false;
    processor.enqueueMessages(stringGui, stringMouse, { 0.f });
}

void GUIComponent::updateValue()
{
    if (edited == false) {
        float const v = gui.getValue();
        if (v != value) {
            value = v;
            update();
            // repaint();
        }
    }
}

void GUIComponent::componentMovedOrResized(Component& component, bool moved, bool resized) {
    if(label) {
        Point<int> position = gui.getLabelPosition(component.getBounds());
  
        const int width = 100;
        const int height = 50;
        label->setBounds(position.x, position.y, width, height);
    }
}

void GUIComponent::updateLabel()
{
    pd::Label const lbl = gui.getLabel();
    const String text = String(lbl.getText());
    if (text.isNotEmpty()) {
        label = std::make_unique<Label>();
        if (label == nullptr) {
            return;
        }
        const int width = 100; // ft.getStringWidth(text) + 1;
        const int height = 100; // ft.getHeight();
        const std::array<int, 2> position = lbl.getPosition();
        label->setBounds(position[0], position[1] - height / 2, width, height);
        // label->setFont(ft);
        label->setJustificationType(Justification::centredLeft);
        label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
        label->setMinimumHorizontalScale(1.f);
        label->setText(text, NotificationType::dontSendNotification);
        label->setEditable(false, false);
        label->setInterceptsMouseClicks(false, false);
        label->setColour(Label::textColourId, Colour(static_cast<uint32>(lbl.getColor())));
        box->cnv->addAndMakeVisible(label.get());
        box->addComponentListener(this);
    }
}

pd::Gui GUIComponent::getGUI()
{
    return gui;
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void GUIComponent::closeOpenedSubpatchers() {
    auto* cnv = box->cnv;
    auto& main = box->cnv->main;
    auto* tabbar = &main.getTabbar();
    
    if (!tabbar)
        return;
    
    for (int n = 0; n < tabbar->getNumTabs(); n++) {
        auto* cnv = main.getCanvas(n);
        if (cnv && cnv->patch ==  *getPatch()) {
            tabbar->removeTab(n);
            main.canvases.removeObject(cnv);
        }
    }

    if (tabbar->getNumTabs() > 1) {
        tabbar->getTabbedButtonBar().setVisible(true);
        tabbar->setTabBarDepth(30);
    } else {
        tabbar->getTabbedButtonBar().setVisible(false);
        tabbar->setTabBarDepth(1);
    }
}

// BangComponent

BangComponent::BangComponent(pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    addAndMakeVisible(bangButton);

    bangButton.setTriggeredOnMouseDown(true);

    bangButton.onClick = [this]() {
        startEdition();
        setValueOriginal(1);
        stopEdition();
    };

    initParameters();
    box->restrainer.setSizeLimits(40, 60, 200, 200);
    box->restrainer.checkComponentBounds(box);
}

void BangComponent::update()
{
    if (getValueOriginal() > std::numeric_limits<float>::epsilon()) {
        bangButton.setToggleState(true, dontSendNotification);
        startTimer(bangInterrupt);
    }
}

void BangComponent::resized()
{
    gui.setSize(getWidth(), getHeight());
    bangButton.setBounds(getLocalBounds().reduced(6));
}

// ToggleComponent
ToggleComponent::ToggleComponent(pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    addAndMakeVisible(toggleButton);
    toggleButton.setToggleState(getValueOriginal(), dontSendNotification);
    toggleButton.setConnectedEdges(12);
    
    toggleButton.onClick = [this]() {
        startEdition();
        auto new_value = 1.f - getValueOriginal();
        setValueOriginal(new_value);
        toggleButton.setToggleState(new_value, dontSendNotification);
        stopEdition();
    };

    initParameters();
    
    box->restrainer.setSizeLimits(40, 60, 200, 200);
    box->restrainer.checkComponentBounds(box);
}

void ToggleComponent::resized()
{
    
    gui.setSize(getWidth(), getHeight());
    toggleButton.setBounds(getLocalBounds().reduced(8));
   
}

void ToggleComponent::update()
{
    toggleButton.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
}

// MessageComponent
MessageComponent::MessageComponent(pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    bangButton.setConnectedEdges(12);

    addAndMakeVisible(input);

    if (gui.getType() != pd::Type::AtomSymbol) {
        box->textLabel.addAndMakeVisible(bangButton);
    }
    
    box->cnv->main.addChangeListener(this);
    isLocked = box->cnv->pd->locked;
    
    // message box behaviour
    if(!gui.isAtom()) {
        input.getLookAndFeel().setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        
        input.onTextChange = [this]() {
            
            auto width = input.getFont().getStringWidth(input.getText()) + 25;
            
            
            if(width > box->getWidth()) {
                box->setSize(width, box->getHeight());
            }
            
            gui.setSymbol(input.getText().toStdString());
        };
        
        bangButton.onClick = [this]() {
            startEdition();
            gui.click();
            stopEdition();
        };
    }
    // symbolatom box behaviour
    else {
        input.onReturnKey = [this](){
            startEdition();
            gui.setSymbol(input.getText().toStdString());
            stopEdition();
            input.setText(juce::String(gui.getSymbol()), juce::NotificationType::dontSendNotification);
        };
    }

    input.onFocusLost = [this](){
        
        auto width = input.getFont().getStringWidth(input.getText()) + 25;
        if(width < box->getWidth()) {
            box->setSize(width, box->getHeight());
            box->restrainer.checkComponentBounds(box);
        }
    };
    
    input.setMultiLine(true);
    
    box->restrainer.setSizeLimits(70, 50, 500, 600);
    box->restrainer.checkComponentBounds(box);
    
}

MessageComponent::~MessageComponent()
{
    box->cnv->main.removeChangeListener(this);
}

void MessageComponent::resized()
{
    input.setBounds(0, 0, getWidth(), getHeight());
    bangButton.setBounds(getWidth() - 23, 0, 23, 22);
}

void MessageComponent::update()
{

    input.setText(String(gui.getSymbol()), NotificationType::sendNotification);
}

void MessageComponent::paint(Graphics& g) {
    // Draw message style
    if (!getGUI().isAtom()) {
        auto baseColour = isDown ? Colour(90, 90, 90) : Colour(60, 60, 60);
        
        auto rect = getLocalBounds().toFloat();
        g.setGradientFill(ColourGradient(baseColour, Point<float>(0.0f, 0.0f), baseColour.darker(0.9f), getPosition().toFloat() + Point<float>(0, getHeight()), false));
        
        g.fillRoundedRectangle(rect.withTrimmedBottom(getHeight() - 31), 2.0f);
    }
}


void MessageComponent::changeListenerCallback(ChangeBroadcaster* source)  {
    isLocked = box->cnv->pd->locked;
    
    if(isLocked && !gui.isAtom()) {
        input.setInterceptsMouseClicks(false, false);
        input.setEnabled(false);
    }
    else {
        input.setInterceptsMouseClicks(true, true);
        input.setEnabled(true);
    }
}

void MessageComponent::updateValue()
{
    if (edited == false) {
        std::string const v = gui.getSymbol();

        if (lastMessage != v && !String(v).startsWith("click")) {
            numLines = 1;
            longestLine = 7;

            int currentLineLength = 0;
            for (auto& c : v) {
                if (c == '\n') {
                    numLines++;
                    longestLine = std::max(longestLine, currentLineLength);
                    currentLineLength = 0;
                } else {
                    currentLineLength++;
                }
            }
            if (numLines == 1)
                longestLine = std::max(longestLine, currentLineLength);

            lastMessage = v;

            update();
            // repaint();
        }
    }
}

// NumboxComponent

NumboxComponent::NumboxComponent(pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    input.addMouseListener(this, false);

    input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            startEdition();
            
            if(!gui.isAtom()) {
                editor->setBorder({0, 10, 0, 0});
            }
            
            if(editor != nullptr)
            {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };
    
    input.onEditorHide = [this]() {
        setValueOriginal(input.getText().getFloatValue());
        stopEdition();
    };
    
    if(!gui.isAtom()) {
        input.setBorderSize({1, 15, 1, 1});
    }
    addAndMakeVisible(input);
    
    input.setText(String(getValueOriginal()), dontSendNotification);
    
    input.onTextChange = [this]() {
        /*
        startEdition();
        float value = input.getText().getFloatValue();
        if (min < max) {
            value = std::clamp(value, min, max);
            input.setText(String(value, 3), dontSendNotification);
        }

        setValueOriginal(value); */
    };
    
    initParameters();
    input.setEditable(false, true);
    
    box->restrainer.setSizeLimits(30, 50, 500, 600);
    box->restrainer.checkComponentBounds(box);
}

void NumboxComponent::resized()
{
    input.setBounds(getLocalBounds());
}

void NumboxComponent::update()
{
    float value = getValueOriginal();
    
    
    input.setText(String(value), dontSendNotification);
}

ListComponent::ListComponent(pd::Gui gui, Box* parent)
    : GUIComponent(gui, parent)
{
    static const int border = 1;

    label.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
    label.setMinimumHorizontalScale(1.f);
    label.setJustificationType(Justification::centredLeft);
    label.setBorderSize(BorderSize<int>(border + 2, border, border, border));
    label.setText(String(getValueOriginal()), NotificationType::dontSendNotification);
    label.setEditable(false, false);
    label.setInterceptsMouseClicks(false, false);
    label.setColour(Label::textColourId, Colour(static_cast<uint32>(gui.getForegroundColor())));
    setInterceptsMouseClicks(true, false);
    addAndMakeVisible(label);

    label.onEditorHide = [this]() {
        auto const newValue = label.getText().getFloatValue();
        if (std::abs(newValue - getValueOriginal()) > std::numeric_limits<float>::epsilon()) {
            startEdition();
            setValueOriginal(newValue);
            stopEdition();
            label.setText(juce::String(getValueOriginal()), juce::NotificationType::dontSendNotification);
        }
    };

    label.onEditorShow = [this]() {
        auto* editor = label.getCurrentTextEditor();
        if (editor != nullptr) {
            editor->setIndents(1, 2);
            editor->setBorder(juce::BorderSize<int>(0));
        }
    };

    updateValue();

    box->restrainer.setSizeLimits(100, 50, 500, 600);
    box->restrainer.checkComponentBounds(box);
}

void ListComponent::paint(juce::Graphics& g)
{
    static auto const border = 1.0f;
    const float h = static_cast<float>(getHeight());
    const float w = static_cast<float>(getWidth());
    const float o = h * 0.25f;
    juce::Path p;
    p.startNewSubPath(0.5f, 0.5f);
    p.lineTo(0.5f, h - 0.5f);
    p.lineTo(w - o, h - 0.5f);
    p.lineTo(w - 0.5f, h - o);
    p.lineTo(w - 0.5f, o);
    p.lineTo(w - o, 0.5f);
    p.closeSubPath();
    g.setColour(juce::Colour(static_cast<uint32>(gui.getBackgroundColor())));
    g.fillPath(p);
    g.setColour(juce::Colours::black);
    g.strokePath(p, juce::PathStrokeType(border));
}

void ListComponent::update()
{

    if (edited == false && !label.isBeingEdited()) {
        auto const array = gui.getList();
        juce::String message;
        for (auto const& atom : array) {
            if (message.isNotEmpty()) {
                message += " ";
            }
            if (atom.isFloat()) {
                message += juce::String(atom.getFloat());
            } else if (atom.isSymbol()) {
                message += juce::String(atom.getSymbol());
            }
        }
        label.setText(message, juce::NotificationType::dontSendNotification);
    }
}

// SliderComponent
SliderComponent::SliderComponent(bool vertical, pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    isVertical = vertical;
    addAndMakeVisible(slider);

    isLogarithmic = gui.isLogScale();

    if (vertical)
        slider.setSliderStyle(Slider::LinearVertical);

    slider.setRange(0., 1., 0.001);
    slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
    slider.setScrollWheelEnabled(false);
    
    slider.setValue(getValueScaled());
    
    slider.onDragStart = [this]() {
        startEdition();
    };
    

    slider.onValueChange = [this]() {
        const float val = slider.getValue();
        if (gui.isLogScale()) {
            float minimum = min == 0.0f ? std::numeric_limits<float>::epsilon() : min;
            setValueOriginal(exp(val * log(max / minimum)) * minimum);
        } else {
            setValueScaled(val);
        }
    };
    


    slider.onDragEnd = [this]() {
        stopEdition();
    };

    initParameters();

    if (isVertical) {
        box->restrainer.setSizeLimits(40, 100, 250, 500);
        box->restrainer.checkComponentBounds(box);
    } else {
        box->restrainer.setSizeLimits(100, 60, 500, 250);
        box->restrainer.checkComponentBounds(box);
    }
}

void SliderComponent::resized()
{
    gui.setSize(getWidth(), getHeight());
    slider.setBounds(getLocalBounds().reduced(isVertical ? 0.0 : 6.0, isVertical ? 6.0 : 0.0));
}

void SliderComponent::update()
{
    slider.setValue(getValueScaled(), dontSendNotification);
}

// RadioComponent
RadioComponent::RadioComponent(bool vertical, pd::Gui pdGui, Box* parent)
    : GUIComponent(pdGui, parent)
{
    isVertical = vertical;

    initParameters();
    updateRange();
    
    int selected = gui.getValue();
    radioButtons[selected]->setToggleState(true, dontSendNotification);

    if (isVertical) {
        box->restrainer.setSizeLimits(30, 100, 250, 500);
        box->restrainer.checkComponentBounds(box);
    } else {
        box->restrainer.setSizeLimits(100, 45, 500, 250);
        box->restrainer.checkComponentBounds(box);
    }
}

void RadioComponent::resized()
{
    gui.setSize(getWidth(), getHeight());
    
    FlexBox fb;
    fb.flexWrap = FlexBox::Wrap::noWrap;
    fb.justifyContent = FlexBox::JustifyContent::flexStart;
    fb.alignContent = FlexBox::AlignContent::flexStart;
    fb.flexDirection = isVertical ? FlexBox::Direction::column : FlexBox::Direction::row;
    
    for (auto* b : radioButtons) {
        auto item = FlexItem (*b).withMinWidth (8.0f).withMinHeight (8.0f);
        item.flexGrow = 1.0f;
        item.flexShrink = 1.0f;
        fb.items.add (item);
    }
    
    fb.performLayout (getLocalBounds().toFloat());
    
}

void RadioComponent::update()
{
    int selected = gui.getValue();
    radioButtons[selected]->setToggleState(true, dontSendNotification);
}

void RadioComponent::updateRange()
{
    minimum = gui.getMinimum();
    maximum = gui.getMaximum();

    int numButtons = maximum - minimum;

    radioButtons.clear();

    for (int i = 0; i < numButtons; i++) {
        radioButtons.add(new TextButton);
        radioButtons[i]->setConnectedEdges(12);
        radioButtons[i]->setRadioGroupId(1001);
        radioButtons[i]->setClickingTogglesState(true);
        addAndMakeVisible(radioButtons[i]);

        radioButtons[i]->onClick = [this, i]() mutable {
            last_state = i;
            setValueOriginal(i);
        };
    }

    box->resized();
    resized();
}

// Array component
ArrayComponent::ArrayComponent(pd::Gui pdGui, Box* box)
    : GUIComponent(pdGui, box)
    , graph(gui.getArray())
    , array(&box->cnv->main.pd, graph)
{
    setInterceptsMouseClicks(false, true);
    array.setBounds(getLocalBounds());
    addAndMakeVisible(&array);

    box->restrainer.setSizeLimits(100, 40, 500, 600);
}

void ArrayComponent::resized()
{
    array.setBounds(getLocalBounds());
}

// Array graph
GraphicalArray::GraphicalArray(PlugDataAudioProcessor* instance, pd::Array& graph)
    : array(graph)
    , edited(false)
    , pd(instance)
{
    if (graph.getName().empty())
        return;

    vec.reserve(8192);
    temp.reserve(8192);
    try {
        array.read(vec);
    } catch (...) {
        error = true;
    }
    startTimer(100);
    setInterceptsMouseClicks(true, false);
    setOpaque(false);
}

void GraphicalArray::paint(Graphics& g)
{
    g.fillAll(findColour(TextButton::buttonColourId));

    if (error) {
        // g.setFont(CamoLookAndFeel::getDefaultFont());
        g.drawText("array " + array.getName() + " is invalid", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
    } else {
        const float h = static_cast<float>(getHeight());
        const float w = static_cast<float>(getWidth());
        if (!vec.empty()) {
            const std::array<float, 2> scale = array.getScale();
            if (array.isDrawingCurve()) {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                for (size_t i = 1; i < vec.size() - 1; i += 2) {
                    const float y1 = h - (clip(vec[i - 1], scale[0], scale[1]) - scale[0]) * dh;
                    const float y2 = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                    const float y3 = h - (clip(vec[i + 1], scale[0], scale[1]) - scale[0]) * dh;
                    p.cubicTo(static_cast<float>(i - 1) * dw, y1,
                        static_cast<float>(i) * dw, y2,
                        static_cast<float>(i + 1) * dw, y3);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            } else if (array.isDrawingLine()) {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                for (size_t i = 1; i < vec.size(); ++i) {
                    const float y = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                    p.lineTo(static_cast<float>(i) * dw, y);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            } else {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size());
                g.setColour(findColour(ComboBox::outlineColourId));
                for (size_t i = 0; i < vec.size(); ++i) {
                    const float y = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                    g.drawLine(static_cast<float>(i) * dw, y, static_cast<float>(i + 1) * dw, y);
                }
            }
        }
    }

    g.setColour(findColour(ComboBox::outlineColourId));
    g.drawRect(getLocalBounds(), 1);
}

void GraphicalArray::mouseDown(const MouseEvent& event)
{
    if (error)
        return;
    edited = true;
    mouseDrag(event);
}

void GraphicalArray::mouseDrag(const MouseEvent& event)
{
    if (error)
        return;
    const float s = static_cast<float>(vec.size() - 1);
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const float x = static_cast<float>(event.x);
    const float y = static_cast<float>(event.y);

    const std::array<float, 2> scale = array.getScale();
    const size_t index = static_cast<size_t>(std::round(clip(x / w, 0.f, 1.f) * s));
    vec[index] = (1.f - clip(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];

    const CriticalSection* cs = pd->getCallbackLock();

    if (cs->tryEnter()) {
        try {
            array.write(index, vec[index]);
        } catch (...) {
            error = true;
        }
        cs->exit();
    }

    pd->enqueueMessages(stringArray, array.getName(), {});
    repaint();
}

void GraphicalArray::mouseUp(const MouseEvent& event)
{
    if (error)
        return;
    edited = false;
}

void GraphicalArray::timerCallback()
{
    if (!edited) {
        error = false;
        try {
            array.read(temp);
        } catch (...) {
            error = true;
        }
        if (temp != vec) {
            vec.swap(temp);
            repaint();
        }
    }
}

size_t GraphicalArray::getArraySize() const noexcept
{
    return vec.size();
}

// Graph On Parent
GraphOnParent::GraphOnParent(pd::Gui pdGui, Box* box)
    : GUIComponent(pdGui, box)
{

    setInterceptsMouseClicks(false, true);

    subpatch = gui.getPatch();
    updateCanvas();

    // subpatch.getPointer()->gl_pixwidth = 140;
    // subpatch.getPointer()->gl_pixheight = 140;

    bestH = subpatch.getPointer()->gl_pixheight;
    bestW = subpatch.getPointer()->gl_pixwidth;

    box->resized();
    resized();
}

GraphOnParent::~GraphOnParent()
{
    closeOpenedSubpatchers();
}

void GraphOnParent::resized()
{
}

void GraphOnParent::paint(Graphics& g)
{
    // g.setColour(findColour(TextButton::buttonColourId));
    // g.fillRect(getLocalBounds().reduced(1));
}

void GraphOnParent::updateCanvas()
{
    // if(isShowing() && !canvas) {
    //  It could be an optimisation to only construct the canvas if its showing
    //  But it's also kinda weird
    if (!canvas) {

        canvas.reset(new Canvas(box->cnv->main, subpatch, true));
        canvas->title = "Subpatcher";
        addAndMakeVisible(canvas.get());

        auto [x, y, w, h] = getPatch()->getBounds();

        canvas->setBounds(-x, -y, w + x, h + y);

        bestH = w;
        bestW = h;

        box->resized();

        // Make sure that the graph doesn't become the current canvas
        box->cnv->patch.setCurrent();
        box->cnv->main.updateUndoState();
    } /*

       else if(!isShowing() && canvas) {
       canvas.reset(nullptr);
       } */

    if (canvas) {
        auto [x, y, w, h] = getPatch()->getBounds();

        // x /= 2.0f;
        // y /= 2.0f;
        canvas->checkBounds();
        canvas->setBounds(-x, -y, w + x, h + y);

        bestW = w;
        bestH = h;
        
        auto parentBounds = box->getBounds();
        if(parentBounds.getWidth() != w - 8 || parentBounds.getHeight() != h - 29) {
            box->setSize(w + 8, h + 29);
        }
    }
}

void GraphOnParent::updateValue()
{

    updateCanvas();

    if (!canvas)
        return;

    for (auto& box : canvas->boxes) {
        if (box->graphics) {
            box->graphics->updateValue();
        }
    }
}

PanelComponent::PanelComponent(pd::Gui gui, Box* box) : GUIComponent(gui, box) {
    box->restrainer.setSizeLimits(50, 50, 2000, 2000);
    box->restrainer.checkComponentBounds(box);
    
    initParameters();
}

// Subpatch, phony GUI object
Subpatch::Subpatch(pd::Gui pdGui, Box* box)
    : GUIComponent(pdGui, box)
{
    subpatch = gui.getPatch();
}

void Subpatch::updateValue() {    
    // Pd sometimes sets the isgraph flag too late...
    // In that case we tell the box to create the gui
    if(static_cast<t_canvas*>(gui.getPointer())->gl_isgraph) {
        box->setType(box->textLabel.getText(), true);
        
        // Makes sure it has the correct size
        box->graphics->updateValue();
    }
    
};

Subpatch::~Subpatch()
{
    closeOpenedSubpatchers();
}

// Comment
CommentComponent::CommentComponent(pd::Gui pdGui, Box* box)
    : GUIComponent(pdGui, box)
{
    setInterceptsMouseClicks(false, false);
    setVisible(false);
}

void CommentComponent::paint(Graphics& g)
{
}

MousePad::MousePad(pd::Gui gui, Box* box)
    : GUIComponent(gui, box)
{
    Desktop::getInstance().addGlobalMouseListener(this);
    // setInterceptsMouseClicks(false, true);
}

MousePad::~MousePad() {
    Desktop::getInstance().removeGlobalMouseListener(this);
}

void MousePad::paint(Graphics& g) {

};

void MousePad::updateValue() {

};

void MousePad::mouseDown(const MouseEvent& e)
{
    
    if(!getScreenBounds().contains(e.getScreenPosition())) return;

    auto* glist = gui.getPatch().getPointer();
    auto* x = static_cast<t_pad*>(gui.getPointer());
    t_atom at[3];

    auto relativeEvent = e.getEventRelativeTo(this);
    
    // int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    x->x_x = (relativeEvent.getPosition().x / (float)getWidth()) * 127.0f;
    x->x_y = (relativeEvent.getPosition().y / (float)getHeight()) * 127.0f;


    SETFLOAT(at, 1.0f);
    sys_lock();
    outlet_anything(x->x_obj.ob_outlet, gensym("click"), 1, at);
    sys_unlock();

    // glist_grab(x->x_glist, &x->x_obj.te_g, (t_glistmotionfn)pad_motion, 0, (float)xpix, (float)ypix);
}

void MousePad::mouseDrag(const MouseEvent& e)
{
    
    mouseMove(e);
}

void MousePad::mouseMove(const MouseEvent& e)
{
    if(!getScreenBounds().contains(e.getScreenPosition())) return;
           
    auto* glist = gui.getPatch().getPointer();
    auto* x = static_cast<t_pad*>(gui.getPointer());
    t_atom at[3];
    
    auto relativeEvent = e.getEventRelativeTo(this);
    
    // int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    x->x_x = (relativeEvent.getPosition().x / (float)getWidth()) * 127.0f;
    x->x_y = (relativeEvent.getPosition().y / (float)getHeight()) * 127.0f;

    SETFLOAT(at, x->x_x);
    SETFLOAT(at + 1, x->x_y);

    sys_lock();
    outlet_anything(x->x_obj.ob_outlet, &s_list, 2, at);
    sys_unlock();
}

void MousePad::mouseUp(const MouseEvent& e)
{
    if(!getScreenBounds().contains(e.getScreenPosition())) return;
       
    auto* x = static_cast<t_pad*>(gui.getPointer());
    t_atom at[1];
    SETFLOAT(at, 0);
    outlet_anything(x->x_obj.ob_outlet, gensym("click"), 1, at);
}

MouseComponent::MouseComponent(pd::Gui gui, Box* box)
    : GUIComponent(gui, box)
{
    Desktop::getInstance().addGlobalMouseListener(this);
}

MouseComponent::~MouseComponent() {
    Desktop::getInstance().removeGlobalMouseListener(this);
}

void MouseComponent::updateValue()
{
};

void MouseComponent::mouseDown(const MouseEvent& e)
{
}
void MouseComponent::mouseMove(const MouseEvent& e)
{
    auto pos = Desktop::getInstance().getMousePosition();

    if (Desktop::getInstance().getMouseSource(0)->isDragging()) {

        t_atom args[1];
        SETFLOAT(args, 0);

        pd_typedmess((t_pd*)gui.getPointer(), gensym("_up"), 1, args);
    } else {
        t_atom args[1];
        SETFLOAT(args, 1);

        pd_typedmess((t_pd*)gui.getPointer(), gensym("_up"), 1, args);
    }

    t_atom args[2];
    SETFLOAT(args, pos.x);
    SETFLOAT(args + 1, pos.y);

    pd_typedmess((t_pd*)gui.getPointer(), gensym("_getscreen"), 2, args);
}

void MouseComponent::mouseUp(const MouseEvent& e)
{
}

void MouseComponent::mouseDrag(const MouseEvent& e)
{
}

KeyboardComponent::KeyboardComponent(pd::Gui gui, Box* box)
    : GUIComponent(gui, box)
    , keyboard(state, MidiKeyboardComponent::horizontalKeyboard)
{
    keyboard.setAvailableRange(36, 83);
    keyboard.setScrollButtonsVisible(false);

    state.addListener(this);
    addAndMakeVisible(keyboard);
}

void KeyboardComponent::resized()
{
    keyboard.setBounds(getLocalBounds());
}

void KeyboardComponent::updateValue() {

};

void KeyboardComponent::handleNoteOn(MidiKeyboardState* source,
    int midiChannel, int note, float velocity)
{
    auto* x = (t_keyboard*)gui.getPointer();

    box->cnv->pd->enqueueFunction([x, note, velocity]() mutable {
        int ac = 2;
        t_atom at[2];
        SETFLOAT(at, note);
        SETFLOAT(at + 1, velocity * 127);
        
        outlet_list(x->x_out, &s_list, ac, at);
        if (x->x_send != &s_ && x->x_send->s_thing)
            pd_list(x->x_send->s_thing, &s_list, ac, at);
    });
}

void KeyboardComponent::handleNoteOff(MidiKeyboardState* source,
    int midiChannel, int note, float velocity)
{

    auto* x = (t_keyboard*)gui.getPointer();
    
    box->cnv->pd->enqueueFunction([x, note, velocity]() mutable {
        int ac = 2;
        t_atom at[2];
        SETFLOAT(at, note);
        SETFLOAT(at + 1, 0);
        
        outlet_list(x->x_out, &s_list, ac, at);
        if (x->x_send != &s_ && x->x_send->s_thing)
            pd_list(x->x_send->s_thing, &s_list, ac, at);
    });
};

#define CLOSED 1 /* polygon */
#define BEZ 2 /* bezier shape */
#define NOMOUSERUN 4 /* disable mouse interaction when in run mode  */
#define NOMOUSEEDIT 8 /* same in edit mode */
#define NOVERTICES 16 /* disable only vertex grabbing in run mode */
#define A_ARRAY 55 /* LATER decide whether to enshrine this in m_pd.h */

/* getting and setting values via fielddescs -- note confusing names;
 the above are setting up the fielddesc itself. */
static t_float fielddesc_getfloat(t_fielddesc* f, t_template* templ,
    t_word* wp, int loud)
{
    if (f->fd_type == A_FLOAT) {
        if (f->fd_var)
            return (template_getfloat(templ, f->fd_un.fd_varsym, wp, loud));
        else
            return (f->fd_un.fd_float);
    } else {
        return (0);
    }
}

static int rangecolor(int n) /* 0 to 9 in 5 steps */
{
    int n2 = (n == 9 ? 8 : n); /* 0 to 8 */
    int ret = (n2 << 5); /* 0 to 256 in 9 steps */
    if (ret > 255)
        ret = 255;
    return (ret);
}

static void numbertocolor(int n, char* s)
{
    int red, blue, green;
    if (n < 0)
        n = 0;
    red = n / 100;
    blue = ((n / 10) % 10);
    green = n % 10;
    sprintf(s, "#%2.2x%2.2x%2.2x", rangecolor(red), rangecolor(blue),
        rangecolor(green));
}

void TemplateDraw::paintOnCanvas(Graphics& g, Canvas* canvas, t_scalar* scalar, t_gobj* obj, int baseX, int baseY)
{
    auto* glist = canvas->patch.getPointer();
    auto* x = (t_curve*)obj;
    auto* templ = template_findbyname(scalar->sc_template);

    bool vis = true;

    int i, n = x->x_npoints;
    t_fielddesc* f = x->x_vec;

    auto* data = scalar->sc_vec;

    /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, templ, data, 0)) {
        return;
    }

    // Reduce clip region
    // g.saveState();
    auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
    auto bounds = canvas->getParentComponent()->getLocalBounds().withPosition(pos);

    Path toDraw;

    if (vis) {
        if (n > 1) {
            int flags = x->x_flags, closed = (flags & CLOSED);
            t_float width = fielddesc_getfloat(&x->x_width, templ, data, 1);

            // TODO: use width to fix!
            char outline[20], fill[20];
            int pix[200];
            if (n > 100)
                n = 100;
            /* calculate the pixel values before we start printing
             out the TK message so that "error" printout won't be
             interspersed with it.  Only show up to 100 points so we don't
             have to allocate memory here. */
            for (i = 0, f = x->x_vec; i < n; i++, f += 2) {
                //glist->gl_havewindow = canvas->isGraphChild;
                //glist->gl_isgraph = canvas->isGraph;
                canvas->pd->canvasLock.lock();
                float xCoord = (baseX + fielddesc_getcoord(f, templ, data, 1)) / glist->gl_pixwidth;
                float yCoord = (baseY + fielddesc_getcoord(f + 1, templ, data, 1)) / glist->gl_pixheight;
                canvas->pd->canvasLock.unlock();
                
                pix[2 * i] = xCoord * bounds.getWidth() + pos.x;
                pix[2 * i + 1] = yCoord * bounds.getHeight() + pos.y;
            }
            if (width < 1)
                width = 1;
            if (glist->gl_isgraph)
                width *= glist_getzoom(glist);

            numbertocolor(fielddesc_getfloat(&x->x_outlinecolor, templ, data, 1), outline);
            if (flags & CLOSED) {
                numbertocolor(fielddesc_getfloat(&x->x_fillcolor, templ, data, 1), fill);

                // sys_vgui(".x%lx.c create polygon\\\n",
                //     glist_getcanvas(glist));
            }
            // else sys_vgui(".x%lx.c create line\\\n", glist_getcanvas(glist));

            // sys_vgui("%d %d\\\n", pix[2*i], pix[2*i+1]);

            if (flags & CLOSED) {
                toDraw.startNewSubPath(pix[0], pix[1]);
                for (i = 1; i < n; i++) {
                    toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
                }
                toDraw.lineTo(pix[0], pix[1]);
            } else {
                toDraw.startNewSubPath(pix[0], pix[1]);
                for (i = 1; i < n; i++) {
                    toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
                }
            }

            Colour juceColourOutline = Colour::fromString("FF" + String::fromUTF8(outline + 1));
            Colour juceColourFill = Colour::fromString("FF" + String::fromUTF8(fill + 1));

            g.setColour(juceColourFill);

            // sys_vgui("-width %f\\\n", width);

            String objName = String::fromUTF8(x->x_obj.te_g.g_pd->c_name->s_name);
            if (objName.contains("fill")) {
                g.fillPath(toDraw);

            } else {
                g.strokePath(toDraw, PathStrokeType(width));
            }

            if (flags & BEZ) {
                // sys_vgui("-smooth 1\\\n")
            };

            // sys_vgui("-tags curve%lx\n", data);
        } else
            post("warning: curves need at least two points to be graphed");
    } else {
        if (n > 1)
            sys_vgui(".x%lx.c delete curve%lx\n",
                glist_getcanvas(glist), data);
    }
}
