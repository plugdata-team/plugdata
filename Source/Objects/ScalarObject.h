/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct _fielddesc
{
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union
    {
        t_float fd_float;    /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1; /* min and max values */
    float fd_v2;
    float fd_screen1; /* min and max screen values */
    float fd_screen2;
    float fd_quantum; /* quantization in value */
};

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

#define CLOSED 1      /* polygon */
#define BEZ 2         /* bezier shape */
#define NOMOUSERUN 4  /* disable mouse interaction when in run mode  */
#define NOMOUSEEDIT 8 /* same in edit mode */
#define NOVERTICES 16 /* disable only vertex grabbing in run mode */
#define A_ARRAY 55    /* LATER decide whether to enshrine this in m_pd.h */

struct t_curve;
struct DrawableTemplate final : public DrawablePath
{
    t_scalar* scalar;
    t_curve* object;
    int baseX, baseY;
    Canvas* canvas;

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

    DrawableTemplate(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y) : scalar(s), object(reinterpret_cast<t_curve*>(obj)), canvas(cnv), baseX(x), baseY(y)
    {
    }

    void update()
    {
        if (String(object->x_obj.te_g.g_pd->c_name->s_name) == "drawtext")
        {
            return;  // not supported yet
        }

        auto* glist = canvas->patch.getPointer();
        auto* templ = template_findbyname(scalar->sc_template);

        int i, n = object->x_npoints;
        t_fielddesc* f = object->x_vec;

        auto* data = scalar->sc_vec;

        /* see comment in plot_vis() */
        if (!fielddesc_getfloat(&object->x_vis, templ, data, 0))
        {
            return;
        }

        auto bounds = canvas->isGraph ? canvas->getParentComponent()->getLocalBounds() : canvas->getLocalBounds();

        if (n > 1)
        {
            int flags = object->x_flags;
            int closed = flags & CLOSED;
            
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

                pix[2 * i] = xCoord * bounds.getWidth();
                pix[2 * i + 1] = yCoord * bounds.getHeight();
            }

            canvas->pd->getCallbackLock()->exit();

            if (width < 1) width = 1;
            if (glist->gl_isgraph) width *= glist_getzoom(glist);

            numbertocolor(fielddesc_getfloat(&object->x_outlinecolor, templ, data, 1), outline);
            
            if (closed)
            {
                numbertocolor(fielddesc_getfloat(&object->x_fillcolor, templ, data, 1), fill);
            }

            Path toDraw;

            toDraw.startNewSubPath(pix[0], pix[1]);
            for (i = 1; i < n; i++)
            {
                toDraw.lineTo(pix[2 * i], pix[2 * i + 1]);
            }
            
            if (closed)
            {
                toDraw.lineTo(pix[0], pix[1]);
            }

            String objName = String::fromUTF8(object->x_obj.te_g.g_pd->c_name->s_name);
            if (!closed)
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
            
            auto drawBounds = toDraw.getBounds();
            // tcl/tk will show a dot for a 0px polygon
            // JUCE doesn't do this, so we have to fake it
            if ((flags & CLOSED) && drawBounds.isEmpty())
            {
                toDraw.clear();
                toDraw.addEllipse(drawBounds.withSizeKeepingCentre(5, 5));
                setStrokeThickness(2);
                setFill(getStrokeFill());
            }
                
            setPath(toDraw);
        }
        else
            post("warning: curves need at least two points to be graphed");
    }
};

struct ScalarObject final : public NonPatchable
{
    OwnedArray<DrawableTemplate> templates;

    ScalarObject(void* obj, Box* box) : NonPatchable(obj, box)
    {
        box->setVisible(false);

        auto* x = reinterpret_cast<t_scalar*>(obj);
        auto* templ = template_findbyname(x->sc_template);
        auto* templatecanvas = template_findcanvas(templ);
        t_gobj* y;
        t_float basex, basey;
        scalar_getbasexy(x, &basex, &basey);

        for (y = templatecanvas->gl_list; y; y = y->g_next)
        {
            const t_parentwidgetbehavior* wb = pd_getparentwidget(&y->g_pd);
            if (!wb) continue;

            auto* drawable = templates.add(new DrawableTemplate(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            cnv->addAndMakeVisible(drawable);
        }

        updateDrawables();
    }

    void updateDrawables() override
    {
        for (auto& drawable : templates)
        {
            drawable->update();
        }
    }
};
