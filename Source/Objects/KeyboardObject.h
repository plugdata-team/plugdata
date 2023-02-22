/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <JuceHeader.h>

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

    /*  Return the amount of white notes in the current displayed range.
     *  We use this to calculate & resize the keyboard width when more range is added
     *  because setKeyWidth sets the width of white keys
     */
    int getCountOfWhiteNotesInRange()
    {
        /*
        ┌──┬─┬─┬─┬──┬──┬─┬─┬─┬─┬─┬──┐
        │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
        │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
        │  └┼┘ └┼┘  │  └┼┘ └┼┘ └┼┘  │
        │ 0 │ 2 │ 4 │ 5 │ 7 │ 9 │11 │
        └───┴───┴───┴───┴───┴───┴───┘
        */
        int count = 0;
        for (int i = getRangeStart(); i <= getRangeEnd(); i++) {
            if (i % 12 == 0 || i % 12 == 2 || i % 12 == 4 || i % 12 == 5 || i % 12 == 7 || i % 12 == 9 || i % 12 == 11) {
                count++;
            }
        }
        return count;
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override
    {
        // TODO: this should be a theme preference, or setting for keyboard
        // yeah but we can set a less ugly default colour for now!

        auto c = Colour(225, 225, 225);
        if (isOver)
            c = Colour(235, 235, 235);
        if (isDown)
            c = object->findColour(PlugDataColour::dataColourId);

        area = area.reduced(0.0f, 0.5f);

        g.setColour(c);

        // Rounded first and last keys to fix objects
        if (midiNoteNumber == getRangeStart()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), PlugDataLook::objectCornerRadius, PlugDataLook::objectCornerRadius, true, false, true, false);

            g.fillPath(keyPath);
        } else if (midiNoteNumber == getRangeEnd()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), PlugDataLook::objectCornerRadius, PlugDataLook::objectCornerRadius, false, true, false, true);

            g.fillPath(keyPath);
        } else {
            g.fillRect(area);
        }

        // don't draw the first separator line to fix object look
        if (midiNoteNumber != getRangeStart()) {
            g.setColour(object->findColour(PlugDataColour::outlineColourId));
            g.fillRect(area.withWidth(1.0f));
        }

        // draw C octave numbers
        if (!(midiNoteNumber % 12)) {
            Array<int> glyphs;
            Array<float> offsets;
            auto font = Fonts::getCurrentFont();
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

            // TODO: C octave number text colour should be a theme prefernece or setting
            g.setColour(Colour(90, 90, 90));
            g.fillPath(outline, outline.getTransformToScaleToFit(rectangle, true));
        }
    }

    void drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override
    {
        // TODO: this should be a theme preference, or setting for keyboard
        auto c = Colour(90, 90, 90);

        if (isOver)
            c = Colour(101, 101, 101);
        if (isDown)
            c = object->findColour(PlugDataColour::dataColourId).darker(0.5f);

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
    int numWhiteKeys = 0;

    MidiKeyboardState state;
    MIDIKeyboard keyboard;
    int keyRatio = 5;

public:
    KeyboardObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , keyboard(object, state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setMidiChannel(1);
        keyboard.setScrollButtonsVisible(false);

        state.addListener(this);

        addAndMakeVisible(keyboard);

        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);
        lowC.setValue(elseKeyboard->x_low_c);
        octaves.setValue(elseKeyboard->x_octaves);

        valueChanged(lowC);
        valueChanged(octaves);

        startTimer(150);

        updateBounds();
    }

    void updateBounds() override
    {
        pd->lockAudioThread();

        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);
        auto bounds = Rectangle<int>(x, y, keyboard.getWidth(), elseKeyboard->x_height);

        pd->unlockAudioThread();

        object->setObjectBounds(bounds);
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);
        elseKeyboard->x_height = b.getHeight();
    }

    void resized() override
    {
        float keyWidth = static_cast<float>(object->getHeight() - Object::doubleMargin) / keyRatio;

        keyboard.setKeyWidth(keyWidth);
        keyboard.setSize(keyWidth * numWhiteKeys, object->getHeight() - Object::doubleMargin);
        applyBounds();
    }

    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        if (midiChannel != 1)
            return;

        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);

        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), elseKeyboard, note, velocity]() mutable {
                if (!_this || _this->cnv->patch.objectWasDeleted(elseKeyboard))
                    return;

                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, velocity * 127);

                outlet_list(elseKeyboard->x_out, &s_list, ac, at);
                if (elseKeyboard->x_send != &s_ && elseKeyboard->x_send->s_thing)
                    pd_list(elseKeyboard->x_send->s_thing, &s_list, ac, at);
            });
    }

    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        if (midiChannel != 1)
            return;

        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);

        cnv->pd->enqueueFunction(
            [_this = SafePointer(this), elseKeyboard, note]() mutable {
                if (!_this || _this->cnv->patch.objectWasDeleted(elseKeyboard))
                    return;

                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, 0);

                outlet_list(elseKeyboard->x_out, &s_list, ac, at);
                if (elseKeyboard->x_send != &s_ && elseKeyboard->x_send->s_thing)
                    pd_list(elseKeyboard->x_send->s_thing, &s_list, ac, at);
            });
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Start octave", tInt, cGeneral, &lowC, {} },
            { "Num. octaves", tInt, cGeneral, &octaves, {} }
        };
    }

    void updateAspectRatio()
    {
        int numOctaves = static_cast<int>(octaves.getValue());
        int lowest = static_cast<int>(lowC.getValue());
        int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
        keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12) - 1, 127));

        float horizontalLength = keyboard.getTotalKeyboardWidth();

        // we only need to get the amount of white notes when the number of keys has changed
        numWhiteKeys = keyboard.getCountOfWhiteNotesInRange();

        object->setSize(horizontalLength + Object::doubleMargin, object->getHeight());
        object->constrainer->setFixedAspectRatio(horizontalLength / static_cast<float>(object->getHeight() - Object::doubleMargin));
        object->constrainer->setMinimumSize((object->minimumSize / 5.0f) * numWhiteKeys, object->minimumSize);
    }

    void valueChanged(Value& value) override
    {
        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);

        if (value.refersToSameSourceAs(lowC)) {
            lowC = std::clamp<int>(static_cast<int>(lowC.getValue()), -1, 9);
            elseKeyboard->x_low_c = static_cast<int>(lowC.getValue());
            updateAspectRatio();
        } else if (value.refersToSameSourceAs(octaves)) {
            octaves = std::clamp<int>(static_cast<int>(octaves.getValue()), 1, 11);
            elseKeyboard->x_octaves = static_cast<int>(octaves.getValue());
            updateAspectRatio();
        }
    }

    void updateValue()
    {
        auto* elseKeyboard = static_cast<t_keyboard*>(ptr);

        for (int i = keyboard.getRangeStart(); i < keyboard.getRangeEnd(); i++) {
            if (elseKeyboard->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOn(2, i, 1.0f);
            }
            if (!elseKeyboard->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOff(2, i, 1.0f);
            }
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            updateValue();
            break;
        }
        case hash("lowc"): {
            setParameterExcludingListener(lowC, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        case hash("oct"): {
            setParameterExcludingListener(lowC, std::clamp<int>(static_cast<int>(lowC.getValue()) + static_cast<int>(atoms[0].getFloat()), -1, 9));
            updateAspectRatio();
            break;
        }
        case hash("8ves"): {
            setParameterExcludingListener(octaves, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        default:
            break;
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
