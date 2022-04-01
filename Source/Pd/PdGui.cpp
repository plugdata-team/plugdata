/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdGui.h"

#include <cmath>
#include <limits>

#include "PdInstance.h"

extern "C"
{
#include <m_pd.h>
#include <g_all_guis.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <z_libpd.h>

#include "x_libpd_extra_utils.h"

    void my_numbox_calc_fontwidth(t_my_numbox* x);
}

namespace pd
{

typedef struct _messresponder
{
    t_pd mr_pd;
    t_outlet* mr_outlet;
} t_messresponder;

typedef struct _message
{
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist* m_glist;
    t_clock* m_clock;
} t_message;

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

static t_atom* fake_gatom_getatom(t_fake_gatom* x)
{
    int ac = binbuf_getnatom(x->a_text.te_binbuf);
    t_atom* av = binbuf_getvec(x->a_text.te_binbuf);
    if (x->a_flavor == A_FLOAT && (ac != 1 || av[0].a_type != A_FLOAT))
    {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "f", 0.);
    }
    else if (x->a_flavor == A_SYMBOL && (ac != 1 || av[0].a_type != A_SYMBOL))
    {
        binbuf_clear(x->a_text.te_binbuf);
        binbuf_addv(x->a_text.te_binbuf, "s", &s_);
    }
    return (binbuf_getvec(x->a_text.te_binbuf));
}

static int glist_getindex(t_glist* x, t_gobj* y)
{
    t_gobj* y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next) indx++;
    return (indx);
}

Gui::Gui(void* ptr, Patch* patch, Instance* instance) noexcept : Object(ptr, patch, instance), type(Type::Undefined)
{
    type = getType(ptr);
}

Type Gui::getType(void* ptr) noexcept
{
    auto type = Type::Undefined;

    const std::string name = libpd_get_object_class_name(ptr);
    if (name == "bng")
    {
        type = Type::Bang;
    }
    else if (name == "hsl")
    {
        type = Type::HorizontalSlider;
    }
    else if (name == "vsl")
    {
        type = Type::VerticalSlider;
    }
    else if (name == "tgl")
    {
        type = Type::Toggle;
    }
    else if (name == "nbx")
    {
        type = Type::Number;
    }
    else if (name == "vradio")
    {
        type = Type::VerticalRadio;
    }
    else if (name == "hradio")
    {
        type = Type::HorizontalRadio;
    }
    else if (name == "cnv")
    {
        type = Type::Panel;
    }
    else if (name == "vu")
    {
        type = Type::VuMeter;
    }
    else if (name == "text")
    {
        auto* textObj = static_cast<t_text*>(ptr);
        if (textObj->te_type == T_OBJECT)
        {
            type = Type::Invalid;
        }
        else
        {
            type = Type::Comment;
        }
    }
    else if (name == "message")
    {
        type = Type::Message;
    }
    else if (name == "pad")
    {
        type = Type::Mousepad;
    }
    else if (name == "mouse")
    {
        type = Type::Mouse;
    }
    else if (name == "keyboard")
    {
        type = Type::Keyboard;
    }

    else if (name == "gatom")
    {
        if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_FLOAT)
            type = Type::AtomNumber;
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_SYMBOL)
            type = Type::AtomSymbol;
        else if (static_cast<t_fake_gatom*>(ptr)->a_flavor == A_NULL)
            type = Type::AtomList;
    }

    else if (name == "canvas" || name == "graph")
    {
        if (static_cast<t_canvas*>(ptr)->gl_list)
        {
            t_class* c = static_cast<t_canvas*>(ptr)->gl_list->g_pd;
            if (c && c->c_name && (std::string(c->c_name->s_name) == std::string("array")))
            {
                type = Type::Array;
            }
            else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
            {
                type = Type::GraphOnParent;
            }
            else
            {  // abstraction or subpatch
                type = Type::Subpatch;
            }
        }
        else if (static_cast<t_canvas*>(ptr)->gl_isgraph)
        {
            type = Type::GraphOnParent;
        }
        else
        {
            type = Type::Subpatch;
        }
    }
    else if (name == "pd")
    {
        type = Type::Subpatch;
    }

    return type;
}

size_t Gui::getNumberOfSteps() const noexcept
{
    if (!ptr) return 0.f;
    if (type == Type::Toggle)
    {
        return 2;
    }
    else if (type == Type::HorizontalRadio)
    {
        return (static_cast<t_hdial*>(ptr))->x_number - 1;
    }
    else if (type == Type::VerticalRadio)
    {
        return (static_cast<t_vdial*>(ptr))->x_number;
    }
    else if (type == Type::AtomNumber)
    {
        return static_cast<t_text*>(ptr)->te_width == 1;
    }
    return 0.f;
}

float Gui::getMinimum() const noexcept
{
    if (!ptr) return 0.f;
    if (type == Type::HorizontalSlider)
    {
        return (static_cast<t_hslider*>(ptr))->x_min;
    }
    else if (type == Type::VerticalSlider)
    {
        return (static_cast<t_vslider*>(ptr))->x_min;
    }
    else if (type == Type::Number)
    {
        return (static_cast<t_my_numbox*>(ptr))->x_min;
    }
    else if (type == Type::AtomNumber)
    {
        auto const* gatom = static_cast<t_fake_gatom const*>(ptr);
        if (std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() && std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
        {
            return gatom->a_draglo;
        }
        return -std::numeric_limits<float>::max();
    }
    return 0.f;
}

float Gui::getMaximum() const noexcept
{
    if (!ptr) return 1.f;
    if (type == Type::HorizontalSlider)
    {
        return (static_cast<t_hslider*>(ptr))->x_max;
    }
    else if (type == Type::VerticalSlider)
    {
        return (static_cast<t_vslider*>(ptr))->x_max;
    }
    else if (type == Type::Number)
    {
        return (static_cast<t_my_numbox*>(ptr))->x_max;
    }
    else if (type == Type::HorizontalRadio)
    {
        return (static_cast<t_hdial*>(ptr))->x_number - 1;
    }
    else if (type == Type::VerticalRadio)
    {
        return (static_cast<t_vdial*>(ptr))->x_number - 1;
    }
    else if (type == Type::Bang)
    {
        return 1;
    }
    else if (type == Type::AtomNumber)
    {
        auto const* gatom = static_cast<t_fake_gatom const*>(ptr);
        if (std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() && std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
        {
            return gatom->a_draghi;
        }
        return std::numeric_limits<float>::max();
    }
    return 1.f;
}

void Gui::setMinimum(float value) noexcept
{
    if (!ptr) return;

    if (type == Type::HorizontalSlider)
    {
        static_cast<t_hslider*>(ptr)->x_min = value;
    }
    else if (type == Type::VerticalSlider)
    {
        static_cast<t_vslider*>(ptr)->x_min = value;
    }
    else if (type == Type::Number)
    {
        static_cast<t_my_numbox*>(ptr)->x_min = value;
    }
    else if (type == Type::AtomNumber)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        if (std::abs(value) > std::numeric_limits<float>::epsilon() && std::abs(value) > std::numeric_limits<float>::epsilon())
        {
            gatom->a_draglo = value;
        }
    }
}

void Gui::setMaximum(float value) noexcept
{
    if (!ptr) return;

    if (type == Type::HorizontalSlider)
    {
        static_cast<t_hslider*>(ptr)->x_max = value;
    }
    else if (type == Type::VerticalSlider)
    {
        static_cast<t_vslider*>(ptr)->x_max = value;
    }
    else if (type == Type::Number)
    {
        static_cast<t_my_numbox*>(ptr)->x_max = value;
    }
    else if (type == Type::HorizontalRadio)
    {
        static_cast<t_hdial*>(ptr)->x_number = value + 1;
    }
    else if (type == Type::VerticalRadio)
    {
        static_cast<t_vdial*>(ptr)->x_number = value + 1;
    }
    else if (type == Type::AtomNumber)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        if (std::abs(value) > std::numeric_limits<float>::epsilon() && std::abs(value) > std::numeric_limits<float>::epsilon())
        {
            gatom->a_draghi = value;
        }
    }
}

float Gui::getValue() const noexcept
{
    if (!ptr) return 0.f;
    if (type == Type::HorizontalSlider)
    {
        return (static_cast<t_hslider*>(ptr))->x_fval;
    }
    else if (type == Type::VerticalSlider)
    {
        return (static_cast<t_vslider*>(ptr))->x_fval;
    }
    else if (type == Type::Toggle)
    {
        return (static_cast<t_toggle*>(ptr))->x_on;
    }
    else if (type == Type::Number)
    {
        return (static_cast<t_my_numbox*>(ptr))->x_val;
    }
    else if (type == Type::HorizontalRadio)
    {
        return (static_cast<t_hdial*>(ptr))->x_on;
    }
    else if (type == Type::VerticalRadio)
    {
        return (static_cast<t_vdial*>(ptr))->x_on;
    }
    else if (type == Type::Bang)
    {
        // hack to trigger off the bang if no GUI update
        if ((static_cast<t_bng*>(ptr))->x_flashed > 0)
        {
            static_cast<t_bng*>(ptr)->x_flashed = 0;
            return 1.0f;
        }
        return 0.0f;
    }
    else if (type == Type::VuMeter)
    {
        // Return RMS
        return static_cast<t_vu*>(ptr)->x_fp;
    }
    else if (type == Type::AtomNumber)
    {
        return atom_getfloat(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)));
    }
    return 0.f;
}

float Gui::getPeak() const noexcept
{
    if (type == Type::VuMeter)
    {
        return static_cast<t_vu*>(ptr)->x_fr;
    }

    return 0;
}

void Gui::setValue(float value) noexcept
{
    if (!instance || !ptr || type == Type::Comment || type == Type::AtomSymbol) return;

    instance->enqueueDirectMessages(ptr, value);
}

std::vector<Atom> Gui::getList() const noexcept
{
    if (!ptr || type != Type::AtomList)
        return {};
    else
    {
        std::vector<Atom> array;
        instance->setThis();

        int ac = binbuf_getnatom(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        t_atom* av = binbuf_getvec(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        array.reserve(ac);
        for (int i = 0; i < ac; ++i)
        {
            if (av[i].a_type == A_FLOAT)
            {
                array.push_back({atom_getfloat(av + i)});
            }
            else if (av[i].a_type == A_SYMBOL)
            {
                array.push_back({atom_getsymbol(av + i)->s_name});
            }
            else
            {
                array.push_back({});
            }
        }
        return array;
    }
}

void Gui::setList(std::vector<Atom> const& value) noexcept
{
    if (!ptr || type != Type::AtomList) return;

    instance->enqueueDirectMessages(ptr, value);
}

bool Gui::jumpOnClick() const noexcept
{
    if (!ptr) return 0.f;
    if (type == Type::HorizontalSlider)
    {
        return (static_cast<t_hslider*>(ptr))->x_steady == 0;
    }
    else if (type == Type::VerticalSlider)
    {
        return (static_cast<t_vslider*>(ptr))->x_steady == 0;
    }
    return false;
}

bool Gui::isLogScale() const noexcept
{
    if (!ptr) return 0.f;
    if (type == Type::HorizontalSlider)
    {
        return (static_cast<t_hslider*>(ptr))->x_lin0_log1 != 0;
    }
    else if (type == Type::VerticalSlider)
    {
        return (static_cast<t_vslider*>(ptr))->x_lin0_log1 != 0;
    }
    return false;
}

void Gui::setLogScale(bool log) noexcept
{
    if (!ptr) return;

    if (type == Type::HorizontalSlider)
    {
        static_cast<t_hslider*>(ptr)->x_lin0_log1 = log;
    }
    else if (type == Type::VerticalSlider)
    {
        static_cast<t_vslider*>(ptr)->x_lin0_log1 = log;
    }
}

std::string Gui::getSymbol() const noexcept
{
    if (ptr && type == Type::Message)
    {
        instance->setThis();

        char* argv;
        int argc;

        binbuf_gettext(static_cast<t_message*>(ptr)->m_text.te_binbuf, &argv, &argc);

        return {argv, static_cast<size_t>(argc)};
    }
    else if (ptr && type == Type::AtomSymbol)
    {
        instance->setThis();
        return atom_getsymbol(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)))->s_name;
    }

    return {};
}

void Gui::click() noexcept
{
    instance->enqueueDirectMessages(ptr, 0);
}

void Gui::setSymbol(std::string const& value) noexcept
{
    if (ptr && type == Type::Message)
    {
        auto valueCopy = value;  // to ensure thread safety
        instance->enqueueFunction(
            [this, valueCopy]() mutable
            {
                auto* messobj = static_cast<t_message*>(ptr);
                binbuf_clear(messobj->m_text.te_binbuf);
                binbuf_text(messobj->m_text.te_binbuf, valueCopy.c_str(), valueCopy.size());
                glist_retext(messobj->m_glist, &messobj->m_text);
            });
    }

    else if (ptr && type == Type::AtomSymbol)
    {
        instance->enqueueDirectMessages(ptr, value);
    }
}

float Gui::getFontHeight() const noexcept
{
    if (!ptr) return 0;
    if (isIEM())
    {
        return static_cast<t_iemgui*>(ptr)->x_fontsize;
    }
    else
    {
        return libpd_get_canvas_font_height(patch->getPointer());
    }
}
void Gui::setFontHeight(float newSize) noexcept
{
    if (!ptr || !isIEM()) return;

    static_cast<t_iemgui*>(ptr)->x_fontsize = newSize;
}

std::string Gui::getFontName() const
{
    if (!ptr) return {sys_font};
    if (isIEM())
    {
        return {(static_cast<t_iemgui*>(ptr))->x_font};
    }
    else
    {
        return {sys_font};
    }
}

static unsigned int fromIemColours(int const color)
{
    auto const c = static_cast<unsigned int>(color << 8 | 0xFF);
    return ((0xFF << 24) | ((c >> 24) << 16) | ((c >> 16) << 8) | (c >> 8));
}

Colour Gui::getBackgroundColour() const noexcept
{
    if (ptr && isIEM())
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_background_color(ptr)));
    }
    return Colours::white;
}

Colour Gui::getForegroundColour() const noexcept
{
    if (ptr && isIEM())
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_foreground_color(ptr)));
    }
    return Colours::white;
}

Colour Gui::getLabelColour() const noexcept
{
    if (ptr && isIEM())
    {
        return Colour(static_cast<uint32>(libpd_iemgui_get_label_color(ptr)));
    }
    return Colours::white;
}

void Gui::setBackgroundColour(Colour colour) noexcept
{
    String colourStr = colour.toString();
    libpd_iemgui_set_background_color(getPointer(), colourStr.toRawUTF8());
}

void Gui::setForegroundColour(Colour colour) noexcept
{
    String colourStr = colour.toString();
    libpd_iemgui_set_foreground_color(getPointer(), colourStr.toRawUTF8());
}

void Gui::setLabelColour(Colour colour) noexcept
{
    String colourStr = colour.toString();
    libpd_iemgui_set_label_color(getPointer(), colourStr.toRawUTF8());
}

Rectangle<int> Gui::getBounds() const noexcept
{
    instance->setThis();
    patch->setCurrent(true);

    int x = 0, y = 0, w = 0, h = 0;
    libpd_get_object_bounds(patch->getPointer(), ptr, &x, &y, &w, &h);
    
    if (type == Type::Keyboard)
    {
        auto* keyboard = static_cast<t_keyboard*>(ptr);
        return {x, y, keyboard->x_width - 28, keyboard->x_height};
    }
    if (type == Type::Mousepad)
    {
        return {x, y, w, h};
    }
    if (type == Type::Panel)
    {
        return {x, y, static_cast<t_my_canvas*>(ptr)->x_vis_w, static_cast<t_my_canvas*>(ptr)->x_vis_h};
    }
    if (type == Type::Array || type == Type::GraphOnParent)
    {
        auto* glist = static_cast<_glist*>(ptr);
        return {x, y, glist->gl_pixwidth, glist->gl_pixheight};
    }
    else if (type == Type::Number)
    {
        auto* nbx = static_cast<t_my_numbox*>(ptr);
        auto* iemgui = &nbx->x_gui;

        int fontwidth = glist_fontwidth(patch->getPointer());

        int width = nbx->x_numwidth * nbxCharWidth;

        return {x, y, width, iemgui->x_h};
    }
    else if (isIEM())
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        return {x, y, iemgui->x_w, iemgui->x_h};
    }

    return Object::getBounds();
}

void Gui::setBounds(Rectangle<int> bounds)
{
    auto oldBounds = getBounds();

    int w = bounds.getWidth();
    int h = bounds.getHeight();

    if (w == oldBounds.getWidth() && h == oldBounds.getHeight()) return;

    if (type != Type::Keyboard && type != Type::Panel && type != Type::Array && type != Type::GraphOnParent && !isIEM())
    {
        Object::setBounds(bounds);
        return;
    }

    addUndoableAction();

    if (type == Type::Keyboard)
    {
        // Don't use zooming for keyboard, it's big enough already
        //static_cast<t_keyboard*>(ptr)->x_width = bounds.getWidth() + 28;
        //static_cast<t_keyboard*>(ptr)->x_height = bounds.getHeight();
        
        // Don't allow keyboard resize for now
        Object::setBounds(bounds);
    }
    if (type == Type::Panel)
    {
        static_cast<t_my_canvas*>(ptr)->x_vis_w = w - 1;
        static_cast<t_my_canvas*>(ptr)->x_vis_h = h - 1;
    }
    if (type == Type::Array || type == Type::GraphOnParent)
    {
        static_cast<_glist*>(ptr)->gl_pixwidth = w;
        static_cast<_glist*>(ptr)->gl_pixheight = h;
    }
    if (type == Type::Number)
    {
        auto* nbx = static_cast<t_my_numbox*>(ptr);

        short newWidth = std::max<short>(3, bounds.getWidth() / nbxCharWidth);
        nbx->x_numwidth = newWidth;
        my_numbox_calc_fontwidth(nbx);
    }
    else if (isIEM())
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);

        iemgui->x_w = w;
        iemgui->x_h = h;
    }

    libpd_moveobj(patch->getPointer(), (t_gobj*)getPointer(), bounds.getX(), bounds.getY());
}

Array Gui::getArray() const noexcept
{
    if (type == Type::Array)
    {
        return instance->getArray(libpd_array_get_name(static_cast<t_canvas*>(ptr)->gl_list));
    }
    return {};
}

Patch Gui::getPatch() const noexcept
{
    if (type == Type::GraphOnParent || type == Type::Subpatch)
    {
        return {ptr, instance};
    }

    return {};
}

void Gui::setSendSymbol(const String& symbol) const noexcept
{
    if (symbol.isEmpty()) return;

    if (ptr && isIEM())
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);

        if (symbol == "empty")
        {
            iemgui->x_fsf.x_snd_able = false;
        }
        else
        {
            iemgui->x_snd_unexpanded = gensym(symbol.toRawUTF8());
            iemgui->x_snd = canvas_realizedollar(iemgui->x_glist, iemgui->x_snd_unexpanded);

            iemgui->x_fsf.x_snd_able = true;
            iemgui_verify_snd_ne_rcv(iemgui);
        }
    }
    if (ptr && isAtom())
    {
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_symto = gensym(symbol.toRawUTF8());
        atom->a_expanded_to = canvas_realizedollar(atom->a_glist, atom->a_symto);
    }
}

void Gui::setReceiveSymbol(const String& symbol) const noexcept
{
    if (symbol.isEmpty()) return;
    if (ptr && isIEM())
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);

        bool rcvable = true;

        if (symbol == "empty")
        {
            rcvable = false;
        }

        // iemgui_all_raute2dollar(srl);
        // iemgui_all_dollararg2sym(iemgui, srl);

        if (rcvable)
        {
            if (strcmp(symbol.toRawUTF8(), iemgui->x_rcv_unexpanded->s_name))
            {
                if (iemgui->x_fsf.x_rcv_able) pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
                iemgui->x_rcv_unexpanded = gensym(symbol.toRawUTF8());
                iemgui->x_rcv = canvas_realizedollar(iemgui->x_glist, iemgui->x_rcv_unexpanded);
                pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            }
        }
        else if (iemgui->x_fsf.x_rcv_able)
        {
            pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv_unexpanded = gensym(symbol.toRawUTF8());
            iemgui->x_rcv = canvas_realizedollar(iemgui->x_glist, iemgui->x_rcv_unexpanded);
        }

        iemgui->x_fsf.x_rcv_able = rcvable;
        iemgui_verify_snd_ne_rcv(iemgui);
    }
    else if (ptr && isAtom())
    {
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        if (*atom->a_symfrom->s_name) pd_unbind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
        atom->a_symfrom = gensym(symbol.toRawUTF8());
        if (*atom->a_symfrom->s_name) pd_bind(&atom->a_text.te_pd, canvas_realizedollar(atom->a_glist, atom->a_symfrom));
    }
}

String Gui::getSendSymbol() noexcept
{
    if (ptr && isIEM())
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);
        std::string name = iemgui->x_snd_unexpanded->s_name;
        if (name == "empty") return "";

        return name;
    }

    return "";
}
String Gui::getReceiveSymbol() noexcept
{
    if (ptr && isIEM())
    {
        t_symbol* srlsym[3];
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        iemgui_all_sym2dollararg(iemgui, srlsym);

        std::string name = iemgui->x_rcv_unexpanded->s_name;

        if (name == "empty") return "";

        return name;
    }

    return "";
}

Point<int> Gui::getLabelPosition(Rectangle<int> bounds) const noexcept
{
    instance->setThis();

    auto const fontheight = 12;

    if (isIEM())
    {
        t_symbol const* sym = canvas_realizedollar(static_cast<t_iemgui*>(ptr)->x_glist, static_cast<t_iemgui*>(ptr)->x_lab);
        if (sym)
        {
            auto const* iemgui = static_cast<t_iemgui*>(ptr);
            int const posx = bounds.getX() + iemgui->x_ldx;
            int const posy = bounds.getY() + iemgui->x_ldy;
            return {posx, posy - 10};
        }
    }
    
    return {bounds.getX(), bounds.getY()};
}

String Gui::getLabelText() const noexcept
{
    instance->setThis();
    if (isIEM())
    {
        t_symbol const* sym = canvas_realizedollar(static_cast<t_iemgui*>(ptr)->x_glist, static_cast<t_iemgui*>(ptr)->x_lab);
        if (sym)
        {
            auto const text = String(sym->s_name);
            if (text.isNotEmpty() && text != "empty")
            {
                return text;
            }
        }
    }
    else if (isAtom())
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        t_symbol const* sym = canvas_realizedollar(gatom->a_glist, gatom->a_label);
        if (sym)
        {
            auto const text = String(sym->s_name);
            if (text.isNotEmpty() && text != "empty")
            {
                return text;
            }
        }
    }

    return "";
}

void Gui::setLabelText(String newText) noexcept
{
    if (newText.isEmpty()) newText = "empty";

    if (isIEM())
    {
        static_cast<t_iemgui*>(ptr)->x_lab = gensym(newText.toRawUTF8());
    }
    if (isAtom())
    {
        static_cast<t_fake_gatom*>(ptr)->a_label = gensym(newText.toRawUTF8());
    }
}

void Gui::setLabelPosition(Point<int> position) noexcept
{
    if (isIEM())
    {
        auto* iem = static_cast<t_iemgui*>(ptr);
        iem->x_ldx = position.x;
        iem->x_ldy = position.y + 10.0f;
        return;
    }
    jassertfalse;
}

void Gui::setLabelPosition(int wherelabel) noexcept
{
    if (isAtom())
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        gatom->a_wherelabel = wherelabel - 1;
        return;
    }
    jassertfalse;
}

}  // namespace pd
