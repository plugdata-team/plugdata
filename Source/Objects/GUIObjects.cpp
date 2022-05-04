/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
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
#include "Pd/PdPatch.h"

#include "ToggleComponent.h"
#include "MessageComponent.h"
#include "MouseComponent.h"
#include "BangComponent.h"
#include "RadioComponent.h"
#include "SliderComponent.h"
#include "ArrayComponent.h"
#include "GraphOnParent.h"
#include "KeyboardComponent.h"
#include "MousePad.h"
#include "NumberComponent.h"
#include "PanelComponent.h"
#include "PictureComponent.h"
#include "VUMeter.h"
#include "ListComponent.h"
#include "SubpatchComponent.h"
#include "CommentComponent.h"

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
        auto rect = gui.getLabelBounds(Rectangle<int>());
        labelX = rect.getX();
        labelY = rect.getY();
        labelHeight = rect.getHeight();
    }
    else if (gui.isAtom())
    {
        labelX = static_cast<int>(static_cast<t_fake_gatom*>(gui.getPointer())->a_wherelabel + 1);

        int h = gui.getFontHeight();

        int idx = static_cast<int>(std::find(atomSizes, atomSizes + 7, h) - atomSizes);
        labelHeight = idx + 1;
    }

    box->addComponentListener(this);
    updateLabel();

    sendSymbol = gui.getSendSymbol();
    receiveSymbol = gui.getReceiveSymbol();

    setWantsKeyboardFocus(true);

    setLookAndFeel(dynamic_cast<PlugDataLook*>(&LookAndFeel::getDefaultLookAndFeel())->getPdLook());
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
    labelText = gui.getLabelText();

    if (gui.isIEM())
    {
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
    else
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(Label::textWhenEditingColourId));
        getLookAndFeel().setColour(Label::textColourId, box->findColour(Label::textColourId));
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
    getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(PlugDataColour::textColourId));

    if (gui.isIEM())
    {
        g.setColour(findColour(TextButton::buttonColourId));
    }
    else
    {
        // make sure text is readable
        getLookAndFeel().setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));
        getLookAndFeel().setColour(TextEditor::textColourId, box->findColour(PlugDataColour::textColourId));
        g.setColour(box->findColour(PlugDataColour::canvasColourId));
    }

    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
}

void GUIComponent::paintOverChildren(Graphics& g)
{
    if (gui.isAtom())
    {
        g.setColour(box->findColour(PlugDataColour::highlightColourId));
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
        params.push_back({"Height", tCombo, cGeneral, &labelHeight, {"auto", "8", "10", "12", "16", "24", "36"}});

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
        if (gui.isIEM())
        {
            gui.setFontHeight(static_cast<int>(labelHeight.getValue()));
        }

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

bool GUIComponent::noGui()
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

    if (!resized) return;

    if (recursiveResize)
    {
        recursiveResize = false;
        return;  // break out of recursion: but doesn't protect against async recursion!!
    }
    else
    {
        recursiveResize = true;
        checkBoxBounds();
        recursiveResize = false;
    }
}

void GUIComponent::updateLabel()
{
    if (gui.isAtom())
    {
        int idx = std::clamp<int>(labelHeight.getValue(), 1, 7);
        gui.setFontHeight(atomSizes[idx - 1]);
    }

    int fontHeight = gui.getFontHeight();

    if (fontHeight == 0)
    {
        fontHeight = glist_getfont(box->cnv->patch.getPointer());
    }
    if (gui.isAtom()) fontHeight += 2;

    const String text = gui.getLabelText();

    if (text.isNotEmpty())
    {
        if (!label)
        {
            label = std::make_unique<Label>();
        }

        auto bounds = gui.getLabelBounds(box->getBounds().reduced(Box::margin));

        if (gui.isIEM())
        {
            bounds.translate(0, fontHeight / -2.0f);
        }

        label->setFont(Font(fontHeight));
        label->setJustificationType(Justification::centredLeft);
        label->setBounds(bounds);
        label->setBorderSize(BorderSize<int>(0, 0, 0, 0));
        label->setMinimumHorizontalScale(1.f);
        label->setText(text, dontSendNotification);
        label->setEditable(false, false);
        label->setInterceptsMouseClicks(false, false);

        if (gui.isIEM())
        {
            label->setColour(Label::textColourId, gui.getLabelColour());
        }
        else
        {
            label->setColour(Label::textColourId, box->findColour(PlugDataColour::textColourId));
        }

        box->cnv->addAndMakeVisible(label.get());
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
            auto* deleted_patch = &cnv->patch;
            main.canvases.removeObject(cnv);
            tabbar->removeTab(n);
            main.pd.patches.removeObject(deleted_patch, false);
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
        return new NumberComponent(gui, parent, newObject);
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
        return new NumberComponent(gui, parent, newObject);
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
    if (gui.getType() == pd::Type::Picture)
    {
        return new PictureComponent(gui, parent, newObject);
    }

    return nullptr;
}
