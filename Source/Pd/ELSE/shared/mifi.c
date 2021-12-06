/* Copyright (c) 2004-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "m_pd.h"
#include "mifi.h"

/* this is for GNU/Linux and also Debian GNU/Hurd and GNU/kFreeBSD */
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__) || defined(__GLIBC__)
#include <sys/types.h>
#ifndef uint32
typedef u_int32_t uint32;
#endif
#ifndef uint16
typedef u_int16_t uint16;
#endif
#ifndef uchar
typedef u_int8_t uchar;
#endif
#elif defined(_WIN32)
#ifndef uint32
typedef unsigned long uint32;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif
#elif defined(IRIX)
#ifndef uint32
typedef unsigned long uint32;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif
#elif defined(__FreeBSD__)
#include <sys/types.h>
#ifndef uint32
typedef u_int32_t uint32;
#endif
#ifndef uint16
typedef u_int16_t uint16;
#endif
#ifndef uchar
typedef u_int8_t uchar;
#endif
#else  /* __APPLE__ */
#ifndef uint32
typedef unsigned int uint32;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif
#endif

/*
#ifdef KRZYSZCZ
# include "loud.h"
# define MIFI_DEBUG
#else
# define loudbug_bug(msg)  fprintf(stderr, "BUG: %s\n", msg), bug(msg)
#endif 
*/

// #define MIFI_VERBOSE

#define MIFI_SHORTESTEVENT        2  /* singlebyte delta and one databyte */
#define MIFI_TICKEPSILON  ((double).0001)

#define MIFIHARD_HEADERSIZE      14  /* in case t_mifiheader is padded to 16 */
#define MIFIHARD_HEADERDATASIZE   6
#define MIFIHARD_TRACKHEADERSIZE  8

/* midi file standard defaults */
#define MIFIHARD_DEFBEATTICKS   192
#define MIFIHARD_DEFTEMPO    500000  /* 120 bpm in microseconds per beat */

/* user-space defaults */
#define MIFIUSER_DEFWHOLETICKS  ((double)241920)  /* whole note, 256*27*5*7 */
#define MIFIUSER_DEFTEMPO       ((double)120960)  /* 120 bpm in ticks/sec */

#define MIFIEVENT_NALLOC    256  /* LATER do some research (average max?) */
#define MIFIEVENT_INISIZE     2  /* always be able to handle channel events */
#define MIFIEVENT_MAXSYSEX  256  /* FIXME */

typedef struct _mifievent
{
    uint32  e_delay;
    uchar   e_status;
    uchar   e_channel;
    uchar   e_meta;  /* meta-event type */
    uint32  e_length;
    size_t  e_datasize;
    uchar  *e_data;
    uchar   e_dataini[MIFIEVENT_INISIZE];
} t_mifievent;

/* midi file header */
typedef struct _mifiheader
{
    char    h_type[4];
    uint32  h_length;
    uint16  h_format;
    uint16  h_ntracks;
    uint16  h_division;
} t_mifiheader;

/* midi file track header */
typedef struct _mifitrackheader
{
    char    th_type[4];
    uint32  th_length;
} t_mifitrackheader;

typedef struct _mifireadtx
{
    double  rt_wholeticks;  /* userticks per whole note (set by user) */
    double  rt_deftempo;    /* userticks per second (default, adjusted) */
    double  rt_tempo;       /* userticks per second (current) */
    double  rt_tickscoef;   /* userticks per hardtick */
    double  rt_mscoef;      /* ms per usertick (current) */
    double  rt_userbar;     /* userticks per bar */
    uint16  rt_beatticks;   /* hardticks per beat or per frame */
    double  rt_hardbar;     /* hardticks per bar */
} t_mifireadtx;

struct _mifiread
{
    t_pd         *mr_owner;
    FILE         *mr_fp;
    t_mifiheader  mr_header;
    t_mifievent   mr_event;
    uint32        mr_scoretime;  /* current time in hardticks */
    uint32        mr_tempo;      /* microseconds per beat */
    uint32        mr_meternum;
    uint32        mr_meterden;
    uchar         mr_status;
    uchar         mr_channel;
    int           mr_nevents;
    int           mr_ntempi;
    uint16        mr_hdtracks;   /* ntracks, as declared in the file header */
    uint16        mr_ntracks;    /* as actually contained in a file */
    uint16        mr_trackndx;
    t_symbol    **mr_tracknames;
    uchar         mr_nframes;    /* fps if nonzero, else use metrical time */
    uint16        mr_format;     /* anything > 0 handled as 1, FIXME */
    uint32        mr_bytesleft;  /* nbytes remaining to be read from a track */
    int           mr_pass;
    int           mr_eof;        /* set in case of early eof (error) */
    int           mr_newtrack;   /* reset after reading track's first event */
    t_mifireadtx  mr_ticks;
};

typedef struct _mifiwritetx
{
    double  wt_wholeticks;  /* userticks per whole note (set by user) */
    double  wt_deftempo;    /* userticks per second (default, adjusted) */
    double  wt_tempo;       /* userticks per second (set by user, quantized) */
    double  wt_tickscoef;   /* hardticks per usertick */
    uint16  wt_beatticks;   /* hardticks per beat or per frame (set by user) */
    double  wt_mscoef;      /* hardticks per ms */
} t_mifiwritetx;

struct _mifiwrite
{
    t_pd          *mw_owner;
    FILE          *mw_fp;
    t_mifiheader   mw_header;
    t_mifievent    mw_event;
    uint32         mw_tempo;       /* microseconds per beat */
    uint32         mw_meternum;
    uint32         mw_meterden;
    uchar          mw_status;
    uchar          mw_channel;
    int            mw_ntempi;
    uint16         mw_ntracks;
    uint16         mw_trackndx;
    t_symbol     **mw_tracknames;
    uchar          mw_nframes;     /* fps if nonzero, else use metrical time */
    uint16         mw_format;      /* anything > 0 handled as 1, FIXME */
    uint32         mw_trackbytes;  /* nbytes written to a track so far */
    int            mw_trackdirty;  /* after opentrack, before adjusttrack */
    t_mifiwritetx  mw_ticks;
};

static int mifi_swapping = 1;

static void mifi_initialize(void)
{
    unsigned short s = 1;
    unsigned char c = *(char *)(&s);
    mifi_swapping = (c != 0);
}

static void mifi_error(t_pd *x, char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    if (x)
    {
	startpost("%s's ", class_getname(*x));
	pd_error(x, "%s", buf);
    }
    else post("mifi error: %s", buf);
    va_end(ap);
}

static void mifi_warning(t_pd *x, char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    if (x)
	post("%s's warning: %s", class_getname(*x), buf);
    else
	post("mifi warning: %s", buf);
    va_end(ap);
}

static uint32 mifi_swap4(uint32 n)
{
    if (mifi_swapping)
    	return (((n & 0xff) << 24) | ((n & 0xff00) << 8) |
		((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24));
    else
	return (n);
}

static uint16 mifi_swap2(uint16 n)
{
    if (mifi_swapping)
    	return (((n & 0xff) << 8) | ((n & 0xff00) >> 8));
    else
	return (n);
}

static int mifievent_initialize(t_mifievent *ep, size_t nalloc)
{
    ep->e_length = 0;
    if ((ep->e_data = getbytes(nalloc)))
    {
	ep->e_datasize = nalloc;
	return (1);
    }
    else
    {
	ep->e_data = ep->e_dataini;
	ep->e_datasize = MIFIEVENT_INISIZE;
	return (0);
    }
}

static int mifievent_setlength(t_mifievent *ep, size_t length)
{
    if (length > ep->e_datasize)
    {
	size_t newsize = ep->e_datasize;
	while (newsize < length)
	    newsize *= 2;
	if ((ep->e_data = resizebytes(ep->e_data, ep->e_datasize, newsize)))
	    ep->e_datasize = newsize;
	else
	{
	    ep->e_length = 0;
	    /* rather hopeless... */
	    newsize = MIFIEVENT_NALLOC;
	    if ((ep->e_data = getbytes(newsize)))
		ep->e_datasize = newsize;
	    else
	    {
		ep->e_data = ep->e_dataini;
		ep->e_datasize = MIFIEVENT_INISIZE;
	    }
	    return (0);
	}
    }
    ep->e_length = (uint32)length;
    return (1);
}

static int mifievent_settext(t_mifievent *ep, unsigned type, char *text)
{
    if (type > 127)
    {
	post("bug: mifievent_settext");
	return (0);
    }
    if (mifievent_setlength(ep, strlen(text) + 1))
    {
	ep->e_status = MIFIEVENT_META;
	ep->e_meta = (uchar)type;
	strcpy(ep->e_data, text);
	return (1);
    }
    else
    {
	ep->e_status = 0;
	return (0);
    }
}

/* #ifdef MIFI_DEBUG
static void mifi_printsysex(int length, uchar *buf)
{
    loudbug_startpost("sysex:");
    while (length--)
	loudbug_startpost(" %d", (int)*buf++);
    loudbug_endpost();
}

static void mifievent_printsysex(t_mifievent *ep)
{
    mifi_printsysex(ep->e_length, ep->e_data);
}
#endif

static void mifievent_printmeta(t_mifievent *ep)
{
#if 0
    static int isprintable[MIFIMETA_MAXPRINTABLE+1] =
    {
#ifdef MIFI_DEBUG
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
#elif defined MIFI_VERBOSE
	0, 0, 1, 1, 1, 1, 1, 1
#endif
    };
    static char *printformat[MIFIMETA_MAXPRINTABLE+1] =
    {
	"", "text: %s", "copyright: %s", "track name: %s",
	"instrument name: %s", "lyric: %s", "marker: %s", "cue point: %s"
    };
    if (ep->e_meta <= MIFIMETA_MAXPRINTABLE)
    {

	if (isprintable[ep->e_meta] && printformat[ep->e_meta])
	    post(printformat[ep->e_meta], ep->e_data);
    }
#endif
 #ifdef MIFI_DEBUG
    else if (ep->e_meta == MIFIMETA_TEMPO)
    {
	int tempo = mifi_swap4(*(uint32 *)ep->e_data);
	loudbug_post("tempo (hard) %d after %d", tempo, ep->e_delay);
    }
    else if (ep->e_meta == MIFIMETA_TIMESIG)
    {
	loudbug_post("meter %d/%d after %d",
		     ep->e_data[0], (1 << ep->e_data[1]), ep->e_delay);
    }
#endif
}*/

static void mifiread_earlyeof(t_mifiread *mr)
{
    mr->mr_bytesleft = 0;
    mr->mr_eof = 1;
}

/* Get next byte from track data.  On error: return 0 (which is a valid
   result) and set mr->mr_eof. */
static uchar mifiread_getbyte(t_mifiread *mr)
{
    if (mr->mr_bytesleft)
    {
	int c;
	if ((c = fgetc(mr->mr_fp)) == EOF)
	{
	    mifiread_earlyeof(mr);
	    return (0);
	}
	else
	{
	    mr->mr_bytesleft--;
	    return ((uchar)c);
	}
    }
    else return (0);
}

static uint32 mifiread_getbytes(t_mifiread *mr, uchar *buf, uint32 size)
{
    size_t res;
    if (size > mr->mr_bytesleft)
	size = mr->mr_bytesleft;
    if ((res = fread(buf, 1, (size_t)size, mr->mr_fp)) == size)
	mr->mr_bytesleft -= res;
    else
	mifiread_earlyeof(mr);
    return (res);
}

static int mifiread_skipbytes(t_mifiread *mr, uint32 size)
{
    if (size > mr->mr_bytesleft)
	size = mr->mr_bytesleft;
    if (size)
    {
	int res = fseek(mr->mr_fp, size, SEEK_CUR);
	if (res < 0)
	    mifiread_earlyeof(mr);
	else
	    mr->mr_bytesleft -= size;
	return res;
    }
    else return (0);
}

static uint32 mifiread_getvarlen(t_mifiread *mr)
{
    uint32 n = 0;
    uchar c;
    uint32 count = mr->mr_bytesleft;
    if (count > 4)
	count = 4;
    while (count--)
    {
	n = (n << 7) + ((c = mifiread_getbyte(mr)) & 0x7f);
	if ((c & 0x80) == 0)
	    break;
    }
    return (n);
}

static size_t mifiwrite_putvarlen(t_mifiwrite *mw, uint32 n)
{
    uint32 buf = n & 0x7f;
    size_t length = 1;
    while ((n >>= 7) > 0)
    {
	buf <<= 8;
	buf |= 0x80;
	buf += n & 0x7f;
	length++;
    }
    return ((fwrite(&buf, 1, length, mw->mw_fp) == length) ? length : 0);
}

static void mifiread_updateticks(t_mifiread *mr)
{
    if (mr->mr_nframes)
    {
	mr->mr_ticks.rt_userbar = mr->mr_ticks.rt_wholeticks;
	/* LATER ntsc */
	mr->mr_ticks.rt_tickscoef = mr->mr_ticks.rt_deftempo /
	    (mr->mr_nframes * mr->mr_ticks.rt_beatticks);
	mr->mr_ticks.rt_hardbar = mr->mr_ticks.rt_userbar /
	    mr->mr_ticks.rt_tickscoef;
	mr->mr_ticks.rt_tempo = mr->mr_ticks.rt_deftempo;
    }
    else
    {
	mr->mr_ticks.rt_userbar =
	    (mr->mr_ticks.rt_wholeticks * mr->mr_meternum) / mr->mr_meterden;
	mr->mr_ticks.rt_hardbar =
	    (mr->mr_ticks.rt_beatticks * 4. * mr->mr_meternum) /
	    mr->mr_meterden;
	mr->mr_ticks.rt_tickscoef =
	    mr->mr_ticks.rt_wholeticks / (mr->mr_ticks.rt_beatticks * 4.);
	mr->mr_ticks.rt_tempo =
	    ((double)MIFIHARD_DEFTEMPO * mr->mr_ticks.rt_deftempo) /
	    ((double)mr->mr_tempo);
	if (mr->mr_ticks.rt_tempo < MIFI_TICKEPSILON)
	{
	    post("bug: mifiread_updateticks");
	    mr->mr_ticks.rt_tempo = mr->mr_ticks.rt_deftempo;
	}
    }
    mr->mr_ticks.rt_mscoef = 1000. / mr->mr_ticks.rt_tempo;
}

static void mifiread_resetticks(t_mifiread *mr)
{
    mr->mr_ticks.rt_wholeticks = MIFIUSER_DEFWHOLETICKS;
    mr->mr_ticks.rt_deftempo = MIFIUSER_DEFTEMPO;
    mr->mr_ticks.rt_beatticks = MIFIHARD_DEFBEATTICKS;
}

static void mifiread_reset(t_mifiread *mr)
{
    mr->mr_eof = 0;
    mr->mr_newtrack = 0;
    mr->mr_fp = 0;
    mr->mr_format = 0;
    mr->mr_nframes = 0;
    mr->mr_tempo = MIFIHARD_DEFTEMPO;
    mr->mr_meternum = 4;
    mr->mr_meterden = 4;
    mr->mr_ntracks = 0;
    mr->mr_status = 0;
    mr->mr_channel = 0;
    mr->mr_bytesleft = 0;
    mr->mr_pass = 0;
    mr->mr_hdtracks = 1;
    mr->mr_tracknames = 0;
    mifiread_updateticks(mr);
}

/* Calling this is optional.  The obligatory part is supplied elsewhere:
   in the constructor (owner), and in the doit() call (hook function). */
void mifiread_setuserticks(t_mifiread *mr, double wholeticks)
{
    mr->mr_ticks.rt_wholeticks = (wholeticks > MIFI_TICKEPSILON ?
				  wholeticks : MIFIUSER_DEFWHOLETICKS);
    mr->mr_ticks.rt_deftempo = mr->mr_ticks.rt_wholeticks *
	(MIFIUSER_DEFTEMPO / MIFIUSER_DEFWHOLETICKS);
    mifiread_updateticks(mr);
}

/* open a file and read its header */
static int mifiread_startfile(t_mifiread *mr, const char *filename,
			      const char *dirname, int complain)
{
    char errmess[MAXPDSTRING], path[MAXPDSTRING], *fnameptr;
    int fd;
    mr->mr_fp = 0;
    if ((fd = open_via_path(dirname, filename,
			    "", path, &fnameptr, MAXPDSTRING, 1)) < 0)
    {
	strcpy(errmess, "cannot open");
	goto rstartfailed;
    }
    close(fd);
    if (path != fnameptr)
    {
	char *slashpos = path + strlen(path);
	*slashpos++ = '/';
	/* try not to be dependent on current open_via_path() implementation */
	if (fnameptr != slashpos)
	    strcpy(slashpos, fnameptr);
    }
    if (!(mr->mr_fp = sys_fopen(path, "rb")))
    {
	strcpy(errmess, "cannot open");
	goto rstartfailed;
    }
    if (fread(&mr->mr_header, 1,
	      MIFIHARD_HEADERSIZE, mr->mr_fp) < MIFIHARD_HEADERSIZE)
    {
	strcpy(errmess, "missing header of");
	goto rstartfailed;
    }
    return (1);
rstartfailed:
    if (complain)
	mifi_error(mr->mr_owner, "%s file \"%s\" (errno %d: %s)",
		   errmess, filename, errno, strerror(errno));
    if (mr->mr_fp)
    {
	fclose(mr->mr_fp);
	mr->mr_fp = 0;
    }
    return (0);
}

static int mifiread_starttrack(t_mifiread *mr)
{
    t_mifitrackheader th;
    long skip;
    int notyet = 1;
    do {
	if (fread(&th, 1, MIFIHARD_TRACKHEADERSIZE,
		  mr->mr_fp) < MIFIHARD_TRACKHEADERSIZE)
	    goto nomoretracks;
	th.th_length = mifi_swap4(th.th_length);
	if (strncmp(th.th_type, "MTrk", 4))
	{
	    char buf[8];
	    strncpy(buf, th.th_type, 4);
	    buf[4] = 0;
	    if (mr->mr_pass == 1)
		mifi_warning(mr->mr_owner,
			     "unknown chunk %s in midi file... skipped", buf);
	}
	else if (th.th_length < MIFI_SHORTESTEVENT)
	{
	    if (mr->mr_pass == 1)
		mifi_warning(mr->mr_owner,
			     "empty track in midi file... skipped");
	}
	else notyet = 0;
	if (notyet && (skip = th.th_length) &&
	    fseek(mr->mr_fp, skip, SEEK_CUR) < 0)
	    goto nomoretracks;
    } while (notyet);
    mr->mr_scoretime = 0;
    mr->mr_newtrack = 1;
    mr->mr_status = mr->mr_channel = 0;
    mr->mr_bytesleft = th.th_length;
    return (1);
nomoretracks:
    if (mr->mr_ntracks == 0 && mr->mr_pass == 1)
	mifi_warning(mr->mr_owner, "no valid miditracks");
    return (0);
}

static int mifiread_nextevent(t_mifiread *mr)
{
    t_mifievent *ep = &mr->mr_event;
    uchar status;
    uint32 length;
    mr->mr_newtrack = 0;
nextattempt:
    if (mr->mr_bytesleft < MIFI_SHORTESTEVENT &&
	!mifiread_starttrack(mr))
	return (MIFIREAD_EOF);
    mr->mr_scoretime += (mr->mr_event.e_delay = mifiread_getvarlen(mr));
    if ((status = mifiread_getbyte(mr)) < 0x80)
    {
	if (MIFI_ISCHANNEL(mr->mr_status))
	{
	    ep->e_data[0] = status;
	    ep->e_length = 1;
	    status = mr->mr_status;
	    ep->e_channel = mr->mr_channel;
	}
	else
	{
	    if (mr->mr_pass == 1)
		mifi_warning(mr->mr_owner,
 "missing running status in midi file... skip to end of track");
	    goto endoftrack;
	}
    }
    else ep->e_length = 0;

    /* channel message */
    if (status < 0xf0)
    {
	if (ep->e_length == 0)
	{
	    ep->e_data[0] = mifiread_getbyte(mr);
	    ep->e_length = 1;
	    mr->mr_status = status & 0xf0;
	    mr->mr_channel = ep->e_channel = status & 0x0f;
	    status = mr->mr_status;
	}
	if (!MIFI_ONEDATABYTE(status))
	{
	    ep->e_data[1] = mifiread_getbyte(mr);
	    ep->e_length = 2;
	}
    }

    /* system exclusive */
    else if (status == MIFISYSEX_FIRST || status == MIFISYSEX_NEXT)
    {
	length = mifiread_getvarlen(mr);
	if (length > MIFIEVENT_MAXSYSEX)  /* FIXME optional read */
	{
	    if (mifiread_skipbytes(mr, length) < 0)
		return (MIFIREAD_FATAL);
	}
	else
	{
	    uchar *tempbuf = getbytes(length);
	    if (mifiread_getbytes(mr, tempbuf, length) != length)
		return (MIFIREAD_FATAL);
#ifdef MIFI_DEBUG
	    mifi_printsysex(length, tempbuf);
#endif
	    freebytes(tempbuf, length);
	}
	goto nextattempt;
    }

    /* meta-event */
    else if (status == MIFIEVENT_META)
    {
	ep->e_meta = mifiread_getbyte(mr);
	length = mifiread_getvarlen(mr);
	if (ep->e_meta > 127)
	{
	    /* try to skip corrupted meta-event (quietly) */
#ifdef MIFI_VERBOSE
	    if (mr->mr_pass == 1)
		mifi_warning(mr->mr_owner, "bad meta: %d > 127", ep->e_meta);
#endif
	    if (mifiread_skipbytes(mr, length) < 0)
		return (MIFIREAD_FATAL);
	    goto nextattempt;
	}
	switch (ep->e_meta)
	{
	case MIFIMETA_EOT:
	    if (length)
	    {
		/* corrupted eot: ignore and skip to the real end of track */
#ifdef MIFI_VERBOSE
		if (mr->mr_pass == 1)
		    mifi_warning(mr->mr_owner,
				 "corrupted eot, length %d", length);
#endif
		goto endoftrack;
	    }
	    break;
	case MIFIMETA_TEMPO:
	    if (length != 3)
	    {
		if (mr->mr_pass == 1)
		    mifi_warning(mr->mr_owner,
 "corrupted tempo event in midi file... skip to end of track");
		goto endoftrack;
	    }
	    if (mifiread_getbytes(mr, ep->e_data + 1, 3) != 3)
		return (MIFIREAD_FATAL);
	    ep->e_data[0] = 0;
	    mr->mr_tempo = mifi_swap4(*(uint32 *)ep->e_data);
	    if (!mr->mr_tempo)
		mr->mr_tempo = MIFIHARD_DEFTEMPO;
	    mifiread_updateticks(mr);
	    break;
	case MIFIMETA_TIMESIG:
	    if (length != 4)
	    {
		if (mr->mr_pass == 1)
		    mifi_warning(mr->mr_owner,
 "corrupted time signature event in midi file... skip to end of track");
		goto endoftrack;
	    }
	    if (mifiread_getbytes(mr, ep->e_data, 4) != 4)
		return (MIFIREAD_FATAL);
	    mr->mr_meternum = ep->e_data[0];
	    mr->mr_meterden = (1 << ep->e_data[1]);
	    if (!mr->mr_meternum || !mr->mr_meterden)
		mr->mr_meternum = mr->mr_meterden = 4;
	    mifiread_updateticks(mr);
/* #ifdef MIFI_DEBUG
	    if (mr->mr_pass == 1)
		loudbug_post("barspan (hard) %g", mr->mr_ticks.rt_hardbar);
#endif */
	    break;
	default:
	    if (length + 1 > MIFIEVENT_NALLOC)
	    {
		if (mifiread_skipbytes(mr, length) < 0)
		    return (MIFIREAD_FATAL);
		goto nextattempt;
	    }
	    if (mifiread_getbytes(mr, ep->e_data, length) != length)
		return (MIFIREAD_FATAL);
	    ep->e_length = length;
	    if (ep->e_meta && ep->e_meta <= MIFIMETA_MAXPRINTABLE)
		ep->e_data[length] = '\0';  /* text meta-event nultermination */
	}
    }
    else
    {
	if (mr->mr_pass == 1)
	    mifi_warning(mr->mr_owner,
 "unknown event type in midi file... skip to end of track");
	goto endoftrack;
    }
    return ((ep->e_status = status) == MIFIEVENT_META ? ep->e_meta : status);
endoftrack:
    if (mifiread_skipbytes(mr, mr->mr_bytesleft) < 0)
	return (MIFIREAD_FATAL);
    else
	return (MIFIREAD_SKIP);
}

static int mifiread_restart(t_mifiread *mr, int complain)
{
    mr->mr_eof = 0;
    mr->mr_newtrack = 0;
    mr->mr_status = 0;
    mr->mr_channel = 0;
    mr->mr_bytesleft = 0;
    mr->mr_pass = 0;
    if (fseek(mr->mr_fp, 0, SEEK_SET))
    {
	if (complain)
	    mifi_error(mr->mr_owner,
		       "file error (errno %d: %s)", errno, strerror(errno));
	return (0);
    }
    else return (1);
}

static int mifiread_doopen(t_mifiread *mr, const char *filename,
			   const char *dirname, int complain)
{
    long skip;
    mifiread_reset(mr);
    if (!mifiread_startfile(mr, filename, dirname, complain))
	return (0);
    if (strncmp(mr->mr_header.h_type, "MThd", 4))
	goto badheader;
    mr->mr_header.h_length = mifi_swap4(mr->mr_header.h_length);
    if (mr->mr_header.h_length < MIFIHARD_HEADERDATASIZE)
	goto badheader;
    if ((skip = mr->mr_header.h_length - MIFIHARD_HEADERDATASIZE))
    {
	mifi_warning(mr->mr_owner,
		     "%ld extra bytes of midi file header... skipped", skip);
	if (fseek(mr->mr_fp, skip, SEEK_CUR) < 0)
	    goto badstart;
    }
    mr->mr_format = mifi_swap2(mr->mr_header.h_format);
    mr->mr_hdtracks = mifi_swap2(mr->mr_header.h_ntracks);
    if (mr->mr_hdtracks > 1000)  /* a sanity check */
	mifi_warning(mr->mr_owner, "%d tracks declared in midi file \"%s\"",
		     mr->mr_hdtracks, filename);
    mr->mr_tracknames = getbytes(mr->mr_hdtracks * sizeof(*mr->mr_tracknames));
    mr->mr_ticks.rt_beatticks = mifi_swap2(mr->mr_header.h_division);
    if (mr->mr_ticks.rt_beatticks & 0x8000)
    {
	mr->mr_nframes = (mr->mr_ticks.rt_beatticks >> 8);
	mr->mr_ticks.rt_beatticks &= 0xff;
    }
    else mr->mr_nframes = 0;
    if (mr->mr_ticks.rt_beatticks == 0)
	goto badheader;
    mifiread_updateticks(mr);
/* #ifdef MIFI_DEBUG
    if (mr->mr_nframes)
	loudbug_post(
	    "midi file (format %d): %d tracks, %d ticks (%d smpte frames)",
	    mr->mr_format, mr->mr_hdtracks,
	    mr->mr_ticks.rt_beatticks, mr->mr_nframes);
    else
	loudbug_post("midi file (format %d): %d tracks, %d ticks per beat",
		     mr->mr_format, mr->mr_hdtracks, mr->mr_ticks.rt_beatticks);
#endif*/
    return (1);
badheader:
    if (complain)
	mifi_error(mr->mr_owner, "\"%s\" is not a valid midi file", filename);
badstart:
    fclose(mr->mr_fp);
    mr->mr_fp = 0;
    return (0);
}

/* Gather statistics (nevents, ntracks, ntempi), pick track names, and
   allocate the maps.  To be called in the first pass of reading.
   LATER consider optional reading of nonchannel events. */
/* FIXME complaining */
static int mifiread_analyse(t_mifiread *mr, int complain)
{
    t_mifievent *ep = &mr->mr_event;
    int i, evtype, isnewtrack = 0;
    char tnamebuf[MAXPDSTRING];
    t_symbol **tnamep = 0;

    mr->mr_pass = 1;
    *tnamebuf = '\0';
    mr->mr_ntracks = 0;
    mr->mr_nevents = 0;
    mr->mr_ntempi = 0;
    while ((evtype = mifiread_nextevent(mr)) >= MIFIREAD_SKIP)
    {
	if (evtype == MIFIREAD_SKIP)
	    continue;
	if (mr->mr_newtrack)
	{
/* #ifdef MIFI_DEBUG
	    loudbug_post("track %d", mr->mr_ntracks);
#endif */
	    isnewtrack = 1;
	    *tnamebuf = '\0';
	    tnamep = 0;  /* set to nonzero for nonempty tracks only */
	}
	if (MIFI_ISCHANNEL(evtype))
	{
	    if (isnewtrack)
	    {
		isnewtrack = 0;
		tnamep = mr->mr_tracknames + mr->mr_ntracks;
		mr->mr_ntracks++;
		if (mr->mr_ntracks > mr->mr_hdtracks)
		{
		    if (complain)
			mifi_error(mr->mr_owner,
 "midi file has more tracks than header-declared %d", mr->mr_hdtracks);
		    /* FIXME grow? */
		    goto anafail;
		}
		if (*tnamebuf)
		{
		    *tnamep = gensym(tnamebuf);
/* #ifdef MIFI_DEBUG
		    loudbug_post("nonempty track name %s", (*tnamep)->s_name);
#endif */
		}
		else *tnamep = &s_;
	    }
	    mr->mr_nevents++;
	}
	/* FIXME sysex */
	else if (evtype < 0x80)
	{
//	    mifievent_printmeta(ep);
	    if (evtype == MIFIMETA_TEMPO)
		mr->mr_ntempi++;
	    else if (evtype == MIFIMETA_TRACKNAME)
	    {
		char *p1 = ep->e_data;
		if (*p1 &&
		    !*tnamebuf) /* take the first one */
		{
		    while (*p1 == ' ') p1++;
		    if (*p1)
		    {
			char *p2 = ep->e_data + ep->e_length - 1;
			while (p2 > p1 && *p2 == ' ') *p2-- = '\0';
			p2 = p1;
			do if (*p2 == ' ' || *p2 == ',' || *p2 == ';')
			    *p2 = '-';
			while (*++p2);
			if (tnamep)
			{
			    if (*tnamep == &s_)
				/* trackname after channel-event */
				*tnamep = gensym(p1);
			    else
				strcpy(tnamebuf, p1);
			}
		    }
		}
	    }
	}
    }
    if (evtype == MIFIREAD_EOF)
    {
	for (i = 0, tnamep = mr->mr_tracknames; i < mr->mr_ntracks; i++, tnamep++)
	{
	    if (!*tnamep || *tnamep == &s_)
	    {
		sprintf(tnamebuf, "%d-track", i);
		*tnamep = gensym(tnamebuf);
	    }
	}
#ifdef MIFI_VERBOSE
	post("got %d midi tracks (out of %d)", mr->mr_ntracks, mr->mr_hdtracks);
#endif
	return (MIFIREAD_EOF);
    }
    else return (evtype);
anafail:
    return (MIFIREAD_FATAL);
}

/* to be called in the second pass of reading */
int mifiread_doit(t_mifiread *mr, t_mifireadhook hook, void *hookdata)
{
    int evtype, ntracks = 0, isnewtrack = 0;
    mr->mr_pass = 2;
    mr->mr_trackndx = 0;
    while ((evtype = mifiread_nextevent(mr)) >= MIFIREAD_SKIP)
    {
	if (evtype == MIFIREAD_SKIP)
	    continue;
	if (mr->mr_newtrack)
	    isnewtrack = 1;
	if (isnewtrack && MIFI_ISCHANNEL(evtype))
	{
	    isnewtrack = 0;
	    mr->mr_trackndx = ntracks++;
	    if (ntracks > mr->mr_ntracks)
	    {
		post("bug: mifiread_doit: too many tracks");
		goto doitfail;
	    }
	    if (!mr->mr_tracknames[mr->mr_trackndx] ||
		mr->mr_tracknames[mr->mr_trackndx] == &s_)
	    {
		post("bug: mifiread_doit: empty track name");
		mr->mr_tracknames[mr->mr_trackndx] = gensym("bug-track");
	    }
	}
	if (!hook(mr, hookdata, evtype))
	    goto doitfail;
    }
    if (evtype == MIFIREAD_EOF)
    {
/* #ifdef MIFI_DEBUG
	if (evtype == MIFIREAD_EOF)
	    loudbug_post("finished reading %d events from midi file",
			 mr->mr_nevents);
#endif */
	return (MIFIREAD_EOF);
    }
doitfail:
    return (MIFIREAD_FATAL);
}

/* mifiread_get... calls to be used in the main read routine */

int mifiread_getnevents(t_mifiread *mr)
{
    return (mr->mr_nevents);
}

int mifiread_getntempi(t_mifiread *mr)
{
    return (mr->mr_ntempi);
}

int mifiread_gethdtracks(t_mifiread *mr)
{
    return (mr->mr_hdtracks);
}

int mifiread_getformat(t_mifiread *mr)
{
    return (mr->mr_format);
}

int mifiread_getnframes(t_mifiread *mr)
{
    return (mr->mr_nframes);
}

int mifiread_getbeatticks(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_beatticks);
}

double mifiread_getdeftempo(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_deftempo);
}

/* mifiread_get... calls to be used in a mifireadhook */

int mifiread_getbarindex(t_mifiread *mr)
{
    return (mr->mr_scoretime / (int)mr->mr_ticks.rt_hardbar);
}

double mifiread_getbarspan(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_userbar);
}

double mifiread_gettick(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_tickscoef *
	    (mr->mr_scoretime % (int)mr->mr_ticks.rt_hardbar));
}

double mifiread_getscoretime(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_tickscoef * mr->mr_scoretime);
}

double mifiread_gettempo(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_tempo);
}

double mifiread_getmscoef(t_mifiread *mr)
{
    return (mr->mr_ticks.rt_mscoef);
}

t_symbol *mifiread_gettrackname(t_mifiread *mr)
{
    if (mr->mr_pass == 2 &&
	mr->mr_tracknames &&
	mr->mr_trackndx < mr->mr_ntracks)
	return (mr->mr_tracknames[mr->mr_trackndx]);
    else
    {
	post("bug: mifiread_gettrackname");
	return (0);
    }
}

unsigned mifiread_getstatus(t_mifiread *mr)
{
    if (mr->mr_pass != 2)
	post("bug: mifiread_getstatus");
    return (mr->mr_event.e_status);
}

unsigned mifiread_getdata1(t_mifiread *mr)
{
    if (mr->mr_pass != 2)
	post("bug: mifiread_getdata1");
    return (mr->mr_event.e_data[0]);
}

unsigned mifiread_getdata2(t_mifiread *mr)
{
    if (mr->mr_pass != 2)
	post("bug: mifiread_getdata2");
    if (mr->mr_event.e_length < 2)
	post("bug: mifiread_getdata2");
    return (mr->mr_event.e_data[1]);
}

unsigned mifiread_getchannel(t_mifiread *mr)
{
    if (mr->mr_pass != 2)
	post("bug: mifiread_getchannel");
    return (mr->mr_event.e_channel);
}

t_pd *mifiread_getowner(t_mifiread *mr)
{
    return (mr->mr_owner);
}

int mifiread_open(t_mifiread *mr, const char *filename,
		  const char *dirname, int complain)
{
    return (mifiread_doopen(mr, filename, dirname, complain) &&
	    (mifiread_analyse(mr, complain) == MIFIREAD_EOF) &&
	    mifiread_restart(mr, complain));
}

void mifiread_close(t_mifiread *mr)
{
    mr->mr_pass = 0;
    if (mr->mr_fp)
    {
	fclose(mr->mr_fp);
	mr->mr_fp = 0;
    }
    if (mr->mr_tracknames)
	freebytes(mr->mr_tracknames,
		  mr->mr_hdtracks * sizeof(*mr->mr_tracknames));
}

void mifiread_free(t_mifiread *mr)
{
    mifiread_close(mr);
    if (mr->mr_event.e_data != mr->mr_event.e_dataini)
	freebytes(mr->mr_event.e_data, mr->mr_event.e_datasize);
    freebytes(mr, sizeof(*mr));
}

t_mifiread *mifiread_new(t_pd *owner)
{
    t_mifiread *mr = getbytes(sizeof(*mr));
    mifi_initialize();
    mr->mr_owner = owner;
    mifievent_initialize(&mr->mr_event, MIFIEVENT_NALLOC);
    mifiread_resetticks(mr);
    mifiread_reset(mr);
    return (mr);
}

static void mifiwrite_updateticks(t_mifiwrite *mw)
{
    if (mw->mw_nframes)
    {
	/* LATER ntsc */
	mw->mw_ticks.wt_tickscoef =
	    (mw->mw_nframes * mw->mw_ticks.wt_beatticks) /
	    mw->mw_ticks.wt_deftempo;
	mw->mw_ticks.wt_tempo = mw->mw_ticks.wt_deftempo;
	mw->mw_ticks.wt_mscoef =
	    .001 * (mw->mw_nframes * mw->mw_ticks.wt_beatticks);
    }
    else
    {
	mw->mw_ticks.wt_tickscoef =
	    (mw->mw_ticks.wt_beatticks * 4.) / mw->mw_ticks.wt_wholeticks;
	mw->mw_ticks.wt_tempo =
	    ((double)MIFIHARD_DEFTEMPO * mw->mw_ticks.wt_deftempo) /
	    ((double)mw->mw_tempo);
	if (mw->mw_ticks.wt_tempo < MIFI_TICKEPSILON)
	{
	    post("bug: mifiwrite_updateticks");
	    mw->mw_ticks.wt_tempo = mw->mw_ticks.wt_deftempo;
	}
	mw->mw_ticks.wt_mscoef =
	    (1000. * mw->mw_ticks.wt_beatticks) / mw->mw_tempo;
    }
}

static void mifiwrite_resetticks(t_mifiwrite *mw)
{
    mw->mw_ticks.wt_wholeticks = MIFIUSER_DEFWHOLETICKS;
    mw->mw_ticks.wt_deftempo = MIFIUSER_DEFTEMPO;
    mw->mw_ticks.wt_beatticks = MIFIHARD_DEFBEATTICKS;
}

static void mifiwrite_reset(t_mifiwrite *mw)
{
    mw->mw_trackndx = 0;
    mw->mw_trackdirty = 0;
    mw->mw_fp = 0;
    mw->mw_format = 1;  /* LATER settable parameter */
    mw->mw_nframes = 0;
    mw->mw_meternum = 4;
    mw->mw_meterden = 4;
    mw->mw_status = 0;
    mw->mw_channel = 0;
    mw->mw_trackbytes = 0;
    mifiwrite_updateticks(mw);
}

void mifiwrite_sethardticks(t_mifiwrite *mw, int beatticks)
{
    mw->mw_ticks.wt_beatticks =
	(beatticks > 0 && beatticks < MIFI_MAXBEATTICKS ?
	 beatticks : MIFIHARD_DEFBEATTICKS);
    mifiwrite_updateticks(mw);
}

void mifiwrite_setuserticks(t_mifiwrite *mw, double wholeticks)
{
    mw->mw_ticks.wt_wholeticks = (wholeticks > MIFI_TICKEPSILON ?
				  wholeticks : MIFIUSER_DEFWHOLETICKS);
    mw->mw_ticks.wt_deftempo = mw->mw_ticks.wt_wholeticks *
	(MIFIUSER_DEFTEMPO / MIFIUSER_DEFWHOLETICKS);
    mifiwrite_updateticks(mw);
}

void mifiwrite_setusertempo(t_mifiwrite *mw, double tickspersec)
{
    mw->mw_tempo = (tickspersec > MIFI_TICKEPSILON ?
		    ((double)MIFIHARD_DEFTEMPO * mw->mw_ticks.wt_deftempo) /
		    tickspersec : MIFIHARD_DEFTEMPO);
    mifiwrite_updateticks(mw);
}

/* LATER analyse shrinking effect caused by truncation */
static int mifiwrite_putnextevent(t_mifiwrite *mw, t_mifievent *ep)
{
    uchar buf[3], *ptr = buf;
    size_t size = mifiwrite_putvarlen(mw, ep->e_delay);
    if (!size)
	return (0);
    mw->mw_trackbytes += size;
    if (MIFI_ISCHANNEL(ep->e_status))
    {
	if ((*ptr = ep->e_status | ep->e_channel) == mw->mw_status)
	    size = 1;
	else
	{
	    mw->mw_status = *ptr++;
	    size = 2;
	}
	*ptr++ = ep->e_data[0];
	if (!MIFI_ONEDATABYTE(ep->e_status))
	{
	    *ptr = ep->e_data[1];
	    size++;
	}
	ptr = buf;
    }
    else if (ep->e_status == MIFIEVENT_META)
    {
	mw->mw_status = 0;  /* sysex and meta cancel any running status */
	buf[0] = ep->e_status;
	buf[1] = ep->e_meta;
	if (fwrite(buf, 1, 2, mw->mw_fp) != 2)
	    return (0);
	mw->mw_trackbytes += 2;
	size = mifiwrite_putvarlen(mw, ep->e_length);
	if (!size)
	    return (0);
	mw->mw_trackbytes += size;
	size = ep->e_length;
	ptr = ep->e_data;
    }
    else return (0);
    if (size)
    {
	if (fwrite(ptr, 1, size, mw->mw_fp) != size)
	    return (0);
	mw->mw_trackbytes += size;
    }
    return (1);
}

/* open a midi file for saving, write the header */
int mifiwrite_open(t_mifiwrite *mw, const char *filename,
		   const char *dirname, int ntracks, int complain)
{
    char errmess[MAXPDSTRING], fnamebuf[MAXPDSTRING];
    if (ntracks < 1 || ntracks > MIFI_MAXTRACKS)
    {
	post("bug: mifiwrite_open 1");
	complain = 0;
	goto wopenfailed;
    }
    mw->mw_ntracks = ntracks;
    mifiwrite_reset(mw);
    if (mw->mw_format == 0)
    {
	if (mw->mw_ntracks != 1)
	{  /* LATER consider replacing with a warning */
	    post("bug: mifiwrite_open 2");
	    complain = 0;
	    goto wopenfailed;
	}
#ifdef MIFI_VERBOSE
	post("writing single-track midi file \"%s\"", filename);
#endif
    }
#ifdef MIFI_VERBOSE
    else post("writing midi file \"%s\" (%d tracks)", filename, mw->mw_ntracks);
#endif
    strncpy(mw->mw_header.h_type, "MThd", 4);
    mw->mw_header.h_length = mifi_swap4(MIFIHARD_HEADERDATASIZE);
    mw->mw_header.h_format = mifi_swap2(mw->mw_format);
    mw->mw_header.h_ntracks = mifi_swap2(mw->mw_ntracks);
    if (mw->mw_nframes)
	mw->mw_header.h_division =
	    ((mw->mw_nframes << 8) | mw->mw_ticks.wt_beatticks) | 0x8000;
    else
	mw->mw_header.h_division = mw->mw_ticks.wt_beatticks & 0x7fff;
    mw->mw_header.h_division = mifi_swap2(mw->mw_header.h_division);
    fnamebuf[0] = 0;
    if (*dirname)
    	strcat(fnamebuf, dirname), strcat(fnamebuf, "/");
    strcat(fnamebuf, filename);
    if (!(mw->mw_fp = sys_fopen(fnamebuf, "wb")))
    {
	strcpy(errmess, "cannot open");
	goto wopenfailed;
    }
    if (fwrite(&mw->mw_header, 1,
	       MIFIHARD_HEADERSIZE, mw->mw_fp) < MIFIHARD_HEADERSIZE)
    {
	strcpy(errmess, "cannot write header of");
	goto wopenfailed;
    }
    return (1);
wopenfailed:
    if (complain)
	mifi_error(mw->mw_owner, "%s file \"%s\" (errno %d: %s)",
		   errmess, filename, errno, strerror(errno));
    if (mw->mw_fp)
    {
	fclose(mw->mw_fp);
	mw->mw_fp = 0;
    }
    return (0);
}

/* append eot meta and update length field in a track header */
static int mifiwrite_adjusttrack(t_mifiwrite *mw, uint32 eotdelay, int complain)
{
    t_mifievent *ep = &mw->mw_event;
    long skip;
    uint32 length;
    mw->mw_trackdirty = 0;
    ep->e_delay = eotdelay;
    ep->e_status = MIFIEVENT_META;
    ep->e_meta = MIFIMETA_EOT;
    ep->e_length = 0;
    if (!mifiwrite_putnextevent(mw, ep))
	return (0);
    skip = mw->mw_trackbytes + 4;
    length = mifi_swap4(mw->mw_trackbytes);
/* #ifdef MIFI_DEBUG
    loudbug_post("adjusting track size to %d", mw->mw_trackbytes);
#endif */
    /* LATER add sanity check (compare to saved filepos) */
    if (skip > 4 &&
	(fseek(mw->mw_fp, -skip, SEEK_CUR) < 0 ||
	fwrite(&length, 1, 4, mw->mw_fp) != 4 ||
	fseek(mw->mw_fp, 0, SEEK_END) < 0)){
	if (complain)
	    mifi_error(mw->mw_owner,
		       "unable to adjust length field to %d in a midi file\
 track header (errno %d: %s)", mw->mw_trackbytes, errno, strerror(errno));
	return (0);
    }
    return (1);
}

int mifiwrite_opentrack(t_mifiwrite *mw, char *trackname, int complain)
{
    t_mifitrackheader th;
    if (mw->mw_trackdirty && !mifiwrite_adjusttrack(mw, 0, complain))
	return (0);
    if (mw->mw_trackndx > mw->mw_ntracks)
	return (0);
    else if (mw->mw_trackndx++ == mw->mw_ntracks)
    {
	post("bug: mifiwrite_opentrack");
	return (0);
    }
    strncpy(th.th_type, "MTrk", 4);
    th.th_length = 0;
    mw->mw_status = mw->mw_channel = 0;
    mw->mw_trackbytes = 0;
    if (fwrite(&th, 1, MIFIHARD_TRACKHEADERSIZE,
	       mw->mw_fp) != MIFIHARD_TRACKHEADERSIZE)
    {
	if (complain)
	    mifi_error(mw->mw_owner,
		       "unable to write midi file header (errno %d: %s)",
		       errno, strerror(errno));
	return (0);
    }
    if (trackname)
    {
	if (!mifiwrite_textevent(mw, 0., MIFIMETA_TRACKNAME, trackname))
	{
	    if (complain)
		mifi_error(mw->mw_owner,
 "unable to write midi file track name \"%s\" (errno %d: %s)",
			   trackname, errno, strerror(errno));
	    return (0);
	}
    }
    mw->mw_trackdirty = 1;
    return (1);
}

/* calling this is optional (if skipped, enddelay defaults to 0.) */
int mifiwrite_closetrack(t_mifiwrite *mw, double enddelay, int complain)
{
    if (mw->mw_trackdirty)
    {
	uint32 eotdelay = (uint32)(enddelay * mw->mw_ticks.wt_mscoef);
	return (mifiwrite_adjusttrack(mw, eotdelay, complain));
    }
    else
    {
	post("bug: mifiwrite_closetrack");
	return (0);
    }
}

int mifiwrite_textevent(t_mifiwrite *mw, double delay,
			unsigned type, char *text)
{
    t_mifievent *ep = &mw->mw_event;
    if (!mifievent_settext(ep, type, text))
	return (0);
    ep->e_delay = (uint32)(delay * mw->mw_ticks.wt_mscoef);
    return (mifiwrite_putnextevent(mw, ep));
}

int mifiwrite_channelevent(t_mifiwrite *mw, double delay, unsigned status,
			   unsigned channel, unsigned data1, unsigned data2)
{
    t_mifievent *ep = &mw->mw_event;
    int shorter = MIFI_ONEDATABYTE(status);
    if (!MIFI_ISCHANNEL(status) || channel > 15 || data1 > 127
	|| (!shorter && data2 > 127))
    {
	post("bug: mifiwrite_channelevent");
	return (0);
    }
    ep->e_delay = (uint32)(delay * mw->mw_ticks.wt_mscoef);
    ep->e_status = (uchar)(status & 0xf0);
    ep->e_channel = (uchar)channel;
    ep->e_data[0] = (uchar)data1;
    if (shorter)
	ep->e_length = 1;
    else
    {
	ep->e_data[1] = (uchar)data2;
	ep->e_length = 2;
    }
    return (mifiwrite_putnextevent(mw, ep));
}

void mifiwrite_close(t_mifiwrite *mw)
{
    if (mw->mw_trackdirty)
	mifiwrite_adjusttrack(mw, 0, 0);
    if (mw->mw_fp)
    {
	fclose(mw->mw_fp);
	mw->mw_fp = 0;
    }
}

void mifiwrite_free(t_mifiwrite *mw)
{
    mifiwrite_close(mw);
    if (mw->mw_event.e_data != mw->mw_event.e_dataini)
	freebytes(mw->mw_event.e_data, mw->mw_event.e_datasize);
    freebytes(mw, sizeof(*mw));
}

t_mifiwrite *mifiwrite_new(t_pd *owner)
{
    t_mifiwrite *mw = getbytes(sizeof(*mw));
    mifi_initialize();
    mw->mw_owner = owner;
    mw->mw_ntracks = 0;
    mw->mw_tempo = MIFIHARD_DEFTEMPO;
    mifievent_initialize(&mw->mw_event, MIFIEVENT_NALLOC);
    mifiwrite_resetticks(mw);
    mifiwrite_reset(mw);
    return (mw);
}
