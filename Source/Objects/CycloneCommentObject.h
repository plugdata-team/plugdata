/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

typedef struct _comment{
    t_object        x_obj;
    t_edit_proxy   *x_proxy;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    t_binbuf       *x_binbuf;
    const char           *x_buf;      // text buf
    int             x_bufsize;  // text buf size
    int             x_keynum;
    t_symbol       *x_keysym;
    int             x_init;
    int             x_changed;
    int             x_edit;
    int             x_max_pixwidth;
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
    unsigned char   x_red;
    unsigned char   x_green;
    unsigned char   x_blue;
    char            x_color[8];
    char            x_bgcolor[8];
    int             x_shift;
    int             x_selstart;
    int             x_start_ndx;
    int             x_end_ndx;
    int             x_selend;
    int             x_active;
    t_symbol       *x_bindsym;
    t_symbol       *x_fontname;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
    int             x_old;
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
    unsigned int    x_bg[3];    // background color
    t_pd           *x_handle;
}t_fake_comment;


struct CycloneCommentObject final : public TextBase, public KeyListener {
    
    Colour textColour;
    
    CycloneCommentObject(void* obj, Box* box)
        : TextBase(obj, box)
    {
        currentText = getText();
        
        auto* comment = static_cast<t_fake_comment*>(ptr);
        font = font.withHeight(comment->x_fontsize);
        
        textColour = Colour(comment->x_red, comment->x_green, comment->x_blue);
    }

    void applyBounds() override
    {
        auto b = box->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* comment = static_cast<t_fake_comment*>(ptr);
        comment->x_text_n = textObjectWidth;
    }
    
    void resized() override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);

        int width = getBestTextWidth(getText());
        numLines = getNumLines(currentText, width);
        int height = numLines * comment->x_fontsize + 6;

        width = std::max(width, 25);

        if (getWidth() != width || getHeight() != height) {
            box->setSize(width + Box::doubleMargin, height + Box::doubleMargin);
        }

        if (editor) {
            editor->setBounds(getLocalBounds());
        }
    }
    
    void paint(Graphics& g) override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);
        
        g.setColour(textColour);
        g.setFont(font.withHeight(comment->x_fontsize));

        auto textArea = border.subtractedFrom(getLocalBounds());
        g.drawFittedText(getText(), textArea, justification, numLines, minimumHorizontalScale);

        auto selected = cnv->isSelected(box);
        if (box->locked == var(false) && (box->isMouseOverOrDragging(true) || selected) && !cnv->isGraph) {
            g.setColour(selected ? box->findColour(PlugDataColour::highlightColourId) : box->findColour(PlugDataColour::canvasOutlineColourId));

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

    
    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            auto newText = outgoingEditor->getText();

            bool changed;
            if (currentText != newText) {
                currentText = newText;
                repaint();
                changed = true;
            } else {
                changed = false;
            }

            outgoingEditor.reset();

            repaint();

            // update if the name has changed, or if pdobject is unassigned
            if (changed) {
                cnv->pd->enqueueFunction(
                    [this, _this = SafePointer<CycloneCommentObject>(this)]() mutable {
                        if (!_this)
                            return;

                        auto* comment = static_cast<t_fake_comment*>(ptr);
                        comment->x_buf = currentText.toRawUTF8();
                        comment->x_bufsize = currentText.getNumBytesAsUTF8();
                        
                        MessageManager::callAsync(
                            [_this]() {
                                if (!_this)
                                    return;
                                _this->box->updateBounds();
                            });
                    });
            }
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(font);

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
            editor->setColour(Label::backgroundWhenEditingColourId, findColour(TextEditor::backgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(false);
            editor->setReturnKeyStartsNewLine(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(justification);

            
            editor->onFocusLost = [this]() {
                // Necessary so the editor doesn't close when clicking on a suggestion
                if (!reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true)) {
                    hideEditor();
                }
            };

            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(currentText, false);
            editor->addListener(this);
            editor->addKeyListener(this);

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, currentText.length()));

            resized();
            repaint();

            editor->grabKeyboardFocus();
        }
    }

    bool hideInGraph() override
    {
        return false;
    }
    
    bool keyPressed(const KeyPress& key, Component* component) override
    {
        if (key == KeyPress::rightKey) {
            if(editor){
                editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
                return true;
            }
        }
        /*
        if (key == KeyPress::returnK) {
            
        } */
        return false;
    }
    
    
    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x, y, w, h;

        auto* comment = static_cast<t_fake_comment*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        Rectangle<int> bounds = { x, y, comment->x_text_n, h };

        int fontWidth = comment->x_fontsize;

        pd->getCallbackLock()->exit();
        

        int width = getBestTextWidth(getText());

        numLines = getNumLines(currentText, width, Font(comment->x_fontsize));
        int height = numLines * comment->x_fontsize + 6;

        bounds.setWidth(width);
        bounds.setHeight(height);

        box->setObjectBounds(bounds);
    }
    
    
    String getText() override
    {
        auto* comment = static_cast<t_fake_comment*>(ptr);
        return String::fromUTF8(comment->x_buf, comment->x_bufsize);
    }

};
