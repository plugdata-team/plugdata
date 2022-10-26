
// Selects Linux and BSD
#if defined(__unix__) || !defined(__APPLE__)
extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}


typedef enum {
    WINDOW_STATE_NONE               = 0,
    WINDOW_STATE_MODAL              = (1 << 0),
    WINDOW_STATE_STICKY             = (1 << 1),
    WINDOW_STATE_MAXIMIZED_VERT     = (1 << 2),
    WINDOW_STATE_MAXIMIZED_HORZ     = (1 << 3),
    WINDOW_STATE_MAXIMIZED          = (WINDOW_STATE_MAXIMIZED_VERT | WINDOW_STATE_MAXIMIZED_HORZ),
    WINDOW_STATE_SHADED             = (1 << 4),
    WINDOW_STATE_SKIP_TASKBAR       = (1 << 5),
    WINDOW_STATE_SKIP_PAGER         = (1 << 6),
    WINDOW_STATE_HIDDEN             = (1 << 7),
    WINDOW_STATE_FULLSCREEN         = (1 << 8),
    WINDOW_STATE_ABOVE              = (1 << 9),
    WINDOW_STATE_BELOW              = (1 << 10),
    WINDOW_STATE_DEMANDS_ATTENTION  = (1 << 11),
    WINDOW_STATE_FOCUSED            = (1 << 12),
    WINDOW_STATE_SIZE               = 13,
} window_state_t;

typedef struct {

    Display *dpy;
    Window id;

    struct {
        Atom NET_WM_STATE;
        Atom NET_WM_STATES[WINDOW_STATE_SIZE];
    } atoms;

} window_t;

/* state names */

static const char* WINDOW_STATE_NAMES[] = {
    "_NET_WM_STATE_MODAL",
    "_NET_WM_STATE_STICKY",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_SHADED",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_BELOW",
    "_NET_WM_STATE_DEMANDS_ATTENTION",
    "_NET_WM_STATE_FOCUSED"
};

bool isMaximised(void* handle)
{


    window_t win;
    auto window = (Window)handle;
    auto* display = XOpenDisplay(NULL); 
      
    win.id = window;
    win.dpy = display;
    
    win.atoms.NET_WM_STATE = XInternAtom(win.dpy, "_NET_WM_STATE", False);

    for (int i = 0; i < WINDOW_STATE_SIZE; ++i) {
        win.atoms.NET_WM_STATES[i] = XInternAtom(win.dpy, WINDOW_STATE_NAMES[i], False);
    }
        

    long max_length = 1024;
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after, i, num_states = 0;
    Atom* states = NULL;
    window_state_t state = WINDOW_STATE_NONE;


    if (XGetWindowProperty(win.dpy,
                           win.id,
                           win.atoms.NET_WM_STATE,
                           0l,
                           max_length,
                           False,
                           XA_ATOM,
                           &actual_type,
                           &actual_format,
                           &num_states,
                           &bytes_after,
                           (unsigned char**) &states) == Success)
    {
        //for every state we get from the server
        for (i = 0; i < num_states; ++i) {

            //for every (known) state
            for (int n=0; n < WINDOW_STATE_SIZE; ++n) {
            
                //test the state at index i
                if (states[i] == win.atoms.NET_WM_STATES[n]) {
                
                    state = static_cast<window_state_t>(static_cast<int>(state) | (1 << n));
                    break;
                }
                
            }

        }

        XFree(states);
    }
    

    return state & WINDOW_STATE_MAXIMIZED;
}

void maximiseLinuxWindow(void* handle) {
      auto win = (Window)handle;
      auto* display = XOpenDisplay(NULL);
      
      XEvent ev;
      ev.xclient.window = win;
      ev.xclient.type = ClientMessage;
      ev.xclient.format = 32;
      ev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
      ev.xclient.data.l[0] = 2;
      ev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
      ev.xclient.data.l[2] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
      ev.xclient.data.l[3] = 1;

      XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
    
    XCloseDisplay(display);
}
#endif
