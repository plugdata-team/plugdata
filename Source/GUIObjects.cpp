#include "Connection.h"
#include "Edge.h"
#include "Box.h"
#include "GUIObjects.h"
#include "MainComponent.h"
#include "Canvas.h"


GUIComponent::GUIComponent(Box* parent)  : box(parent), m_processor(parent->cnv->main->pd), edited(false)
{
    //if(!box->pd_object) return;
    
    auto* checked_object = pd_checkobject(box->pd_object);
    assert(box->pd_object && checked_object);
    
    gui = pd::Gui(static_cast<void*>(checked_object), parent->cnv->main->pd.m_patch, &(parent->cnv->main->pd));
    
    value = gui.getValue();
    min = gui.getMinimum();
    max = gui.getMaximum();
    
    startTimer(25);
    
    setLookAndFeel(&banglook);
}

GUIComponent::~GUIComponent()
{
    setLookAndFeel(nullptr);
}

GUIComponent* GUIComponent::create_gui(String name, Box* parent)
{
    if(name == "bng") {
        return new BangComponent(parent);
    }
    if(name == "tgl") {
        return new ToggleComponent(parent);
    }
    if(name == "hsl") {
        return new SliderComponent(false, parent);
    }
    if(name == "vsl") {
        return new SliderComponent(true, parent);
    }
    if(name == "hradio") {
        return new RadioComponent(false, parent);
    }
    if(name == "vradio") {
        return new RadioComponent(true, parent);
    }
    if(name == "msg") {
        return new MessageComponent(parent);
    }
    if(name == "nbx") {
        return new NumboxComponent(parent);
    }
    if(name == "graph") {
        return new ArrayComponent(parent);
    }
    if(name == "canvas") {
        return new GraphOnParent(parent);
    }
    
    
    
    return nullptr;
}


float GUIComponent::getValueOriginal() const noexcept
{
    return value;
}

void GUIComponent::setValueOriginal(float v, bool sendNotification)
{
    value = (min < max) ? std::max(std::min(v, max), min) : std::max(std::min(v, min), max);
    if(sendNotification) gui.setValue(value);
}

float GUIComponent::getValueScaled() const noexcept
{
    return (min < max) ? (value - min) / (max - min) : 1.f - (value - max) / (min - max);
}

void GUIComponent::setValueScaled(float v)
{
    value = (min < max) ? std::max(std::min(v, 1.f), 0.f) * (max - min) + min
    : (1.f - std::max(std::min(v, 1.f), 0.f)) * (min - max) + max;
    gui.setValue(value);
}

void GUIComponent::startEdition() noexcept
{
    edited = true;
    m_processor.enqueueMessages(string_gui, string_mouse, {1.f});
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
        float const v = gui.getValue();
        if(v != value || gui.getType() == pd::Gui::Type::Message)
        {
            value = v;
            update();
            //repaint();
        }
    }
}


void GUIComponent::timerCallback()
{
    
    updateValue();
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

BangComponent::BangComponent(Box* parent) : GUIComponent(parent)
{
    addAndMakeVisible(bang_button);
    bang_button.onClick = [this](){
        startEdition();
        setValueOriginal(1);
        stopEdition();
    };
    
    auto highlight_colour = Colour (0xff42a2c8).darker(0.2);
    bang_button.setColour(TextButton::buttonOnColourId, highlight_colour);
}

void BangComponent::update()  {
    if(getValueOriginal() > std::numeric_limits<float>::epsilon()) {
        bang_button.setToggleState(true, dontSendNotification);
        bang_timer.startTimer(50);
    }
}

void BangComponent::resized() {
    bang_button.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}

// ToggleComponent
ToggleComponent::ToggleComponent(Box* parent) : GUIComponent(parent)
{
    addAndMakeVisible(toggle_button);
    
    toggle_button.onClick = [this](){
        startEdition();
        auto new_value = 1.f - getValueOriginal();
        setValueOriginal(new_value);
        toggle_button.setToggleState(new_value, dontSendNotification);
        stopEdition();
    };
    
    auto highlight_colour = Colour (0xff42a2c8).darker(0.2);
    toggle_button.setColour(TextButton::buttonOnColourId, highlight_colour);
}


void ToggleComponent::resized() {
    toggle_button.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}


void ToggleComponent::update()  {
    toggle_button.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
}


// MessageComponent
MessageComponent::MessageComponent(Box* parent) : GUIComponent(parent)
{
    
    bang_button.setConnectedEdges(12);
   
    addAndMakeVisible(input);
    addAndMakeVisible(bang_button);
    
    bang_button.onClick = [this](){
        startEdition();
        //gui.setSymbol(input.getText().toStdString());
        gui.click();
        stopEdition();
    };
    
    input.onTextChange = [this]() {
        gui.setSymbol(input.getText().toStdString());
    };
    
    input.onFocusLost = [this](){
        
        
        //t_atom args[1];
        //SETSYMBOL(args, gensym(input.getText().toRawUTF8()));
        //pd_typedmess(static_cast<t_pd*>(gui.getPointer()), gensym("set"), 1, args);
    };
    
    auto highlight_colour = Colour (0xff42a2c8).darker(0.2);
    bang_button.setColour(TextButton::buttonOnColourId, highlight_colour);
}


void MessageComponent::resized() {
    input.setBounds(0, 0, getWidth() - 28, getHeight());
    bang_button.setBounds(getWidth() - 29, 0, 29, getHeight());
}

String MessageComponent::parseData(libcerite::Data d) {
    if(d.type == libcerite::tNumber)
    {
        return String(d.number);
        
    }
    else if(d.type == libcerite::tString)
    {
        
        return String(d.string);
    }
    else if(d.type == libcerite::tBang)
    {
        return "bang";
    }
    else if(d.type == libcerite::tList)
    {
        String new_text;
        
        for(int i = 0; i < d.listlen; i++)
        {
            new_text += parseData(d.list[i]);
            if(i != d.listlen - 1) new_text += " ";
        }
        return new_text;
    }
    else {
        return "";
    }
    
    
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



NumboxComponent::NumboxComponent(Box* parent) : GUIComponent(parent)
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



SliderComponent::SliderComponent(bool is_vertical, Box* parent) : GUIComponent(parent)
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

RadioComponent::RadioComponent(bool is_vertical, Box* parent) : GUIComponent(parent)
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
            gui.setValue(i);
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


ArrayComponent::ArrayComponent(Box* box) : GUIComponent(box), m_graph(gui.getArray()), m_array(box->cnv->get_pd(), m_graph)
{
    setInterceptsMouseClicks(false, true);
    m_array.setBounds(getLocalBounds());
    addAndMakeVisible(&m_array);
    
}

void ArrayComponent::resized()
{
    m_array.setBounds(getLocalBounds());
}

GraphicalArray::GraphicalArray(PlugData* pd, pd::Array& graph) : m_array(graph), m_edited(false), m_instance(pd)
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
    
    //CriticalSection* cs = m_processor.getCallbackLock();
    
    //if(cs->tryEnter())
    //{
        try { m_array.write(index, m_vector[index]); }
        catch(...) { m_error = true; }
        //cs->exit();
    //}
    
    
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


GraphOnParent::GraphOnParent(Box* box) : GUIComponent(box), canvas(new Canvas(ValueTree(Identifiers::canvas), box->cnv->main))
{
    setInterceptsMouseClicks(false, true);
    addAndMakeVisible(canvas.get());

    subpatch = gui.getPatch();
    //canvas->load_patch(subpatch);
    
    resized();
    
}

void GraphOnParent::resized()
{
    canvas->setBounds(getLocalBounds());
}
