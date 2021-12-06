
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "m_imp.h"  /* FIXME need access to c_externdir... */
#include "g_canvas.h"

typedef struct _openfile{
    t_object   x_ob;
    t_glist   *x_glist;
    int        x_isboxed;
    char      *x_vistext;
    int        x_vissize;
    int        x_vislength;
    int        x_rtextactive;
    t_symbol  *x_dirsym;
    t_symbol  *x_ulink;
    int        x_linktype;
} t_openfile;

static t_class *openfile_class;
static t_class *openfilebox_class;

/* Code that might be merged back to g_text.c starts here: */

static void openfile_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_openfile *x = (t_openfile *)z;
    int width, height;
    float x1, y1, x2, y2;
    if(glist->gl_editor && glist->gl_editor->e_rtext){
        if (x->x_rtextactive){
            t_rtext *y = glist_findrtext(glist, (t_text *)x);
            width = rtext_width(y);
            height = rtext_height(y) - 2;
        }
        else{
            int font = glist_getfont(glist);
            width = x->x_vislength * sys_fontwidth(font) + 2;
            height = sys_fontheight(font) + 2;
        }
    }
    else
        width = height = 10;
    x1 = text_xpix((t_text *)x, glist);
    y1 = text_ypix((t_text *)x, glist);
    x2 = x1 + width;
    y2 = y1 + height;
    y1 += 1;
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void openfile_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_text *t = (t_text *)z;
    t->te_xpix += dx;
    t->te_ypix += dy;
    if(glist_isvisible(glist)){
        t_rtext *y = glist_findrtext(glist, t);
        rtext_displace(y, dx, dy);
    }
}

static void openfile_select(t_gobj *z, t_glist *glist, int state){
    t_openfile *x = (t_openfile *)z;
    t_rtext *y = glist_findrtext(glist, (t_text *)x);
    rtext_select(y, state);
    if(state)
        sys_vgui(".x%lx.c itemconfigure %s -fill blue\n", glist, rtext_gettag(y));
    else
        sys_vgui(".x%lx.c itemconfigure %s -text {%s} -fill #0000dd -activefill #e70000\n",
                 glist, rtext_gettag(y), x->x_vistext);
}

static void openfile_activate(t_gobj *z, t_glist *glist, int state){
    t_openfile *x = (t_openfile *)z;
    t_rtext *y = glist_findrtext(glist, (t_text *)x);
    rtext_activate(y, state);
    x->x_rtextactive = state;
}

static void openfile_vis(t_gobj *z, t_glist *glist, int vis){
    t_openfile *x = (t_openfile *)z;
    t_rtext *y = glist_findrtext(glist, (t_text *)x);
    if(vis){
        rtext_draw(y);
        sys_vgui(".x%lx.c itemconfigure %s -text {%s} -fill #0000dd -activefill #e70000\n",
                 glist_getcanvas(glist), rtext_gettag(y), x->x_vistext);
    }
    else
        rtext_erase(y);
}

static void openfile_click(t_openfile *x, t_floatarg xpos, t_floatarg ypos,
                       t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    sys_vgui("openfile_open {%s} {%s}\n",               \
             x->x_ulink->s_name, x->x_dirsym->s_name);
}

static int openfile_wbclick(t_gobj *z, t_glist *glist, int xpix, int ypix,
                        int shift, int alt, int dbl, int doit)
{
    t_openfile *x = (t_openfile *)z;
    if(doit){
        openfile_click(x, (t_floatarg)xpix, (t_floatarg)ypix,
                   (t_floatarg)shift, 0, (t_floatarg)alt);
        return (1);
    }
    else
        return (0);
}

static t_widgetbehavior openfile_widgetbehavior =
{
    openfile_getrect,
    openfile_displace,
    openfile_select,
    openfile_activate,
    0,
    openfile_vis,
    openfile_wbclick,
};

static void openfile_bang(t_openfile *x){
  openfile_click(x, 0, 0, 0, 0, 0);
}

static void openfile_open(t_openfile *x, t_symbol *s){
    x->x_ulink = s;
    openfile_click(x, 0, 0, 0, 0, 0);
}

static int openfile_isoption(char *name){
    if(*name == '-'){
        char c = name[1];
        return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
    }
    else
        return (0);
}

static t_symbol *openfile_nextsymbol(int ac, t_atom *av, int opt, int *skipp){
    int ndx;
    for(ndx = 0; ndx < ac; ndx++, av++){
        if(av->a_type == A_SYMBOL && (!opt || openfile_isoption(av->a_w.w_symbol->s_name))){
            *skipp = ++ndx;
            return (av->a_w.w_symbol);
        }
    }
    return (0);
}

static int openfile_dohyperlink(char *dst, int maxsize, int ac, t_atom *av){
    int i, sz, sep, len;
    char buf[32], *src;
    for(i = 0, sz = 0, sep = 0; i < ac; i++, av++){
        if(sep){
            sz++;
            if (sz >= maxsize)
                break;
            else if(dst){
                *dst++ = ' ';
                *dst = 0;
            }
        }
        else sep = 1;
        if(av->a_type == A_SYMBOL)
            src = av->a_w.w_symbol->s_name;
        else if (av->a_type == A_FLOAT){
            src = buf;
            sprintf(src, "%g", av->a_w.w_float);
        }
        else{
            sep = 0;
            continue;
        }
        len = strlen(src);
        sz += len;
        if (sz >= maxsize)
            break;
        else if (dst){
            strcpy(dst, src);
            dst += len;
        }
    }
    return (sz);
}

static char *openfile_hyperlink(int *sizep, int ac, t_atom *av){
    char *result;
    int sz = openfile_dohyperlink(0, MAXPDSTRING, ac, av);
    *sizep = sz + (sz >= MAXPDSTRING ? 4 : 1);
    result = getbytes(*sizep);
    openfile_dohyperlink(result, sz + 1, ac, av);
    if(sz >= MAXPDSTRING){
        sz = strlen(result);
        strcpy(result + sz, "...");
    }
    return (result);
}

static void openfile_free(t_openfile *x){
    if(x->x_vistext)
        freebytes(x->x_vistext, x->x_vissize);
}

static void *openfile_new(t_symbol *s, int ac, t_atom *av){
    t_openfile xgen, *x;
    int skip;
    xgen.x_isboxed = 1;
    xgen.x_vistext = 0;
    xgen.x_vissize = 0;
    if((xgen.x_ulink = openfile_nextsymbol(ac, av, 0, &skip))){
        t_symbol *opt;
        ac -= skip;
        av += skip;
        while((opt = openfile_nextsymbol(ac, av, 1, &skip))){
            ac -= skip;
            av += skip;
            if(opt == gensym("-h")){
                xgen.x_isboxed = 0;
                t_symbol *nextsym = openfile_nextsymbol(ac, av, 1, &skip);
                int natoms = (nextsym ? skip - 1 : ac);
                if(natoms)
                    xgen.x_vistext = openfile_hyperlink(&xgen.x_vissize, natoms, av);
            }
        }
    }
    x = (t_openfile *)
	pd_new(xgen.x_isboxed ? openfilebox_class : openfile_class);
    x->x_glist = canvas_getcurrent();
    x->x_dirsym = canvas_getdir(x->x_glist);  // FIXME - make it "paths"

    x->x_isboxed = xgen.x_isboxed;
    x->x_vistext = xgen.x_vistext;
    x->x_vissize = xgen.x_vissize;
    x->x_vislength = (x->x_vistext ? strlen(x->x_vistext) : 0);
    x->x_rtextactive = 0;
    if(xgen.x_ulink)
        x->x_ulink = xgen.x_ulink;
    else
        x->x_ulink = &s_;
    if(!x->x_vistext){
        x->x_vislength = strlen(x->x_ulink->s_name);
        x->x_vissize = x->x_vislength + 1;
        x->x_vistext = getbytes(x->x_vissize);
        strcpy(x->x_vistext, x->x_ulink->s_name);
    }
    return (x);
}

void openfile_setup(void){
// GUI
    openfile_class = class_new(gensym("openfile"), (t_newmethod)openfile_new, (t_method)openfile_free,
        sizeof(t_openfile), CLASS_PATCHABLE, // patchable what?
        A_GIMME, 0);
    class_addbang(openfile_class, openfile_bang);
    class_addmethod(openfile_class, (t_method)openfile_open, gensym("open"), A_DEFSYMBOL, 0);
    class_setwidget(openfile_class, &openfile_widgetbehavior);
// Non GUI
    openfilebox_class = class_new(gensym("openfile"), 0, (t_method)openfile_free, sizeof(t_openfile),
        0, A_GIMME, 0);
    class_addbang(openfilebox_class, openfile_bang);
    class_addmethod(openfilebox_class, (t_method)openfile_click, gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(openfilebox_class, (t_method)openfile_open, gensym("open"), A_DEFSYMBOL, 0);
    
    sys_vgui("proc openfile_open {filename dir} {\n");
    sys_vgui("    if {[string first \"://\" $filename] > -1} {\n");
    sys_vgui("        menu_openfile $filename\n");
    sys_vgui("    } elseif {[file pathtype $filename] eq \"absolute\"} {\n");
    sys_vgui("        menu_openfile $filename\n");
    sys_vgui("    } elseif {[file exists [file join $dir $filename]]} {\n");
    sys_vgui("        set fullpath [file normalize [file join $dir $filename]]\n");
    sys_vgui("        set dir [file dirname $fullpath]\n");
    sys_vgui("        set filename [file tail $fullpath]\n");
    sys_vgui("        menu_doc_open $dir $filename\n");
    sys_vgui("    } else {\n");
    sys_vgui("        pdtk_post \"openfile: $filename can't be opened\n\"\n");
    sys_vgui("    }\n");
    sys_vgui("}\n");
}
