/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */



// This object is a dumb version of [cyclone/comment] that only serves to make cyclone's documentation readable
class CycloneCommentObject final : public ObjectBase {

    Colour textColour;
    BorderSize<int> border { 1, 7, 1, 2 };

public:
    CycloneCommentObject(void* obj, Object* object)
        : ObjectBase(obj, object)
    {
    }

    void update() override
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

        // TODO: don't do this!
        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }
    }

    void paint(Graphics& g) override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);

        auto textArea = border.subtractedFrom(getLocalBounds());
        Fonts::drawFittedText(g, getText(), textArea, textColour, comment->x_fontsize);

        auto selected = object->isSelected();
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
