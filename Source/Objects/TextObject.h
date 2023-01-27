/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


struct TextObjectHelper
{
    static Rectangle<int> recalculateTextObjectBounds(void* patchPtr, void* objPtr, String currentText, int fontHeight, int& numLines, bool applyOffset = false, int maxIolets = 0) {
        
        int x, y, w, h;
        libpd_get_object_bounds(patchPtr, objPtr, &x, &y, &w, &h);
        
        int charWidth = getWidthInChars(objPtr);
        int fontWidth = glist_fontwidth(static_cast<t_glist*>(patchPtr));
        int idealTextWidth = getIdealWidthForText(currentText, fontHeight);
        
        // For regular text object, we want to adjust the width so ideal text with aligns with fontWidth
        int offset = applyOffset ? idealTextWidth % fontWidth : 0;
        
        if(currentText.isEmpty()) { // If text is empty, set to minimum width
            w = 35;
        }
        else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            w = idealTextWidth;
        }
        else { // If width was set manually, calculate what the width is
            w = std::max(35, charWidth * fontWidth) + offset;
        }
        
        w = std::max(w, maxIolets * 18);
                
        numLines = StringUtils::getNumLines(currentText, w);
        // Calculate height so that height with 1 line is 21px, after that scale along with fontheight
        h = numLines * fontHeight + (21 - fontHeight);
        
        return {x, y, w, h};
    }
    
    static int getWidthInChars(void* ptr) {
        return static_cast<t_text*>(ptr)->te_width;
    }
    
    static int setWidthInChars(void* ptr, int newWidth) {
        return static_cast<t_text*>(ptr)->te_width = newWidth;
    }
    
    static int getIdealWidthForText(String text, int fontHeight) {
        auto lines = StringArray::fromLines(text);
        int w = 35;

        for (auto& line : lines) {
            w = std::max<int>(Font(fontHeight).getStringWidthFloat(line) + 14.0f, w);
        }
        
        return w;
    }
};

// Text base class that text objects with special implementation details can derive from
class TextBase : public ObjectBase
    , public TextEditor::Listener {

protected:
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 7, 1, 2);

    String objectText;
    int numLines = 1;
    bool isValid = true;

public:
    TextBase(void* obj, Object* parent, bool valid = true)
        : ObjectBase(obj, parent)
        , isValid(valid)
    {
        objectText = getText();

        // To get enter/exit messages
        addMouseListener(object, false);
    }

    virtual ~TextBase()
    {
        removeMouseListener(object);
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        auto textArea = border.subtractedFrom(getLocalBounds());
        
        PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, 0.9f);
        
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (!isValid) {
            outlineColour = selected ? Colours::red.brighter(1.5) : Colours::red;
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    // Override to cancel default behaviour
    void lock(bool isLocked) override
    {
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (editor != nullptr) {
            editor->giveAwayKeyboardFocus();
        }
    }

    void textEditorTextChanged(TextEditor& ed) override
    {
        updateBounds();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        auto* cnvPtr = cnv->patch.getPointer();
        auto objText = editor ? editor->getText() : objectText;
        auto* textObj = static_cast<t_text*>(ptr);
        auto newNumLines = 0;
        
        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, textObj, objText, 15, newNumLines, true, std::max({1, object->numInputs, object->numOutputs}));
        
        if(newNumLines > 0) {
            numLines = newNumLines;
        }
        
        if(newBounds != object->getObjectBounds()) {
            object->setObjectBounds(newBounds);
        }
        
        pd->getCallbackLock()->exit();
    }

    void checkBounds() override
    {
        int fontWidth = glist_fontwidth(cnv->patch.getPointer());
        TextObjectHelper::setWidthInChars(ptr, getWidth() / fontWidth);
        updateBounds();
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        if(TextObjectHelper::getWidthInChars(ptr)) {
            TextObjectHelper::setWidthInChars(ptr,  b.getWidth() / glist_fontwidth(cnv->patch.getPointer()));
        }
    }
    

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            outgoingEditor->setInputFilter(nullptr, false);

            cnv->hideSuggestions();

            auto newText = outgoingEditor->getText();

            bool changed;
            if (objectText != newText) {
                objectText = newText;
                repaint();
                changed = true;
            } else {
                changed = false;
            }

            outgoingEditor.reset();

            repaint();

            // update if the name has changed, or if pdobject is unassigned
            if (changed) {
                object->setType(newText);
            }
        }
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor = std::make_unique<TextEditor>(getName());
            editor->applyFontToAllText(Font(15));

            copyAllExplicitColoursTo(*editor);
            editor->setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
            editor->setColour(TextEditor::backgroundColourId, object->findColour(PlugDataColour::canvasBackgroundColourId));
            editor->setColour(Label::backgroundWhenEditingColourId, object->findColour(TextEditor::backgroundColourId));
            editor->setColour(Label::outlineWhenEditingColourId, object->findColour(TextEditor::focusedOutlineColourId));

            editor->setAlwaysOnTop(true);

            editor->setMultiLine(true);
            editor->setReturnKeyStartsNewLine(false);
            editor->setBorder(border);
            editor->setIndents(0, 0);
            editor->setJustification(Justification::centredLeft);

            editor->onFocusLost = [this]() {
                if (reinterpret_cast<Component*>(cnv->suggestor)->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
                    editor->grabKeyboardFocus();
                    return;
                }

                // TODO: this system is fragile
                // If anything grabs keyboard focus when clicking an object, this will close the editor!
                hideEditor();
            };

            cnv->showSuggestions(object, editor.get());

            editor->setBounds(getLocalBounds());
            addAndMakeVisible(editor.get());

            editor->setText(objectText, false);
            editor->addListener(this);

            if (editor == nullptr) // may be deleted by a callback
                return;

            editor->setHighlightedRegion(Range<int>(0, objectText.length()));

            resized();
            repaint();

            if (isShowing()) {
                editor->grabKeyboardFocus();
            }
        }
    }

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    TextEditor* getCurrentTextEditor() const
    {
        return editor.get();
    }

    bool hideInGraph() override
    {
        return true;
    }
};

// Actual text object, marked final for optimisation
class TextObject final : public TextBase {

public:
    TextObject(void* obj, Object* parent, bool isValid = true)
        : TextBase(obj, parent, isValid)
    {
    }
};
