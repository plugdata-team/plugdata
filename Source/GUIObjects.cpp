#include "Connection.h"
#include "Edge.h"
#include "Box.h"
#include "GUIObjects.h"
#include "PluginEditor.h"
#include "Canvas.h"


GUIComponent::GUIComponent(pd::Gui pd_gui, Box* parent)  : box(parent), m_processor(parent->cnv->main->pd), gui(pd_gui), edited(false)
{
    //if(!box->pdObject) return;
    value = gui.getValue();
    min = gui.getMinimum();
    max = gui.getMaximum();
    
    setLookAndFeel(&banglook);
}

GUIComponent::~GUIComponent()
{
    setLookAndFeel(nullptr);
}

GUIComponent* GUIComponent::create_gui(String name, Box* parent)
{
    
    //auto* checked_object = pd_checkobject(parent->pdObject);
    //jassert(parent->pdObject && checked_object);
    
    auto* gui_ptr = dynamic_cast<pd::Gui*>(parent->pdObject.get());
    
    if(!gui_ptr) return nullptr;
    
    auto& gui = *gui_ptr;
    
    if(gui.getType() == pd::Type::Bang) {
        return new BangComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::Toggle) {
        return new ToggleComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::HorizontalSlider) {
        return new SliderComponent(false, gui, parent);
    }
    if(gui.getType() == pd::Type::VerticalSlider) {
        return new SliderComponent(true, gui, parent);
    }
    if(gui.getType() == pd::Type::HorizontalRadio) {
        return new RadioComponent(false, gui, parent);
    }
    if(gui.getType() == pd::Type::VerticalRadio) {
        return new RadioComponent(true, gui, parent);
    }
    if(gui.getType() == pd::Type::Message) {
        return new MessageComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::Number) {
        return new NumboxComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::Array) {
        return new ArrayComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::GraphOnParent) {
        return new GraphOnParent(gui, parent);
    }
    if(gui.getType() == pd::Type::Subpatch) {
        return new Subpatch(gui, parent);
    }
    if(gui.getType() == pd::Type::VuMeter) {
        return nullptr; //new GraphOnParent(gui, parent);
    }
    if(gui.getType() == pd::Type::Comment) {
        return new CommentComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::AtomNumber) {
        return new NumboxComponent(gui, parent);
    }
    if(gui.getType() == pd::Type::AtomSymbol) {
        return new MessageComponent(gui, parent);
    }
    
    return nullptr;
}


float GUIComponent::getValueOriginal() const noexcept
{
    return value;
}

void GUIComponent::setValueOriginal(float v, bool sendNotification)
{
    ScopedLock lock (*box->cnv->main->pd.getCallbackLock());
    
    value = (min < max) ? std::max(std::min(v, max), min) : std::max(std::min(v, min), max);
    if(sendNotification) gui.setValue(value);
}

float GUIComponent::getValueScaled() const noexcept
{
    
    return (min < max) ? (value - min) / (max - min) : 1.f - (value - max) / (min - max);
}

void GUIComponent::setValueScaled(float v)
{
    ScopedLock lock (*box->cnv->main->pd.getCallbackLock());
    
    value = (min < max) ? std::max(std::min(v, 1.f), 0.f) * (max - min) + min
    : (1.f - std::max(std::min(v, 1.f), 0.f)) * (min - max) + max;
    gui.setValue(value);
}

void GUIComponent::startEdition() noexcept
{
    edited = true;
    m_processor.enqueueMessages(string_gui, string_mouse, {1.f});
    
    ScopedLock lock (*box->cnv->main->pd.getCallbackLock());
    value = gui.getValue();
}

void GUIComponent::stopEdition() noexcept
{
    edited = false;
    m_processor.enqueueMessages(string_gui, string_mouse, {0.f});
}

void GUIComponent::updateValue()
{
    if(edited == false)
    {
        ScopedLock lock (*box->cnv->main->pd.getCallbackLock());
        float const v = gui.getValue();
        if(v != value || gui.getType() == pd::Type::Message)
        {
            value = v;
            update();
            //repaint();
        }
    }
}

std::unique_ptr<Label> GUIComponent::getLabel()
{
    pd::Label const lbl = gui.getLabel();
    const String text = String(lbl.getText());
    if(text.isNotEmpty())
    {
        auto label = std::make_unique<Label>();
        if(label == nullptr)
        {
            return nullptr;
        }
        const int width = 100; //ft.getStringWidth(text) + 1;
        const int height = 100; //ft.getHeight();
        const std::array<int, 2> position = lbl.getPosition();
        label->setBounds(position[0], position[1] - height / 2, width, height);
        //label->setFont(ft);
        label->setJustificationType(Justification::centredLeft);
        label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
        label->setMinimumHorizontalScale(1.f);
        label->setText(text, NotificationType::dontSendNotification);
        label->setEditable(false, false);
        label->setInterceptsMouseClicks(false, false);
        label->setColour(Label::textColourId, Colour(static_cast<uint32>(lbl.getColor())));
        return label;
    }
    return nullptr;
}

pd::Gui GUIComponent::getGUI()
{
    return gui;
}


// BangComponent

BangComponent::BangComponent(pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    addAndMakeVisible(bang_button);
    bang_button.onClick = [this](){
        startEdition();
        setValueOriginal(1);
        stopEdition();
    };
}

void BangComponent::update()  {
    if(getValueOriginal() > std::numeric_limits<float>::epsilon()) {
        bang_button.setToggleState(true, dontSendNotification);
        bang_timer.startTimer(Canvas::guiUpdateMs);
    }
}

void BangComponent::resized() {
    bang_button.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}

// ToggleComponent
ToggleComponent::ToggleComponent(pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    addAndMakeVisible(toggle_button);
    
    toggle_button.onClick = [this](){
        startEdition();
        auto new_value = 1.f - getValueOriginal();
        setValueOriginal(new_value);
        toggle_button.setToggleState(new_value, dontSendNotification);
        stopEdition();
    };
    

}


void ToggleComponent::resized() {
    toggle_button.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}


void ToggleComponent::update()  {
    toggle_button.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
}


// MessageComponent
MessageComponent::MessageComponent(pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    
    bang_button.setConnectedEdges(12);
   
    addAndMakeVisible(input);
    
    if(gui.getType() != pd::Type::AtomSymbol) {
        addAndMakeVisible(bang_button);
    }
    
    bang_button.onClick = [this](){
        startEdition();
        gui.click();
        stopEdition();
    };
    
    input.onTextChange = [this]() {
        gui.setSymbol(input.getText().toStdString());
    };
    
}


void MessageComponent::resized() {
    int button_width = gui.getType() == pd::Type::AtomSymbol ? 0 : 28;
    
    input.setBounds(0, 0, getWidth() - button_width, getHeight());
    bang_button.setBounds(getWidth() - (button_width + 1), 0, (button_width + 1), getHeight());
}

void MessageComponent::update()  {

    input.setText(String(gui.getSymbol()), NotificationType::dontSendNotification);
}

void MessageComponent::updateValue()
{
    if(edited == false)
    {
        std::string const v = gui.getSymbol();
        
        if(last_message != v && !String(v).startsWith("click"))
        {
          
            last_message = v;
            update();
            //repaint();
        }
    }
}

// NumboxComponent



NumboxComponent::NumboxComponent(pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    input.addMouseListener(this, false);
    
    input.setInputRestrictions(0, ".-0123456789");
    input.setText("0.");
    addAndMakeVisible(input);
    
    /*
    input.onFocusGained = [this]()
    {
        
    }; */
    
    input.onTextChange = [this]() {
        startEdition();
        setValueOriginal(input.getText().getFloatValue());
    };
    
    
    input.onFocusLost = [this]()
    {
        setValueOriginal(input.getText().getFloatValue());
        stopEdition();
    };
}

void NumboxComponent::resized() {
    input.setBounds(getLocalBounds());
}


void NumboxComponent::update()  {

    input.setText(String(getValueOriginal()), dontSendNotification);
}

// SliderComponent
SliderComponent::SliderComponent(bool is_vertical, pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    v_slider = is_vertical;
    addAndMakeVisible(slider);
    
    if(is_vertical) slider.setSliderStyle(Slider::LinearVertical);
    
    slider.setRange(0., 1., 0.001);
    slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
    
    slider.onDragStart = [this]() {
        startEdition();
    };
    
    slider.onValueChange = [this]()
    {
        
        const float val = slider.getValue();
        if(gui.isLogScale())
        {
            setValueOriginal(exp(val * log(max / min)) * min);
        }
        else
        {
            setValueScaled(val);
        }
        
    };
    
    slider.onDragEnd = [this]() {
        stopEdition();
    };

}


void SliderComponent::resized() {
    slider.setBounds(getLocalBounds().reduced(v_slider ? 0.0 : 6.0, v_slider ? 6.0 : 0.0));
}


void SliderComponent::update()  {
    slider.setValue(getValueScaled(), dontSendNotification);
}


// RadioComponent
RadioComponent::RadioComponent(bool is_vertical, pd::Gui pd_gui, Box* parent) : GUIComponent(pd_gui, parent)
{
    v_radio = is_vertical;
    
    for(int i = 0; i < 8; i++) {
        radio_buttons[i].setConnectedEdges(12);
        radio_buttons[i].setRadioGroupId(1001);
        radio_buttons[i].setClickingTogglesState(true);
        addAndMakeVisible(radio_buttons[i]);
    }
    
    for(int i = 0; i < radio_buttons.size(); i++) {
        radio_buttons[i].onClick = [this, i]() mutable {
            last_state = i;
            setValueOriginal(i);
        };
    }

}


void RadioComponent::resized() {
    for(int i = 0; i < 8; i++) {
        radio_buttons[i].setBounds(v_radio ? getWidth() / 2 - 11 : i*20, v_radio ? (i*20) - 1 : -1, 21, 21);
    }
}

void RadioComponent::update()  {


}

// Array component
ArrayComponent::ArrayComponent(pd::Gui pd_gui, Box* box) : GUIComponent(pd_gui, box), m_graph(gui.getArray()), m_array(&box->cnv->main->pd, m_graph)
{
    setInterceptsMouseClicks(false, true);
    m_array.setBounds(getLocalBounds());
    addAndMakeVisible(&m_array);
    
    
}

void ArrayComponent::resized()
{
    m_array.setBounds(getLocalBounds());
}

// Array graph
GraphicalArray::GraphicalArray(PlugDataAudioProcessor* pd, pd::Array& graph) : m_array(graph), m_edited(false), m_instance(pd)
{
    if(graph.getName().empty()) return;
    
    m_vector.reserve(8192);
    m_temp.reserve(8192);
    try { m_array.read(m_vector); }
    catch(...) { m_error = true; }
    startTimer(100);
    setInterceptsMouseClicks(true, false);
    setOpaque(false);
}

void GraphicalArray::paint(Graphics& g)
{
    g.fillAll(findColour(TextButton::buttonColourId));
    
    if(m_error)
    {
        //g.setFont(CamoLookAndFeel::getDefaultFont());
        g.drawText("array " + m_array.getName() + " is invalid", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
    }
    else
    {
        const float h = static_cast<float>(getHeight());
        const float w = static_cast<float>(getWidth());
        if(!m_vector.empty())
        {
            const std::array<float, 2> scale = m_array.getScale();
            if(m_array.isDrawingCurve())
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(m_vector.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(m_vector[0], scale[0], scale[1]) - scale[0]) * dh);
                for(size_t i = 1; i < m_vector.size() - 1; i += 2)
                {
                    const float y1 = h - (clip(m_vector[i-1], scale[0], scale[1]) - scale[0]) * dh;
                    const float y2 = h - (clip(m_vector[i], scale[0], scale[1]) - scale[0]) * dh;
                    const float y3 = h - (clip(m_vector[i+1], scale[0], scale[1]) - scale[0]) * dh;
                    p.cubicTo(static_cast<float>(i-1) * dw, y1,
                              static_cast<float>(i) * dw, y2,
                              static_cast<float>(i+1) * dw, y3);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            }
            else if(m_array.isDrawingLine())
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(m_vector.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(m_vector[0], scale[0], scale[1]) - scale[0]) * dh);
                for(size_t i = 1; i < m_vector.size(); ++i)
                {
                    const float y = h - (clip(m_vector[i], scale[0], scale[1]) - scale[0]) * dh;
                    p.lineTo(static_cast<float>(i) * dw, y);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            }
            else
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(m_vector.size());
                g.setColour(findColour(ComboBox::outlineColourId));
                for(size_t i = 0; i < m_vector.size(); ++i)
                {
                    const float y = h - (clip(m_vector[i], scale[0], scale[1]) - scale[0]) * dh;
                    g.drawLine(static_cast<float>(i) * dw, y, static_cast<float>(i+1) * dw, y);
                }
            }
        }
    }
    
    g.setColour(findColour(ComboBox::outlineColourId));
    g.drawRect(getLocalBounds(), 1);
}

void GraphicalArray::mouseDown(const MouseEvent& event)
{
    if(m_error)
        return;
    m_edited = true;
    mouseDrag(event);
}

void GraphicalArray::mouseDrag(const MouseEvent& event)
{
    if(m_error)
        return;
    const float s = static_cast<float>(m_vector.size() - 1);
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const float x = static_cast<float>(event.x);
    const float y = static_cast<float>(event.y);
    
    const std::array<float, 2> scale = m_array.getScale();
    const size_t index = static_cast<size_t>(std::round(clip(x / w, 0.f, 1.f) * s));
    m_vector[index] = (1.f - clip(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];
    
    
    const CriticalSection* cs = m_instance->getCallbackLock();
    
    if(cs->tryEnter())
    {
        try { m_array.write(index, m_vector[index]); }
        catch(...) { m_error = true; }
        cs->exit();
    }
    
    
    m_instance->enqueueMessages(string_array, m_array.getName(), {});
    repaint();
}

void GraphicalArray::mouseUp(const MouseEvent& event)
{
    if(m_error)
        return;
    m_edited = false;
}

void GraphicalArray::timerCallback()
{
    if(!m_edited)
    {
        m_error = false;
        try { m_array.read(m_temp); }
        catch(...) { m_error = true; }
        if(m_temp != m_vector)
        {
            m_vector.swap(m_temp);
            repaint();
        }
    }
}

size_t GraphicalArray::getArraySize() const noexcept
{
    return m_vector.size();
}

// Graph On Parent
GraphOnParent::GraphOnParent(pd::Gui pd_gui, Box* box) : GUIComponent(pd_gui, box)
{
    auto tree = ValueTree(Identifiers::canvas);
    tree.setProperty(Identifiers::isGraph, true, nullptr);
    tree.setProperty("Title", "Subpatcher", nullptr);
    
    canvas = box->appendChild<Canvas>(tree);
    
    setInterceptsMouseClicks(false, true);
    
    addAndMakeVisible(canvas);

    subpatch = gui.getPatch();
    
    canvas->isMainPatch = false;
    canvas->loadPatch(subpatch);

    resized();
    
}

GraphOnParent::~GraphOnParent() {
    if(canvas && box) {
        canvas->closeAllInstances();
    }
    
}

void GraphOnParent::resized()
{
    canvas->setBounds(getLocalBounds());
}

// Subpatch, phony UI
Subpatch::Subpatch(pd::Gui pd_gui, Box* box) : GUIComponent(pd_gui, box)
{
    
    auto tree = ValueTree(Identifiers::canvas);
    tree.setProperty(Identifiers::isGraph, true, nullptr);
    tree.setProperty("Title",  box->textLabel.getText().fromFirstOccurrenceOf("pd ", false, false), nullptr);
        
    canvas = box->appendChild<Canvas>(tree);
    
    subpatch = gui.getPatch();
    
    canvas->isMainPatch = false;
    canvas->loadPatch(subpatch);
}

Subpatch::~Subpatch() {
    if(canvas && box) {
        canvas->closeAllInstances();
    }
}

// Comment
CommentComponent::CommentComponent(pd::Gui pd_gui, Box* box) : GUIComponent(pd_gui, box)
{
    setInterceptsMouseClicks(false, false);
    edited = true;
}

void CommentComponent::paint(Graphics& g)
{
    const auto fheight = gui.getFontHeight();
    //auto const ft = PdGuiLook::getFont(gui.getFontName()).withPointHeight(fheight);
    auto ft = Font(13);
    g.setFont(ft);
    g.setColour(Colours::black);
    g.drawMultiLineText(gui.getText(), 0, static_cast<int>(ft.getAscent()), getWidth());
}
