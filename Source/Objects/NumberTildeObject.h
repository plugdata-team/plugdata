#include "../Utility/DraggableNumber.h"

struct t_number_tilde
{
    t_iemgui x_gui;
    t_outlet *x_signal_outlet;
    t_outlet *x_float_outlet;
    t_clock  *x_clock_reset;
    t_clock  *x_clock_wait;
    t_clock  *x_clock_repaint;
    t_float x_in_val;
    t_float x_out_val;
    t_float x_set_val;
    t_float x_ramp_val;
    t_float x_ramp_time;
    t_float x_ramp;
    t_float x_last_out;
    t_float x_maximum;
    t_float x_minimum;
    t_float  x_interval_ms;
    int      x_numwidth;
    int      x_output_mode;
    int      x_needs_update;
    char     x_buf[IEMGUI_MAX_NUM_LEN];
};


struct NumberTildeObject final : public GUIObject, public Timer {
    DraggableNumber input;

    int nextInterval = 100;
    std::atomic<int> mode = 0;
    
    NumberTildeObject(void* obj, Box* parent)
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
        
        mode = static_cast<t_number_tilde*>(ptr)->x_output_mode;
        
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

        auto* nbx = static_cast<t_number_tilde*>(ptr);
        nbx->x_numwidth = b.getWidth() / fontWidth;
        nbx->x_gui.x_w = b.getWidth();
        nbx->x_gui.x_h = b.getHeight();
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
        input.setText(input.formatNumber(getValueOriginal()), dontSendNotification);
        startTimer(nextInterval);
    }

    void setValue(float newValue)
    {
        auto* object = static_cast<t_number_tilde*>(ptr);
        
        pd->getCallbackLock()->enter();
        object->x_set_val = newValue;
        pd->getCallbackLock()->exit();
    }
    
    float getValue() override
    {
        auto* object = static_cast<t_number_tilde*>(ptr);
        
        // Kinda ugly, but use this audio-thread function to update all the variables
        
        mode = object->x_output_mode;
        nextInterval = object->x_interval_ms;
        
        return object->x_output_mode ? object->x_set_val : object->x_in_val;
    }

    float getMinimum()
    {
        return (static_cast<t_number_tilde*>(ptr))->x_minimum;
    }

    float getMaximum()
    {
        return (static_cast<t_number_tilde*>(ptr))->x_maximum;
    }

    void setMinimum(float value)
    {
        static_cast<t_number_tilde*>(ptr)->x_minimum = value;
    }

    void setMaximum(float value)
    {
        static_cast<t_number_tilde*>(ptr)->x_maximum = value;
    }
};
