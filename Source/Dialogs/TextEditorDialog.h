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

#include <utility>

#include "Utility/Config.h"
#include "Constants.h"

#define GUTTER_WIDTH 48.f
#define CURSOR_WIDTH 3.f
#define TEXT_INDENT 4.f
#define TEST_MULTI_CARET_EDITING false
#define ENABLE_CARET_BLINK true

/**
 Factoring of responsibilities in the text editor classes:
 */
class Caret;                 // draws the caret symbol(s)
class GutterComponent;       // draws the gutter
class GlyphArrangementArray; // like StringArray but caches glyph positions
class HighlightComponent;    // draws the highlight region(s)
struct Selection;            // stores leading and trailing edges of an editing region
class TextDocument;          // stores text data and caret ranges, supplies metrics, accepts actions
class PlugDataTextEditor;    // is a component, issues actions, computes view transform
class Transaction;           // a text replacement, the document computes the inverse on fulfilling it

template<typename ArgType, typename DataType>
class Memoizer {
public:
    using FunctionType = std::function<DataType(ArgType)>;

    explicit Memoizer(FunctionType f)
        : f(std::move(f))
    {
    }
    DataType operator()(ArgType argument) const
    {
        if (map.contains(argument)) {
            return map.getReference(argument);
        }
        map.set(argument, f(argument));
        return this->operator()(argument);
    }
    FunctionType f;
    mutable HashMap<ArgType, DataType> map;
};

/**
 A data structure encapsulating a contiguous range within a TextDocument.
 The head and tail refer to the leading and trailing edges of a selected
 region (the head is where the caret would be rendered). The selection is
 exclusive with respect to the range of columns (y), but inclusive with
 respect to the range of rows (x). It is said to be oriented when
 head <= tail, and singular when head == tail, in which case it would be
 rendered without any highlighting.
 */
struct Selection {
    enum class Part {
        head,
        tail,
        both,
    };

    Selection() = default;
    Selection(Point<int> head)
        : head(head)
        , tail(head)
    {
    }
    Selection(Point<int> head, Point<int> tail)
        : head(head)
        , tail(tail)
    {
    }
    Selection(int r0, int c0, int r1, int c1)
        : head(r0, c0)
        , tail(r1, c1)
    {
    }

    /** Construct a selection whose head is at (0, 0), and whose tail is at the end of
     the given content string, which may span multiple lines.
     */
    explicit Selection(String const& content);

    bool operator==(Selection const& other) const
    {
        return head == other.head && tail == other.tail;
    }

    bool operator<(Selection const& other) const
    {
        auto const A = this->oriented();
        auto const B = other.oriented();
        if (A.head.x == B.head.x)
            return A.head.y < B.head.y;
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
    bool intersectsRow(int row) const
    {
        return isOriented()
            ? head.x <= row && row <= tail.x
            : head.x >= row && row >= tail.x;
    }

    /** Return the range of columns this selection covers on the given row.
     */
    Range<int> getColumnRangeOnRow(int row, int numColumns) const
    {
        auto const A = oriented();

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
    Selection horizontallyMaximized(TextDocument const& document) const;

    /** Return a copy of this selection, with its tail (if oriented) moved to
     account for the shape of the given content, which may span multiple
     lines. If instead head > tail, then the head is bumped forward.
     */
    Selection measuring(String const& content) const;

    /** Return a copy of this selection, with its head (if oriented) placed
     at the given index, and tail moved as to leave the measure the same.
     If instead head > tail, then the tail is moved.
     */
    Selection startingFrom(Point<int> index) const;

    Selection withStyle(int token) const
    {
        auto s = *this;
        s.token = token;
        return s;
    }

    /** Modify this selection (if necessary) to account for the disapearance of a
     selection someplace else.
     */
    void pullBy(Selection disappearingSelection);

    /** Modify this selection (if necessary) to account for the appearance of a
     selection someplace else.
     */
    void pushBy(Selection appearingSelection);

    /** Modify an index (if necessary) to account for the disapearance of
     this selection.
     */
    void pull(Point<int>& index) const;

    /** Modify an index (if necessary) to account for the appearance of
     this selection.
     */
    void push(Point<int>& index) const;

    Point<int> head; // (row, col) of the selection head (where the caret is drawn)
    Point<int> tail; // (row, col) of the tail
    int token = 0;
};

class Transaction {
public:
    using Callback = std::function<void(Transaction const&)>;
    enum class Direction { forward,
        reverse };

    /** Return a copy of this transaction, corrected for delete and backspace
     characters. For example, if content == "\b" then the selection head is
     decremented and the content is erased.
     */
    Transaction accountingForSpecialCharacters(TextDocument const& document) const;

    /** Return an undoable action, whose perform method thill fulfill this
     transaction, and which caches the reciprocal transaction to be
     issued in the undo method.
     */
    UndoableAction* on(TextDocument& document, Callback callback);

    Selection selection;
    String content;
    Rectangle<float> affectedArea;
    Direction direction = Direction::forward;

private:
    class Undoable;
};

/**
 This class wraps a StringArray and memoizes the evaluation of glyph
 arrangements derived from the associated strings.
 */
class GlyphArrangementArray {
public:
    int size() const { return lines.size(); }
    void clear() { lines.clear(); }
    void add(String const& string) { lines.add(string); }
    void insert(int index, String const& string) { lines.insert(index, string); }
    void removeRange(int startIndex, int numberToRemove) { lines.removeRange(startIndex, numberToRemove); }
    String const& operator[](int index) const;

    int getToken(int row, int col, int defaultIfOutOfBounds) const;
    void clearTokens(int index);
    void applyTokens(int index, Selection zone);
    GlyphArrangement getGlyphs(int index,
        float baseline,
        int token,
        bool withTrailingSpace = false) const;

private:
    friend class TextDocument;
    friend class PlugDataTextEditor;
    Font font;
    bool cacheGlyphArrangement = true;

    void ensureValid(int index) const;
    void invalidateAll();

    struct Entry {
        Entry() = default;
        Entry(String string)
            : string(std::move(string))
        {
        }
        String string;
        GlyphArrangement glyphsWithTrailingSpace;
        GlyphArrangement glyphs;
        Array<int> tokens;
        bool glyphsAreDirty = true;
        bool tokensAreDirty = true;
    };
    mutable Array<Entry> lines;
};

class TextDocument {
public:
    enum class Metric {
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
    enum class Target {
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
    enum class Direction { forwardRow,
        backwardRow,
        forwardCol,
        backwardCol,
    };

    struct RowData {
        int rowNumber = 0;
        bool isRowSelected = false;
        Rectangle<float> bounds;
    };

    class Iterator {
    public:
        Iterator(TextDocument const& document, Point<int> index) noexcept
            : document(&document)
            , index(index)
        {
            t = get();
        }
        juce_wchar nextChar() noexcept
        {
            if (isEOF())
                return 0;
            auto s = t;
            document->next(index);
            t = get();
            return s;
        }
        juce_wchar peekNextChar() const noexcept { return t; }
        void skip() noexcept
        {
            if (!isEOF()) {
                document->next(index);
                t = get();
            }
        }
        void skipWhitespace() noexcept
        {
            while (!isEOF() && CharacterFunctions::isWhitespace(t))
                skip();
        }
        void skipToEndOfLine() noexcept
        {
            while (t != '\r' && t != '\n' && t != 0)
                skip();
        }
        bool isEOF() const noexcept { return index == document->getEnd(); }
        Point<int> const& getIndex() const noexcept { return index; }

    private:
        juce_wchar get() { return document->getCharacter(index); }
        juce_wchar t;
        TextDocument const* document;
        Point<int> index;
    };

    /** Get the current font. */
    Font getFont() const { return font; }

    /** Get the line spacing. */
    float getLineSpacing() const { return lineSpacing; }

    /** Set the font to be applied to all text. */
    void setFont(Font const& fontToUse)
    {
        font = fontToUse;
        lines.font = fontToUse;
    }

    StringArray getText() const;

    /** Replace the whole document content. */
    void replaceAll(String const& content);

    /** Replace the list of selections with a new one. */
    void setSelections(Array<Selection> const& newSelections) { selections = newSelections; }

    /** Replace the selection at the given index. The index must be in range. */
    void setSelection(int index, Selection newSelection) { selections.setUnchecked(index, newSelection); }

    /** Get the number of rows in the document. */
    int getNumRows() const;

    /** Get the height of the text document. */
    float getHeight() const { return font.getHeight() * lineSpacing * getNumRows(); }

    /** Get the number of columns in the given row. */
    int getNumColumns(int row) const;

    /** Return the vertical position of a metric on a row. */
    float getVerticalPosition(int row, Metric metric) const;

    /** Return the position in the document at the given index, using the given
     metric for the vertical position. */
    Point<float> getPosition(Point<int> index, Metric metric) const;

    /** Return an array of rectangles covering the given selection. If
     the clip rectangle is empty, the whole selection is returned.
     Otherwise it gets only the overlapping parts.
     */
    Array<Rectangle<float>> getSelectionRegion(Selection selection,
        Rectangle<float> clip = {}) const;

    /** Return the bounds of the entire document. */
    Rectangle<float> getBounds() const;

    /** Return the bounding object for the glyphs on the given row, and within
     the given range of columns. The range start must not be negative, and
     must be smaller than ncols. The range end is exclusive, and may be as
     large as ncols + 1, in which case the bounds include an imaginary
     whitespace character at the end of the line. The vertical extent is
     that of the whole line, not the ascent-to-descent of the glyph.
     */
    Rectangle<float> getBoundsOnRow(int row, Range<int> columns) const;

    /** Return the position of the glyph at the given row and column. */
    Rectangle<float> getGlyphBounds(Point<int> index) const;

    /** Return a glyph arrangement for the given row. If token != -1, then
     only glyphs with that token are returned.
     */
    GlyphArrangement getGlyphsForRow(int row, int token = -1, bool withTrailingSpace = false) const;

    /** Return all glyphs whose bounding boxes intersect the given area. This method
     may be generous (including glyphs that don't intersect). If token != -1, then
     only glyphs with that token mask are returned.
     */
    GlyphArrangement findGlyphsIntersecting(Rectangle<float> area, int token = -1) const;

    /** Return the range of rows intersecting the given rectangle. */
    Range<int> getRangeOfRowsIntersecting(Rectangle<float> area) const;

    /** Return data on the rows intersecting the given area. This is sort
     of a convenience method for calling getBoundsOnRow() over a range,
     but could be faster if horizontal extents are not computed.
     */
    Array<RowData> findRowsIntersecting(Rectangle<float> area,
        bool computeHorizontalExtent = false) const;

    /** Find the row and column index nearest to the given position. */
    Point<int> findIndexNearestPosition(Point<float> position) const;

    /** Return an index pointing to one-past-the-end. */
    Point<int> getEnd() const;

    /** Advance the given index by a single character, moving to the next
     line if at the end. Return false if the index cannot be advanced
     further.
     */
    bool next(Point<int>& index) const;

    /** Move the given index back by a single character, moving to the previous
     line if at the end. Return false if the index cannot be advanced
     further.
     */
    bool prev(Point<int>& index) const;

    /** Move the given index to the next row if possible. */
    bool nextRow(Point<int>& index) const;

    /** Move the given index to the previous row if possible. */
    bool prevRow(Point<int>& index) const;

    /** Navigate an index to the first character of the given categaory.
     */
    void navigate(Point<int>& index, Target target, Direction direction) const;

    /** Navigate all selections. */
    void navigateSelections(Target target, Direction direction, Selection::Part part);

    Selection search(Point<int> start, String const& target) const;

    /** Return the character at the given index. */
    juce_wchar getCharacter(Point<int> index) const;

    /** Add a selection to the list. */
    void addSelection(Selection selection) { selections.add(selection); }

    /** Return the number of active selections. */
    int getNumSelections() const { return selections.size(); }

    /** Return a line in the document. */
    String const& getLine(int lineIndex) const { return lines[lineIndex]; }

    /** Return one of the current selections. */
    Selection const& getSelection(int index) const;

    /** Return the current selection state. */
    Array<Selection> const& getSelections() const;

    /** Return the content within the given selection, with newlines if the
     selection spans muliple lines.
     */
    String getSelectionContent(Selection selection) const;

    /** Apply a transaction to the document, and return its reciprocal. The selection
     identified in the transaction does not need to exist in the document.
     */
    Transaction fulfill(Transaction const& transaction);

    /* Reset glyph token values on the given range of rows. */
    void clearTokens(Range<int> rows);

    /** Apply tokens from a set of zones to a range of rows. */
    void applyTokens(Range<int> rows, Array<Selection> const& zones);

private:
    friend class PlugDataTextEditor;

    float lineSpacing = 1.25f;
    mutable Rectangle<float> cachedBounds;
    GlyphArrangementArray lines;
    Font font;
    Array<Selection> selections;
};

class Caret : public Component
    , private Timer {
public:
    explicit Caret(TextDocument const& document);
    void setViewTransform(AffineTransform const& transformToUse);
    void updateSelections();

    void paint(Graphics& g) override;

private:
    static float squareWave(float wt);
    void timerCallback() override;
    Array<Rectangle<float>> getCaretRectangles() const;

    float phase = 0.f;
    TextDocument const& document;
    AffineTransform transform;
};

class GutterComponent : public Component {
public:
    explicit GutterComponent(TextDocument const& document);
    void setViewTransform(AffineTransform const& transformToUse);
    void updateSelections();

    void paint(Graphics& g) override;

private:
    GlyphArrangement getLineNumberGlyphs(int row) const;

    TextDocument const& document;
    AffineTransform transform;
    Memoizer<int, GlyphArrangement> memoizedGlyphArrangements;
};

class HighlightComponent : public Component {
public:
    explicit HighlightComponent(TextDocument const& document);
    void setViewTransform(AffineTransform const& transformToUse);
    void updateSelections();

    void paint(Graphics& g) override;

private:
    static Path getOutlinePath(Array<Rectangle<float>> const& rectangles);

    TextDocument const& document;
    AffineTransform transform;
    Path outlinePath;
};

class PlugDataTextEditor : public Component {
public:
    enum class RenderScheme {
        usingAttributedStringSingle,
        usingAttributedString,
        usingGlyphArrangement,
    };

    PlugDataTextEditor();

    void setFont(Font const& font);

    void setText(String const& text);
    String getText() const;

    void translateView(float dx, float dy);
    void scaleView(float scaleFactor, float verticalCenter);

    void resized() override;
    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseDoubleClick(MouseEvent const& e) override;
    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d) override;
    void mouseMagnify(MouseEvent const& e, float scaleFactor) override;
    bool keyPressed(KeyPress const& key) override;
    MouseCursor getMouseCursor() override;

    bool hasChanged() const { return changed; }

private:
    bool insert(String const& content);
    void updateViewTransform();
    void updateSelections();
    void translateToEnsureCaretIsVisible();

    void renderTextUsingAttributedStringSingle(Graphics& g);
    void renderTextUsingAttributedString(Graphics& g);
    void renderTextUsingGlyphArrangement(Graphics& g);

    bool enableSyntaxHighlighting = false;
    bool allowCoreGraphics = true;

    RenderScheme renderScheme = RenderScheme::usingAttributedStringSingle;

    double lastTransactionTime;
    bool tabKeyUsed = true;
    bool changed = false;
    TextDocument document;
    Caret caret;
    GutterComponent gutter;
    HighlightComponent highlight;

    float viewScaleFactor = 1.f;
    Point<float> translation;
    AffineTransform transform;
    UndoManager undo;
};

// IMPLEMENTATIONS

Caret::Caret(TextDocument const& document)
    : document(document)
{
    setInterceptsMouseClicks(false, false);
#if ENABLE_CARET_BLINK
    startTimerHz(20);
#endif
}

void Caret::setViewTransform(AffineTransform const& transformToUse)
{
    transform = transformToUse;
    repaint();
}

void Caret::updateSelections()
{
    phase = 0.f;
    repaint();
}

void Caret::paint(Graphics& g)
{
    g.setColour(getParentComponent()->findColour(CaretComponent::caretColourId).withAlpha(squareWave(phase)));

    for (auto const& r : getCaretRectangles())
        g.fillRect(r);
}

float Caret::squareWave(float wt)
{
    float const delta = 0.222f;
    float const A = 1.0;
    return 0.5f + A / 3.14159f * std::atan(std::cos(wt) / delta);
}

void Caret::timerCallback()
{
    phase += 3.2e-1;

    for (auto const& r : getCaretRectangles())
        repaint(r.getSmallestIntegerContainer());
}

Array<Rectangle<float>> Caret::getCaretRectangles() const
{
    Array<Rectangle<float>> rectangles;

    for (auto const& selection : document.getSelections()) {
        rectangles.add(document
                           .getGlyphBounds(selection.head)
                           .removeFromLeft(CURSOR_WIDTH)
                           .translated(selection.head.y == 0 ? 0 : -0.5f * CURSOR_WIDTH, 0.f)
                           .transformedBy(transform)
                           .expanded(0.f, 1.f));
    }
    return rectangles;
}

GutterComponent::GutterComponent(TextDocument const& document)
    : document(document)
    , memoizedGlyphArrangements([this](int row) { return getLineNumberGlyphs(row); })
{
    setInterceptsMouseClicks(false, false);
}

void GutterComponent::setViewTransform(AffineTransform const& transformToUse)
{
    transform = transformToUse;
    repaint();
}

void GutterComponent::updateSelections()
{
    repaint();
}

void GutterComponent::paint(Graphics& g)
{
    /*
     Draw the gutter background, shadow, and outline
     ------------------------------------------------------------------
     */
    auto ln = getParentComponent()->findColour(PlugDataColour::sidebarBackgroundColourId);

    g.setColour(ln);
    g.fillRect(getLocalBounds().removeFromLeft(GUTTER_WIDTH));

    if (transform.getTranslationX() < GUTTER_WIDTH) {
        auto shadowRect = getLocalBounds().withLeft(GUTTER_WIDTH).withWidth(12);

        auto gradient = ColourGradient::horizontal(ln.contrasting().withAlpha(0.3f),
            Colours::transparentBlack, shadowRect);
        g.setFillType(gradient);
        g.fillRect(shadowRect);
    } else {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawVerticalLine(GUTTER_WIDTH - 1.f, 0.f, getHeight());
    }

    /*
     Draw the line numbers and selected rows
     ------------------------------------------------------------------
     */
    auto area = g.getClipBounds().toFloat().transformedBy(transform.inverted());
    auto rowData = document.findRowsIntersecting(area);
    auto verticalTransform = transform.withAbsoluteTranslation(0.f, transform.getTranslationY());

    g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));

    for (auto const& r : rowData) {
        if (r.isRowSelected) {
            auto A = r.bounds
                         .transformedBy(transform)
                         .withX(0)
                         .withWidth(GUTTER_WIDTH);

            g.fillRoundedRectangle(A.reduced(4, 1), Corners::defaultCornerRadius);
        }
    }

    for (auto const& r : rowData) {
        g.setColour(getParentComponent()->findColour(PlugDataColour::panelTextColourId));
        memoizedGlyphArrangements(r.rowNumber).draw(g, verticalTransform);
    }
}

GlyphArrangement GutterComponent::getLineNumberGlyphs(int row) const
{
    GlyphArrangement glyphs;
    glyphs.addLineOfText(document.getFont().withHeight(12.f),
        String(row + 1),
        8.f, document.getVerticalPosition(row, TextDocument::Metric::baseline));
    return glyphs;
}

HighlightComponent::HighlightComponent(TextDocument const& document)
    : document(document)
{
    setInterceptsMouseClicks(false, false);
}

void HighlightComponent::setViewTransform(AffineTransform const& transformToUse)
{
    transform = transformToUse;

    outlinePath.clear();
    auto clip = getLocalBounds().toFloat().transformedBy(transform.inverted());

    for (auto const& s : document.getSelections()) {
        outlinePath.addPath(getOutlinePath(document.getSelectionRegion(s, clip)));
    }
    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::updateSelections()
{
    outlinePath.clear();
    auto clip = getLocalBounds().toFloat().transformedBy(transform.inverted());

    for (auto const& s : document.getSelections()) {
        outlinePath.addPath(getOutlinePath(document.getSelectionRegion(s, clip)));
    }
    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::paint(Graphics& g)
{
    g.addTransform(transform);
    auto highlight = getParentComponent()->findColour(CodeEditorComponent::highlightColourId);

    g.setColour(highlight);
    g.fillPath(outlinePath);

    g.setColour(highlight.darker());
    g.strokePath(outlinePath, PathStrokeType(1.f));
}

Path HighlightComponent::getOutlinePath(Array<Rectangle<float>> const& rectangles)
{
    auto p = Path();
    auto rect = rectangles.begin();

    if (rect == rectangles.end())
        return p;

    p.startNewSubPath(rect->getTopLeft());
    p.lineTo(rect->getBottomLeft());

    while (++rect != rectangles.end()) {
        p.lineTo(rect->getTopLeft());
        p.lineTo(rect->getBottomLeft());
    }

    while (rect-- != rectangles.begin()) {
        p.lineTo(rect->getBottomRight());
        p.lineTo(rect->getTopRight());
    }

    p.closeSubPath();
    return p.createPathWithRoundedCorners(4.f);
}

Selection::Selection(String const& content)
{
    int rowSpan = 0;
    int n = 0, lastLineStart = 0;
    auto c = content.getCharPointer();

    while (*c != '\0') {
        if (*c == '\n') {
            ++rowSpan;
            lastLineStart = n + 1;
        }
        ++c;
        ++n;
    }

    head = { 0, 0 };
    tail = { rowSpan, content.length() - lastLineStart };
}

bool Selection::isOriented() const
{
    return !(head.x > tail.x || (head.x == tail.x && head.y > tail.y));
}

Selection Selection::oriented() const
{
    if (!isOriented())
        return swapped();

    return *this;
}

Selection Selection::swapped() const
{
    Selection s = *this;
    std::swap(s.head, s.tail);
    return s;
}

Selection Selection::horizontallyMaximized(TextDocument const& document) const
{
    Selection s = *this;

    if (isOriented()) {
        s.head.y = 0;
        s.tail.y = document.getNumColumns(s.tail.x);
    } else {
        s.head.y = document.getNumColumns(s.head.x);
        s.tail.y = 0;
    }
    return s;
}

Selection Selection::measuring(String const& content) const
{
    Selection s(content);

    if (isOriented()) {
        return Selection(content).startingFrom(head);
    } else {
        return Selection(content).startingFrom(tail).swapped();
    }
}

Selection Selection::startingFrom(Point<int> index) const
{
    Selection s = *this;

    /*
     Pull the whole selection back to the origin.
     */
    s.pullBy(Selection({}, isOriented() ? head : tail));

    /*
     Then push it forward to the given index.
     */
    s.pushBy(Selection({}, index));

    return s;
}

void Selection::pullBy(Selection disappearingSelection)
{
    disappearingSelection.pull(head);
    disappearingSelection.pull(tail);
}

void Selection::pushBy(Selection appearingSelection)
{
    appearingSelection.push(head);
    appearingSelection.push(tail);
}

void Selection::pull(Point<int>& index) const
{
    auto const S = oriented();

    /*
     If the selection tail is on index's row, then shift its column back,
     either by the difference between our head and tail column indexes if
     our head and tail are on the same row, or otherwise by our tail's
     column index.
     */
    if (S.tail.x == index.x && S.head.y <= index.y) {
        if (S.head.x == S.tail.x) {
            index.y -= S.tail.y - S.head.y;
        } else {
            index.y -= S.tail.y;
        }
    }

    /*
     If this selection starts on the same row or an earlier one,
     then shift the row index back by our row span.
     */
    if (S.head.x <= index.x) {
        index.x -= S.tail.x - S.head.x;
    }
}

void Selection::push(Point<int>& index) const
{
    auto const S = oriented();

    /*
     If our head is on index's row, then shift its column forward, either
     by our head to tail distance if our head and tail are on the
     same row, or otherwise by our tail's column index.
     */
    if (S.head.x == index.x && S.head.y <= index.y) {
        if (S.head.x == S.tail.x) {
            index.y += S.tail.y - S.head.y;
        } else {
            index.y += S.tail.y;
        }
    }

    /*
     If this selection starts on the same row or an earlier one,
     then shift the row index forward by our row span.
     */
    if (S.head.x <= index.x) {
        index.x += S.tail.x - S.head.x;
    }
}

String const& GlyphArrangementArray::operator[](int index) const
{
    if (isPositiveAndBelow(index, lines.size())) {
        return lines.getReference(index).string;
    }

    static String empty;
    return empty;
}

int GlyphArrangementArray::getToken(int row, int col, int defaultIfOutOfBounds) const
{
    if (!isPositiveAndBelow(row, lines.size())) {
        return defaultIfOutOfBounds;
    }
    return lines.getReference(row).tokens[col];
}

void GlyphArrangementArray::clearTokens(int index)
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines.getReference(index);

    ensureValid(index);

    for (int col = 0; col < entry.tokens.size(); ++col) {
        entry.tokens.setUnchecked(col, 0);
    }
}

void GlyphArrangementArray::applyTokens(int index, Selection zone)
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines.getReference(index);
    auto range = zone.getColumnRangeOnRow(index, entry.tokens.size());

    ensureValid(index);

    for (int col = range.getStart(); col < range.getEnd(); ++col) {
        entry.tokens.setUnchecked(col, zone.token);
    }
}

GlyphArrangement GlyphArrangementArray::getGlyphs(int index,
    float baseline,
    int token,
    bool withTrailingSpace) const
{
    if (!isPositiveAndBelow(index, lines.size())) {
        GlyphArrangement glyphs;

        if (withTrailingSpace) {
            glyphs.addLineOfText(font, " ", TEXT_INDENT, baseline);
        }
        return glyphs;
    }
    ensureValid(index);

    auto& entry = lines.getReference(index);
    auto glyphSource = withTrailingSpace ? entry.glyphsWithTrailingSpace : entry.glyphs;
    auto glyphs = GlyphArrangement();

    for (int n = 0; n < glyphSource.getNumGlyphs(); ++n) {
        if (token == -1 || entry.tokens.getUnchecked(n) == token) {
            auto glyph = glyphSource.getGlyph(n);
            glyph.moveBy(TEXT_INDENT, baseline);
            glyphs.addGlyph(glyph);
        }
    }
    return glyphs;
}

void GlyphArrangementArray::ensureValid(int index) const
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines.getReference(index);

    if (entry.glyphsAreDirty) {
        entry.tokens.resize(entry.string.length());
        entry.glyphs.addLineOfText(font, entry.string, 0.f, 0.f);
        entry.glyphsWithTrailingSpace.addLineOfText(font, entry.string + " ", 0.f, 0.f);
        entry.glyphsAreDirty = !cacheGlyphArrangement;
    }
}

void GlyphArrangementArray::invalidateAll()
{
    for (auto& entry : lines) {
        entry.glyphsAreDirty = true;
        entry.tokensAreDirty = true;
    }
}

void TextDocument::replaceAll(String const& content)
{
    lines.clear();

    for (auto const& line : StringArray::fromLines(content)) {
        lines.add(line);
    }
}

StringArray TextDocument::getText() const
{
    StringArray text;
    for (int i = 0; i < lines.size(); i++) {
        text.add(lines[i]);
    }

    return text;
}

int TextDocument::getNumRows() const
{
    return lines.size();
}

int TextDocument::getNumColumns(int row) const
{
    return lines[row].length();
}

float TextDocument::getVerticalPosition(int row, Metric metric) const
{
    float lineHeight = font.getHeight() * lineSpacing;
    float gap = font.getHeight() * (lineSpacing - 1.f) * 0.5f;

    switch (metric) {
    case Metric::top:
        return lineHeight * row;
    case Metric::ascent:
        return lineHeight * row + gap;
    case Metric::baseline:
        return lineHeight * row + gap + font.getAscent();
    case Metric::descent:
        return lineHeight * row + gap + font.getAscent() + font.getDescent();
    case Metric::bottom:
        return lineHeight * row + lineHeight;
    default:
        return lineHeight * row;
    }
}

Point<float> TextDocument::getPosition(Point<int> index, Metric metric) const
{
    return { getGlyphBounds(index).getX(), getVerticalPosition(index.x, metric) };
}

Array<Rectangle<float>> TextDocument::getSelectionRegion(Selection selection, Rectangle<float> clip) const
{
    Array<Rectangle<float>> patches;
    Selection s = selection.oriented();

    if (s.head.x == s.tail.x) {
        int c0 = s.head.y;
        int c1 = s.tail.y;
        patches.add(getBoundsOnRow(s.head.x, Range<int>(c0, c1)));
    } else {
        int r0 = s.head.x;
        int c0 = s.head.y;
        int r1 = s.tail.x;
        int c1 = s.tail.y;

        for (int n = r0; n <= r1; ++n) {
            if (!clip.isEmpty() && !clip.getVerticalRange().intersects({ getVerticalPosition(n, Metric::top), getVerticalPosition(n, Metric::bottom) }))
                continue;

            if (n == r1 && c1 == 0)
                continue;
            else if (n == r0)
                patches.add(getBoundsOnRow(r0, Range<int>(c0, getNumColumns(r0) + 1)));
            else if (n == r1)
                patches.add(getBoundsOnRow(r1, Range<int>(0, c1)));
            else
                patches.add(getBoundsOnRow(n, Range<int>(0, getNumColumns(n) + 1)));
        }
    }
    return patches;
}

Rectangle<float> TextDocument::getBounds() const
{
    if (cachedBounds.isEmpty()) {
        auto bounds = Rectangle<float>();

        for (int n = 0; n < getNumRows(); ++n) {
            bounds = bounds.getUnion(getBoundsOnRow(n, Range<int>(0, std::max(1, getNumColumns(n)))));
        }
        return cachedBounds = bounds;
    }
    return cachedBounds;
}

Rectangle<float> TextDocument::getBoundsOnRow(int row, Range<int> columns) const
{
    return getGlyphsForRow(row, -1, true)
        .getBoundingBox(columns.getStart(), columns.getLength(), true)
        .withTop(getVerticalPosition(row, Metric::top))
        .withBottom(getVerticalPosition(row, Metric::bottom));
}

Rectangle<float> TextDocument::getGlyphBounds(Point<int> index) const
{
    index.y = jlimit(0, getNumColumns(index.x), index.y);
    return getBoundsOnRow(index.x, Range<int>(index.y, index.y + 1));
}

GlyphArrangement TextDocument::getGlyphsForRow(int row, int token, bool withTrailingSpace) const
{
    return lines.getGlyphs(row,
        getVerticalPosition(row, Metric::baseline),
        token,
        withTrailingSpace);
}

GlyphArrangement TextDocument::findGlyphsIntersecting(Rectangle<float> area, int token) const
{
    auto range = getRangeOfRowsIntersecting(area);
    auto rows = Array<RowData>();
    auto glyphs = GlyphArrangement();

    for (int n = range.getStart(); n < range.getEnd(); ++n) {
        glyphs.addGlyphArrangement(getGlyphsForRow(n, token));
    }
    return glyphs;
}

Range<int> TextDocument::getRangeOfRowsIntersecting(Rectangle<float> area) const
{
    auto lineHeight = font.getHeight() * lineSpacing;
    auto row0 = jlimit(0, jmax(getNumRows() - 1, 0), int(area.getY() / lineHeight));
    auto row1 = jlimit(0, jmax(getNumRows() - 1, 0), int(area.getBottom() / lineHeight));
    return { row0, row1 + 1 };
}

Array<TextDocument::RowData> TextDocument::findRowsIntersecting(Rectangle<float> area,
    bool computeHorizontalExtent) const
{
    auto range = getRangeOfRowsIntersecting(area);
    auto rows = Array<RowData>();

    for (int n = range.getStart(); n < range.getEnd(); ++n) {
        RowData data;
        data.rowNumber = n;

        if (computeHorizontalExtent) // slower
        {
            data.bounds = getBoundsOnRow(n, Range<int>(0, getNumColumns(n)));
        } else // faster
        {
            data.bounds.setY(getVerticalPosition(n, Metric::top));
            data.bounds.setBottom(getVerticalPosition(n, Metric::bottom));
        }

        for (auto const& s : selections) {
            if (s.intersectsRow(n)) {
                data.isRowSelected = true;
                break;
            }
        }
        rows.add(data);
    }
    return rows;
}

Point<int> TextDocument::findIndexNearestPosition(Point<float> position) const
{
    auto lineHeight = font.getHeight() * lineSpacing;
    auto row = jlimit(0, jmax(getNumRows() - 1, 0), int(position.y / lineHeight));
    auto col = 0;
    auto glyphs = getGlyphsForRow(row);

    if (position.x > 0.f) {
        col = glyphs.getNumGlyphs();

        for (int n = 0; n < glyphs.getNumGlyphs(); ++n) {
            if (glyphs.getBoundingBox(n, 1, true).getHorizontalRange().contains(position.x)) {
                col = n;
                break;
            }
        }
    }
    return { row, col };
}

Point<int> TextDocument::getEnd() const
{
    return { getNumRows(), 0 };
}

bool TextDocument::next(Point<int>& index) const
{
    if (index.y < getNumColumns(index.x)) {
        index.y += 1;
        return true;
    } else if (index.x < getNumRows()) {
        index.x += 1;
        index.y = 0;
        return true;
    }
    return false;
}

bool TextDocument::prev(Point<int>& index) const
{
    if (index.y > 0) {
        index.y -= 1;
        return true;
    } else if (index.x > 0) {
        index.x -= 1;
        index.y = getNumColumns(index.x);
        return true;
    }
    return false;
}

bool TextDocument::nextRow(Point<int>& index) const
{
    if (index.x < getNumRows()) {
        index.x += 1;
        index.y = jmin(index.y, getNumColumns(index.x));
        return true;
    }
    return false;
}

bool TextDocument::prevRow(Point<int>& index) const
{
    if (index.x > 0) {
        index.x -= 1;
        index.y = jmin(index.y, getNumColumns(index.x));
        return true;
    }
    return false;
}

void TextDocument::navigate(Point<int>& i, Target target, Direction direction) const
{
    std::function<bool(Point<int>&)> advance;
    std::function<juce_wchar(Point<int>&)> get;

    using CF = CharacterFunctions;
    static String punctuation = "{}<>()[],.;:";

    switch (direction) {
    case Direction::forwardRow:
        advance = [this](Point<int>& i) { return nextRow(i); };
        get = [this](Point<int> i) { return getCharacter(i); };
        break;
    case Direction::backwardRow:
        advance = [this](Point<int>& i) { return prevRow(i); };
        get = [this](Point<int> i) { prev (i); return getCharacter (i); };
        break;
    case Direction::forwardCol:
        advance = [this](Point<int>& i) { return next(i); };
        get = [this](Point<int> i) { return getCharacter(i); };
        break;
    case Direction::backwardCol:
        advance = [this](Point<int>& i) { return prev(i); };
        get = [this](Point<int> i) { prev (i); return getCharacter (i); };
        break;
    }

    switch (target) {
    case Target::whitespace:
        while (!CF::isWhitespace(get(i)) && advance(i)) { }
        break;
    case Target::punctuation:
        while (!punctuation.containsChar(get(i)) && advance(i)) { }
        break;
    case Target::character:
        advance(i);
        break;
    case Target::subword:
        jassertfalse;
        break; // IMPLEMENT ME
    case Target::word:
        while (CF::isWhitespace(get(i)) && advance(i)) { }
        break;
    case Target::token: {
        int s = lines.getToken(i.x, i.y, -1);
        int t = s;

        while (s == t && advance(i)) {
            if (getNumColumns(i.x) > 0) {
                s = t;
                t = lines.getToken(i.x, i.y, s);
            }
        }
        break;
    }
    case Target::line:
        while (get(i) != '\n' && advance(i)) { }
        break;
    case Target::paragraph:
        while (getNumColumns(i.x) > 0 && advance(i)) { }
        break;
    case Target::scope:
        jassertfalse;
        break; // IMPLEMENT ME
    case Target::document:
        while (advance(i)) { }
        break;
    }
}

void TextDocument::navigateSelections(Target target, Direction direction, Selection::Part part)
{
    for (auto& selection : selections) {
        switch (part) {
        case Selection::Part::head:
            navigate(selection.head, target, direction);
            break;
        case Selection::Part::tail:
            navigate(selection.tail, target, direction);
            break;
        case Selection::Part::both:
            navigate(selection.head, target, direction);
            selection.tail = selection.head;
            break;
        }
    }
}

Selection TextDocument::search(Point<int> start, String const& target) const
{
    while (start != getEnd()) {
        auto y = lines[start.x].indexOf(start.y, target);

        if (y != -1)
            return Selection(start.x, y, start.x, y + target.length());

        start.y = 0;
        start.x += 1;
    }
    return {};
}

juce_wchar TextDocument::getCharacter(Point<int> index) const
{
    jassert(0 <= index.x && index.x <= lines.size());
    jassert(0 <= index.y && index.y <= lines[index.x].length());

    if (index == getEnd() || index.y == lines[index.x].length()) {
        return '\n';
    }
    return lines[index.x].getCharPointer()[index.y];
}

Selection const& TextDocument::getSelection(int index) const
{
    return selections.getReference(index);
}

Array<Selection> const& TextDocument::getSelections() const
{
    return selections;
}

String TextDocument::getSelectionContent(Selection s) const
{
    s = s.oriented();

    if (s.isSingleLine()) {
        return lines[s.head.x].substring(s.head.y, s.tail.y);
    } else {
        String content = lines[s.head.x].substring(s.head.y) + "\n";

        for (int row = s.head.x + 1; row < s.tail.x; ++row) {
            content += lines[row] + "\n";
        }
        content += lines[s.tail.x].substring(0, s.tail.y);
        return content;
    }
}

Transaction TextDocument::fulfill(Transaction const& transaction)
{
    cachedBounds = {}; // invalidate the bounds

    auto const t = transaction.accountingForSpecialCharacters(*this);
    auto const s = t.selection.oriented();
    auto const L = getSelectionContent(s.horizontallyMaximized(*this));
    auto const i = s.head.y;
    auto const j = L.lastIndexOf("\n") + s.tail.y + 1;
    auto const M = L.substring(0, i) + t.content + L.substring(j);

    for (auto& existingSelection : selections) {
        existingSelection.pullBy(s);
        existingSelection.pushBy(Selection(t.content).startingFrom(s.head));
    }

    lines.removeRange(s.head.x, s.tail.x - s.head.x + 1);
    int row = s.head.x;

    if (M.isEmpty()) {
        lines.insert(row++, String());
    }
    for (auto const& line : StringArray::fromLines(M)) {
        lines.insert(row++, line);
    }

    using D = Transaction::Direction;
    auto inf = std::numeric_limits<float>::max();

    Transaction r;
    r.selection = Selection(t.content).startingFrom(s.head);
    r.content = L.substring(i, j);
    r.affectedArea = Rectangle<float>(0, 0, inf, inf);
    r.direction = t.direction == D::forward ? D::reverse : D::forward;

    return r;
}

void TextDocument::clearTokens(Range<int> rows)
{
    for (int n = rows.getStart(); n < rows.getEnd(); ++n) {
        lines.clearTokens(n);
    }
}

void TextDocument::applyTokens(Range<int> rows, Array<Selection> const& zones)
{
    for (int n = rows.getStart(); n < rows.getEnd(); ++n) {
        for (auto const& zone : zones) {
            if (zone.intersectsRow(n)) {
                lines.applyTokens(n, zone);
            }
        }
    }
}

class Transaction::Undoable : public UndoableAction {
public:
    Undoable(TextDocument& document, Callback callback, Transaction forward)
        : document(document)
        , callback(std::move(callback))
        , forward(std::move(forward))
    {
    }

    bool perform() override
    {
        callback(reverse = document.fulfill(forward));
        return true;
    }

    bool undo() override
    {
        callback(forward = document.fulfill(reverse));
        return true;
    }

    TextDocument& document;
    Callback callback;
    Transaction forward;
    Transaction reverse;
};

Transaction Transaction::accountingForSpecialCharacters(TextDocument const& document) const
{
    Transaction t = *this;
    auto& s = t.selection;

    if (content.getLastCharacter() == KeyPress::tabKey) {
        t.content = "    ";
    }
    if (content.getLastCharacter() == KeyPress::backspaceKey) {
        if (s.head.y == s.tail.y) {
            document.prev(s.head);
        }
        t.content.clear();
    } else if (content.getLastCharacter() == KeyPress::deleteKey) {
        if (s.head.y == s.tail.y) {
            document.next(s.head);
        }
        t.content.clear();
    }
    return t;
}

UndoableAction* Transaction::on(TextDocument& document, Callback callback)
{
    return new Undoable(document, std::move(callback), *this);
}

PlugDataTextEditor::PlugDataTextEditor()
    : caret(document)
    , gutter(document)
    , highlight(document)
{
    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections({ Selection() });

    setFont(Font(Fonts::getCurrentFont().withHeight(15)));

    translateView(GUTTER_WIDTH, 0);
    setWantsKeyboardFocus(true);

    addAndMakeVisible(highlight);
    addAndMakeVisible(caret);
    addAndMakeVisible(gutter);
}

void PlugDataTextEditor::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawHorizontalLine(0, 0, getWidth());
    g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
}

void PlugDataTextEditor::setFont(Font const& font)
{
    document.setFont(font);
    repaint();
}

void PlugDataTextEditor::setText(String const& text)
{
    document.replaceAll(text);
    repaint();
}

String PlugDataTextEditor::getText() const
{
    return document.getText().joinIntoString("\r");
}

void PlugDataTextEditor::translateView(float dx, float dy)
{
    auto W = viewScaleFactor * document.getBounds().getWidth();
    auto H = viewScaleFactor * document.getBounds().getHeight();

    translation.x = jlimit(jmin(GUTTER_WIDTH, -W + getWidth()), GUTTER_WIDTH, translation.x + dx);
    translation.y = jlimit(jmin(-0.f, -H + (getHeight() - 10)), 0.0f, translation.y + dy);

    updateViewTransform();
}

void PlugDataTextEditor::scaleView(float scaleFactorMultiplier, float verticalCenter)
{
    auto newS = viewScaleFactor * scaleFactorMultiplier;
    auto fixedy = Point<float>(0, verticalCenter).transformedBy(transform.inverted()).y;

    translation.y = -newS * fixedy + verticalCenter;
    viewScaleFactor = newS;
    updateViewTransform();
}

void PlugDataTextEditor::updateViewTransform()
{
    transform = AffineTransform::scale(viewScaleFactor).translated(translation.x, translation.y);
    highlight.setViewTransform(transform);
    caret.setViewTransform(transform);
    gutter.setViewTransform(transform);
    repaint();
}

void PlugDataTextEditor::updateSelections()
{
    highlight.updateSelections();
    caret.updateSelections();
    gutter.updateSelections();
}

void PlugDataTextEditor::translateToEnsureCaretIsVisible()
{
    auto i = document.getSelections().getLast().head;
    auto t = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::top)).transformedBy(transform);
    auto b = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::bottom)).transformedBy(transform);

    if (t.y < 0.f) {
        translateView(0.f, -t.y);
    } else if (b.y > getHeight()) {
        translateView(0.f, -b.y + getHeight());
    }
}

void PlugDataTextEditor::resized()
{
    highlight.setBounds(getLocalBounds());
    caret.setBounds(getLocalBounds());
    gutter.setBounds(getLocalBounds());
}

void PlugDataTextEditor::paint(Graphics& g)
{
    g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));

    String renderSchemeString;

    switch (renderScheme) {
    case RenderScheme::usingAttributedStringSingle:
        renderTextUsingAttributedStringSingle(g);
        renderSchemeString = "AttributedStringSingle";
        break;
    case RenderScheme::usingAttributedString:
        renderTextUsingAttributedString(g);
        renderSchemeString = "attr. str";
        break;
    case RenderScheme::usingGlyphArrangement:
        renderTextUsingGlyphArrangement(g);
        renderSchemeString = "glyph arr.";
        break;
    }
}

void PlugDataTextEditor::mouseDown(MouseEvent const& e)
{
    if (e.getNumberOfClicks() > 1) {
        return;
    }

    auto selections = document.getSelections();
    auto index = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));

    if (selections.contains(index)) {
        return;
    }
    if (!e.mods.isCommandDown() || !TEST_MULTI_CARET_EDITING) {
        selections.clear();
    }

    selections.add(index);
    document.setSelections(selections);
    updateSelections();
}

void PlugDataTextEditor::mouseDrag(MouseEvent const& e)
{
    if (e.mouseWasDraggedSinceMouseDown()) {
        auto selection = document.getSelections().getFirst();
        selection.head = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));
        document.setSelections({ selection });
        translateToEnsureCaretIsVisible();
        updateSelections();
    }
}

void PlugDataTextEditor::mouseDoubleClick(MouseEvent const& e)
{
    if (e.getNumberOfClicks() == 2) {
        document.navigateSelections(TextDocument::Target::whitespace, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections(TextDocument::Target::whitespace, TextDocument::Direction::forwardCol, Selection::Part::tail);
        updateSelections();
    } else if (e.getNumberOfClicks() == 3) {
        document.navigateSelections(TextDocument::Target::line, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections(TextDocument::Target::line, TextDocument::Direction::forwardCol, Selection::Part::tail);
        updateSelections();
    }
    updateSelections();
}

void PlugDataTextEditor::mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d)
{
    float dx = d.deltaX;
    /*
     make scrolling away from the gutter just a little "sticky"
     */
    if (translation.x == GUTTER_WIDTH && -0.01f < dx && dx < 0.f) {
        dx = 0.f;
    }
    translateView(dx * 400, d.deltaY * 800);
}

void PlugDataTextEditor::mouseMagnify(MouseEvent const& e, float scaleFactor)
{
    scaleView(scaleFactor, e.position.y);
}

bool PlugDataTextEditor::keyPressed(KeyPress const& key)
{

    using Target = TextDocument::Target;
    using Direction = TextDocument::Direction;
    auto mods = key.getModifiers();
    auto isTab = tabKeyUsed && key == KeyPress::tabKey;
    auto isBackspace = key == KeyPress::backspaceKey;

    auto nav = [this, mods](Target target, Direction direction) {
        if (mods.isShiftDown())
            document.navigateSelections(target, direction, Selection::Part::head);
        else
            document.navigateSelections(target, direction, Selection::Part::both);

        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expandBack = [this](Target target, Direction direction) {
        document.navigateSelections(target, direction, Selection::Part::head);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expand = [this](Target target) {
        document.navigateSelections(target, Direction::backwardCol, Selection::Part::head);
        document.navigateSelections(target, Direction::forwardCol, Selection::Part::tail);
        updateSelections();
        return true;
    };
    auto addCaret = [this](Target target, Direction direction) {
        auto s = document.getSelections().getLast();
        document.navigate(s.head, target, direction);
        document.addSelection(s);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto addSelectionAtNextMatch = [this]() {
        const auto& s = document.getSelections().getLast();

        if (!s.isSingleLine()) {
            return false;
        }
        auto t = document.search(s.tail, document.getSelectionContent(s));

        if (t.isSingular()) {
            return false;
        }
        document.addSelection(t);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    if (key.isKeyCode(KeyPress::escapeKey)) {
        document.setSelections(document.getSelections().getLast());
        updateSelections();
        return true;
    }
    if (mods.isCtrlDown() && mods.isAltDown()) {
        if (key.isKeyCode(KeyPress::downKey))
            return addCaret(Target::character, Direction::forwardRow);
        if (key.isKeyCode(KeyPress::upKey))
            return addCaret(Target::character, Direction::backwardRow);
    }
    if (mods.isCtrlDown()) {
        if (key.isKeyCode(KeyPress::rightKey))
            return nav(Target::whitespace, Direction::forwardCol) && nav(Target::word, Direction::forwardCol);
        if (key.isKeyCode(KeyPress::leftKey))
            return nav(Target::whitespace, Direction::backwardCol) && nav(Target::word, Direction::backwardCol);
        if (key.isKeyCode(KeyPress::downKey))
            return nav(Target::word, Direction::forwardCol) && nav(Target::paragraph, Direction::forwardRow);
        if (key.isKeyCode(KeyPress::upKey))
            return nav(Target::word, Direction::backwardCol) && nav(Target::paragraph, Direction::backwardRow);

        if (key.isKeyCode(KeyPress::backspaceKey))
            return (expandBack(Target::whitespace, Direction::backwardCol)
                && expandBack(Target::word, Direction::backwardCol)
                && insert(""));

        if (key == KeyPress('e', ModifierKeys::ctrlModifier, 0) || key == KeyPress('e', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav(Target::line, Direction::forwardCol);

        if (key == KeyPress('a', ModifierKeys::ctrlModifier, 0) || key == KeyPress('a', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav(Target::line, Direction::backwardCol);
    }
    if (mods.isCommandDown()) {
        if (key.isKeyCode(KeyPress::downKey))
            return nav(Target::document, Direction::forwardRow);
        if (key.isKeyCode(KeyPress::upKey))
            return nav(Target::document, Direction::backwardRow);
    }

    if (key.isKeyCode(KeyPress::rightKey))
        return nav(Target::character, Direction::forwardCol);
    if (key.isKeyCode(KeyPress::leftKey))
        return nav(Target::character, Direction::backwardCol);
    if (key.isKeyCode(KeyPress::downKey))
        return nav(Target::character, Direction::forwardRow);
    if (key.isKeyCode(KeyPress::upKey))
        return nav(Target::character, Direction::backwardRow);

    if (key == KeyPress('a', ModifierKeys::commandModifier, 0))
        return expand(Target::document);
    if (key == KeyPress('d', ModifierKeys::commandModifier, 0))
        return expand(Target::whitespace);
    if (key == KeyPress('e', ModifierKeys::commandModifier, 0))
        return expand(Target::token);
    if (key == KeyPress('l', ModifierKeys::commandModifier, 0))
        return expand(Target::line);
    if (key == KeyPress('f', ModifierKeys::commandModifier, 0))
        return addSelectionAtNextMatch();
    if (key == KeyPress('z', ModifierKeys::commandModifier, 0))
        return undo.undo();
    if (key == KeyPress('r', ModifierKeys::commandModifier, 0))
        return undo.redo();

    if (key == KeyPress('x', ModifierKeys::commandModifier, 0)) {
        SystemClipboard::copyTextToClipboard(document.getSelectionContent(document.getSelections().getFirst()));
        return insert("");
    }
    if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
        SystemClipboard::copyTextToClipboard(document.getSelectionContent(document.getSelections().getFirst()));
        return true;
    }

    if (key == KeyPress('v', ModifierKeys::commandModifier, 0))
        return insert(SystemClipboard::getTextFromClipboard());
    if (key == KeyPress('d', ModifierKeys::ctrlModifier, 0))
        return insert(String::charToString(KeyPress::deleteKey));
    if (key.isKeyCode(KeyPress::returnKey))
        return insert("\n");
    if (key.getTextCharacter() >= ' ' || isTab || isBackspace)
        return insert(String::charToString(key.getTextCharacter()));

    return false;
}

bool PlugDataTextEditor::insert(String const& content)
{
    double now = Time::getApproximateMillisecondCounter();

    if (now > lastTransactionTime + 400) {
        lastTransactionTime = Time::getApproximateMillisecondCounter();
        undo.beginNewTransaction();
    }

    for (int n = 0; n < document.getNumSelections(); ++n) {
        Transaction t;
        t.content = content;
        t.selection = document.getSelection(n);

        auto callback = [this, n](Transaction const& r) {
            switch (r.direction) // NB: switching on the direction of the reciprocal here
            {
            case Transaction::Direction::forward:
                document.setSelection(n, r.selection);
                break;
            case Transaction::Direction::reverse:
                document.setSelection(n, r.selection.tail);
                break;
            }

            if (!r.affectedArea.isEmpty()) {
                repaint(r.affectedArea.transformedBy(transform).getSmallestIntegerContainer());
            }
        };
        undo.perform(t.on(document, callback));
    }
    updateSelections();
    changed = true;

    return true;
}

MouseCursor PlugDataTextEditor::getMouseCursor()
{
    return getMouseXYRelative().x < GUTTER_WIDTH ? MouseCursor::NormalCursor : MouseCursor::IBeamCursor;
}

void PlugDataTextEditor::renderTextUsingAttributedStringSingle(Graphics& g)
{
    g.saveState();
    g.addTransform(transform);

    auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
    auto font = document.getFont();
    auto rows = document.getRangeOfRowsIntersecting(g.getClipBounds().toFloat());
    auto T = document.getVerticalPosition(rows.getStart(), TextDocument::Metric::ascent);
    auto B = document.getVerticalPosition(rows.getEnd(), TextDocument::Metric::top);
    auto W = 10000;
    auto bounds = Rectangle<float>::leftTopRightBottom(TEXT_INDENT, T, W, B);
    auto content = document.getSelectionContent(Selection(rows.getStart(), 0, rows.getEnd(), 0));

    AttributedString s;
    s.setLineSpacing((document.getLineSpacing() - 1.f) * font.getHeight());

    CppTokeniserFunctions::StringIterator si(content);
    auto previous = si.t;

    while (!si.isEOF()) {
        auto tokenType = CppTokeniserFunctions::readNextToken(si);
        auto colour = enableSyntaxHighlighting ? colourScheme.types[tokenType].colour : findColour(PlugDataColour::panelTextColourId);
        auto token = String(previous, si.t);

        previous = si.t;
        s.append(token, font, colour);
    }

    if (allowCoreGraphics) {
        s.draw(g, bounds);
    } else {
        TextLayout layout;
        layout.createLayout(s, bounds.getWidth());
        layout.draw(g, bounds);
    }
    g.restoreState();
}

void PlugDataTextEditor::renderTextUsingAttributedString(Graphics& g)
{
    /*
     Credit to chrisboy2000 for this
     */
    auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
    auto originalHeight = document.getFont().getHeight();

    auto scaleFactor = std::sqrt(std::abs(transform.getDeterminant()));
    auto font = document.getFont().withHeight(originalHeight * scaleFactor);
    auto rows = document.findRowsIntersecting(g.getClipBounds().toFloat().transformedBy(transform.inverted()));

    for (auto const& r : rows) {
        auto line = document.getLine(r.rowNumber);
        auto T = document.getVerticalPosition(r.rowNumber, TextDocument::Metric::ascent);
        auto B = document.getVerticalPosition(r.rowNumber, TextDocument::Metric::bottom);
        auto bounds = Rectangle<float>::leftTopRightBottom(0.f, T, 1000.f, B).transformedBy(transform);

        AttributedString s;

        if (!enableSyntaxHighlighting) {
            s.append(line, font);
        } else {
            CppTokeniserFunctions::StringIterator si(line);
            auto previous = si.t;

            while (!si.isEOF()) {
                auto tokenType = CppTokeniserFunctions::readNextToken(si);
                auto colour = enableSyntaxHighlighting ? colourScheme.types[tokenType].colour : findColour(PlugDataColour::panelTextColourId);
                auto token = String(previous, si.t);

                previous = si.t;
                s.append(token, font, colour);
            }
        }
        if (allowCoreGraphics) {
            s.draw(g, bounds);
        } else {
            TextLayout layout;
            layout.createLayout(s, bounds.getWidth());
            layout.draw(g, bounds);
        }
    }
}

void PlugDataTextEditor::renderTextUsingGlyphArrangement(Graphics& g)
{
    g.saveState();
    g.addTransform(transform);

    if (enableSyntaxHighlighting) {
        auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
        auto rows = document.getRangeOfRowsIntersecting(g.getClipBounds().toFloat());
        auto index = Point<int>(rows.getStart(), 0);
        document.navigate(index, TextDocument::Target::token, TextDocument::Direction::backwardRow);

        auto it = TextDocument::Iterator(document, index);
        auto previous = it.getIndex();
        auto zones = Array<Selection>();

        while (it.getIndex().x < rows.getEnd() && !it.isEOF()) {
            auto tokenType = CppTokeniserFunctions::readNextToken(it);
            zones.add(Selection(previous, it.getIndex()).withStyle(tokenType));
            previous = it.getIndex();
        }
        document.clearTokens(rows);
        document.applyTokens(rows, zones);

        for (int n = 0; n < colourScheme.types.size(); ++n) {
            g.setColour(colourScheme.types[n].colour);
            document.findGlyphsIntersecting(g.getClipBounds().toFloat(), n).draw(g);
        }
    } else {
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        document.findGlyphsIntersecting(g.getClipBounds().toFloat()).draw(g);
    }
    g.restoreState();
}

struct TextEditorDialog : public Component {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;

    PlugDataTextEditor editor;

    std::function<void(String, bool)> onClose;

    String title;
    int margin;

    explicit TextEditorDialog(String name)
        : resizer(this, &constrainer)
        , title(std::move(name))
        , margin(ProjectInfo::canUseSemiTransparentWindows() ? 15 : 0)
    {
        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 200);
        constrainer.setFixedAspectRatio(0.0f);

        closeButton->onClick = [this]() {
            // Call asynchronously because this function may distroy the dialog
            MessageManager::callAsync([this]() {
                onClose(editor.getText(), editor.hasChanged());
            });
        };

        addToDesktop(0);
        setVisible(true);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(600, 400));

        addAndMakeVisible(editor);
        addAndMakeVisible(resizer);
        resizer.setAlwaysOnTop(true);
        // resizer.setAllowHostManagedResize(false);

        editor.grabKeyboardFocus();
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(margin);

        resizer.setBounds(b);

        auto closeButtonBounds = b.removeFromTop(30).removeFromRight(30).translated(-5, 5);
        closeButton->setBounds(closeButtonBounds);
        editor.setBounds(b.withTrimmedTop(10).withTrimmedBottom(20));
    }

    void mouseDown(MouseEvent const& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().reduced(margin).toFloat(), ProjectInfo::canUseSemiTransparentWindows() ? Corners::windowCornerRadius : 0.0f, 1.0f);
    }

    void paint(Graphics& g) override
    {
        if (ProjectInfo::canUseSemiTransparentWindows()) {
            auto shadowPath = Path();
            shadowPath.addRoundedRectangle(getLocalBounds().reduced(20), Corners::windowCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 13.0f);
        }

        auto radius = ProjectInfo::canUseSemiTransparentWindows() ? Corners::windowCornerRadius : 0.0f;

        auto b = getLocalBounds().reduced(margin);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(b.toFloat(), radius);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), radius, 1.0f);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        // g.drawHorizontalLine(b.getX() + 39, b.getY() + 48, b.getWidth());
        g.drawHorizontalLine(b.getHeight() - 20, b.getY() + 48, b.getWidth());

        if (!title.isEmpty()) {
            Fonts::drawText(g, title, b.getX(), b.getY(), b.getWidth(), 40, findColour(PlugDataColour::toolbarTextColourId), 15, Justification::centred);
        }
    }
};
