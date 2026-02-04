/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class BangObject final : public ObjectBase
    , public Timer {
    uint32_t lastBang = 0;

    Value bangInterrupt = SynchronousValue(100.0f);
    Value bangHold = SynchronousValue(40.0f);
    Value sizeProperty = SynchronousValue();

    bool bangState = false;
    bool alreadyBanged = false;

    IEMHelper iemHelper;

    NVGcolor bgCol;
    NVGcolor fgCol;

public:
    BangObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , iemHelper(obj, parent, this)
    {
        iemHelper.iemColourChangedCallback = [this] {
            bgCol = convertColour(getValue<Colour>(iemHelper.secondaryColour));
            fgCol = convertColour(getValue<Colour>(iemHelper.primaryColour));
        };

        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamInt("Min. flash time", cGeneral, &bangInterrupt, 50);
        objectParameters.addParamInt("Max. flash time", cGeneral, &bangHold, 250);

        iemHelper.addIemParameters(objectParameters, true, true, true, 0, -10);
    }

    void onConstrainerCreate() override
    {
        constrainer->setFixedAspectRatio(1);
    }
        
    ResizeDirection getAllowedResizeDirections() const override
    {
        return DiagonalOnly;
    }

    void update() override
    {
        if (auto bng = ptr.get<t_bng>()) {
            sizeProperty = bng->x_gui.x_w;
            bangInterrupt = bng->x_flashtime_break;
            bangHold = bng->x_flashtime_hold;
        }

        iemHelper.update();
    }

    bool hideInlet() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlet() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(labels);
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        iemHelper.setPdBounds(b);
    }

    void toggleObject(Point<int> position) override
    {
        if (!alreadyBanged) {

            startEdition();
            if (auto bng = ptr.get<t_pd>())
                pd_bang(bng.get());
            stopEdition();

            trigger();
            alreadyBanged = true;
        }
    }

    void untoggleObject() override
    {
        alreadyBanged = false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        // startEdition();
        pd->enqueueFunctionAsync<t_pd>(ptr, [](t_pd* bng) {
            sys_lock();
            pd_bang(bng);
            sys_unlock();
        });
        // stopEdition();

        // Make sure we don't re-click with an accidental drag
        alreadyBanged = true;
        trigger();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), bgCol, object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);

        b = b.reduced(1);
        auto const width = std::max(b.getWidth(), b.getHeight());

        auto const sizeReduction = std::min(1.0f, getWidth() / 20.0f);

        float const circleOuter = 80.f * (width * 0.01f);
        float const circleThickness = std::max(width * 0.06f, 1.5f) * sizeReduction;

        auto const outerCircleBounds = b.reduced((width - circleOuter) * sizeReduction);

        nvgBeginPath(nvg);
        nvgCircle(nvg, b.getCentreX(), b.getCentreY(), outerCircleBounds.getWidth() / 2.0f);
        nvgStrokeColor(nvg, cnv->guiObjectInternalOutlineCol);
        nvgStrokeWidth(nvg, circleThickness);
        nvgStroke(nvg);

        // Fill ellipse if bangState is true
        if (bangState) {
            auto const iCB = b.reduced((width - circleOuter + circleThickness) * sizeReduction);
            nvgDrawRoundedRect(nvg, iCB.getX(), iCB.getY(), iCB.getWidth(), iCB.getHeight(), fgCol, fgCol, iCB.getWidth() * 0.5f);
        }
    }

    void trigger()
    {
        if (bangState)
            return;

        bangState = true;
        repaint();

        auto const currentTime = Time::getMillisecondCounter();
        auto const timeSinceLast = currentTime - lastBang;

        int holdTime = bangHold.getValue();

        if (timeSinceLast < getValue<int>(bangHold) * 2) {
            holdTime = timeSinceLast / 2;
        }
        if (holdTime < bangInterrupt) {
            holdTime = bangInterrupt.getValue();
        }

        lastBang = currentTime;
        startTimer(holdTime); // Delay it slightly more, to compensate for audio->gui delay
    }

    void timerCallback() override
    {
        if (bangState) {
            bangState = false;
            repaint();
        }
        stopTimer();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, var(iem->x_w));
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const size = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);
            if (auto bng = ptr.get<t_bng>()) {
                bng->x_gui.x_w = size;
                bng->x_gui.x_h = size;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(bangInterrupt)) {
            if (auto bng = ptr.get<t_bng>())
                bng->x_flashtime_break = bangInterrupt.getValue();
        } else if (value.refersToSameSourceAs(bangHold)) {
            if (auto bng = ptr.get<t_bng>())
                bng->x_flashtime_hold = bangHold.getValue();
        } else {
            iemHelper.valueChanged(value);
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("bang"):
        case hash("list"):
            trigger();
            break;
        case hash("flashtime"): {
            if (atoms.size() > 0)
                setParameterExcludingListener(bangInterrupt, atoms[0].getFloat());
            if (atoms.size() > 1)
                setParameterExcludingListener(bangHold, atoms[1].getFloat());
            break;
        }
        case hash("pos"):
        case hash("size"):
        case hash("loadbang"):
            break;
        default: {
            if (bool const wasIemMessage = iemHelper.receiveObjectMessage(symbol, atoms); !wasIemMessage) {
                trigger();
            }
            break;
        }
        }
    }
};
