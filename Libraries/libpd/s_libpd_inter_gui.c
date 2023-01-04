/* Copyright (c) 2022 IOhannes m zmölnig.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Pd side of the Pd/Pd-gui interface. */

#include "m_pd.h"
#include "s_stuff.h"
#include "s_libpd_inter.h"
#include <stdarg.h>
#include <string.h>

/* NULL-terminated */
#define GUI_VMESS__END     0
/* use space to structure the format-string */
#define GUI_VMESS__IGNORE  ' '

/* floating point number (automatically promoted to double) */
#define GUI_VMESS__FLOAT   'f'
/* fixed point number (automatically promoted to int) */
#define GUI_VMESS__INT     'i'

/* 0-terminated strings are untrusted and need escaping */
#define GUI_VMESS__STRING  's'
/* the same goes for char arrays ("pascalstring") */
#define GUI_VMESS__PASCALSTRING 'p'
/* rawstrings don't need encoding */
#define GUI_VMESS__RAWSTRING  'r'

/* takes an int-representation of a 24bit RGB color */
#define GUI_VMESS__COLOR      'k'

/* generic pointer; this is not actually used, and probably should stay that way */
#define GUI_VMESS__POINTER    'x'
/* a Pd-object */
#define GUI_VMESS__OBJECT     'o'

/* takes a Pd-message of the form "t_symbol*, int argc, t_atom*argv" */
#define GUI_VMESS__MESSAGE    'm'

/* arrays are capitalized */
#define GUI_VMESS__ATOMS          'a' /* flat list of atoms */
#define GUI_VMESS__ATOMARRAY      'A'
#define GUI_VMESS__FLOATARRAY     'F'
#define GUI_VMESS__FLOATWORDS     'w' /* flat list of float words */
#define GUI_VMESS__FLOATWORDARRAY 'W'
#define GUI_VMESS__STRINGARRAY    'S'
#define GUI_VMESS__RAWSTRINGARRAY 'R'

/* canvases */
#define GUI_VMESS__CANVAS 'c'
#define GUI_VMESS__CANVASARRAY 'C'

/* toplevel windows are legacy: this should go away (use 'c' instead) */
#define GUI_VMESS__WINDOW '^'

/* more ideas for types (the IDs need discussion)
 * - float32 array ('1'), float64 array ('2'): think libpd
 * - symbols ('y'), symbolarray ('Y'): this is just a shorthand for 's',
 *   so probably overkill
 * - t_word array ('W')

 * - a continuation char ('+'), to keep formating and values close together:
 *   e.g.  pdgui_vmess(..., "i+", 12, "i+", 13);
 */


static PERTHREAD char* s_escbuffer = 0;
static PERTHREAD size_t s_esclength = 0;
#ifndef GUI_ALLOCCHUNK
# define GUI_ALLOCCHUNK 8192
#endif
static char*get_escapebuffer(const char*s, int size)
{
    size_t len = (size>0)?size:strlen(s);
        /* worst case needs escaping each character; AND a terminating \0... */
    len = 2*len + 1;
    if (len > s_esclength) {
        freebytes(s_escbuffer, s_esclength);
        s_esclength = GUI_ALLOCCHUNK*(1+len/GUI_ALLOCCHUNK);
        s_escbuffer = getbytes(s_esclength);
    }
    return s_escbuffer;
}
static const char* str_escape(const char*s, int size)
{
    if(!s)
        return s;
    if (!get_escapebuffer(s, size))
        return 0;
    return pdgui_strnescape(s_escbuffer, s_esclength, s, size);
}

typedef struct _val {
    int type;
    int size;
    const char* string;
    union
    {
        double d;
        int i;
        const void*p;
    } value;
} t_val;

//#define DEBUGME
/* constructs a string that can be passed on to sys_gui() */
#ifdef DEBUGME
static void print_val(t_val v) {
    char str[80];
    dprintf(2, "value[%c] ", v.type);
    switch (v.type) {
    case GUI_VMESS__ATOMS:
        atom_string(v.value.p, str, sizeof(str));
        dprintf(2, "(atom)%s", str);
        break;
    case GUI_VMESS__FLOAT:
        dprintf(2, "(float)%f", v.value.d);
        break;
    case GUI_VMESS__INT:
        dprintf(2, "(int)%d", v.value.i);
        break;
    case GUI_VMESS__COLOR:
        dprintf(2, "(color)#%06x", v.value.i & 0xFFFFFF);
        break;
    case GUI_VMESS__STRING:
        dprintf(2, "(string)\"%s\"", v.value.p);
        break;
    case GUI_VMESS__PASCALSTRING:
        dprintf(2, "(pascalstring)\"%.*s\"", v.size, v.value.p);
        break;
    case GUI_VMESS__RAWSTRING:
        dprintf(2, "(rawstring)\"%s\"", v.value.p);
        break;
    case GUI_VMESS__WINDOW:
    case GUI_VMESS__CANVAS:
        dprintf(2, "(glist)%p", v.value.p);
            /* estimate the size required for the array */
        break;
    case GUI_VMESS__ATOMARRAY:
    case GUI_VMESS__FLOATARRAY:
    case GUI_VMESS__FLOATWORDS:
    case GUI_VMESS__FLOATWORDARRAY:
    case GUI_VMESS__STRINGARRAY:
    case GUI_VMESS__RAWSTRINGARRAY:
    case GUI_VMESS__POINTER:
    case GUI_VMESS__OBJECT:
    case GUI_VMESS__CANVASARRAY:
        dprintf(2, "%dx @%p", v.size, v.value.p);
            /* estimate the size required for the array */
        break;
    case GUI_VMESS__MESSAGE:
        dprintf(2, "(message)%s %d@%p", v.string, v.size, v.value.p);
        break;
    default:
        break;
    }
    dprintf(2, "\n");
}
#else
static void print_val(t_val v) { ; }
int dprintf(int fd, const char* format, ...) { return -1; }
#endif

static void sendatoms(int argc, t_atom*argv, int raw) {
    int i;
    for(i=0; i<argc; i++)
    {
        t_atom *a=argv++;
        switch (a->a_type) {
        default:
            break;
        case A_FLOAT:
                /* HACK: currently ATOMARRAY is only used for setting fonts
                 * and Tcl/Tk is picky when it receives non-integers as fontsize
                 */
            sys_vgui("%g ", atom_getfloat(a));
            break;
        case A_DOLLAR:
            if(raw)
                sys_vgui("$%d ", a->a_w.w_index);
            else
                sys_vgui("{$%d} ", a->a_w.w_index);
            break;
        case A_DOLLSYM:
        case A_SYMBOL:
            if(raw)
                sys_vgui("%s ", a->a_w.w_symbol->s_name);
            else
                sys_vgui("{%s} ", str_escape(a->a_w.w_symbol->s_name, 0));
            break;
        case A_POINTER:
            sys_vgui("%p ", a->a_w.w_gpointer);
            break;
        case A_SEMI:
            sys_vgui("\\; ");
            break;
        case A_COMMA:
            if (raw)
                sys_vgui(", ");
            else
                sys_vgui("{,} ");
            break;
        }
    }
}

static int addmess(const t_val *v)
{
    int i;
    char escbuf[MAXPDSTRING];
        //dprintf(2, "add-message: "); print_val(v);
    switch (v->type) {
    case GUI_VMESS__IGNORE:
        break;
    case GUI_VMESS__FLOAT:
        sys_vgui("%g", v->value.d);
        break;
    case GUI_VMESS__INT:
        sys_vgui("%d", v->value.i);
        break;
    case GUI_VMESS__COLOR:
        sys_vgui("#%06x", v->value.i & 0xFFFFFF);
        break;
    case GUI_VMESS__RAWSTRING:
        sys_vgui("%s", v->value.p);
        break;
    case GUI_VMESS__STRING:
        sys_vgui("{%s}", str_escape(v->value.p, 0));
        break;
    case GUI_VMESS__PASCALSTRING:
        sys_vgui("{%s}", str_escape(v->value.p, v->size));
        break;
    case GUI_VMESS__POINTER:
    case GUI_VMESS__OBJECT:
        sys_vgui("%p", v->value.p);
        break;
    case GUI_VMESS__MESSAGE:
        sys_vgui("{");
        if (v->string)
            sys_vgui("%s ", v->string);
        else
            ;
        sendatoms(v->size, (t_atom*)v->value.p, 1);
        sys_vgui("}");
        break;
    case GUI_VMESS__WINDOW:
        sys_vgui(".x%lx", v->value.p);
        break;
    case GUI_VMESS__CANVAS:
        sys_vgui(".x%lx.c", v->value.p);
        break;
    case GUI_VMESS__CANVASARRAY:
    {
        const t_canvas**data = (const t_canvas**)v->value.p;
        sys_vgui("{");
        for(i=0; i<v->size; i++)
            sys_vgui(".x%lx.c ", data[i]);
        sys_vgui("}");
        break;
    }
    case GUI_VMESS__FLOATARRAY:
    {
        const t_float*data = (const t_float*)v->value.p;
        sys_vgui("{");
        for(i=0; i<v->size; i++)
            sys_vgui("%f ", *data++);
        sys_vgui("}");
        break;
    }
    case GUI_VMESS__FLOATWORDS:
    case GUI_VMESS__FLOATWORDARRAY:
    {
        const t_word*data = (const t_word*)v->value.p;
        if (GUI_VMESS__FLOATWORDARRAY == v->type)
            sys_vgui("{");
        for(i=0; i<v->size; i++)
            sys_vgui("%g ", data[i].w_float);
        if (GUI_VMESS__FLOATWORDARRAY == v->type)
            sys_vgui("}");
        break;
    }
    case GUI_VMESS__RAWSTRINGARRAY:
    case GUI_VMESS__STRINGARRAY:
    {
        const char**data = (const char**)v->value.p;
        sys_vgui("{");
        for(i=0; i<v->size; i++)
        {
            const char*s=data[i];
            if (GUI_VMESS__RAWSTRINGARRAY == v->type)
                sys_vgui("%s ", s);
            else
                sys_vgui("{%s} ", str_escape(s, 0));
        }
        sys_vgui("}");
        break;
    }
    case GUI_VMESS__ATOMS:
    case GUI_VMESS__ATOMARRAY:
    {
        if (GUI_VMESS__ATOMARRAY == v->type)
            sys_vgui("{");
        sendatoms(v->size, (t_atom*)v->value.p, 0);
        if (GUI_VMESS__ATOMARRAY == v->type)
            sys_vgui("}");
        break;
    }
    default:
        return 1;
    }
    return 0;
}

static int va2value(const char fmt, va_list *args, t_val*v) {
    int result = 1;
    v->type = fmt;
    v->size = 1;
    switch (fmt) {
    case GUI_VMESS__IGNORE: /* <space> */
    case '\n': case '\t': /* other whitespace */
        v->type = GUI_VMESS__IGNORE;
        break;
    case GUI_VMESS__ATOMS:
        v->size = va_arg(*args, int);
        v->value.p = va_arg(*args, t_atom*);
        break;
    case GUI_VMESS__FLOAT:
        v->value.d = va_arg(*args, double);
        break;
    case GUI_VMESS__INT:
    case GUI_VMESS__COLOR:
        v->value.i = va_arg(*args, int);
        break;
    case GUI_VMESS__RAWSTRING:
    case GUI_VMESS__STRING:
    case GUI_VMESS__OBJECT:
    case GUI_VMESS__POINTER:
    case GUI_VMESS__WINDOW:
    case GUI_VMESS__CANVAS:
        v->value.p = va_arg(*args, void*);
        break;
    case GUI_VMESS__ATOMARRAY:
    case GUI_VMESS__CANVASARRAY:
    case GUI_VMESS__FLOATARRAY:
    case GUI_VMESS__FLOATWORDS:
    case GUI_VMESS__FLOATWORDARRAY:
    case GUI_VMESS__STRINGARRAY:
    case GUI_VMESS__RAWSTRINGARRAY:
    case GUI_VMESS__PASCALSTRING:
        v->size = va_arg(*args, int);
        v->value.p = va_arg(*args, void*);
        break;
    case GUI_VMESS__MESSAGE:
    {
        t_symbol*s = va_arg(*args, t_symbol*);
        v->string = s?s->s_name:0;
        v->size = va_arg(*args, int);
        v->value.p = va_arg(*args, void*);
        break;
    }
    default:
        result = v->size = 0;
        fprintf(stderr, "pdgui_vmess: unknown type-ID %d ('%c')\n", fmt, fmt);
        break;
    }
    return result;
}


void pdgui_vamess(const char* message, const char* format, va_list args_)
{
    const char* fmt;
    char* buf;
    t_val v;
    va_list args;

    v.type = GUI_VMESS__RAWSTRING;
    v.size = 1;
    v.value.p = message;

    if(message) {
        addmess(&v);
        sys_vgui("%s", " ");
    }

    va_copy(args, args_);
        /* iterate over the format-string and add elements */
    for(fmt = format; *fmt; fmt++) {
        if(va2value(*fmt, &args, &v) < 1)
            continue;
        addmess(&v);
        if(GUI_VMESS__IGNORE != v.type)
            sys_vgui("%s", " ");
    }
    va_end(args);
}
void pdgui_endmess(void)
{
    t_val v;
    v.type = GUI_VMESS__RAWSTRING;
    v.size = 1;
    v.value.p = ";\n";
    addmess(&v);
}


/* constructs a string that can be passed on to sys_gui() */
/* TODO: shouldn't this have a pointer to t_pdinstance? */
void pdgui_vmess(const char* message, const char* format, ...)
{
    if (message && strlen(message) == strlen("::pd_menucommands::menu_openfile") && strncmp(message, "::pd_menucommands::menu_openfile", strlen("::pd_menucommands::menu_openfile")) == 0) {
        va_list args;
        va_start(args, format);
        const char* str = va_arg(args, const char*);
        trigger_open_file(str);
        va_end(args);
        
    }
    
    va_list args;
    if (!sys_havegui())return;
    if(!format)
    {
        if (message)
            sys_vgui("%s;\n", message);
        return;
    }
    va_start(args, format);
    pdgui_vamess(message, format, args);
    va_end(args);
    pdgui_endmess();
    }
