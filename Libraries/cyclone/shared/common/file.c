/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* The three uses of the 'file' proxy class are:
   1. providing `embedding' facility -- storing master object's state
   in a .pd file,
   2. encapsulating openpanel/savepanel management,
   3. extending the gui of Pd with a simple text editor window.

   A master class which needs embedding feature (like coll), passes
   a nonzero flag to the file setup routine, and a nonzero embedfn
   function pointer to the file constructor.  If a master needs
   access to the panels (like collcommon), then it passes nonzero readfn
   and/or writefn callback pointers to the constructor.  A master which has
   an associated text editor, AND wants to update object's state after
   edits, passes a nonzero updatefn callback in a call to the constructor.

   LATER extract the embedding stuff. */


#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "common/file.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static int ospath_doabsolute(char *path, char *cwd, char *result){
    if(*path == 0){
        if(result)
            strcpy(result, cwd);
        else
            return
            (strlen(cwd));
    }
    else if(*path == '~'){
        path++;
        if(*path == '/' || *path == 0){
#ifndef _WIN32
            char *home = getenv("HOME");
            if(home){
                if(result){
                    strcpy(result, home);
                    if(*path)
                        strcat(result, path);
                }
                else return(strlen(home) + strlen(path));
            }
            else goto badpath;
#else
            goto badpath;
#endif
        }
        else
            goto badpath;
    }
    else if(*path == '/'){
#ifdef _WIN32
        /* path is absolute, drive is implicit, LATER UNC? */
        if(*cwd && cwd[1] == ':'){
            if(result){
                *result = *cwd;
                result[1] = ':';
                strcpy(result + 2, path);
            }
            else
                return(2 + strlen(path));
        }
        else
            goto badpath;
#else
        /* path is absolute */
        if(result)
            strcpy(result, path);
        else
            return(strlen(path));
#endif
    }
    else{
#ifdef _WIN32
        if(path[1] == ':'){
            if(path[2] == '/'){
                /* path is absolute */
                if(result)
                    strcpy(result, path);
                else
                    return(strlen(path));
            }
            else if(*cwd == *path){
                /* path is relative, drive is explicitly current */
                if(result){
                    int ndx = strlen(cwd);
                    strcpy(result, cwd);
                    result[ndx++] = '/';
                    strcpy(result + ndx, path + 2);
                }
                else
                    return(strlen(cwd) + strlen(path) - 1);
            }
            /* we do not maintain per-drive cwd, LATER rethink */
            else
                goto badpath;
        }
        /* LATER devices? */
        else{
            /* path is relative */
            if(result){
                int ndx = strlen(cwd);
                strcpy(result, cwd);
                result[ndx++] = '/';
                strcpy(result + ndx, path);
            }
            else
                return(strlen(cwd) + 1 + strlen(path));
        }
#else
        /* path is relative */
        if(result){
            int ndx = strlen(cwd);
            strcpy(result, cwd);
            result[ndx++] = '/';
            strcpy(result + ndx, path);
        }
        else
            return(strlen(cwd) + 1 + strlen(path));
#endif
    }
    if(result && *result && *result != '.'){
        /* clean-up */
        char *inptr, *outptr = result;
        int ndx = strlen(result);
        if(result[ndx - 1] == '.'){
            result[ndx] = '/';  /* guarding slash */
            result[ndx + 1] = 0;
        }
        for(inptr = result + 1; *inptr; inptr++){
            if(*inptr == '/'){
                if(*outptr == '/')
                    continue;
                else if(*outptr == '.'){
                    if(outptr[-1] == '/'){
                        outptr--;
                        continue;
                    }
                    else if(outptr[-1] == '.' && outptr[-2] == '/'){
                        outptr -= 2;
                        if(outptr == result)
                            continue;
                        else for(outptr--; outptr != result; outptr--)
                            if(*outptr == '/')
                                break;
                        continue;
                    }
                }
            }
            *++outptr = *inptr;
        }
        if(*outptr == '/' && outptr != result)
            *outptr = 0;
        else
            outptr[1] = 0;
    }
    else
        bug("ospath_doabsolute 1");
    return(0);
badpath:
    if(result)
        bug("ospath_doabsolute 2");
    return(0);
}

/* Returns an estimated length of an absolute path made up from the first arg.
 The actual ospath_absolute()'s length may be shorter (since it erases
 superfluous slashes and dots), but not longer.  Both args should be unbashed
 (system-independent), cwd should be absolute.  Returns 0 in case of any
 error (LATER revisit). */
int ospath_length(char *path, char *cwd){
    /* one extra byte used internally (guarding slash) */
    return(ospath_doabsolute(path, cwd, 0) + 1);
}

/* Copies an absolute path to result.  Arguments: path and cwd, are the same
 as in ospath_length().  Caller should first consult ospath_length(), and
 allocate at least ospath_length() + 1 bytes to the result buffer.
 Should never fail (failure is a bug). */
char *ospath_absolute(char *path, char *cwd, char *result){
    ospath_doabsolute(path, cwd, result);
    return(result);
}

FILE *fileread_open(char *filename, t_canvas *cv, int textmode){
    char path[MAXPDSTRING+2], *nameptr;
    t_symbol *dirsym = (cv ? canvas_getdir(cv) : 0);
    /* path arg is returned unbashed (system-independent) */
    int fd = open_via_path((dirsym ? dirsym->s_name : ""), filename, "", path, &nameptr, MAXPDSTRING, 1);
    if(fd < 0)
        return(0);
    /* Closing/reopening dance.  This is unnecessary under linux, and we
     could have tried to convert fd to fp, but under windows open_via_path()
     returns what seems to be an invalid fd.
     LATER try to understand what is going on here... */
    close(fd);
    if(path != nameptr){
        char *slashpos = path + strlen(path);
        *slashpos++ = '/';
        /* try not to be dependent on current open_via_path() implementation */
        if(nameptr != slashpos)
            strcpy(slashpos, nameptr);
    }
    return(sys_fopen(path, (textmode ? "r" : "rb")));
}

FILE *filewrite_open(char *filename, t_canvas *cv, int textmode){
    char path[MAXPDSTRING+2];
    if(cv)
    /* path arg is returned unbashed (system-independent) */
        canvas_makefilename(cv, filename, path, MAXPDSTRING);
    else{
        strncpy(path, filename, MAXPDSTRING);
        path[MAXPDSTRING-1] = 0;
    }
    return(sys_fopen(path, (textmode ? "w" : "wb")));
}

/* FIXME add MSW */

struct _osdir{
#ifndef _WIN32
    DIR            *dir_handle;
    struct dirent  *dir_entry;
#endif
    int             dir_flags;
};

/* returns 0 on error, a caller is then expected to call
 loud_syserror(owner, "cannot open \"%s\"", dirname) */
t_osdir *osdir_open(char *dirname){
#ifndef _WIN32
    DIR *handle = opendir(dirname);
    if(handle){
#endif
        t_osdir *dp = getbytes(sizeof(*dp));
#ifndef _WIN32
        dp->dir_handle = handle;
        dp->dir_entry = 0;
#endif
        dp->dir_flags = 0;
        return(dp);
#ifndef _WIN32
    }
    else
        return(0);
#endif
}

void osdir_setmode(t_osdir *dp, int flags){
    if(dp)
        dp->dir_flags = flags;
}

void osdir_close(t_osdir *dp){
    if(dp){
#ifndef _WIN32
        closedir(dp->dir_handle);
#endif
        freebytes(dp, sizeof(*dp));
    }
}

void osdir_rewind(t_osdir *dp){
    if(dp){
#ifndef _WIN32
        rewinddir(dp->dir_handle);
        dp->dir_entry = 0;
#endif
    }
}

char *osdir_next(t_osdir *dp){
#ifndef _WIN32
    if(dp){
        while((dp->dir_entry = readdir(dp->dir_handle))){
            if(!dp->dir_flags || (dp->dir_entry->d_type == DT_REG
            && (dp->dir_flags & OSDIR_FILEMODE))
            || (dp->dir_entry->d_type == DT_DIR
            && (dp->dir_flags & OSDIR_DIRMODE)))
                return(dp->dir_entry->d_name);
        }
    }
#endif
    return(0);
}

int osdir_isfile(t_osdir *dp){
#ifndef _WIN32
    return(dp && dp->dir_entry && dp->dir_entry->d_type == DT_REG);
#else
    return(0);
#endif
}

int osdir_isdir(t_osdir *dp){
#ifndef _WIN32
    return(dp && dp->dir_entry && dp->dir_entry->d_type == DT_DIR);
#else
    return(0);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct _file{
    t_pd                 f_pd;
    t_pd                *f_master;
    t_canvas            *f_canvas;
    t_symbol            *f_bindname;
    t_symbol            *f_currentdir;
    t_symbol            *f_inidir;
    t_symbol            *f_inifile;
    t_filefn       f_panelfn;
    t_filefn       f_editorfn;
    t_embedfn      f_embedfn;
    t_binbuf            *f_binbuf;
    t_clock             *f_panelclock;
    t_clock             *f_editorclock;
    struct _file  *f_savepanel;
    struct _file  *f_next;
};

static t_class *file_class = 0;
static t_file *file_proxies;
static t_symbol *ps__C;

static t_file *file_getproxy(t_pd *master){
    t_file *f;
    for(f = file_proxies; f; f = f->f_next)
        if(f->f_master == master)
            return(f);
    return(0);
}

static void editor_guidefs(void){
    sys_gui("proc editor_open {name geometry title sendable} {\n");
    sys_gui(" if {[winfo exists $name]} {\n");
    sys_gui("  $name.text delete 1.0 end\n");
    sys_gui(" } else {\n");
    sys_gui("  toplevel $name\n");
    sys_gui("  wm title $name $title\n");
    sys_gui("  wm geometry $name $geometry\n");
    sys_gui("  if {$sendable} {\n");
    sys_gui("   wm protocol $name WM_DELETE_WINDOW \\\n");
    sys_gui("    [concat editor_close $name 1]\n");
    sys_gui("   bind $name <<Modified>> \"editor_dodirty $name\"\n");
    sys_gui("  }\n");
    sys_gui("  text $name.text -relief raised -bd 2 \\\n");
    sys_gui("   -font -*-courier-medium--normal--12-* \\\n");
    sys_gui("   -yscrollcommand \"$name.scroll set\" -background lightgrey\n");
    sys_gui("  scrollbar $name.scroll -command \"$name.text yview\"\n");
    sys_gui("  pack $name.scroll -side right -fill y\n");
    sys_gui("  pack $name.text -side left -fill both -expand 1\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    sys_gui("proc editor_dodirty {name} {\n");
    sys_gui(" if {[catch {$name.text edit modified} dirty]} {set dirty 1}\n");
    sys_gui(" set title [wm title $name]\n");
    sys_gui(" set dt [string equal -length 1 $title \"*\"]\n");
    sys_gui(" if {$dirty} {\n");
    sys_gui("  if {$dt == 0} {wm title $name *$title}\n");
    sys_gui(" } else {\n");
    sys_gui("  if {$dt} {wm title $name [string range $title 1 end]}\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    sys_gui("proc editor_setdirty {name flag} {\n");
    sys_gui(" if {[winfo exists $name]} {\n");
    sys_gui("  catch {$name.text edit modified $flag}\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    sys_gui("proc editor_doclose {name} {\n");
    sys_gui(" destroy $name\n");
    sys_gui("}\n");

    sys_gui("proc editor_append {name contents} {\n");
    sys_gui(" if {[winfo exists $name]} {\n");
    sys_gui("  $name.text insert end $contents\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    /* FIXME make it more reliable */
    sys_gui("proc editor_send {name} {\n");
    sys_gui(" if {[winfo exists $name]} {\n");
    sys_gui("  pdsend \"miXed$name clear\"\n");
    sys_gui("  for {set i 1} \\\n");
    sys_gui("   {[$name.text compare $i.end < end]} \\\n");
    sys_gui("  	{incr i 1} {\n");
    sys_gui("   set lin [$name.text get $i.0 $i.end]\n");
    sys_gui("   if {$lin != \"\"} {\n");
    /* LATER rethink semi/comma mapping */
    sys_gui("    regsub -all \\; $lin \"  _semi_ \" tmplin\n");
    sys_gui("    regsub -all \\, $tmplin \"  _comma_ \" lin\n");
    sys_gui("    pdsend \"miXed$name addline $lin\"\n");
    sys_gui("   }\n");
    sys_gui("  }\n");
    sys_gui("  pdsend \"miXed$name end\"\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    sys_gui("proc editor_close {name ask} {\n");
    sys_gui(" if {[winfo exists $name]} {\n");
    sys_gui("  if {[catch {$name.text edit modified} dirty]} {set dirty 1}\n");
    sys_gui("  if {$ask && $dirty} {\n");
    sys_gui("   set title [wm title $name]\n");
    sys_gui("   if {[string equal -length 1 $title \"*\"]} {\n");
    sys_gui("    set title [string range $title 1 end]\n");
    sys_gui("   }\n");
    sys_gui("   set answer [tk_messageBox \\-type yesnocancel \\\n");
    sys_gui("    \\-icon question \\\n");
    sys_gui("    \\-message [concat Save changes to \\\"$title\\\"?]]\n");
    sys_gui("   if {$answer == \"yes\"} {editor_send $name}\n");
    sys_gui("   if {$answer != \"cancel\"} {editor_doclose $name}\n");
    sys_gui("  } else {editor_doclose $name}\n");
    sys_gui(" }\n");
    sys_gui("}\n");
}

/* null owner defaults to class name, pass "" to suppress */
void editor_open(t_file *f, char *title, char *owner){
    if(!owner)
        owner = (char *)(class_getname(*f->f_master));
    if(!*owner)
        owner = 0;
    if(!title){
        title = owner;
        owner = 0;
    }
    if(owner)
        sys_vgui("editor_open .%lx %dx%d {%s: %s} %d\n",
        (unsigned long)f, 600, 340, owner, title, (f->f_editorfn != 0));
    else
        sys_vgui("editor_open .%lx %dx%d {%s} %d\n",
        (unsigned long)f, 600, 340, (title ? title : "Untitled"),
        (f->f_editorfn != 0));
}

static void editor_tick(t_file *f){
    sys_vgui("editor_close .%lx 1\n", (unsigned long)f);
}

void editor_close(t_file *f, int ask){
    if(ask && f->f_editorfn)
	/* hack: deferring modal dialog creation in order to allow for
	   a message box redraw to happen -- LATER investigate */
        clock_delay(f->f_editorclock, 0);
    else
        sys_vgui("editor_close .%lx 0\n", (unsigned long)f);
}

void editor_append(t_file *f, char *contents){
    if(contents){
        char *ptr;
        for(ptr = contents; *ptr; ptr++){
            if(*ptr == '{' || *ptr == '}'){
                char c = *ptr;
                *ptr = 0;
                sys_vgui("editor_append .%lx {%s}\n", (unsigned long)f, contents);
                sys_vgui("editor_append .%lx \"%c\"\n", (unsigned long)f, c);
                *ptr = c;
                contents = ptr + 1;
            }
        }
        if(*contents)
            sys_vgui("editor_append .%lx {%s}\n", (unsigned long)f, contents);
    }
}

void editor_setdirty(t_file *f, int flag){
    if(f->f_editorfn)
        sys_vgui("editor_setdirty .%lx %d\n", (unsigned long)f, flag);
}

static void editor_clear(t_file *f){
    if(f->f_editorfn){
        if(f->f_binbuf)
            binbuf_clear(f->f_binbuf);
        else
            f->f_binbuf = binbuf_new();
    }
}

static void editor_addline(t_file *f, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(f->f_editorfn){
        int i;
        t_atom *ap;
        for(i = 0, ap = av; i < ac; i++, ap++){
            if(ap->a_type == A_SYMBOL){
                /* LATER rethink semi/comma mapping */
                if(!strcmp(ap->a_w.w_symbol->s_name, "_semi_"))
                    SETSEMI(ap);
                else if(!strcmp(ap->a_w.w_symbol->s_name, "_comma_"))
                    SETCOMMA(ap);
            }
        }
        binbuf_add(f->f_binbuf, ac, av);
    }
}

static void editor_end(t_file *f){
    if(f->f_editorfn){
        (*f->f_editorfn)(f->f_master, 0, binbuf_getnatom(f->f_binbuf), binbuf_getvec(f->f_binbuf));
	binbuf_clear(f->f_binbuf);
    }
}

static void panel_guidefs(void){
    sys_gui("proc panel_open {target inidir} {\n");
    sys_gui(" global pd_opendir\n");
    sys_gui(" if {$inidir == \"\"} {\n");
    sys_gui("  set $inidir $pd_opendir\n");
    sys_gui(" }\n");
    sys_gui(" set filename [tk_getOpenFile \\\n");
    sys_gui("  -initialdir $inidir]\n");
    sys_gui(" if {$filename != \"\"} {\n");
    sys_gui("  set directory [string range $filename 0 \\\n");
    sys_gui("   [expr [string last / $filename ] - 1]]\n");
    sys_gui("  if {$directory == \"\"} {set directory \"/\"}\n");
    sys_gui("  puts stderr [concat $directory]\n");
    sys_gui("  pdsend \"$target path \\\n");
    sys_gui("   [enquote_path $filename] [enquote_path $directory] \"\n");
    sys_gui(" }\n");
    sys_gui("}\n");

    sys_gui("proc panel_save {target inidir inifile} {\n");
    sys_gui(" if {$inifile != \"\"} {\n");
    sys_gui("  set filename [tk_getSaveFile \\\n");
    sys_gui("   -initialdir $inidir -initialfile $inifile]\n");
    sys_gui(" } else {\n");
    sys_gui("  set filename [tk_getSaveFile]\n");
    sys_gui(" }\n");
    sys_gui(" if {$filename != \"\"} {\n");
    sys_gui("  set directory [string range $filename 0 \\\n");
    sys_gui("   [expr [string last / $filename ] - 1]]\n");
    sys_gui("  if {$directory == \"\"} {set directory \"/\"}\n");
    sys_gui("  pdsend \"$target path \\\n");
    sys_gui("   [enquote_path $filename] [enquote_path $directory] \"\n");
    sys_gui(" }\n");
    sys_gui("}\n");
}

/* There are two modes of -initialdir persistence:
   1. Using last reply from gui (if any, default is canvas directory):
   pass null to panel_open/save() (for explicit cd, optionally call
   panel_setopen/savedir() first).
   2. Starting always in the same directory (eg. canvasdir):
   feed panel_open/save().
   Usually, first mode fits opening better, the second -- saving. */

/* This is obsolete, but has to stay, because older versions of miXed libraries
   might overwrite new panel_guidefs().  FIXME we need version control. */
static void panel_symbol(t_file *f, t_symbol *s){
    if(s && s != &s_ && f->f_panelfn)
        (*f->f_panelfn)(f->f_master, s, 0, 0);
}

static void panel_path(t_file *f, t_symbol *s1, t_symbol *s2){
    if(s2 && s2 != &s_)
        f->f_currentdir = s2;
    if(s1 && s1 != &s_ && f->f_panelfn)
        (*f->f_panelfn)(f->f_master, s1, 0, 0);
}

static void panel_tick(t_file *f){
    if(f->f_savepanel)
        sys_vgui("panel_open %s {%s}\n", f->f_bindname->s_name, f->f_inidir->s_name);
    else
        sys_vgui("panel_save %s {%s} {%s}\n", f->f_bindname->s_name,
        f->f_inidir->s_name, f->f_inifile->s_name);
}

/* these are hacks: deferring modal dialog creation in order to allow for
   a message box redraw to happen -- LATER investigate */
void panel_open(t_file *f, t_symbol *inidir){
    if(inidir)
        f->f_inidir = inidir;
    else
        f->f_inidir = (f->f_currentdir ? f->f_currentdir : &s_);
    clock_delay(f->f_panelclock, 0);
}

void panel_setopendir(t_file *f, t_symbol *dir){
    if(f->f_currentdir && f->f_currentdir != &s_){
        if(dir && dir != &s_){
            int length = ospath_length((char *)(dir->s_name), (char *)(f->f_currentdir->s_name));
            if(length){
                char *path = getbytes(length + 1);
                if(ospath_absolute((char *)(dir->s_name), (char *)(f->f_currentdir->s_name), path))
                /* LATER stat (think how to report a failure) */
                    f->f_currentdir = gensym(path);
                freebytes(path, length + 1);
            }
        }
        else if(f->f_canvas)
            f->f_currentdir = canvas_getdir(f->f_canvas);
    }
    else
        bug("panel_setopendir");
}

t_symbol *panel_getopendir(t_file *f){
    return(f->f_currentdir);
}

void panel_save(t_file *f, t_symbol *inidir, t_symbol *inifile){
    if((f = f->f_savepanel)){
        if(inidir)
            f->f_inidir = inidir;
        else
            /* LATER ask if we can rely on s_ pointing to "" */
            f->f_inidir = (f->f_currentdir ? f->f_currentdir : &s_);
        f->f_inifile = (inifile ? inifile : &s_);
        clock_delay(f->f_panelclock, 0);
    }
}

void panel_setsavedir(t_file *f, t_symbol *dir){
    if((f = f->f_savepanel))
        panel_setopendir(f, dir);
}

t_symbol *panel_getsavedir(t_file *f){
    return(f->f_savepanel ? f->f_savepanel->f_currentdir : 0);
}

/* Currently embeddable  classes do not use the 'saveto' method.
   In order to use it, any embeddable class would have to add a creation
   method to pd_canvasmaker -- then saving could be done with a 'proper'
   sequence:  #N <master> <args>; #X <whatever>; ...; #X restore <x> <y>;
   However, this works only for -lib externals.  So, we choose a sequence:
   #X obj <x> <y> <master> <args>; #C <whatever>; ...; #C restore;
   Since the first message in this sequence is a valid creation message
   on its own, we have to distinguish loading from a .pd file, and other
   cases (editing). */

static void embed_gc(t_pd *x, t_symbol *s, int expected){
    t_pd *garbage;
    int count = 0;
    while((garbage = pd_findbyclass(s, *x)))
        pd_unbind(garbage, s), count++;
    if(count != expected)
	bug("embed_gc (%d garbage bindings)", count);
}

static void embed_restore(t_pd *master){
    embed_gc(master, ps__C, 1);
}

void embed_save(t_gobj *master, t_binbuf *bb){
    t_file *f = file_getproxy((t_pd *)master);
    t_text *t = (t_text *)master;
    binbuf_addv(bb, "ssii", &s__X, gensym("obj"), (int)t->te_xpix, (int)t->te_ypix);
    binbuf_addbinbuf(bb, t->te_binbuf);
    binbuf_addsemi(bb);
    if(f && f->f_embedfn)
        (*f->f_embedfn)(f->f_master, bb, ps__C);
    binbuf_addv(bb, "ss;", ps__C, gensym("restore"));
}

int file_ismapped(t_file *f){
    return(f->f_canvas->gl_mapped);
}

int file_isloading(t_file *f){
    return(f->f_canvas->gl_loading);
}

/* LATER find a better way */
int file_ispasting(t_file *f){
    int result = 0;
    t_canvas *cv = f->f_canvas;
    if(!cv->gl_loading){
        t_pd *z = s__X.s_thing;
        if(z == (t_pd *)cv){
            pd_popsym(z);
            if(s__X.s_thing == (t_pd *)cv) result = 1;
            pd_pushsym(z);
        }
        else if(z)
            result = 1;
    }
    return(result);
}

void file_free(t_file *f){
    t_file *prev, *next;
    editor_close(f, 0);
    if(f->f_embedfn)
        /* just in case of missing 'restore' */
        embed_gc(f->f_master, ps__C, 0);
    if(f->f_savepanel){
        pd_unbind((t_pd *)f->f_savepanel, f->f_savepanel->f_bindname);
        pd_free((t_pd *)f->f_savepanel);
    }
    if(f->f_bindname)
        pd_unbind((t_pd *)f, f->f_bindname);
    if(f->f_panelclock)
        clock_free(f->f_panelclock);
    if(f->f_editorclock)
        clock_free(f->f_editorclock);
    for(prev = 0, next = file_proxies; next; prev = next, next = next->f_next)
        if(next == f)
            break;
    if(prev)
        prev->f_next = f->f_next;
    else if(f == file_proxies)
        file_proxies = f->f_next;
    pd_free((t_pd *)f);
}

t_file *file_new(t_pd *master, t_embedfn embedfn,
t_filefn readfn, t_filefn writefn, t_filefn updatefn){
    t_file *result = (t_file *)pd_new(file_class);
    result->f_master = master;
    result->f_next = file_proxies;
    file_proxies = result;
    if(!(result->f_canvas = canvas_getcurrent())){
        bug("file_new: out of context");
        return(result);
    }
    /* 1. embedding */
    if((result->f_embedfn = embedfn)){
        /* just in case of missing 'restore' */
        embed_gc(master, ps__C, 0);
        if(file_isloading(result) || file_ispasting(result))
            pd_bind(master, ps__C);
    }
    /* 2. the panels */
    if(readfn || writefn){
        t_file *f;
        char buf[64];
        sprintf(buf, "miXed.%lx", (unsigned long)result);
        result->f_bindname = gensym(buf);
        pd_bind((t_pd *)result, result->f_bindname);
        result->f_currentdir = result->f_inidir = canvas_getdir(result->f_canvas);
        result->f_panelfn = readfn;
        result->f_panelclock = clock_new(result, (t_method)panel_tick);
        f = (t_file *)pd_new(file_class);
        f->f_master = master;
        f->f_canvas = result->f_canvas;
        sprintf(buf, "miXed.%lx", (unsigned long)f);
        f->f_bindname = gensym(buf);
        pd_bind((t_pd *)f, f->f_bindname);
        f->f_currentdir = f->f_inidir = result->f_currentdir;
        f->f_panelfn = writefn;
        f->f_panelclock = clock_new(f, (t_method)panel_tick);
        result->f_savepanel = f;
    }
    else
        result->f_savepanel = 0;
    /* 3. editor */
    if((result->f_editorfn = updatefn)){
        result->f_editorclock = clock_new(result, (t_method)editor_tick);
        if(!result->f_bindname){
            char buf[64];
            sprintf(buf, "miXed.%lx", (unsigned long)result);
            result->f_bindname = gensym(buf);
            pd_bind((t_pd *)result, result->f_bindname);
        }
    }
    return(result);
}

void file_setup(t_class *c, int embeddable){
    if(embeddable){
        class_setsavefn(c, embed_save);
        class_addmethod(c, (t_method)embed_restore, gensym("restore"), 0);
    }
    if(!file_class){
        ps__C = gensym("#C");
        file_class = class_new(gensym("_file"), 0, 0,sizeof(t_file), CLASS_PD | CLASS_NOINLET, 0);
        class_addsymbol(file_class, panel_symbol);
        class_addmethod(file_class, (t_method)panel_path,gensym("path"), A_SYMBOL, A_DEFSYM, 0);
        class_addmethod(file_class, (t_method)editor_clear, gensym("clear"), 0);
        class_addmethod(file_class, (t_method)editor_addline, gensym("addline"), A_GIMME, 0);
        class_addmethod(file_class, (t_method)editor_end, gensym("end"), 0);
        /* LATER find a way of ensuring that these are not defined yet... */
        editor_guidefs();
        panel_guidefs();
    }
}
