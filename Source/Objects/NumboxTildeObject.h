/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/DraggableNumber.h"

class NumboxTildeObject final : public ObjectBase
    , public Timer {

    DraggableNumber input;

    int nextInterval = 100;
    std::atomic<int> mode = 0;

    Value interval, ramp, init;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    Value primaryColour;
    Value secondaryColour;

public:
    NumboxTildeObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , input(false)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            sendFloatValue(input.getText().getFloatValue());
        };

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.valueChanged = [this](float value) { sendFloatValue(value); };

        startTimer(nextInterval);
        repaint();

        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 0.0f);
        objectParameters.addParamFloat("Interval (ms)", cGeneral, &interval, 100.0f);
        objectParameters.addParamFloat("Ramp time (ms)", cGeneral, &ramp, 10.0f);
        objectParameters.addParamFloat("Initial value", cGeneral, &init, 0.0f);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);
    }

    void update() override
    {
        input.setText(input.formatNumber(getValue()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        auto* object = static_cast<t_fake_numbox*>(ptr);
        interval = object->x_rate;
        ramp = object->x_ramp_ms;
        init = object->x_set_val;

        primaryColour = "ff" + String::fromUTF8(object->x_fg->s_name + 1);
        secondaryColour = "ff" + String::fromUTF8(object->x_bg->s_name + 1);

        auto fg = Colour::fromString(primaryColour.toString());
        getLookAndFeel().setColour(Label::textColourId, fg);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, fg);
        getLookAndFeel().setColour(TextEditor::textColourId, fg);

        mode = object->x_outmode;
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->unlockAudioThread();

        return bounds;
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class NumboxTildeBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            Object* object;

            explicit NumboxTildeBoundsConstrainer(Object* parent)
                : object(parent)
            {
            }
            /*
             * Custom version of checkBounds that takes into consideration
             * the padding around plugdata node objects when resizing
             * to allow the aspect ratio to be interpreted correctly.
             * Otherwise resizing objects with an aspect ratio will
             * resize the object size **including** margins, and not follow the
             * actual size of the visible object
             */
            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool isStretchingTop,
                bool isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                auto* nbx = static_cast<t_fake_numbox*>(object->getPointer());

                nbx->x_fontsize = object->gui->getHeight() - 4;

                BorderSize<int> border(Object::margin);
                border.subtractFrom(bounds);

                // we also have to remove the margin from the old object, but don't alter the old object
                ComponentBoundsConstrainer::checkBounds(bounds, border.subtractedFrom(old), limits, isStretchingTop,
                    isStretchingLeft,
                    isStretchingBottom,
                    isStretchingRight);

                // put back the margins
                border.addTo(bounds);
            }
        };

        auto constrainer = std::make_unique<NumboxTildeBoundsConstrainer>(object);
        constrainer->setMinimumSize(30, 15);

        return constrainer;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* nbx = static_cast<t_fake_numbox*>(ptr);
        nbx->x_width = b.getWidth();
        nbx->x_height = b.getHeight();
        nbx->x_fontsize = b.getHeight() - 4;

        nbx->x_numwidth = (2.0f * (-6.0f + b.getWidth() - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().withTrimmedLeft(getHeight() - 4));
        input.setFont(input.getFont().withHeight(getHeight() - 6));
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(::getValue<float>(min));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
        } else if (value.refersToSameSourceAs(interval)) {
            auto* nbx = static_cast<t_fake_numbox*>(ptr);
            nbx->x_rate = ::getValue<float>(interval);

        } else if (value.refersToSameSourceAs(ramp)) {
            auto* nbx = static_cast<t_fake_numbox*>(ptr);
            nbx->x_ramp_ms = ::getValue<float>(ramp);
        } else if (value.refersToSameSourceAs(init)) {
            auto* nbx = static_cast<t_fake_numbox*>(ptr);
            nbx->x_set_val = ::getValue<float>(init);
        } else if (value.refersToSameSourceAs(primaryColour)) {
            setForegroundColour(primaryColour.toString());
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            setBackgroundColour(secondaryColour.toString());
        }
    }

    void setForegroundColour(String const& colour)
    {
        // Remove alpha channel and add #
        static_cast<t_fake_numbox*>(ptr)->x_fg = pd->generateSymbol("#" + colour.substring(2));

        auto col = Colour::fromString(colour);
        getLookAndFeel().setColour(Label::textColourId, col);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, col);
        getLookAndFeel().setColour(TextEditor::textColourId, col);

        repaint();
    }

    void setBackgroundColour(String const& colour)
    {
        static_cast<t_fake_numbox*>(ptr)->x_bg = pd->generateSymbol("#" + colour.substring(2));
        repaint();
    }

    void paintOverChildren(Graphics& g) override
    {
        auto iconBounds = Rectangle<int>(2, 0, getHeight(), getHeight());
        Fonts::drawIcon(g, mode ? Icons::ThinDown : Icons::Sine, iconBounds, object->findColour(PlugDataColour::dataColourId));
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::guiObjectInternalOutlineColour);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void timerCallback() override
    {
        auto val = getValue();

        if (!mode) {
            input.setText(input.formatNumber(val), dontSendNotification);
        }

        startTimer(nextInterval);
    }

    float getValue()
    {
        auto* obj = static_cast<t_fake_numbox*>(ptr);

        mode = obj->x_outmode;

        nextInterval = obj->x_rate;

        return mode ? obj->x_display : obj->x_in_val;
    }

    float getMinimum()
    {
        return (static_cast<t_fake_numbox*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return (static_cast<t_fake_numbox*>(ptr))->x_max;
    }

    void setMinimum(float minValue)
    {
        static_cast<t_fake_numbox*>(ptr)->x_min = minValue;

        input.setMinimum(minValue);
    }

    void setMaximum(float maxValue)
    {
        static_cast<t_fake_numbox*>(ptr)->x_max = maxValue;

        input.setMaximum(maxValue);
    }
};
