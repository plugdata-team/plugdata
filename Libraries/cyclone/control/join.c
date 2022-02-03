//Work by Derek Kwan in 2016, built on the work of Pierre Guillot for pak


#include "m_pd.h"
#include <common/api.h>
#include<stdlib.h>
#include<string.h>

#define JOIN_MAXINLETS 255 

//taken from puredata's src/x_list.c

static void atoms_copy(int argc, t_atom *from, t_atom *to)
{
    int i;
    for (i = 0; i < argc; i++)
        to[i] = from[i];
}

// ---------------------------

static t_class *join_class;
static t_class* join_inlet_class;

struct _join_inlet;


typedef struct _join
{
    t_object            x_obj;
    int               x_numinlets;
	int 				x_totlen; //total len of all atoms from join_inlet
    struct _join_inlet*  x_ins;
} t_join;

typedef struct _join_inlet
{
    t_class*    x_pd;
    t_atom*     x_atoms;
    int 		x_numatoms;
	int 		x_trig; //trigger flag
	int 		x_id; //inlet number
	t_join*      x_owner;
} t_join_inlet;


static void *join_new(t_symbol *s, int argc, t_atom *argv)
{
    t_join *x = (t_join *)pd_new(join_class);
	t_float numinlets = 0;

	int * triggervals;
	int i;

	//parse first arg, should be giving the number of inlets
	if(argc > 0){
		if(argv -> a_type == A_FLOAT){
			numinlets = atom_getfloatarg(0, argc, argv);
			argc--;
			argv++;
		};
	};
	x->x_numinlets = ((int)numinlets < 2) ? 2 : ((int)numinlets > JOIN_MAXINLETS) ? JOIN_MAXINLETS : (int)numinlets;
	triggervals = (int *)calloc(x->x_numinlets,sizeof(int));
	triggervals[0] = 1; //default, only left inlet is hot

	//the next arg should be for the attribute @trigger
	if(argc > 0){
		if(argv -> a_type == A_SYMBOL){
			t_symbol * curarg = atom_getsymbolarg(0, argc, argv);
			if(strcmp(curarg->s_name, "@triggers") == 0){
				//triggers are specified, make the left inlet cold (for now)
				triggervals[0] = 0;
				argc--;
				argv++;
				while(argc > 0){
					//if there's stuff left to parse and we don't go beyond numinlets
					t_float curval = atom_getfloatarg(0, argc, argv);
					if(curval == -1){
						//this sets all inlets to be hot
						for(i=0; i<x->x_numinlets; i++){
								triggervals[i] = 1;
						};
						break;
					}
					else{
						int trigidx = (int)curval;
						if(trigidx >= 0 && trigidx < x->x_numinlets){
							triggervals[trigidx] = 1;
						};
						argc--;
						argv++;
					};
				};
			};
		};

	};

    x->x_ins = (t_join_inlet *)getbytes(x->x_numinlets * sizeof(t_join_inlet));
    

	x->x_totlen = x->x_numinlets;

    for(i = 0; i < x->x_numinlets; ++i)
    {
            x->x_ins[i].x_pd    = join_inlet_class;
            x->x_ins[i].x_atoms = (t_atom *)getbytes(1 * sizeof(t_atom));
			SETFLOAT(x->x_ins[i].x_atoms, 0);
			x->x_ins[i].x_numatoms = 1;
			x->x_ins[i].x_owner = x;
            x->x_ins[i].x_trig = triggervals[i];
			x->x_ins[i].x_id = i;
            inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
    };
    outlet_new(&x->x_obj, &s_list);
	free(triggervals);
    return (x);
}

static void join_inlet_atoms(t_join_inlet *x, int argc, t_atom * argv ){
//setting of the atoms
	t_join * owner = x->x_owner;
	freebytes(x->x_atoms, x->x_numatoms * sizeof(t_atom));
	owner->x_totlen -= x->x_numatoms;

	x->x_atoms = (t_atom *)getbytes(argc * sizeof(t_atom));
	owner->x_totlen += argc;

	x->x_numatoms = (t_int)argc;
	atoms_copy(argc, argv, x->x_atoms);

}

static void* join_free(t_join *x)
{
	int i;
	for(i=0; i<x->x_numinlets; i++){ 
		//free the t_atoms of each inlet
		freebytes(x->x_ins[i].x_atoms, x->x_ins[i].x_numatoms*sizeof(t_atom));
	};
    freebytes(x->x_ins, x->x_numinlets * sizeof(t_join_inlet));
	return (void *)free;
}

static void join_output(t_join *x){

	int i;
	t_atom * outatom;
	outatom = (t_atom *)getbytes(x->x_totlen * sizeof(t_atom));

	int offset = 0;
		for(i=0; i < x->x_numinlets; i++){
				int curnum = x->x_ins[i].x_numatoms; //number of atoms for current inlet
				atoms_copy(curnum, x->x_ins[i].x_atoms, outatom + offset); //copy them over to outatom
				offset += curnum; //increment offset
		};

    outlet_list(x->x_obj.ob_outlet, &s_list, x->x_totlen, outatom);
	freebytes(outatom, x->x_totlen * sizeof(t_atom));

}

static void join_inlet_bang(t_join_inlet *x)
{
	//if(x->x_id == 0){
		join_output(x->x_owner);
	//};
}

static void join_inlet_list(t_join_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
		join_inlet_atoms(x, argc, argv);
		if(x->x_trig == 1){
			join_output(x->x_owner);
		};

}

static void join_inlet_anything(t_join_inlet *x, t_symbol* s, int argc, t_atom* argv)
{

	//we want to treat "bob tom" and "list bob tom" as the same
	//default way is to treat first symbol as selector, we don't want this!
	if(strcmp(s->s_name, "list") != 0){
		t_atom * tofeed = (t_atom *)getbytes((argc+1)*sizeof(t_atom));
		SETSYMBOL(tofeed, s);
		atoms_copy(argc, argv, tofeed+1);
		join_inlet_list(x, 0, argc+1, tofeed);
		freebytes(tofeed, (argc+1)*sizeof(t_atom));
	}
	else{
		join_inlet_list(x, 0, argc, argv);
	};
}


static void join_inlet_float(t_join_inlet *x, float f)
{
	t_atom * newatom;
	newatom = (t_atom *)getbytes(1 * sizeof(t_atom));
	SETFLOAT(newatom, f);
	join_inlet_list(x, 0, 1, newatom);
	freebytes(newatom, 1*sizeof(t_atom));
}

static void join_inlet_symbol(t_join_inlet *x, t_symbol* s)
{
	t_atom * newatom;
	newatom = (t_atom *)getbytes(1 * sizeof(t_atom));
	SETSYMBOL(newatom, s);
	join_inlet_list(x, 0, 1, newatom);
	freebytes(newatom, 1*sizeof(t_atom));


}


static void join_inlet_set(t_join_inlet *x, t_symbol* s, int argc, t_atom* argv)
{
    join_inlet_atoms(x, argc, argv);
}

static void join_inlet_triggers(t_join_inlet *x, t_symbol* s, int argc, t_atom* argv){
	if(x->x_id == 0){	
		//accept if only leftmost inlet, else ignore
		t_join * owner = x->x_owner;
		int i, trigidx;
		for(i=0; i< owner->x_numinlets; i++){
			//zero out all trigger values
			owner->x_ins[i].x_trig = 0;
		};

		//now start parsing inlet indices
		
		while(argc > 0){
			t_float argval = atom_getfloatarg(0, argc, argv);
			trigidx = (int)argval;
                        if(trigidx == -1){
                            for(i=0; i < owner->x_numinlets; i++){
                                owner->x_ins[i].x_trig = 1;
                            };
                            break;
                        };
			if(trigidx >= 0 && trigidx < owner->x_numinlets){
				owner->x_ins[trigidx].x_trig = 1;
			};
			argc--;
			argv++;
		};
	};

}
extern CYCLONE_OBJ_API void join_setup(void)
{
    t_class* c = NULL;
    
    c = class_new(gensym("join-inlet"), 0, 0, sizeof(t_join_inlet), CLASS_PD, 0);
    if(c)
    {
        class_addbang(c,    (t_method)join_inlet_bang);
        class_addfloat(c,   (t_method)join_inlet_float);
        class_addsymbol(c,  (t_method)join_inlet_symbol);
        class_addlist(c,    (t_method)join_inlet_list);
        class_addanything(c,(t_method)join_inlet_anything);
        class_addmethod(c,  (t_method)join_inlet_set, gensym("set"), A_GIMME, 0);
        class_addmethod(c,  (t_method)join_inlet_triggers, gensym("triggers"), A_GIMME, 0);
    }
    join_inlet_class = c;
    
    c = class_new(gensym("join"), (t_newmethod)join_new, (t_method)join_free, sizeof(t_join), CLASS_NOINLET, A_GIMME, 0);
    join_class = c;
}
