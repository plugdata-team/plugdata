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

        if(auto object = ptr.get<t_fake_numbox>())
        {
            interval = object->x_rate;
            ramp = object->x_ramp_ms;
            init = object->x_set_val;
            primaryColour = "ff" + String::fromUTF8(object->x_fg->s_name + 1);
            secondaryColour = "ff" + String::fromUTF8(object->x_bg->s_name + 1);
            mode = object->x_outmode;
        }

        auto fg = Colour::fromString(primaryColour.toString());
        getLookAndFeel().setColour(Label::textColourId, fg);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, fg);
        getLookAndFeel().setColour(TextEditor::textColourId, fg);
    }

    Rectangle<int> getPdBounds() override
    {
        if(auto gobj = ptr.get<t_gobj>())
        {
            auto* patch = cnv->patch.getPointer().get();
            if(!patch) return;
            
            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(patch, gobj.get(), &x, &y, &w, &h);
            return {x, y, w, h};
        }

        return {};
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
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            auto* patch = cnv->patch.getPointer().get();
            if(!patch) return;
            
            nbx->x_width = b.getWidth();
            nbx->x_height = b.getHeight();
            nbx->x_fontsize = b.getHeight() - 4;
            nbx->x_numwidth = (2.0f * (-6.0f + b.getWidth() - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
            
            libpd_moveobj(patch, nbx.cast<t_gobj>(), b.getX(), b.getY());
        }
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
            if(auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_rate = ::getValue<float>(interval);
            }

        } else if (value.refersToSameSourceAs(ramp)) {
            if(auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_ramp_ms = ::getValue<float>(ramp);
            }
        } else if (value.refersToSameSourceAs(init)) {
            if(auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_set_val = ::getValue<float>(init);
            }
        } else if (value.refersToSameSourceAs(primaryColour)) {
            setForegroundColour(primaryColour.toString());
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            setBackgroundColour(secondaryColour.toString());
        }
    }

    void setForegroundColour(String const& colour)
    {
        // Remove alpha channel and add #
        ptr.get<t_fake_numbox>()->x_fg = pd->generateSymbol("#" + colour.substring(2));

        auto col = Colour::fromString(colour);
        getLookAndFeel().setColour(Label::textColourId, col);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, col);
        getLookAndFeel().setColour(TextEditor::textColourId, col);

        repaint();
    }

    void setBackgroundColour(String const& colour)
    {
        ptr.get<t_fake_numbox>()->x_bg = pd->generateSymbol("#" + colour.substring(2));
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
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            mode = nbx->x_outmode;

            nextInterval = nbx->x_rate;

            return mode ? nbx->x_display : nbx->x_in_val;
        }

        return 0.0f;
    }

    float getMinimum()
    {
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            return nbx->x_min;
        }
        
        return 0.0f;
    }

    float getMaximum()
    {
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            return nbx->x_max;
        }
        
        return 0.0f;
    }

    void setMinimum(float minValue)
    {
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            nbx->x_min = minValue;
        }

        input.setMinimum(minValue);
    }

    void setMaximum(float maxValue)
    {
        if(auto nbx = ptr.get<t_fake_numbox>())
        {
            nbx->x_max = maxValue;
        }
        
        input.setMaximum(maxValue);
    }
};
