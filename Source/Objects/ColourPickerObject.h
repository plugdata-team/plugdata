/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/ColourPicker.h"

struct t_fake_colors {
    t_object x_obj;
    t_int x_hex;
    t_int x_gui;
    t_int x_rgb;
    t_int x_ds;
    t_symbol* x_id;
    char x_color[MAXPDSTRING];
};

class ColourPickerObject final : public TextBase {
public:
    ColourPickerObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked)) {
            showColourPicker();
        }
    }

    void showColourPicker()
    {
        unsigned int red, green, blue;
        auto x = static_cast<t_fake_colors*>(ptr);
        sscanf(x->x_color, "#%02x%02x%02x", &red, &green, &blue);

        ColourPicker::show(getTopLevelComponent(), true, Colour(red, green, blue), object->getScreenBounds(), [_this = SafePointer(this), x](Colour c) {
            if (!_this)
                return;

            _this->pd->enqueueFunction([_this, x, c]() {
                if (!_this || _this->cnv->patch.objectWasDeleted(x))
                    return;

                outlet_symbol(x->x_obj.te_outlet, _this->pd->generateSymbol(String("#") + c.toString().substring(2)));

                snprintf(x->x_color, 1000, "#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue());
            });
        });
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("pick")
        };
    }

    void receiveObjectMessage(hash32 const& symbolHash, std::vector<pd::Atom>& atoms) override
    {
        switch (symbolHash) {

        case hash("pick"): {
            showColourPicker();
            break;
        }
        }
    }
};
