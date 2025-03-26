/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class FunctionObject final : public ObjectBase {

    int hoverIdx = -1;
    int dragIdx = -1;
    int selectedIdx = -1;

    Value initialise = SynchronousValue();
    Value range = SynchronousValue();
    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    SmallArray<Point<float>> points;

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
            sizeProperty = VarArray { var(function->x_width), var(function->x_height) };
            initialise = function->x_init;

            VarArray const arr = { function->x_min, function->x_max };
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
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, function.cast<t_gobj>(), b.getX(), b.getY());
            function->x_width = b.getWidth() - 1;
            function->x_height = b.getHeight() - 1;
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

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
            setParameterExcludingListener(sizeProperty, VarArray { var(function->x_width), var(function->x_height) });
        }
    }

    SmallArray<Point<float>> getRealPoints()
    {
        auto realPoints = SmallArray<Point<float>>();
        for (auto point : points) {
            point.x = jmap<float>(point.x, 0.0f, 1.0f, 3, getWidth() - 3);
            point.y = jmap<float>(point.y, 0.0f, 1.0f, getHeight() - 3, 3);
            realPoints.add(point);
        }

        return realPoints;
    }

    void render(NVGcontext* nvg) override
    {
        bool const selected = object->isSelected() && !cnv->isGraph;
        bool const editing = cnv->locked == var(true) || cnv->presentationMode == var(true) || ModifierKeys::getCurrentModifiers().isCtrlDown();

        auto const b = getLocalBounds().toFloat();
        auto const backgroundColour = convertColour(Colour::fromString(secondaryColour.toString()));

        auto const foregroundColour = convertColour(Colour::fromString(primaryColour.toString()));
        auto const selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto const outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

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

            nvgFillColor(nvg, foregroundColour);
            nvgStrokeColor(nvg, hoverIdx == i && editing ? outlineColour : foregroundColour);
            nvgBeginPath(nvg);
            nvgCircle(nvg, point.getX(), point.getY(), 2.5f);
            if (selectedIdx == i) {
                nvgFill(nvg);
            }
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

    bool keyPressed(KeyPress const& key) override
    {

        if (getValue<bool>(cnv->locked) && key.getKeyCode() == KeyPress::deleteKey && selectedIdx >= 0) {
            removePoint(selectedIdx);
            selectedIdx = -1;
            return true;
        }

        return false;
    }

    static int compareElements(Point<float> const& a, Point<float> const& b)
    {
        return a.x < b.x;
    }

    void setHoverIdx(int const i)
    {
        hoverIdx = i;
        repaint();
    }

    void resetHoverIdx()
    {
        setHoverIdx(-1);
    }

    bool hitTest(int const x, int const y) override
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

        selectedIdx = -1;

        auto realPoints = getRealPoints();
        for (int i = 0; i < realPoints.size(); i++) {
            auto clickBounds = Rectangle<float>().withCentre(realPoints[i]).withSizeKeepingCentre(7, 7);
            if (clickBounds.contains(e.x, e.y)) {
                dragIdx = i;
                selectedIdx = i;
                if (e.getNumberOfClicks() == 2) {
                    removePoint(i);
                }
                return;
            }
        }

        float newX = jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f);
        float newY = jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f);

        dragIdx = points.add_sorted(&compareElements, { newX, newY });

        triggerOutput();
    }

    void removePoint(int const idx)
    {
        if (idx == 0 || idx == points.size() - 1) {
            points[idx].y = 0.0f;
        } else {
            points.remove_at(idx);
        }
        selectedIdx = -1;
        resetHoverIdx();
        triggerOutput();
    }

    Range<float> getRange() const
    {
        auto const& arr = *range.getValue().getArray();

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
    void setRange(Range<float> const& newRange)
    {
        auto const& arr = *range.getValue().getArray();
        arr[0] = newRange.getStart();
        arr[1] = newRange.getEnd();

        if (auto function = ptr.get<t_fake_function>()) {
            if (newRange.getStart() <= function->x_min_point)
                function->x_min = newRange.getStart();
            if (newRange.getEnd() >= function->x_max_point)
                function->x_max = newRange.getEnd();
        }
    }

    bool getInit() const
    {
        bool init = false;
        if (auto function = ptr.get<t_fake_function>()) {
            init = initialise.getValue();
        }
        return init;
    }
    void setInit(bool const init)
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
            float const newY = jlimit(0.0f, 1.0f, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));
            if (newY != points[dragIdx].y) {
                points[dragIdx].y = newY;
                changed = true;
            }
        }

        else if (dragIdx > 0) {
            float const minX = points[dragIdx - 1].x;
            float const maxX = points[dragIdx + 1].x;

            float const newX = jlimit(minX, maxX, jmap(static_cast<float>(e.x), 3.0f, getWidth() - 3.0f, 0.0f, 1.0f));

            float const newY = jlimit(0.0f, 1.0f, jmap(static_cast<float>(e.y), 3.0f, getHeight() - 3.0f, 1.0f, 0.0f));

            auto const newPoint = Point<float>(newX, newY);
            if (points[dragIdx] != newPoint) {
                points[dragIdx] = newPoint;
                changed = true;
            }
        }

        repaint();
        if (changed)
            triggerOutput();
    }

    void mouseUp(MouseEvent const& e) override
    {
        points.sort(compareElements);

        if (auto function = ptr.get<t_fake_function>()) {
            auto const scale = function->x_dur[function->x_n_states];

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
            int const ac = points.size() * 2 + 1;

            auto const scale = function->x_dur[function->x_n_states];

            auto at = SmallArray<t_atom>(ac);
            auto const firstPoint = jmap<float>(points[0].y, 0.0f, 1.0f, function->x_min, function->x_max);
            SETFLOAT(at.data(), firstPoint); // get 1st

            function->x_state = 0;
            for (int i = 1; i < ac; i++) { // get the rest
                auto next = std::min<int>(function->x_state + 1, points.size() - 1);
                auto const dur = jmap<float>(points[next].x - points[function->x_state].x, 0.0f, 1.0f, 0.0f, scale);

                SETFLOAT(at.data() + i, dur); // duration
                i++, function->x_state++;
                next = std::min<int>(function->x_state, points.size() - 1);
                auto const point = jmap<float>(points[next].y, 0.0f, 1.0f, function->x_min, function->x_max);
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

    void propertyChanged(Value& v) override
    {
        if (auto function = ptr.get<t_fake_function>()) {
            if (v.refersToSameSourceAs(sizeProperty)) {
                auto const& arr = *sizeProperty.getValue().getArray();
                auto const* constrainer = getConstrainer();
                auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
                auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

                setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

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
                auto const symbol = sendSymbol.toString();
                if (auto obj = ptr.get<void>())
                    pd->sendDirectMessage(obj.get(), "send", { pd->generateSymbol(symbol) });
            } else if (v.refersToSameSourceAs(receiveSymbol)) {
                auto const symbol = receiveSymbol.toString();
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
        auto const rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && rSymbol != "empty";
    }

    bool outletIsSymbol() override
    {
        auto const sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && sSymbol != "empty";
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("send"): {
            if (atoms.size() > 0)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() > 0)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            object->updateIolets();
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
                VarArray const arr = { function->x_min, function->x_max };
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
