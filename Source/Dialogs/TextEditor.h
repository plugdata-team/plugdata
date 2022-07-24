/** ============================================================================
 *
 *  TextEditor.hpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */

#pragma once
#include <JuceHeader.h>

#include "../LookAndFeel.h"

/**
 Factoring of responsibilities in the text editor classes:
 */
class Caret;         // draws the caret symbol(s)
class GutterComponent;        // draws the gutter
class GlyphArrangementArray;  // like StringArray but caches glyph positions
class HighlightComponent;     // draws the highlight region(s)
class Selection;              // stores leading and trailing edges of an editing region
class TextDocument;           // stores text data and caret ranges, supplies metrics, accepts actions
class PlugDataTextEditor;             // is a component, issues actions, computes view transform
class Transaction;            // a text replacement, the document computes the inverse on fulfilling it


//==============================================================================
template <typename ArgType, typename DataType>
class Memoizer
{
public:
    using FunctionType = std::function<DataType(ArgType)>;
    
    Memoizer (FunctionType f) : f (f) {}
    DataType operator() (ArgType argument) const
    {
        if (map.contains (argument))
        {
            return map.getReference (argument);
        }
        map.set (argument, f (argument));
        return this->operator() (argument);
    }
    FunctionType f;
    mutable HashMap<ArgType, DataType> map;
};



//==============================================================================
/**
 A data structure encapsulating a contiguous range within a TextDocument.
 The head and tail refer to the leading and trailing edges of a selected
 region (the head is where the caret would be rendered). The selection is
 exclusive with respect to the range of columns (y), but inclusive with
 respect to the range of rows (x). It is said to be oriented when
 head <= tail, and singular when head == tail, in which case it would be
 rendered without any highlighting.
 */
struct Selection
{
    enum class Part
    {
        head, tail, both,
    };
    
    Selection() {}
    Selection (Point<int> head) : head (head), tail (head) {}
    Selection (Point<int> head, Point<int> tail) : head (head), tail (tail) {}
    Selection (int r0, int c0, int r1, int c1) : head (r0, c0), tail (r1, c1) {}
    
    /** Construct a selection whose head is at (0, 0), and whose tail is at the end of
     the given content string, which may span multiple lines.
     */
    Selection (const String& content);
    
    bool operator== (const Selection& other) const
    {
        return head == other.head && tail == other.tail;
    }
    
    bool operator< (const Selection& other) const
    {
        const auto A = this->oriented();
        const auto B = other.oriented();
        if (A.head.x == B.head.x) return A.head.y < B.head.y;
        return A.head.x < B.head.x;
    }
    
    String toString() const
    {
        return "(" + head.toString() + ") - (" + tail.toString() + ")";
    }
    
    /** Whether or not this selection covers any extent. */
    bool isSingular() const { return head == tail; }
    
    /** Whether or not this selection is only a single line. */
    bool isSingleLine() const { return head.x == tail.x; }
    
    /** Whether the given row is within the selection. */
    bool intersectsRow (int row) const
    {
        return isOriented()
        ? head.x <= row && row <= tail.x
        : head.x >= row && row >= tail.x;
    }
    
    /** Return the range of columns this selection covers on the given row.
     */
    Range<int> getColumnRangeOnRow (int row, int numColumns) const
    {
        const auto A = oriented();
        
        if (row < A.head.x || row > A.tail.x)
            return { 0, 0 };
        if (row == A.head.x && row == A.tail.x)
            return { A.head.y, A.tail.y };
        if (row == A.head.x)
            return { A.head.y, numColumns };
        if (row == A.tail.x)
            return { 0, A.tail.y };
        return { 0, numColumns };
    }
    
    /** Whether the head precedes the tail. */
    bool isOriented() const;
    
    /** Return a copy of this selection, oriented so that head <= tail. */
    Selection oriented() const;
    
    /** Return a copy of this selection, with its head and tail swapped. */
    Selection swapped() const;
    
    /** Return a copy of this selection, with head and tail at the beginning and end
     of their respective lines if the selection is oriented, or otherwise with
     the head and tail at the end and beginning of their respective lines.
     */
    Selection horizontallyMaximized (const TextDocument& document) const;
    
    /** Return a copy of this selection, with its tail (if oriented) moved to
     account for the shape of the given content, which may span multiple
     lines. If instead head > tail, then the head is bumped forward.
     */
    Selection measuring (const String& content) const;
    
    /** Return a copy of this selection, with its head (if oriented) placed
     at the given index, and tail moved as to leave the measure the same.
     If instead head > tail, then the tail is moved.
     */
    Selection startingFrom (Point<int> index) const;
    
    Selection withStyle (int token) const { auto s = *this; s.token = token; return s; }
    
    /** Modify this selection (if necessary) to account for the disapearance of a
     selection someplace else.
     */
    void pullBy (Selection disappearingSelection);
    
    /** Modify this selection (if necessary) to account for the appearance of a
     selection someplace else.
     */
    void pushBy (Selection appearingSelection);
    
    /** Modify an index (if necessary) to account for the disapearance of
     this selection.
     */
    void pull (Point<int>& index) const;
    
    /** Modify an index (if necessary) to account for the appearance of
     this selection.
     */
    void push (Point<int>& index) const;
    
    Point<int> head; // (row, col) of the selection head (where the caret is drawn)
    Point<int> tail; // (row, col) of the tail
    int token = 0;
};




//==============================================================================
struct Transaction
{
    using Callback = std::function<void(const Transaction&)>;
    enum class Direction { forward, reverse };
    
    /** Return a copy of this transaction, corrected for delete and backspace
     characters. For example, if content == "\b" then the selection head is
     decremented and the content is erased.
     */
    Transaction accountingForSpecialCharacters (const TextDocument& document) const;
    
    /** Return an undoable action, whose perform method thill fulfill this
     transaction, and which caches the reciprocal transaction to be
     issued in the undo method.
     */
    UndoableAction* on (TextDocument& document, Callback callback);
    
    Selection selection;
    String content;
    Rectangle<float> affectedArea;
    Direction direction = Direction::forward;
    
private:
    class Undoable;
};




//==============================================================================
/**
 This class wraps a StringArray and memoizes the evaluation of glyph
 arrangements derived from the associated strings.
 */
class GlyphArrangementArray
{
public:
    int size() const { return lines.size(); }
    void clear() { lines.clear(); }
    void add (const String& string) { lines.add (string); }
    void insert (int index, const String& string) { lines.insert (index, string); }
    void removeRange (int startIndex, int numberToRemove) { lines.removeRange (startIndex, numberToRemove); }
    const String& operator[] (int index) const;
    
    int getToken (int row, int col, int defaultIfOutOfBounds) const;
    void clearTokens (int index);
    void applyTokens (int index, Selection zone);
    GlyphArrangement getGlyphs (int index,
                                float baseline,
                                int token,
                                bool withTrailingSpace=false) const;
    
private:
    friend class TextDocument;
    friend class PlugDataTextEditor;
    Font font;
    bool cacheGlyphArrangement = true;
    
    void ensureValid (int index) const;
    void invalidateAll();
    
    struct Entry
    {
        Entry() {}
        Entry (const String& string) : string (string) {}
        String string;
        GlyphArrangement glyphsWithTrailingSpace;
        GlyphArrangement glyphs;
        Array<int> tokens;
        bool glyphsAreDirty = true;
        bool tokensAreDirty = true;
    };
    mutable Array<Entry> lines;
};




//==============================================================================
class TextDocument
{
public:
    enum class Metric
    {
        top,
        ascent,
        baseline,
        descent,
        bottom,
    };
    
    /**
     Text categories the caret may be targeted to. For forward jumps,
     the caret is moved to be immediately in front of the first character
     in the given catagory. For backward jumps, it goes just after the
     first character of that category.
     */
    enum class Target
    {
        whitespace,
        punctuation,
        character,
        subword,
        word,
        token,
        line,
        paragraph,
        scope,
        document,
    };
    enum class Direction { forwardRow, backwardRow, forwardCol, backwardCol, };
    
    struct RowData
    {
        int rowNumber = 0;
        bool isRowSelected = false;
        Rectangle<float> bounds;
    };
    
    class Iterator
    {
    public:
        Iterator (const TextDocument& document, Point<int> index) noexcept : document (&document), index (index) { t = get(); }
        juce_wchar nextChar() noexcept      { if (isEOF()) return 0; auto s = t; document->next (index); t = get(); return s; }
        juce_wchar peekNextChar() noexcept  { return t; }
        void skip() noexcept                      { if (! isEOF()) { document->next (index); t = get(); } }
        void skipWhitespace() noexcept            { while (! isEOF() && CharacterFunctions::isWhitespace (t)) skip(); }
        void skipToEndOfLine() noexcept           { while (t != '\r' && t != '\n' && t != 0) skip(); }
        bool isEOF() const noexcept               { return index == document->getEnd(); }
        const Point<int>& getIndex() const noexcept { return index; }
    private:
        juce_wchar get() { return document->getCharacter (index); }
        juce_wchar t;
        const TextDocument* document;
        Point<int> index;
    };
    
    /** Get the current font. */
    Font getFont() const { return font; }
    
    /** Get the line spacing. */
    float getLineSpacing() const { return lineSpacing; }
    
    /** Set the font to be applied to all text. */
    void setFont (Font fontToUse) { font = fontToUse; lines.font = fontToUse; }
    
    String getText() const;
    
    /** Replace the whole document content. */
    void replaceAll (const String& content);
    
    /** Replace the list of selections with a new one. */
    void setSelections (const Array<Selection>& newSelections) { selections = newSelections; }
    
    /** Replace the selection at the given index. The index must be in range. */
    void setSelection (int index, Selection newSelection) { selections.setUnchecked (index, newSelection); }
    
    /** Get the number of rows in the document. */
    int getNumRows() const;
    
    /** Get the height of the text document. */
    float getHeight() const { return font.getHeight() * lineSpacing * getNumRows(); }
    
    /** Get the number of columns in the given row. */
    int getNumColumns (int row) const;
    
    /** Return the vertical position of a metric on a row. */
    float getVerticalPosition (int row, Metric metric) const;
    
    /** Return the position in the document at the given index, using the given
     metric for the vertical position. */
    Point<float> getPosition (Point<int> index, Metric metric) const;
    
    /** Return an array of rectangles covering the given selection. If
     the clip rectangle is empty, the whole selection is returned.
     Otherwise it gets only the overlapping parts.
     */
    Array<Rectangle<float>> getSelectionRegion (Selection selection,
                                                Rectangle<float> clip={}) const;
    
    /** Return the bounds of the entire document. */
    Rectangle<float> getBounds() const;
    
    /** Return the bounding box for the glyphs on the given row, and within
     the given range of columns. The range start must not be negative, and
     must be smaller than ncols. The range end is exclusive, and may be as
     large as ncols + 1, in which case the bounds include an imaginary
     whitespace character at the end of the line. The vertical extent is
     that of the whole line, not the ascent-to-descent of the glyph.
     */
    Rectangle<float> getBoundsOnRow (int row, Range<int> columns) const;
    
    /** Return the position of the glyph at the given row and column. */
    Rectangle<float> getGlyphBounds (Point<int> index) const;
    
    /** Return a glyph arrangement for the given row. If token != -1, then
     only glyphs with that token are returned.
     */
    GlyphArrangement getGlyphsForRow (int row, int token=-1, bool withTrailingSpace=false) const;
    
    /** Return all glyphs whose bounding boxes intersect the given area. This method
     may be generous (including glyphs that don't intersect). If token != -1, then
     only glyphs with that token mask are returned.
     */
    GlyphArrangement findGlyphsIntersecting (Rectangle<float> area, int token=-1) const;
    
    /** Return the range of rows intersecting the given rectangle. */
    Range<int> getRangeOfRowsIntersecting (Rectangle<float> area) const;
    
    /** Return data on the rows intersecting the given area. This is sort
     of a convenience method for calling getBoundsOnRow() over a range,
     but could be faster if horizontal extents are not computed.
     */
    Array<RowData> findRowsIntersecting (Rectangle<float> area,
                                         bool computeHorizontalExtent=false) const;
    
    /** Find the row and column index nearest to the given position. */
    Point<int> findIndexNearestPosition (Point<float> position) const;
    
    /** Return an index pointing to one-past-the-end. */
    Point<int> getEnd() const;
    
    /** Advance the given index by a single character, moving to the next
     line if at the end. Return false if the index cannot be advanced
     further.
     */
    bool next (Point<int>& index) const;
    
    /** Move the given index back by a single character, moving to the previous
     line if at the end. Return false if the index cannot be advanced
     further.
     */
    bool prev (Point<int>& index) const;
    
    /** Move the given index to the next row if possible. */
    bool nextRow (Point<int>& index) const;
    
    /** Move the given index to the previous row if possible. */
    bool prevRow (Point<int>& index) const;
    
    /** Navigate an index to the first character of the given categaory.
     */
    void navigate (Point<int>& index, Target target, Direction direction) const;
    
    /** Navigate all selections. */
    void navigateSelections (Target target, Direction direction, Selection::Part part);
    
    Selection search (Point<int> start, const String& target) const;
    
    /** Return the character at the given index. */
    juce_wchar getCharacter (Point<int> index) const;
    
    /** Add a selection to the list. */
    void addSelection (Selection selection) { selections.add (selection); }
    
    /** Return the number of active selections. */
    int getNumSelections() const { return selections.size(); }
    
    /** Return a line in the document. */
    const String& getLine (int lineIndex) const { return lines[lineIndex]; }
    
    /** Return one of the current selections. */
    const Selection& getSelection (int index) const;
    
    /** Return the current selection state. */
    const Array<Selection>& getSelections() const;
    
    /** Return the content within the given selection, with newlines if the
     selection spans muliple lines.
     */
    String getSelectionContent (Selection selection) const;
    
    /** Apply a transaction to the document, and return its reciprocal. The selection
     identified in the transaction does not need to exist in the document.
     */
    Transaction fulfill (const Transaction& transaction);
    
    /* Reset glyph token values on the given range of rows. */
    void clearTokens (Range<int> rows);
    
    /** Apply tokens from a set of zones to a range of rows. */
    void applyTokens (Range<int> rows, const Array<Selection>& zones);
    
private:
    friend class PlugDataTextEditor;
    
    float lineSpacing = 1.25f;
    mutable Rectangle<float> cachedBounds;
    GlyphArrangementArray lines;
    Font font;
    Array<Selection> selections;
};




//==============================================================================
class Caret : public Component, private Timer
{
public:
    Caret (const TextDocument& document);
    void setViewTransform (const AffineTransform& transformToUse);
    void updateSelections();
    
    //==========================================================================
    void paint (Graphics& g) override;
    
private:
    //==========================================================================
    float squareWave (float wt) const;
    void timerCallback() override;
    Array<Rectangle<float>> getCaretRectangles() const;
    //==========================================================================
    float phase = 0.f;
    const TextDocument& document;
    AffineTransform transform;
};




//==============================================================================
class GutterComponent : public Component
{
public:
    GutterComponent (const TextDocument& document);
    void setViewTransform (const AffineTransform& transformToUse);
    void updateSelections();
    
    //==========================================================================
    void paint (Graphics& g) override;
    
private:
    GlyphArrangement getLineNumberGlyphs (int row) const;
    //==========================================================================
    const TextDocument& document;
    AffineTransform transform;
    Memoizer<int, GlyphArrangement> memoizedGlyphArrangements;
};




//==============================================================================
class HighlightComponent : public Component
{
public:
    HighlightComponent (const TextDocument& document);
    void setViewTransform (const AffineTransform& transformToUse);
    void updateSelections();
    
    //==========================================================================
    void paint (Graphics& g) override;
    
private:
    static Path getOutlinePath (const Array<Rectangle<float>>& rectangles);
    
    //==========================================================================
    bool useRoundedHighlight = true;
    const TextDocument& document;
    AffineTransform transform;
    Path outlinePath;
};


//==============================================================================
class PlugDataTextEditor : public Component
{
public:
    enum class RenderScheme {
        usingAttributedStringSingle,
        usingAttributedString,
        usingGlyphArrangement,
    };
    
    PlugDataTextEditor();

    void setFont (Font font);
    
    void setText (const String& text);
    String getText() const;
    
    void translateView (float dx, float dy);
    void scaleView (float scaleFactor, float verticalCenter);
    
    //==========================================================================
    void resized() override;
    void paint (Graphics& g) override;
    void paintOverChildren (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseDoubleClick (const MouseEvent& e) override;
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& d) override;
    void mouseMagnify (const MouseEvent& e, float scaleFactor) override;
    bool keyPressed (const KeyPress& key) override;
    MouseCursor getMouseCursor() override;
    
private:
    //==========================================================================
    bool insert (const String& content);
    void updateViewTransform();
    void updateSelections();
    void translateToEnsureCaretIsVisible();
    
    //==========================================================================
    void renderTextUsingAttributedStringSingle (Graphics& g);
    void renderTextUsingAttributedString (Graphics& g);
    void renderTextUsingGlyphArrangement (Graphics& g);
    void resetProfilingData();
    bool enableSyntaxHighlighting = true;
    bool allowCoreGraphics = true;
    bool drawProfilingInfo = false;
    float accumulatedTimeInPaint = 0.f;
    float lastTimeInPaint = 0.f;
    float lastTokeniserTime = 0.f;
    int numPaintCalls = 0;
    RenderScheme renderScheme = RenderScheme::usingGlyphArrangement;
    
    //==========================================================================
    double lastTransactionTime;
    bool tabKeyUsed = true;
    TextDocument document;
    Caret caret;
    GutterComponent gutter;
    HighlightComponent highlight;
    
    float viewScaleFactor = 1.f;
    Point<float> translation;
    AffineTransform transform;
    UndoManager undo;
};

struct TextEditorDialog : public Component
{
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;
    
    PlugDataTextEditor editor;
    
    std::function<void(String)> onClose;
    
    TextEditorDialog() : resizer(this, &constrainer) {
        
        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(DocumentWindow::closeButton));
        addAndMakeVisible(closeButton.get());
        
        constrainer.setMinimumSize(500, 200);
        
        closeButton->onClick = [this](){
            // Call asynchronously because this function may distroy the dialog
            MessageManager::callAsync([this](){
                onClose(editor.getText());
            });
        };
        
        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);
        
        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(600, 400));
        addAndMakeVisible(resizer);
        addAndMakeVisible(editor);
        
        editor.grabKeyboardFocus();
    }
    
    void resized() {
        resizer.setBounds(getLocalBounds());
        closeButton->setBounds(getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5));
        editor.setBounds(getLocalBounds().withTrimmedTop(40));
    }
    
    
    void mouseDown(const MouseEvent& e) {
        windowDragger.startDraggingComponent(this, e);
    }
    
    void mouseDrag(const MouseEvent& e) {
        windowDragger.dragComponent(this, e, nullptr);
    }
    
    void paint(Graphics& g)
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(48, 39.5, getWidth(), 39.5);
    }
};
