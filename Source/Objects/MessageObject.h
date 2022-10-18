
typedef struct _messresponder {
    t_pd mr_pd;
    t_outlet* mr_outlet;
} t_messresponder;

typedef struct _message {
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist* m_glist;
    t_clock* m_clock;
} t_message;

struct MessageObject final : public TextBase
{
    bool isDown = false;
    bool isLocked = false;
        
    MessageObject(void* obj, Object* parent)
    : TextBase(obj, parent)
    {
    }
    
    void updateBounds() override
    {
        pd->getCallbackLock()->enter();
        
        int x = 0, y = 0, w = 0, h = 0;
        
        // If it's a text object, we need to handle the resizable width, which pd saves in amount of text characters
        auto* textObj = static_cast<t_text*>(ptr);
        
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        
        w = textObj->te_width * glist_fontwidth(cnv->patch.getPointer());
        
        if (textObj->te_width == 0) {
            w = Font(15).getStringWidth(getText()) + 19;
        }
        
        pd->getCallbackLock()->exit();
        
        object->setObjectBounds({ x, y, w, h });
    }
    
    void checkBounds() override
    {
        int numLines = getNumLines(getText(), object->getWidth() - Object::doubleMargin - 5);
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int newHeight = (numLines * 19) + Object::doubleMargin + 2;
        int newWidth = getWidth() / fontWidth;
        
        static_cast<t_text*>(ptr)->te_width = newWidth;
        newWidth = std::max((newWidth * fontWidth), 35) + Object::doubleMargin;
        
        if (getParentComponent() && (object->getHeight() != newHeight || newWidth != object->getWidth())) {
            object->setSize(newWidth, newHeight);
        }
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }
    
    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
        
        auto* textObj = static_cast<t_text*>(ptr);
        textObj->te_width = b.getWidth() / glist_fontwidth(cnv->patch.getPointer());
    }
    
    void paint(Graphics& g) override
    {
        BorderSize<int> border { 1, 6, 1, 4 };
        
        g.setColour(object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        g.setColour(object->findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        auto textArea = border.subtractedFrom(getLocalBounds());
        g.drawFittedText(currentText, textArea, justification, numLines, minimumHorizontalScale);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);
        
        if (!isValid) {
            outlineColour = selected ? Colours::red.brighter(1.5) : Colours::red;
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }
    
    void paintOverChildren(Graphics& g) override
    {
        // GUIObject::paintOverChildren(g);
        
        auto b = getLocalBounds().reduced(1);
        
        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());
        
        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        g.fillPath(flagPath);
        
        if (isDown) {
            g.drawRoundedRectangle(b.reduced(1).toFloat(), 2.0f, 3.0f);
        }
    }
    
    
     void updateValue() override
    {
        String v = getSymbol();
        
        if (currentText != v && !v.startsWith("click")) {
            
            currentText = v;
            repaint();
        }
    }
     
    
    void showEditor() override
    {
        if (editor == nullptr) {
            BorderSize<int> border { 1, 6, 1, 4 };
            
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(font);

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, findColour(TextEditor::textColourId));
            editor->setColour(TextEditor::backgroundColourId, object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, findColour(TextEditor::focusedOutlineColourId));

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(false);
            editor->setReturnKeyStartsNewLine(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(justification);
            
            editor->setSize(10, 10);
            addAndMakeVisible(editor.get());

            editor->setText(currentText, false);
            editor->addListener(this);
            editor->onFocusLost = [this]() {
                hideEditor();
            };

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, currentText.length()));

            resized();
            repaint();

            if (isShowing()) {
                editor->grabKeyboardFocus();
            }
        }
    }
    
    void mouseDown(MouseEvent const& e) override
    {
        if (isLocked) {
            isDown = true;
            repaint();
            
            //startEdition();
            click();
            //stopEdition();
        }
    }
    
    void click()
    {
        cnv->pd->enqueueDirectMessages(ptr, 0);
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
        repaint();
    }
    
    /*
     void valueChanged(Value& v) override
     {
     GUIObject::valueChanged(v);
     } */
    
    String getSymbol() const
    {
        cnv->pd->setThis();
        
        char* text;
        int size;
        
        binbuf_gettext(static_cast<t_message*>(ptr)->m_text.te_binbuf, &text, &size);
        
        auto result = String::fromUTF8(text, size);
        freebytes(text, size);
        
        return result;
    }
    
    void setSymbol(String const& value)
    {
        cnv->pd->enqueueFunction(
                                 [_this = SafePointer(this), ptr = this->ptr, value]() mutable {
                                     
                                     if(!_this) return;
                                     
                                     auto* cstr = value.toRawUTF8();
                                     auto* messobj = static_cast<t_message*>(ptr);
                                     auto* canvas = _this->cnv->patch.getPointer();
                                     
                                     libpd_renameobj(canvas, &messobj->m_text.te_g, cstr, value.getNumBytesAsUTF8());
                                     
                                 });
    }

    
    bool hideInGraph() override
    {
        return true;
    }
};
