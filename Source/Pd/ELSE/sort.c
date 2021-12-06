// Based on matt barber's sort method for cyclone's zl

#include <string.h>
#include <stdlib.h>
#include "m_pd.h"

#define INISIZE 128
#define MAXSIZE 2147483647

typedef struct _sortdata{
    int      d_size;        // as allocated
    int      d_natoms;      // as used
    int		 d_dir;         // direction
    t_atom  *d_buf;
    t_atom   d_bufini[INISIZE];
}t_sortdata;

typedef struct _sort{
    t_object    x_obj;
    t_sortdata  x_inbuf1;
    t_sortdata  x_outbuf1;
    t_sortdata  x_outbuf2;
    float       x_dir;
    t_outlet   *x_out2;
}t_sort;

static t_class *sort_class;

static void swap(t_atom *av, int i, int j){
	t_atom temp = av[j];
	av[j] = av[i];
	av[i] = temp; 
}

static int compare(t_atom *a1, t_atom *a2){
	if(a1->a_type == A_FLOAT && a2->a_type == A_SYMBOL)
		return(-1);
	else if(a1->a_type == A_SYMBOL && a2->a_type == A_FLOAT)
		return(1);
	else if(a1->a_type == A_FLOAT && a2->a_type == A_FLOAT){
		if(a1->a_w.w_float < a2->a_w.w_float)
			return(-1);
		else if(a1->a_w.w_float > a2->a_w.w_float)
			return(1);
        else
            return(0);
	}
	else
		return(strcmp(a1->a_w.w_symbol->s_name, a2->a_w.w_symbol->s_name));
}

static void sort_qsort(t_sort *x, t_atom *av1, t_atom *av2, int left, int right, float dir){
    if(left >= right)
        return;
    swap(av1, left, (left + right)/2);
    if(av2)
    	swap(av2, left, (left + right)/2);
    int last = left;
    for(int i = left+1; i <= right; i++){
        if((dir * compare(av1+i, av1 + left)) < 0){
            swap(av1, ++last, i);
            if(av2)
            	swap(av2, last, i);
        }
    }
    swap(av1, left, last);
    if(av2) 
    	swap(av2, left, last);
    sort_qsort(x, av1, av2, left, last-1, dir);
    sort_qsort(x, av1, av2, last+1, right, dir);
}

static void sort_rev(int n, t_atom *av){
	for(int i = 0, j = n-1; i < n/2; i++, j--)
		swap(av, i, j);
}

static void sort_sort(t_sort *x, int natoms, t_atom *buf, int bang){
    x->x_dir = x->x_dir >= 0 ? 1 : -1;
	if(buf){
    	t_atom *buf2 = x->x_outbuf2.d_buf;
    	x->x_outbuf2.d_natoms = natoms;
    	if(bang){
    		if(x->x_inbuf1.d_dir != x->x_dir){
    			x->x_inbuf1.d_dir = x->x_dir;
    			sort_rev(natoms, buf2);
    			sort_rev(natoms, buf);
    		}
            outlet_list(x->x_out2, &s_list, natoms, buf2);
            outlet_list(((t_object *)x)->ob_outlet, &s_list, natoms, buf);
    	}
    	else{
			memcpy(buf, x->x_inbuf1.d_buf, natoms*sizeof(*buf));
			for(int i = 0; i < natoms; i++)
    			SETFLOAT(&buf2[i], i); // indexes
    		sort_qsort(x, buf, buf2, 0, natoms-1, x->x_inbuf1.d_dir = x->x_dir);
            outlet_list(x->x_out2, &s_list, natoms, buf2);
            outlet_list(((t_object *)x)->ob_outlet, &s_list, natoms, buf);
		}
    }
}

static void doit(t_sort *x, int bang){
    if(x->x_inbuf1.d_natoms){
        t_sortdata *d = &x->x_outbuf1;
        sort_sort(x, x->x_inbuf1.d_natoms, d->d_buf, bang);
    }
    else
        pd_error(x, "[sort]: empty buffer, no output");
}

static void sort_bang(t_sort *x){
    doit(x, 1);
}

static void sort_realloc(t_sortdata *d, int size){
    if(size > MAXSIZE)
        size = MAXSIZE;
    if(d->d_buf == d->d_bufini) // !heaped
        d->d_buf = getbytes(size*sizeof(t_atom));
    else // heaped
        d->d_buf = (t_atom *)resizebytes(d->d_buf, d->d_size*sizeof(t_atom), size*sizeof(t_atom));
    d->d_size = size;
}

static void set_size(t_sort *x, t_sortdata *d, int ac){
    if(ac > d->d_size){
        sort_realloc(&x->x_inbuf1, ac);
        sort_realloc(&x->x_outbuf1, ac);
        sort_realloc(&x->x_outbuf2, ac);
    }
    d->d_natoms = ac > d->d_size ? d->d_size : ac;
}

static void sort_list(t_sort *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    set_size(x, &x->x_inbuf1, ac);
    memcpy(x->x_inbuf1.d_buf, av, x->x_inbuf1.d_natoms*sizeof(*x->x_inbuf1.d_buf));
    doit(x, 0);
}

static void sort_anything(t_sort *x, t_symbol *s, int ac, t_atom *av){
    set_size(x, &x->x_inbuf1, ac+1);
    SETSYMBOL(x->x_inbuf1.d_buf, s);
    if(ac)
        memcpy(x->x_inbuf1.d_buf+1, av, (x->x_inbuf1.d_natoms-1)*sizeof(*x->x_inbuf1.d_buf));
    doit(x, 0);
}

static void sortdata_free(t_sortdata *d){
    if(d->d_buf != d->d_bufini)
        freebytes(d->d_buf, d->d_size*sizeof(*d->d_buf));
}

static void sort_free(t_sort *x){
    sortdata_free(&x->x_inbuf1);
    sortdata_free(&x->x_outbuf1);
    sortdata_free(&x->x_outbuf2);
}

static void sortdata_init(t_sortdata *d){
    d->d_natoms = 0;
    d->d_size = INISIZE;
    d->d_buf = d->d_bufini;
}

static void *sort_new(t_floatarg f){
    t_sort *x = (t_sort *)pd_new(sort_class);
    sortdata_init(&x->x_inbuf1);
    sortdata_init(&x->x_outbuf1);
    sortdata_init(&x->x_outbuf2);
    x->x_dir = f;
    floatinlet_new(&x->x_obj, &x->x_dir);
    outlet_new((t_object *)x, &s_list);
    x->x_out2 = outlet_new((t_object *)x, &s_list);
    return(x);
}

void sort_setup(void){
    sort_class = class_new(gensym("sort"), (t_newmethod)sort_new,
            (t_method)sort_free, sizeof(t_sort), 0, A_DEFFLOAT, 0);
    class_addbang(sort_class, sort_bang);
    class_addlist(sort_class, sort_list);
    class_addanything(sort_class, sort_anything);
}
