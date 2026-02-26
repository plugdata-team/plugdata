/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

struct TextObjectHelper {

    static int getWidthInChars(t_text* ptr)
    {
        return ptr->te_width;
    }

    static int setWidthInChars(void* ptr, int const newWidth)
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
                bool const isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                // Remove margin
                auto const newBounds = bounds.reduced(Object::margin);
                auto const oldBounds = old.reduced(Object::margin);
                auto const maxIolets = std::max<int>({ 1, object->numInputs, object->numOutputs });

                // Set new width
                if (auto ptr = object->gui->ptr.get<t_gobj>()) {
                    auto* patch = object->cnv->patch.getRawPointer();
                    auto const fontWidth = glist_fontwidth(patch);
                    auto const minimumWidth = std::max(1, maxIolets * 18 / fontWidth);
                    TextObjectHelper::setWidthInChars(object->getPointer(), std::max(minimumWidth, newBounds.getWidth() / fontWidth));
                }

                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;

                // If we're resizing the left edge, move the object left
                if (isStretchingLeft) {
                    auto const x = oldBounds.getRight() - (bounds.getWidth() - Object::doubleMargin);
                    auto const y = oldBounds.getY(); // don't allow y resize

                    if (auto ptr = object->gui->ptr.get<t_gobj>()) {
                        auto* patch = object->cnv->patch.getRawPointer();
                        pd::Interface::moveObject(patch, ptr.get(), x - object->cnv->canvasOrigin.x, y - object->cnv->canvasOrigin.y);
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

    static TextEditor* createTextEditor(Object const* object, Font const& f)
    {
        auto* editor = new TextEditor;
        editor->applyFontToAllText(f);

        object->copyAllExplicitColoursTo(*editor);
        editor->setColour(TextEditor::textColourId, PlugDataColours::canvasTextColour);
        editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        editor->setAlwaysOnTop(true);
        editor->setMultiLine(true);
        editor->setReturnKeyStartsNewLine(false);
        editor->setScrollbarsShown(false);
        editor->setIndents(0, 0);
        editor->setScrollToShowCursor(false);
        editor->setJustification(Justification::centredLeft);
        editor->getProperties().set("NoOutline", true);

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
    bool canBeClicked = false;
    bool isValid = true;
    bool isLocked;

    NVGcolor selectedOutlineColour;
    NVGcolor outlineColour;
    NVGcolor ioletAreaColour;

public:
    TextBase(pd::WeakReference obj, Object* parent, bool const valid = true)
        : ObjectBase(obj, parent)
        , isValid(valid)
    {
        objectText = getText();

        isLocked = getValue<bool>(cnv->locked);

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty, var(), true, 0);

        lookAndFeelChanged();
    }

    ~TextBase() override = default;

    void update() override
    {
        if (auto obj = ptr.get<t_text>()) {
            sizeProperty = TextObjectHelper::getWidthInChars(obj.get());
            canBeClicked = zgetfn(&obj->te_g.g_pd, gensym("click")) != nullptr;
        }
    }

    void lookAndFeelChanged() override
    {
        selectedOutlineColour = nvgColour(PlugDataColours::objectSelectedOutlineColour);
        outlineColour = nvgColour(PlugDataColours::objectOutlineColour);
        ioletAreaColour = nvgColour(PlugDataColours::ioletAreaColour);

        updateTextLayout();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds();
        auto const bg = PlugDataColours::textObjectBackgroundColour;
        
        auto finalOutlineColour = object->isSelected() ? selectedOutlineColour : outlineColour;
        auto finalBackgroundColour = nvgColour(PlugDataColours::textObjectBackgroundColour);
        auto const outlineCol = object->isSelected() ? selectedOutlineColour : finalOutlineColour;

        // render invalid text objects with red outline & semi-transparent background
        if (!isValid) {
            finalOutlineColour = nvgColour(object->isSelected() ? Colours::red.brighter(1.5f) : Colours::red);
            finalBackgroundColour = nvgRGBA(outlineColour.r, outlineColour.g, outlineColour.b, 0.2f * 255);
        } else if ((canBeClicked || getPatch()) && isMouseOver() && getValue<bool>(cnv->locked)) {
            finalBackgroundColour = nvgColour(bg.contrasting(bg.getBrightness() > 0.5f ? 0.03f : 0.05f));
        }

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), finalBackgroundColour, finalOutlineColour, Corners::objectCornerRadius);

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

        bool const hasIoletArea = ioletAreaColour.r != bg.getRed() || ioletAreaColour.g != bg.getGreen() || ioletAreaColour.b != bg.getBlue() || ioletAreaColour.a != bg.getAlpha();

        if (isValid && hasIoletArea) {
            nvgFillColor(nvg, ioletAreaColour);
            nvgBeginPath(nvg);
            nvgRoundedRectVarying(nvg, 0, 0, getWidth(), 3.5f, Corners::defaultCornerRadius, Corners::defaultCornerRadius, 0.0f, 0.0f);
            nvgRoundedRectVarying(nvg, 0, getHeight() - 3.5f, getWidth(), 3.5f, 0.0f, 0.0f, Corners::defaultCornerRadius, Corners::defaultCornerRadius);
            nvgFill(nvg);

            nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), outlineCol, Corners::objectCornerRadius);
        }

        if (editor && editor->isVisible()) {
            Graphics g(*cnv->editor->getNanoLLGC());
            editor->paintEntireComponent(g, true);
        } else {
            cachedTextRender.renderText(nvg, border.subtractedFrom(b).toFloat(), getImageScale());
        }
    }

    // Override to cancel default behaviour
    void lock(bool const locked) override
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

        auto const textBounds = getTextSize();

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getRawPointer();
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
            if (cnv->suggestor->getText().isNotEmpty()) {
                objText = cnv->suggestor->getText();
            }
        }

        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<t_text>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getRawPointer());
        }

        auto const textSize = cachedTextRender.getTextBounds();

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int const idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 13;

        int textWidth;
        if (objText.isEmpty()) { // If text is empty, set to minimum width
            textWidth = std::max(charWidth, 6) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            textWidth = std::clamp(idealWidth, fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            // We want to adjust the width so ideal text with aligns with fontWidth
            int const offset = (idealWidth - 5) % fontWidth;
            textWidth = charWidth * fontWidth + offset + 5;
        }

        auto const maxIolets = std::max(object->numInputs, object->numOutputs);
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

        auto const colour = PlugDataColours::canvasTextColour;
        int const textWidth = getTextSize().getWidth() - 12;
        if (cachedTextRender.prepareLayout(objText, Fonts::getCurrentFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty), PlugDataLook::getUseSyntaxHighlighting() && isValid)) {
            repaint();
        }
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());

            if (TextObjectHelper::getWidthInChars(gobj.cast<t_text>())) {
                TextObjectHelper::setWidthInChars(gobj.get(), (b.getWidth() - 5) / glist_fontwidth(patch));
            }

            auto const type = hash(getText().upToFirstOccurrenceOf(" ", false, false));

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
            editor.reset(TextObjectHelper::createTextEditor(object, Fonts::getCurrentFont().withHeight(15)));
            editor->setBorder(border);
            editor->setBounds(getLocalBounds());
            editor->setText(objectText, false);
            editor->addListener(this);
            editor->addKeyListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onFocusLost = [this] {
                if (cnv->suggestor.get()->hasKeyboardFocus(true) || Component::getCurrentlyFocusedComponent() == editor.get()) {
                    editor->grabKeyboardFocus();
                    return;
                }

                // Be careful, if anything grabs keyboard focus when clicking an object, this will close the editor!
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

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const width = getValue<int>(sizeProperty);

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
        
    ResizeDirection getAllowedResizeDirections() const override
    {
        return HorizontalOnly;
    }
};

// Actual text object, marked final for optimisation
class TextObject final : public TextBase {

public:
    TextObject(pd::WeakReference obj, Object* parent, bool const isValid = true)
        : TextBase(obj, parent, isValid)
    {
    }
};
