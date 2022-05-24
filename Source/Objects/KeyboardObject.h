
// Inherit to customise drawing
struct MIDIKeyboard : public MidiKeyboardComponent
{
    MIDIKeyboard(MidiKeyboardState& stateToUse, Orientation orientationToUse) : MidiKeyboardComponent(stateToUse, orientationToUse)
    {
        // Make sure nothing is drawn outside of our custom draw functions
        setColour(MidiKeyboardComponent::whiteNoteColourId, Colours::transparentWhite);
        setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colours::transparentWhite);
        setColour(MidiKeyboardComponent::keyDownOverlayColourId, Colours::transparentWhite);
        setColour(MidiKeyboardComponent::textLabelColourId, Colours::transparentWhite);
        setColour(MidiKeyboardComponent::shadowColourId, Colours::transparentWhite);
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override
    {
        auto c = Colour(225, 225, 225);
        if (isOver) c = Colour(235, 235, 235);
        if (isDown) c = Colour(245, 245, 245);

        area = area.reduced(0.0f, 0.5f);

        g.setColour(c);

        // Rounded first and last keys to fix boxes
        if (midiNoteNumber == getRangeStart())
        {
            // area = area.expanded(0.0f, -0.5f);
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX() + 0.5f, area.getY(), area.getWidth() - 0.5f, area.getHeight(), 2.0f, 2.0f, true, false, true, false);

            g.fillPath(keyPath);
            return;  // skip drawing outline for first key
        }
        if (midiNoteNumber == getRangeEnd())
        {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth() - 3.5f, area.getHeight(), 2.0f, 2.0f, false, true, false, true);

            g.fillPath(keyPath);
        }
        else
        {
            area = area.expanded(0.0f, -0.5f);
            g.fillRect(area);
        }

        g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
        g.fillRect(area.withWidth(1.0f));
    }

    void drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override
    {
        auto c = Colour(90, 90, 90);

        if (isOver) c = Colour(101, 101, 101);
        if (isDown) c = Colour(60, 60, 60);

        g.setColour(c);
        g.fillRect(area);
    }
};
// ELSE keyboard
struct KeyboardObject final : public GUIObject, public MidiKeyboardStateListener
{
    typedef struct _edit_proxy
    {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _keyboard* p_cnv;
    } t_edit_proxy;

    typedef struct _keyboard
    {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int* x_tgl_notes;  // to store which notes should be played
        int x_velocity;    // to store velocity
        int x_last_note;   // to store last note
        float x_vel_in;    // to store the second inlet values
        float x_space;
        int x_width;
        int x_height;
        int x_octaves;
        int x_first_c;
        int x_low_c;
        int x_toggle_mode;
        int x_norm;
        int x_zoom;
        int x_shift;
        int x_xpos;
        int x_ypos;
        int x_snd_set;
        int x_rcv_set;
        int x_flag;
        int x_s_flag;
        int x_r_flag;
        int x_edit;
        t_symbol* x_receive;
        t_symbol* x_rcv_raw;
        t_symbol* x_send;
        t_symbol* x_snd_raw;
        t_symbol* x_bindsym;
        t_outlet* x_out;
    } t_keyboard;

    KeyboardObject(void* ptr, Box* box) : GUIObject(ptr, box), keyboard(state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setAvailableRange(36, 83);
        keyboard.setScrollButtonsVisible(false);

        state.addListener(this);

        addAndMakeVisible(keyboard);

        auto* x = (t_keyboard*)ptr;
        x->x_width = width * 0.595f;

        lowC = x->x_low_c;
        octaves = x->x_octaves;

        if (static_cast<int>(lowC.getValue()) == 0 || static_cast<int>(octaves.getValue()) == 0)
        {
            lowC = 3;
            octaves = 4;
        }

        initialise();
    }

    void updateBounds() override
    {
        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto* keyboard = static_cast<t_keyboard*>(ptr);
        box->setObjectBounds({x, y, keyboard->x_width, keyboard->x_height});
    }

    void checkBounds() override
    {
        int numKeys = static_cast<int>(octaves.getValue()) * 12;
        float ratio = numKeys / 9.55f;

        auto* keyboardObject = static_cast<t_keyboard*>(ptr);

        if (box->getWidth() / ratio != box->getHeight())
        {
            box->setSize(box->getHeight() * ratio, box->getHeight());

            if (getWidth() > 0)
            {
                keyboard.setKeyWidth(getWidth() / (numKeys * 0.6f));
                keyboardObject->x_width = getWidth();
            }
        }
    }

    void applyBounds() override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), box->getX() + Box::margin, box->getY() + Box::margin);

        auto* keyboard = static_cast<t_keyboard*>(ptr);
        keyboard->x_width = getWidth();
        keyboard->x_height = getHeight();
    }

    void resized() override
    {
        keyboard.setBounds(getLocalBounds());
    }

    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)ptr;

        cnv->pd->enqueueFunction(
            [x, note, velocity]() mutable
            {
                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, velocity * 127);

                outlet_list(x->x_out, &s_list, ac, at);
                if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
            });
    }

    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)ptr;

        cnv->pd->enqueueFunction(
            [x, note]() mutable
            {
                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, 0);

                outlet_list(x->x_out, &s_list, ac, at);
                if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
            });
    };

    ObjectParameters defineParameters() override
    {
        return {{"Lowest note", tInt, cGeneral, &lowC, {}}, {"Num. octaves", tInt, cGeneral, &octaves, {}}};
    };

    void valueChanged(Value& value) override
    {
        auto* keyboardObject = static_cast<t_keyboard*>(ptr);

        if (value.refersToSameSourceAs(lowC))
        {
            int numOctaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 0, 10);
            int lowest = std::clamp<int>(lowC.getValue(), 0, 10);
            int highest = std::clamp<int>(lowest + numOctaves, 0, 10);
            keyboard.setAvailableRange(lowest * 12, highest * 12);
            
            keyboardObject->x_low_c = lowest;
            checkBounds();
        }
        else if (value.refersToSameSourceAs(octaves))
        {
            octaves = std::clamp<int>(octaves.getValue(), 0, 10);
            int numOctaves = static_cast<int>(octaves.getValue());
            int lowest = std::clamp<int>(lowC.getValue(), 0, 10);
            int highest = std::clamp<int>(lowest + numOctaves, 0, 10);
                                          
            keyboard.setAvailableRange(lowest * 12, highest * 12);
            keyboardObject->x_octaves = numOctaves;
            checkBounds();
        }
    }

    Value lowC;
    Value octaves;

    MidiKeyboardState state;
    MIDIKeyboard keyboard;
};
