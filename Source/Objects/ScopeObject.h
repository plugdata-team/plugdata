/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#define SCOPE_MINSIZE       18
#define SCOPE_MINPERIOD     2
#define SCOPE_MAXPERIOD     8192
#define SCOPE_MINBUFSIZE    8
#define SCOPE_MAXBUFSIZE    256
#define SCOPE_MINDELAY      0
#define SCOPE_SELCOLOR     "#5aadef" // select color from max
#define SCOPE_SELBDWIDTH    3
#define HANDLE_SIZE         12
#define SCOPE_GUICHUNK      128 // performance-related hacks, LATER investigate


struct t_fake_scope {
    t_object        x_obj;
    t_inlet*        x_rightinlet;
    t_glist*        x_glist;
    t_canvas*       x_cv;
    void*           x_proxy;
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
    int             x_drawstyle;
    int             x_delay;
    int             x_trigmode;
    int             x_bufsize, x_lastbufsize;
    int             x_period;
    int             x_bufphase, x_precount, x_phase;
    int             x_xymode, x_frozen, x_retrigger;
    int             x_zoom;
    int             x_edit;
    t_float*        x_signalscalar;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
    t_symbol*       x_receive;
    t_symbol*       x_rcv_raw;
    t_symbol*       x_bindsym;
    t_clock*        x_clock;
    void*           x_handle;
};

struct ScopeObject final : public GUIObject, public Timer {
    
    std::vector<float> x_buffer;
    std::vector<float> y_buffer;
    
    Value gridColour, triggerMode, triggerValue, samplesPerPoint, bufferSize, delay, receiveSymbol, signalRange;
    
    ScopeObject(void* ptr, Object* object)
        : GUIObject(ptr, object)
    {
        startTimerHz(25);
        
        auto* scope = static_cast<t_fake_scope*>(ptr);
        triggerMode = scope->x_trigmode + 1;
        triggerValue = scope->x_triglevel;
        bufferSize = scope->x_bufsize;
        delay = scope->x_delay;
        samplesPerPoint = scope->x_period;
        secondaryColour = colourFromHexArray(scope->x_bg).toString();
        primaryColour = colourFromHexArray(scope->x_fg).toString();
        gridColour = colourFromHexArray(scope->x_gg).toString();
        
        auto rcv = String(scope->x_rcv_raw->s_name);
        if(rcv == "empty") rcv = "";
        receiveSymbol = rcv;
        
        Array<var> arr = { scope->x_min, scope->x_max };
        signalRange = var(arr);
        
        bufferSize.addListener(this);
        samplesPerPoint.addListener(this);
        secondaryColour.addListener(this);
        primaryColour.addListener(this);
        gridColour.addListener(this);
        signalRange.addListener(this);
        delay.addListener(this);
        triggerMode.addListener(this);
        triggerValue.addListener(this);
    }
    
    Colour colourFromHexArray(unsigned char* hex) {
        return Colour(hex[0], hex[1], hex[2]);
    }
    void colourToHexArray(Colour colour, unsigned char* hex) {
        hex[0] = colour.getRed();
        hex[1] = colour.getGreen();
        hex[2] = colour.getBlue();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        pd->getCallbackLock()->exit();

        object->setObjectBounds({x, y, w, h});
    }

    void resized() override
    {
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(20, maxSize, object->getWidth());
        int h = jlimit(20, maxSize, object->getHeight());

        if (w != object->getWidth() || h != object->getHeight()) {
            object->setSize(w, h);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
        
        Colour outlineColour;

        if(cnv->isSelected(object) && !cnv->isGraph) {
            outlineColour = object->findColour(PlugDataColour::canvasActiveColourId);
        }
        else {
            outlineColour = object->findColour(static_cast<bool>(object->locked.getValue()) ? PlugDataColour::canvasLockedOutlineColourId : PlugDataColour::canvasUnlockedOutlineColourId);
        }
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
                
        auto dx = getWidth() * 0.125f;
        auto dy = getHeight() * 0.25f;
        
        g.setColour(Colour::fromString(gridColour.toString()));
        
        float xx;
        for(int i = 0, xx = dx; i < 7; i++, xx += dx) {
            g.drawLine(xx, 0, xx, getHeight());
        }
            
        float yy;
        for(int i = 0, yy = dy; i < 3; i++, yy += dy) {
            g.drawLine(0, yy, getWidth(), yy);
        }
        
        if(y_buffer.empty() || x_buffer.empty()) return;
        
        Point<float> lastPoint = Point<float>(x_buffer[0], y_buffer[0]);
        Point<float> newPoint;

        g.setColour(Colour::fromString(primaryColour.toString()));
        
        Path p;
        for (size_t i = 1; i < y_buffer.size(); i++) {
            newPoint = Point<float>(x_buffer[i], y_buffer[i]);
            g.drawLine({lastPoint, newPoint});
            lastPoint = newPoint;
        }
        
    }
    
    // Push current object bounds into pd
    void applyBounds() override {
        
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
        
        static_cast<t_fake_scope*>(ptr)->x_width = getWidth();
        static_cast<t_fake_scope*>(ptr)->x_height = getHeight();
    }
    
    void timerCallback() override
    {
        int bufsize, mode;
        float min, max;
        
        if(object->iolets.size() == 3) object->iolets[2]->setVisible(false);

        if(pd->getCallbackLock()->tryEnter())
        {
            auto* x = static_cast<t_fake_scope*>(ptr);
            bufsize = x->x_bufsize;
            min = x->x_min;
            max = x->x_max;
            mode = x->x_xymode;
            
            if(x_buffer.size() != bufsize) {
                x_buffer.resize(bufsize);
                y_buffer.resize(bufsize);
            }
            
            std::copy(x->x_xbuflast, x->x_xbuflast + bufsize, x_buffer.data());
            std::copy(x->x_ybuflast, x->x_ybuflast + bufsize, y_buffer.data());
            pd->getCallbackLock()->exit();
        }
        else {
            return;
        }
        
        if(min > max) {
            auto temp = max;
            max = min;
            min = temp;
        }

        float oldx = 0, oldy = 0;
        float dx = getWidth() / (float)bufsize;
        float dy = getHeight() / (float)bufsize;

        for(int n = 0; n < bufsize; n++) {
            if(mode == 1) {
                y_buffer[n] = jmap<float>(x_buffer[n], min, max, getHeight(), 0);
                x_buffer[n] = oldx;
                oldx += dx;
            }
            else if(mode == 2) {
                x_buffer[n] = jmap<float>(y_buffer[n], min, max, 0, getWidth());
                y_buffer[n] = oldy;
                oldy += dy;
            }
            else if(mode == 3) {
                x_buffer[n] = jmap<float>(x_buffer[n], min, max, 0, getWidth());
                y_buffer[n] = jmap<float>(y_buffer[n], min, max, getHeight(), 0);
            }
        }
        
        repaint();
    }

    void updateValue() override {};
    
    void valueChanged(Value& v) override
    {
        auto* scope = static_cast<t_fake_scope*>(ptr);
        if(v.refersToSameSourceAs(primaryColour)) {
            colourToHexArray(Colour::fromString(primaryColour.toString()), scope->x_fg);
        }
        else if(v.refersToSameSourceAs(secondaryColour)) {
            colourToHexArray(Colour::fromString(secondaryColour.toString()), scope->x_bg);
        }
        else if(v.refersToSameSourceAs(gridColour)) {
            colourToHexArray(Colour::fromString(gridColour.toString()), scope->x_gg);
        }
        else if(v.refersToSameSourceAs(bufferSize)) {
            scope->x_bufsize = static_cast<int>(bufferSize.getValue());
        }
        else if(v.refersToSameSourceAs(samplesPerPoint)) {
            scope->x_period = static_cast<int>(samplesPerPoint.getValue());
        }
        else if (v.refersToSameSourceAs(signalRange)) {
            auto min = static_cast<float>(signalRange.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(signalRange.getValue().getArray()->getReference(1));
            scope->x_min = min;
            scope->x_max = max;
        }
        else if(v.refersToSameSourceAs(delay)) {
            scope->x_delay = static_cast<int>(delay.getValue());
        }
        else if(v.refersToSameSourceAs(triggerMode)) {
            scope->x_trigmode = static_cast<int>(triggerMode.getValue()) - 1;
        }
        else if(v.refersToSameSourceAs(triggerValue)) {
            scope->x_triglevel = static_cast<int>(triggerValue.getValue());
        }
        else if(v.refersToSameSourceAs(receiveSymbol)) {
            auto* rcv = gensym(receiveSymbol.toString().toRawUTF8());
            scope->x_receive = canvas_realizedollar(scope->x_glist, scope->x_rcv_raw = rcv);
            if(scope->x_receive != &s_) {
                pd_bind(&scope->x_obj.ob_pd, scope->x_receive);
            }
            else {
                scope->x_rcv_raw = gensym("empty");
            }
        }
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        
        params.push_back({ "Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({ "Foreground", tColour, cAppearance, &primaryColour, {}});
        params.push_back({ "Grid", tColour, cAppearance, &gridColour, {}});
        
        params.push_back({ "Trigger mode", tCombo, cGeneral, &triggerMode, {"None", "Up", "Down"}});
        params.push_back({ "Trigger value", tFloat, cGeneral, &triggerValue, {}});
        
        params.push_back({ "Samples per point", tInt, cGeneral, &samplesPerPoint, {}});
        params.push_back({ "Buffer size", tInt, cGeneral, &bufferSize, {}});
        params.push_back({ "Delay", tInt, cGeneral, &delay, {}});
        
        params.push_back({ "Signal range", tRange, cGeneral, &signalRange, {}});

        params.push_back({ "Receive symbol", tString, cGeneral, &receiveSymbol, {}});

        return params;
    }
};
