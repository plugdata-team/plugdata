/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKME list of indices */

#include <stdio.h>
#include "m_pd.h"
#include <common/api.h>
#include "g_canvas.h"
#include "common/file.h"
#include <string.h>
#include <errno.h>

//string and errno for fail: in capture_dowrite, copying over functionality from loud.c
#define CAPTURE_DEFSIZE     4096
#define CAPTURE_DEFPRECISION   4
#define CAPTURE_MAXPRECISION  99  /* format array protection */
#define CAPTURE_MAXINDICES  4096  /* FIXME */

typedef struct _capture{
    t_object       x_obj;
    t_inlet       *x_inlet;
    t_glist       *x_glist;
    char           x_mode;  /* 'f' for first or 0 for last */
    int            x_precision;
    char           x_format[8];
    char          *x_indices;
    int            x_szindices;  /* size of x_indices array */
    int            x_nindices;   /* number of reported indices */
    int            x_nblock;
    float         *x_buffer;
    int            x_bufsize;
    int            x_count;
    int            x_head;
    t_file  *x_filehandle;
} t_capture;

static t_class *capture_class;

static int capture_formatfloat(t_capture *x, float f, char *buf, int col, int maxcol){
    char *bp = buf;
    int cnt = 0;
    if (col > 0)
	*bp++ = ' ', cnt++;
    if (x->x_precision)
	cnt += sprintf(bp, x->x_format, f);
    else
	cnt += sprintf(bp, "%d", (int)f);
    if (col + cnt > maxcol)
	buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
    else
	col += cnt;
    return (col);
}

static int capture_writefloat(t_capture *x, float f, char *buf, int col, FILE *fp)
{
    /* CHECKME linebreaks */
    col = capture_formatfloat(x, f, buf, col, 80);
    return(fputs(buf, fp) < 0 ? -1 : col);
}

static void capture_dowrite(t_capture *x, t_symbol *fn)
{
    FILE *fp = 0;
    int count = x->x_count;
    char buf[MAXPDSTRING];
    canvas_makefilename(glist_getcanvas(x->x_glist),
			fn->s_name, buf, MAXPDSTRING);
    if ((fp = sys_fopen(buf, "w")))  /* LATER ask if overwriting, CHECKME */
    {
	int col = 0;
	if (x->x_mode == 'f' || count < x->x_bufsize)
	{
	    float *bp = x->x_buffer;
	    while (count--)
		if ((col = capture_writefloat(x, *bp++, buf, col, fp)) < 0)
		    goto fail;
	}
	else
	{
	    float *bp = x->x_buffer + x->x_head;
	    count = x->x_bufsize - x->x_head;
	    while (count--)
		if ((col = capture_writefloat(x, *bp++, buf, col, fp)) < 0)
		    goto fail;
	    bp = x->x_buffer;
	    count = x->x_head;
	    while (count--)
		if ((col = capture_writefloat(x, *bp++, buf, col, fp)) < 0)
		    goto fail;
	}
	if (col) fputc('\n', fp);
	fclose(fp);
	return;
    }
fail:
    if (fp)
        fclose(fp);
    pd_error(x, "capture~: %s", strerror(errno));
}

static void capture_writehook(t_pd *z, t_symbol *fn, int ac, t_atom *av)
{
    ac = 0;
    av = NULL;
    capture_dowrite((t_capture *)z, fn);
}

static void capture_write(t_capture *x, t_symbol *s)
{
    if (s && s != &s_)
	capture_dowrite(x, s);
    else
	panel_save(x->x_filehandle, 0, 0);
}

static int capture_appendfloat(t_capture *x, float f, char *buf, int col, int linebreak){
    /* CHECKME 80 columns */
    col = capture_formatfloat(x, f, buf, col, 80);
    editor_append(x->x_filehandle, buf);
    if(linebreak){
        if(col){
            editor_append(x->x_filehandle, "\n\n");
            col = 0;
        }
        else
            editor_append(x->x_filehandle, "\n");
    }
    return(col);
}

static void capture_open(t_capture *x){ // CHECKED blank line between blocks
    int count = x->x_count;
    char buf[MAXPDSTRING];
    int nindices = (x->x_nindices > 0 ? x->x_nindices : x->x_nblock);
    editor_open(x->x_filehandle, "Signal Capture", "");  /* CHECKED */
    if(x->x_mode == 'f' || count < x->x_bufsize){
        float *bp = x->x_buffer;
        int col = 0, i;
        for(i = 1; i <= count; i++)
            col = capture_appendfloat(x, *bp++, buf, col, ((i % nindices) == 0));
    }
    else{
        float *bp = x->x_buffer + x->x_head;
        int col = 0, i = x->x_bufsize;
        count = x->x_bufsize - x->x_head;
        while (count--)
            col = capture_appendfloat(x, *bp++, buf, col, ((--i % nindices) == 0));
        bp = x->x_buffer;
        count = x->x_head;
        while(count--)
            col = capture_appendfloat(x, *bp++, buf, col, ((count % nindices) == 0));
    }
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  wm deiconify .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  raise .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  focus .%lx.text\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
}

static void capture_clear(t_capture *x){
    x->x_count = 0;
    x->x_head = 0;
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
}

/* CHECKED without asking and storing the changes */
static void capture_wclose(t_capture *x)
{
    editor_close(x->x_filehandle, 0);
}

static void capture_click(t_capture *x, t_floatarg xpos, t_floatarg ypos,
			  t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    alt = ctrl = shift = ypos = xpos = 0;
    capture_open(x);
}

static t_int *capture_perform_first(t_int *w){
    t_capture *x = (t_capture *)(w[1]);
    int count = x->x_count;
    int bufsize = x->x_bufsize;
    if(count < bufsize){
        t_float *in = (t_float *)(w[2]);
        int nblock = (int)(w[3]);
        float *bp = x->x_buffer + count;
        char *ndxp = x->x_indices;
        if(nblock > x->x_szindices)
            nblock = x->x_szindices;
        while(nblock--){
            if(*ndxp++){
                *bp++ = *in++;
                if(++count == bufsize)
                    break;
            }
            else
                in++;
        }
        x->x_count = count;
    }
    return(w+4);
}

static t_int *capture_perform_allfirst(t_int *w){
    t_capture *x = (t_capture *)(w[1]);
    int count = x->x_count;
    int bufsize = x->x_bufsize;
    if(count < bufsize){
        t_float *in = (t_float *)(w[2]);
        int nblock = (int)(w[3]);
        float *bp = x->x_buffer + count;
        while (nblock--){
            *bp++ = *in++;
            if(++count == bufsize)
                break;
        }
        x->x_count = count;
    }
    return(w+4);
}

static t_int *capture_perform_last(t_int *w){
    t_capture *x = (t_capture *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int nblock = (int)(w[3]);
    float *buffer = x->x_buffer;
    int bufsize = x->x_bufsize;
    int count = x->x_count;
    int head = x->x_head;
    char *ndxp = x->x_indices;
    if (nblock > x->x_szindices)
        nblock = x->x_szindices;
    while (nblock--){
        if(*ndxp++){
            buffer[head++] = *in++;
            if(head >= bufsize)
                head = 0;
            if(count < bufsize)
                count++;
        }
        else
            in++;
    }
    x->x_count = count;
    x->x_head = head;
    return(w+4);
}

static t_int *capture_perform_alllast(t_int *w){
    t_capture *x = (t_capture *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int nblock = (int)(w[3]);
    float *buffer = x->x_buffer;
    int bufsize = x->x_bufsize;
    int count = x->x_count;
    int head = x->x_head;
    while (nblock--){
        buffer[head++] = *in++;
        if(head >= bufsize)
            head = 0;
        if(count < bufsize)
            count++;
    }
    x->x_count = count;
    x->x_head = head;
    return(w+4);
}

static void capture_dsp(t_capture *x, t_signal **sp){
    x->x_nblock = sp[0]->s_n;
    if(x->x_indices)
        dsp_add((x->x_mode == 'f' ?
        capture_perform_first : capture_perform_last),
		3, x, sp[0]->s_vec, sp[0]->s_n);
    else
        dsp_add((x->x_mode == 'f' ?
        capture_perform_allfirst : capture_perform_alllast),
		3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void capture_free(t_capture *x)
{
    file_free(x->x_filehandle);
    if (x->x_indices)
	freebytes(x->x_indices, x->x_szindices * sizeof(*x->x_indices));
    if (x->x_buffer)
	freebytes(x->x_buffer, x->x_bufsize * sizeof(*x->x_buffer));
}

static void *capture_new(t_symbol *s, int ac, t_atom *av)
{
    s = NULL;
    t_capture *x = 0;
    char mode = 0;
    int precision = -1;
    float *buffer;
    int bufsize = 0;
    char *indices = 0;
    int szindices = 0, nindices = -1;
    if (ac && av->a_type == A_SYMBOL)
    {
	t_symbol *cursym = av->a_w.w_symbol;
	if (cursym && *cursym->s_name == 'f')  /* CHECKME */
	    mode = 'f';
	ac--; av++;
    }
    if (ac && av->a_type == A_FLOAT)
    {
	bufsize = (int)av->a_w.w_float;  /* CHECKME */
	ac--; av++;
	if (ac && av->a_type == A_FLOAT)
	{
	    int i;
	    t_atom *ap;
	    precision = (int)av->a_w.w_float;  /* CHECKME */
	    ac--; av++;
	    for (i = 0, ap = av; i < ac; i++, ap++)
	    {
		if (ap->a_type == A_FLOAT)
		{
		    int ndx = (int)ap->a_w.w_float;
		    /* CHECKME noninteger, negative */
		    ndx++;
		    if (ndx >= CAPTURE_MAXINDICES)
		    {
			/* CHECKME complaint */
			szindices = CAPTURE_MAXINDICES;
			break;
		    }
		    else if (ndx > szindices)
			szindices = ndx;
		}
		else break;  /* CHECKME */
	    }
	    if (szindices && (indices = getbytes(szindices * sizeof(*indices))))
	    {
		nindices = 0;
		while (i--)
		{
		    int ndx = (int)av++->a_w.w_float;
		    /* CHECKME noninteger */
		    if (ndx >= 0 && ndx < szindices)
			indices[ndx] = 1, nindices++;
		}
	    }
	}
    }
    if (bufsize <= 0)  /* CHECKME */
	bufsize = CAPTURE_DEFSIZE;
    if ((buffer = getbytes(bufsize * sizeof(*buffer))))
    {
	x = (t_capture *)pd_new(capture_class);
	x->x_glist = canvas_getcurrent();
	x->x_mode = mode;
	if (precision < 0)  /* CHECKME */
	    precision = CAPTURE_DEFPRECISION;
	else if (precision > CAPTURE_MAXPRECISION)  /* CHECKME */
	    precision = CAPTURE_MAXPRECISION;
	if ((x->x_precision = precision))
	    sprintf(x->x_format, "%%.%df", precision);
	x->x_indices = indices;
	x->x_szindices = szindices;
	x->x_nindices = nindices;
	x->x_nblock = 64;  /* redundant */
	x->x_buffer = buffer;
	x->x_bufsize = bufsize;
	x->x_filehandle = file_new((t_pd *)x, 0, 0, capture_writehook, 0);
	capture_clear(x);
    }
    else if (indices)
	freebytes(indices, szindices * sizeof(*indices));
    return (x);
}

CYCLONE_OBJ_API void capture_tilde_setup(void)
{
    capture_class = class_new(gensym("capture~"),
			      (t_newmethod)capture_new,
			      (t_method)capture_free,
			      sizeof(t_capture), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(capture_class, nullfn, gensym("signal"), 0);
    class_addmethod(capture_class, (t_method) capture_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(capture_class, (t_method)capture_clear,
		    gensym("clear"), 0);
    class_addmethod(capture_class, (t_method)capture_write,
		    gensym("write"), A_DEFSYM, 0);
    class_addmethod(capture_class, (t_method)capture_open,
		    gensym("open"), 0);
    class_addmethod(capture_class, (t_method)capture_wclose,
		    gensym("wclose"), 0);
    class_addmethod(capture_class, (t_method)capture_click,
		    gensym("click"),
		    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    file_setup(capture_class, 0);
}
