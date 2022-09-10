/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct t_fake_fielddesc {
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union {
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

struct t_fake_curve {
    t_object x_obj;
    int x_flags; /* CLOSED, BEZ, NOMOUSERUN, NOMOUSEEDIT */
    t_fake_fielddesc x_fillcolor;
    t_fake_fielddesc x_outlinecolor;
    t_fake_fielddesc x_width;
    t_fake_fielddesc x_vis;
    int x_npoints;
    t_fake_fielddesc* x_vec;
    t_canvas* x_canvas;
};

struct t_fake_drawnumber
{
    t_object x_obj;
    t_symbol *x_fieldname;
    t_fake_fielddesc x_xloc;
    t_fake_fielddesc x_yloc;
    t_fake_fielddesc x_color;
    t_fake_fielddesc x_vis;
    t_symbol *x_label;
    t_canvas *x_canvas;
};

extern "C"
{

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

struct DrawableTemplate : public DrawablePath
{
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
    
    bool isLocked;
};

struct DrawableCurve final : public DrawableTemplate {
    t_scalar* scalar;
    t_fake_curve* object;
    int baseX, baseY;
    Canvas* canvas;
    
    DrawableCurve(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y)
    : scalar(s)
    , object(reinterpret_cast<t_fake_curve*>(obj))
    , canvas(cnv)
    , baseX(x)
    , baseY(y)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }
    
    ~DrawableCurve(){
        Desktop::getInstance().removeGlobalMouseListener(this);
    }
    
    
    void mouseDown(const MouseEvent& e) override
    {
        auto relativeEvent = e.getEventRelativeTo(this);
        
        if (!getLocalBounds().contains(relativeEvent.getPosition()) || !isLocked || !canvas->isShowing())
            return;
        
        auto shift = e.mods.isShiftDown();
        auto alt = e.mods.isAltDown();
        auto dbl = 0;
        
        t_template *t = template_findbyname(scalar->sc_template);
        scalar_doclick(scalar->sc_vec, t, scalar, 0, canvas->patch.getPointer(), 0, 0, relativeEvent.x, relativeEvent.y, shift, alt, dbl, 1);
       
        // Update all drawables
        for(auto* box : canvas->boxes)  {
            if(!box->gui) continue;
            box->gui->updateDrawables();
        }
    }
    
    void update() override {

        if (!scalar || !scalar->sc_template)
            return;
        
        auto* glist = canvas->patch.getPointer();
        auto* templ = template_findbyname(scalar->sc_template);
        
        auto* x = reinterpret_cast<t_fake_curve*>(object);
        int i, n = x->x_npoints;
        t_fake_fielddesc* f = x->x_vec;
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
            
            canvas->pd->getCallbackLock()->enter();
            
            for (i = 0, f = x->x_vec; i < n; i++, f += 2) {
                float xCoord = (baseX + fielddesc_getcoord((t_fielddesc*)f, templ, data, 1)) / (glist->gl_x2 - glist->gl_x1);
                float yCoord = (baseY + fielddesc_getcoord((t_fielddesc*)(f + 1), templ, data, 1)) / (glist->gl_y1 - glist->gl_y2);
                
                // In a graph, offset the position by canvas margin
                // This will make sure the drawing is shown at origin in the original subpatch,
                // but at the graph's origin when shown inside a graph
                auto xOffset = canvas->isGraph ? glist->gl_xmargin : 0;
                auto yOffset = canvas->isGraph ? glist->gl_ymargin : 0;
                
                pix[2 * i] = xCoord * bounds.getWidth() + xOffset;
                pix[2 * i + 1] = yCoord * bounds.getHeight() + yOffset;
            }
            
            canvas->pd->getCallbackLock()->exit();
            
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
            }
            else {
                setFill(Colours::transparentBlack);
            }
            
            Path toDraw;
            
            toDraw.startNewSubPath(pix[0], pix[1]);
            for (i = 1; i < n; i++) {
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

struct DrawableSymbol final : public DrawableTemplate {
    t_scalar* scalar;
    t_fake_drawnumber* object;
    int baseX, baseY;
    Canvas* canvas;
    
    DrawableSymbol(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y)
    : scalar(s)
    , object(reinterpret_cast<t_fake_drawnumber*>(obj))
    , canvas(cnv)
    , baseX(x)
    , baseY(y)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }
    
    ~DrawableSymbol(){
        Desktop::getInstance().removeGlobalMouseListener(this);
    }
    
    // TODO: implement number dragging
    void mouseDown(const MouseEvent& e) override
    {
    }

    
#define DRAWNUMBER_BUFSIZE 1024
    void update() override {
        // TODO: hacky workaround for potential crash. Doens't always work. Fix this.
        if (!scalar || !scalar->sc_template)
            return;
        
        auto* glist = canvas->patch.getPointer();
        auto* templ = template_findbyname(scalar->sc_template);
        
        auto* x = reinterpret_cast<t_fake_drawnumber*>(object);
        
        auto* data = scalar->sc_vec;
        t_atom at;
        
        int xloc = glist_xtopixels(glist, baseX + fielddesc_getcoord((t_fielddesc*)&x->x_xloc, templ, data, 0));
        int yloc = glist_ytopixels(glist, baseY + fielddesc_getcoord((t_fielddesc*)&x->x_yloc, templ, data, 0));
        char colorstring[20], buf[DRAWNUMBER_BUFSIZE];
        numbertocolor(fielddesc_getfloat(&x->x_color, templ, data, 1), colorstring);
        
        int type, onset;
        t_symbol *arraytype;
        if (!template_find_field(templ, x->x_fieldname, &onset, &type, &arraytype) || type != DT_ARRAY) {
            type = -1;
        }
        
        int nchars;
        if (type < 0)
            buf[0] = 0;
        else
        {
            strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
            buf[DRAWNUMBER_BUFSIZE - 1] = 0;
            nchars = (int)strlen(buf);
            if (type == DT_TEXT)
            {
                char *buf2;
                int size2, ncopy;
                binbuf_gettext(((t_word *)((char *)data + onset))->w_binbuf,
                    &buf2, &size2);
                ncopy = (size2 > DRAWNUMBER_BUFSIZE-1-nchars ?
                    DRAWNUMBER_BUFSIZE-1-nchars: size2);
                memcpy(buf+nchars, buf2, ncopy);
                buf[nchars+ncopy] = 0;
                if (nchars+ncopy == DRAWNUMBER_BUFSIZE-1)
                    strcpy(buf+(DRAWNUMBER_BUFSIZE-4), "...");
                t_freebytes(buf2, size2);
            }
            else
            {
                t_atom at;
                if (type == DT_FLOAT)
                    SETFLOAT(&at, ((t_word *)((char *)data + onset))->w_float);
                else SETSYMBOL(&at, ((t_word *)((char *)data + onset))->w_symbol);
                atom_string(&at, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
            }
        }
        

        sys_vgui(".x%lx.c create text %d %d -anchor nw -fill %s -text {%s}",
                glist_getcanvas(glist), xloc, yloc, colorstring, buf);
        sys_vgui(" -font {{%s} -%d %s}", sys_font,
            sys_hostfontsize(glist_getfont(glist), glist_getzoom(glist)),
                sys_fontweight);
        sys_vgui(" -tags [list drawnumber%lx label]\n", data);

    }
};


struct ScalarObject final : public NonPatchable {
    OwnedArray<DrawableTemplate> templates;
    
    ScalarObject(void* obj, Box* box)
    : NonPatchable(obj, box)
    {
        box->setVisible(false);
        
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
            
            DrawableTemplate* drawable = nullptr;
            auto name = String::fromUTF8(y->g_pd->c_name->s_name);
            if (name == "drawtext" || name == "drawnumber" || name == "drawsymbol") {
                drawable = templates.add(new DrawableSymbol(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            }
            else if(name == "drawpolygon" || name == "drawcurve" || name == "filledpolygon" || name == "filledcurve") {
                drawable = templates.add(new DrawableCurve(x, y, cnv, static_cast<int>(basex), static_cast<int>(basey)));
            }
            else if(name == "plot") {
                // TODO: implement this
            }
            
            if(drawable) cnv->addAndMakeVisible(drawable);
        }
        
        updateDrawables();
    }
    
    void updateDrawables() override
    {
        for (auto& drawable : templates) {
            drawable->update();
        }
    }
    
    void lock(bool locked) override
    {
        for (auto& drawable : templates) {
            drawable->isLocked = locked;
        }
    }
};
