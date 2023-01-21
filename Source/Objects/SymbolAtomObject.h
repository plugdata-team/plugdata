/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct SymbolAtomObject final : public ObjectBase
, public KeyListener {
    
    bool isDown = false;
    bool isLocked = false;
    
    AtomHelper atomHelper;
    
    String lastMessage;
    
    SymbolAtomObject(void* obj, Object* parent)
    : ObjectBase(obj, parent)
    , atomHelper(obj, parent, this)
    {
        addAndMakeVisible(input);
        
        input.addMouseListener(this, false);
        
        input.onTextChange = [this]() {
            startEdition();
            setSymbol(input.getText().toStdString());
            stopEdition();
            
            auto width = input.getFont().getStringWidth(input.getText()) + 36;
            if (width < object->getWidth()) {
                object->setSize(width, object->getHeight());
                checkBounds();
            }
        };
        
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            editor->setBorder({ 1, 1, 0, 0 });
            editor->addKeyListener(this);
        };
        
        input.setMinimumHorizontalScale(0.9f);
        
        object->addMouseListener(this, false);
    }
    
    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }
    
    void resized() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        int width = jlimit(30, ObjectBase::maxSize, (getWidth() / fontWidth) * fontWidth);
        int height = jlimit(12, ObjectBase::maxSize, getHeight());
        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }
        
        input.setBounds(getLocalBounds());
    }
    
    
    void updateBounds() override
    {
        pd->getCallbackLock()->enter();
        
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        
        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), atom, &x, &y, &w, &h);
        
        w = std::max<int>(4, atom->a_text.te_width) * glist_fontwidth(cnv->patch.getPointer());
        
        auto bounds = Rectangle<int>(x, y, w, atomHelper.getAtomHeight());
        
        pd->getCallbackLock()->exit();
        
        object->setObjectBounds(bounds);
    }
    
    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(30, maxSize, object->getWidth());
        int h = atomHelper.getAtomHeight() + Object::doubleMargin;
        
        if (w != object->getWidth() || h != object->getHeight()) {
            object->setSize(w, h);
        }
    }
    
    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());
        
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        
        auto* atom = static_cast<t_fake_gatom*>(ptr);
        atom->a_text.te_width = b.getWidth() / fontWidth;
    }
    
    ObjectParameters getParameters() override {
        return atomHelper.getParameters();
    }
    
    void setSymbol(String const& value)
    {
        cnv->pd->enqueueDirectMessages(ptr, value.toStdString());
    }
    
    String getSymbol()
    {
        cnv->pd->setThis();
        return String::fromUTF8(atom_getsymbol(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)))->s_name);
    }
    
    
    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;
        
        // Edit messages when unlocked, edit atoms when locked
        if (isLocked) {
            input.showEditor();
        }
        
        repaint();
    }
    
    void paint(Graphics& g)
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
        getLookAndFeel().setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        getLookAndFeel().setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        
        g.setColour(object->findColour(PlugDataColour::defaultObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangle = triangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(triangle);
        
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);
        
        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }
    
    void updateLabel() override
    {
        atomHelper.updateLabel(label);
    }
    
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(atomHelper.labelHeight)) {
            updateLabel();
            if (getParentComponent()) {
                object->updateBounds(); // update object size based on new font
            }
        } else {
            atomHelper.valueChanged(v);
        }
    }
    
    bool keyPressed(KeyPress const& key, Component* originalComponent) override
    {
        if (key == KeyPress::rightKey) {
            if (auto* editor = input.getCurrentTextEditor()) {
                editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
                return true;
            }
        }
        return false;
    }
    
    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        if(symbol == "symbol") {
            input.setText(atoms[0].getSymbol(), dontSendNotification);
        }
        else if (symbol == "send" && atoms.size() >= 1) {
            setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].getSymbol());
        }
        else if (symbol == "receive" && atoms.size() >= 1) {
            setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].getSymbol());
        }
    };
    
    Label input;
};
