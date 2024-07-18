/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class FunctionObject final : public ObjectBase {

    int hoverIdx = -1;
    int dragIdx = -1;

    Value initialise = SynchronousValue();
    Value range = SynchronousValue();
    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    Array<Point<float>> points;

public:
    FunctionObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamRange("Range", cGeneral, &range, { 0.0f, 1.0f });
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
        objectParameters.addParamBool("Initialise", cGeneral, &initialise, { "No", "Yes" }, 0);
    }

    void update() override
    {
        if (auto function = ptr.get<t_fake_function>()) {
            secondaryColour = colourFromHexArray(function->x_bgcolor).toString();
            primaryColour = colourFromHexArray(function->x_fgcolor).toString();
            sizeProperty = Array<var> { var(function->x_width), var(function->x_height) };
            initialise = function->x_init;

            Array<var> arr = { function->x_min, function->x_max };
            range = var(arr);

            auto sndSym = function->x_snd_set ? String::fromUTF8(function->x_snd_raw->s_name) : getBinbufSymbol(3);
            auto rcvSym = function->x_rcv_set ? String::fromUTF8(function->x_rcv_raw->s_name) : getBinbufSymbol(4);

            sendSymbol = sndSym != "empty" ? sndSym : "";
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            getPointsFromFunction(function.get());
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto function = ptr.get<t_fake_function>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, function.cast<t_gobj>(), b.getX(), b.getY());
            function->x_width = b.getWidth() - 1;
            function->x_height = b.getHeight() - 1;
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto function = ptr.get<t_fake_function>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(function->x_width), var(function->x_height) });
        }
    }

    Array<Point<float>> getRealPoints()
    {
        auto realPoints = Array<Point<float>>();
        for (auto point : points) {
            point.x = jmap<float>(point.x, 0.0f, 1.0f, 3, getWidth() - 3);
            point.y = jmap<float>(point.y, 0.0f, 1.0f, getHeight() - 3, 3);
            realPoints.add(point);
        }

        return realPoints;
    }

    void render(NVGcontext* nvg) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        bool editing = cnv->locked == var(true) || cnv->presentationMode == var(true) || ModifierKeys::getCurrentModifiers().isCtrlDown();

        auto b = getLocalBounds().toFloat();
        auto backgroundColour = convertColour(Colour::fromString(secondaryColour.toString()));
        auto foregroundColour = convertColour(Colour::fromString(primaryColour.toString()));
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, selected ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        nvgStrokeColor(nvg, foregroundColour);

        auto realPoints = getRealPoints();
        auto lastPoint = realPoints[0];
        for (int i = 1; i < realPoints.size(); i++) {
            auto newPoint = realPoints[i];
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, lastPoint.getX(), lastPoint.getY());
            nvgLineTo(nvg, newPoint.getX(), newPoint.getY());
            nvgStroke(nvg);
            lastPoint = newPoint;
        }

        for (int i = 0; i < realPoints.size(); i++) {
            auto point = realPoints[i];
            // Make sure line isn't visible through the hole
            nvgBeginPath(nvg);
            nvgFillColor(nvg, backgroundColour);
            nvgCircle(nvg, point.getX(), point.getY(), 2.5f);
            nvgFill(nvg);

            nvgStrokeColor(nvg, hoverIdx == i && editing ? outlineColour : foregroundColour);
            nvgBeginPath(nvg);
            nvgCircle(nvg, point.getX(), point.getY(), 2.5f);
            nvgStrokeWidth(nvg, 1.5f);
            nvgStroke(nvg);
        }
    }

    void getPointsFromFunction(t_fake_function* function)
    {
        // Don't update while dragging
        if (dragIdx != -1)
            return;

        points.clear();

        setRange({ function->x_min, function->x_max });

        for (int i = 0; i < function->x_n_states + 1; i++) {
            auto x = function->x_dur[i] / function->x_dur[function->x_n_states];
            auto y = jmap(function->x_points[i], function->x_min, function->x_max, 0.0f, 1.0f);
            points.add({ x, y });
        }

        repaint();
    }

    static int compareElements(Point<float> a, Point<float> b)
    {
        if (a.x < b.x)
            return -1;
        else if (a.x > b.x)
            return 1;
        else
            return 0;
    }

    void setHoverIdx(int i)
    {
        hoverIdx = i;
        repaint();
    }

    void resetHoverIdx()
    {
        setHoverIdx(-1);
    }

    bool hitTest(int x, int y) override
    {
        resetHoverIdx();
        auto realPoints = getRealPoints();
        for (int i = 0; i < realPoints.size(); i++) {
            auto hoverBounds = Rectangle<float>().withCentre(realPoints[i]).withSizeKeepingCentre(7, 7);
            if (hoverBounds.contains(x, y)) {
                setHoverIdx(i);
            }
        }
        return ObjectBase::hitTest(x, y);
    }

    void mouseExit(MouseEvent const& e) override
    {
        resetHoverIdx();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.mods.isRightButtonDown())
            return;

        auto realPoints = getRealPoints();
        for (int i = 0; i < realPoints.size(); i++) {
            auto clickBounds = Rectangle<float>().withCentre(realPoints[i]).withSizeKeepingCentre(7, 7);
            if (clickBounds.contains(e.x, e.y)) {
                dragIdx = i;
                if (e.getNumberOfClicks() == 2) {
                    dragIdx = -1;
                    if (i == 0 || i == realPoints.size() - 1) {
                        points.getReference(i).y = 0.0f;
                        resetHoverIdx();
                        triggerOutput();
                        return;
                    }
                    points.remove(i);
                    resetHoverIdx();
                    triggerOutput();
                    return;
                }
                return;
            }
        }

        float newX = jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f);
        float newY = jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f);

        dragIdx = points.addSorted(*this, { newX, newY });

        triggerOutput();
    }

    std::pair<float, float> getRange()
    {
        auto& arr = *range.getValue().getArray();

        auto start = static_cast<float>(arr[0]);
        auto end = static_cast<float>(arr[1]);

        if (approximatelyEqual(start, end)) {
            return { start, end + 0.01f };
        }
        if (start < end) {
            return { start, end };
        }
        if (start > end) {
            return { end, start };
        }

        return { start, end };
    }
    void setRange(std::pair<float, float> newRange)
    {
        auto& arr = *range.getValue().getArray();
        arr[0] = newRange.first;
        arr[1] = newRange.second;

        if (auto function = ptr.get<t_fake_function>()) {
            if (newRange.first <= function->x_min_point)
                function->x_min = newRange.first;
            if (newRange.second >= function->x_max_point)
                function->x_max = newRange.second;
        }
    }

    bool getInit()
    {
        bool init = false;
        if (auto function = ptr.get<t_fake_function>()) {
            init = initialise.getValue();
        }
        return init;
    }
    void setInit(bool init)
    {
        if (auto function = ptr.get<t_fake_function>()) {
            function->x_init = static_cast<int>(init);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        bool changed = false;

        // For first and last point, only adjust y position
        if (dragIdx == 0 || dragIdx == points.size() - 1) {
            float newY = jlimit(0.0f, 1.0f, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));
            if (newY != points.getReference(dragIdx).y) {
                points.getReference(dragIdx).y = newY;
                changed = true;
            }
        }

        else if (dragIdx > 0) {
            float minX = points[dragIdx - 1].x;
            float maxX = points[dragIdx + 1].x;

            float newX = jlimit(minX, maxX, jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f));

            float newY = jlimit(0.0f, 1.0f, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));

            auto newPoint = Point<float>(newX, newY);
            if (points[dragIdx] != newPoint) {
                points.set(dragIdx, newPoint);
                changed = true;
            }
        }

        repaint();
        if (changed)
            triggerOutput();
    }

    void mouseUp(MouseEvent const& e) override
    {
        points.sort(*this);

        if (auto function = ptr.get<t_fake_function>()) {
            auto scale = function->x_dur[function->x_n_states];

            for (int i = 0; i < points.size(); i++) {
                function->x_points[i] = jmap(points[i].y, 0.0f, 1.0f, function->x_min, function->x_max);
                function->x_dur[i] = points[i].x * scale;
            }

            function->x_n_states = points.size() - 1;

            getPointsFromFunction(function.get());
        }

        dragIdx = -1;
    }

    void triggerOutput()
    {

        if (auto function = ptr.get<t_fake_function>()) {
            int ac = points.size() * 2 + 1;

            auto scale = function->x_dur[function->x_n_states];

            auto at = std::vector<t_atom>(ac);
            auto firstPoint = jmap<float>(points[0].y, 0.0f, 1.0f, function->x_min, function->x_max);
            SETFLOAT(at.data(), firstPoint); // get 1st

            function->x_state = 0;
            for (int i = 1; i < ac; i++) { // get the rest

                auto dur = jmap<float>(points[function->x_state + 1].x - points[function->x_state].x, 0.0f, 1.0f, 0.0f, scale);

                SETFLOAT(at.data() + i, dur); // duration
                i++, function->x_state++;
                auto point = jmap<float>(points[function->x_state].y, 0.0f, 1.0f, function->x_min, function->x_max);
                if (point < function->x_min_point)
                    function->x_min_point = point;
                if (point > function->x_max_point)
                    function->x_max_point = point;
                SETFLOAT(at.data() + i, point);
            }

            outlet_list(function->x_obj.ob_outlet, gensym("list"), ac - 2, at.data());
            if (function->x_send != gensym("") && function->x_send->s_thing)
                pd_list(function->x_send->s_thing, gensym("list"), ac - 2, at.data());
        }
    }

    void valueChanged(Value& v) override
    {
        if (auto function = ptr.get<t_fake_function>()) {
            if (v.refersToSameSourceAs(sizeProperty)) {
                auto& arr = *sizeProperty.getValue().getArray();
                auto* constrainer = getConstrainer();
                auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
                auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

                setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

                function->x_width = width;
                function->x_height = height;

                object->updateBounds();
            } else if (v.refersToSameSourceAs(primaryColour)) {
                colourToHexArray(Colour::fromString(primaryColour.toString()), function->x_fgcolor);
                repaint();
            } else if (v.refersToSameSourceAs(secondaryColour)) {
                colourToHexArray(Colour::fromString(secondaryColour.toString()), function->x_bgcolor);
                repaint();
            } else if (v.refersToSameSourceAs(sendSymbol)) {
                auto symbol = sendSymbol.toString();
                if (auto obj = ptr.get<void>())
                    pd->sendDirectMessage(obj.get(), "send", { pd->generateSymbol(symbol) });
            } else if (v.refersToSameSourceAs(receiveSymbol)) {
                auto symbol = receiveSymbol.toString();
                if (auto obj = ptr.get<void>())
                    pd->sendDirectMessage(obj.get(), "receive", { pd->generateSymbol(symbol) });

            } else if (v.refersToSameSourceAs(range)) {
                setRange(getRange());
                getPointsFromFunction(function.get());
            } else if (v.refersToSameSourceAs(initialise)) {
                setInit(getInit());
            }
        }
    }

    static Colour colourFromHexArray(unsigned char* hex)
    {
        return { hex[0], hex[1], hex[2] };
    }

    bool inletIsSymbol() override
    {
        auto rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && (rSymbol != "empty");
    }

    bool outletIsSymbol() override
    {
        auto sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && (sSymbol != "empty");
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("send"): {
            if (numAtoms > 0)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms > 0)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("list"): {
            if (auto function = ptr.get<t_fake_function>()) {
                getPointsFromFunction(function.get());
            }
            break;
        }
        case hash("min"):
        case hash("max"): {
            if (auto function = ptr.get<t_fake_function>()) {
                Array<var> arr = { function->x_min, function->x_max };
                setParameterExcludingListener(range, var(arr));
                getPointsFromFunction(function.get());
            }
            break;
        }
        case hash("init"):
        case hash("fgcolor"):
        case hash("bgcolor"):
        case hash("set"): {
            update();
            break;
        }
        default:
            break;
        }
    }
};
