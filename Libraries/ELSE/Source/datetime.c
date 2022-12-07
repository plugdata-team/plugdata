// porres

#include "m_pd.h"
#include <time.h>

#if (defined __WIN32__)
# if (defined __i386__) && (defined __MINGW32__)
// unless compiling under mingw/32bit, we want USE_TIMEB in redmond-land
# else
#  define USE_TIMEB
# endif
#endif

#ifdef _MSC_VER
#  define USE_TIMEB
#endif

#ifdef __APPLE__
#include <sys/types.h>
#endif

#ifdef USE_TIMEB
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

static t_class *time_class;

typedef struct _time{
    t_object   x_obj;
    t_outlet  *x_outlet;
    struct tm *x_resolvetime;
}t_time;

const char * weekday[] = { "Sunday", "Monday", "Tuesday", "Wednesday",
                        "Thursday", "Friday", "Saturday"};

static void time_resolve(t_time *x){
#ifdef USE_TIMEB
    struct timeb mytime;
    ftime(&mytime);
    x->x_resolvetime = localtime(&mytime.time);
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    x->x_resolvetime = localtime(&tv.tv_sec);
#endif
}

static void time_bang(t_time *x){
    time_resolve(x);
    t_atom at[4];
    SETFLOAT(at+0, (t_float)x->x_resolvetime->tm_year + 1900);
    SETFLOAT(at+1, (t_float)x->x_resolvetime->tm_mon + 1);
    SETFLOAT(at+2, (t_float)x->x_resolvetime->tm_mday);
    SETSYMBOL(at+3, gensym(weekday[x->x_resolvetime->tm_wday]));
    outlet_list(x->x_outlet, &s_list, 4, at);
    SETFLOAT(at+0, (t_float)x->x_resolvetime->tm_hour);
    SETFLOAT(at+1, (t_float)x->x_resolvetime->tm_min);
    SETFLOAT(at+2, (t_float)x->x_resolvetime->tm_sec);
    outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
}

static void *time_new(void){
    t_time *x = (t_time *)pd_new(time_class);
    outlet_new(&x->x_obj, gensym("list"));
    x->x_outlet = outlet_new(&x->x_obj, gensym("list"));
    return(x);
}

void datetime_setup(void){
    time_class = class_new(gensym("datetime"), (t_newmethod)time_new,
        0, sizeof(t_time), CLASS_DEFAULT, 0);
    class_addbang(time_class, time_bang);
}
