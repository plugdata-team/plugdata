#include "../Utility/DraggableNumber.h"

typedef struct _numbox {
    t_object x_obj;
    t_clock* x_clock_update;
    t_symbol* x_fg;
    t_symbol* x_bg;
    t_glist* x_glist;
    t_float x_display;
    t_float x_in_val;
    t_float x_out_val;
    t_float x_set_val;
    t_float x_max;
    t_float x_min;
    t_float x_sr_khz;
    t_float x_inc;
    t_float x_ramp_step;
    t_float x_ramp_val;
    int x_ramp_ms;
    int x_rate;
    int x_numwidth;
    int x_fontsize;
    int x_clicked;
    int x_width, x_height;
    int x_zoom;
    int x_outmode;
    char x_buf[32]; // number buffer
} t_numbox;

struct NumboxTildeObject final : public GUIObject
    , public Timer {
    DraggableNumber input;

    int nextInterval = 100;
    std::atomic<int> mode = 0;

    Value interval, ramp, init;

    NumboxTildeObject(void* obj, Box* parent)
        : GUIObject(obj, parent)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            setValue(input.getText().getFloatValue());
        };

        addAndMakeVisible(input);

        input.setText(input.formatNumber(getValue()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        auto* object = static_cast<t_numbox*>(ptr);
        interval = object->x_rate;
        ramp = object->x_ramp_ms;
        init = object->x_set_val;

        primaryColour = "ff" + String(object->x_fg->s_name + 1);
        secondaryColour = "ff" + String(object->x_bg->s_name + 1);

        auto fg = Colour::fromString(primaryColour.toString());
        getLookAndFeel().setColour(Label::textColourId, fg);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, fg);
        getLookAndFeel().setColour(TextEditor::textColourId, fg);

        addMouseListener(this, true);

        input.valueChanged = [this](float value) { setValue(value); };

        mode = static_cast<t_numbox*>(ptr)->x_outmode;

        startTimer(nextInterval);
        repaint();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->getCallbackLock()->exit();

        box->setObjectBounds(bounds);
    }

    void checkBounds() override
    {
        auto* nbx = static_cast<t_numbox*>(ptr);

        nbx->x_fontsize = getHeight() - 4;

        int width = getWidth();
        int numWidth = (2.0f * (-6.0f + width - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
        width = (nbx->x_fontsize - (nbx->x_fontsize / 2) + 2) * (numWidth + 2) + 2;

        int height = jlimit(18, maxSize, getHeight());
        if (getWidth() != width || getHeight() != height) {
            box->setSize(width + Box::doubleMargin, height + Box::doubleMargin);
        }
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* nbx = static_cast<t_numbox*>(ptr);
        nbx->x_width = b.getWidth();
        nbx->x_height = b.getHeight();
        nbx->x_fontsize = b.getHeight() - 4;

        nbx->x_numwidth = (2.0f * (-6.0f + b.getWidth() - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().withTrimmedLeft(getHeight() - 4));
        input.setFont(getHeight() - 6);
    }

    ObjectParameters defineParameters() override
    {
        return {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Interval (ms)", tFloat, cGeneral, &interval, {} },
            { "Ramp time (ms)", tFloat, cGeneral, &ramp, {} },
            { "Initial value", tFloat, cGeneral, &init, {} },
            { "Foreground", tColour, cAppearance, &primaryColour, {} },
            { "Background", tColour, cAppearance, &secondaryColour, {} },
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        } else if (value.refersToSameSourceAs(interval)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_rate = static_cast<float>(interval.getValue());

        } else if (value.refersToSameSourceAs(ramp)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_ramp_ms = static_cast<float>(ramp.getValue());
        } else if (value.refersToSameSourceAs(init)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_set_val = static_cast<float>(init.getValue());
        } else if (value.refersToSameSourceAs(primaryColour)) {
            setForegroundColour(primaryColour.toString());
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            setBackgroundColour(secondaryColour.toString());
        }
    }

    void setForegroundColour(String colour)
    {
        // Remove alpha channel and add #

        ((t_numbox*)ptr)->x_fg = gensym(("#" + colour.substring(2)).toRawUTF8());

        auto col = Colour::fromString(colour);
        getLookAndFeel().setColour(Label::textColourId, col);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, col);
        getLookAndFeel().setColour(TextEditor::textColourId, col);

        repaint();
    }

    void setBackgroundColour(String colour)
    {
        ((t_numbox*)ptr)->x_bg = gensym(("#" + colour.substring(2)).toRawUTF8());
        repaint();
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::highlightColourId));

        auto iconBounds = Rectangle<int>(2, 0, getHeight(), getHeight());

        auto font = dynamic_cast<PlugDataLook&>(box->getLookAndFeel()).iconFont.withHeight(getHeight() - 8);
        g.setFont(font);

        g.drawFittedText(mode ? Icons::ThinDown : Icons::Sine, iconBounds,
            juce::Justification::centred, 1);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        auto outlineColour = box->findColour(cnv->isSelected(box) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    void timerCallback() override
    {
        if (!mode) {
            input.setText(input.formatNumber(getValueOriginal()), dontSendNotification);
        }

        startTimer(nextInterval);
    }

    void setValue(float newValue)
    {
        t_atom at;
        SETFLOAT(&at, newValue);
        setValueOriginal(newValue);

        pd->getCallbackLock()->enter();
        pd_float(static_cast<t_pd*>(ptr), newValue);
        // pd_typedmess(static_cast<t_pd*>(ptr), gensym("set"), 1, &at);
        pd->getCallbackLock()->exit();
    }

    float getValue() override
    {
        auto* object = static_cast<t_numbox*>(ptr);

        // Kinda ugly, but use this audio-thread function to update all the variables

        mode = object->x_outmode;
        nextInterval = object->x_rate;

        return mode ? object->x_display : object->x_in_val;
    }

    float getMinimum()
    {
        return (static_cast<t_numbox*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return (static_cast<t_numbox*>(ptr))->x_max;
    }

    void setMinimum(float value)
    {
        static_cast<t_numbox*>(ptr)->x_min = value;

        input.setMinimum(value);

        if (static_cast<float>(min.getValue()) < static_cast<float>(max.getValue())) {

            setValueOriginal(std::clamp(getValueOriginal(), value, static_cast<float>(max.getValue())));
        }
    }

    void setMaximum(float value)
    {
        static_cast<t_numbox*>(ptr)->x_max = value;

        input.setMaximum(value);

        if (static_cast<float>(max.getValue()) > static_cast<float>(min.getValue())) {
            setValueOriginal(std::clamp(getValueOriginal(), static_cast<float>(min.getValue()), value));
        }
    }
};
