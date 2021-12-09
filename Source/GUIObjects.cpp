#include "Connection.h"
#include "Edge.h"
#include "Box.h"
#include "GUIObjects.h"
#include "PluginEditor.h"
#include "Canvas.h"


GUIComponent::GUIComponent(pd::Gui pdGui, Box* parent)  : box(parent), processor(parent->cnv->main.pd), gui(pdGui), edited(false)
{
    //if(!box->pdObject) return;
    value = gui.getValue();
    min = gui.getMinimum();
    max = gui.getMaximum();
    
    setLookAndFeel(&guiLook);
}

GUIComponent::~GUIComponent()
{
    setLookAndFeel(nullptr);
}

GUIComponent* GUIComponent::createGui(String name, Box* parent)
{
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
    ScopedLock lock (*box->cnv->main.pd.getCallbackLock());
    
    value = (min < max) ? std::max(std::min(v, max), min) : std::max(std::min(v, min), max);
    if(sendNotification) gui.setValue(value);
}

float GUIComponent::getValueScaled() const noexcept
{
    
    return (min < max) ? (value - min) / (max - min) : 1.f - (value - max) / (min - max);
}

void GUIComponent::setValueScaled(float v)
{
    ScopedLock lock (*box->cnv->main.pd.getCallbackLock());
    
    value = (min < max) ? std::max(std::min(v, 1.f), 0.f) * (max - min) + min
    : (1.f - std::max(std::min(v, 1.f), 0.f)) * (min - max) + max;
    gui.setValue(value);
}

void GUIComponent::startEdition() noexcept
{
    edited = true;
    processor.enqueueMessages(stringGui, stringMouse, {1.f});
    
    ScopedLock lock (*box->cnv->main.pd.getCallbackLock());
    value = gui.getValue();
}

void GUIComponent::stopEdition() noexcept
{
    edited = false;
    processor.enqueueMessages(stringGui, stringMouse, {0.f});
}

void GUIComponent::updateValue()
{
    if(edited == false)
    {
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

BangComponent::BangComponent(pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
{
    addAndMakeVisible(bangButton);
    bangButton.onClick = [this](){
        startEdition();
        setValueOriginal(1);
        stopEdition();
    };
}

void BangComponent::update()  {
    if(getValueOriginal() > std::numeric_limits<float>::epsilon()) {
        bangButton.setToggleState(true, dontSendNotification);
        startTimer(Canvas::guiUpdateMs);
    }
}

void BangComponent::resized() {
    bangButton.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}

// ToggleComponent
ToggleComponent::ToggleComponent(pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
{
    addAndMakeVisible(toggleButton);
    
    toggleButton.onClick = [this](){
        startEdition();
        auto new_value = 1.f - getValueOriginal();
        setValueOriginal(new_value);
        toggleButton.setToggleState(new_value, dontSendNotification);
        stopEdition();
    };
    

}


void ToggleComponent::resized() {
    toggleButton.setBounds(getWidth() / 4, getHeight() / 4, getWidth() / 2, getHeight() / 2);
}


void ToggleComponent::update()  {
    toggleButton.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
}


// MessageComponent
MessageComponent::MessageComponent(pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
{
    
    bangButton.setConnectedEdges(12);
   
    addAndMakeVisible(input);
    
    if(gui.getType() != pd::Type::AtomSymbol) {
        addAndMakeVisible(bangButton);
    }
    
    bangButton.onClick = [this](){
        startEdition();
        gui.click();
        stopEdition();
    };
    
    input.onTextChange = [this]() {
        gui.setSymbol(input.getText().toStdString());
    };
    
    input.setMultiLine(true);
    
}


void MessageComponent::resized() {
    int button_width = gui.getType() == pd::Type::AtomSymbol ? 0 : 28;
    
    input.setBounds(0, 0, getWidth() - button_width, getHeight());
    bangButton.setBounds(getWidth() - (button_width + 1), 0, (button_width + 1), getHeight());
}

void MessageComponent::update()  {

    input.setText(String(gui.getSymbol()), NotificationType::dontSendNotification);
}

void MessageComponent::updateValue()
{
    if(edited == false)
    {
        std::string const v = gui.getSymbol();
        
        if(lastMessage != v && !String(v).startsWith("click"))
        {
          
            lastMessage = v;
            update();
            //repaint();
        }
    }
}

// NumboxComponent



NumboxComponent::NumboxComponent(pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
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
SliderComponent::SliderComponent(bool is_vertical, pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
{
    isVertical = is_vertical;
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
    slider.setBounds(getLocalBounds().reduced(isVertical ? 0.0 : 6.0, isVertical ? 6.0 : 0.0));
}


void SliderComponent::update()  {
    slider.setValue(getValueScaled(), dontSendNotification);
}


// RadioComponent
RadioComponent::RadioComponent(bool is_vertical, pd::Gui pdGui, Box* parent) : GUIComponent(pdGui, parent)
{
    isVertical = is_vertical;
    
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
        radio_buttons[i].setBounds(isVertical ? getWidth() / 2 - 11 : i*20, isVertical ? (i*20) - 1 : -1, 21, 21);
    }
}

void RadioComponent::update()  {


}

// Array component
ArrayComponent::ArrayComponent(pd::Gui pdGui, Box* box) : GUIComponent(pdGui, box), graph(gui.getArray()), array(&box->cnv->main.pd, graph)
{
    setInterceptsMouseClicks(false, true);
    array.setBounds(getLocalBounds());
    addAndMakeVisible(&array);
    
    
}

void ArrayComponent::resized()
{
    array.setBounds(getLocalBounds());
}

// Array graph
GraphicalArray::GraphicalArray(PlugDataAudioProcessor* instance, pd::Array& graph) : array(graph), edited(false), pd(instance)
{
    if(graph.getName().empty()) return;
    
    vec.reserve(8192);
    temp.reserve(8192);
    try { array.read(vec); }
    catch(...) { error = true; }
    startTimer(100);
    setInterceptsMouseClicks(true, false);
    setOpaque(false);
}

void GraphicalArray::paint(Graphics& g)
{
    g.fillAll(findColour(TextButton::buttonColourId));
    
    if(error)
    {
        //g.setFont(CamoLookAndFeel::getDefaultFont());
        g.drawText("array " + array.getName() + " is invalid", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
    }
    else
    {
        const float h = static_cast<float>(getHeight());
        const float w = static_cast<float>(getWidth());
        if(!vec.empty())
        {
            const std::array<float, 2> scale = array.getScale();
            if(array.isDrawingCurve())
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                for(size_t i = 1; i < vec.size() - 1; i += 2)
                {
                    const float y1 = h - (clip(vec[i-1], scale[0], scale[1]) - scale[0]) * dh;
                    const float y2 = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                    const float y3 = h - (clip(vec[i+1], scale[0], scale[1]) - scale[0]) * dh;
                    p.cubicTo(static_cast<float>(i-1) * dw, y1,
                              static_cast<float>(i) * dw, y2,
                              static_cast<float>(i+1) * dw, y3);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            }
            else if(array.isDrawingLine())
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size() - 1);
                Path p;
                p.startNewSubPath(0, h - (clip(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                for(size_t i = 1; i < vec.size(); ++i)
                {
                    const float y = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                    p.lineTo(static_cast<float>(i) * dw, y);
                }
                g.setColour(findColour(ComboBox::outlineColourId));
                g.strokePath(p, PathStrokeType(1));
            }
            else
            {
                const float dh = h / (scale[1] - scale[0]);
                const float dw = w / static_cast<float>(vec.size());
                g.setColour(findColour(ComboBox::outlineColourId));
                for(size_t i = 0; i < vec.size(); ++i)
                {
                    const float y = h - (clip(vec[i], scale[0], scale[1]) - scale[0]) * dh;
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
    if(error)
        return;
    edited = true;
    mouseDrag(event);
}

void GraphicalArray::mouseDrag(const MouseEvent& event)
{
    if(error)
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
    
    if(cs->tryEnter())
    {
        try { array.write(index, vec[index]); }
        catch(...) { error = true; }
        cs->exit();
    }
    
    
    pd->enqueueMessages(stringArray, array.getName(), {});
    repaint();
}

void GraphicalArray::mouseUp(const MouseEvent& event)
{
    if(error)
        return;
    edited = false;
}

void GraphicalArray::timerCallback()
{
    if(!edited)
    {
        error = false;
        try { array.read(temp); }
        catch(...) { error = true; }
        if(temp != vec)
        {
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
GraphOnParent::GraphOnParent(pd::Gui pdGui, Box* box) : GUIComponent(pdGui, box)
{
    
    setInterceptsMouseClicks(false, true);

    subpatch = gui.getPatch();
    updateCanvas();
    
    subpatch.getPointer()->gl_pixwidth = 70;
    subpatch.getPointer()->gl_pixheight = 70;

    resized();
    
}

GraphOnParent::~GraphOnParent() {
    if(canvas && box) {
        canvas->closeAllInstances();
    }
    
}

void GraphOnParent::resized()
{

}

void GraphOnParent::paint(Graphics& g) {
    g.setColour(findColour(TextButton::buttonColourId));
    g.fillRect(getLocalBounds().reduced(1));
    
}

void GraphOnParent::updateCanvas() {
    if(isShowing() && !canvas) {
        
        canvas.reset(new Canvas(box->cnv->main, true));
        canvas->title = "Subpatcher";
        addAndMakeVisible(canvas.get());
        canvas->loadPatch(subpatch);
        
        // Make sure that the graph doesn't become the current canvas
        box->cnv->main.getCurrentCanvas()->patch.setCurrent();
        box->cnv->main.updateUndoState();
    }
    else if(!isShowing() && canvas) {
        canvas.reset(nullptr);
    }
    
    if(canvas) {
        auto const* pdCanvas = subpatch.getPointer();
        
        auto [x, y, w, h] = getPatch()->getBounds();
        
        x *= Canvas::zoomX;
        y *= Canvas::zoomY;
        w *= Canvas::zoomX;
        h *= Canvas::zoomY;
        
        canvas->setBounds(-x, -y, w + x, h + y);
        bestW = w;
        bestH = h;
        
        // This must be called but is pretty inefficient for how often this gets called!
        // TODO: optimise this
        box->resized();
        
    }
    
}

void GraphOnParent::updateValue() {
    
    updateCanvas();
    
    if(!canvas) return;
    
   
    
    for(auto& box : canvas->boxes) {
        if(box->graphics) {
            box->graphics->updateValue();
        }
    }
}

// Subpatch, phony UI
Subpatch::Subpatch(pd::Gui pdGui, Box* box) : GUIComponent(pdGui, box)
{
    subpatch = gui.getPatch();
}

Subpatch::~Subpatch() {
}

// Comment
CommentComponent::CommentComponent(pd::Gui pdGui, Box* box) : GUIComponent(pdGui, box)
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
