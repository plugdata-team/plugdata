#define CYCLONE_MAGIC_NAN 0x7FFFFFFFul
#define CYCLONE_MAGIC_INF 0x7F800000ul
#define CYCLONE_MAGIC_NEGATIVE_INF 0xFF800000ul

#include "m_imp.h" // for obj_findsignalscalar - function that returns float fields

typedef unsigned long shared_t_bitmask;

union magic_ui32_fl {
	uint32_t uif_uint32;
	t_float uif_float;
};

void magic_setnan (t_float *in);
int magic_isnan (t_float in);
int magic_isinf (t_float in);

// from old fragile (bits and pieces likely to break with any new Pd version.)

t_outconnect *magic_outlet_connections(t_outlet *o);
t_outconnect *magic_outlet_nextconnection(t_outconnect *last, t_object **destp, int *innop);

// from forky
int magic_inlet_connection(t_object *x, t_glist *glist, int inno, t_symbol *outsym);


// for bitwise classes
int32_t bitwise_getbitmask(int ac, t_atom *av);

typedef union _i32_fl {
	int32_t if_int32;
	t_float if_float;
} t_i32_fl;

typedef  union _isdenorm {
    t_float f;
    uint32_t ui;
}t_isdenorm;

static inline int BITWISE_ISDENORM(t_float f)
{
	t_isdenorm mask;
	mask.f = f;
	return ((mask.ui & 0x07f800000) == 0);
}
