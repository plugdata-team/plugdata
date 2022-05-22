/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

extern "C"
{
#include <m_pd.h>
#include <g_canvas.h>
#include <m_imp.h>
#include <s_stuff.h>
}

#include "GUIObject.h"

class Canvas;
struct _fielddesc
{
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union
    {
        t_float fd_float;    /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1; /* min and max values */
    float fd_v2;
    float fd_screen1; /* min and max screen values */
    float fd_screen2;
    float fd_quantum; /* quantization in value */
};

struct t_curve;
struct DrawableTemplate : public DrawablePath
{
    t_scalar* scalar;
    t_curve* object;
    int baseX, baseY;
    Canvas* canvas;

    Rectangle<int> lastBounds;

    DrawableTemplate(t_scalar* s, t_gobj* obj, Canvas* cnv, int x, int y);

    void update();

    void updateIfMoved();
};

struct ScalarObject : public GUIObject
{
    ScalarObject(void* obj, Box* box) : GUIObject(obj, box)
    {
    }

    void updateBounds() override
    {
    }
};
