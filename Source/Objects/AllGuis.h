/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <g_canvas.h>

// Atoms
struct t_fake_gatom {
    t_text a_text;
    int a_flavor;                     /* A_FLOAT, A_SYMBOL, or A_LIST */
    t_glist* a_glist;                 /* owning glist */
    t_float a_toggle;                 /* value to toggle to */
    t_float a_draghi;                 /* high end of drag range */
    t_float a_draglo;                 /* low end of drag range */
    t_symbol* a_label;                /* symbol to show as label next to box */
    t_symbol* a_symfrom;              /* "receive" name -- bind ourselves to this */
    t_symbol* a_symto;                /* "send" name -- send to this on output */
    t_binbuf* a_revertbuf;            /* binbuf to revert to if typing canceled */
    int a_dragindex;                  /* index of atom being dragged */
    int a_fontsize;
    unsigned int a_shift : 1;         /* was shift key down when drag started? */
    unsigned int a_wherelabel : 2;    /* 0-3 for left, right, above, below */
    unsigned int a_grabbed : 1;       /* 1 if we've grabbed keyboard */
    unsigned int a_doubleclicked : 1; /* 1 if dragging from a double click */
    t_symbol* a_expanded_to;          /* a_symto after $0, $1, ...  expansion */
};

// [else/button]
struct t_fake_button {
    t_object        x_obj;
    t_clock        *x_clock;
    t_glist        *x_glist;
    void   *x_proxy;
    t_symbol       *x_bindname;
    int             x_mode;
    int             x_x;
    int             x_y;
    int             x_w;
    int             x_h;
    int             x_sel;
    int             x_zoom;
    int             x_edit;
    int             x_state;
    unsigned char   x_bgcolor[3];
    unsigned char   x_fgcolor[3];
};

// for [clone]
struct t_copy {
    t_glist* c_gl;
    int c_on; /* DSP running */
};

// for [clone]
struct t_in {
    t_class* i_pd;
    struct _clone* i_owner;
    int i_signal;
    int i_n;
};

// for [clone]
struct t_out {
    t_class* o_pd;
    t_outlet* o_outlet;
    int o_signal;
    int o_n;
};

// [clone]
struct t_fake_clone {
    t_object x_obj;
    int x_n;          /* number of copies */
    t_copy* x_vec;    /* the copies */
    int x_nin;
    t_in* x_invec;    /* inlet proxies */
    int x_nout;
    t_out** x_outvec; /* outlet proxies */
    t_symbol* x_s;    /* name of abstraction */
    int x_argc;       /* creation arguments for abstractions */
    t_atom* x_argv;
    int x_phase;
    int x_startvoice;    /* number of first voice, 0 by default */
    int x_suppressvoice; /* suppress voice number as $1 arg */
};

// [else/colors]
struct t_fake_colors {
    t_object x_obj;
    t_int x_hex;
    t_int x_gui;
    t_int x_rgb;
    t_int x_ds;
    t_symbol* x_id;
    char x_color[MAXPDSTRING];
};

// [cyclone/comment]
struct t_fake_comment {
    t_object x_obj;
    void* x_proxy;
    t_glist* x_glist;
    t_canvas* x_cv;
    t_binbuf* x_binbuf;
    char const* x_buf; // text buf
    int x_bufsize;     // text buf size
    int x_keynum;
    t_symbol* x_keysym;
    int x_init;
    int x_changed;
    int x_edit;
    int x_max_pixwidth;
    int x_bbset;
    int x_bbpending;
    int x_x1;
    int x_y1;
    int x_x2;
    int x_y2;
    int x_newx2;
    int x_dragon;
    int x_select;
    int x_fontsize;
    unsigned char x_red;
    unsigned char x_green;
    unsigned char x_blue;
    char x_color[8];
    char x_bgcolor[8];
    int x_shift;
    int x_selstart;
    int x_start_ndx;
    int x_end_ndx;
    int x_selend;
    int x_active;
    t_symbol* x_bindsym;
    t_symbol* x_fontname;
    t_symbol* x_receive;
    t_symbol* x_rcv_raw;
    int x_rcv_set;
    int x_flag;
    int x_r_flag;
    int x_old;
    int x_text_flag;
    int x_text_n;
    int x_text_size;
    int x_zoom;
    int x_fontface;
    int x_bold;
    int x_italic;
    int x_underline;
    int x_bg_flag;
    int x_textjust;       // 0: left, 1: center, 2: right
    unsigned int x_bg[3]; // background color
    t_pd* x_handle;
};

// [else/function]
struct t_fake_function {
    t_object        x_obj;
    t_glist        *x_glist;
    void           *x_proxy;
    int             x_state;
    int             x_n_states;
    int             x_flag;
    int             x_s_flag;
    int             x_r_flag;
    int             x_sel;
    int             x_width;
    int             x_height;
    int             x_init;
    int             x_grabbed; // number of grabbed point, for moving it/deleting it
    int             x_shift;
    int             x_snd_set;
    int             x_rcv_set;
    int             x_zoom;
    int             x_edit;
    t_symbol       *x_send;
    t_symbol       *x_receive;
    t_symbol       *x_snd_raw;
    t_symbol       *x_rcv_raw;
    float          *x_points;
    float          *x_dur;
    float           x_total_duration;
    float           x_min;
    float           x_max;
    float           x_min_point;
    float           x_max_point;
    float           x_pointer_x;
    float           x_pointer_y;
    unsigned char   x_fgcolor[3];
    unsigned char   x_bgcolor[3];
};

// [else/keyboard]
struct t_fake_keyboard {
    t_object       x_obj;
    t_glist       *x_glist;
    void          *x_proxy;
    int           *x_tgl_notes;    // to store which notes should be played
    int            x_velocity;     // to store velocity
    int            x_last_note;    // to store last note
    float          x_vel_in;       // to store the second inlet values
    float          x_space;
    int            x_width;
    int            x_height;
    int            x_octaves;
    int            x_first_c;
    int            x_low_c;
    int            x_toggle_mode;
    int            x_norm;
    int            x_zoom;
    int            x_shift;
    int            x_xpos;
    int            x_ypos;
    int            x_snd_set;
    int            x_rcv_set;
    int            x_flag;
    int            x_s_flag;
    int            x_r_flag;
    int            x_edit;
    t_symbol      *x_receive;
    t_symbol      *x_rcv_raw;
    t_symbol      *x_send;
    t_symbol      *x_snd_raw;
    t_symbol      *x_bindsym;
    t_outlet      *x_out;
};

// [else/knob]
struct t_fake_knob {
    t_object        x_obj;
    void           *x_proxy;
    t_glist        *x_glist;
    int             x_size;
    t_float         x_pos; // 0-1 normalized position
    t_float         x_exp;
    int             x_expmode;
    int             x_log;
    t_float         x_init;
    int             x_start_angle;
    int             x_end_angle;
    int             x_range;
    int             x_offset;
    int             x_ticks;
    int             x_outline;
    double          x_min;
    double          x_max;
    int             x_clicked;
    int             x_sel;
    int             x_shift;
    int             x_edit;
    int             x_jump;
    t_float         x_fval;
    t_symbol       *x_fg;
    t_symbol       *x_mg;
    t_symbol       *x_bg;
    t_symbol       *x_snd;
    t_symbol       *x_snd_raw;
    int             x_flag;
    int             x_r_flag;
    int             x_s_flag;
    int             x_rcv_set;
    int             x_snd_set;
    t_symbol       *x_rcv;
    t_symbol       *x_rcv_raw;
    int             x_circular;
    int             x_arc;
    int             x_zoom;
    int             x_discrete;
    char            x_tag_obj[128];
    char            x_tag_circle[128];
    char            x_tag_bg_arc[128];
    char            x_tag_arc[128];
    char            x_tag_center[128];
    char            x_tag_wiper[128];
    char            x_tag_wpr_c[128];
    char            x_tag_ticks[128];
    char            x_tag_outline[128];
    char            x_tag_in[128];
    char            x_tag_out[128];
    char            x_tag_sel[128];
};

// [else/messbox]
struct t_fake_messbox {
    t_object         x_obj;
    t_canvas        *x_canvas;
    t_glist         *x_glist;
    t_symbol        *x_bind_sym;
    void            *x_proxy;
    t_symbol        *x_dollzero;
    int              x_flag;
    int              x_height;
    int              x_width;
    int              x_resizing;
    int              x_active;
    int              x_selected;
    char             x_fgcolor[8];
    unsigned int     x_fg[3];    // fg RGB color
    char             x_bgcolor[8];
    unsigned int     x_bg[3];    // bg RGB color
    int              x_font_size;
    int              x_zoom;
    t_symbol        *x_font_weight;
    char            *tcl_namespace;
    char            *x_cv_id;
    char            *frame_id;
    char            *text_id;
//    char            *handle_id;
    char            *window_tag;
    char            *all_tag;
};

// [else/pad]
struct t_fake_pad {
    t_object        x_obj;
    t_glist        *x_glist;
    void            *x_proxy;
    t_symbol       *x_bindname;
    int             x_x;
    int             x_y;
    int             x_w;
    int             x_h;
    int             x_sel;
    int             x_zoom;
    int             x_edit;
    unsigned char   x_color[3];
};

// [else/note]
struct t_fake_note {
    t_object        x_obj;
    void           *x_proxy;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    t_binbuf       *x_binbuf;
    char           *x_buf;      // text buf
    int             x_bufsize;  // text buf size
    int             x_keynum;
    int             x_init;
    int             x_resized;
    int             x_changed;
    int             x_edit;
    int             x_max_pixwidth;
    int             x_text_width;
    int             x_width;
    int             x_height;
    int             x_bbset;
    int             x_bbpending;
    int             x_x1;
    int             x_y1;
    int             x_x2;
    int             x_y2;
    int             x_newx2;
    int             x_dragon;
    int             x_select;
    int             x_fontsize;
    int             x_shift;
    int             x_selstart;
    int             x_start_ndx;
    int             x_end_ndx;
    int             x_selend;
    int             x_active;
    unsigned char   x_red;
    unsigned char   x_green;
    unsigned char   x_blue;
    unsigned char   x_bg[3]; // background color
    char            x_color[8];
    char            x_bgcolor[8];
    t_symbol       *x_keysym;
    t_symbol       *x_bindsym;
    t_symbol       *x_fontname;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
//    int             x_old;
    int             x_text_flag;
    int             x_text_n;
    int             x_text_size;
    int             x_zoom;
    int             x_fontface;
    int             x_bold;
    int             x_italic;
    int             x_underline;
    int             x_bg_flag;
    int             x_textjust; // 0: left, 1: center, 2: right
    int             x_outline;
    t_pd           *x_handle;
};

#define MAX_NUMBOX_LEN 32

// [else/numbox~]
struct t_fake_numbox {
    t_object  x_obj;
    t_clock  *x_clock_update;
    t_symbol *x_fg;
    t_symbol *x_bg;
    t_glist  *x_glist;
    t_float   x_display;
    t_float   x_in_val;
    t_float   x_out_val;
    t_float   x_set_val;
    t_float   x_max;
    t_float   x_min;
    t_float   x_sr_khz;
    t_float   x_inc;
    t_float   x_ramp_step;
    t_float   x_ramp_val;
    int       x_ramp_ms;
    int       x_rate;
    int       x_numwidth;
    int       x_fontsize;
    int       x_clicked;
    int       x_width, x_height;
    int       x_zoom;
    int       x_outmode;
    char      x_buf[MAX_NUMBOX_LEN]; // number buffer
};

// [else/canvas.active]
struct t_fake_active {
    t_object x_obj;
    void* x_proxy;
    t_symbol* x_cname;
    int x_right_click;
    int x_on;
    int x_name;
};

// [else/canvas.mouse]
struct t_fake_canvas_mouse {
    t_object                x_obj;
    void                   *x_proxy;
    t_outlet               *x_outlet_x;
    t_outlet               *x_outlet_y;
    t_canvas               *x_canvas;
    int                     x_edit;
    int                     x_pos;
    int                     x_offset_x;
    int                     x_offset_y;
    int                     x_x;
    int                     x_y;
    int                     x_enable_edit_mode;
};

// [else/canvas.vis]
struct t_fake_canvas_vis {
    t_object x_obj;
    void* x_proxy;
    t_canvas* x_canvas;
};

// [else/canvas.zoom]
struct t_fake_zoom {
    t_object x_obj;
    void* x_proxy;
    t_canvas* x_canvas;
    int x_zoom;
};

// [else/canvas.edit]
struct t_fake_edit {
    t_object x_obj;
    void* x_proxy;
    t_canvas* x_canvas;
    int x_edit;
};

// [else/canvas.mouse]
struct t_fake_mouse {
t_object x_obj;
int x_hzero;
int x_vzero;
int x_zero;
int x_wx;
int x_wy;
t_glist* x_glist;
t_outlet* x_horizontal;
t_outlet* x_vertical;
};

// [else/pic]
struct t_fake_pic {
    t_object       x_obj;
    t_glist       *x_glist;
    void  *x_proxy;
    int            x_zoom;
    int            x_width;
    int            x_height;
    int            x_snd_set;
    int            x_rcv_set;
    int            x_edit;
    int            x_init;
    int            x_def_img;
    int            x_sel;
    int            x_outline;
    int            x_s_flag;
    int            x_r_flag;
    int            x_flag;
    int            x_size;
    int            x_latch;
    t_symbol      *x_fullname;
    t_symbol      *x_filename;
    t_symbol      *x_x;
    t_symbol      *x_receive;
    t_symbol      *x_rcv_raw;
    t_symbol      *x_send;
    t_symbol      *x_snd_raw;
    t_outlet      *x_outlet;
};


// for template drawing
struct t_fake_fielddesc {
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union {
        t_float fd_float;    /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1;             /* min and max values */
    float fd_v2;
    float fd_screen1;        /* min and max screen values */
    float fd_screen2;
    float fd_quantum;        /* quantization in value */
};

// for template drawing
struct t_fake_curve {
    t_object x_obj;
    int x_flags; /* CLOSED, BEZ, NOMOUSERUN, NOMOUSEEDIT */
    t_fake_fielddesc x_fillcolor;
    t_fake_fielddesc x_outlinecolor;
    t_fake_fielddesc x_width;
    t_fake_fielddesc x_vis;
    int x_npoints;
    t_fake_fielddesc* x_vec;
    t_canvas* x_canvas;
};

// for template drawing
struct t_fake_drawnumber {
    t_object x_obj;
    t_symbol* x_fieldname;
    t_fake_fielddesc x_xloc;
    t_fake_fielddesc x_yloc;
    t_fake_fielddesc x_color;
    t_fake_fielddesc x_vis;
    t_symbol* x_label;
    t_canvas* x_canvas;
};


#define SCOPE_MAXBUFSIZE 256

// [else/oscope~]
struct t_fake_oscope {
    t_object        x_obj;
    t_inlet        *x_rightinlet;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    void           *x_proxy;
    unsigned char   x_bg[3], x_fg[3], x_gg[3];
    float           x_xbuffer[SCOPE_MAXBUFSIZE*4];
    float           x_ybuffer[SCOPE_MAXBUFSIZE*4];
    float           x_xbuflast[SCOPE_MAXBUFSIZE*4];
    float           x_ybuflast[SCOPE_MAXBUFSIZE*4];
    float           x_min, x_max;
    float           x_trigx, x_triglevel;
    float           x_ksr;
    float           x_currx, x_curry;
    int             x_select;
    int             x_width, x_height;
    int             x_delay;
    int             x_trigmode;
    int             x_bufsize, x_lastbufsize;
    int             x_period;
    int             x_bufphase, x_precount, x_phase;
    int             x_xymode, x_frozen, x_retrigger;
    int             x_zoom;
    int             x_edit;
    t_float        *x_signalscalar;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    t_symbol       *x_bindsym;
    t_clock        *x_clock;
    t_pd           *x_handle;
};

// [cyclone/scope~]
struct t_fake_scope {
    t_object x_obj;
    t_inlet* x_rightinlet;
    t_glist* x_glist;
    t_canvas* x_cv;
    void* x_proxy;
    unsigned char x_bg[3], x_fg[3], x_gg[3];
    float x_xbuffer[SCOPE_MAXBUFSIZE * 4];
    float x_ybuffer[SCOPE_MAXBUFSIZE * 4];
    float x_xbuflast[SCOPE_MAXBUFSIZE * 4];
    float x_ybuflast[SCOPE_MAXBUFSIZE * 4];
    float x_min, x_max;
    float x_trigx, x_triglevel;
    float x_ksr;
    float x_currx, x_curry;
    int x_select;
    int x_width, x_height;
    int x_drawstyle;
    int x_delay;
    int x_trigmode;
    int x_bufsize, x_lastbufsize;
    int x_period;
    int x_bufphase, x_precount, x_phase;
    int x_xymode, x_frozen, x_retrigger;
    int x_zoom;
    int x_edit;
    t_float* x_signalscalar;
    int x_rcv_set;
    int x_flag;
    int x_r_flag;
    t_symbol* x_receive;
    t_symbol* x_rcv_raw;
    t_symbol* x_bindsym;
    t_clock* x_clock;
    t_pd* x_handle;
};

// For [text define]
struct t_fake_textbuf {
    t_object b_ob;
    t_binbuf* b_binbuf;
    t_canvas* b_canvas;
    t_guiconnect* b_guiconnect;
    t_symbol* b_sym;
};

// For [text define]
struct t_fake_qlist {
    t_fake_textbuf x_textbuf;
    t_outlet* x_bangout;
    int x_onset; /* playback position */
    t_clock* x_clock;
    t_float x_tempo;
    double x_whenclockset;
    t_float x_clockdelay;
    int x_rewound; /* we've been rewound since last start */
    int x_innext;  /* we're currently inside the "next" routine */
};

// [text define]
struct t_fake_text_define {
    t_fake_textbuf x_textbuf;
    t_outlet* x_out;
    t_outlet* x_notifyout;
    t_symbol* x_bindsym;
    t_scalar* x_scalar;   /* faux scalar (struct text-scalar) to point to */
    t_gpointer x_gp;      /* pointer to it */
    t_canvas* x_canvas;   /* owning canvas whose stub we use for x_gp */
    unsigned char x_keep; /* whether to embed contents in patch on save */
};


// class for connections
struct t_fake_outconnect {
    void* oc_next;
    t_pd* oc_to;
    t_symbol* outconnect_path_data;
};
