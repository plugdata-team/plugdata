// from cyclone's magic stuff by matt barber

#include <m_pd.h>
#include "magic.h"
#include <g_canvas.h>
#include <stdint.h>

int magic_inlet_connection(t_object *x, t_glist *glist, int inno, t_symbol *outsym){
    t_linetraverser t;
    linetraverser_start(&t, glist);
    while(linetraverser_next(&t))
        if(t.tr_ob2 == x && t.tr_inno == inno &&
           (!outsym || outsym == outlet_getsymbol(t.tr_outlet)))
            return (1);
    return (0);
}

void magic_setnan(t_float *in) {
	union magic_ui32_fl input_u;
	input_u.uif_uint32 = MAGIC_NAN;
	*in = input_u.uif_float;
}

int magic_isnan(t_float in) {
	union magic_ui32_fl input_u;
	input_u.uif_float = in;
	return (((input_u.uif_uint32 & 0x7f800000ul) == 0x7f800000ul) &&
			(input_u.uif_uint32 & 0x007fffff));
}

