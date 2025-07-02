/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class RadioObject final : public ObjectBase {

    bool alreadyToggled = false;
    bool isVertical;
    int numItems = 0;

    int selected;

    int hoverIdx = -1;
    bool mouseHover = false;

    IEMHelper iemHelper;

    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

public:
    RadioObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamInt("Options", cGeneral, &max, 8, true, 1);
        iemHelper.addIemParameters(objectParameters);

        if (auto radio = ptr.get<t_radio>()) {
            isVertical = radio->x_orientation;
            sizeProperty = isVertical ? radio->x_gui.x_w : radio->x_gui.x_h;
        }
    }

    void update() override
    {
        numItems = getMaximum();
        max = numItems;

        selected = jlimit(0, numItems - 1, static_cast<int>(getValue()));

        if (auto radio = ptr.get<t_radio>()) {
            isVertical = radio->x_orientation;
            sizeProperty = isVertical ? radio->x_gui.x_w : radio->x_gui.x_h;
        }

        iemHelper.update();
    }

    void onConstrainerCreate() override
    {
        updateAspectRatio();
    }

    bool inletIsSymbol() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(labels);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        // radio stores it's height and width as a square to allow changing orientation via message: "orientation 0/1"
        b = isVertical ? b.withHeight(b.getWidth()) : b.withWidth(b.getHeight());
        iemHelper.setPdBounds(b);
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto radio = ptr.get<t_radio>()) {
            auto* patch = cnv->patch.getRawPointer();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, radio.cast<t_gobj>(), &x, &y, &w, &h);
            auto width = !isVertical ? radio->x_gui.x_h * numItems + 1 : radio->x_gui.x_w + 1;
            auto height = isVertical ? radio->x_gui.x_w * numItems + 1 : radio->x_gui.x_h + 1;

            return { x, y, width, height };
        }

        return {};
    }

    void toggleObject(Point<int> position) override
    {
        if (alreadyToggled) {
            alreadyToggled = false;
        }

        float const pos = isVertical ? position.y : position.x;
        float const div = isVertical ? getHeight() : getWidth();

        int const idx = std::clamp<int>(pos / div * numItems, 0, numItems - 1);

        if (idx != selected) {
            startEdition();
            sendFloatValue(idx);
            stopEdition();
            repaint();
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            selected = std::clamp<float>(atoms[0].getFloat(), 0.0f, numItems - 1);
            repaint();
            break;
        }
        case hash("orientation"): {
            if (atoms.size() >= 1) {
                isVertical = static_cast<bool>(atoms[0].getFloat());
                object->updateBounds();
                updateAspectRatio();
            }
            break;
        }
        case hash("number"): {
            if (atoms.size() >= 1)
                max = getMaximum();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
    }

    void mouseMove(MouseEvent const& e) override
    {
        float const pos = isVertical ? e.y : e.x;
        float const div = isVertical ? getHeight() : getWidth();

        hoverIdx = pos / div * numItems;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        float const pos = isVertical ? e.y : e.x;
        float const div = isVertical ? getHeight() : getWidth();

        int const idx = pos / div * numItems;

        alreadyToggled = true;
        startEdition();
        sendFloatValue(idx);
        stopEdition();

        repaint();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        mouseHover = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        mouseHover = false;
        repaint();
    }

    float getValue() const
    {
        if (auto radio = ptr.get<t_radio>())
            return radio->x_on;

        return 0.0f;
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        bool const isSelected = object->isSelected() && !cnv->isGraph;
        auto const selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto const outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(iemHelper.getBackgroundColour()), isSelected ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        float const size = isVertical ? static_cast<float>(getHeight()) / numItems : static_cast<float>(getWidth()) / numItems;

        nvgStrokeColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour)));
        nvgStrokeWidth(nvg, 1.0f);

        nvgBeginPath(nvg);
        for (int i = 1; i < numItems; i++) {
            if (isVertical) {
                nvgMoveTo(nvg, 1, i * size);
                nvgLineTo(nvg, size - 0.5, i * size);
            } else {
                nvgMoveTo(nvg, i * size, 1);
                nvgLineTo(nvg, i * size, size - 0.5);
            }
        }
        nvgStroke(nvg);

        auto const bgColour = ::getValue<Colour>(iemHelper.secondaryColour);

        if (mouseHover) {
            auto const hoverColour = bgColour.contrasting(bgColour.getBrightness() > 0.5f ? 0.03f : 0.05f);
            float const hoverX = isVertical ? 0 : hoverIdx * size;
            float const hoverY = isVertical ? hoverIdx * size : 0;
            auto const hoverBounds = Rectangle<float>(hoverX, hoverY, size, size).reduced(jmin<int>(size * 0.25f, 5));
            nvgFillColor(nvg, convertColour(hoverColour));
            nvgFillRoundedRect(nvg, hoverBounds.getX(), hoverBounds.getY(), hoverBounds.getWidth(), hoverBounds.getHeight(), Corners::objectCornerRadius / 2.0f);
        }

        float const selectionX = isVertical ? 0 : selected * size;
        float const selectionY = isVertical ? selected * size : 0;
        auto const selectionBounds = Rectangle<float>(selectionX, selectionY, size, size).reduced(jmin<int>(size * 0.25f, 5));

        nvgFillColor(nvg, convertColour(::getValue<Colour>(iemHelper.primaryColour)));
        nvgFillRoundedRect(nvg, selectionBounds.getX(), selectionBounds.getY(), selectionBounds.getWidth(), selectionBounds.getHeight(), Corners::objectCornerRadius / 2.0f);
    }

    void updateAspectRatio()
    {
        auto const b = getPdBounds();
        auto const minLongSide = object->minimumSize * numItems;
        constexpr auto minShortSide = Object::minimumSize;
        if (isVertical) {
            float const verticalLength = b.getWidth() * numItems + Object::doubleMargin;
            object->setSize(b.getWidth() + Object::doubleMargin, verticalLength);
            constrainer->setMinimumSize(minShortSide, minLongSide);
        } else {
            float const horizontalLength = b.getHeight() * numItems + Object::doubleMargin;
            object->setSize(horizontalLength, b.getHeight() + Object::doubleMargin);
            constrainer->setMinimumSize(minLongSide, minShortSide);
        }
        constrainer->setFixedAspectRatio(isVertical ? 1.0f / numItems : static_cast<float>(numItems) / 1.0f);
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const size = std::max(::getValue<int>(sizeProperty), isVertical ? constrainer->getMinimumWidth() : constrainer->getMinimumHeight());
            setParameterExcludingListener(sizeProperty, size);

            if (auto radio = ptr.get<t_radio>()) {
                if (isVertical) {
                    radio->x_gui.x_w = size;
                    radio->x_gui.x_h = size;
                } else {
                    radio->x_gui.x_h = size;
                    radio->x_gui.x_w = size;
                }
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(max)) {
            if (::getValue<int>(max) != numItems) {
                numItems = ::getValue<int>(max);
                updateAspectRatio();
                setMaximum(numItems);
            }
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getMaximum() const
    {
        if (auto radio = ptr.get<t_radio>()) {
            return radio->x_number;
        }

        return 0.0f;
    }

    void setMaximum(float const maxValue)
    {
        if (selected >= maxValue) {
            selected = maxValue - 1;
        }

        if (auto radio = ptr.get<t_radio>()) {
            radio->x_number = maxValue;
        }

        resized();
    }

    void updateSizeProperty() override
    {
        if (auto radio = ptr.get<t_radio>()) {
            auto size = isVertical ? object->getWidth() : object->getHeight();
            size -= Object::doubleMargin + 1;

            radio->x_gui.x_w = size;
            radio->x_gui.x_h = size;

            setParameterExcludingListener(sizeProperty, isVertical ? var(radio->x_gui.x_w) : var(radio->x_gui.x_h));
        }
    }
    
    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        // Custom constrainer because a regular ComponentBoundsConstrainer will mess up the aspect ratio
        class RadioObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            RadioObjectBoundsConstrainer() = default;
            
            void checkBounds(Rectangle<int>& bounds,
                             Rectangle<int> const& old,
                             Rectangle<int> const& limits,
                             bool isStretchingTop,
                             bool isStretchingLeft,
                             bool isStretchingBottom,
                             bool isStretchingRight) override
            {
                if (isStretchingLeft)
                    bounds.setLeft (jlimit (old.getRight() - getMaximumWidth(), old.getRight() - getMinimumWidth(), bounds.getX()));
                else
                    bounds.setWidth (jlimit (getMinimumWidth(), getMaximumWidth(), bounds.getWidth()));

                if (isStretchingTop)
                    bounds.setTop (jlimit (old.getBottom() - getMaximumHeight(), old.getBottom() - getMinimumHeight(), bounds.getY()));
                else
                    bounds.setHeight (jlimit (getMinimumHeight(), getMaximumHeight(), bounds.getHeight()));

                if (bounds.isEmpty())
                    return;
                
                const float aspect = getFixedAspectRatio();
                const int margin = Object::margin;

                auto content = bounds.toFloat().reduced(margin + 0.5f);

                bool adjustWidth;

                if ((isStretchingTop || isStretchingBottom) && ! (isStretchingLeft || isStretchingRight))
                {
                    adjustWidth = true;
                }
                else if ((isStretchingLeft || isStretchingRight) && ! (isStretchingTop || isStretchingBottom))
                {
                    adjustWidth = false;
                }
                else
                {
                    const double oldRatio = (old.getHeight() > 0) ? std::abs (old.getWidth() / (double) old.getHeight()) : 0.0;
                    const double newRatio = std::abs (bounds.getWidth() / (double) bounds.getHeight());

                    adjustWidth = (oldRatio > newRatio);
                }

                if (adjustWidth)
                {
                    content.setWidth (roundToInt (content.getHeight() * aspect));
                    content.setHeight (roundToInt (content.getWidth() / aspect));
                }
                else
                {
                    content.setHeight (roundToInt (content.getWidth() / aspect));
                    content.setWidth (roundToInt (content.getHeight() * aspect));
                }

                bounds = content.expanded(margin + 0.5f).toNearestInt();
            }
        };

        return std::make_unique<RadioObjectBoundsConstrainer>();
    }
};
