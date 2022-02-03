/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//Derek Kwan 2016 - redoing fromsymbol_symbol, fromsymbol_separator, adding mtok(), isnums()

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "m_pd.h"
#include <common/api.h>

int isnums(const char *s){
    while(*s){ // check if every char is a digit or period
        if(isdigit(*s) == 0 && *s != '.' && *s != '-' && *s != '+' && *s != 'e'  && *s != 'E')
            return(0);
        s++;
    };
    return(1);
}

char *mtok(char *input, char *delimiter) {
    // adapted from stack overflow - Derek Kwan
    // designed to work like strtok
    static char *string;
    //mif not passed null, use static var
    if(input != NULL)
        string = input;
    //if reached the end, just return the static var, i think
    if(string == NULL)
        return string;
    //return pointer of first occurence of delim
    //added, keep going until first non delim
    char *end = strstr(string, delimiter);
    while(end == string){
        *end = '\0';
        string = end + strlen(delimiter);
        end = strstr(string, delimiter);
    };
    //if not found, just return the string
    if(end == NULL){
        char *temp = string;
        string = NULL;
        return temp;
    }
    char *temp = string;
    // set thing pointed to at end as null char, advance pointer to after delim
    *end = '\0';
    string = end + strlen(delimiter);
    return(temp);
}

typedef struct _fromsymbol{
    t_object   x_ob;
    t_symbol  *x_separator;
}t_fromsymbol;

static t_class *fromsymbol_class;

static void fromsymbol_bang(t_fromsymbol *x){
    outlet_bang(((t_object *)x)->ob_outlet);
}

static void fromsymbol_float(t_fromsymbol *x, t_float f){
    outlet_float(((t_object *)x)->ob_outlet, f);
}

static void fromsymbol_separator(t_fromsymbol *x, t_symbol *s, int argc, t_atom * argv){
    s = NULL;
    // hack to make it detect " " - Derek Kwan
    int numq = 0; //number of quotes in a row, if mod 2 == 0 and  > 0, count as space
    int set = 0; //if separator is set
    while(argc){
        t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
        if(strcmp(cursym->s_name, "@separator")!=0){
            x->x_separator = cursym;
            set = 1;
        };
        if(strcmp(cursym->s_name, "\"\0") == 0 || strcmp(cursym->s_name, "'\0") == 0)
            numq++;
        argc--;
        argv++;
    };
    if((numq > 0 && numq % 2 == 0) || set == 0) //if follows quote conditions or not set
        x->x_separator = gensym(" ");
}

static void fromsymbol_symbol(t_fromsymbol *x, t_symbol *s){
    //new and redone - Derek Kwan
    long unsigned int seplen = strlen(x->x_separator->s_name);
    seplen++;
    char *sep = t_getbytes(seplen * sizeof(*sep));
    memset(sep, '\0', seplen);
    strcpy(sep, x->x_separator->s_name);
    if(s){
        // get length of input string
        long unsigned int iptlen = strlen(s->s_name);
        // allocate t_atom [] on length of string
        // hacky way of making sure there's enough space
        t_atom* out = t_getbytes(iptlen * sizeof(*out));
        iptlen++;
        char *newstr = t_getbytes(iptlen * sizeof(*newstr));
        memset(newstr, '\0', iptlen);
        strcpy(newstr, s->s_name);
        int atompos = 0; //position in atom
        // parsing by token
        char *ret = mtok(newstr, sep);
        char *err; // error pointer
        while(ret != NULL){
            if(strlen(ret) > 0){
                int allnums = isnums(ret); // flag if all nums
                if(allnums){ // if errpointer is at beginning, that means we've got a float
                    double f = strtod(ret, &err);
                    SETFLOAT(&out[atompos], (t_float)f);
                }
                else{ // else we're dealing with a symbol
                    t_symbol * cursym = gensym(ret);
                    SETSYMBOL(&out[atompos], cursym);
                };
                atompos++; //increment position in atom
            };
            ret = mtok(NULL, sep);
        };
        if(out->a_type == A_SYMBOL)
            outlet_anything(((t_object *)x)->ob_outlet, out->a_w.w_symbol, atompos-1, out+1);
        else if(out->a_type == A_FLOAT && atompos >= 1)
            outlet_list(((t_object *)x)->ob_outlet, &s_list, atompos, out);
        t_freebytes(out, iptlen * sizeof(*out));
        t_freebytes(newstr, iptlen * sizeof(*newstr));
    };
    t_freebytes(sep, seplen * sizeof(*sep));
}

static void fromsymbol_list(t_fromsymbol *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(argv->a_type == A_FLOAT){
        t_float f = atom_getfloatarg(0, argc, argv);
        outlet_float(((t_object *)x)->ob_outlet, f);
    }
}

static void fromsymbol_anything(t_fromsymbol *x, t_symbol *s, int ac, t_atom *av){
    ac = 0, av = NULL;
    fromsymbol_symbol(x, s);
}

static void *fromsymbol_new(t_symbol * s, int argc, t_atom * argv){
    s = NULL;
    t_fromsymbol *x = (t_fromsymbol *)pd_new(fromsymbol_class);
    if(argc >= 1){
        t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
        if(curarg == gensym("@separator")){
            argc--;
            argv++;
            fromsymbol_separator(x, 0, argc, argv);
        }
        else
            goto errstate;
    }
    else
        fromsymbol_separator(x, 0, 0, 0);
    outlet_new((t_object *)x, &s_anything);
    return(x);
	errstate:
		pd_error(x, "fromsymbol: improper args");
		return(NULL);
}

CYCLONE_OBJ_API void fromsymbol_setup(void){
    fromsymbol_class = class_new(gensym("fromsymbol"), (t_newmethod)fromsymbol_new, 0,
        sizeof(t_fromsymbol), 0, A_GIMME, 0);
    class_addbang(fromsymbol_class, fromsymbol_bang);
    class_addfloat(fromsymbol_class, fromsymbol_float);
    class_addsymbol(fromsymbol_class, fromsymbol_symbol);
    class_addlist(fromsymbol_class, fromsymbol_list);
    class_addanything(fromsymbol_class, fromsymbol_anything);
    class_addmethod(fromsymbol_class, (t_method)fromsymbol_separator, gensym("separator"), A_GIMME, 0);
}
