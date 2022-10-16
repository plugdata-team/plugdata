
struct t_fake_function
{
    t_object        x_obj;
    t_glist*        x_glist;
    void*           x_proxy;
    int             x_state;
    int             x_n_states;
    int             x_flag;
    int             x_s_flag;
    int             x_r_flag;
    int             x_sel;
    int             x_width;
    int             x_height;
    int             x_init;
    int             x_numdots;
    int             x_grabbed;      // for moving points
    int             x_shift;
    int             x_snd_set;
    int             x_rcv_set;
    int             x_zoom;
    int             x_edit;
    t_symbol*       x_send;
    t_symbol*       x_receive;
    t_symbol*       x_snd_raw;
    t_symbol*       x_rcv_raw;
    float*          x_points;
    float*          x_dur;
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

struct FunctionObject final : public GUIObject {
    
    
    int dragIdx = -1;
    bool newPointAdded = false;
    
    Value range;
    
    FunctionObject(void* ptr, Object* object)
    : GUIObject(ptr, object)
    {
        auto* function = static_cast<t_fake_function*>(ptr);
        secondaryColour = colourFromHexArray(function->x_bgcolor).toString();
        primaryColour = colourFromHexArray(function->x_fgcolor).toString();
        
        Array<var> arr = { function->x_min, function->x_max };
        range = var(arr);
    }
    
    //std::pair<float, float> range;
    Array<Point<float>> points;
    
    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* function = static_cast<t_fake_function*>(ptr);
        function->x_width = b.getWidth();
        function->x_height = b.getHeight();
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
        static_cast<t_my_canvas*>(ptr)->x_vis_w = getWidth();
        static_cast<t_my_canvas*>(ptr)->x_vis_h = getHeight();
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
    
    Array<Point<float>> getRealPoints() {
        auto realPoints = Array<Point<float>>();
        for(auto point : points) {
            point.x = jmap<float>(point.x, 0.0f, 1.0f, 3, getWidth() - 3);
            point.y = jmap<float>(point.y, 0.0f, 1.0f, getHeight() - 3, 3);
            realPoints.add(point);
        }
        
        return realPoints;
    }
    
    void setRealPoints(const Array<Point<float>>& points) {
        auto* function = static_cast<t_fake_function*>(ptr);
        for(int i = 0; i < points.size(); i++) {
            function->x_points[i] = jmap<float>(points[i].y, getHeight() - 3, 3, 1.0f, 0.0f);
            function->x_dur[i] = jmap<float>(points[i].x, 3, getWidth() - 3, 0.0f, 1.0f);
        }
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
        
        auto outlineColour = object->findColour(cnv->isSelected(object) && !cnv->isGraph ? PlugDataColour::canvasActiveColourId : PlugDataColour::outlineColourId);
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
                
        g.setColour(Colour::fromString(primaryColour.toString()));
        
        auto realPoints = getRealPoints();
        auto lastPoint = realPoints[0];
        for(int i = 1; i < realPoints.size(); i++) {
            auto newPoint = realPoints[i];
            g.drawLine({lastPoint, newPoint});
            lastPoint = newPoint;
        }
        
        for(const auto& point : realPoints) {
            // Make sure line isn't visible through the hole
            g.setColour(Colour::fromString(secondaryColour.toString()));
            g.fillEllipse(Rectangle<float>().withCentre(point).withSizeKeepingCentre(5, 5));
            
            g.setColour(Colour::fromString(primaryColour.toString()));
            g.drawEllipse(Rectangle<float>().withCentre(point).withSizeKeepingCentre(5, 5), 1.5f);
        }
    }
    
    void updateValue() override {
        
        // Don't update while dragging
        if(dragIdx != -1) return;
        
        points.clear();
        auto* function = static_cast<t_fake_function*>(ptr);
        
        setRange({function->x_min, function->x_max});
        
        for(int i = 0; i < function->x_numdots; i++) {
            auto x = function->x_dur[i] / function->x_dur[function->x_n_states];
            auto y = jmap(function->x_points[i], function->x_min, function->x_max, 0.0f, 1.0f);
            points.add({x, y});
        }
        
        repaint();
    };
    
    static int compareElements(Point<float> a, Point<float> b)
    {
        if (a.x < b.x)
            return -1;
        else if (a.x > b.x)
            return 1;
        else
            return 0;
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        auto realPoints = getRealPoints();
        for(int i = 0; i < realPoints.size(); i++) {
            auto clickBounds = Rectangle<float>().withCentre(realPoints[i]).withSizeKeepingCentre(7, 7);
            if(clickBounds.contains(e.x, e.y)) {
                dragIdx = i;
                return;
            }
        }
        
        float newX = jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f);
        float newY = jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f);
        
        dragIdx = points.addSorted(*this, {newX, newY});
    }
    
    std::pair<float, float> getRange()
    {
        auto& arr = *range.getValue().getArray();
        return {static_cast<float>(arr[0]), static_cast<float>(arr[1])};
    }
    void setRange(std::pair<float, float> newRange)
    {
        auto& arr = *range.getValue().getArray();
        arr[0] = newRange.first;
        arr[1] = newRange.second;
        
        auto* function = static_cast<t_fake_function*>(ptr);
        function->x_min = newRange.first;
        function->x_max = newRange.second;
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        auto [min, max] = getRange();
        bool changed = false;
        
        // For first and last point, only adjust y position
        if(dragIdx == 0 || dragIdx == points.size() - 1)  {
            float newY = jlimit(min, max, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));
            if(newY != points.getReference(dragIdx).y) {
                points.getReference(dragIdx).y = newY;
                changed = true;
            }
            
        }
        
        else if(dragIdx > 0) {
            float minX = points[dragIdx - 1].x;
            float maxX = points[dragIdx + 1].x;
            
            float newX = jlimit(minX, maxX, jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f));
            
            float newY = jlimit(min, max, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));
            
            auto newPoint = Point<float>(newX, newY);
            if(points[dragIdx] != newPoint) {
                points.set(dragIdx, newPoint);
                changed = true;
            }
        }
        
        repaint();
        if(changed) triggerOutput();
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if(dragIdx < 0) return;
        
        auto* function = static_cast<t_fake_function*>(ptr);
        points.sort(*this);
        
        auto scale = function->x_dur[function->x_n_states];
        
        for(int i = 0; i < points.size(); i++) {
            function->x_points[i] = jmap(points[i].y, 0.0f, 1.0f, function->x_min, function->x_max);
            function->x_dur[i] = points[i].x * scale;
        }
        
        function->x_n_states = points.size() - 1;
        function->x_numdots = points.size();
        
        dragIdx = -1;
        
        updateValue();
    }
    
    void triggerOutput() {

        auto* x = static_cast<t_fake_function*>(ptr);
        int ac = points.size() * 2 + 1;
    
        auto scale = x->x_dur[x->x_n_states];
        
        auto at = std::vector<t_atom>(ac);
        float firstPoint = jmap<float>(points[0].y, 0.0f, 1.0f, x->x_min, x->x_max);
        SETFLOAT(at.data(), firstPoint); // get 1st
        
        x->x_state = 0;
        for(int i = 1; i < ac; i++){ // get the rest
            
            float dur = jmap<float>(points[x->x_state + 1].x - points[x->x_state].x, 0.0f, 1.0f, 0.0f, scale);
            
            SETFLOAT(at.data()+i, dur); // duration
            i++, x->x_state++;
            float point = jmap<float>(points[x->x_state].y, 0.0f, 1.0f, x->x_min, x->x_max);
            if(point < x->x_min_point)
                x->x_min_point = point;
            if(point > x->x_max_point)
                x->x_max_point = point;
            SETFLOAT(at.data()+i, point);
        }

        pd->enqueueFunction([x, at, ac]() mutable {
            outlet_list(x->x_obj.ob_outlet, &s_list, ac - 2, at.data());
            if(x->x_send != &s_ && x->x_send->s_thing)
                pd_list(x->x_send->s_thing, &s_list, ac - 2, at.data());
        });
    }
    
    void valueChanged(Value& v) override
    {
        auto* function = static_cast<t_fake_function*>(ptr);
        if(v.refersToSameSourceAs(primaryColour)) {
            colourToHexArray(Colour::fromString(primaryColour.toString()), function->x_fgcolor);
            repaint();
        }
        else if(v.refersToSameSourceAs(secondaryColour)) {
            colourToHexArray(Colour::fromString(secondaryColour.toString()), function->x_bgcolor);
            repaint();
        }
        else if(v.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            function->x_snd_raw = gensym(symbol.toRawUTF8());
            function->x_send = canvas_realizedollar(function->x_glist, function->x_snd_raw);
        }
        else if(v.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            function->x_rcv_raw = gensym(symbol.toRawUTF8());
            function->x_receive = canvas_realizedollar(function->x_glist, function->x_rcv_raw);
        }
        else if(v.refersToSameSourceAs(range)) {
            setRange(getRange());
            updateValue();
        }
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        params.push_back({ "Foreground", tColour, cAppearance, &primaryColour, {} });
        params.push_back({ "Background", tColour, cAppearance, &secondaryColour, {} });
        params.push_back({ "Range", tRange, cGeneral, &range, {} });
        params.push_back({ "Send Symbol", tString, cGeneral, &sendSymbol, {} });
        params.push_back({ "Receive Symbol", tString, cGeneral, &receiveSymbol, {} });

        return params;
    }
    
    Colour colourFromHexArray(unsigned char* hex) {
        return Colour(hex[0], hex[1], hex[2]);
    }
    void colourToHexArray(Colour colour, unsigned char* hex) {
        hex[0] = colour.getRed();
        hex[1] = colour.getGreen();
        hex[2] = colour.getBlue();
    }
};
