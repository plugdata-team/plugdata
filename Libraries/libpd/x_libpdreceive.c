/*
 * Copyright (c) 2010 Peter Brinkmann (peter.brinkmann@gmail.com)
 * Copyright (c) 2012-2019 libpd team
 *
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/libpd/libpd/wiki for documentation
 *
 */

#include <stdio.h>
#include "x_libpdreceive.h"
#include "z_libpd.h"
#include "z_hooks.h"

static t_class *libpdrec_class;

typedef struct _libpdrec {
    t_object x_obj;
    t_symbol *x_sym;
} t_libpdrec;

static void libpdrecbang(t_libpdrec *x) {
  if (libpd_banghook) (*libpd_banghook)(x->x_sym->s_name);
}

static void libpdrecfloat(t_libpdrec *x, t_float f) {
  if (libpd_floathook) (*libpd_floathook)(x->x_sym->s_name, f);
}

static void libpdrecsymbol(t_libpdrec *x, t_symbol *s) {
  if (libpd_symbolhook) (*libpd_symbolhook)(x->x_sym->s_name, s->s_name);
}

static void libpdrecpointer(t_libpdrec *x, t_gpointer *gp) {
  // just ignore pointers for now...
}

static void libpdreclist(t_libpdrec *x, t_symbol *s, int argc, t_atom *argv) {
  if (libpd_listhook) (*libpd_listhook)(x->x_sym->s_name, argc, argv);
}

static void libpdrecanything(t_libpdrec *x, t_symbol *s,
                int argc, t_atom *argv) {
  if (libpd_messagehook)
    (*libpd_messagehook)(x->x_sym->s_name, s->s_name, argc, argv);
}

static void libpdreceive_free(t_libpdrec *x) {
  pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static void *libpdreceive_donew(t_symbol *s) {
  t_libpdrec *x;
  x = (t_libpdrec *)pd_new(libpdrec_class);
  x->x_sym = s;
  pd_bind(&x->x_obj.ob_pd, s);
  return x;
}

// this is exposed in the libpd API so must set the lock
void *libpdreceive_new(t_symbol *s) {
  t_libpdrec *x;
  sys_lock();
  x = (t_libpdrec *)libpdreceive_donew(s);
  sys_unlock();
  return x;
}

void libpdreceive_setup(void) {
  sys_lock();
  libpdrec_class = class_new(gensym("libpd_receive"),
       (t_newmethod)libpdreceive_donew, (t_method)libpdreceive_free,
       sizeof(t_libpdrec), CLASS_DEFAULT, A_DEFSYM, 0);
  class_addbang(libpdrec_class, libpdrecbang);
  class_addfloat(libpdrec_class, libpdrecfloat);
  class_addsymbol(libpdrec_class, libpdrecsymbol);
  class_addpointer(libpdrec_class, libpdrecpointer);
  class_addlist(libpdrec_class, libpdreclist);
  class_addanything(libpdrec_class, libpdrecanything);
  sys_unlock();
}
