#include "../Utility/DraggableNumber.h"

typedef struct _numbox{
    t_object  x_obj;
    t_clock  *x_clock_update;
    t_symbol *x_fg;
    t_symbol *x_bg;
    t_glist  *x_glist;
    t_float   x_display;
    t_float   x_in_val;
    t_float   x_out_val;
    t_float   x_set_val;
    t_float   x_max;
    t_float   x_min;
    t_float   x_sr_khz;
    t_float   x_inc;
    t_float   x_ramp_step;
    t_float   x_ramp_val;
    int       x_ramp_ms;
    int       x_rate;
    int       x_numwidth;
    int       x_fontsize;
    int       x_clicked;
    int       x_width, x_height;
    int       x_zoom;
    int       x_outmode;
    char      x_buf[32]; // number buffer
} t_numbox;



struct NumboxTildeObject final : public GUIObject, public Timer {
    DraggableNumber input;

    int nextInterval = 100;
    std::atomic<int> mode = 0;
    
    NumboxTildeObject(void* obj, Box* parent)
        : GUIObject(obj, parent)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            editor->setBorder({ 0, 10, 0, 0 });

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            setValue(input.getText().getFloatValue());
        };

        input.setBorderSize({ 1, 22, 1, 1 });

        addAndMakeVisible(input);

        input.setText(input.formatNumber(getValue()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        addMouseListener(this, true);

        input.valueChanged = [this](float value) { setValue(value); };
        
        mode = static_cast<t_numbox*>(ptr)->x_outmode;
        
        startTimer(nextInterval);
    }
    
    void updateBounds() override
    {
        pd->enqueueFunction([this, _this = SafePointer(this)]() {
            if (!_this)
                return;

            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
            auto bounds = Rectangle<int>(x, y, w, h);

            MessageManager::callAsync([this, _this = SafePointer(this), bounds]() mutable {
                if (!_this)
                    return;

                box->setObjectBounds(bounds);
            });
        });
    }

    void checkBounds() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int width = jlimit(30, maxSize, (getWidth() / fontWidth) * fontWidth);
        int height = jlimit(18, maxSize, getHeight());
        if (getWidth() != width || getHeight() != height) {
            box->setSize(width + Box::doubleMargin, height + Box::doubleMargin);
        }
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        int fontWidth = glist_fontwidth(cnv->patch.getPointer());

        auto* nbx = static_cast<t_numbox*>(ptr);
        nbx->x_numwidth = b.getWidth() / fontWidth;
        
        nbx->x_width = b.getWidth();
        nbx->x_height = b.getHeight();
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }


    ObjectParameters defineParameters() override
    {
        return { { "Minimum", tFloat, cGeneral, &min, {} }, { "Maximum", tFloat, cGeneral, &max, {} } };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::highlightColourId));

        g.setFont(22.f);
        g.drawFittedText("~", getLocalBounds().withWidth(getHeight()).withX(2).withY(-1),
            juce::Justification::centred, 1);
    }
    
    void timerCallback() override
    {
        if(!mode) {
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
        pd_typedmess(static_cast<t_pd*>(ptr), gensym("set"), 1, &at);
        pd->getCallbackLock()->exit();
    }
    
    float getValue() override
    {
        auto* object = static_cast<t_numbox*>(ptr);
        
        // Kinda ugly, but use this audio-thread function to update all the variables
        
        mode = object->x_outmode;
        nextInterval = object->x_rate;
        
        return mode ? object->x_set_val : object->x_in_val;
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
        
        if(static_cast<float>(min.getValue()) <= static_cast<float>(max.getValue())) {
            input.setMinimum(value);
            setValue(std::clamp(getValueOriginal(), value, static_cast<float>(max.getValue())));
        }
    }

    void setMaximum(float value)
    {
        static_cast<t_numbox*>(ptr)->x_max = value;
        
        if(static_cast<float>(max.getValue()) <= static_cast<float>(min.getValue())) {
            input.setMaximum(value);
            setValue(std::clamp(getValueOriginal(), static_cast<float>(min.getValue()), value));

        }
    }
};
