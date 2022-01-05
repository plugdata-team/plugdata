
/* Copyright (c) 2004-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// #include "shared.h"

#ifndef __OS_H__
#define __OS_H__

EXTERN_STRUCT _osdir;
#define t_osdir  struct _osdir

#define OSDIR_FILEMODE  1
#define OSDIR_DIRMODE   2

int ospath_length(char *path, char *cwd);
char *ospath_absolute(char *path, char *cwd, char *result);

FILE *fileread_open(char *filename, t_canvas *cv, int textmode);
FILE *filewrite_open(char *filename, t_canvas *cv, int textmode);

t_osdir *osdir_open(char *dirname);
void osdir_setmode(t_osdir *dp, int flags);
void osdir_close(t_osdir *dp);
void osdir_rewind(t_osdir *dp);
char *osdir_next(t_osdir *dp);
int osdir_isfile(t_osdir *dp);
int osdir_isdir(t_osdir *dp);

#endif

#ifndef __HAMMERFILE_H__
#define __HAMMERFILE_H__

EXTERN_STRUCT _hammerfile;
#define t_hammerfile  struct _hammerfile

typedef void (*t_hammerfilefn)(t_pd *, t_symbol *, int, t_atom *);
typedef void (*t_hammerembedfn)(t_pd *, t_binbuf *, t_symbol *);

void hammereditor_open(t_hammerfile *f, char *title, char *owner);
void hammereditor_close(t_hammerfile *f, int ask);
void hammereditor_append(t_hammerfile *f, char *contents);
void hammereditor_setdirty(t_hammerfile *f, int flag);
void hammerpanel_open(t_hammerfile *f, t_symbol *inidir);
void hammerpanel_setopendir(t_hammerfile *f, t_symbol *dir);
t_symbol *hammerpanel_getopendir(t_hammerfile *f);
void hammerpanel_save(t_hammerfile *f, t_symbol *inidir, t_symbol *inifile);
void hammerpanel_setsavedir(t_hammerfile *f, t_symbol *dir);
t_symbol *hammerpanel_getsavedir(t_hammerfile *f);
int hammerfile_ismapped(t_hammerfile *f);
int hammerfile_isloading(t_hammerfile *f);
int hammerfile_ispasting(t_hammerfile *f);
void hammerfile_free(t_hammerfile *f);
t_hammerfile *hammerfile_new(t_pd *master, t_hammerembedfn embedfn,
			     t_hammerfilefn readfn, t_hammerfilefn writefn,
			     t_hammerfilefn updatefn);
void hammerfile_setup(t_class *c, int embeddable);

#endif
