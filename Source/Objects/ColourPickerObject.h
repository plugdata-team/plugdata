/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/ColourPicker.h"

class ColourPickerObject final : public TextBase {
public:
    ColourPickerObject(pd::WeakReference ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (getValue<bool>(object->locked)) {
            showColourPicker();
        }
    }

    void showColourPicker()
    {
        unsigned int red = 0, green = 0, blue = 0;
        if (auto colors = ptr.get<t_fake_colors>()) {
            sscanf(colors->x_color, "#%02x%02x%02x", &red, &green, &blue);
        }

        ColourPicker::getInstance().show(getTopLevelComponent(), true, Colour(red, green, blue), object->getScreenBounds(), [_this = SafePointer(this)](Colour c) {
            if (!_this)
                return;

            if (auto colors = _this->ptr.get<t_fake_colors>()) {
                outlet_symbol(colors->x_obj.te_outlet, _this->pd->generateSymbol(String("#") + c.toString().substring(2)));
                snprintf(colors->x_color, 1000, "#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue());
            }
        });
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("pick")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {

        case hash("pick"): {
            showColourPicker();
            break;
        }
        }
    }
};
