/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct t_fake_note
{
    t_object        x_obj;
    t_edit_proxy   *x_proxy;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    t_binbuf       *x_binbuf;
    char           *x_buf;      // text buf
    int             x_bufsize;  // text buf size
    int             x_keynum;
    int             x_init;
    int             x_resized;
    int             x_changed;
    int             x_edit;
    int             x_max_pixwidth;
    int             x_text_width;
    int             x_width;
    int             x_height;
    int             x_bbset;
    int             x_bbpending;
    int             x_x1;
    int             x_y1;
    int             x_x2;
    int             x_y2;
    int             x_newx2;
    int             x_dragon;
    int             x_select;
    int             x_fontsize;
    int             x_shift;
    int             x_selstart;
    int             x_start_ndx;
    int             x_end_ndx;
    int             x_selend;
    int             x_active;
    unsigned char   x_red;
    unsigned char   x_green;
    unsigned char   x_blue;
    unsigned char   x_bg[3]; // background color
    char            x_color[8];
    char            x_bgcolor[8];
    t_symbol       *x_keysym;
    t_symbol       *x_bindsym;
    t_symbol       *x_fontname;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
//    int             x_old;
    int             x_text_flag;
    int             x_text_n;
    int             x_text_size;
    int             x_zoom;
    int             x_fontface;
    int             x_bold;
    int             x_italic;
    int             x_underline;
    int             x_bg_flag;
    int             x_textjust; // 0: left, 1: center, 2: right
    int             x_outline;
    t_pd           *x_handle;
};


class NoteObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };
    
    TextEditor noteEditor;
    
    Value primaryColour;
    Value secondaryColour;
    Value fontSize;
    Value bold;
    
    bool locked;

public:
    NoteObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        textColour = Colour(note->x_red, note->x_green, note->x_blue);
        
        if(note->x_width == 8)
        {
            note->x_width = noteEditor.getTextWidth() + 50;
        }
        
        addAndMakeVisible(noteEditor);
        
        noteEditor.setText(getText());
        
        noteEditor.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        noteEditor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        noteEditor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        noteEditor.setColour(ScrollBar::thumbColourId, object->findColour(PlugDataColour::scrollbarThumbColourId));

        noteEditor.setAlwaysOnTop(true);
        noteEditor.setMultiLine(true);
        noteEditor.setReturnKeyStartsNewLine(true);
        noteEditor.setScrollbarsShown(false);
        noteEditor.setIndents(0, 0);
        noteEditor.setScrollToShowCursor(true);
        noteEditor.setJustification(Justification::centredLeft);
        noteEditor.setBorder(border);
        noteEditor.setBounds(getLocalBounds().withTrimmedRight(5));
        noteEditor.setColour(TextEditor::textColourId, Colour::fromString(primaryColour.toString()));

        noteEditor.addMouseListener(this, true);
        
        noteEditor.onFocusLost = [this](){
            noteEditor.setReadOnly(true);
        };
        
        locked = static_cast<bool>(object->locked.getValue());
        noteEditor.setReadOnly(locked);
        
        noteEditor.onTextChange = [this](){
            auto* x = static_cast<t_fake_note*>(ptr);
            
            std::vector<t_atom> atoms;
            
            auto words = StringArray::fromTokens(noteEditor.getText(), " ", "\"");
            for (int j = 0; j < words.size(); j++) {
                atoms.emplace_back();
                if (words[j].containsOnly("0123456789e.-+") && words[j] != "-") {
                    SETFLOAT(&atoms.back(), words[j].getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(words[j]));
                }
            }
            
            binbuf_clear(x->x_binbuf);
            binbuf_restore(x->x_binbuf, atoms.size(), atoms.data());
            binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
            
            this->object->updateBounds();
        };
    }
    
    void initialiseParameters() override
    {
        auto* note = static_cast<t_fake_note*>(ptr);

        primaryColour = Colour(note->x_color[0], note->x_color[1], note->x_color[2]).toString();
        secondaryColour = Colour(note->x_bg[0], note->x_bg[1], note->x_bg[2]).toString();

        auto params = getParameters();
        for (auto& [name, type, cat, value, list] : params) {
            value->addListener(this);

            // Push current parameters to pd
            valueChanged(*value);
        }

        repaint();
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        if(!locked && cnv->isSelected(object))
        {
            noteEditor.setReadOnly(false);
            noteEditor.grabKeyboardFocus();
        }
    }
    
    void lock(bool isLocked) override
    {
        locked = isLocked;
        //noteEditor.setReadOnly(isLocked);
        repaint();
        
        object->updateIolets();
    }

    void resized() override
    {
        noteEditor.setBounds(getLocalBounds());
    }
    
    bool hideInlets() override
    {
        return locked;
    }

    void paint(Graphics& g) override
    {
        if(!locked) {
            auto bounds = getLocalBounds();
            // Draw background
            g.setColour(Colour::fromString(secondaryColour.toString()));
            g.fillRoundedRectangle(bounds.toFloat().reduced(0.5f), Corners::objectCornerRadius);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        if(!locked) {
            bool selected = cnv->isSelected(object) && !cnv->isGraph;
            auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);
            
            g.setColour(outlineColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        }
    }
    
    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    bool hideInGraph() override
    {
        return false;
    }

    bool canReceiveMouseEvent(int x, int y) override
    {
        return !locked;
    }
    
    bool isEditorShown() override
    {
        return !noteEditor.isReadOnly();
    }
    
    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();
        
        auto* note = static_cast<t_fake_note*>(ptr);
        int maxWidth = note->x_width;
        
        auto numLines = std::max(1, (noteEditor.getTextWidth() + 50) / maxWidth);
        
        auto height = noteEditor.getTextHeight();
        
        auto bounds = Rectangle<int>(note->x_obj.te_xpix, note->x_obj.te_ypix, maxWidth, height + 4);
        pd->unlockAudioThread();

        return bounds;
    }
    
    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        newBounds.reduce(Object::margin, Object::margin);
        
        auto* note = static_cast<t_fake_note*>(ptr);
        note->x_width = newBounds.getWidth();
        object->updateBounds();

        return true;
    }
    
    void setPdBounds(Rectangle<int> b) override
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        note->x_width = b.getWidth();
        note->x_height = b.getHeight();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
    }

    
    String getText() override
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        return String::fromUTF8(note->x_buf, note->x_bufsize);
    }
    
    void valueChanged(Value& v) override
    {
        auto* note = static_cast<t_fake_note*>(ptr);
        
        if (v.refersToSameSourceAs(primaryColour)) {
            auto colour = Colour::fromString(primaryColour.toString());
            noteEditor.applyColourToAllText(colour);
            colourToHexArray(colour, note->x_color);
            repaint();
        } else if (v.refersToSameSourceAs(secondaryColour)) {
            colourToHexArray(Colour::fromString(secondaryColour.toString()), note->x_bgcolor);
            repaint();
        }
    }
    
    ObjectParameters getParameters() override
    {
        return {
            { "Text colour", tColour, cAppearance, &primaryColour, {} },
            { "Background colour", tColour, cAppearance, &secondaryColour, {} },
            { "Font size", tInt, cAppearance, &fontSize, {} },
            { "Bold", tBool, cAppearance, &bold, { "no", "yes" } }
        };
    }
};
