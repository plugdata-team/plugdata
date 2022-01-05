// porres

#include "m_pd.h"

typedef struct else_obj{
    t_object x_obj;
}t_else_obj;

t_class *else_obj_class;

static int printed;

static int min_major = 0;
static int min_minor = 51;
static int min_bugfix = 3;

static int else_major = 1;
static int else_minor = 0;
static int else_bugfix = 0;

#define STATUS "beta"
static int status_number = 38;

static void else_obj_version(t_else_obj *x){
    int ac = 5;
    t_atom at[5];
    SETFLOAT(at, else_major);
    SETFLOAT(at+1, else_minor);
    SETFLOAT(at+2, else_bugfix);
    SETSYMBOL(at+3, gensym(STATUS));
    SETFLOAT(at+4, status_number);
    outlet_list(x->x_obj.te_outlet,  &s_list, ac, at);
}

void print_else_obj(t_else_obj *x){
    int major = 0, minor = 0, bugfix = 0;
    sys_getversion(&major, &minor, &bugfix);
    post("");
    post("-------------------------------------------------------------------------------");
    post("  ~~~~~~~~~~|| ELSE - EL Locus Solus' Externals for Pure Data ||~~~~~~~~~~");
    post("-------------------------------------------------------------------------------");
    post("- Version: 1.0-0 %s-%d; Released april 14th 2021", STATUS, status_number);
    post("- Author: Alexandre Torres Porres");
    post("- Repository: https://github.com/porres/pd-else");
    post("- License: Do What The Fuck You Want To Public License, unless otherwise noted");
    if(major >= min_major && minor >= min_minor && bugfix >= min_bugfix)
        post("- ELSE 1.0.-0 %s-%d needs at least Pd %d.%d-%d (you have %d.%d-%d, you're good!)",
             STATUS, status_number, min_major, min_minor, min_bugfix, major, minor, bugfix);
    else
        pd_error(x, "- ELSE 1.0-0 %s-%d needs at least Pd %d.%d-%d (you have %d.%d-%d, please upgrade!)",
            STATUS, status_number, min_major, min_minor, min_bugfix, major, minor, bugfix);
    post("-------------------------------------------------------------------------------");
    post("- NOTE: This library also includes a tutorial by Alexandre Torres Porres");
    post("that depends on this library. Find the 'live-electronics-folder' folder");
    post("inside the 'else' folder. Please check its README on how to install it!");
    post("-------------------------------------------------------------------------------");
    post("  ~~~~~~~~~~|| ELSE - EL Locus Solus' Externals for Pure Data ||~~~~~~~~~~");
    post("-------------------------------------------------------------------------------");
    post("");
}

static void else_obj_about(t_else_obj *x){
    print_else_obj(x);
}

static void *else_obj_new(void){
    t_else_obj *x = (t_else_obj *)pd_new(else_obj_class);
    if(!printed){
        else_obj_about(x);
        printed = 1;
    }
    outlet_new((t_object *)x, 0);
    return(x);
}

void else_setup(void){
    else_obj_class = class_new(gensym("else"), else_obj_new, 0, sizeof(t_else_obj), 0, 0);
    t_else_obj *x = (t_else_obj *)pd_new(else_obj_class);
    class_addmethod(else_obj_class, (t_method)else_obj_about, gensym("about"), 0);
    class_addmethod(else_obj_class, (t_method)else_obj_version, gensym("version"), 0);
   if(!printed){
       print_else_obj(x);
       printed = 1;
    }
}
