

typedef struct _edit_proxy {
    t_object p_obj;
    t_symbol* p_sym;
    t_clock* p_clock;
    struct _button* p_cnv;
} t_edit_proxy;

typedef struct _button {
    t_object x_obj;
    t_glist* x_glist;
    t_edit_proxy* x_proxy;
    t_symbol* x_bindname;
    int x_x;
    int x_y;
    int x_w;
    int x_h;
    int x_sel;
    int x_zoom;
    int x_edit;
    int x_state;
    unsigned char x_bgcolor[3];
    unsigned char x_fgcolor[3];
} t_fake_button;

struct ButtonObject : public GUIObject {

    bool state = false;
    bool alreadyTriggered = false;

    ButtonObject(void* obj, Box* parent)
        : GUIObject(obj, parent)
    {
    }

    void checkBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, box->getWidth());
        if (size != box->getHeight() || size != box->getWidth()) {
            box->setSize(size, size);
        }
    }

    void updateParameters() override
    {
        auto* button = static_cast<t_fake_button*>(ptr);

        primaryColour = Colour(button->x_fgcolor[0], button->x_fgcolor[1], button->x_fgcolor[2]).toString();
        secondaryColour = Colour(button->x_bgcolor[0], button->x_bgcolor[1], button->x_bgcolor[2]).toString();

        auto params = getParameters();
        for (auto& [name, type, cat, value, list] : params) {
            value->addListener(this);

            // Push current parameters to pd
            valueChanged(*value);
        }

        repaint();
    }

    /*
    void toggleObject(Point<int> position) override
    {

        if (!alreadyBanged) {

            auto* button = static_cast<t_fake_button*>(ptr);
            outlet_float(button->x_obj.ob_outlet, 1);
            update();
            alreadyBanged = true;
        }
    }
    void untoggleObject() override
    {

        if(alreadyBanged)
        {
            auto* button = static_cast<t_fake_button*>(ptr);
            outlet_float(button->x_obj.ob_outlet, 0);
            update();
        }
        alreadyBanged = false;
    }*/

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->getCallbackLock()->exit();

        box->setObjectBounds(bounds);
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* button = static_cast<t_fake_button*>(ptr);
        button->x_w = b.getWidth();
        button->x_h = b.getHeight();
    }

    void mouseDown(MouseEvent const& e) override
    {
        auto* button = static_cast<t_fake_button*>(ptr);
        outlet_float(button->x_obj.ob_outlet, 1);
        state = true;

        // Make sure we don't re-click with an accidental drag
        alreadyTriggered = true;

        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        alreadyTriggered = false;
        state = false;
        auto* button = static_cast<t_fake_button*>(ptr);
        outlet_float(button->x_obj.ob_outlet, 0);
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat();

        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(bounds.reduced(1), 3.0f);

        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 1.0f);
        g.drawRoundedRectangle(bounds.reduced(6), 3.0f, 2.0f);

        if (state) {
            g.setColour(Colour::fromString(primaryColour.toString()));
            g.fillRoundedRectangle(bounds.reduced(6), 3.0f);
        }
    }

    ObjectParameters defineParameters() override
    {
        return {
            { "Foreground", tColour, cAppearance, &primaryColour, {} },
            { "Background", tColour, cAppearance, &secondaryColour, {} },
        };
    }

    void valueChanged(Value& value) override
    {
        auto* button = static_cast<t_fake_button*>(ptr);
        if (value.refersToSameSourceAs(primaryColour)) {
            button->x_fgcolor[0] = Colour::fromString(primaryColour.toString()).getRed();
            button->x_fgcolor[1] = Colour::fromString(primaryColour.toString()).getGreen();
            button->x_fgcolor[2] = Colour::fromString(primaryColour.toString()).getBlue();
            repaint();
        }
        if (value.refersToSameSourceAs(secondaryColour)) {
            button->x_bgcolor[0] = Colour::fromString(secondaryColour.toString()).getRed();
            button->x_bgcolor[1] = Colour::fromString(secondaryColour.toString()).getGreen();
            button->x_bgcolor[2] = Colour::fromString(secondaryColour.toString()).getBlue();
            repaint();
        }
    }
};
