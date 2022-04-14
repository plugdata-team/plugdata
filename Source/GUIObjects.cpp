/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "GUIObjects.h"

extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <g_all_guis.h>

#include <memory>
}

#include "Box.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

// False GATOM
typedef struct _fake_gatom
{
    t_text a_text;
    int a_flavor;          /* A_FLOAT, A_SYMBOL, or A_LIST */
    t_glist* a_glist;      /* owning glist */
    t_float a_toggle;      /* value to toggle to */
    t_float a_draghi;      /* high end of drag range */
    t_float a_draglo;      /* low end of drag range */
    t_symbol* a_label;     /* symbol to show as label next to box */
    t_symbol* a_symfrom;   /* "receive" name -- bind ourselves to this */
    t_symbol* a_symto;     /* "send" name -- send to this on output */
    t_binbuf* a_revertbuf; /* binbuf to revert to if typing canceled */
    int a_dragindex;       /* index of atom being dragged */
    int a_fontsize;
    unsigned int a_shift : 1;         /* was shift key down when drag started? */
    unsigned int a_wherelabel : 2;    /* 0-3 for left, right, above, below */
    unsigned int a_grabbed : 1;       /* 1 if we've grabbed keyboard */
    unsigned int a_doubleclicked : 1; /* 1 if dragging from a double click */
    t_symbol* a_expanded_to;
} t_fake_gatom;

GUIComponent::GUIComponent(pd::Gui pdGui, Box* parent, bool newObject) : box(parent), processor(*parent->cnv->pd), gui(std::move(pdGui)), edited(false)
{
    // if(!box->pdObject) return;
    const CriticalSection* cs = box->cnv->pd->getCallbackLock();
    
    cs->enter();
    value = gui.getValue();
    min = gui.getMinimum();
    max = gui.getMaximum();
    cs->exit();
    
    if (gui.isIEM())
    {
        
        auto rect = gui.getLabelPosition(Rectangle<int>());
        labelX = rect.getX();
        labelY = rect.getY();
        labelHeight = static_cast<t_iemgui*>(gui.getPointer())->x_fontsize;
    }
    else if (gui.isAtom())
    {
        labelX = static_cast<int>(static_cast<t_fake_gatom*>(gui.getPointer())->a_wherelabel + 1);
        labelHeight = 16;
    }
    
    updateLabel();
    
    sendSymbol = gui.getSendSymbol();
    receiveSymbol = gui.getReceiveSymbol();
    
    setWantsKeyboardFocus(true);
    
    setLookAndFeel(dynamic_cast<PlugDataLook*>(&getLookAndFeel())->getPdLook());
}

GUIComponent::~GUIComponent()
{
    box->removeComponentListener(this);
    auto* lnf = &getLookAndFeel();
    setLookAndFeel(nullptr);
    delete lnf;
}

void GUIComponent::lock(bool isLocked)
{
    setInterceptsMouseClicks(isLocked, isLocked);
}

void GUIComponent::mouseDown(const MouseEvent& e)
{
    if (box->commandLocked == var(true))
    {
        auto& sidebar = box->cnv->main.sidebar;
        inspectorWasVisible = !sidebar.isShowingConsole();
        sidebar.hideParameters();
    }
}

void GUIComponent::mouseUp(const MouseEvent& e)
{
    if (box->commandLocked == var(true) && inspectorWasVisible)
    {
        box->cnv->main.sidebar.showParameters();
    }
}

void GUIComponent::initialise(bool newObject)
{
    if (gui.getType() == pd::Type::Number)
    {
        auto color = Colour::fromString(secondaryColour.toString());
        secondaryColour = color.toString();
    }
    
    labelText = gui.getLabelText();
    
    if (gui.isIEM()) {
    
        primaryColour = Colour(gui.getForegroundColour()).toString();
        secondaryColour = Colour(gui.getBackgroundColour()).toString();
        labelColour = Colour(gui.getLabelColour()).toString();
        
        getLookAndFeel().setColour(TextButton::buttonOnColourId, Colour::fromString(primaryColour.toString()));
        getLookAndFeel().setColour(Slider::thumbColourId, Colour::fromString(primaryColour.toString()));
        
        getLookAndFeel().setColour(TextEditor::backgroundColourId, Colour::fromString(secondaryColour.toString()));
        getLookAndFeel().setColour(TextButton::buttonColourId, Colour::fromString(secondaryColour.toString()));
        
        auto sliderBackground = Colour::fromString(secondaryColour.toString());
        sliderBackground = sliderBackground.getBrightness() > 0.5f ? sliderBackground.darker(0.6f) : sliderBackground.brighter(0.6f);
        
        getLookAndFeel().setColour(Slider::backgroundColourId, sliderBackground);
    }
    
    auto params = getParameters();
    for (auto& [name, type, cat, value, list] : params)
    {
        value->addListener(this);
        
        // Push current parameters to pd
        valueChanged(*value);
    }
    
    repaint();
}

void GUIComponent::paint(Graphics& g)
{
    g.setColour(findColour(TextButton::buttonColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
}

void GUIComponent::paintOverChildren(Graphics& g)
{
    if (gui.isAtom())
    {
        g.setColour(findColour(Slider::thumbColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        
        g.fillPath(triangle);
    }
}

ObjectParameters GUIComponent::defineParameters()
{
    return {};
};

ObjectParameters GUIComponent::getParameters()
{
    ObjectParameters params = defineParameters();
    
    if (gui.isIEM())
    {
        params.push_back({"Foreground", tColour, cAppearance, &primaryColour, {}});
        params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Colour", tColour, cLabel, &labelColour, {}});
        params.push_back({"Label X", tInt, cLabel, &labelX, {}});
        params.push_back({"Label Y", tInt, cLabel, &labelY, {}});
        params.push_back({"Label Height", tInt, cLabel, &labelHeight, {}});
    }
    else if (gui.isAtom())
    {
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        
        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Position", tCombo, cLabel, &labelX, {"left", "right", "top", "bottom"}});
    }
    return params;
}

void GUIComponent::valueChanged(Value& v)
{
    if (v.refersToSameSourceAs(sendSymbol))
    {
        gui.setSendSymbol(sendSymbol.toString());
    }
    else if (v.refersToSameSourceAs(receiveSymbol))
    {
        gui.setReceiveSymbol(receiveSymbol.toString());
    }
    else if (v.refersToSameSourceAs(primaryColour))
    {
        auto colour = Colour::fromString(primaryColour.toString());
        gui.setForegroundColour(colour);
        
        getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
        getLookAndFeel().setColour(Slider::thumbColourId, colour);
        repaint();
    }
    else if (v.refersToSameSourceAs(secondaryColour))
    {
        auto colour = Colour::fromString(secondaryColour.toString());
        gui.setBackgroundColour(colour);
        
        getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
        getLookAndFeel().setColour(TextButton::buttonColourId, colour);
        
        getLookAndFeel().setColour(Label::textColourId, colour.contrasting(1.0f));
        getLookAndFeel().setColour(TextEditor::textColourId, colour.contrasting(1.0f));
        
        auto sliderBackground = Colour::fromString(secondaryColour.toString());
        sliderBackground = sliderBackground.getBrightness() > 0.5f ? sliderBackground.darker(0.5f) : sliderBackground.brighter(0.5f);
        
        auto sliderTrack = Colour::fromString(secondaryColour.toString());
        sliderTrack = sliderTrack.getBrightness() > 0.5f ? sliderTrack.darker(0.2f) : sliderTrack.brighter(0.2f);
        
        getLookAndFeel().setColour(Slider::backgroundColourId, sliderTrack);
        getLookAndFeel().setColour(Slider::trackColourId, sliderBackground);
        
        repaint();
    }
    
    else if (v.refersToSameSourceAs(labelColour))
    {
        gui.setLabelColour(Colour::fromString(labelColour.toString()));
        updateLabel();
    }
    else if (v.refersToSameSourceAs(labelX))
    {
        if (gui.isAtom())
        {
            gui.setLabelPosition(static_cast<int>(labelX.getValue()));
            updateLabel();
        }
        else
        {
            gui.setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
            updateLabel();
        }
    }
    else if (v.refersToSameSourceAs(labelY))
    {
        gui.setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
        updateLabel();
    }
    else if (v.refersToSameSourceAs(labelHeight))
    {
        gui.setFontHeight(static_cast<int>(labelHeight.getValue()));
        updateLabel();
    }
    else if (v.refersToSameSourceAs(labelText))
    {
        gui.setLabelText(labelText.toString());
        updateLabel();
    }
}

pd::Patch* GUIComponent::getPatch()
{
    return nullptr;
}

Canvas* GUIComponent::getCanvas()
{
    return nullptr;
}

bool GUIComponent::fakeGui()
{
    return false;
}

float GUIComponent::getValueOriginal() const noexcept
{
    return value;
}

void GUIComponent::setValueOriginal(float v)
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());
    
    value = (minimum < maximum) ? std::max(std::min(v, maximum), minimum) : std::max(std::min(v, minimum), maximum);
    
    gui.setValue(value);
}

float GUIComponent::getValueScaled() const noexcept
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());
    
    return (minimum < maximum) ? (value - minimum) / (maximum - minimum) : 1.f - (value - maximum) / (minimum - maximum);
}

void GUIComponent::setValueScaled(float v)
{
    auto minimum = static_cast<float>(min.getValue());
    auto maximum = static_cast<float>(max.getValue());
    
    value = (minimum < maximum) ? std::max(std::min(v, 1.f), 0.f) * (maximum - minimum) + minimum : (1.f - std::max(std::min(v, 1.f), 0.f)) * (minimum - maximum) + maximum;
    gui.setValue(value);
}

void GUIComponent::startEdition() noexcept
{
    edited = true;
    processor.enqueueMessages(stringGui, stringMouse, {1.f});
    
    value = gui.getValue();
}

void GUIComponent::stopEdition() noexcept
{
    edited = false;
    processor.enqueueMessages(stringGui, stringMouse, {0.f});
}

void GUIComponent::updateValue()
{
    if (!edited)
    {
        auto thisPtr = SafePointer<GUIComponent>(this);
        box->cnv->pd->enqueueFunction(
                                      [thisPtr]()
                                      {
                                          float const v = thisPtr->gui.getValue();
                                          
                                          MessageManager::callAsync(
                                                                    [thisPtr, v]() mutable
                                                                    {
                                                                        if (thisPtr && v != thisPtr->value)
                                                                        {
                                                                            thisPtr->value = v;
                                                                            thisPtr->update();
                                                                        }
                                                                    });
                                      });
    }
}

void GUIComponent::componentMovedOrResized(Component& component, bool moved, bool resized)
{
    updateLabel();
}

void GUIComponent::updateLabel()
{
    const int fontHeight = static_cast<int>(labelHeight.getValue());
    const String text = gui.getLabelText();
    if (text.isNotEmpty())
    {
        if(!label) {
            label = std::make_unique<Label>();
        }
        
        label->setFont(Font(fontHeight));
        label->setJustificationType(Justification::left);
        
        Point<int> position;
        
        int labelWidth = label->getFont().getStringWidth(text);
        int height = 21;
        
        if(gui.isAtom()) {
            
            int labelPosition = labelX.getValue();
            auto boxBounds = box->getBounds().reduced(Box::margin);
            
            if(labelPosition == 1) { // left
                label->setJustificationType(Justification::right);
                position = {boxBounds.getX() - labelWidth - 4, boxBounds.getY()};
                
            }
            if(labelPosition == 2) { // right
                position = {boxBounds.getRight() + 4, boxBounds.getY()};
            }
            if(labelPosition == 3) { // top
                position = {boxBounds.getX(), boxBounds.getY() - height - 2};
            }
            if(labelPosition == 4) { // bottom
                position = {boxBounds.getX(), boxBounds.getBottom() + 2};
            }
        }
        else {
            position = gui.getLabelPosition(box->getBounds().reduced(Box::margin));
        }
        
        if(fontHeight > height) {
            position.y -= (fontHeight - height) / 2;
            height = fontHeight;
        }
    
        label->setBounds(position.x, position.y, labelWidth, height);
        
        label->setFont(Font(static_cast<int>(labelHeight.getValue())));

        label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
        label->setMinimumHorizontalScale(1.f);
        label->setText(text, dontSendNotification);
        label->setEditable(false, false);
        label->setInterceptsMouseClicks(false, false);
        
        if(gui.isIEM()) {
            label->setColour(Label::textColourId, gui.getLabelColour());
        }
        else {
            
            label->setColour(Label::textColourId, box->cnv->backgroundColour.contrasting());
        }
        
        // label->setColour(Label::textColourId, Colour(static_cast<uint32>(lbl.getColour())));
        box->cnv->addAndMakeVisible(label.get());
        box->addComponentListener(this);
    }
}

pd::Gui GUIComponent::getGui()
{
    return gui;
}

// Called in destructor of subpatch and graph class
// Makes sure that any tabs refering to the now deleted patch will be closed
void GUIComponent::closeOpenedSubpatchers()
{
    auto& main = box->cnv->main;
    auto* tabbar = &main.tabbar;
    
    if (!tabbar) return;
    
    for (int n = 0; n < tabbar->getNumTabs(); n++)
    {
        auto* cnv = main.getCanvas(n);
        if (cnv && cnv->patch == *getPatch())
        {
            tabbar->removeTab(n);
            main.pd.patches.removeFirstMatchingValue(cnv->patch);
            main.canvases.removeObject(cnv);
        }
    }
    
    if (tabbar->getNumTabs() > 1)
    {
        tabbar->getTabbedButtonBar().setVisible(true);
        tabbar->setTabBarDepth(30);
    }
    else
    {
        tabbar->getTabbedButtonBar().setVisible(false);
        tabbar->setTabBarDepth(1);
    }
}

#define CLOSED 1      /* polygon */
#define BEZ 2         /* bezier shape */
#define NOMOUSERUN 4  /* disable mouse interaction when in run mode  */
#define NOMOUSEEDIT 8 /* same in edit mode */
#define NOVERTICES 16 /* disable only vertex grabbing in run mode */
#define A_ARRAY 55    /* LATER decide whether to enshrine this in m_pd.h */

/* getting and setting values via fielddescs -- note confusing names;
 the above are setting up the fielddesc itself. */
static t_float fielddesc_getfloat(t_fielddesc* f, t_template* templ, t_word* wp, int loud)
{
    if (f->fd_type == A_FLOAT)
    {
        if (f->fd_var)
            return (template_getfloat(templ, f->fd_un.fd_varsym, wp, loud));
        else
            return (f->fd_un.fd_float);
    }
    else
    {
        return (0);
    }
}

static int rangecolor(int n) /* 0 to 9 in 5 steps */
{
    int n2 = (n == 9 ? 8 : n); /* 0 to 8 */
    int ret = (n2 << 5);       /* 0 to 256 in 9 steps */
    if (ret > 255) ret = 255;
    return (ret);
}

static void numbertocolor(int n, char* s)
{
    int red, blue, green;
    if (n < 0) n = 0;
    red = n / 100;
    blue = ((n / 10) % 10);
    green = n % 10;
    sprintf(s, "#%2.2x%2.2x%2.2x", rangecolor(red), rangecolor(blue), rangecolor(green));
}

struct t_curve
{
    t_object x_obj;
    int x_flags; /* CLOSED, BEZ, NOMOUSERUN, NOMOUSEEDIT */
    t_fielddesc x_fillcolor;
    t_fielddesc x_outlinecolor;
    t_fielddesc x_width;
    t_fielddesc x_vis;
    int x_npoints;
    t_fielddesc* x_vec;
    t_canvas* x_canvas;
};

DrawableTemplate::DrawableTemplate(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y) : scalar(s), object(reinterpret_cast<t_curve*>(obj)), canvas(cnv), baseX(x), baseY(y)
{
    setBufferedToImage(true);
}

void DrawableTemplate::updateIfMoved()
{
    auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
    auto bounds = canvas->getParentComponent()->getLocalBounds() + pos;
    
    if (lastBounds != bounds)
    {
        update();
    }
}

void DrawableTemplate::update()
{
    auto* glist = canvas->patch.getPointer();
    auto* templ = template_findbyname(scalar->sc_template);
    
    bool vis = true;
    
    int i, n = object->x_npoints;
    t_fielddesc* f = object->x_vec;
    
    auto* data = scalar->sc_vec;
    
    /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&object->x_vis, templ, data, 0))
    {
        // return;
    }
    
    // Reduce clip region
    auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
    auto bounds = canvas->getParentComponent()->getLocalBounds();
    
    lastBounds = bounds + pos;
    
    if (vis)
    {
        if (n > 1)
        {
            int flags = object->x_flags, closed = (flags & CLOSED);
            t_float width = fielddesc_getfloat(&object->x_width, templ, data, 1);
            
            char outline[20], fill[20];
            int pix[200];
            if (n > 100) n = 100;
            
            canvas->pd->getCallbackLock()->enter();
            
            for (i = 0, f = object->x_vec; i < n; i++, f += 2)
            {
                // glist->gl_havewindow = canvas->isGraphChild;
                // glist->gl_isgraph = canvas->isGraph;
                
                float xCoord = (baseX + fielddesc_getcoord(f, templ, data, 1)) / glist->gl_pixwidth;
                float yCoord = (baseY + fielddesc_getcoord(f + 1, templ, data, 1)) / glist->gl_pixheight;
                
                pix[2 * i] = xCoord * bounds.getWidth() + pos.x;
                pix[2 * i + 1] = yCoord * bounds.getHeight() + pos.y;
            }
            
            canvas->pd->getCallbackLock()->exit();
            
            if (width < 1) width = 1;
            if (glist->gl_isgraph) width *= glist_getzoom(glist);
            
            numbertocolor(fielddesc_getfloat(&object->x_outlinecolor, templ, data, 1), outline);
            if (flags & CLOSED)
            {
                numbertocolor(fielddesc_getfloat(&object->x_fillcolor, templ, data, 1), fill);
                
                // sys_vgui(".x%lx.c create polygon\\\n",
                //     glist_getcanvas(glist));
            }
            // else sys_vgui(".x%lx.c create line\\\n", glist_getcanvas(glist));
            
            // sys_vgui("%d %d\\\n", pix[2*i], pix[2*i+1]);
            
            Path toDraw;
            
            if (flags & CLOSED)
            {
                toDraw.startNewSubPath(pix[0], pix[1]);
                for (i = 1; i < n; i++)
                {
                    toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
                }
                toDraw.lineTo(pix[0], pix[1]);
            }
            else
            {
                toDraw.startNewSubPath(pix[0], pix[1]);
                for (i = 1; i < n; i++)
                {
                    toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
                }
            }
            
            String objName = String::fromUTF8(object->x_obj.te_g.g_pd->c_name->s_name);
            if (objName.contains("fill"))
            {
                setFill(Colour::fromString("FF" + String::fromUTF8(fill + 1)));
                setStrokeThickness(0.0f);
            }
            else
            {
                setFill(Colours::transparentBlack);
                setStrokeFill(Colour::fromString("FF" + String::fromUTF8(outline + 1)));
                setStrokeThickness(width);
            }
            
            setPath(toDraw);
            repaint();
        }
        else
            post("warning: curves need at least two points to be graphed");
    }
}

struct BangComponent : public GUIComponent
{
    uint32_t lastBang = 0;
    
    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);
    
    // TODO: fix in LookAndFeel!
    struct BangButton : public TextButton
    {
        void paint(Graphics& g) override
        {
            const auto bounds = getLocalBounds().reduced(1).toFloat();
            const auto width = std::max(bounds.getWidth(), bounds.getHeight());
            
            const float circleOuter = 80.f * (width * 0.01f);
            const float circleThickness = std::max(8.f * (width * 0.01f), 2.0f);
            
            g.setColour(findColour(ComboBox::outlineColourId));
            g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);
            
            g.setColour(getToggleState() ? findColour(TextButton::buttonOnColourId) : Colours::transparentWhite);
            
            g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
        }
    };
    
    BangButton bangButton;
    
    BangComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        addAndMakeVisible(bangButton);
        
        bangButton.setTriggeredOnMouseDown(true);
        bangButton.setName("pd:bang");
        
        bangButton.onClick = [this]()
        {
            startEdition();
            setValueOriginal(1);
            stopEdition();
            update();
        };
        
        initialise(newObject);
        box->constrainer.setSizeLimits(Box::height + 2, Box::height + 2, maxSize, maxSize);
        box->constrainer.setFixedAspectRatio(1.0f);
    }
    
    void update() override
    {
        if (getValueOriginal() > std::numeric_limits<float>::epsilon())
        {
            bangButton.setToggleState(true, dontSendNotification);
            
            auto currentTime = Time::getCurrentTime().getMillisecondCounter();
            auto timeSinceLast = currentTime - lastBang;
            
            int holdTime = bangHold.getValue();
            
            if (timeSinceLast < static_cast<int>(bangHold.getValue()) * 2)
            {
                holdTime = timeSinceLast / 2;
            }
            if (holdTime < bangInterrupt)
            {
                holdTime = bangInterrupt.getValue();
            }
            
            lastBang = currentTime;
            
            auto button = SafePointer<TextButton>(&bangButton);
            Timer::callAfterDelay(holdTime,
                                  [button]() mutable
                                  {
                if (!button) return;
                button->setToggleState(false, dontSendNotification);
                if (button->isDown())
                {
                    button->setState(Button::ButtonState::buttonNormal);
                }
            });
        }
    }
    
    void resized() override
    {
        bangButton.setBounds(getLocalBounds().reduced(1));
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    ObjectParameters defineParameters() override
    {
        return {
            {"Interrupt", tInt, cGeneral, &bangInterrupt, {}},
            {"Hold", tInt, cGeneral, &bangHold, {}},
        };
    }
    
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(bangInterrupt))
        {
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold))
        {
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold = bangHold.getValue();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};

struct ToggleComponent : public GUIComponent
{

    
    struct Toggle : public TextButton
    {
        std::function<void()> onMouseDown;
        
        void paint(Graphics& g) override
        {
            auto offColour = findColour(TextButton::buttonColourId).overlaidWith(Colours::grey.withAlpha(0.42f));
            
            g.setColour(getToggleState() ? findColour(TextButton::buttonOnColourId) : offColour);
            
            const auto crossBounds = getLocalBounds().reduced(6).toFloat();
            
            const auto max = std::max(crossBounds.getWidth(), crossBounds.getHeight());
            const auto strokeWidth = std::max(max * 0.15f, 2.0f);
            
            g.drawLine({crossBounds.getTopLeft(), crossBounds.getBottomRight()}, strokeWidth);
            g.drawLine({crossBounds.getBottomLeft(), crossBounds.getTopRight()}, strokeWidth);
        }
        
        void mouseDown(const MouseEvent& e) override {
            TextButton::mouseDown(e);
            onMouseDown();
        }
    };
    
    Toggle toggleButton;
    
    ToggleComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        addAndMakeVisible(toggleButton);
        toggleButton.setToggleState(getValueOriginal(), dontSendNotification);
        toggleButton.setConnectedEdges(12);
        toggleButton.setName("pd:toggle");
        
        toggleButton.onMouseDown = [this]()
        {
            startEdition();
            auto newValue = 1.f - getValueOriginal();
            setValueOriginal(newValue);
            toggleButton.setToggleState(newValue, dontSendNotification);
            stopEdition();
            
            update();
        };
        
        initialise(newObject);
        
        box->constrainer.setSizeLimits(Box::height + 2, Box::height + 2, maxSize, maxSize);
        box->constrainer.setFixedAspectRatio(1.0f);
    }
    
    void resized() override
    {
        toggleButton.setBounds(getLocalBounds());
    }
    
    void update() override
    {
        toggleButton.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
};

struct MessageComponent : public GUIComponent
{
    bool isDown = false;
    bool isLocked = false;
    bool shouldOpenEditor = false;
    
    Label input;
    
    std::string lastMessage;
    
    MessageComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        addAndMakeVisible(input);
        
        input.setInterceptsMouseClicks(false, false);
        
        input.onTextChange = [this]()
        {
            startEdition();
            gui.setSymbol(input.getText().toStdString());
            stopEdition();
            
            auto width = input.getFont().getStringWidth(input.getText()) + 25;
            if (width < box->getWidth())
            {
                box->setSize(width, box->getHeight());
                box->constrainer.checkComponentBounds(box);
            }
        };
        
        // message box behaviour
        if (!gui.isAtom())
        {
            input.getLookAndFeel().setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            
            // input.onTextChange = [this]() { gui.setSymbol(input.getText().toStdString()); };
            
            // For the autoresize while typing feature
            input.onEditorShow = [this]()
            {
                auto* editor = input.getCurrentTextEditor();
                
                editor->onTextChange = [this, editor]()
                {
                    auto width = input.getFont().getStringWidth(editor->getText()) + 25;
                    
                    if (width > box->getWidth())
                    {
                        box->setSize(width, box->getHeight());
                    }
                };
                
                editor->onFocusLost = [this]()
                {
                    auto width = input.getFont().getStringWidth(input.getText()) + 25;
                    if (width < box->getWidth())
                    {
                        box->setSize(width, box->getHeight());
                        box->constrainer.checkComponentBounds(box);
                    }
                };
            };
        }
        // symbolatom box behaviour
        else
        {
            /*
             input.onEditorShow = [this]()
             {
             auto* editor = input.getCurrentTextEditor();
             editor->onReturnKey = [this, editor]()
             {
             startEdition();
             gui.setSymbol(editor->getText().toStdString());
             stopEdition();
             // input.setText(String(gui.getSymbol()), dontSendNotification);
             };
             
             editor->onFocusLost = [this]()
             {
             auto width = input.getFont().getStringWidth(input.getText()) + 25;
             if (width < box->getWidth())
             {
             box->setSize(width, box->getHeight());
             box->constrainer.checkComponentBounds(box);
             }
             
             };
             }; */
        }
        
        initialise(newObject);
        
        box->addMouseListener(this, false);
        box->constrainer.setSizeLimits(50, Box::height - 2, maxSize, maxSize);
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }
    
    void resized() override
    {
        input.setBounds(getLocalBounds());
    }
    
    void update() override
    {
        input.setText(String(gui.getSymbol()), sendNotification);
    }
    
    void paint(Graphics& g) override
    {
        // Draw message style
        if (!getGui().isAtom())
        {
            auto baseColour = isDown ? Colour(90, 90, 90) : Colour(70, 70, 70);
            
            auto rect = getLocalBounds().toFloat();
            
            g.setGradientFill(ColourGradient::vertical(baseColour, baseColour.darker(1.5f), getLocalBounds()));
            
            g.fillRoundedRectangle(rect, 2.0f);
        }
        else
        {
            g.fillAll(findColour(ComboBox::backgroundColourId));
        }
    }
    
    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 1.5f);
    }
    
    void updateValue() override
    {
        if (!edited)
        {
            std::string const v = gui.getSymbol();
            
            if (lastMessage != v && !String(v).startsWith("click"))
            {
                numLines = 1;
                longestLine = 7;
                
                int currentLineLength = 0;
                for (auto& c : v)
                {
                    if (c == '\n')
                    {
                        numLines++;
                        longestLine = std::max(longestLine, currentLineLength);
                        currentLineLength = 0;
                    }
                    else
                    {
                        currentLineLength++;
                    }
                }
                if (numLines == 1) longestLine = std::max(longestLine, currentLineLength);
                
                lastMessage = v;
                
                update();
                // repaint();
            }
        }
    }
    
    std::pair<int, int> getBestSize() override
    {
        updateValue();  // make sure text is loaded
        
        // auto [x, y, w, h] = gui.getBounds();
        int stringLength = std::max(10, input.getFont().getStringWidth(input.getText()));
        return {stringLength + 20, numLines * 21};
    };
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!gui.isAtom() && isLocked)
        {
            isDown = true;
            repaint();
            
            startEdition();
            gui.click();
            stopEdition();
        }
        if (box->cnv->isSelected(box) && !box->selectionChanged)
        {
            shouldOpenEditor = true;
        }
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        isDown = false;
        
        // Edit messages when unlocked, edit atoms when locked
        if ((!isLocked && shouldOpenEditor && !e.mouseWasDraggedSinceMouseDown() && !gui.isAtom()) || (isLocked && gui.isAtom()))
        {
            input.showEditor();
            shouldOpenEditor = false;
        }
        
        repaint();
    }
    
    int numLines = 1;
    int longestLine = 7;
};

struct NumboxComponent : public GUIComponent
{
    Label input;
    
    float dragValue = 0.0f;
    bool shift = false;
    int decimalDrag = 0;
    
    Point<float> lastDragPos;
    
    NumboxComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            startEdition();
            
            if (!gui.isAtom())
            {
                editor->setBorder({0, 10, 0, 0});
            }
            
            if (editor != nullptr)
            {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };
        
        input.onEditorHide = [this]()
        {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };
        
        if (!gui.isAtom())
        {
            input.setBorderSize({1, 15, 1, 1});
        }
        addAndMakeVisible(input);
        
        input.setText(formatNumber(getValueOriginal()), dontSendNotification);
        
        initialise(newObject);
        input.setEditable(true, false);
        
        addMouseListener(this, true); // why do we need this??
        
        box->constrainer.setSizeLimits(50, Box::height - 2, 500, Box::height - 2);
    }
    
    void resized() override
    {
        input.setBounds(getLocalBounds());
    }
    
    String formatNumber(float value)
    {
        String text;
        text << value;
        if (!text.containsChar('.')) text << '.';
        return text;
    }
    
    void update() override
    {
        input.setText(formatNumber(getValueOriginal()), dontSendNotification);
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (input.isBeingEdited()) return;
        
        startEdition();
        shift = e.mods.isShiftDown();
        dragValue = input.getText().getFloatValue();
        
        lastDragPos = e.position;
        
        const auto textArea = input.getBorderSize().subtractedFrom(input.getBounds());
        
        GlyphArrangement glyphs;
        glyphs.addFittedText(input.getFont(), input.getText(), textArea.getX(), 0., textArea.getWidth(), getHeight(), Justification::centredLeft, 1, input.getMinimumHorizontalScale());
        
        double decimalX = getWidth();
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
        {
            auto const& glyph = glyphs.getGlyph(i);
            if (glyph.getCharacter() == '.')
            {
                decimalX = glyph.getRight();
            }
        }
        
        const bool isDraggingDecimal = e.x > decimalX;
        
        decimalDrag = isDraggingDecimal ? 6 : 0;
        
        if (isDraggingDecimal)
        {
            GlyphArrangement decimalsGlyph;
            static const String decimalStr("000000");
            
            decimalsGlyph.addFittedText(input.getFont(), decimalStr, decimalX, 0, getWidth(), getHeight(), Justification::centredLeft, 1, input.getMinimumHorizontalScale());
            
            for (int i = 0; i < decimalsGlyph.getNumGlyphs(); ++i)
            {
                auto const& glyph = decimalsGlyph.getGlyph(i);
                if (e.x <= glyph.getRight())
                {
                    decimalDrag = i + 1;
                    break;
                }
            }
        }
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if (input.isBeingEdited()) return;
        
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
        stopEdition();
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if (input.isBeingEdited()) return;
        
        setMouseCursor(MouseCursor::NoCursor);
        updateMouseCursor();
        
        const int decimal = decimalDrag + e.mods.isShiftDown();
        const float increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
        const float deltaY = e.y - lastDragPos.y;
        lastDragPos = e.position;
        
        dragValue += increment * -deltaY;
        
        // truncate value and set
        double newValue = dragValue;
        
        if (decimal > 0)
        {
            const int sign = (newValue > 0) ? 1 : -1;
            unsigned int ui_temp = (newValue * std::pow(10, decimal)) * sign;
            newValue = (((double)ui_temp) / std::pow(10, decimal) * sign);
        }
        else
        {
            newValue = static_cast<int64_t>(newValue);
        }
        
        setValueOriginal(newValue);
        
        input.setText(formatNumber(getValueOriginal()), NotificationType::dontSendNotification);
    }
    
    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tFloat, cGeneral, &min, {}}, {"Maximum", tFloat, cGeneral, &max, {}}};
    }
    
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        }
        if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(TextEditor::backgroundColourId));
        g.fillRect(getLocalBounds().reduced(1));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);
        
        if (gui.isAtom()) return;
        
        const int indent = 9;
        
        const Rectangle<int> iconBounds = getLocalBounds().withWidth(indent - 4).withHeight(getHeight() - 8).translated(4, 4);
        
        Path corner;
        
        corner.addTriangle(iconBounds.getTopLeft().toFloat(), iconBounds.getTopRight().toFloat() + Point<float>(0, (iconBounds.getHeight() / 2.)), iconBounds.getBottomLeft().toFloat());
        
        g.setColour(Colour(gui.getForegroundColour()));
        g.fillPath(corner);
    }
};

struct ListComponent : public GUIComponent, public Timer
{
    ListComponent(const pd::Gui& gui, Box* parent, bool newObject) : GUIComponent(gui, parent, newObject)
    {
        label.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        label.setMinimumHorizontalScale(1.f);
        label.setJustificationType(Justification::centredLeft);
        label.setBorderSize(BorderSize<int>(2, 6, 2, 2));
        label.setText(String(getValueOriginal()), dontSendNotification);
        label.setEditable(true, false);
        // label.setInterceptsMouseClicks(false, false);
        
        label.setColour(Label::textColourId, gui.getForegroundColour());
        setInterceptsMouseClicks(true, false);
        addAndMakeVisible(label);
        
        label.onEditorHide = [this]()
        {
            auto gui = getGui();
            auto array = StringArray();
            array.addTokens(label.getText(), true);
            std::vector<pd::Atom> list;
            list.reserve(array.size());
            for (auto const& elem : array)
            {
                if (elem.getCharPointer().isDigit())
                {
                    list.push_back({elem.getFloatValue()});
                }
                else
                {
                    list.push_back({elem.toStdString()});
                }
            }
            if (list != gui.getList())
            {
                startEdition();
                gui.setList(list);
                stopEdition();
                // label.setText(String(gui.getSymbol()), NotificationType::dontSendNotification);
            }
        };
        
        label.onEditorShow = [this]()
        {
            auto* editor = label.getCurrentTextEditor();
            if (editor != nullptr)
            {
                editor->setIndents(1, 2);
                editor->setBorder(BorderSize<int>(2, 6, 2, 2));
            }
        };
        
        updateValue();
        
        initialise(newObject);
        
        box->constrainer.setSizeLimits(100, Box::height - 2, maxSize, maxSize);
        startTimer(100);
    }
    
    ~ListComponent()
    {
        stopTimer();
    }
    
    void timerCallback() override
    {
        update();
    }
    
    void resized() override
    {
        label.setBounds(getLocalBounds());
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(Slider::thumbColourId));
        
        static auto const border = 1.0f;
        const auto h = static_cast<float>(getHeight());
        const auto w = static_cast<float>(getWidth());
        const auto o = h * 0.25f;
        Path p;
        p.startNewSubPath(0.5f, 0.5f);
        p.lineTo(0.5f, h - 0.5f);
        p.lineTo(w - o, h - 0.5f);
        p.lineTo(w - 0.5f, h - o);
        p.lineTo(w - 0.5f, o);
        p.lineTo(w - o, 0.5f);
        p.closeSubPath();
        
        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillPath(p);
        g.strokePath(p, PathStrokeType(border));
    }
    
    void update() override
    {
        if (!edited && !label.isBeingEdited())
        {
            auto const array = gui.getList();
            String message;
            for (auto const& atom : array)
            {
                if (message.isNotEmpty())
                {
                    message += " ";
                }
                if (atom.isFloat())
                {
                    message += String(atom.getFloat());
                }
                else if (atom.isSymbol())
                {
                    message += String(atom.getSymbol());
                }
            }
            label.setText(message, NotificationType::dontSendNotification);
        }
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
private:
    Label label;
};

struct SliderComponent : public GUIComponent
{
    bool isVertical;
    Value isLogarithmic = Value(var(false));
    
    Slider slider;
    
    SliderComponent(bool vertical, const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        isVertical = vertical;
        addAndMakeVisible(slider);
        
        isLogarithmic = gui.isLogScale();
        
        if (vertical) slider.setSliderStyle(Slider::LinearBarVertical);
        else          slider.setSliderStyle(Slider::LinearBar);
        
        slider.setRange(0., 1., 0.001);
        slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        slider.setScrollWheelEnabled(false);
        
        slider.setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
        
        slider.setValue(getValueScaled());
        
        slider.onDragStart = [this]() { startEdition(); };
        
        slider.onValueChange = [this]()
        {
            const float val = slider.getValue();
            if (gui.isLogScale())
            {
                float minValue = static_cast<float>(min.getValue());
                float maxValue = static_cast<float>(max.getValue());
                float minimum = minValue == 0.0f ? std::numeric_limits<float>::epsilon() : minValue;
                setValueOriginal(exp(val * log(maxValue / minimum)) * minimum);
            }
            else
            {
                setValueScaled(val);
            }
        };
        
        slider.onDragEnd = [this]() { stopEdition(); };
        
        initialise(newObject);
        
        if (isVertical)
        {
            box->constrainer.setSizeLimits(15, 77, maxSize, maxSize);
        }
        else
        {
            box->constrainer.setSizeLimits(50, 15, maxSize, maxSize);
        }
    }
    
    void resized() override
    {
        slider.setBounds(getLocalBounds());
    }
    
    void update() override
    {
        slider.setValue(getValueScaled(), dontSendNotification);
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    ObjectParameters defineParameters() override
    {
        return {
            {"Minimum", tFloat, cGeneral, &min, {}},
            {"Maximum", tFloat, cGeneral, &max, {}},
            {"Logarithmic", tBool, cGeneral, &isLogarithmic, {"off", "on"}},
        };
    }
    
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
        }
        else if (value.refersToSameSourceAs(isLogarithmic))
        {
            gui.setLogScale(isLogarithmic == var(true));
            min = gui.getMinimum();
            max = gui.getMaximum();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};

struct RadioComponent : public GUIComponent
{
    int lastState = 0;
    
    Value numButtons = Value(8.0f);
    
    bool isVertical;
    
    RadioComponent(bool vertical, const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        isVertical = vertical;
        
        initialise(newObject);
        updateRange();
        
        numButtons.addListener(this);
        
        int selected = getValueOriginal();
        
        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
        if (isVertical)
        {
            box->constrainer.setSizeLimits(25, 90, maxSize, maxSize);
        }
        else
        {
            box->constrainer.setSizeLimits(100, 25, maxSize, maxSize);
        }
    }
    
    template<typename T>
    T roundMultiple( T value, T multiple )
    {
        if (multiple == 0) return value;
        return static_cast<T>(std::round(static_cast<double>(value)/static_cast<double>(multiple))*static_cast<double>(multiple));
    }

    
    void resized() override
    {
        int size = isVertical ? getHeight() / radioButtons.size() : getWidth() / radioButtons.size();
        
        for(int i = 0; i < radioButtons.size(); i++) {
            if(isVertical) radioButtons[i]->setBounds(0, i * size, size, size);
            else           radioButtons[i]->setBounds(i * size, 0, size, size);
        }
                
        // Fix aspect ratio and
        if(isVertical) {
            if(getWidth() - Box::doubleMargin != size) {
                box->setSize(size + Box::doubleMargin, box->getHeight());
            }
        }
        else
        {
            if(getHeight() - Box::doubleMargin != size) {
                box->setSize(box->getWidth(), size + Box::doubleMargin);
            }
        }
        
    }
    
    void update() override
    {
        int selected = getValueOriginal() - 1;
        
        if (selected < radioButtons.size())
        {
            radioButtons[selected]->setToggleState(true, dontSendNotification);
        }
    }
    
    void updateRange()
    {
        numButtons = gui.getMaximum();
        
        radioButtons.clear();
        
        for (int i = 0; i < numButtons; i++)
        {
            radioButtons.add(new TextButton);
            radioButtons[i]->setConnectedEdges(12);
            radioButtons[i]->setRadioGroupId(1001);
            radioButtons[i]->setClickingTogglesState(true);
            radioButtons[i]->setName("radiobutton");
            addAndMakeVisible(radioButtons[i]);
            
            radioButtons[i]->onClick = [this, i]() mutable
            {
                if (!radioButtons[i]->getToggleState()) return;
                
                lastState = i;
                setValueOriginal(i);
            };
        }
        
        radioButtons[((int)gui.getValue() - 1)]->setToggleState(true, dontSendNotification);
        
        resized();
        //box->updateBounds(false);
    }
    
    OwnedArray<TextButton> radioButtons;
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    ObjectParameters defineParameters() override
    {
        return {{"Options", tInt, cGeneral, &numButtons, {}}};
    }
    
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(numButtons))
        {
            gui.setMaximum(static_cast<int>(numButtons.getValue()));
            updateRange();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};

struct GraphicalArray : public Component, public Timer
{
public:
    GraphicalArray(PlugDataAudioProcessor* instance, pd::Array& graph) : array(graph), edited(false), pd(instance)
    {
        if (graph.getName().empty()) return;
        
        vec.reserve(8192);
        temp.reserve(8192);
        try
        {
            array.read(vec);
        }
        catch (...)
        {
            error = true;
        }
        startTimer(100);
        setInterceptsMouseClicks(true, false);
        setOpaque(false);
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(TextButton::buttonColourId));
        
        if (error)
        {
            g.drawText("array " + array.getName() + " is invalid", 0, 0, getWidth(), getHeight(), Justification::centred);
        }
        else
        {
            const auto h = static_cast<float>(getHeight());
            const auto w = static_cast<float>(getWidth());
            if (!vec.empty())
            {
                const std::array<float, 2> scale = array.getScale();
                if (array.isDrawingCurve())
                {
                    const float dh = h / (scale[1] - scale[0]);
                    const float dw = w / static_cast<float>(vec.size() - 1);
                    Path p;
                    p.startNewSubPath(0, h - (std::clamp(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                    for (size_t i = 1; i < vec.size() - 1; i += 2)
                    {
                        const float y1 = h - (std::clamp(vec[i - 1], scale[0], scale[1]) - scale[0]) * dh;
                        const float y2 = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                        const float y3 = h - (std::clamp(vec[i + 1], scale[0], scale[1]) - scale[0]) * dh;
                        p.cubicTo(static_cast<float>(i - 1) * dw, y1, static_cast<float>(i) * dw, y2, static_cast<float>(i + 1) * dw, y3);
                    }
                    g.setColour(findColour(ComboBox::outlineColourId));
                    g.strokePath(p, PathStrokeType(1));
                }
                else if (array.isDrawingLine())
                {
                    const float dh = h / (scale[1] - scale[0]);
                    const float dw = w / static_cast<float>(vec.size() - 1);
                    Path p;
                    p.startNewSubPath(0, h - (std::clamp(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                    for (size_t i = 1; i < vec.size(); ++i)
                    {
                        const float y = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
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
                    for (size_t i = 0; i < vec.size(); ++i)
                    {
                        const float y = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                        g.drawLine(static_cast<float>(i) * dw, y, static_cast<float>(i + 1) * dw, y);
                    }
                }
            }
        }
        
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        if (error) return;
        edited = true;
        
        const auto s = static_cast<float>(vec.size() - 1);
        const auto w = static_cast<float>(getWidth());
        const auto x = static_cast<float>(e.x);
        
        const std::array<float, 2> scale = array.getScale();
        lastIndex = static_cast<size_t>(std::round(std::clamp(x / w, 0.f, 1.f) * s));
        
        mouseDrag(e);
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if (error) return;
        const auto s = static_cast<float>(vec.size() - 1);
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());
        const auto x = static_cast<float>(e.x);
        const auto y = static_cast<float>(e.y);
        
        const std::array<float, 2> scale = array.getScale();
        const int index = static_cast<int>(std::round(std::clamp(x / w, 0.f, 1.f) * s));
        
        float start = vec[lastIndex];
        float current = (1.f - std::clamp(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];
        
        int interpStart = std::min(index, lastIndex);
        int interpEnd = std::max(index, lastIndex);
        
        float min = index == interpStart ? current : start;
        float max = index == interpStart ? start : current;
        
        const CriticalSection* cs = pd->getCallbackLock();
        
        // Fix to make sure we don't leave any gaps while dragging
        for (int n = interpStart; n <= interpEnd; n++)
        {
            vec[n] = jmap<float>(n, interpStart, interpEnd + 1, min, max);
        }
        
        // Don't want to touch vec on the other thread, so we copy the vector into the lambda
        auto changed = std::vector<float>(vec.begin() + interpStart, vec.begin() + interpEnd);
        
        pd->enqueueFunction(
                            [this, interpStart, changed]() mutable
                            {
                                try
                                {
                                    for (int n = 0; n < changed.size(); n++)
                                    {
                                        array.write(interpStart + n, changed[n]);
                                    }
                                }
                                catch (...)
                                {
                                    error = true;
                                }
                            });
        
        lastIndex = index;
        
        pd->enqueueMessages(stringArray, array.getName(), {});
        repaint();
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if (error) return;
        edited = false;
    }
    
    void timerCallback() override
    {
        if (!edited)
        {
            error = false;
            try
            {
                array.read(temp);
            }
            catch (...)
            {
                error = true;
            }
            if (temp != vec)
            {
                vec.swap(temp);
                repaint();
            }
        }
    }
    
    size_t getArraySize() const noexcept
    {
        return vec.size();
    }
    
    pd::Array array;
    std::vector<float> vec;
    std::vector<float> temp;
    std::atomic<bool> edited;
    bool error = false;
    const std::string stringArray = std::string("array");
    
    int lastIndex = 0;
    
    PlugDataAudioProcessor* pd;
};

struct ArrayComponent : public GUIComponent
{
public:
    // Array component
    ArrayComponent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), graph(gui.getArray()), array(box->cnv->pd, graph)
    {
        setInterceptsMouseClicks(false, true);
        array.setBounds(getLocalBounds());
        addAndMakeVisible(&array);
        
        initialise(newObject);
        box->constrainer.setSizeLimits(100, 40, maxSize, maxSize);
    }
    
    void resized() override
    {
        array.setBounds(getLocalBounds());
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
private:
    pd::Array graph;
    GraphicalArray array;
};

struct GraphOnParent : public GUIComponent
{
    bool isLocked = false;
    
public:
    // Graph On Parent
    GraphOnParent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), subpatch(gui.getPatch())
    {
        setInterceptsMouseClicks(box->locked == var(false), true);
        
        updateCanvas();
        
        initialise(newObject);
        
        addMouseListener(this, true);
        
        box->constrainer.setSizeLimits(25, 25, maxSize, maxSize);
        
        resized();
    }
    
    ~GraphOnParent() override
    {
        closeOpenedSubpatchers();
    }
    
    void resized() override
    {
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, true);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!isLocked)
        {
            box->mouseDown(e.getEventRelativeTo(box));
        }
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if (!isLocked)
        {
            box->mouseDrag(e.getEventRelativeTo(box));
        }
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if (!isLocked)
        {
            box->mouseUp(e.getEventRelativeTo(box));
        }
    }
    
    void updateCanvas()
    {
        if (!canvas)
        {
            canvas = std::make_unique<Canvas>(box->cnv->main, subpatch, true);
            addAndMakeVisible(canvas.get());
            
            // Make sure that the graph doesn't become the current canvas
            box->cnv->patch.setCurrent(true);
            box->cnv->main.updateCommandStatus();
        }
        
        auto b = getPatch()->getBounds();
        // canvas->checkBounds();
        canvas->setBounds(-b.getX(), -b.getY(), b.getWidth() + b.getX(), b.getHeight() + b.getY());
    }
    
    void updateValue() override
    {
        updateCanvas();
        
        if (!canvas) return;
        
        for (auto& box : canvas->boxes)
        {
            if (box->graphics)
            {
                box->graphics->updateValue();
            }
        }
    }
    
    // override to make transparent
    void paint(Graphics& g) override
    {
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    Canvas* getCanvas() override
    {
        return canvas.get();
    }
    
private:
    pd::Patch subpatch;
    std::unique_ptr<Canvas> canvas;
};

struct Subpatch : public GUIComponent
{
    Subpatch(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), subpatch(gui.getPatch())
    {
    }
    
    void updateValue() override
    {
        // Pd sometimes sets the isgraph flag too late...
        // In that case we tell the box to create the gui
        if (static_cast<t_canvas*>(gui.getPointer())->gl_isgraph)
        {
            box->setType(box->currentText, true);
        }
    };
    
    ~Subpatch()
    {
        closeOpenedSubpatchers();
    }
    
    std::pair<int, int> getBestSize() override
    {
        return {0, Box::height};
    };
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    bool fakeGui() override
    {
        return true;
    }
    
private:
    pd::Patch subpatch;
};

struct CommentComponent : public GUIComponent
{
    CommentComponent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject)
    {
        setInterceptsMouseClicks(false, false);
        setVisible(false);
        
        box->setEditable(true);
        box->hideLabel = false;
    }
    
    void paint(Graphics& g) override
    {
    }
    
    std::pair<int, int> getBestSize() override
    {
        return {120, 4};
    };
    
    bool fakeGui() override
    {
        return true;
    }
};

struct VUMeter : public GUIComponent
{
    VUMeter(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        initialise(newObject);
        
        box->constrainer.setSizeLimits(30, 80, maxSize, maxSize);
    }
    
    void resized() override
    {
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(ComboBox::backgroundColourId));
        
        auto values = std::vector<float>{gui.getValue(), gui.getPeak()};
        
        
        int height = getHeight();
        int width = getWidth();
        
        auto outerBorderWidth = 2.0f;
        auto totalBlocks = 15;
        auto spacingFraction = 0.03f;
        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;
        
        auto blockHeight = (height - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockWidth = width - doubleOuterBorderWidth;
        auto blockRectHeight = (1.0f - 2.0f * spacingFraction) * blockHeight;
        auto blockRectSpacing = spacingFraction * blockHeight;
        auto blockCornerSize = 0.1f * blockHeight;
        auto c = Colour(0xff42a2c8);
        
        float lvl = (float)std::exp(std::log(values[1]) / 3.0) * (values[1] > 0.002);
        auto numBlocks = roundToInt(totalBlocks * lvl);
        
        auto verticalGradient = ColourGradient (c, 0, getHeight(), Colours::red, 0, 0, false);
        verticalGradient.addColour(0.5f, c);
        verticalGradient.addColour(0.75f, Colours::orange);
    
        for (auto i = 0; i < totalBlocks; ++i)
        {
            if (i >= numBlocks)
                g.setColour(Colours::darkgrey);
            else
                g.setGradientFill (verticalGradient);
            
            g.fillRoundedRectangle(outerBorderWidth, outerBorderWidth + ((totalBlocks - i) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight, blockCornerSize);
        }
        
        float lvl2 = (float)std::exp(std::log(values[0]) / 3.0) * (values[0] > 0.002);
        auto numBlocks2 = roundToInt(totalBlocks * lvl2);
        
        g.setColour(Colours::white);
        g.fillRoundedRectangle(outerBorderWidth, outerBorderWidth + ((totalBlocks - numBlocks2) * blockHeight) + blockRectSpacing, blockWidth, blockRectHeight / 2.0f, blockCornerSize);
        
        String textValue = String(values[0], 2);
        
        if(getWidth() > g.getCurrentFont().getStringWidth(textValue + " dB")) {
            // Check noscale flag, otherwise display next to slider
            g.setColour(Colours::white);
            g.drawFittedText(textValue + " dB", Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
        }
        else if(getWidth() > g.getCurrentFont().getStringWidth(textValue)) {
            g.setColour(Colours::white);
            g.drawFittedText(textValue, Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
        }
        
    }
    
    void updateValue() override
    {
        auto rms = gui.getValue();
        auto peak = gui.getPeak();
        repaint();
    };
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
};

struct PanelComponent : public GUIComponent
{
    PanelComponent(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        box->constrainer.setSizeLimits(20, 20, maxSize, maxSize);
        
        box->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        initialise(newObject);
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
    }
    
    void updateValue() override{};
    
    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Colour", tColour, cLabel, &labelColour, {}});
        params.push_back({"Label X", tInt, cLabel, &labelX, {}});
        params.push_back({"Label Y", tInt, cLabel, &labelY, {}});
        params.push_back({"Label Height", tInt, cLabel, &labelHeight, {}});
        return params;
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
};

// ELSE mousepad
struct MousePad : public GUIComponent
{
    bool isLocked = false;
    bool isPressed = false;
    
    typedef struct _pad
    {
        t_object x_obj;
        t_glist* x_glist;
        void* x_proxy;  // dont have this object and dont need it
        t_symbol* x_bindname;
        int x_x;
        int x_y;
        int x_w;
        int x_h;
        int x_sel;
        int x_zoom;
        int x_edit;
        unsigned char x_color[3];
    } t_pad;
    
    MousePad(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
        
        // setInterceptsMouseClicks(box->locked, box->locked);
        
        addMouseListener(box, false);
    }
    
    ~MousePad()
    {
        removeMouseListener(box);
        Desktop::getInstance().removeGlobalMouseListener(this);
    }
    
    void paint(Graphics& g) override{
        
    };
    
    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!getScreenBounds().contains(e.getScreenPosition()) || !isLocked) return;
        
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
        
        isPressed = true;
        
        // glist_grab(x->x_glist, &x->x_obj.te_g, (t_glistmotionfn)pad_motion, 0, (float)xpix, (float)ypix);
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        mouseMove(e);
    }
    
    void mouseMove(const MouseEvent& e) override
    {
        if (!getScreenBounds().contains(e.getScreenPosition()) || !isLocked) return;
        
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
    
    void mouseUp(const MouseEvent& e) override
    {
        if (!getScreenBounds().contains(e.getScreenPosition()) && !isPressed) return;
        
        auto* x = static_cast<t_pad*>(gui.getPointer());
        t_atom at[1];
        SETFLOAT(at, 0);
        outlet_anything(x->x_obj.ob_outlet, gensym("click"), 1, at);
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
    }
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
};

// Else "mouse" component
struct MouseComponent : public GUIComponent
{
    typedef struct _mouse
    {
        t_object x_obj;
        int x_hzero;
        int x_vzero;
        int x_zero;
        int x_wx;
        int x_wy;
        t_glist* x_glist;
        t_outlet* x_horizontal;
        t_outlet* x_vertical;
    } t_mouse;
    
    MouseComponent(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }
    
    ~MouseComponent()
    {
        Desktop::getInstance().removeGlobalMouseListener(this);
    }
    
    void mouseMove(const MouseEvent& e) override
    {
        // Do this with a mouselistener?
        auto pos = Desktop::getInstance().getMousePosition();
        
        if (Desktop::getInstance().getMouseSource(0)->isDragging())
        {
            t_atom args[1];
            SETFLOAT(args, 0);
            
            pd_typedmess((t_pd*)gui.getPointer(), gensym("_up"), 1, args);
        }
        else
        {
            t_atom args[1];
            SETFLOAT(args, 1);
            
            pd_typedmess((t_pd*)gui.getPointer(), gensym("_up"), 1, args);
        }
        
        t_atom args[2];
        SETFLOAT(args, pos.x);
        SETFLOAT(args + 1, pos.y);
        
        pd_typedmess((t_pd*)gui.getPointer(), gensym("_getscreen"), 2, args);
    }
    
    std::pair<int, int> getBestSize() override
    {
        return {0, 3};
    };
    
    bool fakeGui() override
    {
        return true;
    }
};

// ELSE keyboard
struct KeyboardComponent : public GUIComponent, public MidiKeyboardStateListener
{
    typedef struct _edit_proxy
    {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _keyboard* p_cnv;
    } t_edit_proxy;
    
    typedef struct _keyboard
    {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int* x_tgl_notes;  // to store which notes should be played
        int x_velocity;    // to store velocity
        int x_last_note;   // to store last note
        float x_vel_in;    // to store the second inlet values
        float x_space;
        int x_width;
        int x_height;
        int x_octaves;
        int x_semitones;
        int x_first_c;
        int x_low_c;
        int x_toggle_mode;
        int x_norm;
        int x_zoom;
        int x_shift;
        int x_xpos;
        int x_ypos;
        int x_snd_set;
        int x_rcv_set;
        int x_flag;
        int x_s_flag;
        int x_r_flag;
        int x_edit;
        t_symbol* x_receive;
        t_symbol* x_rcv_raw;
        t_symbol* x_send;
        t_symbol* x_snd_raw;
        t_symbol* x_bindsym;
        t_outlet* x_out;
    } t_keyboard;
    
    KeyboardComponent(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject), keyboard(state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setAvailableRange(36, 83);
        keyboard.setScrollButtonsVisible(false);
        
        state.addListener(this);
        
        addAndMakeVisible(keyboard);
        


        auto* x = (t_keyboard*)gui.getPointer();
        x->x_width = width * 0.595f;
        
        rangeMin = x->x_low_c;
        rangeMax = x->x_low_c + (x->x_octaves * 12) + x->x_semitones;
        
        if(static_cast<int>(rangeMax.getValue()) == 0) {
            rangeMin = 36;
            rangeMax = 86;
        }
        
        initialise(newObject);
        box->constrainer.setSizeLimits(50, 70, maxSize, maxSize);
        
        //int width = keyboard.getKeyPosition(rangeMax.getValue(), keyboard.getKeyWidth()).getEnd();
        //box->constrainer.setFixedAspectRatio(width / 150);
    }
    
    void resized() override
    {
        /*
        
            
            keyboard.setKeyWidth(getWidth() / (numKeys * 0.595f));
            int width = keyboard.getKeyPosition(rangeMax.getValue(), keyboard.getKeyWidth()).getEnd();
        } */
        int numKeys = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
        float ratio = numKeys / 9.55f;
        
        auto* keyboardObject = static_cast<t_keyboard*>(gui.getPointer());
        
        if(box->getWidth() / ratio != box->getHeight()) {
            box->setSize(box->getHeight() * ratio, box->getHeight());
            
            if(getWidth() > 0) {
                keyboard.setKeyWidth(getWidth() / (numKeys * 0.597f));
                keyboardObject->x_width = getWidth();
            }
        }
        
        
        //keyboard.setBounds(Rectangle<int>(0, 0, width, width / 10.0f));
        keyboard.setBounds(getLocalBounds());
    }
    
    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)gui.getPointer();
        
        box->cnv->pd->enqueueFunction(
                                      [x, note, velocity]() mutable
                                      {
                                          int ac = 2;
                                          t_atom at[2];
                                          SETFLOAT(at, note);
                                          SETFLOAT(at + 1, velocity * 127);
                                          
                                          outlet_list(x->x_out, &s_list, ac, at);
                                          if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
                                      });
    }
    
    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)gui.getPointer();
        
        box->cnv->pd->enqueueFunction(
                                      [x, note]() mutable
                                      {
                                          int ac = 2;
                                          t_atom at[2];
                                          SETFLOAT(at, note);
                                          SETFLOAT(at + 1, 0);
                                          
                                          outlet_list(x->x_out, &s_list, ac, at);
                                          if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
                                      });
    };
    
    std::pair<int, int> getBestSize() override
    {
        auto b = gui.getBounds();
        return {b.getWidth(), b.getHeight()};
    };
    
    
     ObjectParameters defineParameters() override
     {
     return {{"Lowest note", tInt, cGeneral, &rangeMin, {}}, {"Highest note", tInt, cGeneral, &rangeMax, {}}};
     };
    
    
    void valueChanged(Value& value) override
    {
        auto* keyboardObject = static_cast<t_keyboard*>(gui.getPointer());
        
        if (value.refersToSameSourceAs(rangeMin))
        {
            rangeMin = std::clamp<int>(rangeMin.getValue(), 0, 127);
            
            keyboardObject->x_low_c = static_cast<int>(rangeMin.getValue());
            
            int range = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
            keyboardObject->x_octaves = range / 12;
            keyboardObject->x_semitones = range % 12;
            
            keyboard.setAvailableRange(rangeMin.getValue(), rangeMax.getValue());
            resized();
        }
        else if (value.refersToSameSourceAs(rangeMax))
        {
            
            rangeMax = std::clamp<int>(rangeMax.getValue(), 0, 127);
            /*
             static_cast<t_keyboard*>(gui.getPointer())->x_octaves = static_cast<int>(value.getValue()); */
            keyboard.setAvailableRange(rangeMin.getValue(), rangeMax.getValue());
            
            int range = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
            keyboardObject->x_octaves = range / 12;
            keyboardObject->x_semitones = range % 12;
            
            resized();
        }
    }
    
    Value rangeMin;
    Value rangeMax;
    
    MidiKeyboardState state;
    MidiKeyboardComponent keyboard;
};

GUIComponent* GUIComponent::createGui(const String& name, Box* parent, bool newObject)
{
    auto* guiPtr = dynamic_cast<pd::Gui*>(parent->pdObject.get());
    
    if (!guiPtr) return nullptr;
    
    auto& gui = *guiPtr;
    
    if (gui.getType() == pd::Type::Bang)
    {
        return new BangComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Toggle)
    {
        return new ToggleComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::HorizontalSlider)
    {
        return new SliderComponent(false, gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::VerticalSlider)
    {
        return new SliderComponent(true, gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::HorizontalRadio)
    {
        return new RadioComponent(false, gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::VerticalRadio)
    {
        return new RadioComponent(true, gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Message)
    {
        return new MessageComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Number)
    {
        return new NumboxComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::AtomList)
    {
        return new ListComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Array)
    {
        return new ArrayComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::GraphOnParent)
    {
        return new GraphOnParent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Subpatch)
    {
        return new Subpatch(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::VuMeter)
    {
        return new VUMeter(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Panel)
    {
        return new PanelComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Comment)
    {
        return new CommentComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::AtomNumber)
    {
        return new NumboxComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::AtomSymbol)
    {
        return new MessageComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Mousepad)
    {
        return new MousePad(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Mouse)
    {
        return new MouseComponent(gui, parent, newObject);
    }
    if (gui.getType() == pd::Type::Keyboard)
    {
        return new KeyboardComponent(gui, parent, newObject);
    }
    
    return nullptr;
}
