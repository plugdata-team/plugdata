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
    if(String(object->x_obj.te_g.g_pd->c_name->s_name) == "drawtext") {
        return; // not supported yet
    }
    
    auto* glist = canvas->patch.getPointer();
    auto* templ = template_findbyname(scalar->sc_template);

    bool vis = true;

    int i, n = object->x_npoints;
    t_fielddesc* f = object->x_vec;

    auto* data = scalar->sc_vec;


    /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&object->x_vis, templ, data, 0))
    {
        return;
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
