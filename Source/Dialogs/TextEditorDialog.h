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

struct LuaTokeniserFunctions {
    static bool isIdentifierStart(juce_wchar const c) noexcept
    {
        return CharacterFunctions::isLetter(c)
            || c == '_' || c == '@';
    }

    static bool isIdentifierBody(juce_wchar const c) noexcept
    {
        return CharacterFunctions::isLetterOrDigit(c)
            || c == '_' || c == '@';
    }

    static bool isReservedKeyword(String::CharPointerType token, int const tokenLength) noexcept
    {
        static constexpr char const* const keywords2Char[] = { "do", "if", "or", "in", nullptr };

        static constexpr char const* const keywords3Char[] = { "and", "end", "for", "nil", "not", "try", nullptr };

        static constexpr char const* const keywords4Char[] = { "else", "goto", "then", "true", "else", "self", nullptr };

        static constexpr char const* const keywords5Char[] = { "break", "false", "local", "until", "while", "error", nullptr };

        static constexpr char const* const keywords6Char[] = { "return", "repeat", "elseif", "assert", nullptr };

        static constexpr char const* const keywords8Char[] = { "function", nullptr };

        static constexpr char const* const keywordsOther[] = { "collectgarbage", "dofile", "getmetatable", "ipairs",
            "loadfile", "loadstring", "next", "pairs", "pcall", "print", "rawequal",
            "rawget", "rawset", "require", "select", "setmetatable", "tonumber",
            "tostring", "type", "xpcall", nullptr };

        char const* const* k;

        switch (tokenLength) {
        case 2:
            k = keywords2Char;
            break;
        case 3:
            k = keywords3Char;
            break;
        case 4:
            k = keywords4Char;
            break;
        case 5:
            k = keywords5Char;
            break;
        case 6:
            k = keywords6Char;
            break;
        case 8:
            k = keywords8Char;
            break;

        default:
            if (tokenLength < 2 || tokenLength > 16)
                return false;

            k = keywordsOther;
            break;
        }

        for (int i = 0; k[i] != nullptr; ++i)
            if (token.compare(CharPointer_ASCII(k[i])) == 0)
                return true;

        return false;
    }

    template<typename Iterator>
    static int parseIdentifier(Iterator& source) noexcept
    {
        int tokenLength = 0;
        String::CharPointerType::CharType possibleIdentifier[100] = {};
        String::CharPointerType possible(possibleIdentifier);

        while (isIdentifierBody(source.peekNextChar())) {
            auto c = source.nextChar();

            if (tokenLength < 20)
                possible.write(c);

            ++tokenLength;
        }

        if (tokenLength > 1 && tokenLength <= 16) {
            possible.writeNull();

            if (isReservedKeyword(String::CharPointerType(possibleIdentifier), tokenLength))
                return LuaTokeniser::tokenType_keyword;
        }

        return LuaTokeniser::tokenType_identifier;
    }

    template<typename Iterator>
    static bool skipNumberSuffix(Iterator& source)
    {
        auto c = source.peekNextChar();

        if (c == 'l' || c == 'L' || c == 'u' || c == 'U')
            source.skip();

        if (CharacterFunctions::isLetterOrDigit(source.peekNextChar()))
            return false;

        return true;
    }

    static bool isHexDigit(juce_wchar const c) noexcept
    {
        return (c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F');
    }

    template<typename Iterator>
    static bool parseHexLiteral(Iterator& source) noexcept
    {
        if (source.peekNextChar() == '-')
            source.skip();

        if (source.nextChar() != '0')
            return false;

        auto c = source.nextChar();

        if (c != 'x' && c != 'X')
            return false;

        int numDigits = 0;

        while (isHexDigit(source.peekNextChar())) {
            ++numDigits;
            source.skip();
        }

        if (numDigits == 0)
            return false;

        return skipNumberSuffix(source);
    }

    static bool isOctalDigit(juce_wchar const c) noexcept
    {
        return c >= '0' && c <= '7';
    }

    template<typename Iterator>
    static bool parseOctalLiteral(Iterator& source) noexcept
    {
        if (source.peekNextChar() == '-')
            source.skip();

        if (source.nextChar() != '0')
            return false;

        if (!isOctalDigit(source.nextChar()))
            return false;

        while (isOctalDigit(source.peekNextChar()))
            source.skip();

        return skipNumberSuffix(source);
    }

    static bool isDecimalDigit(juce_wchar const c) noexcept
    {
        return c >= '0' && c <= '9';
    }

    template<typename Iterator>
    static bool parseDecimalLiteral(Iterator& source) noexcept
    {
        if (source.peekNextChar() == '-')
            source.skip();

        int numChars = 0;
        while (isDecimalDigit(source.peekNextChar())) {
            ++numChars;
            source.skip();
        }

        if (numChars == 0)
            return false;

        return skipNumberSuffix(source);
    }

    template<typename Iterator>
    static bool parseFloatLiteral(Iterator& source) noexcept
    {
        if (source.peekNextChar() == '-')
            source.skip();

        int numDigits = 0;

        while (isDecimalDigit(source.peekNextChar())) {
            source.skip();
            ++numDigits;
        }

        bool const hasPoint = source.peekNextChar() == '.';

        if (hasPoint) {
            source.skip();

            while (isDecimalDigit(source.peekNextChar())) {
                source.skip();
                ++numDigits;
            }
        }

        if (numDigits == 0)
            return false;

        auto c = source.peekNextChar();
        bool const hasExponent = c == 'e' || c == 'E';

        if (hasExponent) {
            source.skip();
            c = source.peekNextChar();

            if (c == '+' || c == '-')
                source.skip();

            int numExpDigits = 0;

            while (isDecimalDigit(source.peekNextChar())) {
                source.skip();
                ++numExpDigits;
            }

            if (numExpDigits == 0)
                return false;
        }

        c = source.peekNextChar();

        if (c == 'f' || c == 'F')
            source.skip();
        else if (!(hasExponent || hasPoint))
            return false;

        return true;
    }

    template<typename Iterator>
    static int parseNumber(Iterator& source)
    {
        Iterator const original(source);

        if (parseFloatLiteral(source))
            return LuaTokeniser::tokenType_float;
        source = original;

        if (parseHexLiteral(source))
            return LuaTokeniser::tokenType_integer;
        source = original;

        if (parseOctalLiteral(source))
            return LuaTokeniser::tokenType_integer;
        source = original;

        if (parseDecimalLiteral(source))
            return LuaTokeniser::tokenType_integer;
        source = original;

        return LuaTokeniser::tokenType_error;
    }

    template<typename Iterator>
    static void skipQuotedString(Iterator& source) noexcept
    {
        auto quote = source.nextChar();

        for (;;) {
            auto c = source.nextChar();

            if (c == quote || c == 0)
                break;

            if (c == '\\')
                source.skip();
        }
    }

    template<typename Iterator>
    static void skipComment(Iterator& source) noexcept
    {
        source.skip(); // Consume the '['
        while (auto c = source.nextChar()) {
            if (c == ']') {
                if (source.peekNextChar() == ']') {
                    source.nextChar(); // Consume the closing ']'
                    break;
                }
            }
        }
    }

    template<typename Iterator>
    static void skipIfNextCharMatches(Iterator& source, juce_wchar const c) noexcept
    {
        if (source.peekNextChar() == c)
            source.skip();
    }

    template<typename Iterator>
    static void skipIfNextCharMatches(Iterator& source, juce_wchar const c1, juce_wchar const c2) noexcept
    {
        auto c = source.peekNextChar();

        if (c == c1 || c == c2)
            source.skip();
    }

    template<typename Iterator>
    static int readNextToken(Iterator& source)
    {
        source.skipWhitespace();
        auto firstChar = source.peekNextChar();

        switch (firstChar) {
        case 0:
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.': {
            auto result = parseNumber(source);

            if (result == LuaTokeniser::tokenType_error) {
                source.skip();

                if (firstChar == '.')
                    return LuaTokeniser::tokenType_punctuation;
            }

            return result;
        }

        case ',':
        case ';':
        case ':':
            source.skip();
            return LuaTokeniser::tokenType_punctuation;

        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
            source.skip();
            return LuaTokeniser::tokenType_bracket;

        case '"':
        case '\'':
            skipQuotedString(source);
            return LuaTokeniser::tokenType_string;

        case '+':
            source.skip();
            skipIfNextCharMatches(source, '=');
            return LuaTokeniser::tokenType_operator;

        case '-': {
            source.skip();
            auto nextChar = source.peekNextChar();

            if (nextChar == '-') {
                source.skip();
                auto nextChar = source.peekNextChar();

                if (nextChar == '=') {
                    source.skip();
                } else if (nextChar == '[') {
                    source.skip();
                    skipComment(source);
                    return LuaTokeniser::tokenType_comment;
                } else {
                    source.skipToEndOfLine();
                    return LuaTokeniser::tokenType_comment;
                }

                return LuaTokeniser::tokenType_operator;
            }
            auto result = parseNumber(source);

            if (result == LuaTokeniser::tokenType_error) {
                skipIfNextCharMatches(source, '=');
                return LuaTokeniser::tokenType_operator;
            }
            return result;
        }

        case '*':
        case '%':
        case '=':
        case '~':
            source.skip();
            skipIfNextCharMatches(source, '=');
            return LuaTokeniser::tokenType_operator;
        case '?':
            source.skip();
            return LuaTokeniser::tokenType_operator;

        case '<':
        case '>':
        case '|':
        case '&':
        case '^':
            source.skip();
            skipIfNextCharMatches(source, firstChar);
            skipIfNextCharMatches(source, '=');
            return LuaTokeniser::tokenType_operator;

        default:
            if (isIdentifierStart(firstChar))
                return parseIdentifier(source);

            source.skip();
            break;
        }

        return LuaTokeniser::tokenType_error;
    }

    struct StringIterator {
        explicit StringIterator(String const& s) noexcept
            : t(s.getCharPointer())
        {
        }
        explicit StringIterator(String::CharPointerType s) noexcept
            : t(s)
        {
        }

        juce_wchar nextChar() noexcept
        {
            if (isEOF())
                return 0;
            ++numChars;
            return t.getAndAdvance();
        }
        juce_wchar peekNextChar() const noexcept { return *t; }
        void skip() noexcept
        {
            if (!isEOF()) {
                ++t;
                ++numChars;
            }
        }
        void skipWhitespace() noexcept
        {
            while (t.isWhitespace())
                skip();
        }
        void skipToEndOfLine() noexcept
        {
            while (*t != '\r' && *t != '\n' && *t != 0)
                skip();
        }
        bool isEOF() const noexcept { return t.isEmpty(); }

        String::CharPointerType t;
        int numChars = 0;
    };

    //==============================================================================
    /** Takes a UTF8 string and writes it to a stream using standard C++ escape sequences for any
        non-ascii bytes.

        Although not strictly a tokenising function, this is still a function that often comes in
        handy when working with C++ code!

        Note that addEscapeChars() is easier to use than this function if you're working with Strings.

        @see addEscapeChars
    */
    static void writeEscapeChars(OutputStream& out, char const* utf8, int const numBytesToRead,
        int const maxCharsOnLine, bool const breakAtNewLines,
        bool const replaceSingleQuotes, bool const allowStringBreaks)
    {
        int charsOnLine = 0;
        bool lastWasHexEscapeCode = false;
        bool trigraphDetected = false;

        for (int i = 0; i < numBytesToRead || numBytesToRead < 0; ++i) {
            auto const c = static_cast<unsigned char>(utf8[i]);
            bool startNewLine = false;

            switch (c) {

            case '\t':
                out << "\\t";
                trigraphDetected = false;
                lastWasHexEscapeCode = false;
                charsOnLine += 2;
                break;
            case '\r':
                out << "\\r";
                trigraphDetected = false;
                lastWasHexEscapeCode = false;
                charsOnLine += 2;
                break;
            case '\n':
                out << "\\n";
                trigraphDetected = false;
                lastWasHexEscapeCode = false;
                charsOnLine += 2;
                startNewLine = breakAtNewLines;
                break;
            case '\\':
                out << "\\\\";
                trigraphDetected = false;
                lastWasHexEscapeCode = false;
                charsOnLine += 2;
                break;
            case '\"':
                out << "\\\"";
                trigraphDetected = false;
                lastWasHexEscapeCode = false;
                charsOnLine += 2;
                break;

            case '?':
                if (trigraphDetected) {
                    out << "\\?";
                    charsOnLine++;
                    trigraphDetected = false;
                } else {
                    out << "?";
                    trigraphDetected = true;
                }

                lastWasHexEscapeCode = false;
                charsOnLine++;
                break;

            case 0:
                if (numBytesToRead < 0)
                    return;

                out << "\\0";
                lastWasHexEscapeCode = true;
                trigraphDetected = false;
                charsOnLine += 2;
                break;

            case '\'':
                if (replaceSingleQuotes) {
                    out << "\\\'";
                    lastWasHexEscapeCode = false;
                    trigraphDetected = false;
                    charsOnLine += 2;
                    break;
                }
                // deliberate fall-through...
                JUCE_FALLTHROUGH

            default:
                if (c >= 32 && c < 127 && !(lastWasHexEscapeCode // (have to avoid following a hex escape sequence with a valid hex digit)
                        && CharacterFunctions::getHexDigitValue(c) >= 0)) {
                    out << static_cast<char>(c);
                    lastWasHexEscapeCode = false;
                    trigraphDetected = false;
                    ++charsOnLine;
                } else if (allowStringBreaks && lastWasHexEscapeCode && c >= 32 && c < 127) {
                    out << "\"\"" << static_cast<char>(c);
                    lastWasHexEscapeCode = false;
                    trigraphDetected = false;
                    charsOnLine += 3;
                } else {
                    out << (c < 16 ? "\\x0" : "\\x") << String::toHexString(static_cast<int>(c));
                    lastWasHexEscapeCode = true;
                    trigraphDetected = false;
                    charsOnLine += 4;
                }

                break;
            }

            if ((startNewLine || (maxCharsOnLine > 0 && charsOnLine >= maxCharsOnLine))
                && (numBytesToRead < 0 || i < numBytesToRead - 1)) {
                charsOnLine = 0;
                out << "\"" << newLine << "\"";
                lastWasHexEscapeCode = false;
            }
        }
    }

    /** Takes a string and returns a version of it where standard C++ escape sequences have been
        used to replace any non-ascii bytes.

        Although not strictly a tokenising function, this is still a function that often comes in
        handy when working with C++ code!

        @see writeEscapeChars
    */
    static String addEscapeChars(String const& s)
    {
        MemoryOutputStream mo;
        writeEscapeChars(mo, s.toRawUTF8(), -1, -1, false, true, true);
        return mo.toString();
    }
};

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
            return map[argument];
        }
        map[argument] = f(argument);
        return this->operator()(argument);
    }
    FunctionType f;
    mutable UnorderedMap<ArgType, DataType> map;
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
    Selection(int const r0, int const c0, int const r1, int const c1)
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
    bool intersectsRow(int const row) const
    {
        return isOriented()
            ? head.x <= row && row <= tail.x
            : head.x >= row && row >= tail.x;
    }

    /** Return the range of columns this selection covers on the given row.
     */
    Range<int> getColumnRangeOnRow(int const row, int numColumns) const
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

    Selection withStyle(int const token) const
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
    void insert(int const index, String const& string) { lines.insert(index, string); }
    void removeRange(int const startIndex, int const numberToRemove) { lines.remove_range(startIndex, startIndex + numberToRemove); }
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
        SmallArray<int> tokens;
        bool glyphsAreDirty = true;
        bool tokensAreDirty = true;
    };
    mutable SmallArray<Entry> lines;
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
            auto const s = t;
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
        juce_wchar get() const { return document->getCharacter(index); }
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
    void setSelections(SmallArray<Selection> const& newSelections) { selections = newSelections; }

    /** Replace the selection at the given index. The index must be in range. */
    void setSelection(int const index, Selection newSelection) { selections[index] = newSelection; }

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
    SmallArray<Rectangle<float>> getSelectionRegion(Selection selection,
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
    SmallArray<RowData> findRowsIntersecting(Rectangle<float> area,
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

    void search(String const& text);

    /** Return the character at the given index. */
    juce_wchar getCharacter(Point<int> index) const;

    /** Add a selection to the list. */
    void addSelection(Selection selection) { selections.add(selection); }

    /** Return the number of active selections. */
    int getNumSelections() const { return selections.size(); }

    /** Return a line in the document. */
    String const& getLine(int const lineIndex) const { return lines[lineIndex]; }

    /** Return one of the current selections. */
    Selection const& getSelection(int index) const;

    /** Return the current selection state. */
    SmallArray<Selection> const& getSelections() const;

    /** Return the current selection state. */
    SmallArray<Selection> const& getSearchSelections() const;

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
    void applyTokens(Range<int> rows, SmallArray<Selection> const& zones);

    int searchNext()
    {
        currentSearchSelection++;
        currentSearchSelection %= searchSelections.size();
        return currentSearchSelection;
    }

private:
    friend class PlugDataTextEditor;

    float lineSpacing = 1.25f;
    mutable Rectangle<float> cachedBounds;
    GlyphArrangementArray lines;
    Font font;
    SmallArray<Selection> selections;
    SmallArray<Selection> searchSelections;
    int currentSearchSelection = 0;
};

class Caret final : public Component
    , private Timer {
public:
    explicit Caret(TextDocument const& document);
    void setViewTransform(AffineTransform const& transformToUse);
    void updateSelections();

    void paint(Graphics& g) override;

private:
    static float squareWave(float wt);
    void timerCallback() override;
    SmallArray<Rectangle<float>> getCaretRectangles() const;

    float phase = 0.f;
    TextDocument const& document;
    AffineTransform transform;
};

class GutterComponent final : public Component {
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

class HighlightComponent final : public Component {
public:
    explicit HighlightComponent(TextDocument const& document);
    void setViewTransform(AffineTransform const& transformToUse, SmallArray<Selection> const& selections);
    void updateSelections(SmallArray<Selection> const& selections);

    void setHighlightColour(Colour const c) { highlightColour = c; }

    void paint(Graphics& g) override;

private:
    static Path getOutlinePath(SmallArray<Rectangle<float>> const& rectangles);

    TextDocument const& document;
    AffineTransform transform;
    Path outlinePath;
    Colour highlightColour;
};

class PlugDataTextEditor final : public Component
    , public Timer {
public:
    enum class RenderScheme {
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
    void mouseUp(MouseEvent const& e) override;
    void mouseDoubleClick(MouseEvent const& e) override;
    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d) override;
    void mouseMagnify(MouseEvent const& e, float scaleFactor) override;
    void mouseMove(MouseEvent const& e) override;

    void lookAndFeelChanged() override;

    void timerCallback() override;

    bool keyPressed(KeyPress const& key) override;
    MouseCursor getMouseCursor() override;

    Rectangle<float> getScrollBarBounds() const;

    CodeEditorComponent::ColourScheme getSyntaxColourScheme();

    bool hasChanged() const { return changed; }
    void setUnchanged() { changed = false; }

    void performUndo() { undo.undo(); }
    void performRedo() { undo.redo(); }

    bool canUndo() const { return undo.canUndo(); }
    bool canRedo() const { return undo.canRedo(); }
    void setUndoChangeListener(ChangeListener* listener)
    {
        undo.addChangeListener(listener);
        undo.sendSynchronousChangeMessage();
    }

    void setEnableSyntaxHighlighting(bool const enable)
    {
        enableSyntaxHighlighting = enable;
        repaint();
    }

    void setSearchText(String const& searchText);
    void searchNext();

private:
    bool insert(String const& content);
    void updateViewTransform();
    void updateSelections();
    void translateToEnsureCaretIsVisible();
    void translateToEnsureSearchIsVisible(int idx);

    void renderTextUsingAttributedString(Graphics& g);
    void renderTextUsingGlyphArrangement(Graphics& g);

    bool enableSyntaxHighlighting = false;
    bool allowCoreGraphics = true;

    RenderScheme renderScheme = RenderScheme::usingAttributedString;

    double lastTransactionTime;
    bool tabKeyUsed = true;
    bool changed = false;
    TextDocument document;
    Caret caret;
    GutterComponent gutter;
    HighlightComponent highlight;
    HighlightComponent searchHighlight;

    float viewScaleFactor = 1.f;
    float mouseDownViewPosition;
    float scrollbarFadePosition = 0.0f;
    bool isOverScrollBar = false;
    bool scrollBarClicked = false;
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

float Caret::squareWave(float const wt)
{
    constexpr float delta = 0.222f;
    constexpr float A = 1.0;
    return 0.5f + A / 3.14159f * std::atan(std::cos(wt) / delta);
}

void Caret::timerCallback()
{
    phase += 3.2e-1;

    for (auto const& r : getCaretRectangles())
        repaint(r.getSmallestIntegerContainer());
}

SmallArray<Rectangle<float>> Caret::getCaretRectangles() const
{
    SmallArray<Rectangle<float>> rectangles;

    for (auto const& selection : document.getSelections()) {
        if (selection.head == selection.tail) {
            rectangles.add(document
                    .getGlyphBounds(selection.head)
                    .removeFromLeft(CURSOR_WIDTH)
                    .translated(selection.head.y == 0 ? 0 : -0.5f * CURSOR_WIDTH, 0.f)
                    .transformedBy(transform)
                    .expanded(0.f, 1.f));
        }
    }
    return rectangles;
}

GutterComponent::GutterComponent(TextDocument const& document)
    : document(document)
    , memoizedGlyphArrangements([this](int const row) { return getLineNumberGlyphs(row); })
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
    auto const ln = getParentComponent()->findColour(PlugDataColour::sidebarBackgroundColourId);

    g.setColour(ln);
    g.fillRect(getLocalBounds().removeFromLeft(GUTTER_WIDTH));

    if (transform.getTranslationX() < GUTTER_WIDTH) {
        auto const shadowRect = getLocalBounds().withLeft(GUTTER_WIDTH).withWidth(12);

        auto const gradient = ColourGradient::horizontal(ln.contrasting().withAlpha(0.3f),
            Colours::transparentBlack, shadowRect);
        g.setFillType(gradient);
        g.fillRect(shadowRect);
    } else {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawVerticalLine(GUTTER_WIDTH - 1.f, 0.f, getHeight());
    }

    /*
     Draw the line numbers and selected rows
     ------------------------------------------------------------------
     */
    auto const area = g.getClipBounds().toFloat().transformedBy(transform.inverted());
    auto rowData = document.findRowsIntersecting(area);
    auto const verticalTransform = transform.withAbsoluteTranslation(0.f, transform.getTranslationY());

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

GlyphArrangement GutterComponent::getLineNumberGlyphs(int const row) const
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

void HighlightComponent::setViewTransform(AffineTransform const& transformToUse, SmallArray<Selection> const& selections)
{
    transform = transformToUse;

    outlinePath.clear();
    auto const clip = getLocalBounds().toFloat().transformedBy(transform.inverted());

    for (auto const& s : selections) {
        outlinePath.addPath(getOutlinePath(document.getSelectionRegion(s, clip)));
    }
    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::updateSelections(SmallArray<Selection> const& selections)
{
    outlinePath.clear();
    auto const clip = getLocalBounds().toFloat().transformedBy(transform.inverted());

    for (auto const& s : selections) {
        outlinePath.addPath(getOutlinePath(document.getSelectionRegion(s, clip)));
    }

    repaint(outlinePath.getBounds().getSmallestIntegerContainer());
}

void HighlightComponent::paint(Graphics& g)
{
    g.addTransform(transform);

    g.setColour(highlightColour);
    g.fillPath(outlinePath);

    g.setColour(highlightColour.darker());
    g.strokePath(outlinePath, PathStrokeType(1.f));
}

Path HighlightComponent::getOutlinePath(SmallArray<Rectangle<float>> const& rectangles)
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
    }
    return Selection(content).startingFrom(tail).swapped();
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

String const& GlyphArrangementArray::operator[](int const index) const
{
    if (isPositiveAndBelow(index, lines.size())) {
        return lines[index].string;
    }

    static String empty;
    return empty;
}

int GlyphArrangementArray::getToken(int const row, int const col, int const defaultIfOutOfBounds) const
{
    if (!isPositiveAndBelow(row, lines.size())) {
        return defaultIfOutOfBounds;
    }
    return lines[row].tokens[col];
}

void GlyphArrangementArray::clearTokens(int const index)
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines[index];

    ensureValid(index);

    for (int col = 0; col < entry.tokens.size(); ++col) {
        entry.tokens[col] = 0;
    }
}

void GlyphArrangementArray::applyTokens(int const index, Selection zone)
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines[index];
    auto const range = zone.getColumnRangeOnRow(index, entry.tokens.size());

    ensureValid(index);

    for (int col = range.getStart(); col < range.getEnd(); ++col) {
        entry.tokens[col] = zone.token;
    }
}

GlyphArrangement GlyphArrangementArray::getGlyphs(int const index,
    float const baseline,
    int const token,
    bool const withTrailingSpace) const
{
    if (!isPositiveAndBelow(index, lines.size())) {
        GlyphArrangement glyphs;

        if (withTrailingSpace) {
            glyphs.addLineOfText(font, " ", TEXT_INDENT, baseline);
        }
        return glyphs;
    }
    ensureValid(index);

    auto& entry = lines[index];
    auto glyphSource = withTrailingSpace ? entry.glyphsWithTrailingSpace : entry.glyphs;
    auto glyphs = GlyphArrangement();

    for (int n = 0; n < glyphSource.getNumGlyphs(); ++n) {
        if (token == -1 || entry.tokens[n] == token) {
            auto glyph = glyphSource.getGlyph(n);
            glyph.moveBy(TEXT_INDENT, baseline);
            glyphs.addGlyph(glyph);
        }
    }
    return glyphs;
}

void GlyphArrangementArray::ensureValid(int const index) const
{
    if (!isPositiveAndBelow(index, lines.size()))
        return;

    auto& entry = lines[index];

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

int TextDocument::getNumColumns(int const row) const
{
    return lines[row].length();
}

float TextDocument::getVerticalPosition(int const row, Metric const metric) const
{
    float const lineHeight = font.getHeight() * lineSpacing;
    float const gap = font.getHeight() * (lineSpacing - 1.f) * 0.5f;

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

Point<float> TextDocument::getPosition(Point<int> index, Metric const metric) const
{
    return { getGlyphBounds(index).getX(), getVerticalPosition(index.x, metric) };
}

SmallArray<Rectangle<float>> TextDocument::getSelectionRegion(Selection selection, Rectangle<float> clip) const
{
    SmallArray<Rectangle<float>> patches;
    Selection const s = selection.oriented();

    if (s.head.x == s.tail.x) {
        int const c0 = s.head.y;
        int const c1 = s.tail.y;
        patches.add(getBoundsOnRow(s.head.x, Range<int>(c0, c1)));
    } else {
        int const r0 = s.head.x;
        int const c0 = s.head.y;
        int const r1 = s.tail.x;
        int const c1 = s.tail.y;

        for (int n = r0; n <= r1; ++n) {
            if (!clip.isEmpty() && !clip.getVerticalRange().intersects({ getVerticalPosition(n, Metric::top), getVerticalPosition(n, Metric::bottom) }))
                continue;

            if (n == r1 && c1 == 0)
                continue;
            if (n == r0)
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

Rectangle<float> TextDocument::getBoundsOnRow(int const row, Range<int> columns) const
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

GlyphArrangement TextDocument::getGlyphsForRow(int const row, int const token, bool const withTrailingSpace) const
{
    return lines.getGlyphs(row,
        getVerticalPosition(row, Metric::baseline),
        token,
        withTrailingSpace);
}

GlyphArrangement TextDocument::findGlyphsIntersecting(Rectangle<float> area, int const token) const
{
    auto const range = getRangeOfRowsIntersecting(area);
    auto rows = SmallArray<RowData>();
    auto glyphs = GlyphArrangement();

    for (int n = range.getStart(); n < range.getEnd(); ++n) {
        glyphs.addGlyphArrangement(getGlyphsForRow(n, token));
    }
    return glyphs;
}

Range<int> TextDocument::getRangeOfRowsIntersecting(Rectangle<float> area) const
{
    auto const lineHeight = font.getHeight() * lineSpacing;
    auto row0 = jlimit(0, jmax(getNumRows() - 1, 0), static_cast<int>(area.getY() / lineHeight));
    auto const row1 = jlimit(0, jmax(getNumRows() - 1, 0), static_cast<int>(area.getBottom() / lineHeight));
    return { row0, row1 + 1 };
}

SmallArray<TextDocument::RowData> TextDocument::findRowsIntersecting(Rectangle<float> area,
    bool const computeHorizontalExtent) const
{
    auto const range = getRangeOfRowsIntersecting(area);
    auto rows = SmallArray<RowData>();

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
    auto const lineHeight = font.getHeight() * lineSpacing;
    auto row = jlimit(0, jmax(getNumRows() - 1, 0), static_cast<int>(position.y / lineHeight));
    auto col = 0;
    auto const glyphs = getGlyphsForRow(row);

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
    }
    if (index.x < getNumRows()) {
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
    }
    if (index.x > 0) {
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

void TextDocument::navigate(Point<int>& i, Target const target, Direction const direction) const
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

void TextDocument::navigateSelections(Target const target, Direction const direction, Selection::Part part)
{
    auto isHeadBeforeTail = [](Point<int> head, Point<int> tail) -> int {
        if (head.x == tail.x)
            return head.y == tail.y ? -1 : head.y < tail.y;
        return head.x < tail.x;
    };

    for (auto& selection : selections) {
        if (target == Target::character && ((isHeadBeforeTail(selection.head, selection.tail) == 1 && direction == Direction::forwardCol) || (isHeadBeforeTail(selection.head, selection.tail) == 0 && direction == Direction::backwardCol))) {
            selection.head = selection.tail;
            continue;
        }
        if (target == Target::character && ((isHeadBeforeTail(selection.head, selection.tail) == 0 && direction == Direction::forwardCol) || (isHeadBeforeTail(selection.head, selection.tail) == 1 && direction == Direction::backwardCol))) {
            selection.tail = selection.head;
            continue;
        }
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

void TextDocument::search(String const& text)
{
    selections.clear();
    searchSelections.clear();

    for (int i = 0; i < lines.size(); i++) {
        auto const idx = lines[i].indexOf(text);
        if (idx >= 0) {
            searchSelections.add(Selection(Point<int>(i, idx), Point<int>(i, idx + text.length())));
        }
    }
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

Selection const& TextDocument::getSelection(int const index) const
{
    return selections[index];
}

SmallArray<Selection> const& TextDocument::getSelections() const
{
    return selections;
}

SmallArray<Selection> const& TextDocument::getSearchSelections() const
{
    return searchSelections;
}

String TextDocument::getSelectionContent(Selection s) const
{
    s = s.oriented();

    if (s.isSingleLine()) {
        return lines[s.head.x].substring(s.head.y, s.tail.y);
    }
    String content = lines[s.head.x].substring(s.head.y) + "\n";

    for (int row = s.head.x + 1; row < s.tail.x; ++row) {
        content += lines[row] + "\n";
    }
    content += lines[s.tail.x].substring(0, s.tail.y);
    return content;
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
    constexpr auto inf = std::numeric_limits<float>::max();

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

void TextDocument::applyTokens(Range<int> rows, SmallArray<Selection> const& zones)
{
    for (int n = rows.getStart(); n < rows.getEnd(); ++n) {
        for (auto const& zone : zones) {
            if (zone.intersectsRow(n)) {
                lines.applyTokens(n, zone);
            }
        }
    }
}

class Transaction::Undoable final : public UndoableAction {
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
    , searchHighlight(document)
{
    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections({ Selection() });

    setFont(Font(Fonts::getMonospaceFont().withHeight(15.5f)));

    translateView(GUTTER_WIDTH, 0);
    setWantsKeyboardFocus(true);

    addAndMakeVisible(highlight);
    addAndMakeVisible(searchHighlight);
    addAndMakeVisible(caret);
    addAndMakeVisible(gutter);

    lookAndFeelChanged();
}

void PlugDataTextEditor::lookAndFeelChanged()
{
    highlight.setHighlightColour(findColour(CodeEditorComponent::highlightColourId));
    searchHighlight.setHighlightColour(Colours::yellow.withAlpha(0.5f));
}

void PlugDataTextEditor::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
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

void PlugDataTextEditor::translateView(float const dx, float const dy)
{
    auto const W = viewScaleFactor * document.getBounds().getWidth();
    auto const H = viewScaleFactor * document.getBounds().getHeight();

    translation.x = jlimit(jmin(GUTTER_WIDTH, -W + getWidth()), GUTTER_WIDTH, translation.x + dx);
    translation.y = jlimit(jmin(-0.f, -H + (getHeight() - 10)), 0.0f, translation.y + dy);

    updateViewTransform();
}

void PlugDataTextEditor::scaleView(float const scaleFactorMultiplier, float const verticalCenter)
{
    auto const newS = viewScaleFactor * scaleFactorMultiplier;
    auto const fixedy = Point<float>(0, verticalCenter).transformedBy(transform.inverted()).y;

    translation.y = -newS * fixedy + verticalCenter;
    viewScaleFactor = newS;
    updateViewTransform();
}

void PlugDataTextEditor::updateViewTransform()
{
    transform = AffineTransform::scale(viewScaleFactor).translated(translation.x, translation.y);
    highlight.setViewTransform(transform, document.getSelections());
    searchHighlight.setViewTransform(transform, document.getSearchSelections());
    caret.setViewTransform(transform);
    gutter.setViewTransform(transform);
    repaint();
}

void PlugDataTextEditor::updateSelections()
{
    highlight.updateSelections(document.getSelections());
    searchHighlight.updateSelections(document.getSearchSelections());
    caret.updateSelections();
    gutter.updateSelections();
}

void PlugDataTextEditor::translateToEnsureCaretIsVisible()
{
    auto const i = document.getSelections().back().head;
    auto const t = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::top)).transformedBy(transform);
    auto const b = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::bottom)).transformedBy(transform);

    if (t.y < 0.f) {
        translateView(0.f, -t.y);
    } else if (b.y > getHeight()) {
        translateView(0.f, -b.y + getHeight());
    }
}

void PlugDataTextEditor::translateToEnsureSearchIsVisible(int const index)
{
    auto selections = document.getSearchSelections();
    if (index >= selections.size())
        return;

    auto const i = selections[index].head;
    auto const t = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::top)).transformedBy(transform);
    auto const b = Point<float>(0.f, document.getVerticalPosition(i.x, TextDocument::Metric::bottom)).transformedBy(transform);

    if (t.y < 0.f) {
        translateView(0.f, -t.y);
    } else if (b.y > getHeight()) {
        translateView(0.f, -b.y + getHeight());
    }
}

void PlugDataTextEditor::resized()
{
    highlight.setBounds(getLocalBounds());
    searchHighlight.setBounds(getLocalBounds());
    caret.setBounds(getLocalBounds());
    gutter.setBounds(getLocalBounds());
}

void PlugDataTextEditor::paint(Graphics& g)
{
    g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));

    String renderSchemeString;

    switch (renderScheme) {
    case RenderScheme::usingAttributedString:
        renderTextUsingAttributedString(g);
        renderSchemeString = "attr. str";
        break;
    case RenderScheme::usingGlyphArrangement:
        renderTextUsingGlyphArrangement(g);
        renderSchemeString = "glyph arr.";
        break;
    }

    auto const scrollBarBounds = getScrollBarBounds();
    auto const fadeWidth = jmap<float>(scrollbarFadePosition, 0.0f, 1.0f, 4.0f, 8.0f);

    // Draw a scrollbar if content height exceeds visible height
    if (!scrollBarBounds.isEmpty()) {
        auto const scrollbarColour = findColour(PlugDataColour::scrollbarThumbColourId);
        auto const canvasBgColour = findColour(PlugDataColour::canvasBackgroundColourId);
        g.setColour(scrollbarColour.interpolatedWith(canvasBgColour, 0.7f + jmap(scrollbarFadePosition, 0.0f, 1.0f, 0.1f, 0.0f))); // Scrollbar background
        g.fillRoundedRectangle(getWidth() - (fadeWidth + 2.0f), 2, fadeWidth, getHeight() - 4, fadeWidth / 2.0f);

        auto const scrollBarThumbCol = scrollBarClicked ? scrollbarColour : scrollbarColour.interpolatedWith(canvasBgColour.contrasting(0.6f), 0.7f);
        g.setColour(scrollBarThumbCol); // Scrollbar thumb
        g.fillRoundedRectangle(scrollBarBounds.withTrimmedLeft(8.0f - fadeWidth), fadeWidth / 2.0f);
    }
}

Rectangle<float> PlugDataTextEditor::getScrollBarBounds() const
{
    auto const contentHeight = document.getHeight();
    auto const visibleHeight = getHeight();
    if (contentHeight <= visibleHeight)
        return {};

    auto const scrollPosition = -translation.y;
    float const scrollbarHeight = static_cast<float>(visibleHeight) / contentHeight * visibleHeight; // Height of the scrollbar
    float const scrollbarPosition = scrollPosition / contentHeight * visibleHeight;                  // Y position of the scrollbar

    return { getWidth() - 10.f, scrollbarPosition + 2, 8.0f, scrollbarHeight - 4 };
}

void PlugDataTextEditor::mouseDown(MouseEvent const& e)
{
    if (e.getNumberOfClicks() > 1) {
        return;
    }

    auto selections = document.getSelections();
    auto const index = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));

    if (selections.contains(index)) {
        return;
    }

    if (e.x > getWidth() - 10 && document.getHeight() > getHeight()) {
        mouseDownViewPosition = translation.y + e.y * (document.getHeight() / getHeight());
        scrollBarClicked = true;
        repaint();
        return;
    }

    if (e.mods.isShiftDown() && selections.size()) {
        auto& selection = selections[selections.size() - 1];
        bool const wasOriented = selection.isOriented();
        auto orientedSelection = selection.oriented();

        auto isBeforeSelection = [](Point<int> index, Point<int> selection) -> int {
            if (index.x == selection.x)
                return index.y == selection.y ? -1 : index.y < selection.y;
            return index.x < selection.x;
        };

        if (isBeforeSelection(index, orientedSelection.head))
            orientedSelection.head = index;
        else
            orientedSelection.tail = index;

        selection = wasOriented ? orientedSelection : orientedSelection.swapped();

        document.setSelections(selections);
        updateSelections();
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
    // Check if the drag is happening within the scrollbar area (right 10px of the editor)
    if (e.getMouseDownX() > getWidth() - 10 && document.getHeight() > getHeight()) {
        translation.y = jlimit(jmin(-0.f, -(viewScaleFactor * document.getBounds().getHeight()) + (getHeight() - 10)), 0.0f, mouseDownViewPosition - e.y * (document.getHeight() / getHeight()));
        updateViewTransform();
        return;
    }
    if (e.mouseWasDraggedSinceMouseDown()) {
        auto selection = document.getSelections().front();
        selection.head = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));
        document.setSelections({ selection });
        translateToEnsureCaretIsVisible();
        updateSelections();
    }
}

void PlugDataTextEditor::mouseUp(MouseEvent const& e)
{
    scrollBarClicked = false;
    repaint();
}

void PlugDataTextEditor::mouseMove(MouseEvent const& e)
{
    if (e.x > getWidth() - 10 && document.getHeight() > getHeight() && !isOverScrollBar) {
        isOverScrollBar = true;
        startTimerHz(60);
    } else if ((e.x <= getWidth() - 10 || document.getHeight() < getHeight()) && isOverScrollBar) {
        isOverScrollBar = false;
        startTimerHz(60);
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

void PlugDataTextEditor::timerCallback()
{
    if (isOverScrollBar) {
        scrollbarFadePosition += 0.1f;
    } else {
        scrollbarFadePosition -= 0.1f;
    }

    scrollbarFadePosition = std::clamp(scrollbarFadePosition, 0.0f, 1.0f);
    if (!isOverScrollBar && scrollbarFadePosition == 0.0f)
        stopTimer();
    if (isOverScrollBar && scrollbarFadePosition == 1.0f)
        stopTimer();

    repaint();
}

void PlugDataTextEditor::mouseMagnify(MouseEvent const& e, float const scaleFactor)
{
    scaleView(scaleFactor, e.position.y);
}

bool PlugDataTextEditor::keyPressed(KeyPress const& key)
{
    using Target = TextDocument::Target;
    using Direction = TextDocument::Direction;
    auto mods = key.getModifiers();
    auto const isTab = tabKeyUsed && key == KeyPress::tabKey;
    auto const isBackspace = key == KeyPress::backspaceKey;

    auto nav = [this, mods](Target const target, Direction const direction) {
        if (mods.isShiftDown())
            document.navigateSelections(target, direction, Selection::Part::head);
        else
            document.navigateSelections(target, direction, Selection::Part::both);

        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expandBack = [this](Target const target, Direction const direction) {
        document.navigateSelections(target, direction, Selection::Part::head);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expand = [this](Target const target) {
        document.navigateSelections(target, Direction::backwardCol, Selection::Part::head);
        document.navigateSelections(target, Direction::forwardCol, Selection::Part::tail);
        updateSelections();
        return true;
    };
    auto addCaret = [this](Target const target, Direction const direction) {
        auto s = document.getSelections().back();
        document.navigate(s.head, target, direction);
        document.addSelection(s);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    if (key.isKeyCode(KeyPress::escapeKey)) {
        document.setSelections({ document.getSelections().back() });
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
            return expandBack(Target::whitespace, Direction::backwardCol)
                && expandBack(Target::word, Direction::backwardCol)
                && insert("");

        if (key == KeyPress('e', ModifierKeys::ctrlModifier, 0) || key == KeyPress('e', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav(Target::line, Direction::forwardCol);

        if (key == KeyPress('a', ModifierKeys::ctrlModifier, 0) || key == KeyPress('a', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
            return nav(Target::line, Direction::backwardCol);    }
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
    if (key == KeyPress('z', ModifierKeys::commandModifier, 0))
        return undo.undo();
    if (key == KeyPress('r', ModifierKeys::commandModifier, 0))
        return undo.redo();

    if (key == KeyPress('x', ModifierKeys::commandModifier, 0)) {
        SystemClipboard::copyTextToClipboard(document.getSelectionContent(document.getSelections().front()));
        return insert("");
    }
    if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
        SystemClipboard::copyTextToClipboard(document.getSelectionContent(document.getSelections().front()));
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
    double const now = Time::getApproximateMillisecondCounter();

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
    if (isOverScrollBar)
        return MouseCursor::NormalCursor;

    return getMouseXYRelative().x < GUTTER_WIDTH && getMouseXYRelative().x > getWidth() - 10 ? MouseCursor::NormalCursor : MouseCursor::IBeamCursor;
}

CodeEditorComponent::ColourScheme PlugDataTextEditor::getSyntaxColourScheme()
{
    auto const textColour = findColour(PlugDataColour::canvasTextColourId);
    if (findColour(PlugDataColour::canvasBackgroundColourId).getPerceivedBrightness() > 0.5f) {
        static CodeEditorComponent::ColourScheme::TokenType const types[] = {
            { "Error", Colour(0xffcc0000) },
            { "Comment", Colour(0xff3c3c9c) },
            { "Keyword", Colour(0xff0000cc) },
            { "Operator", Colour(0xff225500) },
            { "Identifier", Colour(0xff000000) },
            { "Integer", Colour(0xff880000) },
            { "Float", Colour(0xff885500) },
            { "String", Colour(0xff990099) },
            { "Bracket", Colour(0xff000055) },
            { "Punctuation", textColour }
        };

        CodeEditorComponent::ColourScheme cs;

        for (auto& t : types)
            cs.set(t.name, Colour(t.colour));

        return cs;
    }
    static CodeEditorComponent::ColourScheme::TokenType const types[] = {
        { "Error", Colour(0xffff6666) },
        { "Comment", Colour(0xff8888ff) },
        { "Keyword", Colour(0xff66aaff) },
        { "Operator", Colour(0xff77cc77) },
        { "Identifier", Colour(0xffffffff) },
        { "Integer", Colour(0xffffaa66) },
        { "Float", Colour(0xffffcc88) },
        { "String", Colour(0xffcc88ff) },
        { "Bracket", Colour(0xff66aaff) },
        { "Punctuation", textColour }
    };

    CodeEditorComponent::ColourScheme cs;

    for (auto& t : types)
        cs.set(t.name, Colour(t.colour));

    return cs;
}

void PlugDataTextEditor::setSearchText(String const& searchText)
{
    document.search(searchText);
    updateSelections();
    translateToEnsureSearchIsVisible(0);
}

void PlugDataTextEditor::searchNext()
{
    auto const next = document.searchNext();
    updateSelections();
    translateToEnsureSearchIsVisible(next);
}

void PlugDataTextEditor::renderTextUsingAttributedString(Graphics& g)
{
    /*
     Credit to chrisboy2000 for this
     */
    auto const colourScheme = getSyntaxColourScheme();
    auto const originalHeight = document.getFont().getHeight();

    auto const scaleFactor = std::sqrt(std::abs(transform.getDeterminant()));
    auto const font = document.getFont().withHeight(originalHeight * scaleFactor);
    auto rows = document.findRowsIntersecting(g.getClipBounds().toFloat().transformedBy(transform.inverted()));

    for (auto const& r : rows) {
        auto line = document.getLine(r.rowNumber);
        auto const T = document.getVerticalPosition(r.rowNumber, TextDocument::Metric::ascent);
        auto const B = document.getVerticalPosition(r.rowNumber, TextDocument::Metric::bottom);
        auto bounds = Rectangle<float>::leftTopRightBottom(0.f, T, 1000.f, B).transformedBy(transform).translated(4, 0);

        AttributedString s;

        if (!enableSyntaxHighlighting) {
            s.append(line, font, findColour(PlugDataColour::panelTextColourId));
        } else {
            LuaTokeniserFunctions::StringIterator si(line);
            auto previous = si.t;

            while (!si.isEOF()) {
                auto const tokenType = LuaTokeniserFunctions::readNextToken(si);
                auto const colour = enableSyntaxHighlighting ? colourScheme.types[tokenType].colour : findColour(PlugDataColour::panelTextColourId);
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
        auto const colourScheme = getSyntaxColourScheme();
        auto const rows = document.getRangeOfRowsIntersecting(g.getClipBounds().toFloat());
        auto index = Point<int>(rows.getStart(), 0);
        document.navigate(index, TextDocument::Target::token, TextDocument::Direction::backwardRow);

        auto it = TextDocument::Iterator(document, { 0, 0 });
        auto previous = it.getIndex();
        auto zones = SmallArray<Selection>();

        while (it.getIndex().x < rows.getEnd() && !it.isEOF()) {
            auto const tokenType = LuaTokeniserFunctions::readNextToken(it);
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

struct TextEditorDialog final : public Component
    , public ChangeListener {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;

    PlugDataTextEditor editor;
    MainToolbarButton saveButton = MainToolbarButton(Icons::Save);
    MainToolbarButton undoButton = MainToolbarButton(Icons::Undo);
    MainToolbarButton redoButton = MainToolbarButton(Icons::Redo);
    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);

    SearchEditor searchInput;

    std::function<void(String, bool)> onClose;
    std::function<void(String)> onSave;

    String title;
    int margin;

    explicit TextEditorDialog(String name, bool const enableSyntaxHighlighting, std::function<void(String, bool)> const& closeCallback, std::function<void(String)> const& saveCallback, const float scale)
        : resizer(this, &constrainer)
        , onClose(closeCallback)
        , onSave(saveCallback)
        , title(std::move(name))
        , margin(ProjectInfo::canUseSemiTransparentWindows() ? 15 : 0)
        , desktopScale(scale)
    {
        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 200);
        constrainer.setFixedAspectRatio(0.0f);

        closeButton->onClick = [this] {
            // Call asynchronously because this function may distroy the dialog
            MessageManager::callAsync([_this = SafePointer(this)] {
                if(_this) {
                    _this->onClose(_this->editor.getText(), _this->editor.hasChanged());
                }
            });
        };

        addToDesktop(0);
        setVisible(true);

        // Position in centre of screen
        setBounds((Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea / desktopScale).withSizeKeepingCentre(700, 500));

        addAndMakeVisible(saveButton);
        addAndMakeVisible(undoButton);
        addAndMakeVisible(redoButton);
        addAndMakeVisible(searchButton);

        editor.setUndoChangeListener(this);

        undoButton.onClick = [this] {
            editor.performUndo();
        };
        redoButton.onClick = [this] {
            editor.performRedo();
        };

        saveButton.onClick = [this] {
            onSave(editor.getText());
            editor.setUnchanged();
        };

        searchButton.onClick = [this] {
            searchInput.setVisible(searchButton.getToggleState());
            editor.setSearchText("");
            if (searchButton.getToggleState()) {
                searchInput.setText("");
                searchInput.grabKeyboardFocus();
            }
        };

        searchButton.setClickingTogglesState(true);
        addAndMakeVisible(editor);
        addAndMakeVisible(resizer);
        resizer.setAlwaysOnTop(true);
        resizer.setAllowHostManagedResize(false);

        addChildComponent(searchInput);
        searchInput.setTextToShowWhenEmpty("Type to search", findColour(TextEditor::textColourId).withAlpha(0.5f));
        searchInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        searchInput.setJustification(Justification::centredLeft);
        searchInput.setBorder({ 0, 3, 5, 1 });
        searchInput.onTextChange = [this] {
            editor.setSearchText(searchInput.getText());
        };
        searchInput.onReturnKey = [this] {
            editor.searchNext();
        };

        editor.grabKeyboardFocus();
        editor.setEnableSyntaxHighlighting(enableSyntaxHighlighting);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        undoButton.setEnabled(editor.canUndo());
        redoButton.setEnabled(editor.canRedo());
        saveButton.setEnabled(editor.hasChanged());
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(margin);

        resizer.setBounds(b);

        auto toolbarBounds = b.removeFromTop(43);
        toolbarBounds.removeFromLeft(10);
        toolbarBounds.removeFromTop(2);

        auto const closeButtonBounds = toolbarBounds.removeFromRight(30).reduced(0, 5).translated(-5, 1);
        closeButton->setBounds(closeButtonBounds);

        toolbarBounds.removeFromRight(10);
        auto const searchButtonBounds = toolbarBounds.removeFromRight(39);
        auto const saveButtonBounds = toolbarBounds.removeFromLeft(39);
        toolbarBounds.removeFromLeft(10);
        auto const undoButtonBounds = toolbarBounds.removeFromLeft(39);
        toolbarBounds.removeFromLeft(10);
        auto const redoButtonBounds = toolbarBounds.removeFromLeft(39);

        searchInput.setBounds(toolbarBounds.reduced(5, 5));

        searchButton.setBounds(searchButtonBounds);
        saveButton.setBounds(saveButtonBounds);
        undoButton.setBounds(undoButtonBounds);
        redoButton.setBounds(redoButtonBounds);

        editor.setBounds(b.withTrimmedBottom(20));
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
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(getLocalBounds().reduced(margin).toFloat(), ProjectInfo::canUseSemiTransparentWindows() ? Corners::windowCornerRadius : 0.0f, 1.0f);
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress('s', ModifierKeys::commandModifier, 0)) {
            onSave(editor.getText());
            editor.setUnchanged();
            return true;
        }

        if (key == KeyPress('f', ModifierKeys::commandModifier, 0)) {
            searchButton.setToggleState(true, sendNotification);
            return true;
        }

        return false;
    }

    void paint(Graphics& g) override
    {
        if (ProjectInfo::canUseSemiTransparentWindows()) {
            auto shadowPath = Path();
            shadowPath.addRoundedRectangle(getLocalBounds().reduced(20), Corners::windowCornerRadius);
            StackShadow::renderDropShadow(hash("text_editor_dialog"), g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 13.0f);
        }

        auto const radius = ProjectInfo::canUseSemiTransparentWindows() ? Corners::windowCornerRadius : 0.0f;

        auto const b = getLocalBounds().reduced(margin);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(b.toFloat(), radius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat().reduced(0.5f), radius, 1.0f);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        // g.drawHorizontalLine(b.getX() + 39, b.getY() + 48, b.getWidth());
        g.drawHorizontalLine(b.getHeight() - 20, b.getY() + 48, b.getWidth());

        if (!title.isEmpty()) {
            Fonts::drawText(g, title, b.getX(), b.getY(), b.getWidth(), 40, findColour(PlugDataColour::toolbarTextColourId), 15, Justification::centred);
        }
    }

    float getDesktopScaleFactor() const override { return desktopScale * Desktop::getInstance().getGlobalScaleFactor(); }
private:
    float desktopScale = 1.0f;
};
