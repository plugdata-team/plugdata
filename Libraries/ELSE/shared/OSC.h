/* packingOSC.h */
#ifndef _PACKINGOSC
#include "m_pd.h"

    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <sys/timeb.h>
#else
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <ctype.h>
    #include <sys/time.h>
#endif /* _WIN32 */

/* Declarations */
#ifdef WIN32
  typedef unsigned __int64 osc_time_t;
#else
  typedef unsigned long long osc_time_t;
#endif

#define MAX_MESG 65536 /* same as MAX_UDP_PACKET */
/* The maximum depth of bundles within bundles within bundles within...
   This is the size of a static array.  If you exceed this limit you'll
   get an error message. */
#define MAX_BUNDLE_NESTING 32
typedef struct
{
    uint32_t seconds;
    uint32_t fraction;
} OSCTimeTag;

typedef union
{
    int     i;
    float   f;
} intfloat32;

#endif // _PACKINGOSC
/* end of packingOSC.h */
