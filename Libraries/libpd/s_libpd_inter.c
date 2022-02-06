/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Pd side of the Pd/Pd-gui interface.  Also, some system interface routines
that didn't really belong anywhere. */

#include "s_libpd_inter.h"
#include "m_pd.h"
#include "s_stuff.h"
#include "m_imp.h"
#include "g_canvas.h"   /* for GUI queueing stuff */
#include "s_net.h"
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#endif

#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glob.h>
#else
#include <stdlib.h>
#endif

#define stringify(s) str(s)
#define str(s) #s

#define INTER (pd_this->pd_inter)

#define DEBUG_MESSUP 1      /* messages up from pd to pd-gui */
#define DEBUG_MESSDOWN 2    /* messages down from pd-gui to pd */

#ifndef PDBINDIR
#define PDBINDIR "bin/"
#endif

#ifndef PDGUIDIR
#define PDGUIDIR "tcl/"
#endif

#ifndef WISH
# if defined _WIN32
#  define WISH "wish85.exe"
# elif defined __APPLE__
   // leave undefined to use dummy search path, otherwise
   // this should be a full path to wish on mac
#else
#  define WISH "wish"
# endif
#endif

#define LOCALHOST "localhost"

#if PDTHREADS
#include "pthread.h"
#endif

typedef struct _fdpoll
{
    int fdp_fd;
    t_fdpollfn fdp_fn;
    void *fdp_ptr;
} t_fdpoll;

struct _socketreceiver
{
    char *sr_inbuf;
    int sr_inhead;
    int sr_intail;
    void *sr_owner;
    int sr_udp;
    struct sockaddr_storage *sr_fromaddr; /* optional */
    t_socketnotifier sr_notifier;
    t_socketreceivefn sr_socketreceivefn;
    t_socketfromaddrfn sr_fromaddrfn; /* optional */
};

typedef struct _guiqueue
{
    void *gq_client;
    t_glist *gq_glist;
    t_guicallbackfn gq_fn;
    struct _guiqueue *gq_next;
} t_guiqueue;

struct _instanceinter
{
    int i_havegui;
    int i_nfdpoll;
    t_fdpoll *i_fdpoll;
    int i_maxfd;
    int i_guisock;
    t_socketreceiver *i_socketreceiver;
    t_guiqueue *i_guiqueuehead;
    t_binbuf *i_inbinbuf;
    char *i_guibuf;
    int i_guihead;
    int i_guitail;
    int i_guisize;
    int i_waitingforping;
    int i_bytessincelastping;
    int i_fdschanged;   /* flag to break fdpoll loop if fd list changes */

#ifdef _WIN32
    LARGE_INTEGER i_inittime;
    double i_freq;
#endif
#if PDTHREADS
    pthread_mutex_t i_mutex;
#endif

    unsigned char i_recvbuf[NET_MAXPACKETSIZE];
    
    pd_gui_callback gui_callback;
    void* callback_target;
};

void register_gui_trigger(t_pdinstance* instance, void* target, pd_gui_callback callback) {
    
    instance->pd_inter->gui_callback = callback;
    instance->pd_inter->callback_target = target;
}

void update_gui(void* obj_target) {
    if(pd_this->pd_inter->gui_callback) {
        
        pd_this->pd_inter->gui_callback(pd_this->pd_inter->callback_target, obj_target);
    }
}

extern int sys_guisetportnumber;
extern int sys_addhist(int phase);
void sys_set_searchpath(void);
void sys_set_temppath(void);
void sys_set_extrapath(void);
void sys_set_startup(void);
void sys_stopgui(void);

/* ----------- functions for timing, signals, priorities, etc  --------- */

#ifdef _WIN32

static void sys_initntclock(void)
{
    LARGE_INTEGER f1;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (!QueryPerformanceFrequency(&f1))
    {
          fprintf(stderr, "pd: QueryPerformanceFrequency failed\n");
          f1.QuadPart = 1;
    }
    INTER->i_freq = f1.QuadPart;
    INTER->i_inittime = now;
}

#if 0
    /* this is a version you can call if you did the QueryPerformanceCounter
    call yourself.  Necessary for time tagging incoming MIDI at interrupt
    level, for instance; but we're not doing that just now. */

double nt_tixtotime(LARGE_INTEGER *dumbass)
{
    if (INTER->i_freq == 0) sys_initntclock();
    return (((double)(dumbass->QuadPart -
        INTER->i_inittime.QuadPart)) / INTER->i_freq);
}
#endif
#endif /* _WIN32 */

    /* get "real time" in seconds; take the
    first time we get called as a reference time of zero. */
double sys_getrealtime(void)
{
#ifndef _WIN32
    static struct timeval then;
    struct timeval now;
    gettimeofday(&now, 0);
    if (then.tv_sec == 0 && then.tv_usec == 0) then = now;
    return ((now.tv_sec - then.tv_sec) +
        (1./1000000.) * (now.tv_usec - then.tv_usec));
#else
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (INTER->i_freq == 0) sys_initntclock();
    return (((double)(now.QuadPart -
        INTER->i_inittime.QuadPart)) / INTER->i_freq);
#endif
}

extern int sys_nosleep;

/* sleep (but cancel the sleeping if any file descriptors are
ready - in that case, dispatch any resulting Pd messages and return.  Called
with sys_lock() set.  We will temporarily release the lock if we actually
sleep. */
static int sys_domicrosleep(int microsec)
{
    struct timeval timeout;
    int i, didsomething = 0;
    t_fdpoll *fp;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (INTER->i_nfdpoll)
    {
        fd_set readset, writeset, exceptset;
        FD_ZERO(&writeset);
        FD_ZERO(&readset);
        FD_ZERO(&exceptset);
        for (fp = INTER->i_fdpoll,
            i = INTER->i_nfdpoll; i--; fp++)
                FD_SET(fp->fdp_fd, &readset);
        if(select(INTER->i_maxfd+1,
                  &readset, &writeset, &exceptset, &timeout) < 0)
          perror("microsleep select");
        INTER->i_fdschanged = 0;
        for (i = 0; i < INTER->i_nfdpoll &&
            !INTER->i_fdschanged; i++)
                if (FD_ISSET(INTER->i_fdpoll[i].fdp_fd, &readset))
        {
            (*INTER->i_fdpoll[i].fdp_fn)
                (INTER->i_fdpoll[i].fdp_ptr,
                    INTER->i_fdpoll[i].fdp_fd);
            didsomething = 1;
        }
        if (didsomething)
            return (1);
    }
    if (microsec)
    {
        sys_unlock();
#ifdef _WIN32
        Sleep(microsec/1000);
#else
        usleep(microsec);
#endif
        sys_lock();
    }
    return (0);
}

    /* sleep (but if any incoming or to-gui sending to do, do that instead.)
    Call with the PD instance lock UNSET - we set it here. */
void sys_microsleep( void)
{
    sys_lock();
    sys_domicrosleep(sched_get_sleepgrain());
    sys_unlock();
}

#if !defined(_WIN32) && !defined(__CYGWIN__)
static void sys_signal(int signo, sig_t sigfun)
{
    struct sigaction action;
    action.sa_flags = 0;
    action.sa_handler = sigfun;
    memset(&action.sa_mask, 0, sizeof(action.sa_mask));
#if 0  /* GG says: don't use that */
    action.sa_restorer = 0;
#endif
    if (sigaction(signo, &action, 0) < 0)
        perror("sigaction");
}

static void sys_exithandler(int n)
{
    static int trouble = 0;
    if (!trouble)
    {
        trouble = 1;
        fprintf(stderr, "Pd: signal %d\n", n);
        sys_bail(1);
    }
    else _exit(1);
}

static void sys_alarmhandler(int n)
{
    fprintf(stderr, "Pd: system call timed out\n");
}

static void sys_huphandler(int n)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 30000;
    select(1, 0, 0, 0, &timeout);
}

void sys_setalarm(int microsec)
{
    struct itimerval gonzo;
    int sec = (int)(microsec/1000000);
    microsec %= 1000000;
#if 0
    fprintf(stderr, "timer %d:%d\n", sec, microsec);
#endif
    gonzo.it_interval.tv_sec = 0;
    gonzo.it_interval.tv_usec = 0;
    gonzo.it_value.tv_sec = sec;
    gonzo.it_value.tv_usec = microsec;
    if (microsec)
        sys_signal(SIGALRM, sys_alarmhandler);
    else sys_signal(SIGALRM, SIG_IGN);
    setitimer(ITIMER_REAL, &gonzo, 0);
}

#endif /* NOT _WIN32 && NOT __CYGWIN__ */

    /* on startup, set various signal handlers */
void sys_setsignalhandlers(void)
{
#if !defined(_WIN32) && !defined(__CYGWIN__)
    signal(SIGHUP, sys_huphandler);
    signal(SIGINT, sys_exithandler);
    signal(SIGQUIT, sys_exithandler);
# ifdef SIGIOT
    signal(SIGIOT, sys_exithandler);
# endif
    signal(SIGFPE, SIG_IGN);
    /* signal(SIGILL, sys_exithandler);
    signal(SIGBUS, sys_exithandler);
    signal(SIGSEGV, sys_exithandler); */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
#if 0  /* GG says: don't use that */
    signal(SIGSTKFLT, sys_exithandler);
#endif
#endif /* NOT _WIN32 && NOT __CYGWIN__ */
}

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__)

#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(_POSIX_MEMLOCK)
#include <sched.h>
#endif

#define MODE_NRT 0
#define MODE_RT 1
#define MODE_WATCHDOG 2

void sys_set_priority(int mode)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
    struct sched_param par;
    int p1, p2, p3;
    p1 = sched_get_priority_min(SCHED_FIFO);
    p2 = sched_get_priority_max(SCHED_FIFO);
#ifdef USEAPI_JACK
    p3 = (mode == MODE_WATCHDOG ? p1 + 7 : (mode == MODE_RT ? p1 + 5 : 0));
#else
    p3 = (mode == MODE_WATCHDOG ? p2 - 5 : (mode == MODE_RT ? p2 - 7 : 0));
#endif
    par.sched_priority = p3;
    if (sched_setscheduler(0,
        (mode == MODE_NRT ? SCHED_OTHER : SCHED_FIFO), &par) < 0)
    {
        if (mode == MODE_WATCHDOG)
            fprintf(stderr, "priority %d scheduling failed.\n", p3);
        else post("priority %d scheduling failed; running at normal priority",
                p3);
    }
    else
    {
        if (mode == MODE_RT)
            logpost(NULL, PD_VERBOSE, "priority %d scheduling enabled.\n", p3);
        else logpost(NULL, PD_VERBOSE, "running at normal (non-real-time) priority.\n");
    }
#endif

#if !defined(USEAPI_JACK)
    if (mode != MODE_NRT)
    {
            /* tb: force memlock to physical memory { */
        struct rlimit mlock_limit;
        mlock_limit.rlim_cur=0;
        mlock_limit.rlim_max=0;
        setrlimit(RLIMIT_MEMLOCK,&mlock_limit);
            /* } tb */
        if (mlockall(MCL_FUTURE) != -1 && sys_verbose)
            fprintf(stderr, "memory locking enabled.\n");
    }
    else munlockall();
#endif
}

#endif /* __linux__ */

/* ------------------ receiving incoming messages over sockets ------------- */

unsigned char *sys_getrecvbuf(unsigned int *size)
{
    if (size)
        *size = NET_MAXPACKETSIZE;
    return INTER->i_recvbuf;
}

void sys_sockerror(const char *s)
{
    char buf[MAXPDSTRING];
    int err = socket_errno();
    socket_strerror(err, buf, sizeof(buf));
    pd_error(0, "%s: %s (%d)", s, buf, err);
}

void sys_addpollfn(int fd, t_fdpollfn fn, void *ptr)
{
    int nfd, size;
    t_fdpoll *fp;
    sys_init_fdpoll();
    nfd = INTER->i_nfdpoll;
    size = nfd * sizeof(t_fdpoll);
    INTER->i_fdpoll = (t_fdpoll *)t_resizebytes(
        INTER->i_fdpoll, size, size + sizeof(t_fdpoll));
    fp = INTER->i_fdpoll + nfd;
    fp->fdp_fd = fd;
    fp->fdp_fn = fn;
    fp->fdp_ptr = ptr;
    INTER->i_nfdpoll = nfd + 1;
    if (fd >= INTER->i_maxfd)
        INTER->i_maxfd = fd + 1;
    INTER->i_fdschanged = 1;
}

void sys_rmpollfn(int fd)
{
    int nfd = INTER->i_nfdpoll;
    int i, size = nfd * sizeof(t_fdpoll);
    t_fdpoll *fp;
    INTER->i_fdschanged = 1;
    for (i = nfd, fp = INTER->i_fdpoll; i--; fp++)
    {
        if (fp->fdp_fd == fd)
        {
            while (i--)
            {
                fp[0] = fp[1];
                fp++;
            }
            INTER->i_fdpoll = (t_fdpoll *)t_resizebytes(
                INTER->i_fdpoll, size, size - sizeof(t_fdpoll));
            INTER->i_nfdpoll = nfd - 1;
            return;
        }
    }
    post("warning: %d removed from poll list but not found", fd);
}

    /* Size of the buffer used for parsing FUDI messages
    received over TCP. Must be a power of two!
    LATER make this settable per socketreceiver instance */
#define INBUFSIZE 4096

t_socketreceiver *socketreceiver_new(void *owner, t_socketnotifier notifier,
    t_socketreceivefn socketreceivefn, int udp)
{
    t_socketreceiver *x = (t_socketreceiver *)getbytes(sizeof(*x));
    x->sr_inhead = x->sr_intail = 0;
    x->sr_owner = owner;
    x->sr_notifier = notifier;
    x->sr_socketreceivefn = socketreceivefn;
    x->sr_udp = udp;
    x->sr_fromaddr = NULL;
    x->sr_fromaddrfn = NULL;
    if (!udp)
    {
        if (!(x->sr_inbuf = malloc(INBUFSIZE)))
            bug("t_socketreceiver");
    }
    else
        x->sr_inbuf = NULL;
    return (x);
}

void socketreceiver_free(t_socketreceiver *x)
{
    if (x->sr_inbuf)
        free(x->sr_inbuf);
    if (x->sr_fromaddr) free(x->sr_fromaddr);
    freebytes(x, sizeof(*x));
}

    /* this is in a separately called subroutine so that the buffer isn't
    sitting on the stack while the messages are getting passed. */
static int socketreceiver_doread(t_socketreceiver *x)
{
    char messbuf[INBUFSIZE], *bp = messbuf;
    int indx, first = 1;
    int inhead = x->sr_inhead;
    int intail = x->sr_intail;
    char *inbuf = x->sr_inbuf;
    for (indx = intail; first || (indx != inhead);
        first = 0, (indx = (indx+1)&(INBUFSIZE-1)))
    {
            /* if we hit a semi that isn't preceded by a \, it's a message
            boundary. LATER we should deal with the possibility that the
            preceding \ might itself be escaped! */
        char c = *bp++ = inbuf[indx];
        if (c == ';' && (!indx || inbuf[indx-1] != '\\'))
        {
            intail = (indx+1)&(INBUFSIZE-1);
            binbuf_text(INTER->i_inbinbuf, messbuf, bp - messbuf);
            if (sys_debuglevel & DEBUG_MESSDOWN)
            {
                size_t bufsize = (bp>messbuf)?(bp-messbuf):0;
        #ifdef _WIN32
            #ifdef _MSC_VER
                fwprintf(stderr, L"<< %.*S\n", (int)bufsize, messbuf);
            #else
                fwprintf(stderr, L"<< %.*s\n", (int)bufsize, messbuf);
            #endif
                fflush(stderr);
        #else
                fprintf(stderr, "<< %.*s\n", (int)bufsize, messbuf);
        #endif
            }
            x->sr_inhead = inhead;
            x->sr_intail = intail;
            return (1);
        }
    }
    return (0);
}

static void socketreceiver_getudp(t_socketreceiver *x, int fd)
{
    char *buf = (char *)sys_getrecvbuf(0);
    socklen_t fromaddrlen = sizeof(struct sockaddr_storage);
    int ret, readbytes = 0;
    while (1)
    {
        ret = (int)recvfrom(fd, buf, NET_MAXPACKETSIZE-1, 0,
            (struct sockaddr *)x->sr_fromaddr, (x->sr_fromaddr ? &fromaddrlen : 0));
        if (ret < 0)
        {
                /* socket_errno_udp() ignores some error codes */
            if (socket_errno_udp())
            {
                sys_sockerror("recv (udp)");
                    /* only notify and shutdown a UDP sender! */
                if (x->sr_notifier)
                {
                    (*x->sr_notifier)(x->sr_owner, fd);
                    sys_rmpollfn(fd);
                    sys_closesocket(fd);
                }
            }
            return;
        }
        else if (ret > 0)
        {
                /* handle too large UDP packets */
            if (ret > NET_MAXPACKETSIZE-1)
            {
                post("warning: incoming UDP packet truncated from %d to %d bytes.",
                    ret, NET_MAXPACKETSIZE-1);
                ret = NET_MAXPACKETSIZE-1;
            }
            buf[ret] = 0;
    #if 0
            post("%s", buf);
    #endif
            if (buf[ret-1] != '\n')
            {
    #if 0
                pd_error(0, "dropped bad buffer %s\n", buf);
    #endif
            }
            else
            {
                char *semi = strchr(buf, ';');
                if (semi)
                    *semi = 0;
                if (x->sr_fromaddrfn)
                    (*x->sr_fromaddrfn)(x->sr_owner, (const void *)x->sr_fromaddr);
                binbuf_text(INTER->i_inbinbuf, buf, strlen(buf));
                outlet_setstacklim();
                if (x->sr_socketreceivefn)
                    (*x->sr_socketreceivefn)(x->sr_owner,
                        INTER->i_inbinbuf);
                else bug("socketreceiver_getudp");
            }
            readbytes += ret;
            /* throttle */
            if (readbytes >= NET_MAXPACKETSIZE)
                return;
            /* check for pending UDP packets */
            if (socket_bytes_available(fd) <= 0)
                return;
        }
    }
}

void socketreceiver_read(t_socketreceiver *x, int fd)
{
    if (x->sr_udp)   /* UDP ("datagram") socket protocol */
        socketreceiver_getudp(x, fd);
    else  /* TCP ("streaming") socket protocol */
    {
        char *semi;
        int readto =
            (x->sr_inhead >= x->sr_intail ? INBUFSIZE : x->sr_intail-1);
        int ret;

            /* the input buffer might be full. If so, drop the whole thing */
        if (readto == x->sr_inhead)
        {
            fprintf(stderr, "pd: dropped message from gui\n");
            x->sr_inhead = x->sr_intail = 0;
            readto = INBUFSIZE;
        }
        else
        {
            ret = (int)recv(fd, x->sr_inbuf + x->sr_inhead,
                readto - x->sr_inhead, 0);
            if (ret <= 0)
            {
                if (ret < 0)
                    sys_sockerror("recv (tcp)");
                if (x == INTER->i_socketreceiver)
                {
                    if (pd_this == &pd_maininstance)
                    {
                        fprintf(stderr, "read from GUI socket: %s; stopping\n",
                            strerror(errno));
                        sys_bail(1);
                    }
                    else
                    {
                        sys_rmpollfn(fd);
                        sys_closesocket(fd);
                        sys_stopgui();
                    }
                }
                else
                {
                    if (x->sr_notifier)
                        (*x->sr_notifier)(x->sr_owner, fd);
                    sys_rmpollfn(fd);
                    sys_closesocket(fd);
                }
            }
            else
            {
                x->sr_inhead += ret;
                if (x->sr_inhead >= INBUFSIZE) x->sr_inhead = 0;
                while (socketreceiver_doread(x))
                {
                    if (x->sr_fromaddrfn)
                    {
                        socklen_t fromaddrlen = sizeof(struct sockaddr_storage);
                        if(!getpeername(fd,
                                        (struct sockaddr *)x->sr_fromaddr,
                                        &fromaddrlen))
                            (*x->sr_fromaddrfn)(x->sr_owner,
                                (const void *)x->sr_fromaddr);
                    }
                    outlet_setstacklim();
                    if (x->sr_socketreceivefn)
                        (*x->sr_socketreceivefn)(x->sr_owner,
                            INTER->i_inbinbuf);
                    else binbuf_eval(INTER->i_inbinbuf, 0, 0, 0);
                    if (x->sr_inhead == x->sr_intail)
                        break;
                }
            }
        }
    }
}

void socketreceiver_set_fromaddrfn(t_socketreceiver *x,
    t_socketfromaddrfn fromaddrfn)
{
    x->sr_fromaddrfn = fromaddrfn;
    if (fromaddrfn)
    {
        if (!x->sr_fromaddr)
            x->sr_fromaddr = malloc(sizeof(struct sockaddr_storage));
    }
    else if (x->sr_fromaddr)
    {
        free(x->sr_fromaddr);
        x->sr_fromaddr = NULL;
    }
}

void sys_closesocket(int sockfd)
{
    socket_close(sockfd);
}

/* ---------------------- sending messages to the GUI ------------------ */
#define GUI_ALLOCCHUNK 8192
#define GUI_UPDATESLICE 512 /* how much we try to do in one idle period */
#define GUI_BYTESPERPING 1024 /* how much we send up per ping */

static void sys_trytogetmoreguibuf(int newsize)
{
        /* newsize can be negative if it overflows (at 0x7FFFFFFF)
         * which only happens if we push a huge amount of data to the GUI,
         * such as printing a billion numbers
         *
         * we could fix this by using size_t (or ssize_t), but this will
         * possibly lead to memory exhaustion.
         * as the overflow happens at 2GB which is rather large anyhow,
         * but most machines will still be able to handle this without swapping
         * and crashing, we just use the 2GB limit to trigger a synchronous write.
     * also note that on the Tcl/Tk side, the maximum size of a buffer is 2GB,
     * so there's a nice analogy here.
         */
    char *newbuf = (newsize>=0)?realloc(INTER->i_guibuf, newsize):0;
#if 0
    static int sizewas;
    if (newsize > 70000 && sizewas < 70000)
    {
        int i;
        for (i = INTER->i_guitail; i < INTER->i_guihead; i++)
                fputc(INTER->i_guibuf[i], stderr);
    }
    sizewas = newsize;
#endif
#if 0
    fprintf(stderr, "new size %d (head %d, tail %d)\n",
        newsize, INTER->i_guihead, INTER->i_guitail);
#endif

        /* if realloc fails, make a last-ditch attempt to stay alive by
        synchronously writing out the existing contents.  LATER test
        this by intentionally setting newbuf to zero */
    if (!newbuf)
    {
        int bytestowrite = INTER->i_guihead - INTER->i_guitail;
        int written = 0;
        while (1)
        {
            int res = (int)send(
                INTER->i_guisock,
                INTER->i_guibuf + INTER->i_guitail + written,
                bytestowrite, 0);
            if (res < 0)
            {
                perror("pd output pipe");
                sys_bail(1);
            }
            else
            {
                written += res;
                if (written >= bytestowrite)
                    break;
            }
        }
        INTER->i_guihead = INTER->i_guitail = 0;
    }
    else
    {
        INTER->i_guisize = newsize;
        INTER->i_guibuf = newbuf;
    }
}

int sys_havegui(void)
{
    return (INTER->i_havegui);
}

void sys_vgui(const char *fmt, ...)
{
    // This call causes a circular loop, so ignore it
    if(strncmp(fmt, "pdtk_canvas_raise", strlen("pdtk_canvas_raise")) == 0) {
        return;
    }
    update_gui(NULL);
}


void sys_gui(const char *s)
{
    update_gui(NULL);
}
static int sys_flushtogui(void)
{
    int writesize = INTER->i_guihead - INTER->i_guitail,
        nwrote = 0;
    if (writesize > 0)
        nwrote = (int)send(
            INTER->i_guisock,
            INTER->i_guibuf + INTER->i_guitail,
            writesize, 0);

#if 0
    if (writesize)
        fprintf(stderr, "wrote %d of %d\n", nwrote, writesize);
#endif

    if (nwrote < 0)
    {
        perror("pd-to-gui socket");
        sys_bail(1);
    }
    else if (!nwrote)
        return (0);
    else if (nwrote >= INTER->i_guihead - INTER->i_guitail)
        INTER->i_guihead = INTER->i_guitail = 0;
    else if (nwrote)
    {
        INTER->i_guitail += nwrote;
        if (INTER->i_guitail > (INTER->i_guisize >> 2))
        {
            memmove(INTER->i_guibuf,
                INTER->i_guibuf  + INTER->i_guitail,
                INTER->i_guihead - INTER->i_guitail);
            INTER->i_guihead = INTER->i_guihead - INTER->i_guitail;
            INTER->i_guitail = 0;
        }
    }
    return (1);
}

void glob_ping(t_pd *dummy)
{
    INTER->i_waitingforping = 0;
}

static int sys_flushqueue(void)
{
    int wherestop = INTER->i_bytessincelastping + GUI_UPDATESLICE;
    if (wherestop + (GUI_UPDATESLICE >> 1) > GUI_BYTESPERPING)
        wherestop = 0x7fffffff;
    if (INTER->i_waitingforping)
        return (0);
    if (!INTER->i_guiqueuehead)
        return (0);
    while (1)
    {
        if (INTER->i_bytessincelastping >= GUI_BYTESPERPING)
        {
            sys_gui("pdtk_ping\n");
            INTER->i_bytessincelastping = 0;
            INTER->i_waitingforping = 1;
            return (1);
        }
        if (INTER->i_guiqueuehead)
        {
            t_guiqueue *headwas = INTER->i_guiqueuehead;
            INTER->i_guiqueuehead = headwas->gq_next;
            (*headwas->gq_fn)(headwas->gq_client, headwas->gq_glist);
            t_freebytes(headwas, sizeof(*headwas));
            if (INTER->i_bytessincelastping >= wherestop)
                break;
        }
        else break;
    }
    sys_flushtogui();
    return (1);
}

    /* flush output buffer and update queue to gui in small time slices */
static int sys_poll_togui(void) /* returns 1 if did anything */
{
    if (!sys_havegui())
        return (0);
        /* in case there is stuff still in the buffer, try to flush it. */
    sys_flushtogui();
        /* if the flush wasn't complete, wait. */
    if (INTER->i_guihead > INTER->i_guitail)
        return (0);

        /* check for queued updates */
    if (sys_flushqueue())
        return (1);

    return (0);
}

    /* if some GUI object is having to do heavy computations, it can tell
    us to back off from doing more updates by faking a big one itself. */
void sys_pretendguibytes(int n)
{
    INTER->i_bytessincelastping += n;
}

void sys_queuegui(void *client, t_glist *glist, t_guicallbackfn f)
{
  update_gui(client);
}


void sys_unqueuegui(void *client)
{
    t_guiqueue *gq, *gq2;
    while (INTER->i_guiqueuehead && INTER->i_guiqueuehead->gq_client == client)
    {
        gq = INTER->i_guiqueuehead;
        INTER->i_guiqueuehead = INTER->i_guiqueuehead->gq_next;
        t_freebytes(gq, sizeof(*gq));
    }
    if (!INTER->i_guiqueuehead)
        return;
    for (gq = INTER->i_guiqueuehead; (gq2 = gq->gq_next); gq = gq2)
        if (gq2->gq_client == client)
        {
            gq->gq_next = gq2->gq_next;
            t_freebytes(gq2, sizeof(*gq2));
            break;
        }
}

    /* poll for any incoming packets, or for GUI updates to send.  call with
    the PD instance lock set. */
int sys_pollgui(void)
{
    static double lasttime = 0;
    double now = 0;
    int didsomething = sys_domicrosleep(0);
    if (!didsomething || (now = sys_getrealtime()) > lasttime + 0.5)
    {
        didsomething |= sys_poll_togui();
        if (now)
            lasttime = now;
    }
    return (didsomething);
}

void sys_init_fdpoll(void)
{
    if (INTER->i_fdpoll)
        return;
    /* create an empty FD poll list */
    INTER->i_fdpoll = (t_fdpoll *)t_getbytes(0);
    INTER->i_nfdpoll = 0;
    INTER->i_inbinbuf = binbuf_new();
}

/* --------------------- starting up the GUI connection ------------- */

static int sys_watchfd;

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__)
void glob_watchdog(t_pd *dummy)
{
    if (write(sys_watchfd, "\n", 1) < 1)
    {
        fprintf(stderr, "pd: watchdog process died\n");
        sys_bail(1);
    }
}
#endif

static void sys_init_deken(void)
{
    const char*os =
#if defined __linux__
        "Linux"
#elif defined __APPLE__
        "Darwin"
#elif defined __FreeBSD__
        "FreeBSD"
#elif defined __NetBSD__
        "NetBSD"
#elif defined __OpenBSD__
        "OpenBSD"
#elif defined _WIN32
        "Windows"
#else
# if defined(__GNUC__)
#  warning unknown OS
# endif
        0
#endif
        ;
    const char*machine =
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
        "amd64"
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
        "i386"
#elif defined(__ppc__)
        "ppc"
#elif defined(__aarch64__)
        "arm64"
#elif defined (__ARM_ARCH)
        "armv" stringify(__ARM_ARCH)
# if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#  if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        "b"
#  endif
# endif
#else
# if defined(__GNUC__)
#  warning unknown architecture
# endif
        0
#endif
        ;

        /* only send the arch info, if we are sure about it... */
    if (os && machine)
        sys_vgui("::deken::set_platform %s %s %d %d\n",
                 os, machine,
                 8 * sizeof(char*),
                 8 * sizeof(t_float));
}

static int sys_do_startgui(const char *libdir)
{
    char quotebuf[MAXPDSTRING];
    char apibuf[256], apibuf2[256];
    struct addrinfo *ailist = NULL, *ai;
    int sockfd = -1;
    int portno = -1;
#ifndef _WIN32
    int stdinpipe[2];
    pid_t childpid;
#endif /* _WIN32 */

    sys_init_fdpoll();

    if (sys_guisetportnumber)  /* GUI exists and sent us a port number */
    {
        int status;
#ifdef __APPLE__
            /* guisock might be 1 or 2, which will have offensive results
            if somebody writes to stdout or stderr - so we just open a few
            files to try to fill fds 0 through 2.  (I tried using dup()
            instead, which would seem the logical way to do this, but couldn't
            get it to work.) */
        int burnfd1 = open("/dev/null", 0), burnfd2 = open("/dev/null", 0),
            burnfd3 = open("/dev/null", 0);
        if (burnfd1 > 2)
            close(burnfd1);
        if (burnfd2 > 2)
            close(burnfd2);
        if (burnfd3 > 2)
            close(burnfd3);
#endif

        /* get addrinfo list using hostname & port */
        status = addrinfo_get_list(&ailist,
            LOCALHOST, sys_guisetportnumber, SOCK_STREAM);
        if (status != 0)
        {
            fprintf(stderr,
                "localhost not found (inet protocol not installed?)\n%s (%d)",
                gai_strerror(status), status);
            return (1);
        }

        /* Sort to IPv4 for now as the Pd gui uses IPv4. */
        addrinfo_sort_list(&ailist, addrinfo_ipv4_first);

        /* We don't know in advance whether the GUI uses IPv4 or IPv6,
           so we try both and pick the one which works. */
        for (ai = ailist; ai != NULL; ai = ai->ai_next)
        {
            /* create a socket */
            sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (sockfd < 0)
                continue;
        #if 1
            if (socket_set_boolopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
                fprintf(stderr, "setsockopt (TCP_NODELAY) failed");
        #endif
            /* try to connect */
            if (socket_connect(sockfd, ai->ai_addr, ai->ai_addrlen, 10.f) < 0)
            {
                sys_closesocket(sockfd);
                sockfd = -1;
                continue;
            }
            /* this addr worked */
            break;
        }
        freeaddrinfo(ailist);

        /* confirm that we could connect */
        if (sockfd < 0)
        {
            sys_sockerror("connecting stream socket");
            return (1);
        }

        INTER->i_guisock = sockfd;
    }
    else    /* default behavior: start up the GUI ourselves. */
    {
        struct sockaddr_storage addr;
        int status;
#ifdef _WIN32
        char scriptbuf[MAXPDSTRING+30], wishbuf[MAXPDSTRING+30], portbuf[80];
        int spawnret;
#else
        char cmdbuf[4*MAXPDSTRING];
        const char *guicmd;
#endif
        /* get addrinfo list using hostname (get random port from OS) */
        status = addrinfo_get_list(&ailist, LOCALHOST, 0, SOCK_STREAM);
        if (status != 0)
        {
            fprintf(stderr,
                "localhost not found (inet protocol not installed?)\n%s (%d)",
                gai_strerror(status), status);
            return (1);
        }
        /* we prefer the IPv4 addresses because the GUI might not be IPv6 capable. */
        addrinfo_sort_list(&ailist, addrinfo_ipv4_first);
        /* try each addr until we find one that works */
        for (ai = ailist; ai != NULL; ai = ai->ai_next)
        {
            sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (sockfd < 0)
                continue;
        #if 1
            /* ask OS to allow another process to reopen this port after we close it */
            if (socket_set_boolopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1) < 0)
                fprintf(stderr, "setsockopt (SO_REUSEADDR) failed\n");
        #endif
        #if 1
            /* stream (TCP) sockets are set NODELAY */
            if (socket_set_boolopt(sockfd, IPPROTO_TCP, TCP_NODELAY, 1) < 0)
                fprintf(stderr, "setsockopt (TCP_NODELAY) failed");
        #endif
            /* name the socket */
            if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
            {
                socket_close(sockfd);
                sockfd = -1;
                continue;
            }
            /* this addr worked */
            memcpy(&addr, ai->ai_addr, ai->ai_addrlen);
            break;
        }
        freeaddrinfo(ailist);

        /* confirm that socket/bind worked */
        if (sockfd < 0)
        {
            sys_sockerror("bind");
            return (1);
        }
        /* get the actual port number */
        portno = socket_get_port(sockfd);
        if (sys_verbose) fprintf(stderr, "port %d\n", portno);

#ifndef _WIN32
        if (sys_guicmd)
            guicmd = sys_guicmd;
        else
        {
#ifdef __APPLE__
            int i;
            struct stat statbuf;
            glob_t glob_buffer;
            char *homedir = getenv("HOME");
            char embed_glob[FILENAME_MAX];
            char home_filename[FILENAME_MAX];
            char *wish_paths[11] = {
 "(custom wish not defined)",
 "(did not find a home directory)",
 "/Applications/Utilities/Wish.app/Contents/MacOS/Wish",
 "/Applications/Utilities/Wish Shell.app/Contents/MacOS/Wish Shell",
 "/Applications/Wish.app/Contents/MacOS/Wish",
 "/Applications/Wish Shell.app/Contents/MacOS/Wish Shell",
 "/Library/Frameworks/Tk.framework/Resources/Wish.app/Contents/MacOS/Wish",
 "/Library/Frameworks/Tk.framework/Resources/Wish Shell.app/Contents/MacOS/Wish Shell",
 "/System/Library/Frameworks/Tk.framework/Resources/Wish.app/Contents/MacOS/Wish",
 "/System/Library/Frameworks/Tk.framework/Resources/Wish Shell.app/Contents/MacOS/Wish Shell",
 "/usr/bin/wish"
            };
            /* this glob is needed so the Wish executable can have the same
             * filename as the Pd.app, i.e. 'Pd-0.42-3.app' should have a Wish
             * executable called 'Pd-0.42-3.app/Contents/MacOS/Pd-0.42-3' */
            sprintf(embed_glob, "%s/../MacOS/Pd*", libdir);
            glob_buffer.gl_matchc = 1; /* we only need one match */
            glob(embed_glob, GLOB_LIMIT, NULL, &glob_buffer);
            /* If we are using a copy of Wish embedded in the Pd.app, then it
             * will automatically load pd-gui.tcl if that embedded Wish can
             * find ../Resources/Scripts/AppMain.tcl, then Wish doesn't want
             * to receive the pd-gui.tcl as an argument.  Otherwise it needs
             * to know how to find pd-gui.tcl */
            if (glob_buffer.gl_pathc > 0)
                sprintf(cmdbuf, "\"%s\" %d\n", glob_buffer.gl_pathv[0], portno);
            else
            {
                int wish_paths_count = sizeof(wish_paths)/sizeof(*wish_paths);
                #ifdef WISH
                    wish_paths[0] = WISH;
                #endif
                sprintf(home_filename,
                        "%s/Applications/Wish.app/Contents/MacOS/Wish",homedir);
                wish_paths[1] = home_filename;
                for(i=0; i<wish_paths_count; i++)
                {
                    if (sys_verbose)
                        fprintf(stderr, "Trying Wish at \"%s\"\n",
                            wish_paths[i]);
                    if (stat(wish_paths[i], &statbuf) >= 0)
                        break;
                }
                if(i>=wish_paths_count)
                {
                    fprintf(stderr, "sys_startgui couldn't find tcl/tk\n");
                    sys_closesocket(sockfd);
                    return (1);
                }
                sprintf(cmdbuf, "\"%s\" \"%s/%spd-gui.tcl\" %d\n",
                        wish_paths[i], libdir, PDGUIDIR, portno);
            }
#else /* __APPLE__ */
            /* sprintf the wish command with needed environment variables.
            For some reason the wish script fails if HOME isn't defined so
            if necessary we put that in here too. */
            sprintf(cmdbuf,
  "TCL_LIBRARY=\"%s/lib/tcl/library\" TK_LIBRARY=\"%s/lib/tk/library\"%s \
  " WISH " \"%s/" PDGUIDIR "/pd-gui.tcl\" %d\n",
                 libdir, libdir, (getenv("HOME") ? "" : " HOME=/tmp"),
                    libdir, portno);
#endif /* __APPLE__ */
            guicmd = cmdbuf;
        }
        if (sys_verbose)
            fprintf(stderr, "%s", guicmd);

        childpid = fork();
        if (childpid < 0)
        {
            if (errno) perror("sys_startgui");
            else fprintf(stderr, "sys_startgui failed\n");
            sys_closesocket(sockfd);
            return (1);
        }
        else if (!childpid)                     /* we're the child */
        {
            sys_closesocket(sockfd);     /* child doesn't listen */
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__GNU__)
            sys_set_priority(MODE_NRT);  /* child runs non-real-time */
#endif
#ifndef __APPLE__
// TODO this seems unneeded on any platform hans@eds.org
                /* the wish process in Unix will make a wish shell and
                    read/write standard in and out unless we close the
                    file descriptors.  Somehow this doesn't make the MAC OSX
                        version of Wish happy...*/
            if (pipe(stdinpipe) < 0)
                sys_sockerror("pipe");
            else
            {
                if (stdinpipe[0] != 0)
                {
                    close (0);
                    dup2(stdinpipe[0], 0);
                    close(stdinpipe[0]);
                }
            }
#endif /* NOT __APPLE__ */
            execl("/bin/sh", "sh", "-c", guicmd, (char*)0);
            perror("pd: exec");
            fprintf(stderr, "Perhaps tcl and tk aren't yet installed?\n");
            _exit(1);
       }
#else /* NOT _WIN32 */
        /* fprintf(stderr, "%s\n", libdir); */

        strcpy(scriptbuf, "\"");
        strcat(scriptbuf, libdir);
        strcat(scriptbuf, "/" PDGUIDIR "pd-gui.tcl\"");
        sys_bashfilename(scriptbuf, scriptbuf);

        sprintf(portbuf, "%d", portno);

        strcpy(wishbuf, libdir);
        strcat(wishbuf, "/" PDBINDIR WISH);
        sys_bashfilename(wishbuf, wishbuf);

        spawnret = _spawnl(P_NOWAIT, wishbuf, WISH, scriptbuf, portbuf, NULL);
        if (spawnret < 0)
        {
            perror("spawnl");
            fprintf(stderr, "%s: couldn't load TCL\n", wishbuf);
            return (1);
        }
#endif /* NOT _WIN32 */
        if (sys_verbose)
            fprintf(stderr, "Waiting for connection request... \n");
        if (listen(sockfd, 5) < 0)
        {
            sys_sockerror("listen");
            sys_closesocket(sockfd);
            return (1);
        }

        INTER->i_guisock = accept(sockfd, 0, 0);

        sys_closesocket(sockfd);

        if (INTER->i_guisock < 0)
        {
            sys_sockerror("accept");
            return (1);
        }
        if (sys_verbose)
            fprintf(stderr, "... connected\n");
        INTER->i_guihead = INTER->i_guitail = 0;
    }

    INTER->i_socketreceiver = socketreceiver_new(0, 0, 0, 0);
    sys_addpollfn(INTER->i_guisock,
        (t_fdpollfn)socketreceiver_read,
            INTER->i_socketreceiver);

            /* here is where we start the pinging. */
#if defined(__linux__) || defined(__FreeBSD_kernel__)
    if (sys_hipriority)
        sys_gui("pdtk_watchdog\n");
#endif
    sys_get_audio_apis(apibuf);
    sys_get_midi_apis(apibuf2);
    sys_set_searchpath();     /* tell GUI about path and startup flags */
    sys_set_temppath();
    sys_set_extrapath();
    sys_set_startup();
                       /* ... and about font, medio APIS, etc */
    sys_vgui("pdtk_pd_startup %d %d %d {%s} %s %s {%s} %s\n",
             PD_MAJOR_VERSION, PD_MINOR_VERSION,
             PD_BUGFIX_VERSION, PD_TEST_VERSION,
             apibuf, apibuf2,
             pdgui_strnescape(quotebuf, MAXPDSTRING, sys_font, 0),
             sys_fontweight);
    sys_vgui("set zoom_open %d\n", sys_zoom_open == 2);

    sys_init_deken();

    do {
        t_audiosettings as;
        sys_get_audio_settings(&as);
        sys_vgui("set pd_whichapi %d\n", as.a_api);
    } while(0);
    return (0);
}

void sys_setrealtime(const char *libdir)
{
    char cmdbuf[MAXPDSTRING];
#if defined(__linux__) || defined(__FreeBSD_kernel__)
        /*  promote this process's priority, if we can and want to.
        If sys_hipriority not specified (-1), we assume real-time was wanted.
        Starting in Linux 2.6 one can permit real-time operation of Pd by]
        putting lines like:
                @audio - rtprio 99
                @audio - memlock unlimited
        in the system limits file, perhaps /etc/limits.conf or
        /etc/security/limits.conf, and calling Pd from a user in group audio. */
    if (sys_hipriority == -1)
        sys_hipriority = 1;

    snprintf(cmdbuf, MAXPDSTRING, "%s/bin/pd-watchdog", libdir);
    cmdbuf[MAXPDSTRING-1] = 0;
    if (sys_hipriority)
    {
        struct stat statbuf;
        if (stat(cmdbuf, &statbuf) < 0)
        {
            fprintf(stderr,
              "disabling real-time priority due to missing pd-watchdog (%s)\n",
                cmdbuf);
            sys_hipriority = 0;
        }
    }
    if (sys_hipriority)
    {
        int pipe9[2], watchpid;
            /* To prevent lockup, we fork off a watchdog process with
            higher real-time priority than ours.  The GUI has to send
            a stream of ping messages to the watchdog THROUGH the Pd
            process which has to pick them up from the GUI and forward
            them.  If any of these things aren't happening the watchdog
            starts sending "stop" and "cont" signals to the Pd process
            to make it timeshare with the rest of the system.  (Version
            0.33P2 : if there's no GUI, the watchdog pinging is done
            from the scheduler idle routine in this process instead.) */

        if (pipe(pipe9) < 0)
        {
            sys_sockerror("pipe");
            return;
        }
        watchpid = fork();
        if (watchpid < 0)
        {
            if (errno)
                perror("sys_setpriority");
            else fprintf(stderr, "sys_setpriority failed\n");
            return;
        }
        else if (!watchpid)             /* we're the child */
        {
            sys_set_priority(MODE_WATCHDOG);
            if (pipe9[1] != 0)
            {
                dup2(pipe9[0], 0);
                close(pipe9[0]);
            }
            close(pipe9[1]);

            if (sys_verbose) fprintf(stderr, "%s\n", cmdbuf);
            execl(cmdbuf, cmdbuf, (char*)0);
            perror("pd: exec");
            _exit(1);
        }
        else                            /* we're the parent */
        {
            sys_set_priority(MODE_RT);
            close(pipe9[0]);
                /* set close-on-exec so that watchdog will see an EOF when we
                close our copy - otherwise it might hang waiting for some
                stupid child process (as seems to happen if jackd auto-starts
                for us.) */
            if(fcntl(pipe9[1], F_SETFD, FD_CLOEXEC) < 0)
              perror("close-on-exec");
            sys_watchfd = pipe9[1];
                /* We also have to start the ping loop in the GUI;
                this is done later when the socket is open. */
        }
    }
    else logpost(NULL, PD_VERBOSE, "not setting real-time priority");
#endif /* __linux__ */

#ifdef _WIN32
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        fprintf(stderr, "pd: couldn't set high priority class\n");
#endif
#ifdef __APPLE__
    if (sys_hipriority)
    {
        struct sched_param param;
        int policy = SCHED_RR;
        int err;
        param.sched_priority = 80; /* adjust 0 : 100 */

        err = pthread_setschedparam(pthread_self(), policy, &param);
        if (err)
            post("warning: high priority scheduling failed");
    }
#endif /* __APPLE__ */
}

/* This is called when something bad has happened, like a segfault.
Call glob_quit() below to exit cleanly.
LATER try to save dirty documents even in the bad case. */
void sys_bail(int n)
{
    static int reentered = 0;
    if (!reentered)
    {
        reentered = 1;
#if !defined(__linux__) && !defined(__FreeBSD_kernel__) && !defined(__GNU__)
            /* sys_close_audio() hangs if you're in a signal? */
        fprintf(stderr ,"gui socket %d - \n", INTER->i_guisock);
        fprintf(stderr, "closing audio...\n");
        sys_close_audio();
        fprintf(stderr, "closing MIDI...\n");
        sys_close_midi();
        fprintf(stderr, "... done.\n");
#endif
        exit(n);
    }
    else _exit(1);
}

extern void sys_exit(void);

void glob_exit(void *dummy, t_float status)
{
        /* sys_exit() sets the sys_quit flag, so all loops end */
    sys_exit();
    sys_close_audio();
    sys_close_midi();
    if (sys_havegui())
    {
        sys_closesocket(INTER->i_guisock);
        sys_rmpollfn(INTER->i_guisock);
    }
    exit((int)status);
}
void glob_quit(void *dummy)
{
    glob_exit(dummy, 0);
}

    /* recursively descend to all canvases and send them "vis" messages
    if they believe they're visible, to make it really so. */
static void glist_maybevis(t_glist *gl)
{
    t_gobj *g;
    for (g = gl->gl_list; g; g = g->g_next)
        if (pd_class(&g->g_pd) == canvas_class)
            glist_maybevis((t_glist *)g);
    if (gl->gl_havewindow)
    {
        canvas_vis(gl, 0);
        canvas_vis(gl, 1);
    }
}

int sys_startgui(const char *libdir)
{
    t_canvas *x;
    for (x = pd_getcanvaslist(); x; x = x->gl_next)
        canvas_vis(x, 0);
    INTER->i_havegui = 1;
    INTER->i_guihead = INTER->i_guitail = 0;
    if (sys_do_startgui(libdir))
        return (-1);
    for (x = pd_getcanvaslist(); x; x = x->gl_next)
        if (strcmp(x->gl_name->s_name, "_float_template") &&
            strcmp(x->gl_name->s_name, "_float_array_template") &&
                strcmp(x->gl_name->s_name, "_text_template"))
    {
        glist_maybevis(x);
        canvas_vis(x, 1);
    }
    return (0);
}

 /* more work needed here - for some reason we can't restart the gui after
 shutting it down this way.  I think the second 'init' message never makes
 it because the to-gui buffer isn't re-initialized. */
void sys_stopgui(void)
{
    t_canvas *x;
    for (x = pd_getcanvaslist(); x; x = x->gl_next)
        canvas_vis(x, 0);
    sys_vgui("%s", "exit\n");
    if (INTER->i_guisock >= 0)
    {
        sys_closesocket(INTER->i_guisock);
        sys_rmpollfn(INTER->i_guisock);
        INTER->i_guisock = -1;
    }
    INTER->i_havegui = 0;
}

/* ----------- mutexes for thread safety --------------- */

void s_inter_newpdinstance(void)
{
    INTER = getbytes(sizeof(*INTER));
#if PDTHREADS
    pthread_mutex_init(&INTER->i_mutex, NULL);
    pd_this->pd_islocked = 0;
#endif
#ifdef _WIN32
    INTER->i_freq = 0;
#endif
    INTER->i_havegui = 0;
}

void s_inter_free(t_instanceinter *inter)
{
    if (inter->i_fdpoll)
    {
        binbuf_free(inter->i_inbinbuf);
        inter->i_inbinbuf = 0;
        t_freebytes(inter->i_fdpoll, inter->i_nfdpoll * sizeof(t_fdpoll));
        inter->i_fdpoll = 0;
        inter->i_nfdpoll = 0;
    }
    freebytes(inter, sizeof(*inter));
}

void s_inter_freepdinstance(void)
{
    s_inter_free(INTER);
}

#if PDTHREADS
#ifdef PDINSTANCE
static pthread_rwlock_t sys_rwlock = PTHREAD_RWLOCK_INITIALIZER;
#else /* PDINSTANCE */
static pthread_mutex_t sys_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif /* PDINSTANCE */
#endif /* PDTHREADS */

#if PDTHREADS

/* routines to lock and unlock Pd's global class structure or list of Pd
instances.  These are called internally within Pd when creating classes, adding
methods to them, or creating or freeing Pd instances.  They should probably
not be called from outside Pd.  They should be called at a point where the
current instance of Pd is currently locked via sys_lock() below; this gains
read access to the class and instance lists which must be released for the
write-lock to be available. */

void pd_globallock(void)
{
#ifdef PDINSTANCE
    if (!pd_this->pd_islocked)
        bug("pd_globallock");
    pthread_rwlock_unlock(&sys_rwlock);
    pthread_rwlock_wrlock(&sys_rwlock);
#endif /* PDINSTANCE */
}

void pd_globalunlock(void)
{
#ifdef PDINSTANCE
    pthread_rwlock_unlock(&sys_rwlock);
    pthread_rwlock_rdlock(&sys_rwlock);
#endif /* PDINSTANCE */
}

/* routines to lock/unlock a Pd instance for thread safety.  Call pd_setinsance
first.  The "pd_this"  variable can be written and read thread-safely as it
is defined as per-thread storage. */
void sys_lock(void)
{
#ifdef PDINSTANCE
    pthread_mutex_lock(&INTER->i_mutex);
    pthread_rwlock_rdlock(&sys_rwlock);
    pd_this->pd_islocked = 1;
#else
    pthread_mutex_lock(&sys_mutex);
#endif
}

void sys_unlock(void)
{
#ifdef PDINSTANCE
    pd_this->pd_islocked = 0;
    pthread_rwlock_unlock(&sys_rwlock);
    pthread_mutex_unlock(&INTER->i_mutex);
#else
    pthread_mutex_unlock(&sys_mutex);
#endif
}

int sys_trylock(void)
{
#ifdef PDINSTANCE
    int ret;
    if (!(ret = pthread_mutex_trylock(&INTER->i_mutex)))
    {
        if (!(ret = pthread_rwlock_tryrdlock(&sys_rwlock)))
            return (0);
        else
        {
            pthread_mutex_unlock(&INTER->i_mutex);
            return (ret);
        }
    }
    else return (ret);
#else
    return pthread_mutex_trylock(&sys_mutex);
#endif
}

#else /* PDTHREADS */

#ifdef TEST_LOCKING /* run standalone Pd with this to find deadlocks */
static int amlocked;
void sys_lock(void)
{
    if (amlocked) bug("duplicate lock");
    amlocked = 1;
}

void sys_unlock(void)
{
    if (!amlocked) bug("duplicate unlock");
    amlocked = 0;
}
#else
void sys_lock(void) {}
void sys_unlock(void) {}
#endif
void pd_globallock(void) {}
void pd_globalunlock(void) {}

#endif /* PDTHREADS */

