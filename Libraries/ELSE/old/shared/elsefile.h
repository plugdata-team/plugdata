
/* Copyright (c) 2004-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __OS_H__
#define __OS_H__

EXTERN_STRUCT _osdir;
#define t_osdir  struct _osdir

#define OSDIR_elsefileMODE  1
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

#ifndef __ELSEFILE_H__
#define __ELSEFILE_H__

EXTERN_STRUCT _elsefile;
#define t_elsefile  struct _elsefile

typedef void (*t_elsefilefn)(t_pd *, t_symbol *, int, t_atom *);

void panel_open(t_elsefile *f, t_symbol *inidir);
void panel_setopendir(t_elsefile *f, t_symbol *dir);
t_symbol *panel_getopendir(t_elsefile *f);
void panel_save(t_elsefile *f, t_symbol *inidir, t_symbol *inifile);
void panel_setsavedir(t_elsefile *f, t_symbol *dir);
t_symbol *panel_getsavedir(t_elsefile *f);
int elsefile_ismapped(t_elsefile *f);
int elsefile_isloading(t_elsefile *f);
int elsefile_ispasting(t_elsefile *f);
void elsefile_free(t_elsefile *f);
t_elsefile *elsefile_new(t_pd *master, t_elsefilefn readfn, t_elsefilefn writefn);
void elsefile_setup(void);

#endif
