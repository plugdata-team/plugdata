// by schiavoni and porres 2017-2020

#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>

static t_class *median_class;

typedef struct _median {
    t_object     x_obj;
    t_inlet     *median;
    t_float      x_samples;
    t_float     *x_temp;
    t_int        x_block_size;
    t_outlet    *x_outlet;
}t_median;

void median_sort(t_float *a, int n) {
    if(n < 2)
        return;
    t_float p = a[n / 2];
    t_float *l = a;
    t_float *r = a + n - 1;
    while(l <= r){
        while(*l < p)
            l++;
        while(*r > p)
            r--;
        if(l <= r){
            t_float t = *l;
            *l++ = *r;
            *r-- = t;
        }
    }
    median_sort(a, r - a + 1);
    median_sort(l, a + n - l);
}

t_float median_calculate(t_float * array, int begin, int end){
    median_sort(array + begin, end - begin + 1);
    int qtd = end - begin + 1;
    t_float median = 0;
    median = (qtd%2 == 1) ? array[begin + ((qtd - 1)/2)] :
        (array[begin + qtd / 2 - 1] + array[begin + qtd / 2])/2.0f;
    return(median);
}

static t_int * median_perform(t_int *w){
    t_median *x = (t_median *)(w[1]);
    t_int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out1 = (t_float *)(w[4]);
    int i = 0;
    for(i = 0 ; i < n ; i++)
        x->x_temp[i] = in1[i];
    if(x->x_samples > n)
        x->x_samples = n;
    if(x->x_samples < 1)
        x->x_samples = 1;
    int begin = 0;
    int end = x->x_samples - 1;
    t_float median = 0;
    while(begin < n){
        median = median_calculate(x->x_temp, begin, end);
        i = 0;
        for(i = begin; i < end + 1; i++)
            out1[i] = median;
        begin += x->x_samples;
        end = (end + x->x_samples > n)? n - 1 : end + x->x_samples;
    }
    return(w+5);
}

static void median_dsp(t_median *x, t_signal **sp){
    t_int block = (t_int)sp[0]->s_vec;
    if(block != x->x_block_size){
        x->x_block_size = (t_int)sp[0]->s_vec;
        x->x_temp = realloc(x->x_temp, sizeof(t_float)*x->x_block_size);
    }
    dsp_add(median_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void median_free(t_median *x){
    free(x->x_temp);
}

void * median_new(t_floatarg f) {
    t_median *x = (t_median *) pd_new(median_class);
    x->x_samples = (f < 1) ? 1 : f;
    x->x_block_size = 1;
    x->x_temp = (t_float *)malloc(x->x_block_size * sizeof(t_float));
    x->x_outlet = outlet_new(&x->x_obj, &s_signal); // outlet
    floatinlet_new(&x->x_obj, &x->x_samples);
    return(void *)x;
}

void median_tilde_setup(void) {
    median_class = class_new(gensym("median~"), (t_newmethod) median_new,
        (t_method) median_free, sizeof (t_median), 0, A_DEFFLOAT, 0);
    class_addmethod(median_class, nullfn, gensym("signal"), 0);
    class_addmethod(median_class, (t_method) median_dsp, gensym("dsp"), A_CANT, 0);
}
