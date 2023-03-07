/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

typedef struct _comment {
    t_object x_obj;
    t_edit_proxy* x_proxy;
    t_glist* x_glist;
    t_canvas* x_cv;
    t_binbuf* x_binbuf;
    char const* x_buf; // text buf
    int x_bufsize;     // text buf size
    int x_keynum;
    t_symbol* x_keysym;
    int x_init;
    int x_changed;
    int x_edit;
    int x_max_pixwidth;
    int x_bbset;
    int x_bbpending;
    int x_x1;
    int x_y1;
    int x_x2;
    int x_y2;
    int x_newx2;
    int x_dragon;
    int x_select;
    int x_fontsize;
    unsigned char x_red;
    unsigned char x_green;
    unsigned char x_blue;
    char x_color[8];
    char x_bgcolor[8];
    int x_shift;
    int x_selstart;
    int x_start_ndx;
    int x_end_ndx;
    int x_selend;
    int x_active;
    t_symbol* x_bindsym;
    t_symbol* x_fontname;
    t_symbol* x_receive;
    t_symbol* x_rcv_raw;
    int x_rcv_set;
    int x_flag;
    int x_r_flag;
    int x_old;
    int x_text_flag;
    int x_text_n;
    int x_text_size;
    int x_zoom;
    int x_fontface;
    int x_bold;
    int x_italic;
    int x_underline;
    int x_bg_flag;
    int x_textjust;       // 0: left, 1: center, 2: right
    unsigned int x_bg[3]; // background color
    t_pd* x_handle;
} t_fake_comment;

// This object is a dumb version of [cyclone/comment] that only serves to make cyclone's documentation readable
class CycloneCommentObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

public:
    CycloneCommentObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);
        textColour = Colour(comment->x_red, comment->x_green, comment->x_blue);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
    }

    void resized() override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);

        int width = getBestTextWidth(getText()) * 8;
        int height = comment->x_fontsize + 18;

        width = std::max(width, 25);

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }
    }

    void paint(Graphics& g) override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);

        auto textArea = border.subtractedFrom(getLocalBounds());
        Fonts::drawFittedText(g, getText(), textArea, textColour, comment->x_fontsize);

        auto selected = cnv->isSelected(object);
        if (object->locked == var(false) && (object->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(selected ? object->findColour(PlugDataColour::objectSelectedOutlineColourId) : object->findColour(PlugDataColour::objectOutlineColourId));

            g.drawRect(getLocalBounds().toFloat(), 0.5f);
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

    Rectangle<int> getPdBounds() override
    {
        int width = getBestTextWidth(getText()) * 8;
      
        pd->lockAudioThread();
        auto* comment = static_cast<t_fake_comment*>(ptr);
        int height = comment->x_fontsize + 18;
        auto bounds = Rectangle<int>(comment->x_obj.te_xpix, comment->x_obj.te_ypix, width, height);
        pd->unlockAudioThread();



        return bounds;
    }

    String getText() override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);
        return String::fromUTF8(comment->x_buf, comment->x_bufsize);
    }

    int getBestTextWidth(String const& text)
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);

        return std::max<float>(round(Font(comment->x_fontsize).getStringWidthFloat(text) + 14.0f), 32);
    }
};
