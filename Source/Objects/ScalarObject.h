/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/GlobalMouseListener.h"

extern "C" {

int scalar_doclick(t_word* data, t_template* t, t_scalar* sc,
    t_array* ap, struct _glist* owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit);
}

#define CLOSED 1      /* polygon */
#define BEZ 2         /* bezier shape */
#define NOMOUSERUN 4  /* disable mouse interaction when in run mode  */
#define NOMOUSEEDIT 8 /* same in edit mode */
#define NOVERTICES 16 /* disable only vertex grabbing in run mode */
#define A_ARRAY 55    /* LATER decide whether to enshrine this in m_pd.h */

// Global mouse listener class:
// If you attach a normal global mouse listener to a component on canvas, you run the risk of
// accidentally passing on mouse scroll events to the viewport.
// This prevents that with a separation layer.

class DrawableTemplate {

public:
    virtual void update() = 0;

    /* getting and setting values via fielddescs -- note confusing names;
     the above are setting up the fielddesc itself. */
    static t_float fielddesc_getfloat(t_fake_fielddesc* f, t_template* templ, t_word* wp, int loud)
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
        int ret = (n2 << 5);       /* 0 to 256 in 9 steps */
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
        sprintf(s, "#%2.2x%2.2x%2.2x", rangecolor(red), rangecolor(blue), rangecolor(green));
    }
};

class DrawableCurve final : public DrawableTemplate
    , public DrawablePath {
    t_scalar* scalar;
    t_fake_curve* object;
    int baseX, baseY;
    Canvas* canvas;

    GlobalMouseListener mouseListener;

public:
    DrawableCurve(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y)
        : scalar(s)
        , object(reinterpret_cast<t_fake_curve*>(obj))
        , canvas(cnv)
        , baseX(x)
        , baseY(y)
        , mouseListener(this)
    {
        mouseListener.globalMouseDown = [this](MouseEvent const& e) {
            handleMouseDown(e);
        };
    }

    void handleMouseDown(MouseEvent const& e)
    {
        if (!getLocalBounds().contains(e.getPosition()) || !getValue<bool>(canvas->locked) || !canvas->isShowing() || !scalar->sc_template)
            return;

        auto shift = e.mods.isShiftDown();
        auto alt = e.mods.isAltDown();
        auto dbl = 0;

        canvas->pd->setThis();
        
        auto* patch = canvas->patch.getPointer().get();
        if(!patch) return;

        t_template* t = template_findbyname(scalar->sc_template);
        scalar_doclick(scalar->sc_vec, t, scalar, nullptr, patch, 0, 0, e.x, getHeight() - e.y, shift, alt, dbl, 1);

        // Update all drawables
        for (auto* object : canvas->objects) {
            if (!object->gui)
                continue;
            object->gui->updateDrawables();
        }
    }

    void update() override
    {
        if (!scalar || !scalar->sc_template)
            return;

        canvas->pd->setThis();

        auto* glist = canvas->patch.getPointer().get();
        if(!glist) return;
        
        auto* templ = template_findbyname(scalar->sc_template);

        auto* x = reinterpret_cast<t_fake_curve*>(object);
        int n = x->x_npoints;
        auto* data = scalar->sc_vec;

        if (!fielddesc_getfloat(&x->x_vis, templ, data, 0)) {
            return;
        }

        auto bounds = canvas->isGraph ? canvas->getParentComponent()->getLocalBounds() : canvas->getLocalBounds();

        if (n > 1) {
            int flags = x->x_flags;
            int closed = flags & CLOSED;

            t_float width = fielddesc_getfloat(&x->x_width, templ, data, 1);

            char outline[20], fill[20];
            int pix[200];
            if (n > 100)
                n = 100;

            canvas->pd->lockAudioThread();

            for (int i = 0; i < n; i++) {
                auto* f = x->x_vec + (i * 2);
                float xCoord = (baseX + fielddesc_getcoord((t_fielddesc*)f, templ, data, 1)) / (glist->gl_x2 - glist->gl_x1);
                float yCoord = (baseY + fielddesc_getcoord((t_fielddesc*)(f + 1), templ, data, 1)) / (glist->gl_y1 - glist->gl_y2);

                yCoord = 1.0f - yCoord;
                // In a graph, offset the position by canvas margin
                // This will make sure the drawing is shown at origin in the original subpatch,
                // but at the graph's origin when shown inside a graph
                auto xOffset = canvas->isGraph ? glist->gl_xmargin : 0;
                auto yOffset = canvas->isGraph ? glist->gl_ymargin : 0;

                xOffset += canvas->canvasOrigin.x;
                yOffset += canvas->canvasOrigin.y;

                pix[2 * i] = xCoord * bounds.getWidth() + xOffset;
                pix[2 * i + 1] = yCoord * bounds.getHeight() + yOffset;
            }

            canvas->pd->unlockAudioThread();

            if (width < 1)
                width = 1;
            if (glist->gl_isgraph)
                width *= glist_getzoom(glist);

            numbertocolor(fielddesc_getfloat(&x->x_outlinecolor, templ, data, 1), outline);
            setStrokeFill(Colour::fromString("FF" + String::fromUTF8(outline + 1)));
            setStrokeThickness(width);

            if (closed) {
                numbertocolor(fielddesc_getfloat(&x->x_fillcolor, templ, data, 1), fill);
                setFill(Colour::fromString("FF" + String::fromUTF8(fill + 1)));
            } else {
                setFill(Colours::transparentBlack);
            }

            Path toDraw;

            toDraw.startNewSubPath(pix[0], pix[1]);
            for (int i = 1; i < n; i++) {
                toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
            }

            if (closed) {
                toDraw.lineTo(pix[0], pix[1]);
            }

            auto drawBounds = toDraw.getBounds();

            // tcl/tk will show a dot for a 0px polygon
            // JUCE doesn't do this, so we have to fake it
            if (closed && drawBounds.isEmpty()) {
                toDraw.clear();
                toDraw.addEllipse(drawBounds.withSizeKeepingCentre(5, 5));
                setStrokeThickness(2);
                setFill(getStrokeFill());
            }

            setPath(toDraw);
        } else {
            post("warning: curves need at least two points to be graphed");
        }
    }
};

class DrawableSymbol final : public DrawableTemplate
    , public DrawableText {
    t_scalar* scalar;
    t_fake_drawnumber* object;
    int baseX, baseY;
    Canvas* canvas;

public:
    DrawableSymbol(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y)
        : scalar(s)
        , object(reinterpret_cast<t_fake_drawnumber*>(obj))
        , canvas(cnv)
        , baseX(x)
        , baseY(y)
    {
    }

    // TODO: implement number dragging
    void mouseDown(MouseEvent const& e) override
    {
    }

#define DRAWNUMBER_BUFSIZE 1024
    void update() override
    {
        if (!scalar || !scalar->sc_template)
            return;

        canvas->pd->setThis();


        auto* templ = template_findbyname(scalar->sc_template);

        auto* x = reinterpret_cast<t_fake_drawnumber*>(object);

        auto* data = scalar->sc_vec;
        t_atom at;

        int xloc = 0, yloc = 0;
        if(auto glist = canvas->patch.getPointer())
        {
            xloc = glist_xtopixels(glist.get(), baseX + fielddesc_getcoord((t_fielddesc*)&x->x_xloc, templ, data, 0));
            yloc = glist_ytopixels(glist.get(), baseY + fielddesc_getcoord((t_fielddesc*)&x->x_yloc, templ, data, 0));
        }
        
        char colorstring[20], buf[DRAWNUMBER_BUFSIZE];
        numbertocolor(fielddesc_getfloat(&x->x_color, templ, data, 1), colorstring);

        int type, onset;
        t_symbol* arraytype;
        if (!template_find_field(templ, x->x_fieldname, &onset, &type, &arraytype) || type == DT_ARRAY) {
            type = -1;
        }

        int nchars;
        if (type < 0)
            buf[0] = 0;
        else {
            strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
            buf[DRAWNUMBER_BUFSIZE - 1] = 0;
            nchars = (int)strlen(buf);
            if (type == DT_TEXT) {
                char* buf2;
                int size2, ncopy;
                binbuf_gettext(((t_word*)((char*)data + onset))->w_binbuf,
                    &buf2, &size2);
                ncopy = (size2 > DRAWNUMBER_BUFSIZE - 1 - nchars ? DRAWNUMBER_BUFSIZE - 1 - nchars : size2);
                memcpy(buf + nchars, buf2, ncopy);
                buf[nchars + ncopy] = 0;
                if (nchars + ncopy == DRAWNUMBER_BUFSIZE - 1)
                    strcpy(buf + (DRAWNUMBER_BUFSIZE - 4), "...");
                t_freebytes(buf2, size2);
            } else {
                t_atom at;
                if (type == DT_FLOAT)
                    SETFLOAT(&at, ((t_word*)((char*)data + onset))->w_float);
                else
                    SETSYMBOL(&at, ((t_word*)((char*)data + onset))->w_symbol);
                atom_string(&at, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
            }
        }

        setColour(Colour::fromString("FF" + String::fromUTF8(colorstring + 1)));
        setBoundingBox(Parallelogram<float>(Rectangle<float>(xloc, yloc, 200, 100)));
        if(auto glist = canvas->patch.getPointer())
        {
            setFontHeight(sys_hostfontsize(glist_getfont(glist.get()), glist_getzoom(glist.get())));
        }
        setText(String::fromUTF8(buf));

        /*
        sys_vgui(".x%lx.c create text %d %d -anchor nw -fill %s -text {%s}",
            glist_getcanvas(glist), xloc, yloc, colorstring, buf);
        sys_vgui(" -font {{%s} -%d %s}", sys_font,
            sys_hostfontsize(glist_getfont(glist), glist_getzoom(glist)),
            sys_fontweight);
        sys_vgui(" -tags [list drawnumber%lx label]\n", data); */
    }
};

struct ScalarObject final : public ObjectBase {
    OwnedArray<Component> templates;

    ScalarObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        cnv->pd->setThis();

        // Make object invisible
        object->setVisible(false);

        auto* x = reinterpret_cast<t_scalar*>(obj);
        auto* templ = template_findbyname(x->sc_template);
        auto* templatecanvas = template_findcanvas(templ);
        t_gobj* y;
        t_float basex, basey;
        scalar_getbasexy(x, &basex, &basey);

        for (y = templatecanvas->gl_list; y; y = y->g_next) {
            t_parentwidgetbehavior const* wb = pd_getparentwidget(&y->g_pd);
            if (!wb)
                continue;

            Component* drawable = nullptr;
            auto name = String::fromUTF8(y->g_pd->c_name->s_name);
            if (name == "drawtext" || name == "drawnumber" || name == "drawsymbol") {
                drawable = templates.add(new DrawableSymbol(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            } else if (name == "drawpolygon" || name == "drawcurve" || name == "filledpolygon" || name == "filledcurve") {
                drawable = templates.add(new DrawableCurve(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            } else if (name == "plot") {
                // TODO: implement this
            }

            cnv->addAndMakeVisible(drawable);
        }

        updateDrawables();
    }

    ~ScalarObject() override
    {
        for (auto* drawable : templates) {
            cnv->removeChildComponent(drawable);
        }
    }

    void updateDrawables() override
    {
        pd->setThis();

        for (auto* drawable : templates) {
            dynamic_cast<DrawableTemplate*>(drawable)->update();
        }
    }

    Rectangle<int> getPdBounds() override { return { 0, 0, 0, 0 }; };
    void setPdBounds(Rectangle<int> b) override {};
};
