/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/GlobalMouseListener.h"

// Else "mouse" component
class MouseObject final : public TextBase
    , public Timer {
    typedef struct _mouse {
        t_object x_obj;
        int x_hzero;
        int x_vzero;
        int x_zero;
        int x_wx;
        int x_wy;
        t_glist* x_glist;
        t_outlet* x_horizontal;
        t_outlet* x_vertical;
    } t_mouse;

public:
    MouseObject(void* ptr, Object* object)
        : TextBase(ptr, object)
        , mouseSource(Desktop::getInstance().getMainMouseSource())
    {
        lastPosition = mouseSource.getScreenPosition();
        lastMouseDownTime = mouseSource.getLastMouseDownTime();
        startTimer(timerInterval);
    }

    void timerCallback() override
    {
        if (lastPosition != mouseSource.getScreenPosition()) {
            auto pos = mouseSource.getScreenPosition();

            t_atom args[2];
            SETFLOAT(args, pos.x);
            SETFLOAT(args + 1, pos.y);

            pd_typedmess((t_pd*)(this->ptr), pd->generateSymbol("_getscreen"), 2, args);

            lastPosition = pos;
        }
        if (mouseSource.isDragging()) {
            if (!isDown) {
                t_atom args[1];
                SETFLOAT(args, 0);

                pd_typedmess((t_pd*)(this->ptr), pd->generateSymbol("_up"), 1, args);
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (mouseSource.getLastMouseDownTime() > lastMouseDownTime) {
            if (!isDown) {
                t_atom args[1];
                SETFLOAT(args, 0);

                pd_typedmess((t_pd*)(this->ptr), pd->generateSymbol("_up"), 1, args);
            }
            isDown = true;
            lastMouseDownTime = mouseSource.getLastMouseDownTime();
        } else if (isDown) {
            t_atom args[1];
            SETFLOAT(args, 1);

            pd_typedmess((t_pd*)(this->ptr), pd->generateSymbol("_up"), 1, args);

            isDown = false;
        }
    }

    MouseInputSource mouseSource;

    Time lastMouseDownTime;
    Point<float> lastPosition;
    bool isDown = false;
    int const timerInterval = 30;
};
