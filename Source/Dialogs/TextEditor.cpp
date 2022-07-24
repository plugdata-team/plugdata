/** ============================================================================
 *
 * PlugDataTextEditor.cpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */

#include "TextEditor.h"

#define GUTTER_WIDTH 48.f
#define CURSOR_WIDTH 3.f
#define TEXT_INDENT 4.f
#define TEST_MULTI_CARET_EDITING true
#define TEST_SYNTAX_SUPPORT true
#define ENABLE_CARET_BLINK false
#define PROFILE_PAINTS false

static bool DEBUG_TOKENS = false;

//==============================================================================
Caret::Caret (const TextDocument& document) : document (document)
{
    setInterceptsMouseClicks (false, false);
#if ENABLE_CARET_BLINK
    startTimerHz (20);
#endif
}

void Caret::setViewTransform (const AffineTransform& transformToUse)
{
    transform = transformToUse;
    repaint();
}

void Caret::updateSelections()
{
    phase = 0.f;
    repaint();
}

void Caret::paint (Graphics& g)
{
#if PROFILE_PAINTS
    auto start = Time::getMillisecondCounterHiRes();
#endif
    
    g.setColour (getParentComponent()->findColour (CaretComponent::caretColourId).withAlpha (squareWave (phase)));
    
    for (const auto &r : getCaretRectangles())
        g.fillRect (r);
    
#if PROFILE_PAINTS
    std::cout << "[Caret::paint] " << Time::getMillisecondCounterHiRes() - start << std::endl;
#endif
}

float Caret::squareWave (float wt) const
{
    //(Time::getCurrentTime().getMillisecondCounter() % 2000) > 2000
    const float delta = 0.222f;
    const float A = 1.0;
    return 0.5f + A / 3.14159f * std::atanf (std::cosf (wt) / delta);
}

void Caret::timerCallback()
{
    phase += 3.2e-1;
    
    for (const auto &r : getCaretRectangles())
        repaint (r.getSmallestIntegerContainer());
}

Array<Rectangle<float>> Caret::getCaretRectangles() const
{
    Array<Rectangle<float>> rectangles;
    
    for (const auto& selection : document.getSelections())
    {
        rectangles.add (document
                        .getGlyphBounds (selection.head)
                        .removeFromLeft (CURSOR_WIDTH)
                        .translated (selection.head.y == 0 ? 0 : -0.5f * CURSOR_WIDTH, 0.f)
                        .transformedBy (transform)
                        .expanded (0.f, 1.f));
    }
    return rectangles;
}




//==========================================================================
GutterComponent::GutterComponent (const TextDocument& document)
: document (document)
, memoizedGlyphArrangements ([this] (int row) { return getLineNumberGlyphs (row); })
{
    setInterceptsMouseClicks (false, false);
}

void GutterComponent::setViewTransform (const AffineTransform& transformToUse)
{
    transform = transformToUse;
    repaint();
}

void GutterComponent::updateSelections()
{
    repaint();
}

void GutterComponent::paint (Graphics& g)
{
#if PROFILE_PAINTS
    auto start = Time::getMillisecondCounterHiRes();
#endif
    
    /*
     Draw the gutter background, shadow, and outline
     ------------------------------------------------------------------
     */
    auto bg = getParentComponent()->findColour (PlugDataColour::canvasColourId);
    auto ln = bg.overlaidWith (getParentComponent()->findColour (PlugDataColour::toolbarColourId));
    
    g.setColour (ln);
    g.fillRect (getLocalBounds().removeFromLeft (GUTTER_WIDTH));
    
    if (transform.getTranslationX() < GUTTER_WIDTH)
    {
        auto shadowRect = getLocalBounds().withLeft (GUTTER_WIDTH).withWidth (12);
        
        auto gradient = ColourGradient::horizontal (ln.contrasting().withAlpha (0.3f),
                                                    Colours::transparentBlack, shadowRect);
        g.setFillType (gradient);
        g.fillRect (shadowRect);
    }
    else
    {
        g.setColour (ln.darker (0.2f));
        g.drawVerticalLine (GUTTER_WIDTH - 1.f, 0.f, getHeight());
    }
    
    /*
     Draw the line numbers and selected rows
     ------------------------------------------------------------------
     */
    auto area = g.getClipBounds().toFloat().transformedBy (transform.inverted());
    auto rowData = document.findRowsIntersecting (area);
    auto verticalTransform = transform.withAbsoluteTranslation (0.f, transform.getTranslationY());
    
    g.setColour (ln.contrasting (0.1f));
    
    for (const auto& r : rowData)
    {
        if (r.isRowSelected)
        {
            auto A = r.bounds
                .transformedBy (transform)
                .withX (0)
                .withWidth (GUTTER_WIDTH);
            
            g.fillRect (A);
        }
    }
    
    g.setColour (getParentComponent()->findColour (CodeEditorComponent::lineNumberTextId));
    
    for (const auto& r : rowData)
    {
        memoizedGlyphArrangements (r.rowNumber).draw(g, verticalTransform);
    }
    
#if PROFILE_PAINTS
    std::cout << "[GutterComponent::paint] " << Time::getMillisecondCounterHiRes() - start << std::endl;
#endif
}

GlyphArrangement GutterComponent::getLineNumberGlyphs (int row) const
{
    GlyphArrangement glyphs;
    glyphs.addLineOfText (document.getFont().withHeight (12.f),
                          String (row + 1),
                          8.f, document.getVerticalPosition (row, TextDocument::Metric::baseline));
    return glyphs;
}




//==========================================================================
HighlightComponent::HighlightComponent (const TextDocument& document) : document (document)
{
    setInterceptsMouseClicks (false, false);
}

void HighlightComponent::setViewTransform (const AffineTransform& transformToUse)
{
    transform = transformToUse;
    
    outlinePath.clear();
    auto clip = getLocalBounds().toFloat().transformedBy (transform.inverted());
    
    for (const auto& s : document.getSelections())
    {
        outlinePath.addPath (getOutlinePath (document.getSelectionRegion (s, clip)));
    }
    repaint (outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::updateSelections()
{
    outlinePath.clear();
    auto clip = getLocalBounds().toFloat().transformedBy (transform.inverted());
    
    for (const auto& s : document.getSelections())
    {
        outlinePath.addPath (getOutlinePath (document.getSelectionRegion (s, clip)));
    }
    repaint (outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::paint (Graphics& g)
{
#if PROFILE_PAINTS
    auto start = Time::getMillisecondCounterHiRes();
#endif
    
    g.addTransform (transform);
    auto highlight = getParentComponent()->findColour (CodeEditorComponent::highlightColourId);
    
    g.setColour (highlight);
    g.fillPath (outlinePath);
    
    g.setColour (highlight.darker());
    g.strokePath (outlinePath, PathStrokeType (1.f));
    
#if PROFILE_PAINTS
    std::cout << "[HighlightComponent::paint] " << Time::getMillisecondCounterHiRes() - start << std::endl;
#endif
}

Path HighlightComponent::getOutlinePath (const Array<Rectangle<float>>& rectangles)
{
    auto p = Path();
    auto rect = rectangles.begin();
    
    if (rect == rectangles.end())
        return p;
    
    p.startNewSubPath (rect->getTopLeft());
    p.lineTo (rect->getBottomLeft());
    
    while (++rect != rectangles.end())
    {
        p.lineTo (rect->getTopLeft());
        p.lineTo (rect->getBottomLeft());
    }
    
    while (rect-- != rectangles.begin())
    {
        p.lineTo (rect->getBottomRight());
        p.lineTo (rect->getTopRight());
    }
    
    p.closeSubPath();
    return p.createPathWithRoundedCorners (4.f);
}




//==============================================================================
Selection::Selection (const String& content)
{
    int rowSpan = 0;
    int n = 0, lastLineStart = 0;
    auto c = content.getCharPointer();
    
    while (*c != '\0')
    {
        if (*c == '\n')
        {
            ++rowSpan;
            lastLineStart = n + 1;
        }
        ++c; ++n;
    }
    
    head = { 0, 0 };
    tail = { rowSpan, content.length() - lastLineStart };
}

bool Selection::isOriented() const
{
    return ! (head.x > tail.x || (head.x == tail.x && head.y > tail.y));
}

Selection Selection::oriented() const
{
    if (! isOriented())
        return swapped();
    
    return *this;
}

Selection Selection::swapped() const
{
    Selection s = *this;
    std::swap (s.head, s.tail);
    return s;
}

Selection Selection::horizontallyMaximized (const TextDocument& document) const
{
    Selection s = *this;
    
    if (isOriented())
    {
        s.head.y = 0;
        s.tail.y = document.getNumColumns (s.tail.x);
    }
    else
    {
        s.head.y = document.getNumColumns (s.head.x);
        s.tail.y = 0;
    }
    return s;
}

Selection Selection::measuring (const String& content) const
{
    Selection s (content);
    
    if (isOriented())
    {
        return Selection (content).startingFrom (head);
    }
    else
    {
        return Selection (content).startingFrom (tail).swapped();
    }
}

Selection Selection::startingFrom (Point<int> index) const
{
    Selection s = *this;
    
    /*
     Pull the whole selection back to the origin.
     */
    s.pullBy (Selection ({}, isOriented() ? head : tail));
    
    /*
     Then push it forward to the given index.
     */
    s.pushBy (Selection ({}, index));
    
    return s;
}

void Selection::pullBy (Selection disappearingSelection)
{
    disappearingSelection.pull (head);
    disappearingSelection.pull (tail);
}

void Selection::pushBy (Selection appearingSelection)
{
    appearingSelection.push (head);
    appearingSelection.push (tail);
}

void Selection::pull (Point<int>& index) const
{
    const auto S = oriented();
    
    /*
     If the selection tail is on index's row, then shift its column back,
     either by the difference between our head and tail column indexes if
     our head and tail are on the same row, or otherwise by our tail's
     column index.
     */
    if (S.tail.x == index.x && S.head.y <= index.y)
    {
        if (S.head.x == S.tail.x)
        {
            index.y -= S.tail.y - S.head.y;
        }
        else
        {
            index.y -= S.tail.y;
        }
    }
    
    /*
     If this selection starts on the same row or an earlier one,
     then shift the row index back by our row span.
     */
    if (S.head.x <= index.x)
    {
        index.x -= S.tail.x - S.head.x;
    }
}

void Selection::push (Point<int>& index) const
{
    const auto S = oriented();
    
    /*
     If our head is on index's row, then shift its column forward, either
     by our head to tail distance if our head and tail are on the
     same row, or otherwise by our tail's column index.
     */
    if (S.head.x == index.x && S.head.y <= index.y)
    {
        if (S.head.x == S.tail.x)
        {
            index.y += S.tail.y - S.head.y;
        }
        else
        {
            index.y += S.tail.y;
        }
    }
    
    /*
     If this selection starts on the same row or an earlier one,
     then shift the row index forward by our row span.
     */
    if (S.head.x <= index.x)
    {
        index.x += S.tail.x - S.head.x;
    }
}




//==============================================================================
const String& GlyphArrangementArray::operator[] (int index) const
{
    if (isPositiveAndBelow (index, lines.size()))
    {
        return lines.getReference (index).string;
    }
    
    static String empty;
    return empty;
}

int GlyphArrangementArray::getToken (int row, int col, int defaultIfOutOfBounds) const
{
    if (! isPositiveAndBelow (row, lines.size()))
    {
        return defaultIfOutOfBounds;
    }
    return lines.getReference (row).tokens[col];
}

void GlyphArrangementArray::clearTokens (int index)
{
    if (! isPositiveAndBelow (index, lines.size()))
        return;
    
    auto& entry = lines.getReference (index);
    
    ensureValid (index);
    
    for (int col = 0; col < entry.tokens.size(); ++col)
    {
        entry.tokens.setUnchecked (col, 0);
    }
}

void GlyphArrangementArray::applyTokens (int index, Selection zone)
{
    if (! isPositiveAndBelow (index, lines.size()))
        return;
    
    auto& entry = lines.getReference (index);
    auto range = zone.getColumnRangeOnRow (index, entry.tokens.size());
    
    ensureValid (index);
    
    for (int col = range.getStart(); col < range.getEnd(); ++col)
    {
        entry.tokens.setUnchecked (col, zone.token);
    }
}

GlyphArrangement GlyphArrangementArray::getGlyphs (int index,
                                                   float baseline,
                                                   int token,
                                                   bool withTrailingSpace) const
{
    if (! isPositiveAndBelow (index, lines.size()))
    {
        GlyphArrangement glyphs;
        
        if (withTrailingSpace)
        {
            glyphs.addLineOfText (font, " ", TEXT_INDENT, baseline);
        }
        return glyphs;
    }
    ensureValid (index);
    
    auto& entry = lines.getReference (index);
    auto glyphSource = withTrailingSpace ? entry.glyphsWithTrailingSpace : entry.glyphs;
    auto glyphs = GlyphArrangement();
    
    if (DEBUG_TOKENS)
    {
        String line;
        String hex ("0123456789abcdefg");
        
        for (auto token : entry.tokens)
            line << hex[token % 16];
        
        if (withTrailingSpace)
            line << " ";
        
        glyphSource.clear();
        glyphSource.addLineOfText (font, line, 0.f, 0.f);
    }
    
    for (int n = 0; n < glyphSource.getNumGlyphs(); ++n)
    {
        if (token == -1 || entry.tokens.getUnchecked (n) == token)
        {
            auto glyph = glyphSource.getGlyph (n);
            glyph.moveBy (TEXT_INDENT, baseline);
            glyphs.addGlyph (glyph);
        }
    }
    return glyphs;
}

void GlyphArrangementArray::ensureValid (int index) const
{
    if (! isPositiveAndBelow (index, lines.size()))
        return;
    
    auto& entry = lines.getReference (index);
    
    if (entry.glyphsAreDirty)
    {
        entry.tokens.resize (entry.string.length());
        entry.glyphs.addLineOfText (font, entry.string, 0.f, 0.f);
        entry.glyphsWithTrailingSpace.addLineOfText (font, entry.string + " ", 0.f, 0.f);
        entry.glyphsAreDirty = ! cacheGlyphArrangement;
    }
}

void GlyphArrangementArray::invalidateAll()
{
    for (auto& entry : lines)
    {
        entry.glyphsAreDirty = true;
        entry.tokensAreDirty = true;
    }
}




//==============================================================================
void TextDocument::replaceAll (const String& content)
{
    lines.clear();
    
    
    for (const auto& line : StringArray::fromLines (content))
    {
        lines.add (line);
    }
}

String TextDocument::getText() const
{
    String text;
    for (int i = 0; i < lines.size(); i++)
    {
        text += lines[i] + "\n\r";
    }
    
    return text;
}

int TextDocument::getNumRows() const
{
    return lines.size();
}

int TextDocument::getNumColumns (int row) const
{
    return lines[row].length();
}

float TextDocument::getVerticalPosition (int row, Metric metric) const
{
    float lineHeight = font.getHeight() * lineSpacing;
    float gap = font.getHeight() * (lineSpacing - 1.f) * 0.5f;
    
    switch (metric)
    {
        case Metric::top     : return lineHeight * row;
        case Metric::ascent  : return lineHeight * row + gap;
        case Metric::baseline: return lineHeight * row + gap + font.getAscent();
        case Metric::descent : return lineHeight * row + gap + font.getAscent() + font.getDescent();
        case Metric::bottom  : return lineHeight * row + lineHeight;
    }
}

Point<float> TextDocument::getPosition (Point<int> index, Metric metric) const
{
    return Point<float> (getGlyphBounds (index).getX(), getVerticalPosition (index.x, metric));
}

Array<Rectangle<float>> TextDocument::getSelectionRegion (Selection selection, Rectangle<float> clip) const
{
    Array<Rectangle<float>> patches;
    Selection s = selection.oriented();
    
    if (s.head.x == s.tail.x)
    {
        int c0 = s.head.y;
        int c1 = s.tail.y;
        patches.add (getBoundsOnRow (s.head.x, Range<int> (c0, c1)));
    }
    else
    {
        int r0 = s.head.x;
        int c0 = s.head.y;
        int r1 = s.tail.x;
        int c1 = s.tail.y;
        
        for (int n = r0; n <= r1; ++n)
        {
            if (! clip.isEmpty() &&
                ! clip.getVerticalRange().intersects (
                                                      {
                                                          getVerticalPosition (n, Metric::top),
                                                          getVerticalPosition (n, Metric::bottom)
                                                      })) continue;
            
            if      (n == r1 && c1 == 0) continue;
            else if (n == r0) patches.add (getBoundsOnRow (r0, Range<int> (c0, getNumColumns (r0) + 1)));
            else if (n == r1) patches.add (getBoundsOnRow (r1, Range<int> (0, c1)));
            else              patches.add (getBoundsOnRow (n,  Range<int> (0, getNumColumns (n) + 1)));
        }
    }
    return patches;
}

Rectangle<float> TextDocument::getBounds() const
{
    if (cachedBounds.isEmpty())
    {
        auto bounds = Rectangle<float>();
        
        for (int n = 0; n < getNumRows(); ++n)
        {
            bounds = bounds.getUnion (getBoundsOnRow (n, Range<int> (0, getNumColumns (n))));
        }
        return cachedBounds = bounds;
    }
    return cachedBounds;
}

Rectangle<float> TextDocument::getBoundsOnRow (int row, Range<int> columns) const
{
    return getGlyphsForRow (row, -1, true)
        .getBoundingBox    (columns.getStart(), columns.getLength(), true)
        .withTop           (getVerticalPosition (row, Metric::top))
        .withBottom        (getVerticalPosition (row, Metric::bottom));
}

Rectangle<float> TextDocument::getGlyphBounds (Point<int> index) const
{
    index.y = jlimit (0, getNumColumns (index.x), index.y);
    return getBoundsOnRow (index.x, Range<int> (index.y, index.y + 1));
}

GlyphArrangement TextDocument::getGlyphsForRow (int row, int token, bool withTrailingSpace) const
{
    return lines.getGlyphs (row,
                            getVerticalPosition (row, Metric::baseline),
                            token,
                            withTrailingSpace);
}

GlyphArrangement TextDocument::findGlyphsIntersecting (Rectangle<float> area, int token) const
{
    auto range = getRangeOfRowsIntersecting (area);
    auto rows = Array<RowData>();
    auto glyphs = GlyphArrangement();
    
    for (int n = range.getStart(); n < range.getEnd(); ++n)
    {
        glyphs.addGlyphArrangement (getGlyphsForRow (n, token));
    }
    return glyphs;
}

Range<int> TextDocument::getRangeOfRowsIntersecting (Rectangle<float> area) const
{
    auto lineHeight = font.getHeight() * lineSpacing;
    auto row0 = jlimit (0, jmax (getNumRows() - 1, 0), int (area.getY() / lineHeight));
    auto row1 = jlimit (0, jmax (getNumRows() - 1, 0), int (area.getBottom() / lineHeight));
    return { row0, row1 + 1 };
}

Array<TextDocument::RowData> TextDocument::findRowsIntersecting (Rectangle<float> area,
                                                                 bool computeHorizontalExtent) const
{
    auto range = getRangeOfRowsIntersecting (area);
    auto rows = Array<RowData>();
    
    for (int n = range.getStart(); n < range.getEnd(); ++n)
    {
        RowData data;
        data.rowNumber = n;
        
        if (computeHorizontalExtent) // slower
        {
            data.bounds = getBoundsOnRow (n, Range<int> (0, getNumColumns (n)));
        }
        else // faster
        {
            data.bounds.setY      (getVerticalPosition (n, Metric::top));
            data.bounds.setBottom (getVerticalPosition (n, Metric::bottom));
        }
        
        for (const auto& s : selections)
        {
            if (s.intersectsRow (n))
            {
                data.isRowSelected = true;
                break;
            }
        }
        rows.add (data);
    }
    return rows;
}

Point<int> TextDocument::findIndexNearestPosition (Point<float> position) const
{
    auto lineHeight = font.getHeight() * lineSpacing;
    auto row = jlimit (0, jmax (getNumRows() - 1, 0), int (position.y / lineHeight));
    auto col = 0;
    auto glyphs = getGlyphsForRow (row);
    
    if (position.x > 0.f)
    {
        col = glyphs.getNumGlyphs();
        
        for (int n = 0; n < glyphs.getNumGlyphs(); ++n)
        {
            if (glyphs.getBoundingBox (n, 1, true).getHorizontalRange().contains (position.x))
            {
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

bool TextDocument::next (Point<int>& index) const
{
    if (index.y < getNumColumns (index.x))
    {
        index.y += 1;
        return true;
    }
    else if (index.x < getNumRows())
    {
        index.x += 1;
        index.y = 0;
        return true;
    }
    return false;
}

bool TextDocument::prev (Point<int>& index) const
{
    if (index.y > 0)
    {
        index.y -= 1;
        return true;
    }
    else if (index.x > 0)
    {
        index.x -= 1;
        index.y = getNumColumns (index.x);
        return true;
    }
    return false;
}

bool TextDocument::nextRow (Point<int>& index) const
{
    if (index.x < getNumRows())
    {
        index.x += 1;
        index.y = jmin (index.y, getNumColumns (index.x));
        return true;
    }
    return false;
}

bool TextDocument::prevRow (Point<int>& index) const
{
    if (index.x > 0)
    {
        index.x -= 1;
        index.y = jmin (index.y, getNumColumns (index.x));
        return true;
    }
    return false;
}

void TextDocument::navigate (Point<int>& i, Target target, Direction direction) const
{
    std::function<bool(Point<int>&)> advance;
    std::function<juce_wchar(Point<int>&)> get;
    
    using CF = CharacterFunctions;
    static String punctuation = "{}<>()[],.;:";
    
    switch (direction)
    {
        case Direction::forwardRow:
            advance = [this] (Point<int>& i) { return nextRow (i); };
            get     = [this] (Point<int> i) { return getCharacter (i); };
            break;
        case Direction::backwardRow:
            advance = [this] (Point<int>& i) { return prevRow (i); };
            get     = [this] (Point<int> i) { prev (i); return getCharacter (i); };
            break;
        case Direction::forwardCol:
            advance = [this] (Point<int>& i) { return next (i); };
            get     = [this] (Point<int> i) { return getCharacter (i); };
            break;
        case Direction::backwardCol:
            advance = [this] (Point<int>& i) { return prev (i); };
            get     = [this] (Point<int> i) { prev (i); return getCharacter (i); };
            break;
    }
    
    switch (target)
    {
        case Target::whitespace : while (! CF::isWhitespace (get (i)) && advance (i)) { } break;
        case Target::punctuation: while (! punctuation.containsChar (get (i)) && advance (i)) { } break;
        case Target::character  : advance (i); break;
        case Target::subword    : jassertfalse; break; // IMPLEMENT ME
        case Target::word       : while (CF::isWhitespace (get (i)) && advance (i)) { } break;
        case Target::token:
        {
            int s = lines.getToken (i.x, i.y, -1);
            int t = s;
            
            while (s == t && advance (i))
            {
                if (getNumColumns (i.x) > 0)
                {
                    s = t;
                    t = lines.getToken (i.x, i.y, s);
                }
            }
            break;
        }
        case Target::line       : while (get (i) != '\n' && advance (i)) { } break;
        case Target::paragraph  : while (getNumColumns (i.x) > 0 && advance (i)) {} break;
        case Target::scope      : jassertfalse; break; // IMPLEMENT ME
        case Target::document   : while (advance (i)) { } break;
    }
}

void TextDocument::navigateSelections (Target target, Direction direction, Selection::Part part)
{
    for (auto& selection : selections)
    {
        switch (part)
        {
            case Selection::Part::head: navigate (selection.head, target, direction); break;
            case Selection::Part::tail: navigate (selection.tail, target, direction); break;
            case Selection::Part::both: navigate (selection.head, target, direction); selection.tail = selection.head; break;
        }
    }
}

Selection TextDocument::search (Point<int> start, const String& target) const
{
    while (start != getEnd())
    {
        auto y = lines[start.x].indexOf (start.y, target);
        
        if (y != -1)
            return Selection (start.x, y, start.x, y + target.length());
        
        start.y = 0;
        start.x += 1;
    }
    return Selection();
}

juce_wchar TextDocument::getCharacter (Point<int> index) const
{
    jassert (0 <= index.x && index.x <= lines.size());
    jassert (0 <= index.y && index.y <= lines[index.x].length());
    
    if (index == getEnd() || index.y == lines[index.x].length())
    {
        return '\n';
    }
    return lines[index.x].getCharPointer()[index.y];
}

const Selection& TextDocument::getSelection (int index) const
{
    return selections.getReference (index);
}

const Array<Selection>& TextDocument::getSelections() const
{
    return selections;
}

String TextDocument::getSelectionContent (Selection s) const
{
    s = s.oriented();
    
    if (s.isSingleLine())
    {
        return lines[s.head.x].substring (s.head.y, s.tail.y);
    }
    else
    {
        String content = lines[s.head.x].substring (s.head.y) + "\n";
        
        for (int row = s.head.x + 1; row < s.tail.x; ++row)
        {
            content += lines[row] + "\n";
        }
        content += lines[s.tail.x].substring (0, s.tail.y);
        return content;
    }
}

Transaction TextDocument::fulfill (const Transaction& transaction)
{
    cachedBounds = {}; // invalidate the bounds
    
    const auto t = transaction.accountingForSpecialCharacters (*this);
    const auto s = t.selection.oriented();
    const auto L = getSelectionContent (s.horizontallyMaximized (*this));
    const auto i = s.head.y;
    const auto j = L.lastIndexOf ("\n") + s.tail.y + 1;
    const auto M = L.substring (0, i) + t.content + L.substring (j);
    
    for (auto& existingSelection : selections)
    {
        existingSelection.pullBy (s);
        existingSelection.pushBy (Selection (t.content).startingFrom (s.head));
    }
    
    lines.removeRange (s.head.x, s.tail.x - s.head.x + 1);
    int row = s.head.x;
    
    if (M.isEmpty())
    {
        lines.insert (row++, String());
    }
    for (const auto& line : StringArray::fromLines (M))
    {
        lines.insert (row++, line);
    }
    
    using D = Transaction::Direction;
    auto inf = std::numeric_limits<float>::max();
    
    Transaction r;
    r.selection = Selection (t.content).startingFrom (s.head);
    r.content = L.substring (i, j);
    r.affectedArea = Rectangle<float> (0, 0, inf, inf);
    r.direction = t.direction == D::forward ? D::reverse : D::forward;
    
    return r;
}

void TextDocument::clearTokens (Range<int> rows)
{
    for (int n = rows.getStart(); n < rows.getEnd(); ++n)
    {
        lines.clearTokens (n);
    }
}

void TextDocument::applyTokens (Range<int> rows, const Array<Selection>& zones)
{
    for (int n = rows.getStart(); n < rows.getEnd(); ++n)
    {
        for (const auto& zone : zones)
        {
            if (zone.intersectsRow (n))
            {
                lines.applyTokens (n, zone);
            }
        }
    }
}




//==============================================================================
class Transaction::Undoable : public UndoableAction
{
public:
    Undoable (TextDocument& document, Callback callback, Transaction forward)
    : document (document)
    , callback (callback)
    , forward (forward) {}
    
    bool perform() override
    {
        callback (reverse = document.fulfill (forward));
        return true;
    }
    
    bool undo() override
    {
        callback (forward = document.fulfill (reverse));
        return true;
    }
    
    TextDocument& document;
    Callback callback;
    Transaction forward;
    Transaction reverse;
};




//==============================================================================
Transaction Transaction::accountingForSpecialCharacters (const TextDocument& document) const
{
    Transaction t = *this;
    auto& s = t.selection;
    
    if (content.getLastCharacter() == KeyPress::tabKey)
    {
        t.content = "    ";
    }
    if (content.getLastCharacter() == KeyPress::backspaceKey)
    {
        if (s.head.y == s.tail.y)
        {
            document.prev (s.head);
        }
        t.content.clear();
    }
    else if (content.getLastCharacter() == KeyPress::deleteKey)
    {
        if (s.head.y == s.tail.y)
        {
            document.next (s.head);
        }
        t.content.clear();
    }
    return t;
}

UndoableAction* Transaction::on (TextDocument& document, Callback callback)
{
    return new Undoable (document, callback, *this);
}




//==============================================================================
PlugDataTextEditor::PlugDataTextEditor()
: caret (document)
, gutter (document)
, highlight (document)
{
    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections ({ Selection() });
    
    setFont (Font (Font::getDefaultMonospacedFontName(), 16, 0));
    
    translateView (GUTTER_WIDTH, 0);
    setWantsKeyboardFocus (true);
    
    addAndMakeVisible (highlight);
    addAndMakeVisible (caret);
    addAndMakeVisible (gutter);
}

void PlugDataTextEditor::setFont (Font font)
{
    document.setFont (font);
    repaint();
}

void PlugDataTextEditor::setText (const String& text)
{
    document.replaceAll (text);
    repaint();
}

String PlugDataTextEditor::getText () const
{
    return document.getText();
}

void PlugDataTextEditor::translateView (float dx, float dy)
{
    auto W = viewScaleFactor * document.getBounds().getWidth();
    auto H = viewScaleFactor * document.getBounds().getHeight();
    
    translation.x = jlimit (jmin (GUTTER_WIDTH, -W + getWidth()), GUTTER_WIDTH, translation.x + dx);
    translation.y = jlimit (jmin (-0.f, -H + getHeight()), 0.0f, translation.y + dy);
    
    updateViewTransform();
}

void PlugDataTextEditor::scaleView (float scaleFactorMultiplier, float verticalCenter)
{
    auto newS = viewScaleFactor * scaleFactorMultiplier;
    auto fixedy = Point<float> (0, verticalCenter).transformedBy (transform.inverted()).y;
    
    translation.y = -newS * fixedy + verticalCenter;
    viewScaleFactor = newS;
    updateViewTransform();
}

void PlugDataTextEditor::updateViewTransform()
{
    transform = AffineTransform::scale (viewScaleFactor).translated (translation.x, translation.y);
    highlight.setViewTransform (transform);
    caret.setViewTransform (transform);
    gutter.setViewTransform (transform);
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
    auto t = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::top))   .transformedBy (transform);
    auto b = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::bottom)).transformedBy (transform);
    
    if (t.y < 0.f)
    {
        translateView (0.f, -t.y);
    }
    else if (b.y > getHeight())
    {
        translateView (0.f, -b.y + getHeight());
    }
}

void PlugDataTextEditor::resized()
{
    highlight.setBounds (getLocalBounds());
    caret.setBounds (getLocalBounds());
    gutter.setBounds (getLocalBounds());
    resetProfilingData();
}

void PlugDataTextEditor::paint (Graphics& g)
{
    auto start = Time::getMillisecondCounterHiRes();
    g.fillAll (findColour (PlugDataColour::canvasColourId));
    
    String renderSchemeString;
    
    switch (renderScheme)
    {
        case RenderScheme::usingAttributedStringSingle:
            renderTextUsingAttributedStringSingle (g);
            renderSchemeString = "AttributedStringSingle";
            break;
        case RenderScheme::usingAttributedString:
            renderTextUsingAttributedString (g);
            renderSchemeString = "attr. str";
            break;
        case RenderScheme::usingGlyphArrangement:
            renderTextUsingGlyphArrangement (g);
            renderSchemeString = "glyph arr.";
            break;
    }
    
    lastTimeInPaint = Time::getMillisecondCounterHiRes() - start;
    accumulatedTimeInPaint += lastTimeInPaint;
    numPaintCalls += 1;
    
    if (drawProfilingInfo)
    {
        String info;
        info += "paint mode         : " + renderSchemeString + "\n";
        info += "cache glyph bounds : " + String (document.lines.cacheGlyphArrangement ? "yes" : "no") + "\n";
        info += "core graphics      : " + String (allowCoreGraphics ? "yes" : "no") + "\n";
        info += "syntax highlight   : " + String (enableSyntaxHighlighting ? "yes" : "no") + "\n";
        info += "mean render time   : " + String (accumulatedTimeInPaint / numPaintCalls) + " ms\n";
        info += "last render time   : " + String (lastTimeInPaint) + " ms\n";
        info += "tokeniser time     : " + String (lastTokeniserTime) + " ms\n";
        
        g.setColour (findColour (CodeEditorComponent::defaultTextColourId));
        g.setFont (Font ("Courier New", 12, 0));
        g.drawMultiLineText (info, getWidth() - 280, 10, 280);
    }
    
#if PROFILE_PAINTS
    std::cout << "[PlugDataTextEditor::paint] " << lastTimeInPaint << std::endl;
#endif
}

void PlugDataTextEditor::paintOverChildren (Graphics& g)
{
}

void PlugDataTextEditor::mouseDown (const MouseEvent& e)
{
    if (e.getNumberOfClicks() > 1)
    {
        return;
    }
    else if (e.mods.isRightButtonDown())
    {
        PopupMenu menu;
        
        menu.addItem (1, "Render scheme: AttributedStringSingle", true, renderScheme == RenderScheme::usingAttributedStringSingle, nullptr);
        menu.addItem (2, "Render scheme: AttributedString", true, renderScheme == RenderScheme::usingAttributedString, nullptr);
        menu.addItem (3, "Render scheme: GlyphArrangement", true, renderScheme == RenderScheme::usingGlyphArrangement, nullptr);
        menu.addItem (4, "Cache glyph positions", true, document.lines.cacheGlyphArrangement, nullptr);
        menu.addItem (5, "Allow Core Graphics", true, allowCoreGraphics, nullptr);
        menu.addItem (6, "Syntax highlighting", true, enableSyntaxHighlighting, nullptr);
        menu.addItem (7, "Draw profiling info", true, drawProfilingInfo, nullptr);
        menu.addItem (8, "Debug tokens", true, DEBUG_TOKENS, nullptr);
        
        auto targetArea = Rectangle<int>(10, 10).withCentre(e.getPosition());
        
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(this).withTargetScreenArea(targetArea),
                           [this](int result){
            
            switch (result)
            {
                case 1: renderScheme = RenderScheme::usingAttributedStringSingle; break;
                case 2: renderScheme = RenderScheme::usingAttributedString; break;
                case 3: renderScheme = RenderScheme::usingGlyphArrangement; break;
                case 4: document.lines.cacheGlyphArrangement = ! document.lines.cacheGlyphArrangement; break;
                case 5: allowCoreGraphics = ! allowCoreGraphics; break;
                case 6: enableSyntaxHighlighting = ! enableSyntaxHighlighting; break;
                case 7: drawProfilingInfo = ! drawProfilingInfo; break;
                case 8: DEBUG_TOKENS = ! DEBUG_TOKENS; break;
            }
        });
        
        
        
        resetProfilingData();
        repaint();
        return;
    }
    
    auto selections = document.getSelections();
    auto index = document.findIndexNearestPosition (e.position.transformedBy (transform.inverted()));
    
    if (selections.contains (index))
    {
        return;
    }
    if (! e.mods.isCommandDown() || ! TEST_MULTI_CARET_EDITING)
    {
        selections.clear();
    }
    
    selections.add (index);
    document.setSelections (selections);
    updateSelections();
}

void PlugDataTextEditor::mouseDrag (const MouseEvent& e)
{
    if (e.mouseWasDraggedSinceMouseDown())
    {
        auto selection = document.getSelections().getFirst();
        selection.head = document.findIndexNearestPosition (e.position.transformedBy (transform.inverted()));
        document.setSelections ({ selection });
        translateToEnsureCaretIsVisible();
        updateSelections();
    }
}

void PlugDataTextEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (e.getNumberOfClicks() == 2)
    {
        document.navigateSelections (TextDocument::Target::whitespace, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::whitespace, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
    }
    else if (e.getNumberOfClicks() == 3)
    {
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
    }
    updateSelections();
}

void PlugDataTextEditor::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& d)
{
    float dx = d.deltaX;
    /*
     make scrolling away from the gutter just a little "sticky"
     */
    if (translation.x == GUTTER_WIDTH && -0.01f < dx && dx < 0.f)
    {
        dx = 0.f;
    }
    translateView (dx * 400, d.deltaY * 800);
}

void PlugDataTextEditor::mouseMagnify (const MouseEvent& e, float scaleFactor)
{
    scaleView (scaleFactor, e.position.y);
}

bool PlugDataTextEditor::keyPressed (const KeyPress& key)
{
    // =======================================================================================
    using Target     = TextDocument::Target;
    using Direction  = TextDocument::Direction;
    auto mods        = key.getModifiers();
    auto isTab       = tabKeyUsed && (key.getTextCharacter() == '\t');
    
    
    // =======================================================================================
    auto nav = [this, mods] (Target target, Direction direction)
    {
        if (mods.isShiftDown())
            document.navigateSelections (target, direction, Selection::Part::head);
        else
            document.navigateSelections (target, direction, Selection::Part::both);
        
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expandBack = [this, mods] (Target target, Direction direction)
    {
        document.navigateSelections (target, direction, Selection::Part::head);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expand = [this] (Target target)
    {
        document.navigateSelections (target, Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (target, Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
        return true;
    };
    auto addCaret = [this] (Target target, Direction direction)
    {
        auto s = document.getSelections().getLast();
        document.navigate (s.head, target, direction);
        document.addSelection (s);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto addSelectionAtNextMatch = [this] ()
    {
        const auto& s = document.getSelections().getLast();
        
        if (! s.isSingleLine())
        {
            return false;
        }
        auto t = document.search (s.tail, document.getSelectionContent (s));
        
        if (t.isSingular())
        {
            return false;
        }
        document.addSelection (t);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    
    
    // =======================================================================================
    if (key.isKeyCode (KeyPress::escapeKey))
    {
        document.setSelections (document.getSelections().getLast());
        updateSelections();
        return true;
    }
    if (mods.isCtrlDown() && mods.isAltDown())
    {
        if (key.isKeyCode (KeyPress::downKey)) return addCaret (Target::character, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return addCaret (Target::character, Direction::backwardRow);
    }
    if (mods.isCtrlDown())
    {
        if (key.isKeyCode (KeyPress::rightKey)) return nav (Target::whitespace, Direction::forwardCol)  && nav (Target::word, Direction::forwardCol);
        if (key.isKeyCode (KeyPress::leftKey )) return nav (Target::whitespace, Direction::backwardCol) && nav (Target::word, Direction::backwardCol);
        if (key.isKeyCode (KeyPress::downKey )) return nav (Target::word, Direction::forwardCol)  && nav (Target::paragraph, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey   )) return nav (Target::word, Direction::backwardCol) && nav (Target::paragraph, Direction::backwardRow);
        
        if (key.isKeyCode (KeyPress::backspaceKey)) return (   expandBack (Target::whitespace, Direction::backwardCol)
                                                            && expandBack (Target::word, Direction::backwardCol)
                                                            && insert (""));
        
        if (key == KeyPress ('e', ModifierKeys::ctrlModifier, 0) ||
            key == KeyPress ('e', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav (Target::line, Direction::forwardCol);
        
        if (key == KeyPress ('a', ModifierKeys::ctrlModifier, 0) ||
            key == KeyPress ('a', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav (Target::line, Direction::backwardCol);
    }
    if (mods.isCommandDown())
    {
        if (key.isKeyCode (KeyPress::downKey)) return nav (Target::document, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return nav (Target::document, Direction::backwardRow);
    }
    
    if (key.isKeyCode (KeyPress::rightKey)) return nav (Target::character, Direction::forwardCol);
    if (key.isKeyCode (KeyPress::leftKey )) return nav (Target::character, Direction::backwardCol);
    if (key.isKeyCode (KeyPress::downKey )) return nav (Target::character, Direction::forwardRow);
    if (key.isKeyCode (KeyPress::upKey   )) return nav (Target::character, Direction::backwardRow);
    
    if (key == KeyPress ('a', ModifierKeys::commandModifier, 0)) return expand (Target::document);
    if (key == KeyPress ('d', ModifierKeys::commandModifier, 0)) return expand (Target::whitespace);
    if (key == KeyPress ('e', ModifierKeys::commandModifier, 0)) return expand (Target::token);
    if (key == KeyPress ('l', ModifierKeys::commandModifier, 0)) return expand (Target::line);
    if (key == KeyPress ('f', ModifierKeys::commandModifier, 0)) return addSelectionAtNextMatch();
    if (key == KeyPress ('z', ModifierKeys::commandModifier, 0)) return undo.undo();
    if (key == KeyPress ('r', ModifierKeys::commandModifier, 0)) return undo.redo();
    
    if (key == KeyPress ('x', ModifierKeys::commandModifier, 0))
    {
        SystemClipboard::copyTextToClipboard (document.getSelectionContent (document.getSelections().getFirst()));
        return insert ("");
    }
    if (key == KeyPress ('c', ModifierKeys::commandModifier, 0))
    {
        SystemClipboard::copyTextToClipboard (document.getSelectionContent (document.getSelections().getFirst()));
        return true;
    }
    
    if (key == KeyPress ('v', ModifierKeys::commandModifier, 0))   return insert (SystemClipboard::getTextFromClipboard());
    if (key == KeyPress ('d', ModifierKeys::ctrlModifier, 0))      return insert (String::charToString (KeyPress::deleteKey));
    if (key.isKeyCode (KeyPress::returnKey))                       return insert ("\n");
    if (key.getTextCharacter() >= ' ' || isTab)                    return insert (String::charToString (key.getTextCharacter()));
    
    return false;
}

bool PlugDataTextEditor::insert (const String& content)
{
    double now = Time::getApproximateMillisecondCounter();
    
    if (now > lastTransactionTime + 400)
    {
        lastTransactionTime = Time::getApproximateMillisecondCounter();
        undo.beginNewTransaction();
    }
    
    for (int n = 0; n < document.getNumSelections(); ++n)
    {
        Transaction t;
        t.content = content;
        t.selection = document.getSelection (n);
        
        auto callback = [this, n] (const Transaction& r)
        {
            switch (r.direction) // NB: switching on the direction of the reciprocal here
            {
                case Transaction::Direction::forward: document.setSelection (n, r.selection); break;
                case Transaction::Direction::reverse: document.setSelection (n, r.selection.tail); break;
            }
            
            if (! r.affectedArea.isEmpty())
            {
                repaint (r.affectedArea.transformedBy (transform).getSmallestIntegerContainer());
            }
        };
        undo.perform (t.on (document, callback));
    }
    updateSelections();
    return true;
}

MouseCursor PlugDataTextEditor::getMouseCursor()
{
    return getMouseXYRelative().x < GUTTER_WIDTH ? MouseCursor::NormalCursor : MouseCursor::IBeamCursor;
}




//==============================================================================
void PlugDataTextEditor::renderTextUsingAttributedStringSingle (Graphics& g)
{
    g.saveState();
    g.addTransform (transform);
    
    auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
    auto font = document.getFont();
    auto rows = document.getRangeOfRowsIntersecting (g.getClipBounds().toFloat());
    auto T = document.getVerticalPosition (rows.getStart(), TextDocument::Metric::ascent);
    auto B = document.getVerticalPosition (rows.getEnd(),   TextDocument::Metric::top);
    auto W = 10000;
    auto bounds = Rectangle<float>::leftTopRightBottom (TEXT_INDENT, T, W, B);
    auto content = document.getSelectionContent (Selection (rows.getStart(), 0, rows.getEnd(), 0));
    
    AttributedString s;
    s.setLineSpacing ((document.getLineSpacing() - 1.f) * font.getHeight());
    
    CppTokeniserFunctions::StringIterator si (content);
    auto previous = si.t;
    auto start = Time::getMillisecondCounterHiRes();
    
    while (! si.isEOF())
    {
        auto tokenType = CppTokeniserFunctions::readNextToken (si);
        auto colour = findColour(PlugDataColour::textColourId);
        auto token = String (previous, si.t);
        
        previous = si.t;
        
        if (enableSyntaxHighlighting)
        {
            s.append (token, font, colour);
        }
        else
        {
            s.append (token, font);
        }
    }
    
    lastTokeniserTime = Time::getMillisecondCounterHiRes() - start;
    
    if (allowCoreGraphics)
    {
        s.draw (g, bounds);
    }
    else
    {
        TextLayout layout;
        layout.createLayout (s, bounds.getWidth());
        layout.draw (g, bounds);
    }
    g.restoreState();
}

void PlugDataTextEditor::renderTextUsingAttributedString (Graphics& g)
{
    /*
     Credit to chrisboy2000 for this
     */
    auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
    auto originalHeight = document.getFont().getHeight();
    auto font = document.getFont().withHeight (originalHeight * transform.getScaleFactor());
    auto rows = document.findRowsIntersecting (g.getClipBounds().toFloat().transformedBy (transform.inverted()));
    
    lastTokeniserTime = 0.f;
    
    for (const auto& r: rows)
    {
        auto line = document.getLine (r.rowNumber);
        auto T = document.getVerticalPosition (r.rowNumber, TextDocument::Metric::ascent);
        auto B = document.getVerticalPosition (r.rowNumber, TextDocument::Metric::bottom);
        auto bounds = Rectangle<float>::leftTopRightBottom (0.f, T, 1000.f, B).transformedBy (transform);
        
        AttributedString s;
        
        if (! enableSyntaxHighlighting)
        {
            s.append (line, font);
        }
        else
        {
            auto start = Time::getMillisecondCounterHiRes();
            
            CppTokeniserFunctions::StringIterator si (line);
            auto previous = si.t;
            
            while (! si.isEOF())
            {
                auto tokenType = CppTokeniserFunctions::readNextToken (si);
                auto colour = colourScheme.types[tokenType].colour;
                auto token = String (previous, si.t);
                
                previous = si.t;
                s.append (token, font, colour);
            }
            
            lastTokeniserTime += Time::getMillisecondCounterHiRes() - start;
        }
        if (allowCoreGraphics)
        {
            s.draw (g, bounds);
        }
        else
        {
            TextLayout layout;
            layout.createLayout (s, bounds.getWidth());
            layout.draw (g, bounds);
        }
    }
}

void PlugDataTextEditor::renderTextUsingGlyphArrangement (Graphics& g)
{
    g.saveState();
    g.addTransform (transform);
    
    if (enableSyntaxHighlighting)
    {
        auto colourScheme = CPlusPlusCodeTokeniser().getDefaultColourScheme();
        auto rows = document.getRangeOfRowsIntersecting (g.getClipBounds().toFloat());
        auto index = Point<int> (rows.getStart(), 0);
        document.navigate (index, TextDocument::Target::token, TextDocument::Direction::backwardRow);
        
        auto it = TextDocument::Iterator (document, index);
        auto previous = it.getIndex();
        auto zones = Array<Selection>();
        auto start = Time::getMillisecondCounterHiRes();
        
        while (it.getIndex().x < rows.getEnd() && ! it.isEOF())
        {
            auto tokenType = CppTokeniserFunctions::readNextToken (it);
            zones.add (Selection (previous, it.getIndex()).withStyle (tokenType));
            previous = it.getIndex();
        }
        document.clearTokens (rows);
        document.applyTokens (rows, zones);
        
        lastTokeniserTime = Time::getMillisecondCounterHiRes() - start;
        
        for (int n = 0; n < colourScheme.types.size(); ++n)
        {
            g.setColour (colourScheme.types[n].colour);
            document.findGlyphsIntersecting (g.getClipBounds().toFloat(), n).draw (g);
        }
    }
    else
    {
        lastTokeniserTime = 0.f;
        document.findGlyphsIntersecting (g.getClipBounds().toFloat()).draw (g);
    }
    g.restoreState();
}

void PlugDataTextEditor::resetProfilingData()
{
    accumulatedTimeInPaint = 0.f;
    numPaintCalls = 0;
}

