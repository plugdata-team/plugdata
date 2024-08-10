/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/DraggableNumber.h"

class NumboxTildeObject final : public ObjectBase
    , public Timer {

    DraggableNumber input;

    int nextInterval = 100;
    int mode = 0;

    Value interval = SynchronousValue();
    Value ramp = SynchronousValue();
    Value init = SynchronousValue();

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sizeProperty = SynchronousValue();

public:
    NumboxTildeObject(pd::WeakReference obj, Object* parent)
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

        input.onValueChange = [this](float value) {
            if (auto obj = ptr.get<t_pd>()) {
                pd_float(obj.get(), value);
                pd_bang(obj.get());
            }
        };

        startTimer(nextInterval);
        repaint();

        objectParameters.addParamSize(&sizeProperty);
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
        if (input.isShowing())
            return;

        input.setText(input.formatNumber(getValue()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        if (auto object = ptr.get<t_fake_numbox>()) {
            interval = object->x_rate;
            ramp = object->x_ramp_ms;
            init = object->x_set_val;
            primaryColour = "ff" + String::fromUTF8(object->x_fg->s_name + 1);
            secondaryColour = "ff" + String::fromUTF8(object->x_bg->s_name + 1);
            mode = object->x_outmode;
            sizeProperty = Array<var> { var(object->x_width), var(object->x_height) };
        }

        auto fg = Colour::fromString(primaryColour.toString());
        getLookAndFeel().setColour(Label::textColourId, fg);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, fg);
        getLookAndFeel().setColour(TextEditor::textColourId, fg);
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            return { x, y, w, h };
        }

        return {};
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto nbx = ptr.get<t_fake_numbox>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(nbx->x_width), var(nbx->x_height) });
        }
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
                auto* nbx = reinterpret_cast<t_fake_numbox*>(object->getPointer());

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
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            nbx->x_width = b.getWidth();
            nbx->x_height = b.getHeight();
            nbx->x_fontsize = b.getHeight() - 4;
            nbx->x_numwidth = (2.0f * (-6.0f + b.getWidth() - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);

            pd::Interface::moveObject(patch, nbx.cast<t_gobj>(), b.getX(), b.getY());
        }
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().withTrimmedLeft(getHeight() - 4));
        input.setFont(input.getFont().withHeight(getHeight() - 6));
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_width = width;
                nbx->x_height = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            setMinimum(::getValue<float>(min));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
        } else if (value.refersToSameSourceAs(interval)) {
            if (auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_rate = ::getValue<float>(interval);
            }

        } else if (value.refersToSameSourceAs(ramp)) {
            if (auto nbx = ptr.get<t_fake_numbox>()) {
                nbx->x_ramp_ms = ::getValue<float>(ramp);
            }
        } else if (value.refersToSameSourceAs(init)) {
            if (auto nbx = ptr.get<t_fake_numbox>()) {
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
        if(auto numbox = ptr.get<t_fake_numbox>())
        {
            numbox->x_fg = pd->generateSymbol("#" + colour.substring(2));
        }

        auto col = Colour::fromString(colour);
        getLookAndFeel().setColour(Label::textColourId, col);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, col);
        getLookAndFeel().setColour(TextEditor::textColourId, col);

        repaint();
    }

    void setBackgroundColour(String const& colour)
    {
        if(auto numbox = ptr.get<t_fake_numbox>())
        {
            numbox->x_bg = pd->generateSymbol("#" + colour.substring(2));
        }
        
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        auto backgroundColour = Colour::fromString(secondaryColour.toString());
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(backgroundColour), convertColour(outlineColour), Corners::objectCornerRadius);

        {
            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, input.getX(), input.getY());
            input.render(nvg);
        }


        auto icon = mode ? Icons::ThinDown : Icons::Sine;
        auto iconBounds = Rectangle<int>(7, 3, getHeight(), getHeight());
        nvgFontFace(nvg, "icon_font-Regular");
        nvgFontSize(nvg, 12.0f);
        nvgFillColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::dataColourId)));
        nvgTextAlign(nvg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
        nvgText(nvg, iconBounds.getX(), iconBounds.getY(), icon.toRawUTF8(), nullptr);
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
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            mode = nbx->x_outmode;

            nextInterval = nbx->x_rate;

            return mode ? nbx->x_display : nbx->x_in_val;
        }

        return 0.0f;
    }

    float getMinimum()
    {
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            return nbx->x_lower;
        }

        return 0.0f;
    }

    float getMaximum()
    {
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            return nbx->x_upper;
        }

        return 0.0f;
    }

    void setMinimum(float minValue)
    {
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            nbx->x_lower = minValue;
        }

        input.setMinimum(minValue);
    }

    void setMaximum(float maxValue)
    {
        if (auto nbx = ptr.get<t_fake_numbox>()) {
            nbx->x_upper = maxValue;
        }

        input.setMaximum(maxValue);
    }
};
