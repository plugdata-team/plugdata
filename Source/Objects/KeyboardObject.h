/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Inherit to customise drawing
class MIDIKeyboard : public MidiKeyboardComponent {

    Object* object;

public:
    MIDIKeyboard(Object* parent, MidiKeyboardState& stateToUse, Orientation orientationToUse)
        : MidiKeyboardComponent(stateToUse, orientationToUse)
        , object(parent)
    {
        // Make sure nothing is drawn outside of our custom draw functions
        setColour(MidiKeyboardComponent::whiteNoteColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keyDownOverlayColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::textLabelColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::shadowColourId, Colours::transparentBlack);
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override
    {
        auto c = Colour(225, 225, 225);
        if (isOver)
            c = Colour(235, 235, 235);
        if (isDown)
            c = Colour(245, 245, 245);

        area = area.reduced(0.0f, 0.5f);

        g.setColour(c);

        // Rounded first and last keys to fix objects
        if (midiNoteNumber == getRangeStart()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX() + 0.5f, area.getY(), area.getWidth() - 0.5f, area.getHeight(), PlugDataLook::objectCornerRadius, PlugDataLook::objectCornerRadius, true, false, true, false);

            g.fillPath(keyPath);
        }
        if (midiNoteNumber == getRangeEnd()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth() - 3.5f, area.getHeight(), PlugDataLook::objectCornerRadius, PlugDataLook::objectCornerRadius, false, true, false, true);

            g.fillPath(keyPath);
        } else {
            area = area.expanded(0.0f, -0.5f);
            g.fillRect(area);
        }

        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        g.fillRect(area.withWidth(1.0f));

        if (!(midiNoteNumber % 12)) {
            Array<int> glyphs;
            Array<float> offsets;
            auto font = Fonts::getDefaultFont();
            Path p;
            Path outline;
            font.getGlyphPositions(String(floor(midiNoteNumber / 12) - 1), glyphs, offsets);

            auto rectangle = area.withTrimmedTop(area.proportionOfHeight(0.8f)).reduced(area.getWidth() / 6.0f);

            int prev_size = 0;
            AffineTransform transform;
            for (auto glyph : glyphs) {
                font.getTypefacePtr()->getOutlineForGlyph(glyph, p);
                if (glyphs.size() > 1) {
                    prev_size = outline.getBounds().getWidth();
                }
                transform = AffineTransform::scale(20).followedBy(AffineTransform::translation(prev_size, 0.0));
                outline.addPath(p, transform);
                p.clear();
            }

            g.setColour(object->findColour(PlugDataColour::canvasTextColourId));
            g.fillPath(outline, outline.getTransformToScaleToFit(rectangle, true));
        }
    }

    void drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override
    {
        auto c = Colour(90, 90, 90);

        if (isOver)
            c = Colour(101, 101, 101);
        if (isDown)
            c = Colour(60, 60, 60);

        g.setColour(c);
        g.fillRect(area);
    }
};
// ELSE keyboard
class KeyboardObject final : public ObjectBase
    , public Timer
    , public MidiKeyboardStateListener {
    typedef struct _edit_proxy {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _keyboard* p_cnv;
    } t_edit_proxy;

    typedef struct _keyboard {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int* x_tgl_notes; // to store which notes should be played
        int x_velocity;   // to store velocity
        int x_last_note;  // to store last note
        float x_vel_in;   // to store the second inlet values
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

    Value lowC;
    Value octaves;
    int numKeys = 0;

    MidiKeyboardState state;
    MIDIKeyboard keyboard;

public:
    KeyboardObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , keyboard(object, state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setMidiChannel(1);
        keyboard.setAvailableRange(36, 83);
        keyboard.setScrollButtonsVisible(false);

        state.addListener(this);

        addAndMakeVisible(keyboard);

        auto* x = (t_keyboard*)ptr;
        x->x_width = getWidth();

        lowC = x->x_low_c;
        octaves = x->x_octaves;

        if (static_cast<int>(lowC.getValue()) == 0 || static_cast<int>(octaves.getValue()) == 0) {
            lowC = 3;
            octaves = 4;
        }

        startTimer(150);
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto* keyboard = static_cast<t_keyboard*>(ptr);
        auto bounds = Rectangle<int>(x, y, keyboard->x_width, keyboard->x_height);

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    void checkBounds() override
    {
        int range_end = keyboard.getRangeEnd();
        if (range_end == 127) {
            range_end = 128;
        }
        numKeys = std::clamp<int>(range_end - keyboard.getRangeStart(), 8, 128);
        float ratio = numKeys / 9.55f;

        auto* keyboardObject = static_cast<t_keyboard*>(ptr);

        if (object->getWidth() / ratio != object->getHeight()) {
            object->setSize(object->getHeight() * ratio, object->getHeight());

            if (getWidth() > 0) {
                keyboard.setKeyWidth(getWidth() / (numKeys * 0.584f));
                keyboardObject->x_width = getWidth();
            }
        }
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* keyboard = static_cast<t_keyboard*>(ptr);
        keyboard->x_width = b.getWidth();
        keyboard->x_height = b.getHeight();
    }

    void resized() override
    {
        auto* keyboardObject = static_cast<t_keyboard*>(ptr);
        keyboard.setSize(getWidth(), getHeight());
        if (getWidth() > 0) {
            keyboard.setKeyWidth(getWidth() / (numKeys * 0.584f));
            keyboardObject->x_width = getWidth();
        }
    }

    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        if (midiChannel != 1)
            return;

        auto* x = (t_keyboard*)ptr;

        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), x, note, velocity]() mutable {
                if(!_this || _this->cnv->patch.objectWasDeleted(x)) return;
                
                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, velocity * 127);

                outlet_list(x->x_out, &s_list, ac, at);
                if (x->x_send != &s_ && x->x_send->s_thing)
                    pd_list(x->x_send->s_thing, &s_list, ac, at);
            });
    }

    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        if (midiChannel != 1)
            return;

        auto* x = (t_keyboard*)ptr;

        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), x, note]() mutable {
                
                if(!_this || _this->cnv->patch.objectWasDeleted(x)) return;
                
                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, 0);

                outlet_list(x->x_out, &s_list, ac, at);
                if (x->x_send != &s_ && x->x_send->s_thing)
                    pd_list(x->x_send->s_thing, &s_list, ac, at);
            });
    };

    ObjectParameters getParameters() override
    {
        return { { "Start octave", tInt, cGeneral, &lowC, {} }, { "Num. octaves", tInt, cGeneral, &octaves, {} } };
    };

    void valueChanged(Value& value) override
    {
        auto* keyboardObject = static_cast<t_keyboard*>(ptr);

        if (value.refersToSameSourceAs(lowC)) {
            int numOctaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 1, 11);
            lowC = std::clamp<int>(static_cast<int>(lowC.getValue()), -1, 9);
            int lowest = static_cast<int>(lowC.getValue());
            int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
            keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12), 127));
            keyboardObject->x_low_c = lowest;
            checkBounds();
        } else if (value.refersToSameSourceAs(octaves)) {
            octaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 1, 11);
            int numOctaves = static_cast<int>(octaves.getValue());
            int lowest = std::clamp<int>(lowC.getValue(), -1, 9);
            int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
            keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12), 127));
            keyboardObject->x_octaves = numOctaves;
            checkBounds();
        }
    }

    void updateValue()
    {
        auto* keyboardObject = static_cast<t_keyboard*>(ptr);

        for (int i = keyboard.getRangeStart(); i < keyboard.getRangeEnd(); i++) {
            if (keyboardObject->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOn(2, i, 1.0f);
            }
            if (!keyboardObject->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOff(2, i, 1.0f);
            }
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        if (symbol == "float") {
            updateValue();
        }
        if (symbol == "list") {
            updateValue();
        }
        if (symbol == "lowc") {
            setParameterExcludingListener(lowC, static_cast<int>(atoms[0].getFloat()));
            int numOctaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 1, 11);
            int lowest = std::clamp<int>(lowC.getValue(), -1, 9);
            int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
            keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12), 127));
            checkBounds();
        } else if (symbol == "8ves") {
            setParameterExcludingListener(octaves, static_cast<int>(atoms[0].getFloat()));
            int numOctaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 1, 11);
            int lowest = std::clamp<int>(lowC.getValue(), -1, 9);
            int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
            keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12), 127));
            checkBounds();
        }
    }

    void timerCallback() override
    {
        pd->enqueueFunction([_this = SafePointer(this)] {
            if (!_this || _this->cnv->patch.objectWasDeleted(_this->ptr))
                return;
            _this->updateValue();
        });
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }
};
