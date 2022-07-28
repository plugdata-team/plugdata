#include "../Utility/DraggableNumber.h"

struct NumberObject final : public IEMObject {
    Label input;
    DraggableNumber dragger;

    NumberObject(void* obj, Box* parent)
        : IEMObject(obj, parent)
        , dragger(input)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setBorder({ 0, 10, 0, 0 });

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };

        input.setBorderSize({ 1, 15, 1, 1 });

        addAndMakeVisible(input);

        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        input.setEditable(true, false);

        addMouseListener(this, true);

        dragger.dragStart = [this]() { startEdition(); };

        dragger.valueChanged = [this](float value) { setValueOriginal(value); };

        dragger.dragEnd = [this]() { stopEdition(); };
    }

    void updateBounds() override
    {
        box->cnv->pd->enqueueFunction([this, _this = SafePointer(this)]() {
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

        auto* nbx = static_cast<t_my_numbox*>(ptr);
        nbx->x_numwidth = b.getWidth() / fontWidth;
        nbx->x_gui.x_w = b.getWidth();
        nbx->x_gui.x_h = b.getHeight();
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }

    void update() override
    {
        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);
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
        } else {
            IEMObject::valueChanged(value);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        int const indent = 9;

        Rectangle<int> const iconBounds = getLocalBounds().withWidth(indent - 4).withHeight(getHeight() - 8).translated(4, 4);

        Path corner;

        corner.addTriangle(iconBounds.getTopLeft().toFloat(), iconBounds.getTopRight().toFloat() + Point<float>(0, (iconBounds.getHeight() / 2.)), iconBounds.getBottomLeft().toFloat());

        g.setColour(Colour(getForegroundColour()).interpolatedWith(box->findColour(PlugDataColour::toolbarColourId), 0.5f));
        g.fillPath(corner);
    }

    float getValue() override
    {
        return (static_cast<t_my_numbox*>(ptr))->x_val;
    }

    float getMinimum()
    {
        return (static_cast<t_my_numbox*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return (static_cast<t_my_numbox*>(ptr))->x_max;
    }

    void setMinimum(float value)
    {
        static_cast<t_my_numbox*>(ptr)->x_min = value;
    }

    void setMaximum(float value)
    {
        static_cast<t_my_numbox*>(ptr)->x_max = value;
    }
};
