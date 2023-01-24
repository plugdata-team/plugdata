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

#include "UnicodeTextEditor.h"

// a word or space that can't be broken down any further
struct TextAtom
{
    //==============================================================================
    String atomText;
    float width;
    int numChars;

    //==============================================================================
    bool isWhitespace() const noexcept       { return CharacterFunctions::isWhitespace (atomText[0]); }
    bool isNewLine() const noexcept          { return atomText[0] == '\r' || atomText[0] == '\n'; }

    String getText (juce_wchar passwordCharacter) const
    {
        if (passwordCharacter == 0)
            return atomText;

        return String::repeatedString (String::charToString (passwordCharacter),
                                       atomText.length());
    }

    String getTrimmedText (const juce_wchar passwordCharacter) const
    {
        if (passwordCharacter == 0)
            return atomText.substring (0, numChars);

        if (isNewLine())
            return {};

        return String::repeatedString (String::charToString (passwordCharacter), numChars);
    }

    JUCE_LEAK_DETECTOR (TextAtom)
};

//==============================================================================
// a run of text with a single font and colour
class UnicodeTextEditor::UniformTextSection
{
public:
    UniformTextSection (const String& text, const Font& f, Colour col, juce_wchar passwordCharToUse)
        : font (f), colour (col), passwordChar (passwordCharToUse)
    {
        initialiseAtoms (text);
    }

    UniformTextSection (const UniformTextSection&) = default;
    UniformTextSection (UniformTextSection&&) = default;

    UniformTextSection& operator= (const UniformTextSection&) = delete;

    void append (UniformTextSection& other)
    {
        if (! other.atoms.isEmpty())
        {
            int i = 0;

            if (! atoms.isEmpty())
            {
                auto& lastAtom = atoms.getReference (atoms.size() - 1);

                if (! CharacterFunctions::isWhitespace (lastAtom.atomText.getLastCharacter()))
                {
                    auto& first = other.atoms.getReference(0);

                    if (! CharacterFunctions::isWhitespace (first.atomText[0]))
                    {
                        lastAtom.atomText += first.atomText;
                        lastAtom.numChars = (uint16) (lastAtom.numChars + first.numChars);
                        lastAtom.width = font.getStringWidthFloat (lastAtom.getText (passwordChar));
                        ++i;
                    }
                }
            }

            atoms.ensureStorageAllocated (atoms.size() + other.atoms.size() - i);

            while (i < other.atoms.size())
            {
                atoms.add (other.atoms.getReference(i));
                ++i;
            }
        }
    }

    UniformTextSection* split (int indexToBreakAt)
    {
        auto* section2 = new UniformTextSection ({}, font, colour, passwordChar);
        int index = 0;

        for (int i = 0; i < atoms.size(); ++i)
        {
            auto& atom = atoms.getReference(i);
            auto nextIndex = index + atom.numChars;

            if (index == indexToBreakAt)
            {
                for (int j = i; j < atoms.size(); ++j)
                    section2->atoms.add (atoms.getUnchecked (j));

                atoms.removeRange (i, atoms.size());
                break;
            }

            if (indexToBreakAt >= index && indexToBreakAt < nextIndex)
            {
                TextAtom secondAtom;
                secondAtom.atomText = atom.atomText.substring (indexToBreakAt - index);
                secondAtom.width = font.getStringWidthFloat (secondAtom.getText (passwordChar));
                secondAtom.numChars = (uint16) secondAtom.atomText.length();

                section2->atoms.add (secondAtom);

                atom.atomText = atom.atomText.substring (0, indexToBreakAt - index);
                atom.width = font.getStringWidthFloat (atom.getText (passwordChar));
                atom.numChars = (uint16) (indexToBreakAt - index);

                for (int j = i + 1; j < atoms.size(); ++j)
                    section2->atoms.add (atoms.getUnchecked (j));

                atoms.removeRange (i + 1, atoms.size());
                break;
            }

            index = nextIndex;
        }

        return section2;
    }

    void appendAllText (MemoryOutputStream& mo) const
    {
        for (auto& atom : atoms)
            mo << atom.atomText;
    }

    void appendSubstring (MemoryOutputStream& mo, Range<int> range) const
    {
        int index = 0;

        for (auto& atom : atoms)
        {
            auto nextIndex = index + atom.numChars;

            if (range.getStart() < nextIndex)
            {
                if (range.getEnd() <= index)
                    break;

                auto r = (range - index).getIntersectionWith ({ 0, (int) atom.numChars });

                if (! r.isEmpty())
                    mo << atom.atomText.substring (r.getStart(), r.getEnd());
            }

            index = nextIndex;
        }
    }

    int getTotalLength() const noexcept
    {
        int total = 0;

        for (auto& atom : atoms)
            total += atom.numChars;

        return total;
    }

    void setFont (const Font& newFont, const juce_wchar passwordCharToUse)
    {
        if (font != newFont || passwordChar != passwordCharToUse)
        {
            font = newFont;
            passwordChar = passwordCharToUse;

            for (auto& atom : atoms)
                atom.width = newFont.getStringWidthFloat (atom.getText (passwordChar));
        }
    }

    //==============================================================================
    Font font;
    Colour colour;
    Array<TextAtom> atoms;
    juce_wchar passwordChar;

private:
    void initialiseAtoms (const String& textToParse)
    {
        auto text = textToParse.getCharPointer();

        while (! text.isEmpty())
        {
            size_t numChars = 0;
            auto start = text;

            // create a whitespace atom unless it starts with non-ws
            if (text.isWhitespace() && *text != '\r' && *text != '\n')
            {
                do
                {
                    ++text;
                    ++numChars;
                }
                while (text.isWhitespace() && *text != '\r' && *text != '\n');
            }
            else
            {
                if (*text == '\r')
                {
                    ++text;
                    ++numChars;

                    if (*text == '\n')
                    {
                        ++start;
                        ++text;
                    }
                }
                else if (*text == '\n')
                {
                    ++text;
                    ++numChars;
                }
                else
                {
                    while (! (text.isEmpty() || text.isWhitespace()))
                    {
                        ++text;
                        ++numChars;
                    }
                }
            }

            TextAtom atom;
            atom.atomText = String (start, numChars);
            atom.width = (atom.isNewLine() ? 0.0f : font.getStringWidthFloat (atom.getText (passwordChar)));
            atom.numChars = (uint16) numChars;
            atoms.add (atom);
        }
    }

    JUCE_LEAK_DETECTOR (UniformTextSection)
};

//==============================================================================
struct UnicodeTextEditor::Iterator
{
    Iterator (const UnicodeTextEditor& ed)
      : sections (ed.sections),
        justification (ed.justification),
        bottomRight ((float) ed.getMaximumTextWidth(), (float) ed.getMaximumTextHeight()),
        wordWrapWidth ((float) ed.getWordWrapWidth()),
        passwordCharacter (ed.passwordCharacter),
        lineSpacing (ed.lineSpacing),
        underlineWhitespace (ed.underlineWhitespace)
    {
        jassert (wordWrapWidth > 0);

        if (! sections.isEmpty())
        {
            currentSection = sections.getUnchecked (sectionIndex);

            if (currentSection != nullptr)
                beginNewLine();
        }

        lineHeight = ed.currentFont.getHeight();
    }

    Iterator (const Iterator&) = default;
    Iterator& operator= (const Iterator&) = delete;

    //==============================================================================
    bool next()
    {
        if (atom == &longAtom && chunkLongAtom (true))
            return true;

        if (sectionIndex >= sections.size())
        {
            moveToEndOfLastAtom();
            return false;
        }

        bool forceNewLine = false;

        if (atomIndex >= currentSection->atoms.size() - 1)
        {
            if (atomIndex >= currentSection->atoms.size())
            {
                if (++sectionIndex >= sections.size())
                {
                    moveToEndOfLastAtom();
                    return false;
                }

                atomIndex = 0;
                currentSection = sections.getUnchecked (sectionIndex);
            }
            else
            {
                auto& lastAtom = currentSection->atoms.getReference (atomIndex);

                if (! lastAtom.isWhitespace())
                {
                    // handle the case where the last atom in a section is actually part of the same
                    // word as the first atom of the next section...
                    float right = atomRight + lastAtom.width;
                    float lineHeight2 = lineHeight;
                    float maxDescent2 = maxDescent;

                    for (int section = sectionIndex + 1; section < sections.size(); ++section)
                    {
                        auto* s = sections.getUnchecked (section);

                        if (s->atoms.size() == 0)
                            break;

                        auto& nextAtom = s->atoms.getReference (0);

                        if (nextAtom.isWhitespace())
                            break;

                        right += nextAtom.width;

                        lineHeight2 = jmax (lineHeight2, s->font.getHeight());
                        maxDescent2 = jmax (maxDescent2, s->font.getDescent());

                        if (shouldWrap (right))
                        {
                            lineHeight = lineHeight2;
                            maxDescent = maxDescent2;

                            forceNewLine = true;
                            break;
                        }

                        if (s->atoms.size() > 1)
                            break;
                    }
                }
            }
        }

        bool isInPreviousAtom = false;

        if (atom != nullptr)
        {
            atomX = atomRight;
            indexInText += atom->numChars;

            if (atom->isNewLine())
                beginNewLine();
            else
                isInPreviousAtom = true;
        }

        atom = &(currentSection->atoms.getReference (atomIndex));
        atomRight = atomX + atom->width;
        ++atomIndex;

        if (shouldWrap (atomRight) || forceNewLine)
        {
            if (atom->isWhitespace())
            {
                // leave whitespace at the end of a line, but truncate it to avoid scrolling
                atomRight = jmin (atomRight, wordWrapWidth);
            }
            else if (shouldWrap (atom->width))  // atom too big to fit on a line, so break it up..
            {
                longAtom = *atom;
                longAtom.numChars = 0;
                atom = &longAtom;
                chunkLongAtom (isInPreviousAtom);
            }
            else
            {
                beginNewLine();
                atomRight = atomX + atom->width;
            }
        }

        return true;
    }

    void beginNewLine()
    {
        lineY += lineHeight * lineSpacing;
        float lineWidth = 0;

        auto tempSectionIndex = sectionIndex;
        auto tempAtomIndex = atomIndex;
        auto* section = sections.getUnchecked (tempSectionIndex);

        lineHeight = section->font.getHeight();
        maxDescent = section->font.getDescent();

        float nextLineWidth = (atom != nullptr) ? atom->width : 0.0f;

        while (! shouldWrap (nextLineWidth))
        {
            lineWidth = nextLineWidth;

            if (tempSectionIndex >= sections.size())
                break;

            bool checkSize = false;

            if (tempAtomIndex >= section->atoms.size())
            {
                if (++tempSectionIndex >= sections.size())
                    break;

                tempAtomIndex = 0;
                section = sections.getUnchecked (tempSectionIndex);
                checkSize = true;
            }

            if (! isPositiveAndBelow (tempAtomIndex, section->atoms.size()))
                break;

            auto& nextAtom = section->atoms.getReference (tempAtomIndex);
            nextLineWidth += nextAtom.width;

            if (shouldWrap (nextLineWidth) || nextAtom.isNewLine())
                break;

            if (checkSize)
            {
                lineHeight = jmax (lineHeight, section->font.getHeight());
                maxDescent = jmax (maxDescent, section->font.getDescent());
            }

            ++tempAtomIndex;
        }

        atomX = getJustificationOffsetX (lineWidth);
    }

    float getJustificationOffsetX (float lineWidth) const
    {
        if (justification.testFlags (Justification::horizontallyCentred))    return jmax (0.0f, (bottomRight.x - lineWidth) * 0.5f);
        if (justification.testFlags (Justification::right))                  return jmax (0.0f, bottomRight.x - lineWidth);

        return 0;
    }

    //==============================================================================
    void draw (Graphics& g, const UniformTextSection*& lastSection, AffineTransform transform) const
    {
        if (atom == nullptr)
            return;

        if (passwordCharacter != 0 || (underlineWhitespace || ! atom->isWhitespace()))
        {
            jassert (atom->getTrimmedText (passwordCharacter).isNotEmpty());

            AttributedString attributedString;
            attributedString.append(atom->getTrimmedText(passwordCharacter));

            attributedString.setJustification(justification);
            attributedString.setColour(currentSection->colour);
            attributedString.setFont(currentSection->font);

            g.saveState();
            g.addTransform(transform);
            attributedString.draw(g, {atomX, lineY, atom->width, lineHeight});
            g.restoreState();
        }
    }

    void drawUnderline (Graphics& g, Range<int> underline, Colour colour, AffineTransform transform) const
    {
        auto startX    = roundToInt (indexToX (underline.getStart()));
        auto endX      = roundToInt (indexToX (underline.getEnd()));
        auto baselineY = roundToInt (lineY + currentSection->font.getAscent() + 0.5f);

        Graphics::ScopedSaveState state (g);
        g.addTransform (transform);
        g.reduceClipRegion ({ startX, baselineY, endX - startX, 1 });
        g.fillCheckerBoard ({ (float) endX, (float) baselineY + 1.0f }, 3.0f, 1.0f, colour, Colours::transparentBlack);
    }

    void drawSelectedText (Graphics& g, Range<int> selected, Colour selectedTextColour, AffineTransform transform) const
    {
        if (atom == nullptr)
            return;

        if (passwordCharacter != 0 || ! atom->isWhitespace())
        {
            AttributedString attributedString;

            attributedString.setJustification(justification);
            attributedString.append(atom->getTrimmedText(passwordCharacter));
            attributedString.setFont(currentSection->font);
            attributedString.setColour(currentSection->colour);

            if(!selected.isEmpty()) {
                attributedString.setColour(selected, selectedTextColour);
            }

            g.saveState();
            g.addTransform(transform);
            attributedString.draw(g, {atomX, lineY, atom->width, lineHeight});
            g.restoreState();
        }
    }

    //==============================================================================
    float indexToX (int indexToFind) const
    {
        if (indexToFind <= indexInText || atom == nullptr)
            return atomX;

        if (indexToFind >= indexInText + atom->numChars)
            return atomRight;

        GlyphArrangement g;
        g.addLineOfText (currentSection->font,
                         atom->getText (passwordCharacter),
                         atomX, 0.0f);

        if (indexToFind - indexInText >= g.getNumGlyphs())
            return atomRight;

        return jmin (atomRight, g.getGlyph (indexToFind - indexInText).getLeft());
    }

    int xToIndex (float xToFind) const
    {
        if (xToFind <= atomX || atom == nullptr || atom->isNewLine())
            return indexInText;

        if (xToFind >= atomRight)
            return indexInText + atom->numChars;

        GlyphArrangement g;
        g.addLineOfText (currentSection->font,
                         atom->getText (passwordCharacter),
                         atomX, 0.0f);

        auto numGlyphs = g.getNumGlyphs();

        int j;
        for (j = 0; j < numGlyphs; ++j)
        {
            auto& pg = g.getGlyph(j);

            if ((pg.getLeft() + pg.getRight()) / 2 > xToFind)
                break;
        }

        return indexInText + j;
    }

    //==============================================================================
    bool getCharPosition (int index, Point<float>& anchor, float& lineHeightFound)
    {
        while (next())
        {
            if (indexInText + atom->numChars > index)
            {
                anchor = { indexToX (index), lineY };
                lineHeightFound = lineHeight;
                return true;
            }
        }

        anchor = { atomX, lineY };
        lineHeightFound = lineHeight;
        return false;
    }

    float getYOffset()
    {
        if (justification.testFlags (Justification::top) || lineY >= bottomRight.y)
            return 0;

        while (next())
        {
            if (lineY >= bottomRight.y)
                return 0;
        }

        auto bottom = jmax (0.0f, bottomRight.y - lineY - lineHeight);

        if (justification.testFlags (Justification::bottom))
            return bottom;

        return bottom * 0.5f;
    }

    int getTotalTextHeight()
    {
        while (next()) {}

        auto height = lineY + lineHeight + getYOffset();

        if (atom != nullptr && atom->isNewLine())
            height += lineHeight;

        return roundToInt (height);
    }

    int getTextRight()
    {
        float maxWidth = 0.0f;

        while (next())
            maxWidth = jmax (maxWidth, atomRight);

        return roundToInt (maxWidth);
    }

    Rectangle<int> getTextBounds (Range<int> range) const
    {
        auto startX = indexToX (range.getStart());
        auto endX   = indexToX (range.getEnd());

        return Rectangle<float> (startX, lineY, endX - startX, lineHeight * lineSpacing).getSmallestIntegerContainer();
    }

    //==============================================================================
    int indexInText = 0;
    float lineY = 0, lineHeight = 0, maxDescent = 0;
    float atomX = 0, atomRight = 0;
    const TextAtom* atom = nullptr;

private:
    const OwnedArray<UniformTextSection>& sections;
    const UniformTextSection* currentSection = nullptr;
    int sectionIndex = 0, atomIndex = 0;
    Justification justification;
    const Point<float> bottomRight;
    const float wordWrapWidth;
    const juce_wchar passwordCharacter;
    const float lineSpacing;
    const bool underlineWhitespace;
    TextAtom longAtom;

    bool chunkLongAtom (bool shouldStartNewLine)
    {
        const auto numRemaining = longAtom.atomText.length() - longAtom.numChars;

        if (numRemaining <= 0)
            return false;

        longAtom.atomText = longAtom.atomText.substring (longAtom.numChars);
        indexInText += longAtom.numChars;

        GlyphArrangement g;
        g.addLineOfText (currentSection->font, atom->getText (passwordCharacter), 0.0f, 0.0f);

        int split;
        for (split = 0; split < g.getNumGlyphs(); ++split)
            if (shouldWrap (g.getGlyph (split).getRight()))
                break;

        const auto numChars = jmax (1, split);
        longAtom.numChars = (uint16) numChars;
        longAtom.width = g.getGlyph (numChars - 1).getRight();

        atomX = getJustificationOffsetX (longAtom.width);

        if (shouldStartNewLine)
        {
            if (split == numRemaining)
                beginNewLine();
            else
                lineY += lineHeight * lineSpacing;
        }

        atomRight = atomX + longAtom.width;
        return true;
    }

    void moveToEndOfLastAtom()
    {
        if (atom != nullptr)
        {
            atomX = atomRight;

            if (atom->isNewLine())
            {
                atomX = getJustificationOffsetX (0);
                lineY += lineHeight * lineSpacing;
            }
        }
    }

    bool shouldWrap (const float x) const noexcept
    {
        return (x - 0.0001f) >= wordWrapWidth;
    }

    JUCE_LEAK_DETECTOR (Iterator)
};


//==============================================================================
struct UnicodeTextEditor::InsertAction  : public UndoableAction
{
    InsertAction (UnicodeTextEditor& ed, const String& newText, int insertPos,
                  const Font& newFont, Colour newColour, int oldCaret, int newCaret)
        : owner (ed),
          text (newText),
          insertIndex (insertPos),
          oldCaretPos (oldCaret),
          newCaretPos (newCaret),
          font (newFont),
          colour (newColour)
    {
    }

    bool perform() override
    {
        owner.insert (text, insertIndex, font, colour, nullptr, newCaretPos);
        return true;
    }

    bool undo() override
    {
        owner.remove ({ insertIndex, insertIndex + text.length() }, nullptr, oldCaretPos);
        return true;
    }

    int getSizeInUnits() override
    {
        return text.length() + 16;
    }

private:
    UnicodeTextEditor& owner;
    const String text;
    const int insertIndex, oldCaretPos, newCaretPos;
    const Font font;
    const Colour colour;

    JUCE_DECLARE_NON_COPYABLE (InsertAction)
};

//==============================================================================
struct UnicodeTextEditor::RemoveAction  : public UndoableAction
{
    RemoveAction (UnicodeTextEditor& ed, Range<int> rangeToRemove, int oldCaret, int newCaret,
                  const Array<UniformTextSection*>& oldSections)
        : owner (ed),
          range (rangeToRemove),
          oldCaretPos (oldCaret),
          newCaretPos (newCaret)
    {
        removedSections.addArray (oldSections);
    }

    bool perform() override
    {
        owner.remove (range, nullptr, newCaretPos);
        return true;
    }

    bool undo() override
    {
        owner.reinsert (range.getStart(), removedSections);
        owner.moveCaretTo (oldCaretPos, false);
        return true;
    }

    int getSizeInUnits() override
    {
        int n = 16;

        for (auto* s : removedSections)
            n += s->getTotalLength();

        return n;
    }

private:
    UnicodeTextEditor& owner;
    const Range<int> range;
    const int oldCaretPos, newCaretPos;
    OwnedArray<UniformTextSection> removedSections;

    JUCE_DECLARE_NON_COPYABLE (RemoveAction)
};

//==============================================================================
struct UnicodeTextEditor::TextHolderComponent  : public Component,
public Timer,
                                          public Value::Listener
{
    TextHolderComponent (UnicodeTextEditor& ed)  : owner (ed)
    {
        setWantsKeyboardFocus (false);
        setInterceptsMouseClicks (false, true);
        setMouseCursor (MouseCursor::ParentCursor);

        owner.getTextValue().addListener (this);
    }

    ~TextHolderComponent() override
    {
        owner.getTextValue().removeListener (this);
    }

    void paint (Graphics& g) override
    {
        owner.drawContent (g);
    }

    void restartTimer()
    {
        startTimer (350);
    }

    void timerCallback() override
    {
        owner.timerCallbackInt();
    }

    void valueChanged (Value&) override
    {
        owner.textWasChangedByValue();
    }

    UnicodeTextEditor& owner;

private:
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler (*this);
    }

    JUCE_DECLARE_NON_COPYABLE (TextHolderComponent)
};

//==============================================================================
struct UnicodeTextEditor::TextEditorViewport  : public Viewport
{
    TextEditorViewport (UnicodeTextEditor& ed) : owner (ed) {}

    void visibleAreaChanged (const Rectangle<int>&) override
    {
        if (! reentrant) // it's rare, but possible to get into a feedback loop as the viewport's scrollbars
                         // appear and disappear, causing the wrap width to change.
        {
            auto wordWrapWidth = owner.getWordWrapWidth();

            if (wordWrapWidth != lastWordWrapWidth)
            {
                lastWordWrapWidth = wordWrapWidth;

                ScopedValueSetter<bool> svs (reentrant, true);
                owner.checkLayout();
            }
        }
    }

private:
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler (*this);
    }

    UnicodeTextEditor& owner;
    int lastWordWrapWidth = 0;
    bool reentrant = false;

    JUCE_DECLARE_NON_COPYABLE (TextEditorViewport)
};

//==============================================================================
namespace TextEditorDefs
{
    const int textChangeMessageId = 0x10003001;
    const int returnKeyMessageId  = 0x10003002;
    const int escapeKeyMessageId  = 0x10003003;
    const int focusLossMessageId  = 0x10003004;

    const int maxActionsPerTransaction = 100;

    static int getCharacterCategory (juce_wchar character) noexcept
    {
        return CharacterFunctions::isLetterOrDigit (character)
                    ? 2 : (CharacterFunctions::isWhitespace (character) ? 0 : 1);
    }
}

//==============================================================================
UnicodeTextEditor::UnicodeTextEditor (const String& name, juce_wchar passwordChar)
    : Component (name),
      passwordCharacter (passwordChar)
{
    setMouseCursor (MouseCursor::IBeamCursor);

    viewport.reset (new TextEditorViewport (*this));
    addAndMakeVisible (viewport.get());
    viewport->setViewedComponent (textHolder = new TextHolderComponent (*this));
    viewport->setWantsKeyboardFocus (false);
    viewport->setScrollBarsShown (false, false);

    setWantsKeyboardFocus (true);
    recreateCaret();

    Desktop::getInstance().addGlobalMouseListener (this);
}

UnicodeTextEditor::~UnicodeTextEditor()
{
    Desktop::getInstance().removeGlobalMouseListener (this);

    textValue.removeListener (textHolder);
    textValue.referTo (Value());

    viewport.reset();
    textHolder = nullptr;
}

//==============================================================================
void UnicodeTextEditor::newTransaction()
{
    lastTransactionTime = Time::getApproximateMillisecondCounter();
    undoManager.beginNewTransaction();
}

bool UnicodeTextEditor::undoOrRedo (const bool shouldUndo)
{
    if (! isReadOnly())
    {
        newTransaction();

        if (shouldUndo ? undoManager.undo()
                       : undoManager.redo())
        {
            repaint();
            textChanged();
            scrollToMakeSureCursorIsVisible();

            return true;
        }
    }

    return false;
}

bool UnicodeTextEditor::undo()     { return undoOrRedo (true); }
bool UnicodeTextEditor::redo()     { return undoOrRedo (false); }

//==============================================================================
void UnicodeTextEditor::setMultiLine (const bool shouldBeMultiLine,
                               const bool shouldWordWrap)
{
    if (multiline != shouldBeMultiLine
         || wordWrap != (shouldWordWrap && shouldBeMultiLine))
    {
        multiline = shouldBeMultiLine;
        wordWrap = shouldWordWrap && shouldBeMultiLine;

        checkLayout();

        viewport->setViewPosition (0, 0);
        resized();
        scrollToMakeSureCursorIsVisible();
    }
}

bool UnicodeTextEditor::isMultiLine() const
{
    return multiline;
}

void UnicodeTextEditor::setScrollbarsShown (bool shown)
{
    if (scrollbarVisible != shown)
    {
        scrollbarVisible = shown;
        checkLayout();
    }
}

void UnicodeTextEditor::setReadOnly (bool shouldBeReadOnly)
{
    if (readOnly != shouldBeReadOnly)
    {
        readOnly = shouldBeReadOnly;
        enablementChanged();
        invalidateAccessibilityHandler();

        if (auto* peer = getPeer())
            peer->refreshTextInputTarget();
    }
}

void UnicodeTextEditor::setClicksOutsideDismissVirtualKeyboard (bool newValue)
{
    clicksOutsideDismissVirtualKeyboard = newValue;
}

bool UnicodeTextEditor::isReadOnly() const noexcept
{
    return readOnly || ! isEnabled();
}

bool UnicodeTextEditor::isTextInputActive() const
{
    return ! isReadOnly() && (! clicksOutsideDismissVirtualKeyboard || mouseDownInEditor);
}

void UnicodeTextEditor::setReturnKeyStartsNewLine (bool shouldStartNewLine)
{
    returnKeyStartsNewLine = shouldStartNewLine;
}

void UnicodeTextEditor::setTabKeyUsedAsCharacter (bool shouldTabKeyBeUsed)
{
    tabKeyUsed = shouldTabKeyBeUsed;
}

void UnicodeTextEditor::setPopupMenuEnabled (bool b)
{
    popupMenuEnabled = b;
}

void UnicodeTextEditor::setSelectAllWhenFocused (bool b)
{
    selectAllTextWhenFocused = b;
}

void UnicodeTextEditor::setJustification (Justification j)
{
    if (justification != j)
    {
        justification = j;

        resized();
        repaint();
    }
}

//==============================================================================
void UnicodeTextEditor::setFont (const Font& newFont)
{
    currentFont = newFont;
    scrollToMakeSureCursorIsVisible();
}

void UnicodeTextEditor::applyFontToAllText (const Font& newFont, bool changeCurrentFont)
{
    if (changeCurrentFont)
        currentFont = newFont;

    auto overallColour = findColour (textColourId);

    for (auto* uts : sections)
    {
        uts->setFont (newFont, passwordCharacter);
        uts->colour = overallColour;
    }

    coalesceSimilarSections();
    checkLayout();
    scrollToMakeSureCursorIsVisible();
    repaint();
}

void UnicodeTextEditor::applyColourToAllText (const Colour& newColour, bool changeCurrentTextColour)
{
    for (auto* uts : sections)
        uts->colour = newColour;

    if (changeCurrentTextColour)
        setColour (TextEditor::textColourId, newColour);
    else
        repaint();
}

void UnicodeTextEditor::lookAndFeelChanged()
{
    caret.reset();
    recreateCaret();
    repaint();
}

void UnicodeTextEditor::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void UnicodeTextEditor::enablementChanged()
{
    recreateCaret();
    repaint();
}

void UnicodeTextEditor::setCaretVisible (bool shouldCaretBeVisible)
{
    if (caretVisible != shouldCaretBeVisible)
    {
        caretVisible = shouldCaretBeVisible;
        recreateCaret();
    }
}

void UnicodeTextEditor::recreateCaret()
{
    if (isCaretVisible())
    {
        if (caret == nullptr)
        {
            caret.reset (getLookAndFeel().createCaretComponent (this));
            textHolder->addChildComponent (caret.get());
            updateCaretPosition();
        }
    }
    else
    {
        caret.reset();
    }
}

void UnicodeTextEditor::updateCaretPosition()
{
    if (caret != nullptr
        && getWidth() > 0 && getHeight() > 0)
    {
        Iterator i (*this);
        caret->setCaretPosition (getCaretRectangle().translated (leftIndent,
                                                                 topIndent + roundToInt (i.getYOffset())) - getTextOffset());

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (AccessibilityEvent::textSelectionChanged);
    }
}

UnicodeTextEditor::LengthAndCharacterRestriction::LengthAndCharacterRestriction (int maxLen, const String& chars)
    : allowedCharacters (chars), maxLength (maxLen)
{
}

String UnicodeTextEditor::LengthAndCharacterRestriction::filterNewText (UnicodeTextEditor& ed, const String& newInput)
{
    String t (newInput);

    if (allowedCharacters.isNotEmpty())
        t = t.retainCharacters (allowedCharacters);

    if (maxLength > 0)
        t = t.substring (0, maxLength - (ed.getTotalNumChars() - ed.getHighlightedRegion().getLength()));

    return t;
}

void UnicodeTextEditor::setInputFilter (InputFilter* newFilter, bool takeOwnership)
{
    inputFilter.set (newFilter, takeOwnership);
}

void UnicodeTextEditor::setInputRestrictions (int maxLen, const String& chars)
{
    setInputFilter (new LengthAndCharacterRestriction (maxLen, chars), true);
}

void UnicodeTextEditor::setTextToShowWhenEmpty (const String& text, Colour colourToUse)
{
    textToShowWhenEmpty = text;
    colourForTextWhenEmpty = colourToUse;
}

void UnicodeTextEditor::setPasswordCharacter (juce_wchar newPasswordCharacter)
{
    if (passwordCharacter != newPasswordCharacter)
    {
        passwordCharacter = newPasswordCharacter;
        applyFontToAllText (currentFont);
    }
}

void UnicodeTextEditor::setScrollBarThickness (int newThicknessPixels)
{
    viewport->setScrollBarThickness (newThicknessPixels);
}

//==============================================================================
void UnicodeTextEditor::clear()
{
    clearInternal (nullptr);
    checkLayout();
    undoManager.clearUndoHistory();
    repaint();
}

void UnicodeTextEditor::setText (const String& newText, bool sendTextChangeMessage)
{
    auto newLength = newText.length();

    if (newLength != getTotalNumChars() || getText() != newText)
    {
        if (! sendTextChangeMessage)
            textValue.removeListener (textHolder);

        textValue = newText;

        auto oldCursorPos = caretPosition;
        bool cursorWasAtEnd = oldCursorPos >= getTotalNumChars();

        clearInternal (nullptr);
        insert (newText, 0, currentFont, findColour (textColourId), nullptr, caretPosition);

        // if you're adding text with line-feeds to a single-line text editor, it
        // ain't gonna look right!
        //jassert (multiline || ! newText.containsAnyOf ("\r\n"));

        if (cursorWasAtEnd && ! isMultiLine())
            oldCursorPos = getTotalNumChars();

        moveCaretTo (oldCursorPos, false);

        if (sendTextChangeMessage)
            textChanged();
        else
            textValue.addListener (textHolder);

        checkLayout();
        scrollToMakeSureCursorIsVisible();
        undoManager.clearUndoHistory();

        repaint();
    }
}

//==============================================================================
void UnicodeTextEditor::updateValueFromText()
{
    if (valueTextNeedsUpdating)
    {
        valueTextNeedsUpdating = false;
        textValue = getText();
    }
}

Value& UnicodeTextEditor::getTextValue()
{
    updateValueFromText();
    return textValue;
}

void UnicodeTextEditor::textWasChangedByValue()
{
    if (textValue.getValueSource().getReferenceCount() > 1)
        setText (textValue.getValue());
}

//==============================================================================
void UnicodeTextEditor::textChanged()
{
    checkLayout();

    if (listeners.size() != 0 || onTextChange != nullptr)
        postCommandMessage (TextEditorDefs::textChangeMessageId);

    if (textValue.getValueSource().getReferenceCount() > 1)
    {
        valueTextNeedsUpdating = false;
        textValue = getText();
    }

    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (AccessibilityEvent::textChanged);
}

void UnicodeTextEditor::setSelection (Range<int> newSelection) noexcept
{
    if (newSelection != selection)
    {
        selection = newSelection;

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (AccessibilityEvent::textSelectionChanged);
    }
}

void UnicodeTextEditor::returnPressed()    { postCommandMessage (TextEditorDefs::returnKeyMessageId); }
void UnicodeTextEditor::escapePressed()    { postCommandMessage (TextEditorDefs::escapeKeyMessageId); }

void UnicodeTextEditor::addListener (Listener* l)      { listeners.add (l); }
void UnicodeTextEditor::removeListener (Listener* l)   { listeners.remove (l); }

//==============================================================================
void UnicodeTextEditor::timerCallbackInt()
{
    checkFocus();

    auto now = Time::getApproximateMillisecondCounter();

    if (now > lastTransactionTime + 200)
        newTransaction();
}

void UnicodeTextEditor::checkFocus()
{
    if (! wasFocused && hasKeyboardFocus (false) && ! isCurrentlyBlockedByAnotherModalComponent())
        wasFocused = true;
}

void UnicodeTextEditor::repaintText (Range<int> range)
{
    if (! range.isEmpty())
    {
        if (range.getEnd() >= getTotalNumChars())
        {
            textHolder->repaint();
            return;
        }

        Iterator i (*this);

        Point<float> anchor;
        auto lh = currentFont.getHeight();
        i.getCharPosition (range.getStart(), anchor, lh);

        auto y1 = std::trunc (anchor.y);
        int y2 = 0;

        if (range.getEnd() >= getTotalNumChars())
        {
            y2 = textHolder->getHeight();
        }
        else
        {
            i.getCharPosition (range.getEnd(), anchor, lh);
            y2 = (int) (anchor.y + lh * 2.0f);
        }

        auto offset = i.getYOffset();
        textHolder->repaint (0, roundToInt (y1 + offset), textHolder->getWidth(), roundToInt ((float) y2 - y1 + offset));
    }
}

//==============================================================================
void UnicodeTextEditor::moveCaret (int newCaretPos)
{
    if (newCaretPos < 0)
        newCaretPos = 0;
    else
        newCaretPos = jmin (newCaretPos, getTotalNumChars());

    if (newCaretPos != getCaretPosition())
    {
        caretPosition = newCaretPos;

        if (hasKeyboardFocus (false))
            textHolder->restartTimer();

        scrollToMakeSureCursorIsVisible();
        updateCaretPosition();

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (AccessibilityEvent::textChanged);
    }
}

int UnicodeTextEditor::getCaretPosition() const
{
    return caretPosition;
}

void UnicodeTextEditor::setCaretPosition (const int newIndex)
{
    moveCaretTo (newIndex, false);
}

void UnicodeTextEditor::moveCaretToEnd()
{
    setCaretPosition (std::numeric_limits<int>::max());
}

void UnicodeTextEditor::scrollEditorToPositionCaret (const int desiredCaretX,
                                              const int desiredCaretY)

{
    updateCaretPosition();
    auto caretRect = getCaretRectangle().translated (leftIndent, topIndent);

    auto vx = caretRect.getX() - desiredCaretX;
    auto vy = caretRect.getY() - desiredCaretY;

    if (desiredCaretX < jmax (1, proportionOfWidth (0.05f)))
        vx += desiredCaretX - proportionOfWidth (0.2f);
    else if (desiredCaretX > jmax (0, viewport->getMaximumVisibleWidth() - (wordWrap ? 2 : 10)))
        vx += desiredCaretX + (isMultiLine() ? proportionOfWidth (0.2f) : 10) - viewport->getMaximumVisibleWidth();

    vx = jlimit (0, jmax (0, textHolder->getWidth() + 8 - viewport->getMaximumVisibleWidth()), vx);

    if (! isMultiLine())
    {
        vy = viewport->getViewPositionY();
    }
    else
    {
        vy = jlimit (0, jmax (0, textHolder->getHeight() - viewport->getMaximumVisibleHeight()), vy);

        if (desiredCaretY < 0)
            vy = jmax (0, desiredCaretY + vy);
        else if (desiredCaretY > jmax (0, viewport->getMaximumVisibleHeight() - caretRect.getHeight()))
            vy += desiredCaretY + 2 + caretRect.getHeight() - viewport->getMaximumVisibleHeight();
    }

    viewport->setViewPosition (vx, vy);
}

Rectangle<int> UnicodeTextEditor::getCaretRectangleForCharIndex (int index) const
{
    Point<float> anchor;
    auto cursorHeight = currentFont.getHeight(); // (in case the text is empty and the call below doesn't set this value)
    getCharPosition (index, anchor, cursorHeight);

    return Rectangle<float> { anchor.x, anchor.y, 2.0f, cursorHeight }.getSmallestIntegerContainer() + getTextOffset();
}

Point<int> UnicodeTextEditor::getTextOffset() const noexcept
{
    Iterator i (*this);
    auto yOffset = i.getYOffset();

    return { getLeftIndent() + borderSize.getLeft() - viewport->getViewPositionX(),
        roundToInt ((float) getTopIndent() + (float) borderSize.getTop() + yOffset) - viewport->getViewPositionY() };
}

RectangleList<int> UnicodeTextEditor::getTextBounds (Range<int> textRange) const
{
    RectangleList<int> boundingBox;
    Iterator i (*this);

    while (i.next())
    {
        if (textRange.intersects ({ i.indexInText,
                                    i.indexInText + i.atom->numChars }))
        {
            boundingBox.add (i.getTextBounds (textRange));
        }
    }

    boundingBox.offsetAll (getTextOffset());
    return boundingBox;
}

//==============================================================================
// Extra space for the cursor at the right-hand-edge
constexpr int rightEdgeSpace = 2;

int UnicodeTextEditor::getWordWrapWidth() const
{
    return wordWrap ? getMaximumTextWidth()
                    : std::numeric_limits<int>::max();
}

int UnicodeTextEditor::getMaximumTextWidth() const
{
    return jmax (1, viewport->getMaximumVisibleWidth() - leftIndent - rightEdgeSpace);
}

int UnicodeTextEditor::getMaximumTextHeight() const
{
    return jmax (1, viewport->getMaximumVisibleHeight() - topIndent);
}

void UnicodeTextEditor::checkLayout()
{
    if (getWordWrapWidth() > 0)
    {
        const auto textBottom = Iterator (*this).getTotalTextHeight() + topIndent;
        const auto textRight = jmax (viewport->getMaximumVisibleWidth(),
                                     Iterator (*this).getTextRight() + leftIndent + rightEdgeSpace);

        textHolder->setSize (textRight, textBottom);
        viewport->setScrollBarsShown (scrollbarVisible && multiline && textBottom > viewport->getMaximumVisibleHeight(),
                                      scrollbarVisible && multiline && ! wordWrap && textRight > viewport->getMaximumVisibleWidth());
    }
}

int UnicodeTextEditor::getTextWidth() const    { return textHolder->getWidth(); }
int UnicodeTextEditor::getTextHeight() const   { return textHolder->getHeight(); }

void UnicodeTextEditor::setIndents (int newLeftIndent, int newTopIndent)
{
    if (leftIndent != newLeftIndent || topIndent != newTopIndent)
    {
        leftIndent = newLeftIndent;
        topIndent  = newTopIndent;

        resized();
        repaint();
    }
}

void UnicodeTextEditor::setBorder (BorderSize<int> border)
{
    borderSize = border;
    resized();
}

BorderSize<int> UnicodeTextEditor::getBorder() const
{
    return borderSize;
}

void UnicodeTextEditor::setScrollToShowCursor (const bool shouldScrollToShowCursor)
{
    keepCaretOnScreen = shouldScrollToShowCursor;
}

void UnicodeTextEditor::scrollToMakeSureCursorIsVisible()
{
    updateCaretPosition();

    if (keepCaretOnScreen)
    {
        auto viewPos = viewport->getViewPosition();
        auto caretRect = getCaretRectangle().translated (leftIndent, topIndent) - getTextOffset();
        auto relativeCursor = caretRect.getPosition() - viewPos;

        if (relativeCursor.x < jmax (1, proportionOfWidth (0.05f)))
        {
            viewPos.x += relativeCursor.x - proportionOfWidth (0.2f);
        }
        else if (relativeCursor.x > jmax (0, viewport->getMaximumVisibleWidth() - (wordWrap ? 2 : 10)))
        {
            viewPos.x += relativeCursor.x + (isMultiLine() ? proportionOfWidth (0.2f) : 10) - viewport->getMaximumVisibleWidth();
        }

        viewPos.x = jlimit (0, jmax (0, textHolder->getWidth() + 8 - viewport->getMaximumVisibleWidth()), viewPos.x);

        if (! isMultiLine())
        {
            viewPos.y = (getHeight() - textHolder->getHeight() - topIndent) / -2;
        }
        else if (relativeCursor.y < 0)
        {
            viewPos.y = jmax (0, relativeCursor.y + viewPos.y);
        }
        else if (relativeCursor.y > jmax (0, viewport->getMaximumVisibleHeight() - caretRect.getHeight()))
        {
            viewPos.y += relativeCursor.y + 2 + caretRect.getHeight() - viewport->getMaximumVisibleHeight();
        }

        viewport->setViewPosition (viewPos);
    }
}

void UnicodeTextEditor::moveCaretTo (const int newPosition, const bool isSelecting)
{
    if (isSelecting)
    {
        moveCaret (newPosition);

        auto oldSelection = selection;

        if (dragType == notDragging)
        {
            if (std::abs (getCaretPosition() - selection.getStart()) < std::abs (getCaretPosition() - selection.getEnd()))
                dragType = draggingSelectionStart;
            else
                dragType = draggingSelectionEnd;
        }

        if (dragType == draggingSelectionStart)
        {
            if (getCaretPosition() >= selection.getEnd())
                dragType = draggingSelectionEnd;

            setSelection (Range<int>::between (getCaretPosition(), selection.getEnd()));
        }
        else
        {
            if (getCaretPosition() < selection.getStart())
                dragType = draggingSelectionStart;

            setSelection (Range<int>::between (getCaretPosition(), selection.getStart()));
        }

        repaintText (selection.getUnionWith (oldSelection));
    }
    else
    {
        dragType = notDragging;

        repaintText (selection);

        moveCaret (newPosition);
        setSelection (Range<int>::emptyRange (getCaretPosition()));
    }
}

int UnicodeTextEditor::getTextIndexAt (const int x, const int y) const
{
    const auto offset = getTextOffset();

    return indexAtPosition ((float) (x - offset.x),
                            (float) (y - offset.y));
}

int UnicodeTextEditor::getTextIndexAt (const Point<int> pt) const
{
    return getTextIndexAt (pt.x, pt.y);
}

int UnicodeTextEditor::getCharIndexForPoint (const Point<int> point) const
{
    return getTextIndexAt (isMultiLine() ? point : getTextBounds ({ 0, getTotalNumChars() }).getBounds().getConstrainedPoint (point));
}

void UnicodeTextEditor::insertTextAtCaret (const String& t)
{
    String newText (inputFilter != nullptr ? inputFilter->filterNewText (*this, t) : t);

    if (isMultiLine())
        newText = newText.replace ("\r\n", "\n");
    else
        newText = newText.replaceCharacters ("\r\n", "  ");

    const int insertIndex = selection.getStart();
    const int newCaretPos = insertIndex + newText.length();

    remove (selection, getUndoManager(),
            newText.isNotEmpty() ? newCaretPos - 1 : newCaretPos);

    insert (newText, insertIndex, currentFont, findColour (textColourId),
            getUndoManager(), newCaretPos);

    textChanged();
}

void UnicodeTextEditor::setHighlightedRegion (const Range<int>& newSelection)
{
    if (newSelection == getHighlightedRegion())
        return;

    const auto cursorAtStart = newSelection.getEnd() == getHighlightedRegion().getStart()
                            || newSelection.getEnd() == getHighlightedRegion().getEnd();
    moveCaretTo (cursorAtStart ? newSelection.getEnd() : newSelection.getStart(), false);
    moveCaretTo (cursorAtStart ? newSelection.getStart() : newSelection.getEnd(), true);
}

//==============================================================================
void UnicodeTextEditor::copy()
{
    if (passwordCharacter == 0)
    {
        auto selectedText = getHighlightedText();

        if (selectedText.isNotEmpty())
            SystemClipboard::copyTextToClipboard (selectedText);
    }
}

void UnicodeTextEditor::paste()
{
    if (! isReadOnly())
    {
        auto clip = SystemClipboard::getTextFromClipboard();

        if (clip.isNotEmpty())
            insertTextAtCaret (clip);
    }
}

void UnicodeTextEditor::cut()
{
    if (! isReadOnly())
    {
        moveCaret (selection.getEnd());
        insertTextAtCaret (String());
    }
}

//==============================================================================
void UnicodeTextEditor::drawContent (Graphics& g)
{
    if (getWordWrapWidth() > 0)
    {
        g.setOrigin (leftIndent, topIndent);
        auto clip = g.getClipBounds();

        auto yOffset = Iterator (*this).getYOffset();

        AffineTransform transform;

        if (yOffset > 0)
        {
            transform = AffineTransform::translation (0.0f, yOffset);
            clip.setY (roundToInt ((float) clip.getY() - yOffset));
        }

        Iterator i (*this);
        Colour selectedTextColour;

        if (! selection.isEmpty())
        {
            selectedTextColour = findColour (highlightedTextColourId);

            g.setColour (findColour (highlightColourId).withMultipliedAlpha (hasKeyboardFocus (true) ? 1.0f : 0.5f));

            auto boundingBox = getTextBounds (selection);
            boundingBox.offsetAll (-getTextOffset());

            g.fillPath (boundingBox.toPath(), transform);
        }

        const UniformTextSection* lastSection = nullptr;

        while (i.next() && i.lineY < (float) clip.getBottom())
        {
            if (i.lineY + i.lineHeight >= (float) clip.getY())
            {
                if (selection.intersects ({ i.indexInText, i.indexInText + i.atom->numChars }))
                {
                    i.drawSelectedText (g, selection, selectedTextColour, transform);
                    lastSection = nullptr;
                }
                else
                {
                    i.draw (g, lastSection, transform);
                }
            }
        }

        for (auto& underlinedSection : underlinedSections)
        {
            Iterator i2 (*this);

            while (i2.next() && i2.lineY < (float) clip.getBottom())
            {
                if (i2.lineY + i2.lineHeight >= (float) clip.getY()
                      && underlinedSection.intersects ({ i2.indexInText, i2.indexInText + i2.atom->numChars }))
                {
                    i2.drawUnderline (g, underlinedSection, findColour (textColourId), transform);
                }
            }
        }
    }
}

void UnicodeTextEditor::paint (Graphics& g)
{
    fillTextEditorBackground (g, getWidth(), getHeight());
}

void UnicodeTextEditor::paintOverChildren (Graphics& g)
{
    if (textToShowWhenEmpty.isNotEmpty()
         && (! hasKeyboardFocus (false))
         && getTotalNumChars() == 0)
    {
        g.setColour (colourForTextWhenEmpty);
        g.setFont (getFont());

        Rectangle<int> textBounds (leftIndent,
                                   topIndent,
                                   viewport->getWidth() - leftIndent,
                                   getHeight() - topIndent);

        if (! textBounds.isEmpty())
            g.drawText (textToShowWhenEmpty, textBounds, justification, true);
    }

    drawTextEditorOutline (g, getWidth(), getHeight());
}

//==============================================================================
void UnicodeTextEditor::addPopupMenuItems (PopupMenu& m, const MouseEvent*)
{
    const bool writable = ! isReadOnly();

    if (passwordCharacter == 0)
    {
        m.addItem (StandardApplicationCommandIDs::cut,   TRANS("Cut"), writable);
        m.addItem (StandardApplicationCommandIDs::copy,  TRANS("Copy"), ! selection.isEmpty());
    }

    m.addItem (StandardApplicationCommandIDs::paste,     TRANS("Paste"), writable);
    m.addItem (StandardApplicationCommandIDs::del,       TRANS("Delete"), writable);
    m.addSeparator();
    m.addItem (StandardApplicationCommandIDs::selectAll, TRANS("Select All"));
    m.addSeparator();

    if (getUndoManager() != nullptr)
    {
        m.addItem (StandardApplicationCommandIDs::undo, TRANS("Undo"), undoManager.canUndo());
        m.addItem (StandardApplicationCommandIDs::redo, TRANS("Redo"), undoManager.canRedo());
    }
}

void UnicodeTextEditor::performPopupMenuAction (const int menuItemID)
{
    switch (menuItemID)
    {
        case StandardApplicationCommandIDs::cut:        cutToClipboard(); break;
        case StandardApplicationCommandIDs::copy:       copyToClipboard(); break;
        case StandardApplicationCommandIDs::paste:      pasteFromClipboard(); break;
        case StandardApplicationCommandIDs::del:        cut(); break;
        case StandardApplicationCommandIDs::selectAll:  selectAll(); break;
        case StandardApplicationCommandIDs::undo:       undo(); break;
        case StandardApplicationCommandIDs::redo:       redo(); break;
        default: break;
    }
}

//==============================================================================
void UnicodeTextEditor::mouseDown (const MouseEvent& e)
{
    mouseDownInEditor = e.originalComponent == this;

    if (! mouseDownInEditor)
        return;

    beginDragAutoRepeat (100);
    newTransaction();

    if (wasFocused || ! selectAllTextWhenFocused)
    {
        if (! (popupMenuEnabled && e.mods.isPopupMenu()))
        {
            moveCaretTo (getTextIndexAt (e.getPosition()), e.mods.isShiftDown());

            if (auto* peer = getPeer())
                peer->closeInputMethodContext();
        }
        else
        {
            PopupMenu m;
            m.setLookAndFeel (&getLookAndFeel());
            addPopupMenuItems (m, &e);

            menuActive = true;

            m.showMenuAsync (PopupMenu::Options(),
                             [safeThis = SafePointer<UnicodeTextEditor> { this }] (int menuResult)
                             {
                                 if (auto* editor = safeThis.getComponent())
                                 {
                                     editor->menuActive = false;

                                     if (menuResult != 0)
                                         editor->performPopupMenuAction (menuResult);
                                 }
                             });
        }
    }
}

void UnicodeTextEditor::mouseDrag (const MouseEvent& e)
{
    if (! mouseDownInEditor)
        return;

    if (wasFocused || ! selectAllTextWhenFocused)
        if (! (popupMenuEnabled && e.mods.isPopupMenu()))
            moveCaretTo (getTextIndexAt (e.getPosition()), true);
}

void UnicodeTextEditor::mouseUp (const MouseEvent& e)
{
    if (! mouseDownInEditor)
        return;

    newTransaction();
    textHolder->restartTimer();

    if (wasFocused || ! selectAllTextWhenFocused)
        if (e.mouseWasClicked() && ! (popupMenuEnabled && e.mods.isPopupMenu()))
            moveCaret (getTextIndexAt (e.getPosition()));

    wasFocused = true;
}

void UnicodeTextEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (! mouseDownInEditor)
        return;

    int tokenEnd = getTextIndexAt (e.getPosition());
    int tokenStart = 0;

    if (e.getNumberOfClicks() > 3)
    {
        tokenEnd = getTotalNumChars();
    }
    else
    {
        auto t = getText();
        auto totalLength = getTotalNumChars();

        while (tokenEnd < totalLength)
        {
            auto c = t[tokenEnd];

            // (note the slight bodge here - it's because iswalnum only checks for alphabetic chars in the current locale)
            if (CharacterFunctions::isLetterOrDigit (c) || c > 128)
                ++tokenEnd;
            else
                break;
        }

        tokenStart = tokenEnd;

        while (tokenStart > 0)
        {
            auto c = t[tokenStart - 1];

            // (note the slight bodge here - it's because iswalnum only checks for alphabetic chars in the current locale)
            if (CharacterFunctions::isLetterOrDigit (c) || c > 128)
                --tokenStart;
            else
                break;
        }

        if (e.getNumberOfClicks() > 2)
        {
            while (tokenEnd < totalLength)
            {
                auto c = t[tokenEnd];

                if (c != '\r' && c != '\n')
                    ++tokenEnd;
                else
                    break;
            }

            while (tokenStart > 0)
            {
                auto c = t[tokenStart - 1];

                if (c != '\r' && c != '\n')
                    --tokenStart;
                else
                    break;
            }
        }
    }

    moveCaretTo (tokenEnd, false);
    moveCaretTo (tokenStart, true);
}

void UnicodeTextEditor::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    if (! mouseDownInEditor)
        return;

    if (! viewport->useMouseWheelMoveIfNeeded (e, wheel))
        Component::mouseWheelMove (e, wheel);
}

//==============================================================================
bool UnicodeTextEditor::moveCaretWithTransaction (const int newPos, const bool selecting)
{
    newTransaction();
    moveCaretTo (newPos, selecting);

    if (auto* peer = getPeer())
        peer->closeInputMethodContext();

    return true;
}

bool UnicodeTextEditor::moveCaretLeft (bool moveInWholeWordSteps, bool selecting)
{
    auto pos = getCaretPosition();

    if (moveInWholeWordSteps)
        pos = findWordBreakBefore (pos);
    else
        --pos;

    return moveCaretWithTransaction (pos, selecting);
}

bool UnicodeTextEditor::moveCaretRight (bool moveInWholeWordSteps, bool selecting)
{
    auto pos = getCaretPosition();

    if (moveInWholeWordSteps)
        pos = findWordBreakAfter (pos);
    else
        ++pos;

    return moveCaretWithTransaction (pos, selecting);
}

bool UnicodeTextEditor::moveCaretUp (bool selecting)
{
    if (! isMultiLine())
        return moveCaretToStartOfLine (selecting);

    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition (caretPos.getX(), caretPos.getY() - 1.0f), selecting);
}

bool UnicodeTextEditor::moveCaretDown (bool selecting)
{
    if (! isMultiLine())
        return moveCaretToEndOfLine (selecting);

    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition (caretPos.getX(), caretPos.getBottom() + 1.0f), selecting);
}

bool UnicodeTextEditor::pageUp (bool selecting)
{
    if (! isMultiLine())
        return moveCaretToStartOfLine (selecting);

    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition (caretPos.getX(), caretPos.getY() - (float) viewport->getViewHeight()), selecting);
}

bool UnicodeTextEditor::pageDown (bool selecting)
{
    if (! isMultiLine())
        return moveCaretToEndOfLine (selecting);

    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition (caretPos.getX(), caretPos.getBottom() + (float) viewport->getViewHeight()), selecting);
}

void UnicodeTextEditor::scrollByLines (int deltaLines)
{
    viewport->getVerticalScrollBar().moveScrollbarInSteps (deltaLines);
}

bool UnicodeTextEditor::scrollDown()
{
    scrollByLines (-1);
    return true;
}

bool UnicodeTextEditor::scrollUp()
{
    scrollByLines (1);
    return true;
}

bool UnicodeTextEditor::moveCaretToTop (bool selecting)
{
    return moveCaretWithTransaction (0, selecting);
}

bool UnicodeTextEditor::moveCaretToStartOfLine (bool selecting)
{
    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition (0.0f, caretPos.getY()), selecting);
}

bool UnicodeTextEditor::moveCaretToEnd (bool selecting)
{
    return moveCaretWithTransaction (getTotalNumChars(), selecting);
}

bool UnicodeTextEditor::moveCaretToEndOfLine (bool selecting)
{
    const auto caretPos = (getCaretRectangle() - getTextOffset()).toFloat();
    return moveCaretWithTransaction (indexAtPosition ((float) textHolder->getWidth(), caretPos.getY()), selecting);
}

bool UnicodeTextEditor::deleteBackwards (bool moveInWholeWordSteps)
{
    if (moveInWholeWordSteps)
        moveCaretTo (findWordBreakBefore (getCaretPosition()), true);
    else if (selection.isEmpty() && selection.getStart() > 0)
        setSelection ({ selection.getEnd() - 1, selection.getEnd() });

    cut();
    return true;
}

bool UnicodeTextEditor::deleteForwards (bool /*moveInWholeWordSteps*/)
{
    if (selection.isEmpty() && selection.getStart() < getTotalNumChars())
        setSelection ({ selection.getStart(), selection.getStart() + 1 });

    cut();
    return true;
}

bool UnicodeTextEditor::copyToClipboard()
{
    newTransaction();
    copy();
    return true;
}

bool UnicodeTextEditor::cutToClipboard()
{
    newTransaction();
    copy();
    cut();
    return true;
}

bool UnicodeTextEditor::pasteFromClipboard()
{
    newTransaction();
    paste();
    return true;
}

bool UnicodeTextEditor::selectAll()
{
    newTransaction();
    moveCaretTo (getTotalNumChars(), false);
    moveCaretTo (0, true);
    return true;
}

//==============================================================================
void UnicodeTextEditor::setEscapeAndReturnKeysConsumed (bool shouldBeConsumed) noexcept
{
    consumeEscAndReturnKeys = shouldBeConsumed;
}

bool UnicodeTextEditor::keyPressed (const KeyPress& key)
{
    if (isReadOnly() && key != KeyPress ('c', ModifierKeys::commandModifier, 0)
                     && key != KeyPress ('a', ModifierKeys::commandModifier, 0))
        return false;

    if (! TextEditorKeyMapper<UnicodeTextEditor>::invokeKeyFunction (*this, key))
    {
        if (key == KeyPress::returnKey)
        {
            newTransaction();

            if (returnKeyStartsNewLine)
            {
                insertTextAtCaret ("\n");
            }
            else
            {
                returnPressed();
                return consumeEscAndReturnKeys;
            }
        }
        else if (key.isKeyCode (KeyPress::escapeKey))
        {
            newTransaction();
            moveCaretTo (getCaretPosition(), false);
            escapePressed();
            return consumeEscAndReturnKeys;
        }
        else if (key.getTextCharacter() >= ' '
                  || (tabKeyUsed && (key.getTextCharacter() == '\t')))
        {
            insertTextAtCaret (String::charToString (key.getTextCharacter()));

            lastTransactionTime = Time::getApproximateMillisecondCounter();
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool UnicodeTextEditor::keyStateChanged (const bool isKeyDown)
{
    if (! isKeyDown)
        return false;

   #if JUCE_WINDOWS
    if (KeyPress (KeyPress::F4Key, ModifierKeys::altModifier, 0).isCurrentlyDown())
        return false;  // We need to explicitly allow alt-F4 to pass through on Windows
   #endif

    if ((! consumeEscAndReturnKeys)
         && (KeyPress (KeyPress::escapeKey).isCurrentlyDown()
          || KeyPress (KeyPress::returnKey).isCurrentlyDown()))
        return false;

    // (overridden to avoid forwarding key events to the parent)
    return ! ModifierKeys::currentModifiers.isCommandDown();
}

//==============================================================================
void UnicodeTextEditor::focusGained (FocusChangeType cause)
{
    newTransaction();

    if (selectAllTextWhenFocused)
    {
        moveCaretTo (0, false);
        moveCaretTo (getTotalNumChars(), true);
    }

    checkFocus();

    if (cause == FocusChangeType::focusChangedByMouseClick && selectAllTextWhenFocused)
        wasFocused = false;

    repaint();
    updateCaretPosition();
}

void UnicodeTextEditor::focusLost (FocusChangeType)
{
    newTransaction();

    wasFocused = false;
    textHolder->stopTimer();

    underlinedSections.clear();

    updateCaretPosition();

    postCommandMessage (TextEditorDefs::focusLossMessageId);
    repaint();
}

//==============================================================================
void UnicodeTextEditor::resized()
{
    viewport->setBoundsInset (borderSize);
    viewport->setSingleStepSizes (16, roundToInt (currentFont.getHeight()));

    checkLayout();

    if (isMultiLine())
        updateCaretPosition();
    else
        scrollToMakeSureCursorIsVisible();
}

void UnicodeTextEditor::handleCommandMessage (const int commandId)
{
    Component::BailOutChecker checker (this);

    switch (commandId)
    {
    case TextEditorDefs::textChangeMessageId:
        listeners.callChecked (checker, [this] (Listener& l) { l.textEditorTextChanged (*this); });

        if (! checker.shouldBailOut() && onTextChange != nullptr)
            onTextChange();

        break;

    case TextEditorDefs::returnKeyMessageId:
        listeners.callChecked (checker, [this] (Listener& l) { l.textEditorReturnKeyPressed (*this); });

        if (! checker.shouldBailOut() && onReturnKey != nullptr)
            onReturnKey();

        break;

    case TextEditorDefs::escapeKeyMessageId:
        listeners.callChecked (checker, [this] (Listener& l) { l.textEditorEscapeKeyPressed (*this); });

        if (! checker.shouldBailOut() && onEscapeKey != nullptr)
            onEscapeKey();

        break;

    case TextEditorDefs::focusLossMessageId:
        updateValueFromText();
        listeners.callChecked (checker, [this] (Listener& l) { l.textEditorFocusLost (*this); });

        if (! checker.shouldBailOut() && onFocusLost != nullptr)
            onFocusLost();

        break;

    default:
        jassertfalse;
        break;
    }
}

void UnicodeTextEditor::setTemporaryUnderlining (const Array<Range<int>>& newUnderlinedSections)
{
    underlinedSections = newUnderlinedSections;
    repaint();
}

//==============================================================================
UndoManager* UnicodeTextEditor::getUndoManager() noexcept
{
    return readOnly ? nullptr : &undoManager;
}

void UnicodeTextEditor::clearInternal (UndoManager* const um)
{
    remove ({ 0, getTotalNumChars() }, um, caretPosition);
}

void UnicodeTextEditor::insert (const String& text, int insertIndex, const Font& font,
                                Colour colour, UndoManager* um, int caretPositionToMoveTo)
{
    if (text.isNotEmpty())
    {
        if (um != nullptr)
        {
            if (um->getNumActionsInCurrentTransaction() > TextEditorDefs::maxActionsPerTransaction)
                newTransaction();

            um->perform (new InsertAction (*this, text, insertIndex, font, colour,
                                           caretPosition, caretPositionToMoveTo));
        }
        else
        {
            repaintText ({ insertIndex, getTotalNumChars() }); // must do this before and after changing the data, in case
                                                               // a line gets moved due to word wrap

            int index = 0;
            int nextIndex = 0;

            for (int i = 0; i < sections.size(); ++i)
            {
                nextIndex = index + sections.getUnchecked (i)->getTotalLength();

                if (insertIndex == index)
                {
                    sections.insert (i, new UniformTextSection (text, font, colour, passwordCharacter));
                    break;
                }

                if (insertIndex > index && insertIndex < nextIndex)
                {
                    splitSection (i, insertIndex - index);
                    sections.insert (i + 1, new UniformTextSection (text, font, colour, passwordCharacter));
                    break;
                }

                index = nextIndex;
            }

            if (nextIndex == insertIndex)
                sections.add (new UniformTextSection (text, font, colour, passwordCharacter));

            coalesceSimilarSections();
            totalNumChars = -1;
            valueTextNeedsUpdating = true;

            checkLayout();
            moveCaretTo (caretPositionToMoveTo, false);

            repaintText ({ insertIndex, getTotalNumChars() });
        }
    }
}

void UnicodeTextEditor::reinsert (int insertIndex, const OwnedArray<UniformTextSection>& sectionsToInsert)
{
    int index = 0;
    int nextIndex = 0;

    for (int i = 0; i < sections.size(); ++i)
    {
        nextIndex = index + sections.getUnchecked (i)->getTotalLength();

        if (insertIndex == index)
        {
            for (int j = sectionsToInsert.size(); --j >= 0;)
                sections.insert (i, new UniformTextSection (*sectionsToInsert.getUnchecked(j)));

            break;
        }

        if (insertIndex > index && insertIndex < nextIndex)
        {
            splitSection (i, insertIndex - index);

            for (int j = sectionsToInsert.size(); --j >= 0;)
                sections.insert (i + 1, new UniformTextSection (*sectionsToInsert.getUnchecked(j)));

            break;
        }

        index = nextIndex;
    }

    if (nextIndex == insertIndex)
        for (auto* s : sectionsToInsert)
            sections.add (new UniformTextSection (*s));

    coalesceSimilarSections();
    totalNumChars = -1;
    valueTextNeedsUpdating = true;
}

void UnicodeTextEditor::remove (Range<int> range, UndoManager* const um, const int caretPositionToMoveTo)
{
    if (! range.isEmpty())
    {
        int index = 0;

        for (int i = 0; i < sections.size(); ++i)
        {
            auto nextIndex = index + sections.getUnchecked(i)->getTotalLength();

            if (range.getStart() > index && range.getStart() < nextIndex)
            {
                splitSection (i, range.getStart() - index);
                --i;
            }
            else if (range.getEnd() > index && range.getEnd() < nextIndex)
            {
                splitSection (i, range.getEnd() - index);
                --i;
            }
            else
            {
                index = nextIndex;

                if (index > range.getEnd())
                    break;
            }
        }

        index = 0;

        if (um != nullptr)
        {
            Array<UniformTextSection*> removedSections;

            for (auto* section : sections)
            {
                if (range.getEnd() <= range.getStart())
                    break;

                auto nextIndex = index + section->getTotalLength();

                if (range.getStart() <= index && range.getEnd() >= nextIndex)
                    removedSections.add (new UniformTextSection (*section));

                index = nextIndex;
            }

            if (um->getNumActionsInCurrentTransaction() > TextEditorDefs::maxActionsPerTransaction)
                newTransaction();

            um->perform (new RemoveAction (*this, range, caretPosition,
                                           caretPositionToMoveTo, removedSections));
        }
        else
        {
            auto remainingRange = range;

            for (int i = 0; i < sections.size(); ++i)
            {
                auto* section = sections.getUnchecked (i);
                auto nextIndex = index + section->getTotalLength();

                if (remainingRange.getStart() <= index && remainingRange.getEnd() >= nextIndex)
                {
                    sections.remove (i);
                    remainingRange.setEnd (remainingRange.getEnd() - (nextIndex - index));

                    if (remainingRange.isEmpty())
                        break;

                    --i;
                }
                else
                {
                    index = nextIndex;
                }
            }

            coalesceSimilarSections();
            totalNumChars = -1;
            valueTextNeedsUpdating = true;

            checkLayout();
            moveCaretTo (caretPositionToMoveTo, false);

            repaintText ({ range.getStart(), getTotalNumChars() });
        }
    }
}

//==============================================================================
String UnicodeTextEditor::getText() const
{
    MemoryOutputStream mo;
    mo.preallocate ((size_t) getTotalNumChars());

    for (auto* s : sections)
        s->appendAllText (mo);

    return mo.toUTF8();
}

String UnicodeTextEditor::getTextInRange (const Range<int>& range) const
{
    if (range.isEmpty())
        return {};

    MemoryOutputStream mo;
    mo.preallocate ((size_t) jmin (getTotalNumChars(), range.getLength()));

    int index = 0;

    for (auto* s : sections)
    {
        auto nextIndex = index + s->getTotalLength();

        if (range.getStart() < nextIndex)
        {
            if (range.getEnd() <= index)
                break;

            s->appendSubstring (mo, range - index);
        }

        index = nextIndex;
    }

    return mo.toUTF8();
}

String UnicodeTextEditor::getHighlightedText() const
{
    return getTextInRange (selection);
}

int UnicodeTextEditor::getTotalNumChars() const
{
    if (totalNumChars < 0)
    {
        totalNumChars = 0;

        for (auto* s : sections)
            totalNumChars += s->getTotalLength();
    }

    return totalNumChars;
}

bool UnicodeTextEditor::isEmpty() const
{
    return getTotalNumChars() == 0;
}

void UnicodeTextEditor::getCharPosition (int index, Point<float>& anchor, float& lineHeight) const
{
    if (getWordWrapWidth() <= 0)
    {
        anchor = {};
        lineHeight = currentFont.getHeight();
    }
    else
    {
        Iterator i (*this);

        if (sections.isEmpty())
        {
            anchor = { i.getJustificationOffsetX (0), 0 };
            lineHeight = currentFont.getHeight();
        }
        else
        {
            i.getCharPosition (index, anchor, lineHeight);
        }
    }
}

int UnicodeTextEditor::indexAtPosition (const float x, const float y) const
{
    if (getWordWrapWidth() > 0)
    {
        for (Iterator i (*this); i.next();)
        {
            if (y < i.lineY + i.lineHeight)
            {
                if (y < i.lineY)
                    return jmax (0, i.indexInText - 1);

                if (x <= i.atomX || i.atom->isNewLine())
                    return i.indexInText;

                if (x < i.atomRight)
                    return i.xToIndex (x);
            }
        }
    }

    return getTotalNumChars();
}

//==============================================================================
int UnicodeTextEditor::findWordBreakAfter (const int position) const
{
    auto t = getTextInRange ({ position, position + 512 });
    auto totalLength = t.length();
    int i = 0;

    while (i < totalLength && CharacterFunctions::isWhitespace (t[i]))
        ++i;

    auto type = TextEditorDefs::getCharacterCategory (t[i]);

    while (i < totalLength && type == TextEditorDefs::getCharacterCategory (t[i]))
        ++i;

    while (i < totalLength && CharacterFunctions::isWhitespace (t[i]))
        ++i;

    return position + i;
}

int UnicodeTextEditor::findWordBreakBefore (const int position) const
{
    if (position <= 0)
        return 0;

    auto startOfBuffer = jmax (0, position - 512);
    auto t = getTextInRange ({ startOfBuffer, position });

    int i = position - startOfBuffer;

    while (i > 0 && CharacterFunctions::isWhitespace (t [i - 1]))
        --i;

    if (i > 0)
    {
        auto type = TextEditorDefs::getCharacterCategory (t [i - 1]);

        while (i > 0 && type == TextEditorDefs::getCharacterCategory (t [i - 1]))
            --i;
    }

    jassert (startOfBuffer + i >= 0);
    return startOfBuffer + i;
}


//==============================================================================
void UnicodeTextEditor::splitSection (const int sectionIndex, const int charToSplitAt)
{
    jassert (sections[sectionIndex] != nullptr);

    sections.insert (sectionIndex + 1,
                     sections.getUnchecked (sectionIndex)->split (charToSplitAt));
}

void UnicodeTextEditor::coalesceSimilarSections()
{
    for (int i = 0; i < sections.size() - 1; ++i)
    {
        auto* s1 = sections.getUnchecked (i);
        auto* s2 = sections.getUnchecked (i + 1);

        if (s1->font == s2->font
             && s1->colour == s2->colour)
        {
            s1->append (*s2);
            sections.remove (i + 1);
            --i;
        }
    }
}

//==============================================================================
class UnicodeTextEditor::EditorAccessibilityHandler  : public AccessibilityHandler
{
public:
    explicit EditorAccessibilityHandler (UnicodeTextEditor& textEditorToWrap)
        : AccessibilityHandler (textEditorToWrap,
                                textEditorToWrap.isReadOnly() ? AccessibilityRole::staticText : AccessibilityRole::editableText,
                                {},
                                { std::make_unique<TextEditorTextInterface> (textEditorToWrap) }),
          unicodeTextEditor (textEditorToWrap)
    {
    }

    String getHelp() const override  { return unicodeTextEditor.getTooltip(); }

private:
    class TextEditorTextInterface  : public AccessibilityTextInterface
    {
    public:
        explicit TextEditorTextInterface (UnicodeTextEditor& editor)
            : unicodeTextEditor (editor)
        {
        }

        bool isDisplayingProtectedText() const override      { return unicodeTextEditor.getPasswordCharacter() != 0; }
        bool isReadOnly() const override                     { return unicodeTextEditor.isReadOnly(); }

        int getTotalNumCharacters() const override           { return unicodeTextEditor.getText().length(); }
        Range<int> getSelection() const override             { return unicodeTextEditor.getHighlightedRegion(); }

        void setSelection (Range<int> r) override
        {
            unicodeTextEditor.setHighlightedRegion (r);
        }

        String getText (Range<int> r) const override
        {
            if (isDisplayingProtectedText())
                return String::repeatedString (String::charToString (unicodeTextEditor.getPasswordCharacter()),
                                               getTotalNumCharacters());

            return unicodeTextEditor.getTextInRange (r);
        }

        void setText (const String& newText) override
        {
            unicodeTextEditor.setText (newText);
        }

        int getTextInsertionOffset() const override          { return unicodeTextEditor.getCaretPosition(); }

        RectangleList<int> getTextBounds (Range<int> textRange) const override
        {
            auto localRects = unicodeTextEditor.getTextBounds (textRange);
            RectangleList<int> globalRects;

            std::for_each (localRects.begin(), localRects.end(),
                           [&] (const Rectangle<int>& r) { globalRects.add ( unicodeTextEditor.localAreaToGlobal (r)); });

            return globalRects;
        }

        int getOffsetAtPoint (Point<int> point) const override
        {
            return unicodeTextEditor.getTextIndexAt (unicodeTextEditor.getLocalPoint (nullptr, point));
        }

    private:
        UnicodeTextEditor& unicodeTextEditor;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextEditorTextInterface)
    };

    UnicodeTextEditor& unicodeTextEditor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorAccessibilityHandler)
};

std::unique_ptr<AccessibilityHandler> UnicodeTextEditor::createAccessibilityHandler()
{
    return std::make_unique<EditorAccessibilityHandler> (*this);
}

// LookAndFeel-ish functions: you can override these for custom drawing behaviour
void UnicodeTextEditor::fillTextEditorBackground (Graphics& g, int width, int height)
{
    g.setColour (findColour (TextEditor::backgroundColourId));
    g.fillRect (0, 0, width, height);

    if(getProperties()["NoOutline"].isVoid()) {
        g.setColour (findColour (TextEditor::outlineColourId));
        g.drawHorizontalLine (height - 1, 0.0f, static_cast<float> (width));
    }
}

void UnicodeTextEditor::drawTextEditorOutline (Graphics& g, int width, int height)
{
    if (isEnabled() && getProperties()["NoOutline"].isVoid())
    {
        if (hasKeyboardFocus (true) && ! isReadOnly())
        {
            g.setColour (findColour (TextEditor::focusedOutlineColourId));
            g.drawRect (0, 0, width, height, 2);
        }
        else
        {
            g.setColour (findColour (TextEditor::outlineColourId));
            g.drawRect (0, 0, width, height);
        }
    }
}

CaretComponent* UnicodeTextEditor::createCaretComponent (Component* keyFocusOwner)
{
    return new CaretComponent (keyFocusOwner);
}



// Unicode Label

UnicodeLabel::UnicodeLabel (const String& name, const String& labelText)
    : Component (name),
      textValue (labelText),
      lastTextValue (labelText)
{
    setColour (TextEditor::textColourId, Colours::black);
    setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
    setColour (TextEditor::outlineColourId, Colours::transparentBlack);

    textValue.addListener (this);
}

UnicodeLabel::~UnicodeLabel()
{
    textValue.removeListener (this);

    if (ownerComponent != nullptr)
        ownerComponent->removeComponentListener (this);

    editor.reset();
}

//==============================================================================
void UnicodeLabel::setText (const String& newText, NotificationType notification)
{
    hideEditor (true);

    if (lastTextValue != newText)
    {
        lastTextValue = newText;
        textValue = newText;
        repaint();

        textWasChanged();

        if (ownerComponent != nullptr)
            componentMovedOrResized (*ownerComponent, true, true);

        if (notification != dontSendNotification)
            callChangeListeners();
    }
}

String UnicodeLabel::getText (bool returnActiveEditorContents) const
{
    return (returnActiveEditorContents && isBeingEdited())
                ? editor->getText()
                : textValue.toString();
}

void UnicodeLabel::valueChanged (Value&)
{
    if (lastTextValue != textValue.toString())
        setText (textValue.toString(), sendNotification);
}

//==============================================================================
void UnicodeLabel::setFont (const Font& newFont)
{
    if (font != newFont)
    {
        font = newFont;
        repaint();
    }
}

Font UnicodeLabel::getFont() const noexcept
{
    return font;
}

void UnicodeLabel::setEditable (bool editOnSingleClick,
                         bool editOnDoubleClick,
                         bool lossOfFocusDiscards)
{
    editSingleClick = editOnSingleClick;
    editDoubleClick = editOnDoubleClick;
    lossOfFocusDiscardsChanges = lossOfFocusDiscards;

    const auto isKeybordFocusable = (editOnSingleClick || editOnDoubleClick);

    setWantsKeyboardFocus (isKeybordFocusable);
    setFocusContainerType (isKeybordFocusable ? FocusContainerType::keyboardFocusContainer
                                              : FocusContainerType::none);

    invalidateAccessibilityHandler();
}

void UnicodeLabel::setJustificationType (Justification newJustification)
{
    if (justification != newJustification)
    {
        justification = newJustification;
        repaint();
    }
}

void UnicodeLabel::setBorderSize (BorderSize<int> newBorder)
{
    if (border != newBorder)
    {
        border = newBorder;
        repaint();
    }
}

//==============================================================================
Component* UnicodeLabel::getAttachedComponent() const
{
    return ownerComponent.get();
}

void UnicodeLabel::attachToComponent (Component* owner, bool onLeft)
{
    jassert (owner != this); // Not a great idea to try to attach it to itself!

    if (ownerComponent != nullptr)
        ownerComponent->removeComponentListener (this);

    ownerComponent = owner;
    leftOfOwnerComp = onLeft;

    if (ownerComponent != nullptr)
    {
        setVisible (ownerComponent->isVisible());
        ownerComponent->addComponentListener (this);
        componentParentHierarchyChanged (*ownerComponent);
        componentMovedOrResized (*ownerComponent, true, true);
    }
}

void UnicodeLabel::componentMovedOrResized (Component& component, bool /*wasMoved*/, bool /*wasResized*/)
{
    auto f = getFont();
    auto borderSize = getBorderSize();

    if (leftOfOwnerComp)
    {
        auto width = jmin (roundToInt (f.getStringWidthFloat (textValue.toString()) + 0.5f)
                             + borderSize.getLeftAndRight(),
                           component.getX());

        setBounds (component.getX() - width, component.getY(), width, component.getHeight());
    }
    else
    {
        auto height = borderSize.getTopAndBottom() + 6 + roundToInt (f.getHeight() + 0.5f);

        setBounds (component.getX(), component.getY() - height, component.getWidth(), height);
    }
}

void UnicodeLabel::componentParentHierarchyChanged (Component& component)
{
    if (auto* parent = component.getParentComponent())
        parent->addChildComponent (this);
}

void UnicodeLabel::componentVisibilityChanged (Component& component)
{
    setVisible (component.isVisible());
}

//==============================================================================
void UnicodeLabel::textWasEdited() {}
void UnicodeLabel::textWasChanged() {}

void UnicodeLabel::editorShown (UnicodeTextEditor* textEditor)
{
    Component::BailOutChecker checker (this);
    listeners.callChecked (checker, [this, textEditor] (UnicodeLabel::Listener& l) { l.editorShown (this, *textEditor); });

    if (checker.shouldBailOut())
        return;

    if (onEditorShow != nullptr)
        onEditorShow();
}

void UnicodeLabel::editorAboutToBeHidden (UnicodeTextEditor* textEditor)
{
    Component::BailOutChecker checker (this);
    listeners.callChecked (checker, [this, textEditor] (UnicodeLabel::Listener& l) { l.editorHidden (this, *textEditor); });

    if (checker.shouldBailOut())
        return;

    if (onEditorHide != nullptr)
        onEditorHide();
}

void UnicodeLabel::showEditor()
{
    if (editor == nullptr)
    {
        editor.reset (createEditorComponent());
        editor->setSize (10, 10);
        addAndMakeVisible (editor.get());
        editor->setText (getText(), false);
        editor->setKeyboardType (keyboardType);
        editor->addListener (this);
        editor->grabKeyboardFocus();

        if (editor == nullptr) // may be deleted by a callback
            return;

        editor->setHighlightedRegion (Range<int> (0, textValue.toString().length()));

        resized();
        repaint();

        editorShown (editor.get());

        enterModalState (false);
        editor->grabKeyboardFocus();
    }
}

bool UnicodeLabel::updateFromTextEditorContents (UnicodeTextEditor& ed)
{
    auto newText = ed.getText();

    if (textValue.toString() != newText)
    {
        lastTextValue = newText;
        textValue = newText;
        repaint();

        textWasChanged();

        if (ownerComponent != nullptr)
            componentMovedOrResized (*ownerComponent, true, true);

        return true;
    }

    return false;
}

void UnicodeLabel::hideEditor (bool discardCurrentEditorContents)
{
    if (editor != nullptr)
    {
        WeakReference<Component> deletionChecker (this);
        std::unique_ptr<UnicodeTextEditor> outgoingEditor;
        std::swap (outgoingEditor, editor);

        editorAboutToBeHidden (outgoingEditor.get());

        const bool changed = (! discardCurrentEditorContents)
                               && updateFromTextEditorContents (*outgoingEditor);
        outgoingEditor.reset();

        if (deletionChecker != nullptr)
            repaint();

        if (changed)
            textWasEdited();

        if (deletionChecker != nullptr)
            exitModalState (0);

        if (changed && deletionChecker != nullptr)
            callChangeListeners();
    }
}

void UnicodeLabel::inputAttemptWhenModal()
{
    if (editor != nullptr)
    {
        if (lossOfFocusDiscardsChanges)
            textEditorEscapeKeyPressed (*editor);
        else
            textEditorReturnKeyPressed (*editor);
    }
}

bool UnicodeLabel::isBeingEdited() const noexcept
{
    return editor != nullptr;
}

static void copyColourIfSpecified (UnicodeLabel& l, UnicodeTextEditor& ed, int colourID, int targetColourID)
{
    if (l.isColourSpecified (colourID) || l.getLookAndFeel().isColourSpecified (colourID))
        ed.setColour (targetColourID, l.findColour (colourID));
}

UnicodeTextEditor* UnicodeLabel::createEditorComponent()
{
    auto* ed = new UnicodeTextEditor (getName());
    ed->applyFontToAllText (getFont());
    copyAllExplicitColoursTo (*ed);

    copyColourIfSpecified (*this, *ed, Label::textWhenEditingColourId, TextEditor::textColourId);
    copyColourIfSpecified (*this, *ed, Label::backgroundWhenEditingColourId, TextEditor::backgroundColourId);
    copyColourIfSpecified (*this, *ed, Label::outlineWhenEditingColourId, TextEditor::focusedOutlineColourId);

    return ed;
}

UnicodeTextEditor* UnicodeLabel::getCurrentTextEditor() const noexcept
{
    return editor.get();
}

//==============================================================================
void UnicodeLabel::paint (Graphics& g)
{
    g.fillAll(findColour(Label::backgroundColourId));

    if (!isBeingEdited()) {
        auto alpha = isEnabled() ? 1.0f : 0.5f;
        const Font font = getFont();
        auto textArea = getBorderSize().subtractedFrom(getLocalBounds());
        auto colour = findColour(Label::textColourId).withMultipliedAlpha(alpha);

        auto attributedString = AttributedString();
        attributedString.setJustification(getJustificationType());
        attributedString.append(getText(), font, colour);
        attributedString.draw(g, textArea.toFloat());

        g.setColour(findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
    } else if (isEnabled()) {
        g.setColour(findColour(Label::outlineColourId));
    }

    g.drawRect(getLocalBounds());
}

void UnicodeLabel::mouseUp (const MouseEvent& e)
{
    if (editSingleClick
         && isEnabled()
         && contains (e.getPosition())
         && ! (e.mouseWasDraggedSinceMouseDown() || e.mods.isPopupMenu()))
    {
        showEditor();
    }
}

void UnicodeLabel::mouseDoubleClick (const MouseEvent& e)
{
    if (editDoubleClick
         && isEnabled()
         && ! e.mods.isPopupMenu())
    {
        showEditor();
    }
}

void UnicodeLabel::resized()
{
    if (editor != nullptr)
        editor->setBounds (getLocalBounds());
}

void UnicodeLabel::focusGained (FocusChangeType cause)
{
    if (editSingleClick
         && isEnabled()
         && cause == focusChangedByTabKey)
    {
        showEditor();
    }
}

void UnicodeLabel::enablementChanged()
{
    repaint();
}

void UnicodeLabel::colourChanged()
{
    repaint();
}

void UnicodeLabel::setMinimumHorizontalScale (const float newScale)
{
    if (minimumHorizontalScale != newScale)
    {
        minimumHorizontalScale = newScale;
        repaint();
    }
}

//==============================================================================
// We'll use a custom focus traverser here to make sure focus goes from the
// text editor to another component rather than back to the label itself.
class UnicodeLabelKeyboardFocusTraverser   : public KeyboardFocusTraverser
{
public:
    explicit UnicodeLabelKeyboardFocusTraverser (UnicodeLabel& l)  : owner (l)  {}

    Component* getDefaultComponent (Component* parent) override
    {
        if (auto* container = getKeyboardFocusContainer (parent))
            return KeyboardFocusTraverser::getDefaultComponent (container);

        return nullptr;
    }

    Component* getNextComponent     (Component* c) override  { return KeyboardFocusTraverser::getNextComponent     (getComp (c)); }
    Component* getPreviousComponent (Component* c) override  { return KeyboardFocusTraverser::getPreviousComponent (getComp (c)); }

    std::vector<Component*> getAllComponents (Component* parent) override
    {
        if (auto* container = getKeyboardFocusContainer (parent))
            return KeyboardFocusTraverser::getAllComponents (container);

        return {};
    }

private:
    Component* getComp (Component* current) const
    {
        if (auto* ed = owner.getCurrentTextEditor())
            if (current == ed)
                return current->getParentComponent();

        return current;
    }

    Component* getKeyboardFocusContainer (Component* parent) const
    {
        if (owner.getCurrentTextEditor() != nullptr && parent == &owner)
            return owner.findKeyboardFocusContainer();

        return parent;
    }

    UnicodeLabel& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnicodeLabelKeyboardFocusTraverser)
};

std::unique_ptr<ComponentTraverser> UnicodeLabel::createKeyboardFocusTraverser()
{
    return std::make_unique<UnicodeLabelKeyboardFocusTraverser> (*this);
}

//==============================================================================
void UnicodeLabel::addListener    (UnicodeLabel::Listener* l)     { listeners.add (l); }
void UnicodeLabel::removeListener (UnicodeLabel::Listener* l)     { listeners.remove (l); }

void UnicodeLabel::callChangeListeners()
{
    Component::BailOutChecker checker (this);
    listeners.callChecked (checker, [this] (Listener& l) { l.labelTextChanged (this); });

    if (checker.shouldBailOut())
        return;

    if (onTextChange != nullptr)
        onTextChange();
}

//==============================================================================
void UnicodeLabel::textEditorTextChanged (UnicodeTextEditor& ed)
{
    if (editor != nullptr)
    {
        jassert (&ed == editor.get());

        if (! (hasKeyboardFocus (true) || isCurrentlyBlockedByAnotherModalComponent()))
        {
            if (lossOfFocusDiscardsChanges)
                textEditorEscapeKeyPressed (ed);
            else
                textEditorReturnKeyPressed (ed);
        }
    }
}

void UnicodeLabel::textEditorReturnKeyPressed (UnicodeTextEditor& ed)
{
    if (editor != nullptr)
    {
        jassert (&ed == editor.get());

        WeakReference<Component> deletionChecker (this);
        bool changed = updateFromTextEditorContents (ed);
        hideEditor (true);

        if (changed && deletionChecker != nullptr)
        {
            textWasEdited();

            if (deletionChecker != nullptr)
                callChangeListeners();
        }
    }
}

void UnicodeLabel::textEditorEscapeKeyPressed (UnicodeTextEditor& ed)
{
    if (editor != nullptr)
    {
        jassertquiet (&ed == editor.get());

        editor->setText (textValue.toString(), false);
        hideEditor (true);
    }
}

void UnicodeLabel::textEditorFocusLost (UnicodeTextEditor& ed)
{
    textEditorTextChanged (ed);
}

//==============================================================================
class UnicodeLabelAccessibilityHandler  : public AccessibilityHandler
{
public:
    explicit UnicodeLabelAccessibilityHandler (UnicodeLabel& labelToWrap)
        : AccessibilityHandler (labelToWrap,
                                labelToWrap.isEditable() ? AccessibilityRole::editableText : AccessibilityRole::label,
                                getAccessibilityActions (labelToWrap),
                                { std::make_unique<UnicodeLabelValueInterface> (labelToWrap) }),
          label (labelToWrap)
    {
    }

    String getTitle() const override  { return label.getText(); }
    String getHelp() const override   { return label.getTooltip(); }

    AccessibleState getCurrentState() const override
    {
        if (label.isBeingEdited())
            return {}; // allow focus to pass through to the TextEditor

        return AccessibilityHandler::getCurrentState();
    }

private:
    class UnicodeLabelValueInterface  : public AccessibilityTextValueInterface
    {
    public:
        explicit UnicodeLabelValueInterface (UnicodeLabel& labelToWrap)
            : label (labelToWrap)
        {
        }

        bool isReadOnly() const override                 { return true; }
        String getCurrentValueAsString() const override  { return label.getText(); }
        void setValueAsString (const String&) override   {}

    private:
        UnicodeLabel& label;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnicodeLabelValueInterface)
    };

    static AccessibilityActions getAccessibilityActions (UnicodeLabel& label)
    {
        if (label.isEditable())
            return AccessibilityActions().addAction (AccessibilityActionType::press, [&label] { label.showEditor(); });

        return {};
    }

    UnicodeLabel& label;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnicodeLabelAccessibilityHandler)
};

std::unique_ptr<AccessibilityHandler> UnicodeLabel::createAccessibilityHandler()
{
    return std::make_unique<UnicodeLabelAccessibilityHandler> (*this);
}
