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
        if (auto comment = ptr.get<t_fake_comment>()) {
            textColour = Colour(comment->x_red, comment->x_green, comment->x_blue);
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            libpd_moveobj(patch, gobj.get(), b.getX(), b.getY());
        }
    }

    void resized() override
    {
        int width = std::max(getBestTextWidth(getText()) * 8, 25);
        int height = 23;

        if (auto comment = ptr.get<t_fake_comment>()) {
            height = comment->x_fontsize + 18;
        }

        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }
    }

    void paint(Graphics& g) override
    {
        // TODO: we don't want to lock here!
        int fontsize = 14;
        if (auto comment = ptr.get<t_fake_comment>()) {
            fontsize = comment->x_fontsize;
        }

        auto textArea = border.subtractedFrom(getLocalBounds());
        Fonts::drawFittedText(g, getText(), textArea, textColour, fontsize);

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
        Rectangle<int> bounds;

        if (auto comment = ptr.get<t_fake_comment>()) {
            bounds = Rectangle<int>(comment->x_obj.te_xpix, comment->x_obj.te_ypix, width, comment->x_fontsize + 18);
        }

        return bounds;
    }

    String getText() override
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            return String::fromUTF8(comment->x_buf, comment->x_bufsize);
        }

        return {};
    }

    int getBestTextWidth(String const& text)
    {
        if (auto comment = ptr.get<t_fake_comment>()) {
            return std::max<float>(round(Font(comment->x_fontsize).getStringWidthFloat(text) + 14.0f), 32);
        }

        return 32;
    }
};
