/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct TextObjectHelper {

    inline static int minWidth = 3;

    static int getWidthInChars(void* ptr)
    {
        return static_cast<t_text*>(ptr)->te_width;
    }

    static int setWidthInChars(void* ptr, int newWidth)
    {
        return static_cast<t_text*>(ptr)->te_width = newWidth;
    }

    static std::unique_ptr<ComponentBoundsConstrainer> createConstrainer(Object* object)
    {
        class TextObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            Object* object;

            explicit TextObjectBoundsConstrainer(Object* parent)
                : object(parent)
            {
            }
            /*
             * Custom version of checkBounds that takes into consideration
             * the padding around plugdata node objects when resizing
             * to allow the aspect ratio to be interpreted correctly.
             * Otherwise resizing objects with an aspect ratio will
             * resize the object size **including** margins, and not follow the
             * actual size of the visible object
             */
            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool isStretchingTop,
                bool isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                auto* patch = object->cnv->patch.getPointer().get();
                if (!patch)
                    return;

                auto fontWidth = glist_fontwidth(patch);

                // Remove margin
                auto newBounds = bounds.reduced(Object::margin);
                auto oldBounds = old.reduced(Object::margin);

                auto maxIolets = std::max({ 1, object->numInputs, object->numOutputs });
                auto minimumWidth = std::max(TextObjectHelper::minWidth, (maxIolets * 18) / fontWidth);
                
                // Set new width
                TextObjectHelper::setWidthInChars(object->getPointer(), std::max(minimumWidth, newBounds.getWidth() / fontWidth));

                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
                
                // If we're resizing the left edge, move the object left
                if (isStretchingLeft) {
                    auto x = oldBounds.getRight() - (bounds.getWidth() - Object::doubleMargin);
                    auto y = oldBounds.getY(); // don't allow y resize
                    
                    if(auto ptr = object->gui->ptr.get<t_gobj>())
                    {
                        pd::Interface::moveObject(static_cast<t_glist*>(patch), ptr.get(), x - object->cnv->canvasOrigin.x, y - object->cnv->canvasOrigin.y);
                    }

                    bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
                }
            }
        };

        return std::make_unique<TextObjectBoundsConstrainer>(object);
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

    // Used by text objects for estimating best text height for a set width
    static int getNumLines(String const& text, int width, int fontSize)
    {
        int numLines = 1;

        Array<int> glyphs;
        Array<float> xOffsets;

        auto font = Font(fontSize);
        font.getGlyphPositions(text.trimCharactersAtEnd(";\n"), glyphs, xOffsets);

        wchar_t lastChar;
        for (int i = 0; i < xOffsets.size(); i++) {
            if ((xOffsets[i] + 11) >= static_cast<float>(width) || (text.getCharPointer()[i] == '\n' && lastChar == ';')) {
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
        auto* editor = new TextEditor;
        editor->applyFontToAllText(Font(fontHeight));

        object->copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
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
    , public TextEditor::Listener
    , public KeyListener {

protected:
    std::unique_ptr<TextEditor> editor;
    BorderSize<int> border = BorderSize<int>(1, 6, 1, 1);

    CachedTextRender cachedTextRender;

    Value sizeProperty = SynchronousValue();
    String objectText;
    bool isValid = true;
    bool isLocked;

    Colour backgroundColour;
    NVGcolor selectedOutlineColour;
    NVGcolor outlineColour;
    NVGcolor ioletAreaColour;

public:
    TextBase(pd::WeakReference obj, Object* parent, bool valid = true)
        : ObjectBase(obj, parent)
        , isValid(valid)
    {
        objectText = getText();

        isLocked = getValue<bool>(cnv->locked);

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);

        lookAndFeelChanged();
    }

    ~TextBase() override = default;

    void update() override
    {
        if (auto obj = ptr.get<t_text>()) {
            sizeProperty = TextObjectHelper::getWidthInChars(obj.get());
        }
    }

    void lookAndFeelChanged() override
    {
        backgroundColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::textObjectBackgroundColourId);
        selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        ioletAreaColour = convertColour(object->findColour(PlugDataColour::ioletAreaColourId));

        updateTextLayout();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds();

        auto finalOutlineColour = outlineColour;
        auto finalBackgroundColour = convertColour(backgroundColour);

        // render invalid text objects with red outline & semi-transparent background
        if (!isValid) {
            finalOutlineColour = convertColour(object->isSelected() ? Colours::red.brighter(1.5f) : Colours::red);
            finalBackgroundColour = nvgRGBAf(outlineColour.r, outlineColour.g, outlineColour.b, 0.2f);
        }
        else if(getPatch() && isMouseOver() && getValue<bool>(cnv->locked))
        {
            finalBackgroundColour = convertColour(backgroundColour.contrasting(backgroundColour.getBrightness() > 0.5f ? 0.03f : 0.05f));
        }
        
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), finalBackgroundColour, object->isSelected() ? selectedOutlineColour : finalOutlineColour, Corners::objectCornerRadius);

        // if the object is valid & iolet area colour is differnet from background colour
        // draw two non-rounded rectangles at top / bottom
        // scissor with rounded rectangle
        //   ┌──────────────────┐
        //   │┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼│
        //   ├──────────────────┤
        //   │                  │
        //   │      object      │
        //   │                  │
        //   ├──────────────────┤
        //   │┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼│
        //   └──────────────────┘

        bool hasIoletArea = static_cast<int>(ioletAreaColour.r * 255) != backgroundColour.getRed()  ||
                            static_cast<int>(ioletAreaColour.g * 255) != backgroundColour.getGreen()||
                            static_cast<int>(ioletAreaColour.b * 255) != backgroundColour.getBlue() ||
                            static_cast<int>(ioletAreaColour.a * 255) != backgroundColour.getAlpha();
        
        if (isValid && hasIoletArea) {
            NVGScopedState scopedState(nvg);
            float const padding = 1.3f;
            float const padding2x = padding * 2;
            nvgRoundedScissor(nvg, padding, padding, getWidth() - padding2x, getHeight() - padding2x, jmax(0.0f, Corners::objectCornerRadius - 0.7f));

            nvgFillColor(nvg, ioletAreaColour);
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, getWidth(), 3.5f);
            nvgRect(nvg, 0, getHeight() - 3.5f, getWidth(), 3.5f);
            nvgFill(nvg);
        }

        if (editor && editor->isVisible()) {
            imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
        } else {
            cachedTextRender.renderText(nvg, border.subtractedFrom(b), getImageScale());
        }
    }

    // Override to cancel default behaviour
    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (editor != nullptr) {
            cnv->grabKeyboardFocus();
        }
    }

    void textEditorTextChanged(TextEditor& ed) override
    {
        object->updateBounds();
    }

    Rectangle<int> getPdBounds() override
    {
        updateTextLayout(); // make sure layout height is updated

        auto textBounds = getTextSize();

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getPointer().get();
            if (!cnvPtr)
                return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 5, 20) };

            pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
        }

        return { x, y, textBounds.getWidth(), std::max<int>(textBounds.getHeight() + 5, 20) };
    }

    virtual Rectangle<int> getTextSize()
    {
        auto objText = editor ? editor->getText() : objectText;
        
        if (editor && cnv->suggestor) {
            cnv->suggestor->updateSuggestions(objText);
            if(cnv->suggestor->getText().isNotEmpty()) {
                objText = cnv->suggestor->getText();
            }
        }

        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<void>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getPointer().get());
        }

        auto textSize = cachedTextRender.getTextBounds();

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 11;

        // We want to adjust the width so ideal text with aligns with fontWidth
        int offset = idealWidth % fontWidth;

        int textWidth;
        if (objText.isEmpty()) { // If text is empty, set to minimum width
            textWidth = std::max(charWidth, 6) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            textWidth = std::clamp(idealWidth, TextObjectHelper::minWidth * fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth + offset;
        }

        auto maxIolets = std::max(object->numInputs, object->numOutputs);
        textWidth = std::max(textWidth, maxIolets * 18);

        return { textWidth, textSize.getHeight() };
    }

    virtual void updateTextLayout()
    {
        if (cnv->isGraph)
            return; // Text layouting is expensive, so skip if it's not necessary

        auto objText = editor ? editor->getText() : objectText;
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        }

        auto colour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId);
        int textWidth = getTextSize().getWidth() - 11;
        if (cachedTextRender.prepareLayout(objText, Fonts::getDefaultFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty))) {
            repaint();
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());

            if (TextObjectHelper::getWidthInChars(gobj.get())) {
                TextObjectHelper::setWidthInChars(gobj.get(), b.getWidth() / glist_fontwidth(patch));
            }

            auto type = hash(getText().upToFirstOccurrenceOf(" ", false, false));

            if (type == hash("inlet") || type == hash("inlet~")) {
                canvas_resortinlets(patch);
            } else if (type == hash("outlet") || type == hash("outlet~")) {
                canvas_resortoutlets(patch);
            }
        }

        updateTextLayout();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (isLocked) {
            click(e.getPosition(), e.mods.isShiftDown(), e.mods.isAltDown());
        }
    }

    bool showParametersWhenSelected() override
    {
        return false;
    }

    void hideEditor() override
    {
        if (editor != nullptr) {
            std::unique_ptr<TextEditor> outgoingEditor;
            std::swap(outgoingEditor, editor);

            cnv->hideSuggestions();

            auto newText = outgoingEditor->getText();
            
            newText = TextObjectHelper::fixNewlines(newText);

            bool changed;
            if (objectText != newText) {
                objectText = newText;
                updateTextLayout();
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

            editor->setBorder(border.addedTo(BorderSize<int>(0, 0, 1, 0)));
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onFocusLost = [this]() {
                if (reinterpret_cast<Component*>(cnv->suggestor.get())->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
                    editor->grabKeyboardFocus();
                    return;
                }

                // TODO: this system is fragile
                // If anything grabs keyboard focus when clicking an object, this will close the editor!
                hideEditor();
            };

            cnv->showSuggestions(object, editor.get());

            object->updateBounds();
            repaint();
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto text = ptr.get<t_text>()) {
            setParameterExcludingListener(sizeProperty, TextObjectHelper::getWidthInChars(text.get()));
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(getValue<int>(sizeProperty), constrainer->getMinimumWidth());

            setParameterExcludingListener(sizeProperty, width);

            if (auto text = ptr.get<t_text>()) {
                TextObjectHelper::setWidthInChars(text.get(), width);
            }

            object->updateBounds();
        }
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        if (key.getKeyCode() == KeyPress::returnKey && editor && key.getModifiers().isShiftDown()) {
            int caretPosition = editor->getCaretPosition();
            auto text = editor->getText();

            if (!editor->getHighlightedRegion().isEmpty())
                return false;
            if (text[caretPosition - 1] == ';') {
                text = text.substring(0, caretPosition) + "\n" + text.substring(caretPosition);
                caretPosition += 1;
            } else {
                text = text.substring(0, caretPosition) + ";\n" + text.substring(caretPosition);
                caretPosition += 2;
            }

            editor->setText(text);
            editor->setCaretPosition(caretPosition);

            return true;
        }

        return false;
    }

    void resized() override
    {
        updateTextLayout();

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

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return TextObjectHelper::createConstrainer(object);
    }
};

// Actual text object, marked final for optimisation
class TextObject final : public TextBase {

public:
    TextObject(pd::WeakReference obj, Object* parent, bool isValid = true)
        : TextBase(obj, parent, isValid)
    {
    }
};
