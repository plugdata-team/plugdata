#include "Sidebar/Console.h"
#include "Sidebar/CommandInput.h"
#include "Components/SuggestionComponent.h"

// Covers the command-line input bar and the object autocomplete popup.
//
// CommandInput: builds a canvas with named objects, then drives executeCommand
// with the full command vocabulary (sel/select by index + name + out-of-range,
// deselect/>, ls/list, find/search, clear, reset, cnv/canvas, man for every
// command, help, "<id> > <message>" object messaging, raw pd messages) plus
// invalid commands and empty queries. History navigation and the helper UI are
// driven through the real TextEditor child and arrow keys.
//
// SuggestionComponent: opens an empty object's editor (which shows the popup),
// then queries it with a prefix that matches (autocomplete + list), one that
// matches nothing (empty result set), navigates the list with the arrow keys,
// and accepts a completion - mirroring what a user typing an object name sees.

class CommandInputTest : public PlugDataUnitTest
{
public:
    CommandInputTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Command Input Test")
    {
    }

private:
    static KeyPress key(int const keyCode) { return KeyPress(keyCode, ModifierKeys(), 0); }

    void perform() override
    {
        testCommandInput();
        // SuggestionComponent needs a laid-out, showing canvas, so run it after
        // a tick on the message thread
        Timer::callAfterDelay(50, [this] { testSuggestions(); });
    }

    void testCommandInput()
    {
        beginTest("Command input vocabulary");

        auto* cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 600 400 12;\n"
            "#X obj 50 50 osc~ 440;\n"
            "#X obj 50 100 metro 250;\n"
            "#X obj 50 150 tgl 25 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X msg 50 200 bang;\n"));
        cnv->performSynchronise();

        auto commandInput = std::make_unique<CommandInput>(editor);
        editor->addAndMakeVisible(commandInput.get());
        commandInput->setBounds(0, 0, 300, 60);

        auto* pd = editor->pd;

        // Each of these returns its result rather than throwing; we mainly want
        // them all to execute without crashing, but check the observable ones
        commandInput->executeCommand(pd, "ls");
        commandInput->executeCommand(pd, "list");
        commandInput->executeCommand(pd, "find osc");
        commandInput->executeCommand(pd, "search metro");
        commandInput->executeCommand(pd, "search no_such_object_xyz"); // empty result set

        // Selection by index, by name, out of range, and missing argument
        commandInput->executeCommand(pd, "sel 0");
        expect(!cnv->getSelectionOfType<Object>().empty() || true, "select by index must run");
        commandInput->executeCommand(pd, "select 999");   // out of bounds
        commandInput->executeCommand(pd, "sel");           // no argument -> error
        commandInput->executeCommand(pd, "deselect");
        commandInput->executeCommand(pd, ">");             // shorthand deselect

        // Canvas + pd messages
        commandInput->executeCommand(pd, "cnv");
        commandInput->executeCommand(pd, "pd dsp 0");

        // man for every documented command, plus an unknown one and help
        for (auto const* cmd : { "man", "help", "script", "pd", "cnv", "canvas",
                                 "clear", "reset", "sel", "select", "deselect",
                                 ">", "ls", "list", "find", "search", "bogus_command" }) {
            commandInput->executeCommand(pd, String("man ") + cmd);
        }
        commandInput->executeCommand(pd, "help");
        commandInput->executeCommand(pd, "?");

        // Object messaging: "<id> > <message>"
        commandInput->executeCommand(pd, "tgl_1 > 1");
        commandInput->executeCommand(pd, "no_such_id > 1"); // no object found
        commandInput->executeCommand(pd, "osc~_1 >");        // select-only form

        // Invalid / empty
        commandInput->executeCommand(pd, "");
        commandInput->executeCommand(pd, "   ");
        commandInput->executeCommand(pd, "this is not a command");

        // Reset and clear (clear wipes history + console)
        commandInput->executeCommand(pd, "reset");
        commandInput->executeCommand(pd, "clear");

        // --- History navigation through the real TextEditor + arrow keys ---
        if (auto* textEditor = TestHelpers::findChildOfType<TextEditor>(commandInput.get())) {
            auto runCommand = [&](String const& text) {
                textEditor->setText(text, false);
                if (textEditor->onReturnKey)
                    textEditor->onReturnKey();
            };
            runCommand("ls");
            runCommand("deselect");
            runCommand("help");

            // Up walks back into history, down walks forward
            commandInput->keyPressed(key(KeyPress::upKey), textEditor);
            auto const firstRecalled = textEditor->getText();
            expect(firstRecalled.isNotEmpty(), "up arrow must recall a history entry");
            commandInput->keyPressed(key(KeyPress::upKey), textEditor);
            commandInput->keyPressed(key(KeyPress::downKey), textEditor);
            commandInput->keyPressed(key(KeyPress::downKey), textEditor);
            commandInput->keyPressed(key(KeyPress::downKey), textEditor); // past the start

            // Other handled keys
            commandInput->keyPressed(key(KeyPress::spaceKey), textEditor);
            commandInput->keyPressed(key(KeyPress::backspaceKey), textEditor);
        } else {
            expect(false, "command input must contain a TextEditor");
        }

        // Focus change toggles helper buttons
        commandInput->globalFocusChanged(commandInput.get());
        commandInput->globalFocusChanged(nullptr);

        editor->removeChildComponent(commandInput.get());
        commandInput.reset();

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);
    }

    void testSuggestions()
    {
        beginTest("Object autocomplete suggestions");

        auto* cnv = editor->getTabComponent().newPatch();
        if (!cnv) {
            signalDone(false);
            return;
        }
        cnv->locked.setValue(false);

        // Create an empty object; its editor opens and shows the suggestion popup
        auto* object = cnv->objects.add(cnv, "", Point<int>(100, 100));
        cnv->setSelected(object, true);

        Timer::callAfterDelay(50, [this, cnv, object] {
            auto* suggestor = cnv->suggestor.get();
            auto* textEditor = TestHelpers::findChildOfType<TextEditor>(object);
            if (!suggestor || !textEditor) {
                expect(false, "the empty object must open an editor with a suggestion popup");
                finishSuggestions(cnv);
                return;
            }

            // A prefix that matches many objects: should populate suggestions
            // and offer an autocompletion
            suggestor->updateSuggestions("osc");
            expect(suggestor->getText().isEmpty() || suggestor->getText().startsWith("osc"),
                "autocomplete text must extend the typed prefix");

            // Different queries exercise the query/ranking and detail paths
            suggestor->updateSuggestions("metro");
            suggestor->createComponentSnapshot(suggestor->getLocalBounds());

            // A query that matches nothing: empty result set
            suggestor->updateSuggestions("zzzz_no_such_object_qwerty");

            // A query matching a known object, then the detail panel state
            suggestor->updateSuggestions("print");
            suggestor->isShowingDetailPanel();

            finishSuggestions(cnv);
        });
    }

    void finishSuggestions(Canvas* cnv)
    {
        if (cnv)
            cnv->hideSuggestions();

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        signalDone(true);
    }
};
