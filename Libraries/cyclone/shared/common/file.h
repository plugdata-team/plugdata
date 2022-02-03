
/* Copyright (c) 2004-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

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

#ifndef __FILE_H__
#define __FILE_H__

EXTERN_STRUCT _file;
#define t_file  struct _file

typedef void (*t_filefn)(t_pd *, t_symbol *, int, t_atom *);
typedef void (*t_embedfn)(t_pd *, t_binbuf *, t_symbol *);

void editor_open(t_file *f, char *title, char *owner);
void editor_close(t_file *f, int ask);
void editor_append(t_file *f, char *contents);
void editor_setdirty(t_file *f, int flag);
void panel_open(t_file *f, t_symbol *inidir);
void panel_setopendir(t_file *f, t_symbol *dir);
t_symbol *panel_getopendir(t_file *f);
void panel_save(t_file *f, t_symbol *inidir, t_symbol *inifile);
void panel_setsavedir(t_file *f, t_symbol *dir);
t_symbol *panel_getsavedir(t_file *f);
int file_ismapped(t_file *f);
int file_isloading(t_file *f);
int file_ispasting(t_file *f);
void file_free(t_file *f);
t_file *file_new(t_pd *master, t_embedfn embedfn, t_filefn readfn, t_filefn writefn,
    t_filefn updatefn);
void file_setup(t_class *c, int embeddable);

#endif
