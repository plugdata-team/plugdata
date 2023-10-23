/* Copyright (c) 2004-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __MIFI_H__
#define __MIFI_H__

EXTERN_STRUCT _mifiread;
#define t_mifiread  struct _mifiread
EXTERN_STRUCT _mifiwrite;
#define t_mifiwrite  struct _mifiwrite

typedef int (*t_mifireadhook)(t_mifiread *mf, void *hookdata, int evtype);

#define MIFI_MAXTRACKS     0x7fff
#define MIFI_MAXBEATTICKS  0x7fff

/* event types, as returned by mifiread_nextevent(), ... */

#define MIFIREAD_FATAL  -3  /* unexpected eof, last track's or file error */
#define MIFIREAD_EOF    -2  /* regular eof */
#define MIFIREAD_SKIP   -1  /* error and successful skip to the next track */

#define MIFIMETA_SEQNUM           0
#define MIFIMETA_TEXT             1
#define MIFIMETA_COPYRIGHT        2
#define MIFIMETA_TRACKNAME        3
#define MIFIMETA_INSTRUMENT       4
#define MIFIMETA_LYRIC            5
#define MIFIMETA_MARKER           6
#define MIFIMETA_CUE              7
#define MIFIMETA_MAXPRINTABLE    15  /* 1..15 are various text meta-events */
#define MIFIMETA_CHANNEL       0x20  /* channel prefix (obsolete) */
#define MIFIMETA_PORT          0x21  /* port prefix (obsolete) */
#define MIFIMETA_EOT           0x2f  /* end of track */
#define MIFIMETA_TEMPO         0x51
#define MIFIMETA_SMPTE         0x54  /* SMPTE offset */
#define MIFIMETA_TIMESIG       0x58  /* time signature */
#define MIFIMETA_KEYSIG        0x59  /* key signature */
#define MIFIMETA_PROPRIETARY   0x7f

/* ...channel status codes go here, too obvious to #define... */

#define MIFISYSEX_FIRST        0xf0
#define MIFISYSEX_NEXT         0xf7
#define MIFISYSEX_ESCAPE       0xf7  /* without preceding MIFISYSEX_FIRST */

/* this code is not returned as an event type, but in e_status of t_mifievent */
#define MIFIEVENT_META         0xff

/* system messages (expected inside of sysex escape events) */
#define MIFISYS_SONGPOINTER    0xf2
#define MIFISYS_SONGSELECT     0xf3
#define MIFISYS_TUNEREQUEST    0xf6
#define MIFISYS_CLOCK          0xf8
#define MIFISYS_START          0xfa
#define MIFISYS_CONTINUE       0xfb
#define MIFISYS_STOP           0xfc
#define MIFISYS_ACTIVESENSING  0xfe

/* true if one of channel messages */
#define MIFI_ISCHANNEL(status)    (((status) & 0x80) && (status) < 0xf0)
/* true if one of the two shorter channel messages */
#define MIFI_ONEDATABYTE(status)  (((status) & 0xe0) == 0xc0)

int mifiread_getnevents(t_mifiread *mr);
int mifiread_getntempi(t_mifiread *mr);
int mifiread_gethdtracks(t_mifiread *mr);
int mifiread_getformat(t_mifiread *mr);
int mifiread_getnframes(t_mifiread *mr);
int mifiread_getbeatticks(t_mifiread *mr);
double mifiread_getdeftempo(t_mifiread *mr);

int mifiread_getbarindex(t_mifiread *mr);
double mifiread_getbarspan(t_mifiread *mr);
double mifiread_gettick(t_mifiread *mr);
double mifiread_getscoretime(t_mifiread *mr);
double mifiread_gettempo(t_mifiread *mr);
double mifiread_getmscoef(t_mifiread *mr);
t_symbol *mifiread_gettrackname(t_mifiread *mr);
unsigned mifiread_getstatus(t_mifiread *mr);
unsigned mifiread_getdata1(t_mifiread *mr);
unsigned mifiread_getdata2(t_mifiread *mr);
unsigned mifiread_getchannel(t_mifiread *mr);
t_pd *mifiread_getowner(t_mifiread *mr);

t_mifiread *mifiread_new(t_pd *owner);
void mifiread_setuserticks(t_mifiread *mr, double wholeticks);
int mifiread_open(t_mifiread *mr, const char *filename,
		  const char *dirname, int complain);
int mifiread_doit(t_mifiread *mr, t_mifireadhook hook, void *hookdata);
void mifiread_close(t_mifiread *mr);
void mifiread_free(t_mifiread *mr);

t_mifiwrite *mifiwrite_new(t_pd *owner);
void mifiwrite_sethardticks(t_mifiwrite *mw, int beatticks);
void mifiwrite_setuserticks(t_mifiwrite *mw, double wholeticks);
void mifiwrite_setusertempo(t_mifiwrite *mw, double tickspersec);
int mifiwrite_open(t_mifiwrite *mw, const char *filename,
		   const char *dirname, int ntracks, int complain);
int mifiwrite_opentrack(t_mifiwrite *mw, char *trackname, int complain);
int mifiwrite_textevent(t_mifiwrite *mw, double delay,
			unsigned type, char *text);
int mifiwrite_channelevent(t_mifiwrite *mw, double delay, unsigned status,
			   unsigned channel, unsigned data1, unsigned data2);
int mifiwrite_closetrack(t_mifiwrite *mw, double enddelay, int complain);
void mifiwrite_close(t_mifiwrite *mw);
void mifiwrite_free(t_mifiwrite *mw);

#endif
