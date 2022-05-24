#include "../Utility/DraggableNumber.h"

struct FloatAtomObject final : public AtomObject
{
    Label input;
    DraggableNumber dragger;

    FloatAtomObject(void* obj, Box* parent) : AtomObject(obj, parent), dragger(input)
    {
        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            if (editor != nullptr)
            {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]()
        {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };

        addAndMakeVisible(input);

        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);

        initialise();

        input.setEditable(true, false);

        addMouseListener(this, true);

        dragger.dragStart = [this]() { startEdition(); };

        dragger.valueChanged = [this](float value) { setValueOriginal(value); };

        dragger.dragEnd = [this]() { stopEdition(); };
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(30, maxSize, box->getWidth());
        int h = jlimit(Box::height - 12, maxSize, atomSizes[6]);

        h = getBounds().getHeight() + Box::doubleMargin;

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void resized() override
    {
        AtomObject::resized();

        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }

    void update() override
    {
        input.setText(dragger.formatNumber(getValueOriginal()), dontSendNotification);
    }

    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tFloat, cGeneral, &min, {}}, {"Maximum", tFloat, cGeneral, &max, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        }
        else if (value.refersToSameSourceAs(max))
        {
            setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
        else
        {
            AtomObject::valueChanged(value);
        }
    }

    float getValue() override
    {
        return atom_getfloat(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)));
    }

    float getMinimum()
    {
        auto const* gatom = static_cast<t_fake_gatom const*>(ptr);
        if (std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() && std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
        {
            return gatom->a_draglo;
        }
        return -std::numeric_limits<float>::max();
    }

    float getMaximum()
    {
        auto const* gatom = static_cast<t_fake_gatom const*>(ptr);
        if (std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() && std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
        {
            return gatom->a_draghi;
        }
        return std::numeric_limits<float>::max();
    }

    void setMinimum(float value)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        if (std::abs(value) > std::numeric_limits<float>::epsilon() && std::abs(value) > std::numeric_limits<float>::epsilon())
        {
            gatom->a_draglo = value;
        }
    }
    void setMaximum(float value)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        if (std::abs(value) > std::numeric_limits<float>::epsilon() && std::abs(value) > std::numeric_limits<float>::epsilon())
        {
            gatom->a_draghi = value;
        }
    }
};
