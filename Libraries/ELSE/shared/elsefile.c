// The 'file' proxy encapsulates openpanel/savepanel management (stolen from cyclone)

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#endif

#include "m_pd.h"
#include "g_canvas.h"
#include "elsefile.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int elsefile_ospath_doabsolute(char *path, char *cwd, char *result){
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
int elsefile_ospath_length(char *path, char *cwd){ // one extra byte used internally (guarding slash)
    return(elsefile_ospath_doabsolute(path, cwd, 0) + 1);
}

/* Copies an absolute path to result.  Arguments: path and cwd, are the same
 as in ospath_length().  Caller should first consult ospath_length(), and
 allocate at least ospath_length() + 1 bytes to the result buffer.
 Should never fail (failure is a bug). */
char *elsefile_ospath_absolute(char *path, char *cwd, char *result){
    elsefile_ospath_doabsolute(path, cwd, result);
    return(result);
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
t_osdir *elsefile_osdir_open(char *dirname){
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

void elsefile_osdir_setmode(t_osdir *dp, int flags){
    if(dp)
        dp->dir_flags = flags;
}

void elsefile_osdir_close(t_osdir *dp){
    if(dp){
#ifndef _WIN32
        closedir(dp->dir_handle);
#endif
        freebytes(dp, sizeof(*dp));
    }
}

void elsefile_osdir_rewind(t_osdir *dp){
    if(dp){
#ifndef _WIN32
        rewinddir(dp->dir_handle);
        dp->dir_entry = 0;
#endif
    }
}

char *elsefile_osdir_next(t_osdir *dp){
#ifndef _WIN32
    if(dp){
        while((dp->dir_entry = readdir(dp->dir_handle))){
            if(!dp->dir_flags || (dp->dir_entry->d_type == DT_REG
            && (dp->dir_flags & OSDIR_elsefileMODE))
            || (dp->dir_entry->d_type == DT_DIR
            && (dp->dir_flags & OSDIR_DIRMODE)))
                return(dp->dir_entry->d_name);
        }
    }
#endif
    return(0);
}

int elsefile_osdir_isfile(t_osdir *dp){
#ifndef _WIN32
    return(dp && dp->dir_entry && dp->dir_entry->d_type == DT_REG);
#else
    return(0);
#endif
}

int elsefile_osdir_isdir(t_osdir *dp){
#ifndef _WIN32
    return(dp && dp->dir_entry && dp->dir_entry->d_type == DT_DIR);
#else
    return(0);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct _elsefile{
    t_pd                 f_pd;
    t_pd                *f_master;
    t_canvas            *f_canvas;
    t_symbol            *f_bindname;
    t_symbol            *f_currentdir;
    t_symbol            *f_inidir;
    t_symbol            *f_inifile;
    t_elsefilefn             f_panelfn;
    t_binbuf            *f_binbuf;
    t_clock             *f_panelclock;
    struct _elsefile        *f_savepanel;
    struct _elsefile        *f_next;
};

static t_class *elsefile_class = 0;
static t_elsefile *elsefile_proxies;
static t_symbol *ps__C;

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
static void elsefile_panel_symbol(t_elsefile *f, t_symbol *s){
    if(s && s != &s_ && f->f_panelfn)
        (*f->f_panelfn)(f->f_master, s, 0, 0);
}

static void elsefile_panel_path(t_elsefile *f, t_symbol *s1, t_symbol *s2){
    if(s2 && s2 != &s_)
        f->f_currentdir = s2;
    if(s1 && s1 != &s_ && f->f_panelfn)
        (*f->f_panelfn)(f->f_master, s1, 0, 0);
}

static void elsefile_panel_tick(t_elsefile *f){
    if(f->f_savepanel)
        sys_vgui("panel_open %s {%s}\n", f->f_bindname->s_name, f->f_inidir->s_name);
    else
        sys_vgui("panel_save %s {%s} {%s}\n", f->f_bindname->s_name, f->f_inidir->s_name, f->f_inifile->s_name);
}

/* these are hacks: deferring modal dialog creation in order to allow for
   a message box redraw to happen -- LATER investigate */
void elsefile_panel_click_open(t_elsefile *f){
    f->f_inidir = (f->f_currentdir ? f->f_currentdir : &s_);
    clock_delay(f->f_panelclock, 0);
}

void elsefile_panel_setopendir(t_elsefile *f, t_symbol *dir){
    if(f->f_currentdir && f->f_currentdir != &s_){
        if(dir && dir != &s_){
            int length = elsefile_ospath_length((char *)(dir->s_name), (char *)(f->f_currentdir->s_name));
            if(length){
                char *path = getbytes(length + 1);
                if(elsefile_ospath_absolute((char *)(dir->s_name), (char *)(f->f_currentdir->s_name), path))
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

t_symbol *elsefile_panel_getopendir(t_elsefile *f){
    return(f->f_currentdir);
}

void elsefile_panel_save(t_elsefile *f, t_symbol *inidir, t_symbol *inifile){
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

void elsefile_panel_setsavedir(t_elsefile *f, t_symbol *dir){
    if((f = f->f_savepanel))
        elsefile_panel_setopendir(f, dir);
}

t_symbol *elsefile_panel_getsavedir(t_elsefile *f){
    return(f->f_savepanel ? f->f_savepanel->f_currentdir : 0);
}

int elsefile_ismapped(t_elsefile *f){
    return(f->f_canvas->gl_mapped);
}

int elsefile_isloading(t_elsefile *f){
    return(f->f_canvas->gl_loading);
}

/* LATER find a better way */
int elsefile_ispasting(t_elsefile *f){
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

void elsefile_free(t_elsefile *f){
    t_elsefile *prev, *next;
    if(f->f_savepanel){
        pd_unbind((t_pd *)f->f_savepanel, f->f_savepanel->f_bindname);
        pd_free((t_pd *)f->f_savepanel);
    }
    if(f->f_bindname)
        pd_unbind((t_pd *)f, f->f_bindname);
    if(f->f_panelclock)
        clock_free(f->f_panelclock);
    for(prev = 0, next = elsefile_proxies; next; prev = next, next = next->f_next)
        if(next == f)
            break;
    if(prev)
        prev->f_next = f->f_next;
    else if(f == elsefile_proxies)
        elsefile_proxies = f->f_next;
    pd_free((t_pd *)f);
}

t_elsefile *elsefile_new(t_pd *master, t_elsefilefn readfn, t_elsefilefn writefn){
    t_elsefile *result = (t_elsefile *)pd_new(elsefile_class);
    result->f_master = master;
    result->f_next = elsefile_proxies;
    elsefile_proxies = result;
    if(!(result->f_canvas = canvas_getcurrent())){
        bug("elsefile_new: out of context");
        return(result);
    }
    // the panels
    if(readfn || writefn){
        t_elsefile *f;
        char buf[64];
        sprintf(buf, "miXed.%lx", (unsigned long)result);
        result->f_bindname = gensym(buf);
        pd_bind((t_pd *)result, result->f_bindname);
        result->f_currentdir = result->f_inidir = canvas_getdir(result->f_canvas);
        result->f_panelfn = readfn;
        result->f_panelclock = clock_new(result, (t_method)elsefile_panel_tick);
        f = (t_elsefile *)pd_new(elsefile_class);
        f->f_master = master;
        f->f_canvas = result->f_canvas;
        sprintf(buf, "miXed.%lx", (unsigned long)f);
        f->f_bindname = gensym(buf);
        pd_bind((t_pd *)f, f->f_bindname);
        f->f_currentdir = f->f_inidir = result->f_currentdir;
        f->f_panelfn = writefn;
        f->f_panelclock = clock_new(f, (t_method)elsefile_panel_tick);
        result->f_savepanel = f;
    }
    else
        result->f_savepanel = 0;
    return(result);
}

void elsefile_setup(void){
    if(!elsefile_class){
        ps__C = gensym("#C");
        elsefile_class = class_new(gensym("_elsefile"), 0, 0,sizeof(t_elsefile), CLASS_PD | CLASS_NOINLET, 0);
        class_addsymbol(elsefile_class, elsefile_panel_symbol);
        class_addmethod(elsefile_class, (t_method)elsefile_panel_path,gensym("path"), A_SYMBOL, A_DEFSYM, 0);
        panel_guidefs();
    }
}
