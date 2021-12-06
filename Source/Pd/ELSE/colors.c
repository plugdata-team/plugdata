
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <m_pd.h>
#include <m_imp.h>

static t_class *colors_class;

typedef struct _colors{
    t_object    x_obj;
    t_int       x_hex;
    t_int       x_gui;
    t_int       x_rgb;
    t_int       x_ds;
    t_symbol   *x_id;
    char        x_color[MAXPDSTRING];
}t_colors;

static void colors_pick(t_colors *x){
    sys_vgui("after idle [list after 10 else_colors_pick %s %s]\n", x->x_id->s_name, x->x_color);
}

static void colors_bang(t_colors *x){
    if(x->x_hex)
        outlet_symbol(x->x_obj.ob_outlet, gensym(x->x_color));
    else{
        unsigned int red, green, blue;
        sscanf(x->x_color, "#%02x%02x%02x", &red, &green, &blue);
        if(x->x_rgb){
            t_atom at[3];
            SETFLOAT(at, red);
            SETFLOAT(at+1, green);
            SETFLOAT(at+2, blue);
            outlet_list(x->x_obj.ob_outlet, &s_list, 3, at);
        }
        else if(x->x_gui)
            outlet_float(x->x_obj.ob_outlet, -(float)(red*65536 + green*256 + blue) - 1);
        else if(x->x_ds){
            t_float ds = 100*(int)((float)red*8/255 + 0.5)
                        + 10*(int)((float)green*8/255 + 0.5)
                        + (int)((float)blue*8/255 + 0.5);
            outlet_float(x->x_obj.ob_outlet, ds);
            
            red = (int)(ds/100);
            if(red > 8)
                red = 8;
            red = (int)((float)red * 255./8. + 0.5);
            
            green = (int)(fmod(ds, 100) / 10);
            if(green > 8)
                green = 8;
            green = (int)((float)green * 255./8. + 0.5);
            
            blue = (int)(fmod(ds, 10));
            if(blue > 8)
                blue = 8;
            blue = (int)((float)blue * 255./8. + 0.5);
            
            char hex[MAXPDSTRING];
            sprintf(hex, "#%02x%02x%02x", red, green, blue);
            strncpy(x->x_color, hex, 7);
        }
    }
}

static void colors_convert_to(t_colors *x, t_symbol *s){
    if(s == gensym("rgb")){
        x->x_ds = x->x_gui = x->x_hex = 0;
        x->x_rgb = 1;
    }
    else if(s == gensym("hex")){
        x->x_ds = x->x_gui = x->x_rgb = 0;
        x->x_hex = 1;
    }
    else if(s == gensym("ds")){
        x->x_hex = x->x_gui = x->x_rgb = 0;
        x->x_ds = 1;
    }
    else if(s == gensym("iemgui")){
        x->x_hex = x->x_ds = x->x_rgb = 0;
        x->x_gui = 1;
    }
    else
        post("[colors]: can't convert to %s", s);
}

static void colors_hex(t_colors *x, t_symbol *s){
    strncpy(x->x_color, s->s_name, 7);
    colors_bang(x);
}

static void colors_ds(t_colors *x, t_floatarg ds){
    t_float red = (int)(ds/100);
    if(red > 8)
        red = 8;
    unsigned char r = (int)(red * 255./8. + 0.5);
    
    t_float green = (int)(fmod(ds, 100) / 10);
    if(green > 8)
        green = 8;
    unsigned char g = (int)(green * 255./8. + 0.5);
    
    t_float blue = (int)(fmod(ds, 10));
    if(blue > 8)
        blue = 8;
    unsigned char b = (int)(blue * 255./8. + 0.5);
    
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", r, g, b);
    strncpy(x->x_color, hex, 7);
    
    colors_bang(x);
}

static void colors_gui(t_colors *x, t_floatarg gui){
    gui = -(gui + 1);
    unsigned int red = (unsigned int)(gui / 65536);
    unsigned int green = (unsigned int)(fmod(gui, 65536) / 255);
    unsigned int blue = (unsigned int)(fmod(gui, 256));
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", red, green, blue);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

static void colors_gray(t_colors *x, t_floatarg g){
    float gray = (g > 100 ? 1 : g < 0 ? 0 : g/100.);
    unsigned int rgb = (unsigned int)(rint(255 * gray));
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", rgb, rgb, rgb);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

static void colors_cmyk(t_colors *x, t_floatarg c, t_floatarg m, t_floatarg y, t_floatarg k){
    float oneminusc = 1 - (c > 100 ? 1 : c < 0 ? 0 : c/100.);
    float oneminusm = 1 - (m > 100 ? 1 : m < 0 ? 0 : m/100.);
    float oneminusy = 1 - (y > 100 ? 1 : y < 0 ? 0 : y/100.);
    float oneminusk = 1 - (k > 100 ? 1 : k < 0 ? 0 : k/100.);
    unsigned int red = (unsigned int)(rint(255 * oneminusc * oneminusk));
    unsigned int green = (unsigned int)(rint(255 * oneminusm * oneminusk));
    unsigned int blue = (unsigned int)(rint(255 * oneminusy * oneminusk));
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", red, green, blue);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

static void colors_rgb(t_colors *x, t_floatarg r, t_floatarg g, t_floatarg b){
    unsigned int red = (unsigned int)(r > 255 ? 255 : r < 0 ? 0 : rint(r));
    unsigned int green = (unsigned int)(g > 255 ? 255 : g < 0 ? 0 : rint(g));
    unsigned int blue = (unsigned int)(b > 255 ? 255 : b < 0 ? 0 : rint(b));
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", red, green, blue);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

static void colors_hsv(t_colors *x, t_floatarg H, t_floatarg S, t_floatarg V){
    if(H > 360)
        H = 360;
    if(H < 0)
        H = 0;
    float s = S > 100 ? 1 : S < 0 ? 0 : S/100.;
    float v = V > 100 ? 1 : V < 0 ? 0 : V/100.;
    float C = s*v;
    float X = C*(1-fabs(fmod(H/60.0, 2)-1));
    float m = v-C;
    float r,g,b;
    if(H >= 0 && H < 60)
        r = C, g = X, b = 0;
    else if(H >= 60 && H < 120)
        r = X, g = C, b = 0;
    else if(H >= 120 && H < 180)
        r = 0, g = C, b = X;
    else if(H >= 180 && H < 240)
        r = 0, g = X, b = C;
    else if(H >= 240 && H < 300)
        r = X, g = 0, b = C;
    else
        r = C, g = 0, b = X;
    int R = rint((r+m)*255);
    int G = rint((g+m)*255);
    int B = rint((b+m)*255);
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", R, G, B);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

float HueToRGB(float v1, float v2, float vH){
    if(vH < 0)
        vH += 1;
    if(vH > 1)
        vH -= 1;
    if((6 * vH) < 1)
        return (v1 + (v2 - v1) * 6 * vH);
    if((2 * vH) < 1)
        return(v2);
    if((3 * vH) < 2)
        return(v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);
    return(v1);
}

static void colors_hsl(t_colors *x, t_floatarg H, t_floatarg S, t_floatarg L){
    unsigned char R = 0, G = 0, B = 0;
    if(H > 360)
        H = 360;
    if(H < 0)
        H = 0;
    L = L > 100 ? 1 : L < 0 ? 0 : L/100.;
    S = S > 100 ? 1 : S < 0 ? 0 : S/100.;
    if(S == 0)
        R = G = B = (unsigned char)(L * 255);
    else{
        float v1, v2;
        float hue = (float)H / 360;
        v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
        v1 = 2 * L - v2;
        R = (unsigned char)rint((255 * HueToRGB(v1, v2, hue + (1.0f / 3))));
        G = (unsigned char)rint((255 * HueToRGB(v1, v2, hue)));
        B = (unsigned char)rint((255 * HueToRGB(v1, v2, hue - (1.0f / 3))));
    }
    char hex[MAXPDSTRING];
    sprintf(hex, "#%02x%02x%02x", R, G, B);
    strncpy(x->x_color, hex, 7);
    colors_bang(x);
}

static void colors_picked_color(t_colors *x, t_symbol *color){
    if(color != &s_){
        strncpy(x->x_color, color->s_name, MAXPDSTRING);
        colors_bang(x);
    }
}

static void colors_free(t_colors *x){
    pd_unbind(&x->x_obj.ob_pd, x->x_id);
}

static void *colors_new(t_symbol *s){
    t_colors *x = (t_colors *)pd_new(colors_class);
    x->x_hex = x->x_gui = x->x_ds = x->x_rgb = 0;
    char buf[MAXPDSTRING];
    sprintf(buf, "#%lx", (t_int)x);
    x->x_id = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->x_id);
    outlet_new(&x->x_obj, &s_list);
    strcpy(x->x_color,"#ffffff"); // initial color
    if(s == gensym("-rgb"))
        x->x_rgb = 1;
    else if(s == gensym("-iemgui"))
        x->x_gui = 1;
    else if(s == gensym("-ds"))
        x->x_ds = 1;
    else
        x->x_hex = 1; // default
    return(x);
}

void colors_setup(void){
    colors_class = class_new(gensym("colors"), (t_newmethod)colors_new,
                (t_method)colors_free, sizeof(t_colors), 0, A_DEFSYMBOL, 0);
    class_addbang(colors_class, (t_method)colors_bang);
    class_addmethod(colors_class, (t_method)colors_hsl, gensym("hsl"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_hsv, gensym("hsv"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_rgb, gensym("rgb"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_gray, gensym("gray"), A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_cmyk, gensym("cmyk"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_pick, gensym("pick"), 0);
    class_addmethod(colors_class, (t_method)colors_convert_to, gensym("to"), A_DEFSYMBOL, 0);
    class_addmethod(colors_class, (t_method)colors_hex, gensym("hex"), A_DEFSYMBOL, 0);
    class_addmethod(colors_class, (t_method)colors_gui, gensym("iemgui"), A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_ds, gensym("ds"), A_DEFFLOAT, 0);
    class_addmethod(colors_class, (t_method)colors_picked_color, gensym("picked_color"), A_DEFSYMBOL, 0);
    class_addmethod(colors_class, (t_method)colors_pick, gensym("click"), 0);
    sys_vgui("proc else_colors_pick {obj_id color} {\n");
    sys_vgui("set color [tk_chooseColor -initialcolor $color]\n");
    sys_vgui("  pdsend [concat $obj_id picked_color $color]\n");
    sys_vgui("}\n");
}
