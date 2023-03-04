/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct TextObjectHelper {

    inline static int minWidth = 3;

    static Rectangle<int> recalculateTextObjectBounds(void* patch, void* obj, String const& currentText, int fontHeight, int& numLines, bool applyOffset = false, int maxIolets = 0)
    {
        int x, y, w, h;
        libpd_get_object_bounds(patch, obj, &x, &y, &w, &h);

        auto fontWidth = glist_fontwidth(static_cast<t_glist*>(patch));
        int idealTextWidth = getIdealWidthForText(currentText, fontHeight);

        // For regular text object, we want to adjust the width so ideal text with aligns with fontWidth
        int offset = applyOffset ? idealTextWidth % fontWidth : 0;
        int charWidth = getWidthInChars(obj);

        if (currentText.isEmpty()) { // If text is empty, set to minimum width
            w = std::max(charWidth, minWidth) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            w = std::clamp(idealTextWidth, minWidth * fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            w = std::max(charWidth, minWidth) * fontWidth + offset;
        }

        w = std::max(w, maxIolets * 18);

        numLines = getNumLines(currentText, w, fontHeight);
        // Calculate height so that height with 1 line is 21px, after that scale along with fontheight
        h = numLines * fontHeight + (21 - fontHeight);

        return { x, y, w, h };
    }

    static void checkBounds(void* patch, void* obj, Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft, int fontWidth, int maxIolets = 0)
    {
        // Remove margin
        newBounds = newBounds.reduced(Object::margin);
        oldBounds = oldBounds.reduced(Object::margin);

        auto minimumWidth = std::max(minWidth, (maxIolets * 18) / fontWidth);

        // Calculate the width in text characters for both
        auto oldCharWidth = oldBounds.getWidth() / fontWidth;
        auto newCharWidth = std::max(minimumWidth, newBounds.getWidth() / fontWidth);

        // If we're resizing the left edge, move the object left
        if (resizingOnLeft) {
            auto widthDiff = (newCharWidth - oldCharWidth) * fontWidth;
            auto x = oldBounds.getX() - widthDiff;
            auto y = oldBounds.getY(); // don't allow y resize

            libpd_moveobj(static_cast<t_glist*>(patch), static_cast<t_gobj*>(obj), x, y);
        }

        // Set new width
        TextObjectHelper::setWidthInChars(obj, newCharWidth);
    }

    static int getWidthInChars(void* ptr)
    {
        return static_cast<t_text*>(ptr)->te_width;
    }

    static int setWidthInChars(void* ptr, int newWidth)
    {
        return static_cast<t_text*>(ptr)->te_width = newWidth;
    }

    static String fixNewlines(String text)
    {
        // Don't want \r
        text = text.replace("\r", "");

        // Temporarily use \r to represent a real newline in pd
        text = text.replace(";\n", "\r");

        // Remove \n
        text = text.replace("\n", " ");

        // Replace the real newlines with \n
        text = text.replace("\r", ";\n");

        // Remove whitespace from end
        text = text.trimEnd();

        return text;
    }

    static int getIdealWidthForText(String text, int fontHeight)
    {
        auto lines = StringArray::fromLines(text);
        int w = minWidth;

        for (auto& line : lines) {
            w = std::max<int>(Font(fontHeight).getStringWidthFloat(line) + 14.0f, w);
        }

        return w;
    }

    // Used by text objects for estimating best text height for a set width
    static int getNumLines(String const& text, int width, int fontSize)
    {
        int numLines = 1;

        Array<int> glyphs;
        Array<float> xOffsets;

        auto font = Font(fontSize);
        font.getGlyphPositions(text, glyphs, xOffsets);

        wchar_t lastChar;
        for (int i = 0; i < xOffsets.size(); i++) {
            if ((xOffsets[i] + 12) >= static_cast<float>(width) || (text.getCharPointer()[i] == '\n' && lastChar == ';')) {
                for (int j = i + 1; j < xOffsets.size(); j++) {
                    xOffsets.getReference(j) -= xOffsets[i];
                }
                numLines++;
            }
            lastChar = text.getCharPointer()[i];
        }

        return numLines;
    }

    static TextEditor* createTextEditor(Object* object, int fontHeight)
    {
        TextEditor* editor = new TextEditor;
        editor->applyFontToAllText(Font(fontHeight));

        object->copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        editor->setAlwaysOnTop(true);
        editor->setMultiLine(true);
        editor->setReturnKeyStartsNewLine(false);
        editor->setScrollbarsShown(false);
        editor->setIndents(0, 0);
        editor->setScrollToShowCursor(false);
        editor->setJustification(Justification::centredLeft);

        return editor;
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
    bool isLocked;

public:
    TextBase(void* obj, Object* parent, bool valid = true)
        : ObjectBase(obj, parent)
        , isValid(valid)
    {
        objectText = getText();
        isLocked = static_cast<bool>(cnv->locked.getValue());
    }

    virtual ~TextBase()
    {
    }

    void paint(Graphics& g) override
    {
        auto backgroundColour = object->findColour(PlugDataColour::textObjectBackgroundColourId);
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        auto ioletAreaColour = object->findColour(PlugDataColour::ioletAreaColourId);

        if (ioletAreaColour != backgroundColour) {
            g.setColour(ioletAreaColour);
            g.fillRect(getLocalBounds().removeFromTop(3));
            g.fillRect(getLocalBounds().removeFromBottom(3));
        }

        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());

            auto scale = getWidth() < 40 ? 0.9f : 1.0f;
            PlugDataLook::drawFittedText(g, objectText, textArea, object->findColour(PlugDataColour::canvasTextColourId), numLines, scale);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = cnv->isSelected(object) && !cnv->isGraph;

        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (!isValid) {
            outlineColour = selected ? Colours::red.brighter(1.5) : Colours::red;
        }

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    // Override to cancel default behaviour
    void lock(bool locked) override
    {
        isLocked = locked;
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
        pd->lockAudioThread();

        auto* cnvPtr = cnv->patch.getPointer();

        String objText;
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        } else if (editor) {
            objText = editor->getText();
        } else {
            objText = objectText;
        }

        auto newNumLines = 0;

        bool resizingOnLeft = object->resizeZone.isDraggingLeftEdge();
        int oldWidth = object->originalBounds.getWidth() - Object::doubleMargin;
        int currentWidth = getWidth();
        int oldX = object->originalBounds.getX();

        auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, ptr, objText, 15, newNumLines, true, std::max({ 1, object->numInputs, object->numOutputs }));

        numLines = newNumLines;

        if (newBounds != object->getObjectBounds()) {
            object->setObjectBounds(newBounds);
        }

        pd->unlockAudioThread();
    }

    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        auto fontWidth = glist_fontwidth(cnv->patch.getPointer());
        auto* patch = cnv->patch.getPointer();
        TextObjectHelper::checkBounds(patch, ptr, oldBounds, newBounds, resizingOnLeft, fontWidth, std::max(object->numInputs, object->numOutputs));
        updateBounds();
        return true;
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        if (TextObjectHelper::getWidthInChars(ptr)) {
            TextObjectHelper::setWidthInChars(ptr, b.getWidth() / glist_fontwidth(cnv->patch.getPointer()));
        }
    }
        
    void mouseDown(const MouseEvent& e) override
    {
        if(isLocked) {
            click();
        }
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            WeakReference<Component> deletionChecker(this);
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            cnv->hideSuggestions();

            auto newText = outgoingEditor->getText();

            outgoingEditor->removeListener(cnv->suggestor);

            newText = TextObjectHelper::fixNewlines(newText);

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

    bool isEditorShown() override
    {
        return editor != nullptr;
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));

            editor->setBorder(border);
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

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

            resized();
            repaint();
        }
    }

    void resized() override
    {
        if (editor) {
            editor->setBounds(getLocalBounds());
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
