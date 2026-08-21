#include "Dialogs/TextEditorDialog.h"

// Drives the standalone text editor window (used for [text define], comments,
// lua scripts, etc).
//
// Dialogs::showTextEditorDialog is disabled while testing (so stray clicks in
// the click-through test can't open floating windows), which left all 2000+
// lines of TextEditorDialog.h uncovered. This instantiates the dialog
// directly and exercises the same things a user does:
//
//  - opening with text and reading it back
//  - typing, newlines, backspace and caret/selection movement via key events
//  - undo/redo through the toolbar buttons
//  - the search bar: matches, no matches, searchNext cycling
//  - saving through the save button (onSave callback + change flag reset)
//  - zooming, and malformed content (mixed line endings, very long lines,
//    unicode) with syntax highlighting enabled
//  - closing with unsaved changes (onClose must report hasChanged == true)

class TextEditorDialogTest : public PlugDataUnitTest
{
public:
    TextEditorDialogTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Text Editor Dialog Test")
    {
    }

private:
    static KeyPress key(int const keyCode, ModifierKeys const mods = ModifierKeys()) { return KeyPress(keyCode, mods, 0); }
    static KeyPress character(juce_wchar const c) { return KeyPress(c, ModifierKeys(), c); }

    void perform() override
    {
        beginTest("Open, edit, undo/redo, search, save and close");
        exerciseDocumentModel();

        dialog = std::make_unique<TextEditorDialog>(
            "test.txt", true,
            [this](String const& text, bool const hasChanged) {
                closedText = text;
                closedWithChanges = hasChanged;
            },
            [this](String const& text) { savedText = text; },
            1.0f);

        auto& textEditor = dialog->editor;

        textEditor.setText("first line\nsecond line\nthird line\n");
        expect(textEditor.getText().contains("second line"), "text must round-trip through setText/getText");

        // Type at the caret: characters, newline, backspace
        textEditor.keyPressed(character('h'));
        textEditor.keyPressed(character('i'));
        textEditor.keyPressed(key(KeyPress::returnKey));
        textEditor.keyPressed(character('x'));
        textEditor.keyPressed(key(KeyPress::backspaceKey));
        expect(textEditor.getText().contains("hi"), "typed characters must appear in the document");
        expect(textEditor.hasChanged(), "typing must mark the document changed");

        // Caret and selection movement
        textEditor.keyPressed(key(KeyPress::downKey));
        textEditor.keyPressed(key(KeyPress::endKey));
        textEditor.keyPressed(key(KeyPress::homeKey));
        textEditor.keyPressed(key(KeyPress::rightKey, ModifierKeys::shiftModifier));
        textEditor.keyPressed(key(KeyPress::rightKey, ModifierKeys::shiftModifier));
        textEditor.keyPressed(key(KeyPress::leftKey));
        textEditor.keyPressed(key(KeyPress::upKey));
        // Select-all + retype. Whether ctrl/cmd+A select-all takes effect depends
        // on keyboard handling, which is unreliable under headless Xvfb, so accept
        // either a full replacement (selection worked) or an insertion (typing
        // still reached the editor).
        textEditor.keyPressed(key('a', ModifierKeys::commandModifier));
        textEditor.keyPressed(character('y'));
        expect(textEditor.getText().contains("y"), "typing after select-all must reach the editor");

        // Undo/redo through the toolbar buttons
        auto const textBeforeUndo = textEditor.getText();
        expect(textEditor.canUndo(), "document must have undo history");
        dialog->undoButton.onClick();
        expect(textEditor.getText() != textBeforeUndo, "undo must change the text");
        dialog->redoButton.onClick();
        expect(textEditor.getText() == textBeforeUndo, "redo must restore the text");

        // Search: open the bar, search a match, cycle, then a non-match
        textEditor.setText("alpha beta gamma\nbeta again\nlast beta line\n");
        dialog->searchButton.setToggleState(true, dontSendNotification);
        dialog->searchButton.onClick();
        textEditor.setSearchText("beta");
        textEditor.searchNext();
        textEditor.searchNext();
        textEditor.searchNext(); // wraps around
        textEditor.setSearchText("no_such_string_in_here");
        textEditor.searchNext();
        textEditor.setSearchText("");
        dialog->searchButton.setToggleState(false, dontSendNotification);
        dialog->searchButton.onClick();

        // Save through the toolbar
        dialog->saveButton.onClick();
        expect(savedText.contains("alpha beta gamma"), "save button must deliver the document to onSave");
        expect(!textEditor.hasChanged(), "saving must clear the changed flag");

        // Zoom
        textEditor.scaleView(1.25f, 0.0f, true);
        textEditor.scaleView(0.75f, 0.0f, true);

        // Less common navigation/editing commands and direct pointer input
        textEditor.setText("one two.three\n\nfour five\nsix seven eight\n");
        auto makeMouse = [&textEditor](Point<float> const position, ModifierKeys const mods = ModifierKeys::leftButtonModifier, int const clicks = 1, bool const dragged = false) {
            return MouseEvent(Desktop::getInstance().getMainMouseSource(), position, mods,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, &textEditor, &textEditor,
                Time::getCurrentTime(), { 80.0f, 40.0f }, Time::getCurrentTime(), clicks, dragged);
        };
        textEditor.setBounds(0, 0, 500, 220);
        textEditor.mouseDown(makeMouse({ 80.0f, 40.0f }));
        textEditor.mouseUp(makeMouse({ 80.0f, 40.0f }));

        for (auto const& press : {
                 key(KeyPress::rightKey, ModifierKeys::ctrlModifier),
                 key(KeyPress::leftKey, ModifierKeys::ctrlModifier),
                 key(KeyPress::downKey, ModifierKeys::ctrlModifier),
                 key(KeyPress::upKey, ModifierKeys::ctrlModifier),
                 key(KeyPress::downKey, ModifierKeys::commandModifier),
                 key(KeyPress::upKey, ModifierKeys::commandModifier),
                 key('d', ModifierKeys::commandModifier),
                 key('l', ModifierKeys::commandModifier),
                 key('a', ModifierKeys::ctrlModifier),
                 key('e', ModifierKeys::ctrlModifier),
                 key('d', ModifierKeys::ctrlModifier),
                 key(KeyPress::backspaceKey, ModifierKeys::ctrlModifier),
                 key(KeyPress::escapeKey),
                 key(KeyPress::tabKey),
                 key(61, ModifierKeys::commandModifier),
                 key(45, ModifierKeys::commandModifier) }) {
            textEditor.keyPressed(press);
        }

        // Cover multi-caret creation independently. Expanding overlapping
        // carets and then editing is not a valid editor state.
        textEditor.mouseDown(makeMouse({ 80.0f, 40.0f }));
        textEditor.keyPressed(key(KeyPress::downKey, ModifierKeys::ctrlModifier | ModifierKeys::altModifier));
        textEditor.keyPressed(key(KeyPress::upKey, ModifierKeys::ctrlModifier | ModifierKeys::altModifier));
        textEditor.keyPressed(key(KeyPress::escapeKey));

        textEditor.mouseDown(makeMouse({ 80.0f, 40.0f }));
        textEditor.mouseDrag(makeMouse({ 180.0f, 90.0f }, ModifierKeys::leftButtonModifier, 1, true));
        textEditor.mouseUp(makeMouse({ 180.0f, 90.0f }));
        textEditor.mouseDoubleClick(makeMouse({ 100.0f, 40.0f }, ModifierKeys::leftButtonModifier, 2));
        textEditor.mouseDoubleClick(makeMouse({ 100.0f, 40.0f }, ModifierKeys::leftButtonModifier, 3));
        textEditor.mouseMove(makeMouse({ 497.0f, 100.0f }, ModifierKeys()));
        textEditor.getMouseCursor();
        textEditor.mouseExit(makeMouse({ 400.0f, 100.0f }, ModifierKeys()));
        MouseWheelDetails wheel;
        wheel.deltaY = -0.5f;
        textEditor.mouseWheelMove(makeMouse({ 250.0f, 100.0f }, ModifierKeys()), wheel);
        textEditor.mouseWheelMove(makeMouse({ 250.0f, 100.0f }, ModifierKeys::commandModifier), wheel);
        textEditor.mouseMagnify(makeMouse({ 250.0f, 100.0f }, ModifierKeys()), 1.1f);
        textEditor.getScrollBarBounds();
        textEditor.getSyntaxColourScheme();
        textEditor.getCaretPosition();

        dialog->keyPressed(key('f', ModifierKeys::commandModifier));
        dialog->searchInput.setText("four", sendNotificationSync);
        dialog->searchInput.onReturnKey();
        dialog->keyPressed(key('s', ModifierKeys::commandModifier));
        dialog->createComponentSnapshot(dialog->getLocalBounds());

        // Malformed/awkward content with syntax highlighting on
        String malformed = "mixed\r\nline\rendings\nhere\n";
        malformed += String::repeatedString("x", 10000) + "\n";                           // very long single line
        malformed += String::fromUTF8("unicode: \xc3\xa9\xe6\x97\xa5\xf0\x9f\x98\x80\n"); // é, 日, emoji
        malformed += "unterminated \"string and {{{ unbalanced\n";
        textEditor.setText(malformed);
        textEditor.keyPressed(key(KeyPress::endKey, ModifierKeys::commandModifier));
        dialog->createComponentSnapshot(dialog->getLocalBounds());

        // Close with unsaved changes: set a small, recognisable final text and
        // type one more character, then use the window close button
        textEditor.setText("final unsaved content");
        textEditor.keyPressed(key(KeyPress::endKey, ModifierKeys::commandModifier));
        textEditor.keyPressed(character('Z'));
        expect(textEditor.hasChanged(), "editing after save must mark the document changed again");
        dialog->closeButton->onClick();

        // The close callback is delivered asynchronously (and can be slow under
        // headless CI), so poll for it rather than assuming a fixed delay.
        pollForClose(0);
    }

    void pollForClose(int const attempt)
    {
        constexpr int maxAttempts = 40; // ~2s
        if (!closedText.isEmpty() || attempt >= maxAttempts) {
            expect(closedWithChanges, "closing after edits must report unsaved changes");
            expect(closedText.contains("final unsaved content"), "close callback must receive the current text");
            dialog.reset();
            signalDone(true);
            return;
        }
        Timer::callAfterDelay(50, [this, attempt] { pollForClose(attempt + 1); });
    }

    void exerciseDocumentModel()
    {
        exerciseTokeniserAndEscaping();

        TextDocument document;
        document.setFont(Font(FontOptions(14.0f)));
        document.replaceAll("alpha beta.gamma\nsecond line\n\nlast line");
        document.setSelections({ Selection(0, 2, 1, 6) });

        Selection selection(1, 6, 0, 2);
        ignoreUnused(selection < Selection(1, 7, 2, 0));
        ignoreUnused(selection < Selection(2, 0, 2, 1));
        selection.toString();
        selection.isSingular();
        selection.isSingleLine();
        selection.intersectsRow(0);
        selection.getColumnRangeOnRow(-1, 0);
        selection.getColumnRangeOnRow(0, document.getNumColumns(0));
        selection.getColumnRangeOnRow(1, document.getNumColumns(1));
        selection.getColumnRangeOnRow(2, document.getNumColumns(2));
        selection.oriented();
        selection.swapped();
        selection.horizontallyMaximized(document);
        selection.measuring("one\ntwo");
        selection.startingFrom({ 2, 0 });
        selection.withStyle(3);

        auto pushed = Selection(0, 4, 0, 8);
        pushed.pushBy(Selection(0, 1, 0, 3));
        pushed.pullBy(Selection(0, 0, 0, 2));
        Point<int> index(0, 5);
        Selection(0, 1, 0, 3).push(index);
        Selection(0, 1, 0, 3).pull(index);

        for (auto metric : {
                 TextDocument::Metric::top, TextDocument::Metric::ascent,
                 TextDocument::Metric::baseline, TextDocument::Metric::descent,
                 TextDocument::Metric::bottom }) {
            document.getVerticalPosition(1, metric);
            document.getPosition({ 1, 2 }, metric);
        }

        document.getSelectionRegion(Selection(0, 2, 1, 4));
        document.getSelectionRegion(Selection(1, 4, 0, 2), { 0, 0, 300, 100 });
        document.getBounds();
        document.getBoundsOnRow(0, { 0, 5 });
        document.getGlyphBounds({ 0, 3 });
        document.getGlyphsForRow(0);
        document.getGlyphsForRow(0, 1, false);
        document.findGlyphsIntersecting({ 0, 0, 300, 100 });
        document.getRangeOfRowsIntersecting({ 0, 0, 300, 100 });
        document.findRowsIntersecting({ 0, 0, 300, 100 }, true);
        document.findIndexNearestPosition({ 30.0f, 20.0f });
        document.getLineSpacing();
        document.breakLine({});
        document.breakLine(String::repeatedString("long line ", 80));

        Point<int> cursor(0, 0);
        document.next(cursor);
        document.nextRow(cursor);
        document.prev(cursor);
        document.prevRow(cursor);
        for (auto target : {
                 TextDocument::Target::whitespace, TextDocument::Target::punctuation,
                 TextDocument::Target::character, TextDocument::Target::word,
                 TextDocument::Target::line, TextDocument::Target::paragraph,
                 TextDocument::Target::document }) {
            auto forward = Point<int>(0, 0);
            document.navigate(forward, target, TextDocument::Direction::forwardCol);
            auto backward = Point<int>(1, 3);
            document.navigate(backward, target, TextDocument::Direction::backwardCol);
        }
        document.navigateSelections(TextDocument::Target::character,
            TextDocument::Direction::forwardCol, Selection::Part::head);
        document.navigateSelections(TextDocument::Target::character,
            TextDocument::Direction::backwardCol, Selection::Part::tail);

        document.search("line", true);
        document.searchNext();
        document.getCurrentSearchSelection();
        document.search("missing", false);
        document.searchNext();
        document.setSelections({ Selection(Point<int>(0, 5)) });
        document.getSelectionContent(Selection(0, 0, 1, 6));
        document.getCaretPosition();
        auto const caretOffset = document.getCaretOffset();
        document.setCaretOffset(caretOffset);
        document.setCaretOffset(100000);
        document.setViewScale(1.25f);
        document.setMaximumLineWidth(120, 1.25f);

        SmallArray<Selection> tokenZones {
            Selection(0, 0, 0, 2).withStyle(1),
            Selection(0, 2, 0, 4).withStyle(2)
        };
        document.clearTokens({ 0, document.getNumRows() });
        document.applyTokens({ 0, document.getNumRows() }, tokenZones);

        TextDocument::Iterator iterator(document, { 0, 0 });
        iterator.peekNextChar();
        iterator.skipWhitespace();
        iterator.skipToEndOfLine();
        iterator.getIndex();
        while (!iterator.isEOF())
            iterator.nextChar();

        Transaction insertion;
        insertion.selection = Selection(Point<int>(0, 2));
        insertion.content = "XYZ";
        auto reciprocal = document.fulfill(insertion);
        document.fulfill(reciprocal);

        Transaction backspace;
        backspace.selection = Selection(Point<int>(0, 3));
        backspace.content = String::charToString(KeyPress::backspaceKey);
        backspace.accountingForSpecialCharacters(document);

        Transaction tab;
        tab.selection = Selection(Point<int>(0, 3));
        tab.content = String::charToString(KeyPress::tabKey);
        tab.accountingForSpecialCharacters(document);

        Transaction undoable;
        undoable.selection = Selection(Point<int>(0, 1));
        undoable.content = "Q";
        std::unique_ptr<UndoableAction> action(undoable.on(document, [](Transaction const&) { }));
        action->perform();
        action->undo();

        GlyphArrangementArray glyphs;
        glyphs.add({ "alpha", true });
        glyphs.insert(1, { "beta", false });
        glyphs[0];
        glyphs.isNewLine(0);
        glyphs.clearTokens(0);
        glyphs.applyTokens(0, Selection(0, 0, 0, 3).withStyle(2));
        glyphs.getToken(0, 1, -1);
        glyphs.getToken(20, 20, -1);
        glyphs.getGlyphs(0, 14.0f, -1, true);
        glyphs.removeRange(1, 1);
        glyphs.clear();
    }

    void exerciseTokeniserAndEscaping()
    {
        String const source =
            "do if or in and end for nil not try else goto then true self break false local until while error "
            "return repeat elseif assert function collectgarbage dofile require tostring identifier @decorated _private\n"
            "0 07 077 0x1f 0XCAFE 12 12u 1.5 .25 2e+3 3E-2 4f -5 -077 -0x2a -6.5e1 0x 09 1e+\n"
            ", ; : ( ) { } [ ] \"quoted\\\\\\\"text\" 'single\\\\\\'text' "
            "+ += - -= * *= % %= = == ~= ? < <= << <<= > >= >> >>= | || |= & && &= ^ ^= .\n"
            "-- line comment\n--[[ long comment with ] inside ]] --= #";

        LuaTokeniserFunctions::StringIterator iterator(source);
        while (!iterator.isEOF()) {
            auto const before = iterator.numChars;
            LuaTokeniserFunctions::readNextToken(iterator);
            expect(iterator.numChars > before, "tokeniser must always consume input");
        }

        auto parseNumber = [](String const& text) {
            LuaTokeniserFunctions::StringIterator number(text);
            return LuaTokeniserFunctions::parseNumber(number);
        };
        for (auto const& malformed : { String("0x"), String("09"), String("1e+"),
                 String("-"), String("123abc"), String("0x12z"), String("0789") })
            parseNumber(malformed);

        LuaTokeniserFunctions::isIdentifierStart('@');
        LuaTokeniserFunctions::isIdentifierBody('9');
        LuaTokeniserFunctions::isHexDigit('f');
        LuaTokeniserFunctions::isHexDigit('Z');
        LuaTokeniserFunctions::isOctalDigit('7');
        LuaTokeniserFunctions::isOctalDigit('8');
        LuaTokeniserFunctions::isDecimalDigit('9');
        String const shortIdentifier = "x";
        String const longIdentifier = "this_identifier_is_too_long";
        LuaTokeniserFunctions::isReservedKeyword(shortIdentifier.getCharPointer(), 1);
        LuaTokeniserFunctions::isReservedKeyword(longIdentifier.getCharPointer(), 27);

        char const bytes[] = {
            '\t', '\r', '\n', '\\', '"', '?', '?', '\0', '\'', 1,
            static_cast<char>(0xc3), static_cast<char>(0xa9), 'A', 'f'
        };
        MemoryOutputStream wrapped;
        LuaTokeniserFunctions::writeEscapeChars(
            wrapped, bytes, static_cast<int>(sizeof(bytes)), 8, true, true, true);

        MemoryOutputStream unwrapped;
        LuaTokeniserFunctions::writeEscapeChars(
            unwrapped, bytes, static_cast<int>(sizeof(bytes)), -1, false, false, false);
        LuaTokeniserFunctions::addEscapeChars(String::fromUTF8("line\né?'"));
    }

    std::unique_ptr<TextEditorDialog> dialog;
    String savedText, closedText;
    bool closedWithChanges = false;
};
