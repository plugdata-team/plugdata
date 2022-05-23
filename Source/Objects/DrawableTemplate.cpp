#include "DrawableTemplate.h"
#include "../Canvas.h"

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

static void scalar_getrect(t_gobj *z, t_glist *owner,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_scalar *x = (t_scalar *)z;
    t_template *templ = template_findbyname(x->sc_template);
    t_canvas *templatecanvas = template_findcanvas(templ);
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;
    t_gobj *y;
    t_float basex, basey;
    scalar_getbasexy(x, &basex, &basey);
        /* if someone deleted the template canvas, we're just a point */
    if (!templatecanvas)
    {
        x1 = x2 = glist_xtopixels(owner, basex);
        y1 = y2 = glist_ytopixels(owner, basey);
    }
    else
    {
        x1 = y1 = 0x7fffffff;
        x2 = y2 = -0x7fffffff;
        for (y = templatecanvas->gl_list; y; y = y->g_next)
        {
            const t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
            int nx1, ny1, nx2, ny2;
            if (!wb) continue;
            (*wb->w_parentgetrectfn)(y, owner,
                x->sc_vec, templ, basex, basey,
                &nx1, &ny1, &nx2, &ny2);
            if (nx1 < x1) x1 = nx1;
            if (ny1 < y1) y1 = ny1;
            if (nx2 > x2) x2 = nx2;
            if (ny2 > y2) y2 = ny2;
        }
        if (x2 < x1 || y2 < y1)
            x1 = y1 = x2 = y2 = 0;
    }
    /* post("scalar x1 %d y1 %d x2 %d y2 %d", x1, y1, x2, y2); */
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
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
    if (String(object->x_obj.te_g.g_pd->c_name->s_name) == "drawtext")
    {
        return;  // not supported yet
    }

    auto* glist = canvas->patch.getPointer();
    auto* templ = template_findbyname(scalar->sc_template);

    bool vis = true;

    int i, n = object->x_npoints;
    t_fielddesc* f = object->x_vec;

    auto* data = scalar->sc_vec;

    /* see comment in plot_vis() */
    /*
    if (vis && !fielddesc_getfloat(&object->x_vis, templ, data, 0))
    {
        return;
    } */

    // Reduce clip region
    auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
    
    
    auto bounds = canvas->isGraph ? canvas->getParentComponent()->getLocalBounds() : canvas->getLocalBounds();

    
    lastBounds = bounds;// + pos;

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

                pix[2 * i] = xCoord * bounds.getWidth();// + pos.x;
                pix[2 * i + 1] = yCoord * bounds.getHeight();// + pos.y;
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
