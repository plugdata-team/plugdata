// Porres 2018

#include "m_pd.h"
#include "g_canvas.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static t_class *initmess_class;

typedef struct _initmess{
  t_object      x_obj;
  t_int         x_ac;
  t_int         x_first;
  t_atom       *x_atom;
  t_atom       *x_av;
  t_symbol     *x_s;
  t_symbol     *x_sel;
}t_initmess;

static void initmess_send(t_initmess *x, t_symbol *s, int ac, t_atom *av){
    if(!ac){ // only a selector (but not ";" or ",")
        if(!strcmp(s->s_name, ";") || !strcmp(s->s_name, ",")){
        }  // ignore
        else // output selector
            outlet_anything(x->x_obj.ob_outlet, s, 0, 0);
    }
    else{ // Not only a Selector
        char separator = gensym(",")->s_name[0]; // split character
        char send = gensym(";")->s_name[0]; // send character
        t_int send_flag = 0;
        t_int last_send_flag = 0, send_this = 0;
        t_int last_comma_flag = 0, split_this = 0;
        t_int i = 0, j = 0, n = 0;
        t_symbol *send_sym = NULL;
        while(i < ac){
            if(last_send_flag == 1)
                send_flag = 1; // from now on, send all the shit
            send_this = last_send_flag;
            split_this = last_comma_flag;
            if(send_this && split_this)
                send_this = 0;
            for(j = i; j < ac; j++){ // <=====    FOR
                if((av+j)->a_type == A_SYMBOL && atom_getsymbol(av+j)->s_name[0] == send){
                    last_comma_flag = 0; // next break point is a send, so comma = 0
                    last_send_flag = 1; // update for next round cause next break point is a send
                    break;
                }
                else if((av+j)->a_type == A_SYMBOL && atom_getsymbol(av+j)->s_name[0] == separator){
                    last_send_flag = 0;  // next break is a comma, so fuck this
                    last_comma_flag = 1; // update for next round cause next break point is a comma
                    break;
                }
            } // <=====    END OF FOR
            n = j - i; // j = breakpoint / n = elements in a initmess - counting the comma/semicolon 1st element
            if(i == 0){ // <=====    FIRST initmess!!!!
                if(n == 0){ // the initmess only contains a selector
                    if(!strcmp(s->s_name, "list")) // if selector is list, turn to bang
                        outlet_bang(x->x_obj.ob_outlet);
                    else if(!strcmp(s->s_name, ";") || !strcmp(s->s_name, ",")){
                    }    // ignore ";" or ","
                    else // output selector
                        outlet_anything(x->x_obj.ob_outlet, s, n, av-1);
                }
                else{ // it's not only the selector
                    if(!strcmp(s->s_name, ";")){ // IT'S A SEND!!!!!!
                        if((av)->a_type == A_FLOAT)
                            send_sym = gensym("float");
                        else if((av)->a_type == A_SYMBOL)
                            send_sym = atom_getsymbol(av);
                        if(send_sym->s_thing && n > 1){ // there's a receive and a initmess, send it!
                            if((av+1)->a_type == A_FLOAT)
                                typedmess(send_sym->s_thing, &s_list, n-1, av+1);
                            else if((av+1)->a_type == A_SYMBOL)
                                typedmess(send_sym->s_thing, atom_getsymbol(av+1), n-2, av+2);
                        }
                        last_send_flag = 1; // mark it as a flag
                    }
                    else if(!strcmp(s->s_name, ",")){ // Selector is a comma, so ignore it and output the rest
                        if((av)->a_type == A_FLOAT)
                            outlet_anything(x->x_obj.ob_outlet, &s_list, n, av);
                        else if((av)->a_type == A_SYMBOL)
                            outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(av), n-1, av+1);
                    }
                    else // output the initmess (can be the whole initmess if there's no breakage)
                        outlet_anything(x->x_obj.ob_outlet, s, n, av);
                }
            }
            else{ // <=====    NEXT initmessS!!!!
                if(send_flag){ // we're sending stuff
                    if(send_this){ // it's a new send!
                        if((av+i)->a_type == A_FLOAT)
                            send_sym = gensym("float");
                        else if((av+i)->a_type == A_SYMBOL)
                            send_sym = atom_getsymbol(av+i);
                        if(send_sym->s_thing && n > 1){ // there's a receive and a initmess, send it!
                            if((av+i+1)->a_type == A_FLOAT)
                                typedmess(send_sym->s_thing, &s_list, n-1, av+i+1);
                            else if((av+i+1)->a_type == A_SYMBOL)
                                typedmess(send_sym->s_thing, atom_getsymbol(av+i+1), n-2, av+i+2);
                        }
                    }
                    else if(split_this){ // it's an old send
                        if(send_sym->s_thing){ // there's a receive and a initmess, send it!
                            if((av+i)->a_type == A_FLOAT)
                                typedmess(send_sym->s_thing, &s_list, n, av+i);
                            else if((av+i)->a_type == A_SYMBOL){
                                if((!strcmp(atom_getsymbol(av+i)->s_name, ",") ||
                                    !strcmp(atom_getsymbol(av+i)->s_name, ";")) && !n){
                                } // ignore if the initmess is only a comma or semicolon selector
                                else
                                    typedmess(send_sym->s_thing, atom_getsymbol(av+i), n-1, av+i+1);
                            }
                        }
                    }
                }
                else{ // we're *NOT* sending stuff
                    if((av+i)->a_type == A_SYMBOL){
                        if((!strcmp(atom_getsymbol(av+i)->s_name, ",") ||
                            !strcmp(atom_getsymbol(av+i)->s_name, ";")) && !n){
                        } // ignore if the initmess is only a comma or semicolon selector
                        else
                            outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(av+i), n-1, av + i+1);
                    }
                    else if((av+i)->a_type == A_FLOAT)
                        outlet_anything(x->x_obj.ob_outlet, &s_list, n, av+i);
                }
            }
            i = j + 1; // next start point
        } // <=====    END OF WHILE
    }
}

static void initmess_output(t_initmess *x){
    if(x->x_first){
        if(x->x_av){
            x->x_atom = (t_atom *)getbytes((x->x_ac+1) * sizeof(t_atom));
            for(int i = 0; i < x->x_ac; i++)
                x->x_atom[i] = x->x_av[i];
            initmess_send(x, x->x_sel, x->x_ac, x->x_atom);
        }
        x->x_first = 0;
    }
    else if(x->x_av)
        initmess_send(x, x->x_sel, x->x_ac, x->x_atom);
}

static void initmess_loadbang(t_initmess *x, t_floatarg action){
    if(action == LB_LOAD)
        initmess_output(x);
}

static void initmess_free(t_initmess *x){
  if(x->x_av)
    freebytes(x->x_av, x->x_ac * sizeof(t_atom));
}

static void *initmess_new(t_symbol *s, int ac, t_atom *av){
    t_initmess *x = (t_initmess *)pd_new(initmess_class);
    x->x_first = 1;
    x->x_sel = s; // get rid of warning
    if(!ac){
        x->x_ac = 0;
        x->x_av = NULL;
        x->x_sel = NULL;
    }
    else{
        if(av->a_type == A_SYMBOL){ // get selector
            x->x_s = x->x_sel = atom_getsymbol(av++);
            ac--;
        }
        else
            x->x_s = x->x_sel = &s_list; // list selector otherwise
        x->x_ac = ac;
        x->x_av = x->x_atom = (t_atom *)getbytes(x->x_ac * sizeof(t_atom));
        int i;
        for(i = 0; i < ac; i++)
            x->x_av[i] = x->x_atom[i] = av[i];
    }
    outlet_new(&x->x_obj, &s_anything);
    return(x);
}

void initmess_setup(void){
  initmess_class = class_new(gensym("initmess"), (t_newmethod)initmess_new,
        (t_method)initmess_free, sizeof(t_initmess), 0, A_GIMME, 0);
  class_addmethod(initmess_class, (t_method)initmess_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
  class_addbang(initmess_class, (t_method)initmess_output);
  class_addmethod(initmess_class, (t_method)initmess_output, gensym("click"), 0);
}
