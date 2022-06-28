// from old fragile (bits and pieces likely to break with any new Pd version.)
// and forky

#include "m_pd.h"
#include "magicbit.h"
#include "g_canvas.h"
#include <stdint.h> // needed?
#include <string.h> // needed?

struct _outlet // local to m_obj.c.
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};
/* LATER export write access to o_connections field ('grab' class).
LATER encapsulate 'traverseoutlet' routines (not in the stable API yet). */


void magic_setnan(t_float *in) {
	union magic_ui32_fl input_u;
	input_u.uif_uint32 = CYCLONE_MAGIC_NAN;
	*in = input_u.uif_float;
}

int magic_isnan(t_float in) {
	union magic_ui32_fl input_u;
	input_u.uif_float = in;
	return (((input_u.uif_uint32 & 0x7f800000ul) == 0x7f800000ul) &&
			(input_u.uif_uint32 & 0x007fffff));
}

int magic_isinf(t_float in) {
	union magic_ui32_fl input_u;
	input_u.uif_float = in;
	return ( input_u.uif_uint32 == CYCLONE_MAGIC_INF ||
		input_u.uif_uint32 == CYCLONE_MAGIC_NEGATIVE_INF);
}

t_outconnect *magic_outlet_connections(t_outlet *o) // obj_starttraverseoutlet() replacement
    {
    return (o ? o->o_connections : 0);
    }

t_outconnect *magic_outlet_nextconnection(t_outconnect *last, t_object **destp, int *innop)
    {
    t_inlet *dummy;
    return (obj_nexttraverseoutlet(last, destp, &dummy, innop));
    }

int magic_inlet_connection(t_object *x, t_glist *glist, int inno, t_symbol *outsym)
{
    t_linetraverser t;
    linetraverser_start(&t, glist);
    while (linetraverser_next(&t))
        if (t.tr_ob2 == x && t.tr_inno == inno &&
            (!outsym || outsym == outlet_getsymbol(t.tr_outlet)))
        return (1);
    return (0);
}

/*---------------------------------------*/

int32_t bitwise_getbitmask(int ac, t_atom *av){
    int32_t result = 0;
    if (sizeof(shared_t_bitmask) >= sizeof(int32_t)){
        int32_t nbits = sizeof(int32_t) * 8;
        if (ac > nbits)
            ac = nbits;
        while (ac--){
            if (av->a_type == A_FLOAT &&
            (int)av->a_w.w_float)  /* CHECKED */
		    result += 1 << ac;
            av++;
        }
    }
    else bug("sizeof(shared_t_bitmask)");
    return (result);
}
