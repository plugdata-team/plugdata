// porres

#include "m_pd.h"
#include "m_imp.h"
#include <string.h>

typedef struct else_obj{
    t_object  x_obj;
    t_outlet *x_out2;
    t_outlet *x_out3;
}t_else_obj;

t_class *else_obj_class;

//static int printed;

static int min_major = 0;
static int min_minor = 53;
static int min_bugfix = 2;

static int else_major = 1;
static int else_minor = 0;
static int else_bugfix = 0;

#define STATUS "rc"
static int status_number = 8;

static void else_obj_version(t_else_obj *x){
    int ac = 5;
    t_atom at[5];
#ifdef PD_FLAVOR
    SETSYMBOL(at, gensym(PD_FLAVOR));
#ifdef PD_L2ORK_VERSION
    SETSYMBOL(at+1, gensym(PD_L2ORK_VERSION));
#elif defined(PD_PLUGDATA_VERSION)
    SETSYMBOL(at+1, gensym(PD_PLUGDATA_VERSION));
#endif
    outlet_list(x->x_out3,  &s_list, 2, at);
#else
    outlet_symbol(x->x_out3, gensym("Pd-Vanilla"));
#endif
    
    int major = 0, minor = 0, bugfix = 0;
    sys_getversion(&major, &minor, &bugfix);
    SETFLOAT(at+0, major);
    SETFLOAT(at+1, minor);
    SETFLOAT(at+2, bugfix);
    outlet_list(x->x_out2,  &s_list, 3, at);
    
    SETFLOAT(at, else_major);
    SETFLOAT(at+1, else_minor);
    SETFLOAT(at+2, else_bugfix);
    SETSYMBOL(at+3, gensym(STATUS));
    SETFLOAT(at+4, status_number);
    outlet_list(x->x_obj.te_outlet,  &s_list, ac, at);
}

void else_obj_about(t_else_obj *x){
    int major = 0, minor = 0, bugfix = 0;
    sys_getversion(&major, &minor, &bugfix);
    post("");
    post("-------------------------------------------------------------------");
    post("  -----> ELSE - EL Locus Solus' Externals for Pure Data <-----");
    post("-------------------------------------------------------------------");
    post("- Version: %d.%d-%d %s-%d; Released may 10th 2023", else_major, else_minor, else_bugfix, STATUS, status_number);
    post("- Author: Alexandre Torres Porres");
    post("- Repository: https://github.com/porres/pd-else");
    post("- License: Do What The Fuck You Want To Public License");
    post("(unless otherwise noted in particular objects, check the code)");
    if((major > min_major)
       || (major == min_major && minor > min_minor)
       || (major == min_major && minor == min_minor && bugfix >= min_bugfix)){
        post("- ELSE %d.%d-%d %s-%d needs at least Pd %d.%d-%d",
             else_major, else_minor, else_bugfix, STATUS, status_number,
             min_major, min_minor, min_bugfix);
        post("(you have %d.%d-%d, you're good!)", major, minor, bugfix);
    }
    else{
#ifndef PDL2ORK
        pd_error(x, "- ELSE %d.%d-%d %s-%d needs at least Pd %d.%d-%d",
            else_major, else_minor, else_bugfix, STATUS, status_number,
            min_major, min_minor, min_bugfix);
        pd_error(x, "(you have %d.%d-%d, please upgrade)", major, minor, bugfix);
#endif
    }
    post("-------------------------------------------------------------------");
    post("- NOTE: There's an accompanying tutorial by Alexandre Torres Porres.");
    post("You can find it as part of the ELSE library download, look for the");
    post("\"Live-Electronics-Tutorial\" folder inside the ELSE folder.");
    post("Please check its README on how to install it!");
    post("-------------------------------------------------------------------");
    post("- ALSO NOTE: Loading this binary did not install the ELSE library");
    post("you still need to add it to the \"preferences => path\"");
    post("or use [declare -path else]. But it did install a plugin for");
    post("browsing ELSE's objects when right clicking on a patch canvas!");
    post("-------------------------------------------------------------------");
    post("  -----> ELSE - EL Locus Solus' Externals for Pure Data <-----");
    post("-------------------------------------------------------------------");
    post("");
}

static void *else_obj_new(void){
    t_else_obj *x = (t_else_obj *)pd_new(else_obj_class);
    outlet_new((t_object *)x, 0);
    x->x_out2 = outlet_new((t_object *)x, &s_list);
    x->x_out3 = outlet_new((t_object *)x, &s_list);
    return(x);
}

void else_setup(void){
    else_obj_class = class_new(gensym("else"), (t_newmethod)else_obj_new, 0, sizeof(t_else_obj), 0, 0);
    t_else_obj *x = (t_else_obj *)pd_new(else_obj_class);
    class_addmethod(else_obj_class, (t_method)else_obj_about, gensym("about"), 0);
    class_addmethod(else_obj_class, (t_method)else_obj_version, gensym("version"), 0);
    else_obj_about(x);
    char else_dir[MAXPDSTRING];
    strcpy(else_dir, else_obj_class->c_externdir->s_name);
    sys_vgui("eval [read [open [file join %s else-browser.tcl]]]\n", else_obj_class->c_externdir->s_name);
}
