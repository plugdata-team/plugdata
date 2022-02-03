/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//Derek Kwan 2017 
//notes on changes: v2 ignored floats in int modes, this is not the case anymore
//also ignored all symbols, also no longer the case.

//OVERVIEW OF MODES AS DESCRIBED BY PORRES for v3 (maybe diff than before)
//in every mode, symbols are printed just as they are
//other modes: floats with mantissas (mantissae?) always displayed as floats
//other in 'd': integer as decimals
//'x': integers in hex
//'m': < 128, integers are decimal, > ints are hex
//'a': all numbers invisible

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/file.h"

#define CAPTURE_DEFSIZE  512
#define CAPTURE_DEFPREC 4 //default precision
#define CAPTURE_MAXPREC 99 //
#define CAPTURE_MINPREC 1 //minimum preision
#define CAPTURE_MAXCOL 80 

typedef struct _capture
{
    t_object       x_ob;
    t_canvas      *x_canvas;
    char           x_intmode;
    t_atom         *x_buffer;
    int            x_bufsize; //number of stored values
    int            x_count; //number of values actually in buffer
    unsigned int   x_counter; //counting number of values received
    int            x_head;
    int            x_precision;
    t_outlet * x_count_outlet;
    t_file  *x_filehandle;
} t_capture;

static t_class *capture_class;

static void capture_symbol(t_capture *x, t_symbol *s)
{
    SETSYMBOL(&x->x_buffer[x->x_head], s) ;
    x->x_head = x->x_head + 1;
    //post("%s", x->x_buffer[x->x_head ].a_w.w_symbol->s_name);
    if (x->x_head >= x->x_bufsize)
	x->x_head = 0;
    if (x->x_count < x->x_bufsize)
	x->x_count++;
    x->x_counter++;


}

static void capture_precision(t_capture *x, t_float f)
{
	int precision;
	precision = f > CAPTURE_MINPREC ? (int)f : CAPTURE_MINPREC;
    precision = precision > CAPTURE_MAXPREC ? CAPTURE_MAXPREC : precision;
    x->x_precision = precision;
}

//counts number of items received since last count
static void capture_count(t_capture *x)
{
    outlet_float(x->x_count_outlet, (t_float)x->x_counter);
    x->x_counter = 0;
}

static void capture_dump(t_capture *x)
{
    int count = x->x_count;
    int i;
    //post("bufsize: %d, count: %d", x->x_bufsize, count);
    if (count < x->x_bufsize)
    {
        for(i=0; i < count; i++)
        {
            //printf("%ddump\n", i);
            if(x->x_buffer[i].a_type == A_FLOAT)
            {
                //printf("float\n");
	        outlet_float(((t_object *)x)->ob_outlet, x->x_buffer[i].a_w.w_float);
            }
            else if (x->x_buffer[i].a_type == A_SYMBOL)
            {
                //printf("symbol\n");
	        outlet_symbol(((t_object *)x)->ob_outlet, x->x_buffer[i].a_w.w_symbol);
            };
                
        };
    }
    else
    {
        //this is for wrapping around and rewriting old values
        //for the proper input order while dumping
        int reali;
        for(i=0; i < x->x_bufsize; i++)
        {
            reali = (x->x_head + i) % x->x_bufsize;
            if(x->x_buffer[reali].a_type == A_FLOAT)
	        outlet_float(((t_object *)x)->ob_outlet, x->x_buffer[reali].a_w.w_float);
            else if (x->x_buffer[reali].a_type == A_SYMBOL)
	        outlet_symbol(((t_object *)x)->ob_outlet, x->x_buffer[reali].a_w.w_symbol);
        };
    };
}

//used by formatnumber in tern used by appendfloat to write to editor window
static int capture_formatint(int i, char *buf, int col,
			     int maxcol, char *fmt)
{
    char *bp = buf;
    int cnt = 0;
    if (col > 0)
	*bp++ = ' ', cnt++;
    cnt += sprintf(bp, fmt, i);
    if (col + cnt > maxcol)
	buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
    else
	col += cnt;
    return (col);
}

//used by capture_formatnumber used by append float to write to ed window
static int capture_formatfloat(t_capture *x, float f, char *buf, int col,
			       int maxcol, char *fmt)
{
    char *bp = buf;
    int precision = x->x_precision;
    int cnt = 0;
    //not at beginning, have to enter space before
    if (col > 0)
		*bp++ = ' ', cnt++;
    cnt += sprintf(bp, fmt, f, precision);
    //if too many columns for ed window, start on new line (instead of space)
    if (col + cnt > maxcol)
	buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
    else
	col += cnt;
    return (col);
}

static int capture_insertblank(char *buf, int col, int maxcol)
{
    //basically follow the gameplan of inserting a float, but only one character
    //so insert a space before if in the middle of a col, etc.
     char *bp = buf;
    int cnt = 0;
    //not at beginning, have to enter space before
    if (col > 0)
		*bp++ = ' ', cnt++;
    cnt += sprintf(bp, "%s", " ");
    //if too many columns for ed window, start on new line (instead of space)
    if (col + cnt > maxcol)
	buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
    else
	col += cnt;
    return (col);

}

//called by capture_append float to add floats to x->x_buffer for editor window
static int capture_formatnumber(t_capture *x, float f, char *buf,
				int col, int maxcol)
{
    char intmode = x->x_intmode;
    if (intmode == 'm')
	intmode = (f < 128 && f > -128 ? 'd' : 'x');  /* CHECKME */
    //first cond: for modes x,m,d,
    if ((f != (int)f) && intmode != 'a')
	col = capture_formatfloat(x, f, buf, col, maxcol, "%.*f");
    else if (intmode == 'x')
	col = capture_formatint((int)f, buf, col, maxcol, "%x");
    else if (intmode == 'd')
	col = capture_formatint((int)f, buf, col, maxcol, "%d");
    else 
        col = capture_insertblank(buf, col, maxcol);
    return (col);
}

//called by capture_dowrite to write floats to a file
static int capture_writefloat(t_capture *x, float f, char *buf, int col,
			      FILE *fp)
{
    /* CHECKED no linebreaks (FIXME) */
    col = capture_formatnumber(x, f, buf, col, CAPTURE_MAXCOL);
    return (fputs(buf, fp) < 0 ? -1 : col);
}

static int capture_writesymbol(t_symbol *s, char *buf, int col, FILE *fp)
{
    /* keeping this just in case we want to convert to ascii at some point
    int i;
    int symlen = strlen(s->s_name);
    unsigned char c;
    char intmode = x->x_intmode;
    if(intmode=='a')
    {
        for(i=0; i<symlen; i++)
        {
            c = s->s_name[i];
            col = capture_formatnumber(x, (t_float)c, buf, col, CAPTURE_MAXCOL);
            col = fputs(buf, fp) < 0 ? -1 : col;
            if(col < 0) break;
         };
    }
    */
        char *bp = buf;
        int cnt = 0;

        if (col > 0)
        {   *bp++ = ' ';
            cnt++;
        
        }
        cnt += sprintf(bp, "%s", s->s_name);
        //if too many columns for ed window, start on new line (instead of space)
        if (col + cnt > CAPTURE_MAXCOL)
            buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
        else
            col += cnt;
        col = fputs(buf, fp) < 0 ? -1 : col;

    return (col);
}

//called by capture_write method
static void capture_dowrite(t_capture *x, t_symbol *fn){
    FILE *fp = 0;
    int i, count = x->x_count;
    char buf[MAXPDSTRING];
    canvas_makefilename(x->x_canvas, fn->s_name, buf, MAXPDSTRING);
    if ((fp = sys_fopen(buf, "w"))){  /* LATER ask if overwriting, CHECKED */
        int col = 0;
        if (count < x->x_bufsize){
            for(i=0; i < count ; i++){
                if(x->x_buffer[i].a_type == A_FLOAT)
                    col = capture_writefloat(x, x->x_buffer[i].a_w.w_float, buf, col, fp);
                else if (x->x_buffer[i].a_type == A_SYMBOL)
                    col = capture_writesymbol(x->x_buffer[i].a_w.w_symbol, buf, col, fp);
                if(col < 0)
                    goto fail;
            };
        }
        else{
            //this is for wrapping around and rewriting old values
            //for the proper input order while dumping
            int reali;
            for(i=0; i < x->x_bufsize; i++){
                reali = (x->x_head + i) % x->x_bufsize;
                if(x->x_buffer[reali].a_type == A_FLOAT)
                    col = capture_writefloat(x, x->x_buffer[reali].a_w.w_float, buf, col, fp);
                else if (x->x_buffer[reali].a_type == A_SYMBOL)
                    col = capture_writesymbol(x->x_buffer[reali].a_w.w_symbol, buf, col, fp);
                if(col < 0)
                    goto fail;
            };
        }
        if (col) fputc('\n', fp);
            fclose(fp);
        return;
    }
    fail:
    if (fp)
        fclose(fp);
    pd_error(x, "capture: %s", strerror(errno));
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

static int capture_appendsymbol(t_capture *x, t_symbol *s, char *buf, int col)
{
    /* keeping this just in case we decide we want to convert to ascii
     * at some point
    int i;
    int symlen = strlen(s->s_name);
    unsigned char c;
    char intmode = x->x_intmode;
    if(intmode == 'a')
    {   //convert into ascii
        for(i=0; i<symlen; i++)
        {
            c = s->s_name[i];
            col = capture_formatnumber(x, (t_float)c, buf, col, CAPTURE_MAXCOL);
            editor_append(x->x_filehandle, buf);
        };
    }
    */
        char *bp = buf;
        int cnt = 0;

        if (col > 0)
        {   *bp++ = ' ';
            cnt++;
        
        }
        cnt += sprintf(bp, "%s", s->s_name);
        //if too many columns for ed window, start on new line (instead of space)
        if (col + cnt > CAPTURE_MAXCOL)
            buf[0] = '\n', col = cnt - 1;  /* assuming col > 0 */
        else
            col += cnt;
        editor_append(x->x_filehandle, buf);


    return (col);
}

//use by capture_open (opens editor window) to append floats to x->x_buffer
static int capture_appendfloat(t_capture *x, float f, char *buf, int col)
{
    /* CHECKED CAPTURE_MAXCOL columns */
    col = capture_formatnumber(x, f, buf, col, CAPTURE_MAXCOL);
    editor_append(x->x_filehandle, buf);
    return (col);
}

static void capture_update(t_capture *x){
    int count = x->x_count;
    char buf[MAXPDSTRING];
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  .%lx.text delete 1.0 end\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
    int i;
    if(count < x->x_bufsize){
        int col = 0;
        for(i = 0; i < count; i++){
            if(x->x_buffer[i].a_type == A_FLOAT)
            col = capture_appendfloat(x, x->x_buffer[i].a_w.w_float, buf, col);
            else if (x->x_buffer[i].a_type == A_SYMBOL)
            col = capture_appendsymbol(x, x->x_buffer[i].a_w.w_symbol, buf, col);
                
        };
    }
    else{
        //this is for wrapping around and rewriting old values
        //for the proper input order while dumping
        int reali;
        int col = 0;
        for(i = 0; i < x->x_bufsize; i++){
            reali = (x->x_head + i) % x->x_bufsize;
            if(x->x_buffer[reali].a_type == A_FLOAT)
            col = capture_appendfloat(x, x->x_buffer[reali].a_w.w_float, buf, col);
            else if (x->x_buffer[reali].a_type == A_SYMBOL)
            col = capture_appendsymbol(x, x->x_buffer[reali].a_w.w_symbol, buf, col);
        };
    };
}

static void capture_open(t_capture *x){
    int count = x->x_count;
    char buf[MAXPDSTRING];
    int i;
    editor_open(x->x_filehandle, "Capture", "");  /* CHECKED */
    if(count < x->x_bufsize){
        int col = 0;
        for(i = 0; i < count; i++){
            if(x->x_buffer[i].a_type == A_FLOAT)
	        col = capture_appendfloat(x, x->x_buffer[i].a_w.w_float, buf, col);
            else if (x->x_buffer[i].a_type == A_SYMBOL)
	        col = capture_appendsymbol(x, x->x_buffer[i].a_w.w_symbol, buf, col);
                
        };
    }
    else{
        //this is for wrapping around and rewriting old values
        //for the proper input order while dumping
        int reali;
        int col = 0;
        for(i = 0; i < x->x_bufsize; i++){
            reali = (x->x_head + i) % x->x_bufsize;
            if(x->x_buffer[reali].a_type == A_FLOAT)
	        col = capture_appendfloat(x, x->x_buffer[reali].a_w.w_float, buf, col);
            else if (x->x_buffer[reali].a_type == A_SYMBOL)
	        col = capture_appendsymbol(x, x->x_buffer[reali].a_w.w_symbol, buf, col);
        };
    };
    sys_vgui(" if {[winfo exists .%lx]} {\n", (unsigned long)x->x_filehandle);
    sys_vgui("  wm deiconify .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  raise .%lx\n", (unsigned long)x->x_filehandle);
    sys_vgui("  focus .%lx.text\n", (unsigned long)x->x_filehandle);
    sys_gui(" }\n");
}

/* CHECKED without asking and storing the changes */
static void capture_wclose(t_capture *x)
{
    editor_close(x->x_filehandle, 0);
}

static void capture_click(t_capture *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    alt = ctrl = shift = ypos = xpos;
    capture_open(x);
}

static void capture_free(t_capture *x)
{
    outlet_free(x->x_count_outlet);
    file_free(x->x_filehandle);
    if (x->x_buffer)
	freebytes(x->x_buffer, x->x_bufsize * sizeof(*x->x_buffer));
}

static void capture_float(t_capture *x, t_float f)
{
    SETFLOAT(&x->x_buffer[x->x_head], f) ;
    //post("%f", x->x_buffer[x->x_head].a_w.w_float);
    x->x_head = x->x_head + 1;
    if (x->x_head >= x->x_bufsize)
    x->x_head = 0;
    if (x->x_count < x->x_bufsize)
    x->x_count++;
    x->x_counter++;
    capture_update(x);
}

static void capture_list(t_capture *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    while(ac--){
        if (av->a_type == A_FLOAT)  /* CHECKME */
            capture_float(x, av->a_w.w_float);
        else if(av->a_type == A_SYMBOL)
            capture_symbol(x, av->a_w.w_symbol);
        av++;
    }
    capture_update(x);
}

static void capture_anything(t_capture *x, t_symbol *s, int argc, t_atom * argv){
    //basically copying over mtrack_anything here for not including list selectors 
    if(s && argv){
        if(strcmp(s->s_name, "list") != 0 || argv->a_type != A_FLOAT){
            //copy list to new t_atom with symbol as first elt
            int destpos = 0; //position in copied list
            int atsize = argc + 1;
            t_atom* at = t_getbytes(atsize * sizeof(*at));
            SETSYMBOL(&at[destpos], s);
            destpos++;
            int arrpos = 0; //position in arriving list
            for(destpos=1; destpos<argc+1; destpos++){
                if((argv+arrpos)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(arrpos, argc, argv);
                    SETFLOAT(&at[destpos], curfloat);
                }
                else{
                    t_symbol * cursym = atom_getsymbolarg(arrpos, argc, argv);
                    SETSYMBOL(&at[destpos], cursym);
                };
            //increment
            arrpos++;
            };
            capture_list(x, s, argc+1, at);
            t_freebytes(at, atsize * sizeof(*at));
        }
        else
            capture_list(x, s, argc, argv);
    }
    else
        capture_list(x, s, argc, argv);
    capture_update(x);
}

//clear stored contents
static void capture_clear(t_capture *x)
{
    x->x_count = 0;
    x->x_head = 0;
    capture_update(x);
}

static void *capture_new(t_symbol *s, int argc, t_atom * argv)
{
    s = NULL;
    t_capture *x = 0;
    t_atom  *buffer;
    int bufsize;
    int precision;
    t_float _bufsize = CAPTURE_DEFSIZE;
    t_float _precision = CAPTURE_DEFPREC;
    t_symbol * dispfmt = NULL;
    while(argc){
        if(argv->a_type == A_FLOAT){
            _bufsize = atom_getfloatarg(0, argc, argv);
            argc--;
            argv++;
        }
        else if(argv->a_type == A_SYMBOL){
            t_symbol * curargt = atom_getsymbolarg(0, argc, argv);
            argc--;
            argv++;
            if((strcmp(curargt->s_name, "@precision") == 0) && argc){
                _precision = atom_getfloatarg(0, argc, argv);
                argc--;
                argv++;
            }
            else{
                dispfmt = curargt;
            };
        };
    };
    
    precision = _precision > CAPTURE_MINPREC ? (int)_precision : CAPTURE_MINPREC;
    precision = precision > CAPTURE_MAXPREC ? CAPTURE_MAXPREC : precision;
    bufsize = _bufsize > 0 ? (int)_bufsize : CAPTURE_DEFSIZE; 
    
    if ((buffer = getbytes(bufsize * sizeof(*buffer))))
    {
	x = (t_capture *)pd_new(capture_class);
	x->x_canvas = canvas_getcurrent();
	if (dispfmt)
	{
	    if (dispfmt == gensym("x"))
		x->x_intmode = 'x';
	    else if (dispfmt == gensym("m"))
		x->x_intmode = 'm';
            else if (dispfmt == gensym("a"))
                x->x_intmode = 'a';
	    else
		x->x_intmode = 'd'; 
	}
        else x->x_intmode = 'd'; 

        x->x_count = 0;
        x->x_counter = 0;
        x->x_head = 0;
        x->x_precision = precision;
	x->x_buffer = buffer;
	x->x_bufsize = bufsize;
	outlet_new((t_object *)x, &s_anything);
    x->x_count_outlet = outlet_new((t_object *)x, &s_float);
	x->x_filehandle = file_new((t_pd *)x, 0, 0, capture_writehook, 0);
	capture_clear(x);
    }
    return (x);
}

CYCLONE_OBJ_API void capture_setup(void){
    capture_class = class_new(gensym("capture"), (t_newmethod)capture_new,
        (t_method)capture_free, sizeof(t_capture), 0, A_GIMME, 0);
    class_addfloat(capture_class, capture_float);
    class_addlist(capture_class, capture_list);
    class_addsymbol(capture_class, capture_symbol);
    class_addanything(capture_class, capture_anything);
    class_addmethod(capture_class, (t_method)capture_precision, gensym("precision"), A_FLOAT, 0);
    class_addmethod(capture_class, (t_method)capture_clear, gensym("clear"), 0);
    class_addmethod(capture_class, (t_method)capture_count, gensym("count"), 0);
    class_addmethod(capture_class, (t_method)capture_dump, gensym("dump"), 0);
    class_addmethod(capture_class, (t_method)capture_write, gensym("write"), A_DEFSYM, 0);
    class_addmethod(capture_class, (t_method)capture_open, gensym("open"), 0);
    class_addmethod(capture_class, (t_method)capture_wclose, gensym("wclose"), 0);
    class_addmethod(capture_class, (t_method)capture_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    file_setup(capture_class, 0);
}
