/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================

    Modified by Timothy Schoen in 2022-2023 to support fallback fonts
*/
#pragma once

//==============================================================================
/**
    An editable text box with support for fallback fonts

    A UnicodeTextEditor can either be in single- or multi-line mode, and supports mixed
    fonts and colours.
*/

#include <JuceHeader.h>

class UnicodeTextEditor : public Component
    , public TextInputTarget
    , public SettableTooltipClient {
public:
    //==============================================================================
    /** Creates a new, empty text editor.

        @param componentName        the name to pass to the component for it to use as its name
        @param passwordCharacter    if this is not zero, this character will be used as a replacement
                                    for all characters that are drawn on screen - e.g. to create
                                    a password-style textbox containing circular blobs instead of text,
                                    you could set this value to 0x25cf, which is the unicode character
                                    for a black splodge (not all fonts include this, though), or 0x2022,
                                    which is a bullet (probably the best choice for linux).
    */
    explicit UnicodeTextEditor(String const& componentName = String(),
        juce_wchar passwordCharacter = 0);

    /** Destructor. */
    ~UnicodeTextEditor() override;

    //==============================================================================
    /** Puts the editor into either multi- or single-line mode.

        By default, the editor will be in single-line mode, so use this if you need a multi-line
        editor.

        See also the setReturnKeyStartsNewLine() method, which will also need to be turned
        on if you want a multi-line editor with line-breaks.

        @param shouldBeMultiLine whether the editor should be multi- or single-line.
        @param shouldWordWrap    sets whether long lines should be broken up in multi-line editors.
                                 If this is false and scrollbars are enabled a horizontal scrollbar
                                 will be shown.

        @see isMultiLine, setReturnKeyStartsNewLine, setScrollbarsShown
    */
    void setMultiLine(bool shouldBeMultiLine,
        bool shouldWordWrap = true);

    /** Returns true if the editor is in multi-line mode. */
    bool isMultiLine() const;

    //==============================================================================
    /** Changes the behaviour of the return key.

        If set to true, the return key will insert a new-line into the text; if false
        it will trigger a call to the UnicodeTextEditor::Listener::textEditorReturnKeyPressed()
        method. By default this is set to false, and when true it will only insert
        new-lines when in multi-line mode (see setMultiLine()).
    */
    void setReturnKeyStartsNewLine(bool shouldStartNewLine);

    /** Returns the value set by setReturnKeyStartsNewLine().
        See setReturnKeyStartsNewLine() for more info.
    */
    bool getReturnKeyStartsNewLine() const { return returnKeyStartsNewLine; }

    /** Indicates whether the tab key should be accepted and used to input a tab character,
        or whether it gets ignored.

        By default the tab key is ignored, so that it can be used to switch keyboard focus
        between components.
    */
    void setTabKeyUsedAsCharacter(bool shouldTabKeyBeUsed);

    /** Returns true if the tab key is being used for input.
        @see setTabKeyUsedAsCharacter
    */
    bool isTabKeyUsedAsCharacter() const { return tabKeyUsed; }

    /** This can be used to change whether escape and return keypress events are
        propagated up to the parent component.
        The default here is true, meaning that these events are not allowed to reach the
        parent, but you may want to allow them through so that they can trigger other
        actions, e.g. closing a dialog box, etc.
    */
    void setEscapeAndReturnKeysConsumed(bool shouldBeConsumed) noexcept;

    //==============================================================================
    /** Changes the editor to read-only mode.

        By default, the text editor is not read-only. If you're making it read-only, you
        might also want to call setCaretVisible (false) to get rid of the caret.

        The text can still be highlighted and copied when in read-only mode.

        @see isReadOnly, setCaretVisible
    */
    void setReadOnly(bool shouldBeReadOnly);

    /** Returns true if the editor is in read-only mode. */
    bool isReadOnly() const noexcept;

    //==============================================================================
    /** Makes the caret visible or invisible.
        By default the caret is visible.
        @see setCaretColour, setCaretPosition
    */
    void setCaretVisible(bool shouldBeVisible);

    /** Returns true if the caret is enabled.
        @see setCaretVisible
    */
    bool isCaretVisible() const noexcept { return caretVisible && !isReadOnly(); }

    //==============================================================================
    /** Enables or disables scrollbars (this only applies when in multi-line mode).

        When the text gets too long to fit in the component, a scrollbar can appear to
        allow it to be scrolled. Even when this is enabled, the scrollbar will be hidden
        unless it's needed.

        By default scrollbars are enabled.
    */
    void setScrollbarsShown(bool shouldBeEnabled);

    /** Returns true if scrollbars are enabled.
        @see setScrollbarsShown
    */
    bool areScrollbarsShown() const noexcept { return scrollbarVisible; }

    /** Changes the password character used to disguise the text.

        @param passwordCharacter    if this is not zero, this character will be used as a replacement
                                    for all characters that are drawn on screen - e.g. to create
                                    a password-style textbox containing circular blobs instead of text,
                                    you could set this value to 0x25cf, which is the unicode character
                                    for a black splodge (not all fonts include this, though), or 0x2022,
                                    which is a bullet (probably the best choice for linux).
    */
    void setPasswordCharacter(juce_wchar passwordCharacter);

    /** Returns the current password character.
        @see setPasswordCharacter
    */
    juce_wchar getPasswordCharacter() const noexcept { return passwordCharacter; }

    //==============================================================================
    /** Allows a right-click menu to appear for the editor.

        (This defaults to being enabled).

        If enabled, right-clicking (or command-clicking on the Mac) will pop up a menu
        of options such as cut/copy/paste, undo/redo, etc.
    */
    void setPopupMenuEnabled(bool menuEnabled);

    /** Returns true if the right-click menu is enabled.
        @see setPopupMenuEnabled
    */
    bool isPopupMenuEnabled() const noexcept { return popupMenuEnabled; }

    /** Returns true if a popup-menu is currently being displayed. */
    bool isPopupMenuCurrentlyActive() const noexcept { return menuActive; }

    //==============================================================================
    /** A set of colour IDs to use to change the colour of various aspects of the editor.

        These constants can be used either via the Component::setColour(), or LookAndFeel::setColour()
        methods.

        NB: You can also set the caret colour using CaretComponent::caretColourId

        @see Component::setColour, Component::findColour, LookAndFeel::setColour, LookAndFeel::findColour
    */
    enum ColourIds {
        backgroundColourId = 0x1000200, /**< The colour to use for the text component's background - this can be
                                             transparent if necessary. */

        textColourId = 0x1000201, /**< The colour that will be used when text is added to the editor. Note
                                       that because the editor can contain multiple colours, calling this
                                       method won't change the colour of existing text - to do that, use
                                       the applyColourToAllText() method */

        highlightColourId = 0x1000202, /**< The colour with which to fill the background of highlighted sections of
                                            the text - this can be transparent if you don't want to show any
                                            highlighting.*/

        highlightedTextColourId = 0x1000203, /**< The colour with which to draw the text in highlighted sections. */

        outlineColourId = 0x1000205, /**< If this is non-transparent, it will be used to draw a box around
                                          the edge of the component. */

        focusedOutlineColourId = 0x1000206, /**< If this is non-transparent, it will be used to draw a box around
                                                 the edge of the component when it has focus. */

        shadowColourId = 0x1000207, /**< If this is non-transparent, it'll be used to draw an inner shadow
                                         around the edge of the editor. */
    };

    //==============================================================================
    /** Sets the font to use for newly added text.

        This will change the font that will be used next time any text is added or entered
        into the editor. It won't change the font of any existing text - to do that, use
        applyFontToAllText() instead.

        @see applyFontToAllText
    */
    void setFont(Font const& newFont);

    /** Applies a font to all the text in the editor.

        If the changeCurrentFont argument is true then this will also set the
        new font as the font to be used for any new text that's added.

        @see setFont
    */
    void applyFontToAllText(Font const& newFont, bool changeCurrentFont = true);

    /** Returns the font that's currently being used for new text.

        @see setFont
    */
    Font const& getFont() const noexcept { return currentFont; }

    /** Applies a colour to all the text in the editor.

        If the changeCurrentTextColour argument is true then this will also set the
        new colour as the colour to be used for any new text that's added.
    */
    void applyColourToAllText(Colour const& newColour, bool changeCurrentTextColour = true);

    /** Sets whether whitespace should be underlined when the editor font is underlined.

        @see isWhitespaceUnderlined
    */
    void setWhitespaceUnderlined(bool shouldUnderlineWhitespace) noexcept { underlineWhitespace = shouldUnderlineWhitespace; }

    /** Returns true if whitespace is underlined for underlined fonts.

        @see setWhitespaceIsUnderlined
    */
    bool isWhitespaceUnderlined() const noexcept { return underlineWhitespace; }

    //==============================================================================
    /** If set to true, focusing on the editor will highlight all its text.

        (Set to false by default).

        This is useful for boxes where you expect the user to re-enter all the
        text when they focus on the component, rather than editing what's already there.
    */
    void setSelectAllWhenFocused(bool shouldSelectAll);

    /** When the text editor is empty, it can be set to display a message.

        This is handy for things like telling the user what to type in the box - the
        string is only displayed, it's not taken to actually be the contents of
        the editor.
    */
    void setTextToShowWhenEmpty(String const& text, Colour colourToUse);

    /** Returns the text that will be shown when the text editor is empty.

        @see setTextToShowWhenEmpty
    */
    String getTextToShowWhenEmpty() const noexcept { return textToShowWhenEmpty; }

    //==============================================================================
    /** Changes the size of the scrollbars that are used.
        Handy if you need smaller scrollbars for a small text box.
    */
    void setScrollBarThickness(int newThicknessPixels);

    //==============================================================================
    /**
        Receives callbacks from a UnicodeTextEditor component when it changes.

        @see UnicodeTextEditor::addListener
    */
    class Listener {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when the user changes the text in some way. */
        virtual void textEditorTextChanged(UnicodeTextEditor&) { }

        /** Called when the user presses the return key. */
        virtual void textEditorReturnKeyPressed(UnicodeTextEditor&) { }

        /** Called when the user presses the escape key. */
        virtual void textEditorEscapeKeyPressed(UnicodeTextEditor&) { }

        /** Called when the text editor loses focus. */
        virtual void textEditorFocusLost(UnicodeTextEditor&) { }
    };

    /** Registers a listener to be told when things happen to the text.
        @see removeListener
    */
    void addListener(Listener* newListener);

    /** Deregisters a listener.
        @see addListener
    */
    void removeListener(Listener* listenerToRemove);

    //==============================================================================
    /** You can assign a lambda to this callback object to have it called when the text is changed. */
    std::function<void()> onTextChange;

    /** You can assign a lambda to this callback object to have it called when the return key is pressed. */
    std::function<void()> onReturnKey;

    /** You can assign a lambda to this callback object to have it called when the escape key is pressed. */
    std::function<void()> onEscapeKey;

    /** You can assign a lambda to this callback object to have it called when the editor loses key focus. */
    std::function<void()> onFocusLost;

    //==============================================================================
    /** Returns the entire contents of the editor. */
    String getText() const;

    /** Returns a section of the contents of the editor. */
    String getTextInRange(Range<int> const& textRange) const override;

    /** Returns true if there are no characters in the editor.
        This is far more efficient than calling getText().isEmpty().
    */
    bool isEmpty() const;

    /** Sets the entire content of the editor.

        This will clear the editor and insert the given text (using the current text colour
        and font). You can set the current text colour using
        @code setColour (UnicodeTextEditor::textColourId, ...);
        @endcode

        @param newText                  the text to add
        @param sendTextChangeMessage    if true, this will cause a change message to
                                        be sent to all the listeners.
        @see insertTextAtCaret
    */
    void setText(String const& newText,
        bool sendTextChangeMessage = true);

    /** Returns a Value object that can be used to get or set the text.

        Bear in mind that this operate quite slowly if your text box contains large
        amounts of text, as it needs to dynamically build the string that's involved.
        It's best used for small text boxes.
    */
    Value& getTextValue();

    /** Inserts some text at the current caret position.

        If a section of the text is highlighted, it will be replaced by
        this string, otherwise it will be inserted.

        To delete a section of text, you can use setHighlightedRegion() to
        highlight it, and call insertTextAtCaret (String()).

        @see setCaretPosition, getCaretPosition, setHighlightedRegion
    */
    void insertTextAtCaret(String const& textToInsert) override;

    /** Deletes all the text from the editor. */
    void clear();

    /** Deletes the currently selected region.
        This doesn't copy the deleted section to the clipboard - if you need to do that, call copy() first.
        @see copy, paste, SystemClipboard
    */
    void cut();

    /** Copies the currently selected region to the clipboard.
        @see cut, paste, SystemClipboard
    */
    void copy();

    /** Pastes the contents of the clipboard into the editor at the caret position.
        @see cut, copy, SystemClipboard
    */
    void paste();

    //==============================================================================
    /** Returns the current index of the caret.
        @see setCaretPosition
    */
    int getCaretPosition() const override;

    /** Moves the caret to be in front of a given character.
        @see getCaretPosition, moveCaretToEnd
    */
    void setCaretPosition(int newIndex);

    /** Attempts to scroll the text editor so that the caret ends up at
        a specified position.

        This won't affect the caret's position within the text, it tries to scroll
        the entire editor vertically and horizontally so that the caret is sitting
        at the given position (relative to the top-left of this component).

        Depending on the amount of text available, it might not be possible to
        scroll far enough for the caret to reach this exact position, but it
        will go as far as it can in that direction.
    */
    void scrollEditorToPositionCaret(int desiredCaretX, int desiredCaretY);

    /** Get the graphical position of the caret for a particular index in the text.

        The rectangle returned is relative to the component's top-left corner.
    */
    Rectangle<int> getCaretRectangleForCharIndex(int index) const override;

    /** Selects a section of the text. */
    void setHighlightedRegion(Range<int> const& newSelection) override;

    /** Returns the range of characters that are selected.
        If nothing is selected, this will return an empty range.
        @see setHighlightedRegion
    */
    Range<int> getHighlightedRegion() const override { return selection; }

    /** Returns the section of text that is currently selected. */
    String getHighlightedText() const;

    /** Finds the index of the character at a given position.
        The coordinates are relative to the component's top-left.
    */
    int getTextIndexAt(int x, int y) const;

    /** Finds the index of the character at a given position.
        The coordinates are relative to the component's top-left.
    */
    int getTextIndexAt(Point<int>) const;

    /** Like getTextIndexAt, but doesn't snap to the beginning/end of the range for
        points vertically outside the text.
    */
    int getCharIndexForPoint(Point<int> point) const override;

    /** Counts the number of characters in the text.

        This is quicker than getting the text as a string if you just need to know
        the length.
    */
    int getTotalNumChars() const override;

    /** Returns the total width of the text, as it is currently laid-out.

        This may be larger than the size of the UnicodeTextEditor, and can change when
        the UnicodeTextEditor is resized or the text changes.
    */
    int getTextWidth() const;

    /** Returns the maximum height of the text, as it is currently laid-out.

        This may be larger than the size of the UnicodeTextEditor, and can change when
        the UnicodeTextEditor is resized or the text changes.
    */
    int getTextHeight() const;

    /** Changes the size of the gap at the top and left-edge of the editor.
        By default there's a gap of 4 pixels.
    */
    void setIndents(int newLeftIndent, int newTopIndent);

    /** Returns the gap at the top edge of the editor.
        @see setIndents
    */
    int getTopIndent() const noexcept { return topIndent; }

    /** Returns the gap at the left edge of the editor.
        @see setIndents
    */
    int getLeftIndent() const noexcept { return leftIndent; }

    /** Changes the size of border left around the edge of the component.
        @see getBorder
    */
    void setBorder(BorderSize<int> border);

    /** Returns the size of border around the edge of the component.
        @see setBorder
    */
    BorderSize<int> getBorder() const;

    /** Used to disable the auto-scrolling which keeps the caret visible.

        If true (the default), the editor will scroll when the caret moves offscreen. If
        set to false, it won't.
    */
    void setScrollToShowCursor(bool shouldScrollToShowCaret);

    /** Modifies the justification of the text within the editor window. */
    void setJustification(Justification newJustification);

    /** Returns the type of justification, as set in setJustification(). */
    Justification getJustificationType() const noexcept { return justification; }

    /** Sets the line spacing of the UnicodeTextEditor.
        The default (and minimum) value is 1.0 and values > 1.0 will increase the line spacing as a
        multiple of the line height e.g. for double-spacing call this method with an argument of 2.0.
    */
    void setLineSpacing(float newLineSpacing) noexcept { lineSpacing = jmax(1.0f, newLineSpacing); }

    /** Returns the current line spacing of the UnicodeTextEditor. */
    float getLineSpacing() const noexcept { return lineSpacing; }

    /** Returns the bounding box for a range of text in the editor. As the range may span
        multiple lines, this method returns a RectangleList.

        The bounds are relative to the component's top-left and may extend beyond the bounds
        of the component if the text is long and word wrapping is disabled.
    */
    RectangleList<int> getTextBounds(Range<int> textRange) const override;

    //==============================================================================
    void moveCaretToEnd();
    bool moveCaretLeft(bool moveInWholeWordSteps, bool selecting);
    bool moveCaretRight(bool moveInWholeWordSteps, bool selecting);
    bool moveCaretUp(bool selecting);
    bool moveCaretDown(bool selecting);
    bool pageUp(bool selecting);
    bool pageDown(bool selecting);
    bool scrollDown();
    bool scrollUp();
    bool moveCaretToTop(bool selecting);
    bool moveCaretToStartOfLine(bool selecting);
    bool moveCaretToEnd(bool selecting);
    bool moveCaretToEndOfLine(bool selecting);
    bool deleteBackwards(bool moveInWholeWordSteps);
    bool deleteForwards(bool moveInWholeWordSteps);
    bool copyToClipboard();
    bool cutToClipboard();
    bool pasteFromClipboard();
    bool selectAll();
    bool undo();
    bool redo();

    //==============================================================================
    /** This adds the items to the popup menu.

        By default it adds the cut/copy/paste items, but you can override this if
        you need to replace these with your own items.

        If you want to add your own items to the existing ones, you can override this,
        call the base class's addPopupMenuItems() method, then append your own items.

        When the menu has been shown, performPopupMenuAction() will be called to
        perform the item that the user has chosen.

        The default menu items will be added using item IDs from the
        StandardApplicationCommandIDs namespace.

        If this was triggered by a mouse-click, the mouseClickEvent parameter will be
        a pointer to the info about it, or may be null if the menu is being triggered
        by some other means.

        @see performPopupMenuAction, setPopupMenuEnabled, isPopupMenuEnabled
    */
    virtual void addPopupMenuItems(PopupMenu& menuToAddTo,
        MouseEvent const* mouseClickEvent);

    /** This is called to perform one of the items that was shown on the popup menu.

        If you've overridden addPopupMenuItems(), you should also override this
        to perform the actions that you've added.

        If you've overridden addPopupMenuItems() but have still left the default items
        on the menu, remember to call the superclass's performPopupMenuAction()
        so that it can perform the default actions if that's what the user clicked on.

        @see addPopupMenuItems, setPopupMenuEnabled, isPopupMenuEnabled
    */
    virtual void performPopupMenuAction(int menuItemID);

    //==============================================================================
    /** Base class for input filters that can be applied to a UnicodeTextEditor to restrict
        the text that can be entered.
    */
    class InputFilter {
    public:
        InputFilter() = default;
        virtual ~InputFilter() = default;

        /** This method is called whenever text is entered into the editor.
            An implementation of this class should should check the input string,
            and return an edited version of it that should be used.
        */
        virtual String filterNewText(UnicodeTextEditor&, String const& newInput) = 0;
    };

    /** An input filter for a UnicodeTextEditor that limits the length of text and/or the
        characters that it may contain.
    */
    class LengthAndCharacterRestriction : public InputFilter {
    public:
        /** Creates a filter that limits the length of text, and/or the characters that it can contain.
            @param maxNumChars          if this is > 0, it sets a maximum length limit; if <= 0, no
                                        limit is set
            @param allowedCharacters    if this is non-empty, then only characters that occur in
                                        this string are allowed to be entered into the editor.
        */
        LengthAndCharacterRestriction(int maxNumChars, String const& allowedCharacters);

        String filterNewText(UnicodeTextEditor&, String const&) override;

    private:
        String allowedCharacters;
        int maxLength;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LengthAndCharacterRestriction)
    };

    /** Sets an input filter that should be applied to this editor.
        The filter can be nullptr, to remove any existing filters.
        If takeOwnership is true, then the filter will be owned and deleted by the editor
        when no longer needed.
    */
    void setInputFilter(InputFilter* newFilter, bool takeOwnership);

    /** Returns the current InputFilter, as set by setInputFilter(). */
    InputFilter* getInputFilter() const noexcept { return inputFilter; }

    /** Sets limits on the characters that can be entered.
        This is just a shortcut that passes an instance of the LengthAndCharacterRestriction
        class to setInputFilter().

        @param maxTextLength        if this is > 0, it sets a maximum length limit; if 0, no
                                    limit is set
        @param allowedCharacters    if this is non-empty, then only characters that occur in
                                    this string are allowed to be entered into the editor.
    */
    void setInputRestrictions(int maxTextLength,
        String const& allowedCharacters = String());

    /** Sets the type of virtual keyboard that should be displayed when this editor has
        focus.
    */
    void setKeyboardType(VirtualKeyboardType type) noexcept { keyboardType = type; }

    /** Sets the behaviour of mouse/touch interactions outside this component.

        If true, then presses outside of the UnicodeTextEditor will dismiss the virtual keyboard.
        If false, then the virtual keyboard will remain onscreen for as long as the UnicodeTextEditor has
        keyboard focus.
    */
    void setClicksOutsideDismissVirtualKeyboard(bool);

    /** Returns true if the editor is configured to hide the virtual keyboard when the mouse is
        pressed on another component.
    */
    bool getClicksOutsideDismissVirtualKeyboard() const { return clicksOutsideDismissVirtualKeyboard; }

    // LookAndFeel-ish functions: you can override these for custom drawing behaviour
    virtual void fillTextEditorBackground(Graphics&, int width, int height);
    virtual void drawTextEditorOutline(Graphics&, int width, int height);
    virtual CaretComponent* createCaretComponent(Component* keyFocusOwner);

    //==============================================================================
    /** @internal */
    void paint(Graphics&) override;
    /** @internal */
    void paintOverChildren(Graphics&) override;
    /** @internal */
    void mouseDown(MouseEvent const&) override;
    /** @internal */
    void mouseUp(MouseEvent const&) override;
    /** @internal */
    void mouseDrag(MouseEvent const&) override;
    /** @internal */
    void mouseDoubleClick(MouseEvent const&) override;
    /** @internal */
    void mouseWheelMove(MouseEvent const&, MouseWheelDetails const&) override;
    /** @internal */
    bool keyPressed(KeyPress const&) override;
    /** @internal */
    bool keyStateChanged(bool) override;
    /** @internal */
    void focusGained(FocusChangeType) override;
    /** @internal */
    void focusLost(FocusChangeType) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    void lookAndFeelChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    bool isTextInputActive() const override;
    /** @internal */
    void setTemporaryUnderlining(Array<Range<int>> const&) override;
    /** @internal */
    VirtualKeyboardType getKeyboardType() override { return keyboardType; }

protected:
    //==============================================================================
    /** Scrolls the minimum distance needed to get the caret into view. */
    void scrollToMakeSureCursorIsVisible();

    /** Used internally to dispatch a text-change message. */
    void textChanged();

    /** Begins a new transaction in the UndoManager. */
    void newTransaction();

    /** Can be overridden to intercept return key presses directly */
    virtual void returnPressed();

    /** Can be overridden to intercept escape key presses directly */
    virtual void escapePressed();

private:
    //==============================================================================
    class UniformTextSection;
    struct Iterator;
    struct TextHolderComponent;
    struct TextEditorViewport;
    struct InsertAction;
    struct RemoveAction;
    class EditorAccessibilityHandler;

    std::unique_ptr<Viewport> viewport;
    TextHolderComponent* textHolder;
    BorderSize<int> borderSize { 1, 1, 1, 3 };
    Justification justification { Justification::topLeft };

    bool readOnly = false;
    bool caretVisible = true;
    bool multiline = false;
    bool wordWrap = false;
    bool returnKeyStartsNewLine = false;
    bool popupMenuEnabled = true;
    bool selectAllTextWhenFocused = false;
    bool scrollbarVisible = true;
    bool wasFocused = false;
    bool keepCaretOnScreen = true;
    bool tabKeyUsed = false;
    bool menuActive = false;
    bool valueTextNeedsUpdating = false;
    bool consumeEscAndReturnKeys = true;
    bool underlineWhitespace = true;
    bool mouseDownInEditor = false;
    bool clicksOutsideDismissVirtualKeyboard = false;

    UndoManager undoManager;
    std::unique_ptr<CaretComponent> caret;
    Range<int> selection;
    int leftIndent = 4, topIndent = 4;
    unsigned int lastTransactionTime = 0;
    Font currentFont { 14.0f };
    mutable int totalNumChars = 0;
    int caretPosition = 0;
    OwnedArray<UniformTextSection> sections;
    String textToShowWhenEmpty;
    Colour colourForTextWhenEmpty;
    juce_wchar passwordCharacter;
    OptionalScopedPointer<InputFilter> inputFilter;
    Value textValue;
    VirtualKeyboardType keyboardType = TextInputTarget::textKeyboard;
    float lineSpacing = 1.0f;

    enum DragType {
        notDragging,
        draggingSelectionStart,
        draggingSelectionEnd
    };

    DragType dragType = notDragging;

    ListenerList<Listener> listeners;
    Array<Range<int>> underlinedSections;

    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;
    void moveCaret(int newCaretPos);
    void moveCaretTo(int newPosition, bool isSelecting);
    void recreateCaret();
    void handleCommandMessage(int) override;
    void coalesceSimilarSections();
    void splitSection(int sectionIndex, int charToSplitAt);
    void clearInternal(UndoManager*);
    void insert(String const&, int insertIndex, Font const&, Colour, UndoManager*, int newCaretPos);
    void reinsert(int insertIndex, OwnedArray<UniformTextSection> const&);
    void remove(Range<int>, UndoManager*, int caretPositionToMoveTo);
    void getCharPosition(int index, Point<float>&, float& lineHeight) const;
    void updateCaretPosition();
    void updateValueFromText();
    void textWasChangedByValue();
    int indexAtPosition(float x, float y) const;
    int findWordBreakAfter(int position) const;
    int findWordBreakBefore(int position) const;
    bool moveCaretWithTransaction(int newPos, bool selecting);
    void drawContent(Graphics&);
    void checkLayout();
    int getWordWrapWidth() const;
    int getMaximumTextWidth() const;
    int getMaximumTextHeight() const;
    void timerCallbackInt();
    void checkFocus();
    void repaintText(Range<int>);
    void scrollByLines(int deltaLines);
    bool undoOrRedo(bool shouldUndo);
    UndoManager* getUndoManager() noexcept;
    void setSelection(Range<int>) noexcept;
    Point<int> getTextOffset() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnicodeTextEditor)
};

class UnicodeLabel : public Component
    , public SettableTooltipClient
    , protected UnicodeTextEditor::Listener
    , private ComponentListener
    , private Value::Listener {
public:
    //==============================================================================
    /** Creates a Label.

        @param componentName    the name to give the component
        @param labelText        the text to show in the label
    */
    UnicodeLabel(String const& componentName = String(),
        String const& labelText = String());

    /** Destructor. */
    ~UnicodeLabel() override;

    //==============================================================================
    /** Changes the label text.

        The NotificationType parameter indicates whether to send a change message to
        any Label::Listener objects if the new text is different.
    */
    void setText(String const& newText,
        NotificationType notification);

    /** Returns the label's current text.

        @param returnActiveEditorContents   if this is true and the label is currently
                                            being edited, then this method will return the
                                            text as it's being shown in the editor. If false,
                                            then the value returned here won't be updated until
                                            the user has finished typing and pressed the return
                                            key.
    */
    String getText(bool returnActiveEditorContents = false) const;

    /** Returns the text content as a Value object.
        You can call Value::referTo() on this object to make the label read and control
        a Value object that you supply.
    */
    Value& getTextValue() noexcept { return textValue; }

    //==============================================================================
    /** Changes the font to use to draw the text.
        @see getFont
    */
    void setFont(Font const& newFont);

    /** Returns the font currently being used.
        This may be the one set by setFont(), unless it has been overridden by the current LookAndFeel
        @see setFont
    */
    Font getFont() const noexcept;

    //==============================================================================
    /** Sets the style of justification to be used for positioning the text.
        (The default is Justification::centredLeft)
    */
    void setJustificationType(Justification justification);

    /** Returns the type of justification, as set in setJustificationType(). */
    Justification getJustificationType() const noexcept { return justification; }

    /** Changes the border that is left between the edge of the component and the text.
        By default there's a small gap left at the sides of the component to allow for
        the drawing of the border, but you can change this if necessary.
    */
    void setBorderSize(BorderSize<int> newBorderSize);

    /** Returns the size of the border to be left around the text. */
    BorderSize<int> getBorderSize() const noexcept { return border; }

    /** Makes this label "stick to" another component.

        This will cause the label to follow another component around, staying
        either to its left or above it.

        @param owner    the component to follow
        @param onLeft   if true, the label will stay on the left of its component; if
                        false, it will stay above it.
    */
    void attachToComponent(Component* owner, bool onLeft);

    /** If this label has been attached to another component using attachToComponent, this
        returns the other component.

        Returns nullptr if the label is not attached.
    */
    Component* getAttachedComponent() const;

    /** If the label is attached to the left of another component, this returns true.

        Returns false if the label is above the other component. This is only relevant if
        attachToComponent() has been called.
    */
    bool isAttachedOnLeft() const noexcept { return leftOfOwnerComp; }

    /** Specifies the minimum amount that the font can be squashed horizontally before it starts
        using ellipsis. Use a value of 0 for a default value.

        @see Graphics::drawFittedText
    */
    void setMinimumHorizontalScale(float newScale);

    /** Specifies the amount that the font can be squashed horizontally. */
    float getMinimumHorizontalScale() const noexcept { return minimumHorizontalScale; }

    /** Set a keyboard type for use when the text editor is shown. */
    void setKeyboardType(TextInputTarget::VirtualKeyboardType type) noexcept { keyboardType = type; }

    //==============================================================================
    /**
        A class for receiving events from a Label.

        You can register a Label::Listener with a Label using the Label::addListener()
        method, and it will be called when the text of the label changes, either because
        of a call to Label::setText() or by the user editing the text (if the label is
        editable).

        @see Label::addListener, Label::removeListener
    */
    class Listener {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when a Label's text has changed. */
        virtual void labelTextChanged(UnicodeLabel* labelThatHasChanged) = 0;

        /** Called when a Label goes into editing mode and displays a TextEditor. */
        virtual void editorShown(UnicodeLabel*, UnicodeTextEditor&) { }

        /** Called when a Label is about to delete its TextEditor and exit editing mode. */
        virtual void editorHidden(UnicodeLabel*, UnicodeTextEditor&) { }
    };

    /** Registers a listener that will be called when the label's text changes. */
    void addListener(Listener* listener);

    /** Deregisters a previously-registered listener. */
    void removeListener(Listener* listener);

    //==============================================================================
    /** You can assign a lambda to this callback object to have it called when the label text is changed. */
    std::function<void()> onTextChange;

    /** You can assign a lambda to this callback object to have it called when the label's editor is shown. */
    std::function<void()> onEditorShow;

    /** You can assign a lambda to this callback object to have it called when the label's editor is hidden. */
    std::function<void()> onEditorHide;

    //==============================================================================
    /** Makes the label turn into a TextEditor when clicked.

        By default this is turned off.

        If turned on, then single- or double-clicking will turn the label into
        an editor. If the user then changes the text, then the ChangeBroadcaster
        base class will be used to send change messages to any listeners that
        have registered.

        If the user changes the text, the textWasEdited() method will be called
        afterwards, and subclasses can override this if they need to do anything
        special.

        @param editOnSingleClick            if true, just clicking once on the label will start editing the text
        @param editOnDoubleClick            if true, a double-click is needed to start editing
        @param lossOfFocusDiscardsChanges   if true, clicking somewhere else while the text is being
                                            edited will discard any changes; if false, then this will
                                            commit the changes.
        @see showEditor, setEditorColours, TextEditor
    */
    void setEditable(bool editOnSingleClick,
        bool editOnDoubleClick = false,
        bool lossOfFocusDiscardsChanges = false);

    /** Returns true if this option was set using setEditable(). */
    bool isEditableOnSingleClick() const noexcept { return editSingleClick; }

    /** Returns true if this option was set using setEditable(). */
    bool isEditableOnDoubleClick() const noexcept { return editDoubleClick; }

    /** Returns true if this option has been set in a call to setEditable(). */
    bool doesLossOfFocusDiscardChanges() const noexcept { return lossOfFocusDiscardsChanges; }

    /** Returns true if the user can edit this label's text. */
    bool isEditable() const noexcept { return editSingleClick || editDoubleClick; }

    /** Makes the editor appear as if the label had been clicked by the user.
        @see textWasEdited, setEditable
    */
    void showEditor();

    /** Hides the editor if it was being shown.

        @param discardCurrentEditorContents     if true, the label's text will be
                                                reset to whatever it was before the editor
                                                was shown; if false, the current contents of the
                                                editor will be used to set the label's text
                                                before it is hidden.
    */
    void hideEditor(bool discardCurrentEditorContents);

    /** Returns true if the editor is currently focused and active. */
    bool isBeingEdited() const noexcept;

    /** Returns the currently-visible text editor, or nullptr if none is open. */
    UnicodeTextEditor* getCurrentTextEditor() const noexcept;

    //==============================================================================
    /** This abstract base class is implemented by LookAndFeel classes to provide
        label drawing functionality.
    */
    struct JUCE_API LookAndFeelMethods {
        virtual ~LookAndFeelMethods() = default;

        virtual void drawLabel(Graphics&, UnicodeLabel&) = 0;
        virtual Font getLabelFont(UnicodeLabel&) = 0;
        virtual BorderSize<int> getLabelBorderSize(UnicodeLabel&) = 0;
    };

protected:
    //==============================================================================
    /** Creates the TextEditor component that will be used when the user has clicked on the label.
        Subclasses can override this if they need to customise this component in some way.
    */
    virtual UnicodeTextEditor* createEditorComponent();

    /** Called after the user changes the text. */
    virtual void textWasEdited();

    /** Called when the text has been altered. */
    virtual void textWasChanged();

    /** Called when the text editor has just appeared, due to a user click or other focus change. */
    virtual void editorShown(UnicodeTextEditor*);

    /** Called when the text editor is going to be deleted, after editing has finished. */
    virtual void editorAboutToBeHidden(UnicodeTextEditor*);

    //==============================================================================
    /** @internal */
    void paint(Graphics&) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseUp(MouseEvent const&) override;
    /** @internal */
    void mouseDoubleClick(MouseEvent const&) override;
    /** @internal */
    void componentMovedOrResized(Component&, bool wasMoved, bool wasResized) override;
    /** @internal */
    void componentParentHierarchyChanged(Component&) override;
    /** @internal */
    void componentVisibilityChanged(Component&) override;
    /** @internal */
    void inputAttemptWhenModal() override;
    /** @internal */
    void focusGained(FocusChangeType) override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override;
    /** @internal */
    void textEditorTextChanged(UnicodeTextEditor&) override;
    /** @internal */
    void textEditorReturnKeyPressed(UnicodeTextEditor&) override;
    /** @internal */
    void textEditorEscapeKeyPressed(UnicodeTextEditor&) override;
    /** @internal */
    void textEditorFocusLost(UnicodeTextEditor&) override;
    /** @internal */
    void colourChanged() override;
    /** @internal */
    void valueChanged(Value&) override;
    /** @internal */
    void callChangeListeners();

private:
    //==============================================================================
    Value textValue;
    String lastTextValue;
    Font font { 15.0f };
    Justification justification = Justification::centredLeft;
    std::unique_ptr<UnicodeTextEditor> editor;
    ListenerList<Listener> listeners;
    WeakReference<Component> ownerComponent;
    BorderSize<int> border { 1, 5, 1, 5 };
    float minimumHorizontalScale = 0;
    TextInputTarget::VirtualKeyboardType keyboardType = TextInputTarget::textKeyboard;
    bool editSingleClick = false;
    bool editDoubleClick = false;
    bool lossOfFocusDiscardsChanges = false;
    bool leftOfOwnerComp = false;

    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;
    bool updateFromTextEditorContents(UnicodeTextEditor&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnicodeLabel)
};
