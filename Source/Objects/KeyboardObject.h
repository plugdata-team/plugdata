/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

// ELSE keyboard
class KeyboardObject final : public ObjectBase
    , public Timer {

    Value lowC = SynchronousValue();
    Value octaves = SynchronousValue();
    Value keyWidth = SynchronousValue();

    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value toggleMode = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    int lastKey = -1;
    int clickedKey = -1;

    UnorderedSet<int> heldKeys;
    UnorderedSet<int> toggledKeys;

    static constexpr uint8 whiteNotes[] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr uint8 blackNotes[] = { 1, 3, 6, 8, 10 };

public:
    KeyboardObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        objectParameters.addParamInt("Height", cDimensions, &sizeProperty);
        objectParameters.addParamInt("Start octave", cGeneral, &lowC, 2);
        objectParameters.addParamInt("Num. octaves", cGeneral, &octaves, 4);
        objectParameters.addParamInt("Key width", cGeneral, &keyWidth, 4);
        objectParameters.addParamBool("Toggle Mode", cGeneral, &toggleMode, { "Off", "On" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);

        startTimer(50);
    }

    void onConstrainerCreate() override
    {
        updateMinimumSize();
    }

    void update() override
    {
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            lowC.setValue(obj->x_low_c);
            octaves.setValue(obj->x_octaves);
            keyWidth.setValue(obj->x_space);
            toggleMode.setValue(obj->x_toggle_mode);
            sizeProperty.setValue(obj->x_height);

            auto sndSym = obj->x_snd_set ? String::fromUTF8(obj->x_snd_raw->s_name) : getBinbufSymbol(7);
            auto rcvSym = obj->x_rcv_set ? String::fromUTF8(obj->x_rcv_raw->s_name) : getBinbufSymbol(8);

            sendSymbol = sndSym != "empty" ? sndSym : "";
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    // Call async to make sure pd obj has updated
                    _this->object->updateBounds();
                }
            });
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds();

        bool const selected = object->isSelected() && !cnv->isGraph;
        auto const outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));

        auto const strokeColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour));
        auto const whiteKeyColour = nvgRGB(225, 225, 225);
        auto const blackKeyColour = nvgRGB(90, 90, 90);
        auto const activeKeyColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::dataColourId);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), whiteKeyColour, outlineColour, Corners::objectCornerRadius);

        nvgStrokeColor(nvg, strokeColour);

        auto const whiteNoteWidth = getWhiteKeyWidth();
        auto const blackNoteWidth = whiteNoteWidth * 0.7f;
        auto const blackKeyHeight = (getHeight() - 2) * 0.66f;
        auto const numWhiteNotes = getNumWhiteKeys();
        auto const numBlackNotes = getNumBlackKeys();
        auto const startOctave = getValue<int>(lowC);
        auto const lowest = startOctave * 12;
        auto const highest = lowest + getValue<int>(octaves) * 12;

        // Fill held white notes
        if (!heldKeys.empty()) {
            nvgBeginPath(nvg);
            for (auto& key : heldKeys) {
                if (key < lowest || key >= highest)
                    continue;
                auto pos = getKeyPosition(key - lowest, true);
                if (!MidiMessage::isMidiNoteBlack(key)) {
                    nvgRect(nvg, pos.getStart(), 1.0f, whiteNoteWidth, getHeight() - 2.0f);
                }
            }
            nvgFillColor(nvg, convertColour(activeKeyColour));
            nvgFill(nvg);
        }

        // Draw outlines for white notes
        nvgBeginPath(nvg);
        for (int i = 1; i < numWhiteNotes; i++) {
            nvgMoveTo(nvg, i * whiteNoteWidth, 1.0f);
            nvgLineTo(nvg, i * whiteNoteWidth, getHeight() - 1.0f);
        }
        nvgStroke(nvg);

        // Fill black notes
        nvgBeginPath(nvg);
        for (int i = 0; i < numBlackNotes; i++) {
            auto const octave = i / 5 * 12;
            auto pos = getKeyPosition(blackNotes[i % 5] + octave, true);
            nvgRect(nvg, pos.getStart(), 1.0f, blackNoteWidth, blackKeyHeight);
        }

        nvgFillColor(nvg, blackKeyColour);
        nvgFill(nvg);

        // Fill held black notes
        if (!heldKeys.empty()) {
            nvgBeginPath(nvg);
            for (auto& key : heldKeys) {
                if (key < lowest || key >= highest)
                    continue;
                auto pos = getKeyPosition(key - lowest, true);
                if (MidiMessage::isMidiNoteBlack(key)) {
                    nvgRect(nvg, pos.getStart(), 1.0f, blackNoteWidth, blackKeyHeight);
                }
            }
            nvgFillColor(nvg, convertColour(activeKeyColour.darker(0.5f)));
            nvgFill(nvg);
        }

        // Draw octave numbers
        if (!cnv->locked.getValue() && !cnv->editor->isInPluginMode()) {
            nvgFillColor(nvg, nvgRGB(90, 90, 90));
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            auto const fontSizeScaled = b.getHeight() - 2 < 60 ? 13.0f * (b.getHeight() - 2) / 60.0f : 13;
            nvgFontSize(nvg, jmax(4.0f, fontSizeScaled));
            auto const octaveNumHeight = whiteNoteWidth * 1.2f;
            auto const scaledHeight = jmin(13.0f, b.getHeight() - 2 < 60 ? octaveNumHeight * (b.getHeight() - 2.0f) / 60.0f : octaveNumHeight);
            for (int i = 0; i < getValue<int>(octaves); i++) {
                auto const position = i * 7 * whiteNoteWidth;
                auto text = String(i + startOctave);
                auto rectangle = Rectangle<int>(position, b.getHeight() - scaledHeight, whiteNoteWidth, scaledHeight);
                nvgText(nvg, rectangle.getCentreX(), rectangle.getCentreY(), text.toRawUTF8(), nullptr);
            }
        }
    }

    void updateSizeProperty() override
    {
        if (auto keyboard = ptr.get<t_fake_keyboard>()) {
            setParameterExcludingListener(sizeProperty, object->getObjectBounds().getHeight());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x, y, w, h;
            pd::Interface::getObjectBounds(patch, obj.cast<t_gobj>(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, obj->x_space * getNumWhiteKeys(), obj->x_height);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_fake_keyboard>()) {
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, gobj.cast<t_gobj>(), b.getX(), b.getY());
            gobj->x_height = b.getHeight();
        }
    }

    void resized() override
    {
        auto const numWhiteKeys = getNumWhiteKeys();
        auto const newKeyWidth = static_cast<int>(getWidth() / numWhiteKeys);

        if (newKeyWidth > 7) {
            keyWidth.setValue(newKeyWidth);
            object->setSize(static_cast<int>(numWhiteKeys * getWhiteKeyWidth()) + Object::doubleMargin, object->getHeight());
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const height = std::max(getValue<int>(sizeProperty), constrainer->getMinimumHeight());
            setParameterExcludingListener(sizeProperty, height);
            if (auto keyboard = ptr.get<t_fake_keyboard>()) {
                keyboard->x_height = height;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(lowC)) {
            auto const lowest = limitValueRange(lowC, -1, 9);
            if (auto obj = ptr.get<t_fake_keyboard>())
                obj->x_low_c = lowest;
            repaint();
        } else if (value.refersToSameSourceAs(keyWidth)) {
            auto const width = limitValueMin(keyWidth, 7);
            if (auto obj = ptr.get<t_fake_keyboard>())
                obj->x_space = width;
            object->updateBounds();
        } else if (value.refersToSameSourceAs(octaves)) {
            auto const oct = limitValueRange(octaves, 1, 11);
            if (auto obj = ptr.get<t_fake_keyboard>())
                obj->x_octaves = oct;
            updateMinimumSize();
            object->updateBounds();
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto const symbol = sendSymbol.toString();
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "send", { pd->generateSymbol(symbol) });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto const symbol = receiveSymbol.toString();
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "receive", { pd->generateSymbol(symbol) });
        } else if (value.refersToSameSourceAs(toggleMode)) {
            auto const toggle = getValue<int>(toggleMode);
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "toggle", { static_cast<float>(toggle) });
        }
    }

    void updateValue()
    {
        StackArray<int, 256> notes;
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            memcpy(notes.data(), obj->x_tgl_notes + 12, 244 * sizeof(int));
        }

        auto const numOctaves = getValue<int>(octaves) * 12;
        auto const lowest = getValue<int>(lowC) * 12;

        for (int i = lowest; i <= lowest + numOctaves; i++) {
            if (notes[i] && !heldKeys.contains(i)) {
                heldKeys.insert(i);
                repaint();
            }
            if (!notes[i] && heldKeys.contains(i) && clickedKey != i && !getValue<bool>(toggleMode)) {
                heldKeys.erase(i);
                repaint();
            }
        }
    }

    void receiveNoteOn(int const midiNoteNumber, bool const isOn)
    {
        if (isOn)
            heldKeys.insert(midiNoteNumber - 12);
        else
            heldKeys.erase(midiNoteNumber - 12);

        repaint();
    }

    void receiveNotesOn(SmallArray<pd::Atom> const& atoms, bool const isOn)
    {
        for (int at = 0; at < atoms.size(); at++) {
            if (isOn)
                heldKeys.insert(atoms[at].getFloat() - 12);
            else
                heldKeys.erase(atoms[at].getFloat() - 12);
        }
        repaint();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        auto elseKeyboard = ptr.get<t_fake_keyboard>();

        switch (symbol) {
        case hash("float"): {
            auto const note = std::clamp<int>(atoms[0].getFloat(), 0, 128);
            receiveNoteOn(atoms[0].getFloat(), elseKeyboard->x_tgl_notes[note]);
            break;
        }
        case hash("list"): {
            if (atoms.size() == 2) {
                receiveNoteOn(atoms[0].getFloat(), atoms[1].getFloat() > 0);
            }
            break;
        }
        case hash("set"): {
            // not implemented yet
            break;
        }
        case hash("on"): {
            receiveNotesOn(atoms, true);
            break;
        }
        case hash("off"): {
            receiveNotesOn(atoms, false);
            break;
        }
        case hash("lowc"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(lowC, static_cast<int>(atoms[0].getFloat()));
            repaint();
            break;
        }
        case hash("width"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(keyWidth, static_cast<int>(atoms[0].getFloat()));
            object->updateBounds();
            break;
        }
        case hash("oct"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(lowC, std::clamp<int>(getValue<int>(lowC) + static_cast<int>(atoms[0].getFloat()), -1, 9));
            object->updateBounds();
            break;
        }
        case hash("8ves"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(octaves, static_cast<int>(atoms[0].getFloat()));
            object->updateBounds();
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("toggle"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(toggleMode, atoms[0].getFloat());
        }
        case hash("flush"): {

            resetToggledKeys();
        }
        default:
            break;
        }
    }

    void updateMinimumSize()
    {
        if (auto* constrainer = getConstrainer()) {
            constrainer->setMinimumSize(8 * getNumWhiteKeys(), 10);
        }
    }

    bool inletIsSymbol() override
    {
        auto const rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && rSymbol != "empty";
    }

    bool outletIsSymbol() override
    {
        auto const sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && sSymbol != "empty";
    }

    void timerCallback() override
    {
        updateValue();
    }

    float getWhiteKeyWidth() const
    {
        return getValue<int>(keyWidth);
    }

    float getNumWhiteKeys() const
    {
        return getValue<int>(octaves) * 7;
    }

    float getNumBlackKeys() const
    {
        return getValue<int>(octaves) * 5;
    }

    Range<float> getKeyPosition(int const midiNoteNumber, bool const isBlackNote) const
    {
        constexpr auto ratio = 0.7f;
        auto const targetKeyWidth = getWhiteKeyWidth();

        static constexpr float notePos[] = { 0.0f, 1 - ratio * 0.6f,
            1.0f, 2 - ratio * 0.4f,
            2.0f,
            3.0f, 4 - ratio * 0.7f,
            4.0f, 5 - ratio * 0.5f,
            5.0f, 6 - ratio * 0.3f,
            6.0f };

        auto const octave = midiNoteNumber / 12;
        auto const note = midiNoteNumber % 12;

        auto start = static_cast<float>(octave) * 7.0f * targetKeyWidth + notePos[note] * targetKeyWidth;
        auto const width = isBlackNote ? ratio * targetKeyWidth : targetKeyWidth;

        return { start, start + width };
    }

    std::pair<int, int> positionToNoteAndVelocity(Point<float> pos) const
    {
        auto constexpr rangeStart = 0;
        auto const rangeEnd = getValue<int>(octaves) * 12;
        auto const whiteNoteLength = getHeight();
        auto const blackNoteLength = 0.7f * whiteNoteLength;

        if (pos.getY() < blackNoteLength) {
            for (int octaveStart = 12 * (rangeStart / 12); octaveStart <= rangeEnd; octaveStart += 12) {
                for (int i = 0; i < 5; ++i) {
                    auto note = octaveStart + blackNotes[i];

                    if (rangeStart <= note && note <= rangeEnd) {
                        if (getKeyPosition(note, true).contains(pos.x)) {
                            return { note, jmax(0.0f, pos.y / blackNoteLength) * 127 };
                        }
                    }
                }
            }
        }

        for (int octaveStart = 12 * (rangeStart / 12); octaveStart <= rangeEnd; octaveStart += 12) {
            for (int i = 0; i < 7; ++i) {
                auto note = octaveStart + whiteNotes[i];

                if (note >= rangeStart && note <= rangeEnd) {
                    if (getKeyPosition(note, false).contains(pos.x)) {
                        return { note, jmax(0.0f, pos.y / static_cast<float>(whiteNoteLength)) * 127 };
                    }
                }
            }
        }

        return { -1, 0 };
    }

    void mouseDown(MouseEvent const& e) override
    {
        auto [midiNoteNumber, midiNoteVelocity] = positionToNoteAndVelocity(e.position);
        midiNoteNumber += getValue<int>(lowC) * 12;

        clickedKey = midiNoteNumber;

        if (e.mods.isShiftDown()) {
            if (toggledKeys.count(midiNoteNumber)) {
                toggledKeys.erase(midiNoteNumber);
                sendNoteOff(midiNoteNumber);
            } else {
                toggledKeys.insert(midiNoteNumber);
                sendNoteOn(midiNoteNumber, midiNoteVelocity);
            }
        } else if (getValue<bool>(toggleMode)) {
            if (heldKeys.count(midiNoteNumber)) {
                heldKeys.erase(midiNoteNumber);
                sendNoteOff(midiNoteNumber);
            } else {
                heldKeys.insert(midiNoteNumber);
                lastKey = midiNoteNumber;
                sendNoteOn(midiNoteNumber, midiNoteVelocity);
            }
        } else {
            heldKeys.insert(midiNoteNumber);
            lastKey = midiNoteNumber;

            sendNoteOn(midiNoteNumber, midiNoteVelocity);
        }

        repaint();
    }

    void resetToggledKeys()
    {
        for (auto const key : toggledKeys) {
            sendNoteOff(key);
        }
        toggledKeys.clear();
        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        auto [midiNoteNumber, midiNoteVelocity] = positionToNoteAndVelocity(e.position);
        midiNoteNumber += getValue<int>(lowC) * 12;

        clickedKey = midiNoteNumber;

        if (!getValue<bool>(toggleMode) && !e.mods.isShiftDown() && !heldKeys.count(midiNoteNumber)) {
            for (auto& note : heldKeys) {
                sendNoteOff(note);
            }
            if (lastKey != midiNoteNumber) {
                heldKeys.erase(lastKey);
            }

            lastKey = midiNoteNumber;

            heldKeys.insert(midiNoteNumber);
            sendNoteOn(midiNoteNumber, midiNoteVelocity);

            repaint();
        }
    }

    // When dragging over the keyboard, the cursor may leave the keyboard object.
    // If the user ends the drag action (mouse up) when not over the keyboard object,
    // the keyboard will not register the mouse up, and the key will be stuck on.
    // This could possibly be a bug in juce.
    // So we completely replace mouseUpOnKey functionality here, mouseUp() will stop mouseUpOnKey() being called.
    void mouseUp(MouseEvent const& e) override
    {
        clickedKey = -1;

        if (!getValue<bool>(toggleMode) && !e.mods.isShiftDown()) {
            heldKeys.erase(lastKey);
            sendNoteOff(lastKey);
        }
        repaint();
    }

    void sendNoteOn(int note, int const velocity)
    {
        note = std::clamp(note + 12, 0, 255);

        StackArray<t_atom, 2> at;
        SETFLOAT(&at[0], note);
        SETFLOAT(&at[1], velocity);

        if (auto obj = this->ptr.get<t_fake_keyboard>()) {
            obj->x_tgl_notes[note] = 1;
            outlet_list(obj->x_out, gensym("list"), 2, at.data());
            if (obj->x_send != gensym("") && obj->x_send->s_thing)
                pd_list(obj->x_send->s_thing, gensym("list"), 2, at.data());
        }
    }

    void sendNoteOff(int note)
    {
        note = std::clamp(note + 12, 0, 255);
        StackArray<t_atom, 2> at;
        SETFLOAT(&at[0], note);
        SETFLOAT(&at[1], 0);

        if (auto obj = this->ptr.get<t_fake_keyboard>()) {
            obj->x_tgl_notes[note] = 0;
            outlet_list(obj->x_out, gensym("list"), 2, at.data());
            if (obj->x_send != gensym("") && obj->x_send->s_thing)
                pd_list(obj->x_send->s_thing, gensym("list"), 2, at.data());
        }
    }
};
