/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// notes: main inlet calls proper data handlers (zl_anything, etc), which deal with the input through zldata_add/set functions which fill inbuf1, THEN zl_doit is called which calls the "natoms_fn" (suffixed by _count) and calls the doitfn (usu just the mode name like zl_len) called on the OUTPUT buffer outbuf

#include <string.h>
#include <stdlib.h>
#include "m_pd.h"
#include <common/api.h>

// LATER test reentrancy, tune speedwise

#define ZL_DEF_SIZE    256    // default size
#define ZL_MINSIZE     1      // min size
#define ZL_MAXSIZE     32768  // max size
#define ZL_N_MODES     32     // number of modes

struct _zl;

typedef int (*t_zlintargfn)(struct _zl *, int);
typedef void (*t_zlanyargfn)(struct _zl *, t_symbol *, int, t_atom *);
typedef int (*t_zlnatomsfn)(struct _zl *);
typedef void (*t_zldoitfn)(struct _zl *, int, t_atom *, int);

static t_symbol      *zl_modesym[ZL_N_MODES];
static int            zl_modeflags[ZL_N_MODES];
static t_zlintargfn   zl_intargfn[ZL_N_MODES];
static t_zlanyargfn   zl_anyargfn[ZL_N_MODES];
static t_zlnatomsfn   zl_natomsfn[ZL_N_MODES];
static t_zldoitfn     zl_doitfn[ZL_N_MODES];

typedef struct _zldata{
    int      d_size;    /* as allocated */
    int      d_max;     // max size allowed, must be <= d_size
    int      d_natoms;  /* as used */
    int		 d_sorted; //-1 for sorted descending, otherwise sorted ascending
    t_atom  *d_buf;
    t_atom   d_bufini[ZL_DEF_SIZE];
} t_zldata;

typedef struct _zl{
    t_object          x_ob;
    struct _zlproxy  *x_proxy;
    int               x_entered;
    int               x_locked;  /* locking inbuf1 in modes: iter, reg, slice */
    t_zldata          x_inbuf1;
    t_zldata          x_inbuf2;
    t_zldata          x_outbuf1;
    t_zldata		  x_outbuf2;
    int               x_mode;
    int               x_modearg;
    int				  x_counter; /* generic counter */
    t_outlet         *x_out2;
} t_zl;

typedef struct _zlproxy{
    t_object  p_ob;
    t_zl     *p_master;
} t_zlproxy;

static t_class *zl_class;
static t_class *zlproxy_class;

// ********************************************************************
// ******************** ZL MODE FUNCTIONS *****************************
// ********************************************************************

static void zlhelp_copylist(t_atom *old, t_atom *new, int natoms){
  int i;
  //post("copying %d atoms", natoms);
  for(i=0; i<natoms; i++){
      switch(old[i].a_type){
          case(A_FLOAT):
              SETFLOAT(&new[i], old[i].a_w.w_float);
	      //post("copy: %f", old[i].a_w.w_float);
              break;
          case(A_SYMBOL):
              SETSYMBOL(&new[i], old[i].a_w.w_symbol);
              break;
          case(A_POINTER):
              SETPOINTER(&new[i], old[i].a_w.w_gpointer);
              break;
          default:
              break;
      };
  };
}

static void zldata_realloc(t_zldata *d, int reqsz){
    int cursz = d->d_size;
	int curmax = d->d_max;
	int heaped = d->d_buf != d->d_bufini;
	if(reqsz > ZL_MAXSIZE)
        reqsz = ZL_MAXSIZE;
    else if(reqsz < ZL_MINSIZE)
        reqsz = ZL_MINSIZE;
    if(reqsz <= ZL_DEF_SIZE && heaped){
	    memcpy(d->d_bufini, d->d_buf, ZL_DEF_SIZE * sizeof(t_atom));
	    freebytes(d->d_buf, cursz * sizeof(t_atom));
	    d->d_buf = d->d_bufini;
    }
	else if(reqsz > ZL_DEF_SIZE && !heaped){
	    d->d_buf = getbytes(reqsz * sizeof(t_atom));
        int maxsize = curmax > ZL_DEF_SIZE ? ZL_DEF_SIZE : curmax;
	    memcpy(d->d_buf, d->d_bufini, maxsize * sizeof(t_atom));
    }
	else if(reqsz > ZL_DEF_SIZE && heaped)
	    d->d_buf = (t_atom *)resizebytes(d->d_buf, cursz*sizeof(t_atom), reqsz*sizeof(t_atom));
	d->d_max = reqsz;
	if(reqsz < ZL_DEF_SIZE)
        reqsz = ZL_DEF_SIZE;
	if(d->d_natoms > d->d_max)
        d->d_natoms = d->d_max;
    d->d_size = reqsz;
}

static void zldata_init(t_zldata *d, int sz){
    d->d_size = ZL_DEF_SIZE;
    d->d_natoms = 0;
    d->d_buf = d->d_bufini;
    if(sz > ZL_DEF_SIZE)
        zldata_realloc(d, sz);
}

static void zldata_free(t_zldata *d){
    if(d->d_buf != d->d_bufini)
        freebytes(d->d_buf, d->d_size * sizeof(*d->d_buf));
}

static void zldata_reset(t_zldata *d, int sz){
  zldata_free(d);
  zldata_init(d, sz);
}

static void zldata_setfloat(t_zldata *d, t_float f){
    SETFLOAT(d->d_buf, f);
    d->d_natoms = 1;
}

static void zldata_addfloat(t_zldata *d, t_float f){
    int natoms = d->d_natoms;
    int nrequested = natoms + 1;
    if (nrequested <= d->d_max)
      {
        SETFLOAT(d->d_buf + natoms, f);
	d->d_natoms = natoms + 1;
      };
}

static void zldata_setsymbol(t_zldata *d, t_symbol *s){
    SETSYMBOL(d->d_buf, s);
    d->d_natoms = 1;
}

static void zldata_addsymbol(t_zldata *d, t_symbol *s){
    int natoms = d->d_natoms;
    int nrequested = natoms + 1;
    if (nrequested <= d->d_max){
        SETSYMBOL(d->d_buf + natoms, s);
        d->d_natoms = natoms + 1;
    };
}

static void zldata_setlist(t_zldata *d, int ac, t_atom *av){
    if (ac > d->d_max) ac = d->d_max;
    memcpy(d->d_buf, av, ac * sizeof(*d->d_buf));
    d->d_natoms = ac;
}

static void zldata_set(t_zldata *d, t_symbol *s, int ac, t_atom *av){
    if(s && s != &s_){
        int nrequested = ac + 1;
		if(nrequested > d->d_max) {
	    	ac = d->d_max - 1;
	    	if (ac < 0) ac = 0;
	  	}
        if(d->d_max >= 1){
            SETSYMBOL(d->d_buf, s);
            if(ac > 0)
                memcpy(d->d_buf + 1, av, ac * sizeof(*d->d_buf));
	    d->d_natoms = ac + 1;
        }
    }
    else
        zldata_setlist(d, ac, av);
}

static void zldata_addlist(t_zldata *d, int ac, t_atom *av){
    int natoms = d->d_natoms;
    int nrequested = natoms + ac;
    if(nrequested > d->d_max)
      {
	ac = d->d_max - natoms; //truncate
	if (ac < 0) ac = 0;
      };
    if((natoms + ac <= d->d_max) && ac > 0){
        memcpy(d->d_buf + natoms, av, ac * sizeof(*d->d_buf));
		d->d_natoms = natoms + ac;
    };
}

static void zldata_add(t_zldata *d, t_symbol *s, int ac, t_atom *av){
    if(s && s != &s_){
      //want to add: ac + 1 (ac + symbol selector) to natoms (already existing)
        int natoms = d->d_natoms;
        int nrequested = natoms + 1 + ac;
	if(d->d_max < nrequested)
	  {
	    ac = d->d_max - 1 - natoms; //truncate
	    if(ac < 0) ac = 0;
	  };
        if(d->d_max >= natoms + 1){
            SETSYMBOL(d->d_buf + natoms, s);
            if (ac > 0)
                memcpy(d->d_buf + natoms + 1, av, ac * sizeof(*d->d_buf));
	    d->d_natoms = natoms + 1 + ac;
        };
    }
    else
        zldata_addlist(d, ac, av);
}

static void zl_dooutput(t_outlet *o, int ac, t_atom *av){
    if(ac > 1){
        if(av->a_type == A_FLOAT)
            outlet_list(o, &s_list, ac, av);
        else if (av->a_type == A_SYMBOL)
            outlet_anything(o, av->a_w.w_symbol, ac - 1, av + 1);
    }
    else if (ac){
        if (av->a_type == A_FLOAT)
            outlet_float(o, av->a_w.w_float);
        else if (av->a_type == A_SYMBOL)
            outlet_anything(o, av->a_w.w_symbol, 0, 0);
    }
}

static void zl_output(t_zl *x, int ac, t_atom *av){
    zl_dooutput(((t_object *)x)->ob_outlet, ac, av);
}

static void zl_output2(t_zl *x, int ac, t_atom *av){
    zl_dooutput(x->x_out2, ac, av);
}

static int zl_equal(t_atom *ap1, t_atom *ap2){
    return (ap1->a_type == ap2->a_type
	    &&
	    ((ap1->a_type == A_FLOAT
	      && ap1->a_w.w_float == ap2->a_w.w_float)
	     ||
	     (ap1->a_type == A_SYMBOL
	      && ap1->a_w.w_symbol == ap2->a_w.w_symbol)));
}

static void zl_sizecheck(t_zl *x, int size){
	if (x->x_modearg > size) x->x_modearg = size;
}

static void zl_swap(t_atom *av, int i, int j) {
	t_atom temp = av[j];
	av[j] = av[i];
	av[i] = temp; 
}


// ********************************************************************
// ************************* ZL MODES *********************************
// ********************************************************************

/* Mode handlers:
   If x_mode is positive, then the main routine uses an output
   buffer 'buf' (outbuf, or a separately allocated one).
   If it's 0, then the main routine is passed a null 'buf' 
   (see below). And if negative, then the main routine isn't called.

   zl_<mode> (main routine) arguments:  if 'buf' is null, 'natoms'
   is always zero - in modes other than len (no buffer used), group,
   iter, reg, slice/ecils (inbuf1 used), there should be no output.
   If 'buf' is not null, then 'natoms' is guaranteed to be positive.
*/


// ************************* UNKNOWN *********************************

static int zl_nop_count(t_zl *x){
    x = NULL;
    return(0);
}

static void zl_nop(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = natoms = 0;
    buf = NULL;
    pd_error(x, "[zl]: unknown mode");
}

// ************************* ECILS *********************************

static int zl_ecils_intarg(t_zl *x, int i){
    x = NULL;
    return (i > 0 ? i : 0);
}

static int zl_ecils_count(t_zl *x){
    return (x->x_entered ? -1 : 0);
}

static void zl_ecils(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    int cnt1, cnt2 = x->x_modearg;
    natoms = x->x_inbuf1.d_natoms;
    buf = x->x_inbuf1.d_buf;
    if (cnt2 > natoms)
        cnt2 = natoms, cnt1 = 0;  /* CHECKED */
    else
        cnt1 = natoms - cnt2;
    x->x_locked = 1;
    if (cnt2)
        zl_output2(x, cnt2, buf + cnt1);
    if (cnt1)
        zl_output(x, cnt1, buf);
}

// ************************* GROUP *********************************

static int zl_group_intarg(t_zl *x, int i){
    x = NULL;
    return (i > 0 ? i : 0);
}

static int zl_group_count(t_zl *x){
    return (x->x_entered ? -1 : 0);
}

static void zl_group(t_zl *x, int natoms, t_atom *inbuf, int banged){
    natoms = 0;
    int count = x->x_modearg;
    //idea: use outbuf to store output
    if(count > 0){
    int i = 0;
    int inatoms = x->x_inbuf1.d_natoms;
	int outatoms = x->x_outbuf1.d_natoms;
	int outmax = x->x_outbuf1.d_max;
	t_atom * outbuf = x->x_outbuf1.d_buf;
	inbuf = x->x_inbuf1.d_buf;
	if(inatoms > 0) x->x_locked = 1; // when do things get unlocked?! - DK
	while(i < inatoms)
	  {
	    int numcopy = count - outatoms;
	    if(numcopy < 0) numcopy = 0;
	    if((numcopy + outatoms)> outmax) numcopy = outmax - outatoms;
	    if(numcopy > (inatoms - i)) numcopy = inatoms - i;
	    if(numcopy > count) numcopy = count;
	    zlhelp_copylist(&inbuf[i], &outbuf[outatoms], numcopy);
	    //post("i:%d in:%d out:%d nc:%d", i, inatoms, outatoms, numcopy);
	    i += numcopy;
	    x->x_outbuf1.d_natoms = outatoms += numcopy;
	    if(outatoms >= count)
	      {
		zl_output(x, count, outbuf);
		x->x_outbuf1.d_natoms = outatoms -= count;
	      };
	    
	  };
	x->x_inbuf1.d_natoms = 0; //since inlet methods add to inbuf1, make sure zeroed to treat as empty
	x->x_outbuf1.d_natoms = outatoms;
        if (banged && outatoms)
	  {
            zl_output(x, outatoms, outbuf);
            x->x_outbuf1.d_natoms = 0;
	  };
    }
    else
        x->x_inbuf1.d_natoms = 0;
}

// ************************* ITER *********************************

static int zl_iter_intarg(t_zl *x, int i){
    x = NULL;
    return (i > 0 ? i : 0);
}

static int zl_iter_count(t_zl *x){
    return (x->x_entered ?
	    (x->x_modearg < x->x_inbuf1.d_natoms ?
	     x->x_modearg : x->x_inbuf1.d_natoms)
	    : 0);
}

static void zl_iter(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    int nremaining = x->x_inbuf1.d_natoms;
    t_atom *ptr = x->x_inbuf1.d_buf;
    if(!buf){
        if((natoms = (x->x_modearg < nremaining ? x->x_modearg : nremaining)))
            x->x_locked = 1;
        else
            return;
    }
    while(nremaining){
        if(natoms > nremaining)
            natoms = nremaining;
        if(buf){
            memcpy(buf, ptr, natoms * sizeof(*buf));
            zl_output(x, natoms, buf);
        }
        else
            zl_output(x, natoms, ptr);
	nremaining -= natoms;
	ptr += natoms;
    }
}

// ************************* JOIN *********************************

static void zl_join_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(!x->x_locked)
        zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_join_count(t_zl *x){
    return (x->x_inbuf1.d_natoms + x->x_inbuf2.d_natoms);
}

static void zl_join(t_zl *x, int natoms, t_atom *buf, int banged){
    if(buf){
        banged = 0;
        int ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms;
        if (ac1)
            memcpy(buf, x->x_inbuf1.d_buf, ac1 * sizeof(*buf));
        if (ac2)
            memcpy(buf + ac1, x->x_inbuf2.d_buf, ac2 * sizeof(*buf));
        zl_output(x, natoms, buf);
    }
}

// ************************* LEN *********************************

static int zl_len_count(t_zl *x){
    x = NULL;
    return(0);
}

static void zl_len(t_zl *x, int natoms, t_atom *buf, int banged){
    buf = NULL;
    banged = natoms = 0;
    outlet_float(((t_object *)x)->ob_outlet, x->x_inbuf1.d_natoms);
}

// ************************* NTH *********************************

static int zl_nth_intarg(t_zl *x, int i){
    x = NULL;
    return (i > 0 ? i : 0);
}

static void zl_nth_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(!s && ac && av->a_type == A_FLOAT)
        zldata_setlist(&x->x_inbuf2, ac - 1, av + 1);
}

static int zl_nth_count(t_zl *x){
    int ac1 = x->x_inbuf1.d_natoms;
    if(ac1){
        if(x->x_modearg > 0)
            return (ac1 - 1 + x->x_inbuf2.d_natoms);
        else
            return (x->x_entered ? ac1 : 0);
    }
    else return (-1);
}

static void zl_nth(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    int ac1 = x->x_inbuf1.d_natoms,
	ndx = x->x_modearg - 1;  // one-indexed
    if(ac1){
        t_atom *av1 = x->x_inbuf1.d_buf;
        if(ndx < 0 || ndx >= ac1){
            if(buf)
                memcpy(buf, av1, ac1 * sizeof(*buf));
            else{
                buf = av1;
                x->x_locked = 1;
            }
            zl_output2(x, ac1, buf);
        }
        else{
            t_atom at = av1[ndx];
            if(buf){
                int ac2 = x->x_inbuf2.d_natoms, ntail = ac1 - ndx - 1;
                t_atom *ptr = buf;
                if(ndx){
                    memcpy(ptr, av1, ndx * sizeof(*buf));
                    ptr += ndx;
                }
                if(ac2){  /* replacement */
                    memcpy(ptr, x->x_inbuf2.d_buf, ac2 * sizeof(*buf));
                    ptr += ac2;
                }
                if(ntail)
                    memcpy(ptr, av1 + ndx + 1, ntail * sizeof(*buf));
                zl_output2(x, natoms, buf);
            }
	    zl_output(x, 1, &at);
        }
    }
}

// ************************* MTH *********************************

static int zl_mth_intarg(t_zl *x, int i){
    x = NULL;
    return (i);
}

static void zl_mth_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(!s && ac && av->a_type == A_FLOAT)
        zldata_setlist(&x->x_inbuf2, ac - 1, av + 1);
}
static int zl_mth_count(t_zl *x){
    int ac1 = x->x_inbuf1.d_natoms;
    if(ac1){
        if(x->x_modearg >= 0)
            return (ac1 - 1 + x->x_inbuf2.d_natoms);
        else
            return (x->x_entered ? ac1 : 0);
    }
    else return(-1);
}

static void zl_mth(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    int ac1 = x->x_inbuf1.d_natoms,
    ndx = x->x_modearg;  // zero-indexed
    if(ac1){
        t_atom *av1 = x->x_inbuf1.d_buf;
        if(ndx < 0 || ndx >= ac1){
            if(buf)
                memcpy(buf, av1, ac1 * sizeof(*buf));
            else{
                buf = av1;
                x->x_locked = 1;
            }
            zl_output2(x, ac1, buf);
        }
        else{
            t_atom at = av1[ndx];
            if(buf){
                int ac2 = x->x_inbuf2.d_natoms, ntail = ac1 - ndx - 1;
                t_atom *ptr = buf;
                if(ndx){
                    memcpy(ptr, av1, ndx * sizeof(*buf));
                    ptr += ndx;
                }
                if(ac2){  /* replacement */
                    memcpy(ptr, x->x_inbuf2.d_buf, ac2 * sizeof(*buf));
                    ptr += ac2;
                }
                if(ntail)
                    memcpy(ptr, av1 + ndx + 1, ntail * sizeof(*buf));
                zl_output2(x, natoms, buf);
            }
            zl_output(x, 1, &at);
        }
    }
}

// ************************* REG *********************************

static void zl_reg_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if (!x->x_locked)
        zldata_set(&x->x_inbuf1, s, ac, av);
}

static int zl_reg_count(t_zl *x){
    return (x->x_entered ? x->x_inbuf1.d_natoms : 0);
}

static void zl_reg(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    if(buf)
        memcpy(buf, x->x_inbuf1.d_buf, natoms * sizeof(*buf));
    else{
        natoms = x->x_inbuf1.d_natoms;
        buf = x->x_inbuf1.d_buf;
        x->x_locked = 1;
    }
    if (natoms)
        zl_output(x, natoms, buf);
}

// ************************* REV *********************************

static int zl_rev_count(t_zl *x){
    return (x->x_inbuf1.d_natoms);
}

static void zl_rev(t_zl *x, int natoms, t_atom *buf, int banged){
    if (buf){
        banged = 0;
        t_atom *from = x->x_inbuf1.d_buf, *to = buf + natoms;
        while (to-- > buf)
            *to = *from++;
        zl_output(x, natoms, buf);
    }
}

// ************************* ROT *********************************

static int zl_rot_intarg(t_zl *x, int i){
    x = NULL;
    return (i);  // CHECKED anything goes (modulo)
}

static int zl_rot_count(t_zl *x){
    return (x->x_inbuf1.d_natoms);
}

static void zl_rot(t_zl *x, int natoms, t_atom *buf, int banged){
    if(buf){
        banged = 0;
        int cnt1 = x->x_modearg, cnt2;
        if(cnt1){
            if (cnt1 > 0){
                cnt1 %= natoms;
                cnt2 = natoms - cnt1;
            }
            else{
                cnt2 = -cnt1 % natoms;
                cnt1 = natoms - cnt2;
            }
            /* CHECKED right rotation for positive args */
            memcpy(buf, x->x_inbuf1.d_buf + cnt2, cnt1 * sizeof(*buf));
            memcpy(buf + cnt1, x->x_inbuf1.d_buf, cnt2 * sizeof(*buf));
        }
	else
        memcpy(buf, x->x_inbuf1.d_buf, natoms * sizeof(*buf));
	zl_output(x, natoms, buf);
    }
}

// ************************* SECT *********************************

static void zl_sect_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if (!x->x_locked)
        zldata_set(&x->x_inbuf2, s, ac, av);
}

/* LATER rethink */
static int zl_sect_count(t_zl *x){
    int result = 0;
    int ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms, i1;
    t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf, *ap1;
    for(i1 = 0, ap1 = av1; i1 < ac1; i1++, ap1++){
        int i2;
        t_atom *testp;
        for(i2 = 0, testp = av1; i2 < i1; i2++, testp++)
            if(zl_equal(ap1, testp))
                goto
                skip;
        for (i2 = 0, testp = av2; i2 < ac2; i2++, testp++){
            if (zl_equal(ap1, testp)){
                result++;
                break;
            }
        }
    skip:;
    }
    return(result);
}

// CHECKED in-buffer duplicates are skipped
static void zl_sect(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    if(!natoms)
        outlet_bang(x->x_out2);
    if(buf){
        int ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms, i1;
        t_atom *ap1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf, *to = buf;
        for(i1 = 0; i1 < ac1; i1++, ap1++){
            int i2;
            t_atom *testp;
            for(testp = buf; testp < to; testp++)
                if(zl_equal(ap1, testp))
                    goto
                        skip;
            for(i2 = 0, testp = av2; i2 < ac2; i2++, testp++){
                if(zl_equal(ap1, testp)){
                    *to++ = *ap1;
                    break;
                }
            }
        skip:;
        }
    zl_output(x, natoms, buf);
    }
}

// ************************* SLICE *********************************

static int zl_slice_intarg(t_zl *x, int i){
    x = NULL;
    return (i > 0 ? i : 0);  /* CHECKED */
}

static int zl_slice_count(t_zl *x){
    return (x->x_entered ? -1 : 0);
}

static void zl_slice(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    int cnt1 = x->x_modearg, cnt2;
    natoms = x->x_inbuf1.d_natoms;
    buf = x->x_inbuf1.d_buf;
    if(cnt1 > natoms)
        cnt1 = natoms, cnt2 = 0;  /* CHECKED */
    else
        cnt2 = natoms - cnt1;
    x->x_locked = 1;
    if(cnt2)
        zl_output2(x, cnt2, buf + cnt1);
    if(cnt1)
        zl_output(x, cnt1, buf);
}

// ************************* SORT *********************************

static int zl_sort_intarg(t_zl *x, int i){
    x = NULL;
    return (i == -1 ? -1 : 1);
}

static int zl_sort_count(t_zl *x){
    return (x->x_inbuf1.d_natoms);
}

static int zl_sort_cmp(t_zl *x, t_atom *a1, t_atom *a2){
    x = NULL;
	if(a1->a_type == A_FLOAT && a2->a_type == A_SYMBOL)
		return(-1);
	if(a1->a_type == A_SYMBOL && a2->a_type == A_FLOAT)
		return(1);
	if(a1->a_type == A_FLOAT && a2->a_type == A_FLOAT){
		if(a1->a_w.w_float < a2->a_w.w_float)
			return(-1);
		if(a1->a_w.w_float > a2->a_w.w_float)
			return (1);
		return(0);
	}
	if(a1->a_type == A_SYMBOL && a2->a_type == A_SYMBOL)
		return(strcmp(a1->a_w.w_symbol->s_name, a2->a_w.w_symbol->s_name));
	if(a1->a_type == A_POINTER)
        return(1);
    if(a2->a_type == A_POINTER)
		return(-1);
    else
        return(0);
}

static void zl_sort_qsort(t_zl *x, t_atom *av1, t_atom *av2, int left, int right, int dir) {
    int i, last;
    if(left >= right)
        return;
    zl_swap(av1, left, (left + right)/2);
    if(av2)
    	zl_swap(av2, left, (left + right)/2);
    last = left;
    for (i = left+1; i <= right; i++) {
        if((dir * zl_sort_cmp(x, av1 + i, av1 + left)) < 0){
            zl_swap(av1, ++last, i);
            if (av2)
            	zl_swap(av2, last, i);
        }
    }
    zl_swap(av1, left, last);
    if (av2)
    	zl_swap(av2, left, last);
    zl_sort_qsort(x, av1, av2, left, last-1, dir);
    zl_sort_qsort(x, av1, av2, last+1, right, dir);
}

static void zl_sort_rev(t_zl *x, int natoms, t_atom *av) {
    x = NULL;
	for (int i = 0, j = natoms - 1; i < natoms/2; i++, j--)
		zl_swap(av, i, j);
	
}

static void zl_sort(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
    	t_atom *buf2 = x->x_outbuf2.d_buf;
    	x->x_outbuf2.d_natoms = natoms;
    	if(banged) {
    		if (x->x_inbuf1.d_sorted != x->x_modearg) {
    			x->x_inbuf1.d_sorted = x->x_modearg;
    			zl_sort_rev(x, natoms, buf2);
    			zl_sort_rev(x, natoms, buf);
    		}
    		zl_output2(x, natoms, buf2);
    		zl_output(x, natoms, buf);		
    	}
    	else {

			memcpy(buf, x->x_inbuf1.d_buf, natoms * sizeof(*buf));
			for (int i = 0; i < natoms; i++)
    			SETFLOAT(&buf2[i], i);
    		zl_sort_qsort(x, buf, buf2, 0, natoms - 1, x->x_modearg);
    		//
    		x->x_inbuf1.d_sorted = x->x_modearg;
    		zl_output2(x, natoms, buf2);
    		zl_output(x, natoms, buf);
    		
		}
    	
    } // if(buf)
}

// ************************* SUB *********************************

static void zl_sub_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(!x->x_locked)
        zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_sub_count(t_zl *x){
    x = NULL;
    return(0);
}

static void zl_sub(t_zl *x, int natoms, t_atom *buf, int banged){
    int natoms2 = x->x_inbuf2.d_natoms;
    if(natoms2){
        buf = NULL;
        banged = natoms = 0;
        int found = 0;
        int ndx1, natoms1 = x->x_inbuf1.d_natoms;
        t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
        for(ndx1 = 0; ndx1 < natoms1; ndx1++){
            int indx2;
            t_atom *ap1 = av1 + ndx1, *ap2 = av2;
            for(indx2 = 0; indx2 < natoms2; indx2++, ap1++, ap2++)
                if(!zl_equal(ap1, ap2))
                    break;
            if(indx2 == natoms2)
                found++;
        }
        outlet_float(x->x_out2, found);
        for(ndx1 = 0; ndx1 < natoms1; ndx1++, av1++){
            int ndx2;
            t_atom *ap3 = av1, *ap4 = av2;
            for(ndx2 = 0; ndx2 < natoms2; ndx2++, ap3++, ap4++)
                if(!zl_equal(ap3, ap4))
                    break;
                if(ndx2 == natoms2)
                    outlet_float(((t_object *)x)->ob_outlet, ndx1 + 1);
        }
    }
}

// ************************* UNION *********************************

static void zl_union_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(!x->x_locked)
        zldata_set(&x->x_inbuf2, s, ac, av);
}

/* LATER rethink */
static int zl_union_count(t_zl *x){
    int result, ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms, i2;
    t_atom *av1 = x->x_inbuf1.d_buf, *ap2 = x->x_inbuf2.d_buf;
    result = ac1 + ac2;
    for (i2 = 0; i2 < ac2; i2++, ap2++){
        int i1;
        t_atom *ap1;
        for (i1 = 0, ap1 = av1; i1 < ac1; i1++, ap1++){
            if (zl_equal(ap1, ap2)){
                result--;
                break;
            }
        }
    }
    return (result);
}

/* CHECKED in-buffer duplicates not skipped */
static void zl_union(t_zl *x, int natoms, t_atom *buf, int banged){
    if (buf){
        banged = 0;
        int ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms, i2;
        t_atom *av1 = x->x_inbuf1.d_buf, *ap2 = x->x_inbuf2.d_buf;
        if (ac1){
            t_atom *to = buf + ac1;
            memcpy(buf, av1, ac1 * sizeof(*buf));
            for (i2 = 0; i2 < ac2; i2++, ap2++){
                int i1;
                t_atom *ap1;
                for (i1 = 0, ap1 = av1; i1 < ac1; i1++, ap1++)
                    if (zl_equal(ap1, ap2))
                        break;
                if (i1 == ac1)
                    *to++ = *ap2;
            }
        }
	else
        memcpy(buf, ap2, ac2 * sizeof(*buf));
	zl_output(x, natoms, buf);
    }
}

// ************************* CHANGE *********************************

static void zl_change_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
        zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_change_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_change(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		int ac2 = x->x_inbuf2.d_natoms;
    	t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
    	if ((natoms == ac2) && !memcmp(av1, av2, natoms * sizeof(*av1))) {
    		outlet_float(x->x_out2, 0);
    	}
    	else {
    		memcpy(av2, av1, natoms * sizeof(*buf));
    		x->x_inbuf2.d_natoms = natoms;
    		outlet_float(x->x_out2, 1);
    		zl_output(x, natoms, av1);
    	}
    }
}

// ************************* COMPARE *********************************

static void zl_compare_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
        zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_compare_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_compare(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		int ac2 = x->x_inbuf2.d_natoms;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		if (natoms != ac2) {
			outlet_float(x->x_out2, (natoms < ac2 ? natoms : ac2)) ;
			outlet_float(((t_object *)x)->ob_outlet, 0);
		}
		else {
			int i;
			for (i = 0 ; i < natoms && zl_equal(av1 + i,av2 + i) ; i++) {
				//
			} 
			if (i == natoms)
				outlet_float(((t_object *)x)->ob_outlet, 1);
			else {
				outlet_float(x->x_out2, i) ;
				outlet_float(((t_object *)x)->ob_outlet, 0);
			}	
		}
		 
	}
}

// ************************* DELACE *********************************

static int zl_delace_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_delace(t_zl *x, int n, t_atom *buf, int banged){
    if(buf && n > 1){
        banged = 0;
        t_atom *av1 = x->x_inbuf1.d_buf, *buf2 = x->x_outbuf2.d_buf;
        int odd = n % 2, i, j;
        for(i = 0, j = 0; i < n-odd; i++, j++){
            buf[j] = av1[i++];
            buf2[j] = av1[i];
        }
        zl_output2(x, n/2, buf2);
        zl_output(x, (n/2), buf);
    }
}

// ************************* FILTER *********************************

static void zl_filter_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_filter_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_filter(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf,
			*buf2 = x->x_outbuf2.d_buf;
		int filtatoms = x->x_inbuf2.d_natoms;
		int i, j, total = 0;
		for (i = 0; i < natoms ; i++) {
			for (j = 0; j < filtatoms; j++) {
				if (zl_equal(&av1[i], &av2[j]))
					break;
			}
			if (j == filtatoms) {
				//post("total = %d", total);
				SETFLOAT(&buf2[total], i);
				buf[total++] = av1[i];
			}
		}
		zl_output2(x, total, buf2);
		zl_output(x, total, buf);
	}
}

// ************************* LACE *********************************

static void zl_lace_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_lace_count(t_zl *x){
	int ac1 = x->x_inbuf1.d_natoms, ac2 = x->x_inbuf2.d_natoms;
	return (2*(ac1 < ac2 ? ac1 : ac2));
}

static void zl_lace(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int i, j = 0, ac = natoms/2;
		for (i = 0; i < ac; i++) {
			 buf[j++] = av1[i];
			 buf[j++] = av2[i];
		}
		zl_output(x, natoms, buf);
		
	}
}

// ************************* LOOKUP *********************************

static void zl_lookup_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_lookup_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_lookup(t_zl *x, int natoms, t_atom *buf, int banged){
	if (buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int ac2 = x->x_inbuf2.d_natoms, total = 0, i, j;
		for (i = 0; i < natoms; i++) {
			//post("total %d", total);
			if (av1[i].a_type == A_FLOAT && (j = (int)av1[i].a_w.w_float) < ac2) {
				//post ("index %d", j);
				buf[total++] = av2[j];
			}
		}
		zl_output(x,total,buf);
	}
}

// ************************* MEDIAN *********************************

static int zl_median_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_median(t_zl *x, int natoms, t_atom *buf, int banged){
	if (buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf;
		int i, total = 0;
		for (i = 0; i < natoms; i++) {
			if (av1[i].a_type == A_FLOAT)
				buf[total++] = av1[i];
			//post ("total %d", total);
		}
		if (total) {
			zl_sort_qsort(x, buf, 0, 0, total - 1, 1);
			//zl_output2(x,total,buf);
			if (total % 2)
				outlet_float(((t_object *)x)->ob_outlet, buf[total/2].a_w.w_float);
			else 
				outlet_float(((t_object *)x)->ob_outlet, 
				0.5*(buf[total/2 - 1].a_w.w_float + buf[total/2].a_w.w_float));			
		}
	}
}

// ************************* QUEUE *********************************

static int zl_queue_count(t_zl *x){
	return(x->x_inbuf1.d_natoms);
}

static void zl_queue(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf){
		t_atom *av1 = x->x_inbuf1.d_buf;
		int count = x->x_counter, max = x->x_outbuf1.d_max;
		int bufrp = x->x_outbuf1.d_natoms, bufwp;
		int i;
		if(banged){
			if(count){
				outlet_float(x->x_out2, --count);
				zl_output(x, 1, &buf[bufrp++]);
				bufrp %= max;
				x->x_outbuf1.d_natoms = bufrp;
				x->x_counter = count;
			}
		}
		else{
			if (natoms + count > max) natoms = max - count;
			bufwp = (bufrp + count) % max;
			for(i = 0; i < natoms ; i++) {
				buf[bufwp++] = av1[i];
				bufwp %= max;
			}
			x->x_counter = count += natoms;
			outlet_float(x->x_out2, count) ;
		}
	}
	
}

// ************************* SCRAMBLE *********************************

static void zl_scramble_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf1, s, ac, av);
}

static int zl_scramble_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_scramble(t_zl *x, int natoms, t_atom *buf, int banged){
    if(buf){
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *buf2 = x->x_outbuf2.d_buf;
		int n = natoms , i;
		memcpy(buf, av1, natoms * sizeof(*buf));
		for (i = 0; i < natoms; i++)
			SETFLOAT(&buf2[i], i);
		while(n-- > 1) {
			int r = rand() % (n);
			zl_swap(buf, r, n);
			zl_swap(buf2, r, n);
		}
		zl_output2(x,natoms,buf2);
		zl_output(x,natoms,buf);
	}
}

// ************************* STACK *********************************
static int zl_stack_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_stack(t_zl *x, int natoms, t_atom *buf, int banged){
    buf = NULL;
	t_atom *av1 = x->x_inbuf1.d_buf;
	if (banged) {
		if (natoms) {
			outlet_float(x->x_out2, --natoms);
			zl_output(x, 1, &av1[natoms]);
			x->x_inbuf1.d_natoms = natoms;
		}
		else
			outlet_float(x->x_out2, -1);
	}
	else {
		outlet_float(x->x_out2, natoms);
	}
}

// ************************* STREAM *********************************

static int zl_stream_intarg(t_zl *x, int i){
	x->x_counter = 0;
    return (i);  
}

static int zl_stream_count(t_zl *x){
    return (x->x_inbuf1.d_natoms);
}

static void zl_stream(t_zl *x, int natoms, t_atom *buf, int banged){
    int len = (x->x_modearg < 0 ? -x->x_modearg : x->x_modearg);
    int reverse = (x->x_modearg < 0), count = x->x_counter;
    //idea: use outbuf to store output
    if (banged || len == 0) {
    	if (len && count >= len) {
    		outlet_float(x->x_out2, 1) ;
			zl_output(x, len, buf);
    	}
    	else
    		outlet_float(x->x_out2, 0);
    }
    else {
    	int i, j;
    	t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int ap2 = x->x_inbuf2.d_natoms;

			for (i = (natoms > len ? natoms - len : 0); i < natoms ; i++) {
				av2[ap2++] = av1[i];
				ap2 %= len;
				count++;
			}
			if (count >= len) {
				count = len;
				i = (reverse ? -1 : 0);
				for (j = 0; j < len; j++) {
					buf[j] = av2[((ap2 + i) % len + len) % len]; //modulo for positive and negative
					i += (reverse ? -1 : 1);
				}
				outlet_float(x->x_out2, 1) ;
				zl_output(x, len, buf);
			}
			else
				outlet_float(x->x_out2, 0);
			x->x_counter = count;
			x->x_inbuf2.d_natoms = ap2;	
    }
}




// ************************* SUM *********************************
static int zl_sum_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}


static void zl_sum(t_zl *x, int natoms, t_atom *buf, int banged){
    banged = 0;
    buf = NULL;
	t_atom *av1 = x->x_inbuf1.d_buf;
	int i;
	t_float sum = 0;
	for (i = 0; i < natoms; i++) {
	if (av1[i].a_type == A_FLOAT)
		sum += av1[i].a_w.w_float;
	}
	outlet_float(((t_object *)x)->ob_outlet, sum);	
}

// ************************* THIN *********************************

static int zl_thin_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_thin(t_zl *x, int natoms, t_atom *buf, int banged){
	if (buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf;
		int i, j, total = 0;
		for (i = 0; i < natoms ; i++) {
			for (j = 0; j < total; j++) {
				if (zl_equal(&av1[i], &buf[j]))
					break;
			}
			if (j == total) 
				buf[total++] = av1[i];
		}
		zl_output(x,total,buf);
	}
}


// ************************* UNIQUE *********************************

static void zl_unique_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_unique_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_unique(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int filtatoms = x->x_inbuf2.d_natoms;
		int i, j, total = 0;
		for (i = 0; i < natoms ; i++) {
			for (j = 0; j < filtatoms; j++) {
				if (zl_equal(&av1[i], &av2[j]))
					break;
			}
			if (j == filtatoms) {
				//post("total = %d", total);
				//SETFLOAT(&buf2[total], i);
				buf[total++] = av1[i];
			}
		}
		//zl_output2(x, total, buf2);
		zl_output(x, total, buf);
	}
}

// ************************* INDEXMAP *********************************

static void zl_indexmap_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_indexmap_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_indexmap(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int nindices = x->x_inbuf2.d_natoms;
		for (int i = 0; i < nindices; i++) {
			int index;
			if (av2[i].a_type == A_SYMBOL) {
				index = 0;
				pd_error(x,"%s: bad number", av2[i].a_w.w_symbol->s_name);
			}
			else
				index = av2[i].a_w.w_float;
			if (index < 0) index = 0;
			if (index >= natoms) index = natoms-1;
			buf[i] = av1[index];
		}
		zl_output(x, nindices, buf);
	}
}

// ************************* SWAP *********************************

static void zl_swapmode_anyarg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    zldata_set(&x->x_inbuf2, s, ac, av);
}

static int zl_swapmode_count(t_zl *x){
	return (x->x_inbuf1.d_natoms);
}

static void zl_swapmode(t_zl *x, int natoms, t_atom *buf, int banged){
	if(buf) {
        banged = 0;
		t_atom *av1 = x->x_inbuf1.d_buf, *av2 = x->x_inbuf2.d_buf;
		int nswaps = (x->x_inbuf2.d_natoms/2)*2;
		int i, i1, i2;
		memcpy(buf, av1, natoms * sizeof(*buf));
		for (i = 0; i < nswaps; i += 2) {
			if (av2[i].a_type == A_SYMBOL) {
				i1 = 0;
				pd_error(x,"%s: bad number", av2[i].a_w.w_symbol->s_name);
			}
			else
				i1 = av2[i].a_w.w_float;
			if (av2[i+1].a_type == A_SYMBOL) {
				i2 = 0;
				pd_error(x,"%s: bad number", av2[i+1].a_w.w_symbol->s_name);
			}
			else
				i2 = av2[i+1].a_w.w_float;
			if (i1 < 0 || i1 >= natoms || i2 < 0 || i2 >= natoms)
				continue;
			zl_swap(buf, i1, i2);
		}
		zl_output(x, natoms, buf);
	}
}


// ********************************************************************
// ************************* METHODS **********************************
// ********************************************************************

static void zl_doit(t_zl *x, int banged){
    int reentered = x->x_entered;
    int natoms = (*zl_natomsfn[x->x_mode])(x);
    if(natoms < 0)
        return;
    x->x_entered = 1;
    if(natoms){
        t_zldata *d = &x->x_outbuf1;
        if(natoms > d->d_max) // giving this a shot...
            natoms = d->d_max;
        // basically will limit output buffer to specified size instead of allowing it to go over...
        //if(prealloc)
            (*zl_doitfn[x->x_mode])(x, natoms, d->d_buf, banged);
    }
    else
        (*zl_doitfn[x->x_mode])(x, 0, 0, banged);
    if(!reentered)
        x->x_entered = x->x_locked = 0;
}

static void zl_bang(t_zl *x){
    zl_doit(x, 1);
}

static void zl_float(t_zl *x, t_float f){
   // if (!x->x_locked){
        if (zl_modeflags[x->x_mode])
            zldata_addfloat(&x->x_inbuf1, f);
        else
            zldata_setfloat(&x->x_inbuf1, f);
    //}
    zl_doit(x, 0);
}

static void zl_symbol(t_zl *x, t_symbol *s){
    //if (!x->x_locked){
        if (zl_modeflags[x->x_mode])
            zldata_addsymbol(&x->x_inbuf1, s); 
        else
            zldata_setsymbol(&x->x_inbuf1, s);
    //}
    zl_doit(x, 0);
}

static void zl_list(t_zl *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(zl_modeflags[x->x_mode])
        zldata_addlist(&x->x_inbuf1, ac, av);
    else
        zldata_setlist(&x->x_inbuf1, ac, av);
    zl_doit(x, 0);
}

static void zl_anything(t_zl *x, t_symbol *s, int ac, t_atom *av){
    //if(!x->x_locked){
        if(zl_modeflags[x->x_mode])
            zldata_add(&x->x_inbuf1, s, ac, av);
        else
            zldata_set(&x->x_inbuf1, s, ac, av);
    //}
    zl_doit(x, 0);
}


static int zl_modeargfn(t_zl *x){
    return (zl_intargfn[x->x_mode] || zl_anyargfn[x->x_mode]);
}

static void zl_setmodearg(t_zl *x, t_symbol *s, int ac, t_atom *av){
    if(zl_intargfn[x->x_mode]){
        int i = (!s && ac && av->a_type == A_FLOAT ?
                 (int)av->a_w.w_float :  /* CHECKED silent truncation */
                 0);  /* CHECKED current x->x_modearg not kept */
        x->x_modearg = (*zl_intargfn[x->x_mode])(x, i);
    }
    if (zl_anyargfn[x->x_mode])
        (*zl_anyargfn[x->x_mode])(x, s, ac, av);
}

static void zlproxy_bang(t_zlproxy *d){
    d = NULL;
    // nop
}

static void zlproxy_float(t_zlproxy *p, t_float f){
    t_zl *x = p->p_master;
    if (zl_modeargfn(x)){
        t_atom at;
        SETFLOAT(&at, f);
        zl_setmodearg(x, 0, 1, &at);
    }
    else  /* CHECKED inbuf2 filled only when used */
        zldata_setfloat(&x->x_inbuf2, f);
}

static void zlproxy_symbol(t_zlproxy *p, t_symbol *s){
    t_zl *x = p->p_master;
    if (zl_modeargfn(x)){
        t_atom at;
        SETSYMBOL(&at, s);
        zl_setmodearg(x, 0, 1, &at);
    }
    else  /* CHECKED inbuf2 filled only when used */
        zldata_setsymbol(&x->x_inbuf2, s);
}

/* LATER gpointer */

static void zlproxy_list(t_zlproxy *p, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if (ac){
        t_zl *x = p->p_master;
        if (zl_modeargfn(x))
            zl_setmodearg(x, 0, ac, av);
        else  /* CHECKED inbuf2 filled only when used */
            zldata_setlist(&x->x_inbuf2, ac, av);
    }
}

static void zlproxy_anything(t_zlproxy *p, t_symbol *s, int ac, t_atom *av){
    t_zl *x = p->p_master;
    if (zl_modeargfn(x))
        zl_setmodearg(x, s, ac, av);
    else  /* CHECKED inbuf2 filled only when used */
        zldata_set(&x->x_inbuf2, s, ac, av);
}

static void zl_free(t_zl *x){
    zldata_free(&x->x_inbuf1);
    zldata_free(&x->x_inbuf2);
    zldata_free(&x->x_outbuf1);
    zldata_free(&x->x_outbuf2);
    if(x->x_proxy)
        pd_free((t_pd *)x->x_proxy);
}

static void zl_mode(t_zl *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac && av->a_type == A_SYMBOL){
        t_symbol *modesym = av->a_w.w_symbol;
        int i;
        for(i = 0; i < ZL_N_MODES; i++){
            // Because the gensym() happens before object creation,
            // we can't compare the symbols if we want it to work in multi-instance pd
            if(strcmp(modesym->s_name, zl_modesym[i]->s_name) == 0){
            x->x_mode = i;
            zl_setmodearg(x, 0, ac - 1, av + 1);
            break;
            }
        }
    }
}

static void zl_zlmaxsize(t_zl *x, t_floatarg f){
    int sz = (int)f;
    zldata_realloc(&x->x_inbuf1,sz);
    zldata_realloc(&x->x_inbuf2,sz);
    zldata_realloc(&x->x_outbuf1,sz);
    zldata_realloc(&x->x_outbuf2,sz);
    if(zl_modesym[x->x_mode] == gensym("group") || zl_modesym[x->x_mode] == gensym("stream"))
    	zl_sizecheck(x, sz);
}

static void zl_zlclear(t_zl *x){
    int sz1 = x->x_inbuf1.d_size;
    int sz2 = x->x_inbuf2.d_size;
    int sz3 = x->x_outbuf1.d_size;
    int sz4 = x->x_outbuf2.d_size;
    zldata_reset(&x->x_inbuf1, sz1);
    zldata_reset(&x->x_inbuf2, sz2);
    zldata_reset(&x->x_outbuf1, sz3);
    zldata_reset(&x->x_outbuf2, sz4);
    if(zl_modesym[x->x_mode] == gensym("stream")){
    	x->x_counter = 0;
    	outlet_float(x->x_out2, 0);
    }
    else if(zl_modesym[x->x_mode] == gensym("queue"))
        x->x_counter = 0;
}

static void *zl_new(t_symbol *s, int argc, t_atom *argv){
    t_zl *x = (t_zl *)pd_new(zl_class);
    t_zlproxy *y = (t_zlproxy *)pd_new(zlproxy_class);
    x->x_proxy = y;
    y->p_master = x;
    x->x_entered = 0;
    x->x_locked = 0;
    x->x_mode = 0; // Unknown mode
    int sz = ZL_DEF_SIZE;
    int first_arg = 0;
    int size_arg = 0;
    int size_attr = 0;
    int i = argc;
    t_atom *a = argv;
    while(i){
        if(a->a_type == A_FLOAT){
            if(!first_arg){ // no first arg yet, get size
                sz = (int)atom_getfloatarg(0, i, a);
                first_arg = size_arg = 1;
            }
            i--; // iterate
            a++;
        }
        else if(a->a_type == A_SYMBOL){ // is symbol
            if(!first_arg) // is first arg, so mark it
                first_arg = 1;
            t_symbol * cursym = atom_getsymbolarg(0, i, a);
            if(cursym == gensym("@zlmaxsize")){ // is the attribute
                i--;
                a++;
                if(i == 1){
                    if(a->a_type == A_FLOAT){
                        sz  = (int)atom_getfloatarg(0, i, a);
                        size_attr = 2;
                    }
                    else
                        goto errstate;
                };
                if(i != 1)
                    goto errstate;
            };
            i--;
            a++;
        };
    };
    if(sz < ZL_MINSIZE)
        sz = ZL_MINSIZE;
    if(sz > ZL_MAXSIZE)
        sz = ZL_MAXSIZE;
    x->x_inbuf1.d_max = sz;
    x->x_inbuf2.d_max = sz;
    x->x_outbuf1.d_max = sz;
    x->x_outbuf2.d_max = sz;
    zldata_init(&x->x_inbuf1, sz);
    zldata_init(&x->x_inbuf2, sz);
    zldata_init(&x->x_outbuf1, sz);
    zldata_init(&x->x_outbuf2, sz);
    zl_mode(x, s, argc - size_arg - size_attr, argv + size_arg);
    if(!x->x_mode)
        pd_error(x, "[zl]: unknown mode (needs a symbol argument)");
    inlet_new((t_object *)x, (t_pd *)y, 0, 0);
    outlet_new((t_object *)x, &s_anything);
    x->x_out2 = outlet_new((t_object *)x, &s_anything);
    if(zl_modesym[x->x_mode] == gensym("group") ||
    zl_modesym[x->x_mode] == gensym("stream"))
    	zl_sizecheck(x, sz);
    if(zl_modesym[x->x_mode] == gensym("scramble"))
    	srand((unsigned int)clock_getlogicaltime());
    return(x);
errstate:
    post("zl: improper args");
    return NULL;
}

static void zl_setupmode(char *id, int flags,
    	t_zlintargfn ifn, t_zlanyargfn afn, t_zlnatomsfn nfn, t_zldoitfn dfn, int i){
    zl_modesym[i] = gensym(id);
    zl_modeflags[i] = flags;
    zl_intargfn[i] = ifn;
    zl_anyargfn[i] = afn;
    zl_natomsfn[i] = nfn;
    zl_doitfn[i] = dfn;
}

CYCLONE_OBJ_API void zl_setup(void){
    zl_class = class_new(gensym("zl"), (t_newmethod)zl_new,
            (t_method)zl_free, sizeof(t_zl), 0, A_GIMME, 0);
    class_addbang(zl_class, zl_bang);
    class_addfloat(zl_class, zl_float);
    class_addsymbol(zl_class, zl_symbol);
    class_addlist(zl_class, zl_list);
    class_addanything(zl_class, zl_anything);
    class_addmethod(zl_class, (t_method)zl_mode, gensym("mode"), A_GIMME, 0);
    class_addmethod(zl_class, (t_method)zl_zlmaxsize, gensym("zlmaxsize"), A_FLOAT, 0);
    class_addmethod(zl_class, (t_method)zl_zlclear, gensym("zlclear"), 0);
    zlproxy_class = class_new(gensym("_zlproxy"), 0, 0,
        sizeof(t_zlproxy), CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(zlproxy_class, zlproxy_bang);
    class_addfloat(zlproxy_class, zlproxy_float);
    class_addsymbol(zlproxy_class, zlproxy_symbol);
    class_addlist(zlproxy_class, zlproxy_list);
    class_addanything(zlproxy_class, zlproxy_anything);
    zl_setupmode("unknown", 0, 0, 0, zl_nop_count, zl_nop, 0);
    zl_setupmode("ecils", 0, zl_ecils_intarg, 0, zl_ecils_count, zl_ecils, 1);
    zl_setupmode("group", 1, zl_group_intarg, 0, zl_group_count, zl_group, 2);
    zl_setupmode("iter", 0, zl_iter_intarg, 0, zl_iter_count, zl_iter, 3);
    zl_setupmode("join", 0, 0, zl_join_anyarg, zl_join_count, zl_join, 4);
    zl_setupmode("len", 0, 0, 0, zl_len_count, zl_len, 5);
    zl_setupmode("mth", 0, zl_mth_intarg, zl_mth_anyarg, zl_mth_count, zl_mth, 6);
    zl_setupmode("nth", 0, zl_nth_intarg, zl_nth_anyarg, zl_nth_count, zl_nth, 7);
    zl_setupmode("reg", 0, 0, zl_reg_anyarg, zl_reg_count, zl_reg, 8);
    zl_setupmode("rev", 0, 0, 0, zl_rev_count, zl_rev, 9);
    zl_setupmode("rot",	0, zl_rot_intarg, 0, zl_rot_count, zl_rot, 10);
    zl_setupmode("sect", 0, 0, zl_sect_anyarg, zl_sect_count, zl_sect, 11);
    zl_setupmode("slice", 0, zl_slice_intarg, 0, zl_slice_count, zl_slice, 12);
    zl_setupmode("sort", 0, zl_sort_intarg, 0, zl_sort_count, zl_sort, 13);
    zl_setupmode("sub", 0, 0, zl_sub_anyarg, zl_sub_count, zl_sub, 14);
    zl_setupmode("union", 0, 0, zl_union_anyarg, zl_union_count, zl_union, 15);
    zl_setupmode("change", 0, 0, zl_change_anyarg, zl_change_count, zl_change, 16);
    zl_setupmode("compare", 0, 0, zl_compare_anyarg, zl_compare_count, zl_compare, 17);
    zl_setupmode("delace", 0, 0, 0, zl_delace_count, zl_delace, 18);
    zl_setupmode("filter", 0, 0, zl_filter_anyarg, zl_filter_count, zl_filter, 19);
    zl_setupmode("lace", 0, 0, zl_lace_anyarg, zl_lace_count, zl_lace, 20);
    zl_setupmode("lookup", 0, 0, zl_lookup_anyarg, zl_lookup_count, zl_lookup, 21);
    zl_setupmode("median", 0, 0, 0, zl_median_count, zl_median, 22);
    zl_setupmode("queue", 0, 0, 0, zl_queue_count, zl_queue, 23);
    zl_setupmode("scramble", 0, 0, zl_scramble_anyarg, zl_scramble_count, zl_scramble, 24);
    zl_setupmode("stack", 1, 0, 0, zl_stack_count, zl_stack, 25);
    zl_setupmode("stream", 0, zl_stream_intarg, 0, zl_stream_count, zl_stream, 26);
    zl_setupmode("sum", 0, 0, 0, zl_sum_count, zl_sum, 27);
    zl_setupmode("thin", 0, 0, 0, zl_thin_count, zl_thin, 28);
    zl_setupmode("unique", 0, 0, zl_unique_anyarg, zl_unique_count, zl_unique, 29);
    zl_setupmode("indexmap", 0, 0, zl_indexmap_anyarg, zl_indexmap_count, zl_indexmap, 30);
    zl_setupmode("swap", 0, 0, zl_swapmode_anyarg, zl_swapmode_count, zl_swapmode, 31);    
}


#define ZL_ALIAS_SETUP(MODE) \
    static void* zl_##MODE##_new(t_symbol *s, int argc, t_atom *argv) \
    { \
        int ac = argc + 1; \
        t_atom* av = malloc(ac * sizeof(t_atom)); \
        memcpy(av + sizeof(t_atom), argv, argc * sizeof(t_atom)); \
        SETSYMBOL(av, gensym(#MODE)); \
        return zl_new(s, ac, av); \
    } \
    \
    CYCLONE_OBJ_API void setup_zl0x2e##MODE(void) { \
        zl_class = class_new(gensym("zl." #MODE), (t_newmethod)zl_##MODE##_new, \
                (t_method)zl_free, sizeof(t_zl), 0, A_GIMME, 0); \
        class_addbang(zl_class, zl_bang); \
        class_addfloat(zl_class, zl_float); \
        class_addsymbol(zl_class, zl_symbol); \
        class_addlist(zl_class, zl_list); \
        class_addanything(zl_class, zl_anything); \
        class_addmethod(zl_class, (t_method)zl_mode, gensym("mode"), A_GIMME, 0); \
        class_addmethod(zl_class, (t_method)zl_zlmaxsize, gensym("zlmaxsize"), A_FLOAT, 0); \
        class_addmethod(zl_class, (t_method)zl_zlclear, gensym("zlclear"), 0); \
        class_sethelpsymbol(zl_class, gensym("zl")); \
        zlproxy_class = class_new(gensym("_zlproxy"), 0, 0, \
        sizeof(t_zlproxy), CLASS_PD | CLASS_NOINLET, 0); \
        class_addbang(zlproxy_class, zlproxy_bang); \
        class_addfloat(zlproxy_class, zlproxy_float); \
        class_addsymbol(zlproxy_class, zlproxy_symbol); \
        class_addlist(zlproxy_class, zlproxy_list); \
        class_addanything(zlproxy_class, zlproxy_anything); \
        zl_setupmode("unknown", 0, 0, 0, zl_nop_count, zl_nop, 0); \
        zl_setupmode("ecils", 0, zl_ecils_intarg, 0, zl_ecils_count, zl_ecils, 1); \
        zl_setupmode("group", 1, zl_group_intarg, 0, zl_group_count, zl_group, 2); \
        zl_setupmode("iter", 0, zl_iter_intarg, 0, zl_iter_count, zl_iter, 3); \
        zl_setupmode("join", 0, 0, zl_join_anyarg, zl_join_count, zl_join, 4); \
        zl_setupmode("len", 0, 0, 0, zl_len_count, zl_len, 5); \
        zl_setupmode("mth", 0, zl_mth_intarg, zl_mth_anyarg, zl_mth_count, zl_mth, 6); \
        zl_setupmode("nth", 0, zl_nth_intarg, zl_nth_anyarg, zl_nth_count, zl_nth, 7); \
        zl_setupmode("reg", 0, 0, zl_reg_anyarg, zl_reg_count, zl_reg, 8); \
        zl_setupmode("rev", 0, 0, 0, zl_rev_count, zl_rev, 9); \
        zl_setupmode("rot",	0, zl_rot_intarg, 0, zl_rot_count, zl_rot, 10); \
        zl_setupmode("sect", 0, 0, zl_sect_anyarg, zl_sect_count, zl_sect, 11); \
        zl_setupmode("slice", 0, zl_slice_intarg, 0, zl_slice_count, zl_slice, 12); \
        zl_setupmode("sort", 0, zl_sort_intarg, 0, zl_sort_count, zl_sort, 13); \
        zl_setupmode("sub", 0, 0, zl_sub_anyarg, zl_sub_count, zl_sub, 14); \
        zl_setupmode("union", 0, 0, zl_union_anyarg, zl_union_count, zl_union, 15); \
        zl_setupmode("change", 0, 0, zl_change_anyarg, zl_change_count, zl_change, 16); \
        zl_setupmode("compare", 0, 0, zl_compare_anyarg, zl_compare_count, zl_compare, 17); \
        zl_setupmode("delace", 0, 0, 0, zl_delace_count, zl_delace, 18); \
        zl_setupmode("filter", 0, 0, zl_filter_anyarg, zl_filter_count, zl_filter, 19); \
        zl_setupmode("lace", 0, 0, zl_lace_anyarg, zl_lace_count, zl_lace, 20); \
        zl_setupmode("lookup", 0, 0, zl_lookup_anyarg, zl_lookup_count, zl_lookup, 21); \
        zl_setupmode("median", 0, 0, 0, zl_median_count, zl_median, 22); \
        zl_setupmode("queue", 0, 0, 0, zl_queue_count, zl_queue, 23); \
        zl_setupmode("scramble", 0, 0, zl_scramble_anyarg, zl_scramble_count, zl_scramble, 24); \
        zl_setupmode("stack", 1, 0, 0, zl_stack_count, zl_stack, 25); \
        zl_setupmode("stream", 0, zl_stream_intarg, 0, zl_stream_count, zl_stream, 26); \
        zl_setupmode("sum", 0, 0, 0, zl_sum_count, zl_sum, 27); \
        zl_setupmode("thin", 0, 0, 0, zl_thin_count, zl_thin, 28); \
        zl_setupmode("unique", 0, 0, zl_unique_anyarg, zl_unique_count, zl_unique, 29); \
        zl_setupmode("indexmap", 0, 0, zl_indexmap_anyarg, zl_indexmap_count, zl_indexmap, 30); \
        zl_setupmode("swap", 0, 0, zl_swapmode_anyarg, zl_swapmode_count, zl_swapmode, 31);     \
    }

ZL_ALIAS_SETUP(ecils)
ZL_ALIAS_SETUP(group)
ZL_ALIAS_SETUP(iter)
ZL_ALIAS_SETUP(join)
ZL_ALIAS_SETUP(len)
ZL_ALIAS_SETUP(mth)
ZL_ALIAS_SETUP(nth)
ZL_ALIAS_SETUP(reg)
ZL_ALIAS_SETUP(rev)
ZL_ALIAS_SETUP(rot)
ZL_ALIAS_SETUP(sect)
ZL_ALIAS_SETUP(slice)
ZL_ALIAS_SETUP(sort)
ZL_ALIAS_SETUP(sub)
ZL_ALIAS_SETUP(union)
ZL_ALIAS_SETUP(change)
ZL_ALIAS_SETUP(compare)
ZL_ALIAS_SETUP(delace)
ZL_ALIAS_SETUP(filter)
ZL_ALIAS_SETUP(lace)
ZL_ALIAS_SETUP(lookup)
ZL_ALIAS_SETUP(median)
ZL_ALIAS_SETUP(queue)
ZL_ALIAS_SETUP(scramble)
ZL_ALIAS_SETUP(stack)
ZL_ALIAS_SETUP(stream)
ZL_ALIAS_SETUP(sum)
ZL_ALIAS_SETUP(thin)
ZL_ALIAS_SETUP(unique)
ZL_ALIAS_SETUP(indexmap)
ZL_ALIAS_SETUP(swap)
