/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <utility>
extern "C" {
#include <pd-lua/luas/luajit/src/lua.h>
#include <pd-lua/luas/luajit/src/lauxlib.h>
#include <pd-lua/luas/luajit/src/lualib.h>
#include <pd-lua/luas/lua-compat-5.3/c-api/compat-5.3.h>
}

#include "Components/BouncingViewport.h"
#include "Object.h"
#include "PluginEditor.h"
#include "Objects/ObjectBase.h"
#include "Sidebar/Sidebar.h"
#include "Components/MarkupDisplay.h"
#include "Utility/CachedStringWidth.h"

class CommandProcessor {
public:
    virtual ~CommandProcessor() = default;
    virtual SmallArray<std::pair<int, String>> executeCommand(pd::Instance* pd, String message) = 0;
};

class LuaExpressionParser {
public:
    using LuaResult = std::variant<double, String>;

    // Constructor: initialize Lua
    explicit LuaExpressionParser(pd::Instance* pd)
        : pd(pd)
    {
        L = luaL_newstate();
        luaL_openlibs(L); // Load Lua standard libraries

        luaopen_math(L);

        // Create the global "pd" table and set up "pd.post"
        lua_newtable(L); // Create a new table for "pd"

        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, LuaExpressionParser::luaPost, 1); // 1 upvalue for "pd"
        lua_setfield(L, -2, "post");                          // Sets pd.post in the table

        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, LuaExpressionParser::luaEval, 1); // 1 upvalue for "LuaExpressionParser"
        lua_setfield(L, -2, "eval");                          // Sets pd.eval in the table

        lua_setglobal(L, "pd"); // Set the table as the global "pd"
    }

    // Destructor: close Lua
    ~LuaExpressionParser()
    {
        if (L) {
            lua_close(L);
        }
    }

    void executeScript(String const& filePath) const
    {
        // Load the script without executing it
        if (luaL_loadfile(L, filePath.toRawUTF8()) == LUA_OK) {
            // Set the environment of the loaded chunk to _G to make everything global
            lua_pushglobaltable(L);   // Push _G onto the stack
            lua_setupvalue(L, -2, 1); // Set _G as the environment of the chunk

            // Execute the chunk, which will register all functions globally
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                char const* error = lua_tostring(L, -1);
                pd->logError("Error executing Lua script: " + String::fromUTF8(error));
                lua_pop(L, 1); // Remove error message from stack
            }
        } else {
            char const* error = lua_tostring(L, -1);
            pd->logError("Error loading Lua script: " + String::fromUTF8(error));
            lua_pop(L, 1); // Remove error message from stack
        }
    }

    // Function to execute an expression and return result as LuaResult (either double or string)
    LuaResult executeExpression(String const& expression, bool const hasReturnValue) const
    {
        String luaCode = "local __eval = function()\n";
        if (hasReturnValue)
            luaCode += "\nreturn ";
        luaCode += expression.trim(); // Append the expression without altering it
        luaCode += R"(
        end
        local success, result = pcall(__eval)
        if success then
            return result
        else
            error(result)
        end
        )";

        // Run the Lua code and check for errors
        if (luaL_dostring(L, luaCode.toRawUTF8()) == LUA_OK) {
            if (lua_isnumber(L, -1)) {
                double result = lua_tonumber(L, -1);
                lua_pop(L, 1); // Remove result from stack
                return result;
            }
            if (lua_isboolean(L, -1)) {
                bool const result = lua_toboolean(L, -1);
                lua_pop(L, 1); // Remove result from stack
                return static_cast<double>(result);
            }
            if (lua_isstring(L, -1)) {
                String result = lua_tostring(L, -1);
                lua_pop(L, 1); // Remove result from stack
                return result;
            }
            lua_pop(L, 1); // Remove nil result
            return "";     // Default to empty string if not a number or string
        }

        char const* error = lua_tostring(L, -1);
        pd->logError("Lua error: " + String::fromUTF8(error));
        lua_pop(L, 1); // Remove error message from stack
        return "";     // Return empty string on error
    }

    static int luaPost(lua_State* L)
    {
        // Retrieve the LuaWrapper instance (via userdata, upvalues, etc.)
        auto* pd = static_cast<LuaExpressionParser*>(lua_touserdata(L, lua_upvalueindex(1)))->pd;

        if (lua_isstring(L, 1)) {
            String const message = lua_tostring(L, 1);
            pd->logMessage(message); // Use Pd instance to post message
        } else {
            pd->logError("pd.post requires a string argument");
        }
        return 0;
    }

    static int luaEval(lua_State* L)
    {
        auto const* parser = static_cast<LuaExpressionParser*>(lua_touserdata(L, lua_upvalueindex(1)));

        if (lua_isstring(L, 1)) {
            String const command = lua_tostring(L, 1);
            auto result = parser->commandInput->executeCommand(parser->pd, command); // Execute the command

            // Create a Lua table to store result messages
            lua_newtable(L); // Creates an empty table on the stack

            int index = 1;
            for (auto const& [type, string] : result) {
                if (type == 0) {
                    lua_pushstring(L, string.toRawUTF8()); // Push the string onto the Lua stack
                    lua_rawseti(L, -2, index);             // Set it in the table at position `index`
                    ++index;
                }
            }

            return 1;
        }

        parser->pd->logError("pd.eval requires a string argument");
        return 0;
    }

    void setCommandProcessor(CommandProcessor* newCommandInput)
    {
        commandInput = newCommandInput;
    }

private:
    lua_State* L; // Lua state
    pd::Instance* pd;
    CommandProcessor* commandInput = nullptr;
};

class CommandInput final
    : public Component
    , public KeyListener
    , public CommandProcessor
    , public FocusChangeListener
    , public MarkupDisplay::URLHandler {
public:
    explicit CommandInput(PluginEditor* editor)
        : editor(editor)
    {
        // We need to set the target for the command manager, otherwise it will default to PlugDataApp and fail to find CommandID
        editor->commandManager.setFirstCommandTarget(editor);

        if (!luas.contains(editor->pd)) {
            luas[editor->pd] = std::make_unique<LuaExpressionParser>(editor->pd);
        }
        lua = luas[editor->pd].get();

        commandInput.setMultiLine(true);
        commandInput.setReturnKeyStartsNewLine(false);

        commandInput.onTextChange = [this] {
            currentCommand = commandInput.getText();
            updateSize();
        };

        commandInput.onReturnKey = [this, pd = editor->pd] {
            currentCommand.clear();

            auto const text = commandInput.getText();

            if (countBraces(text) > 0) {
                commandInput.insertTextAtCaret("\n");
                setConsoleTargetName("lua");
                updateSize();
                return;
            }

            auto result = executeCommand(pd, text);
            for (auto& [type, message] : result) {
                if (type == 0) {
                    pd->logMessage(message);
                } else {
                    pd->logError(message);
                }
            }
            auto const it = std::ranges::find(commandHistory, commandInput.getText());
            if (!commandInput.isEmpty() && it != commandHistory.end()) {
                commandHistory.erase(it);
            }

            commandHistory.push_front(commandInput.getText());
            currentHistoryIndex = -1;

            commandInput.clear();
            updateCommandInputTarget();
            updateSize();
        };

        addAndMakeVisible(commandInput);
        addAndMakeVisible(clearButton);

        clearButton.setWantsKeyboardFocus(false);
        clearButton.onClick = [this] {
            commandInput.clear();
            currentCommand.clear();
            updateCommandInputTarget();
            updateSize();
        };

        commandInput.setBorder({ 4, 3, 0, 0 });
        commandInput.addKeyListener(this);
        commandInput.addMouseListener(this, false);
        commandInput.setFont(Fonts::getDefaultFont().withHeight(15));

        commandInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        commandInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        commandInput.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

        if (currentCommand.isNotEmpty())
            commandInput.setText(currentCommand);

        updateSize();

        updateHelperCommands();
        Desktop::getInstance().addFocusChangeListener(this);
    }

    void updateHelperCommands()
    {
        auto isGlobalTarget = consoleTargetName == ">" || consoleTargetName == "lua >";
        auto& currentHelpers = isGlobalTarget ? helperCommands : objectHelperCommands;

        helperButtons.clear();

        for (auto const& cmd : currentHelpers) {
            auto* btn = helperButtons.add(new TextButton(cmd));
            btn->setWantsKeyboardFocus(false);
            btn->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            btn->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
            btn->setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
            btn->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            btn->onClick = [this, cmd] {
                commandInput.setText(cmd + " ", sendNotification);
                commandInput.grabKeyboardFocus();
                commandInput.moveCaretToEnd();
            };
            addChildComponent(btn);
            btn->setVisible(hasInputFocus);
        }
        resized();
    }

    void updateSize(bool animate = false)
    {
        int const extraHeight = hasInputFocus ? helperRowHeight : 0;
        int const newHeight = std::max(commandInput.getTextHeight() + 4, 30) + extraHeight;

        auto const fromBounds = getBounds();
        auto const targetBounds = Rectangle<int>(fromBounds.getX(), fromBounds.getBottom() - newHeight, fromBounds.getWidth(), newHeight);

        if (fromBounds == targetBounds)
            return;

        if (!animate) {
            sizeAnimator.complete();
            setBounds(targetBounds);
            return;
        }

        sizeAnimator = ValueAnimatorBuilder { }
                           .withEasing(Easings::createEaseInOut())
                           .withDurationMs(180)
                           .withValueChangedCallback([this, fromBounds, targetBounds](auto v) {
                               auto start = std::make_tuple(fromBounds.getX(), fromBounds.getY(), fromBounds.getWidth(), fromBounds.getHeight());
                               auto end = std::make_tuple(targetBounds.getX(), targetBounds.getY(), targetBounds.getWidth(), targetBounds.getHeight());
                               auto const [x, y, w, h] = makeAnimationLimits(start, end).lerp(v);
                               setBounds(x, y, w, h);
                           })
                           .build();

        animatorUpdater.addAnimator(sizeAnimator, [this]() {
            for (auto* btn : helperButtons)
                btn->setVisible(hasInputFocus);
        });
        sizeAnimator.start();
    }

    static int countBraces(String const& text)
    {
        int braceCount = 0;

        for (int i = 0; i < text.length(); ++i) {
            juce_wchar const currentChar = text[i];

            if (currentChar == '{') {
                ++braceCount; // Increment for each '{'
            } else if (currentChar == '}') {
                --braceCount; // Decrement for each '}'
            }
        }

        return braceCount;
    }

    void updateCommandInputTarget()
    {
        auto name = String("empty");
        if (auto* cnv = editor->getCurrentCanvas()) {
            auto objects = cnv->getSelectionOfType<Object>();
            if (objects.size() == 1) {
                name = objects[0]->getType();
            } else if (objects.size() > 1) {
                name = "(" + String(objects.size()) + " selected)";
            }
        }

        setConsoleTargetName(name);
    }

    static UnorderedMap<String, Object*> getUniqueObjectNames(Canvas* cnv)
    {
        UnorderedMap<String, Object*> result;
        UnorderedMap<String, int> nameCount;
        for (auto* object : cnv->objects) {
            if (!object->gui)
                continue;

            auto type = object->gui->getType();
            if (allAtoms.contains(type)) {
                auto& found = nameCount[type];
                found++;
                result[type + "_" + String(found)] = object;
            } else {
                auto tokens = StringArray::fromTokens(object->gui->getText(), false);
                if (tokens.size()) {
                    auto const isGui = allGuis.contains(tokens[0]);
                    auto const numArgs = std::min(isGui ? 1 : 2, tokens.size());
                    tokens.removeRange(numArgs, tokens.size() - numArgs);

                    auto uniqueName = tokens.joinIntoString("_");
                    auto& found = nameCount[uniqueName];
                    found++;

                    result[uniqueName + "_" + String(found)] = object;
                }
            }
        }

        return result;
    }

    static SmallArray<Object*> findObjects(Canvas* cnv, String const& name)
    {
        SmallArray<Object*> found;
        auto names = getUniqueObjectNames(cnv);
        if (name.endsWith("*")) {
            auto const wildcard = name.upToLastOccurrenceOf("*", false, false);
            for (auto [name, ptr] : names) {
                if (name.contains(wildcard)) {
                    found.add(ptr);
                }
            }
        }
        if (names.contains(name)) {
            found.add(names[name]);
        }
        return found;
    }

    String parseExpressions(String const& message)
    {
        String parsedMessage;
        int startPos = 0;

        // if the lua expression is not at the start of the message, we expect a return value
        auto const hasReturnValue = !message.trim().startsWith("{") || consoleTargetName != ">";

        while (startPos < message.length()) {
            auto const openBrace = message.indexOf(startPos, "{");

            if (openBrace == -1) {
                parsedMessage += message.substring(startPos);
                break;
            }

            parsedMessage += message.substring(startPos, openBrace);

            auto const closeBrace = message.indexOf(openBrace, "}");
            if (closeBrace == -1) {
                editor->pd->logError("Unmatched '{' in expression.");
                parsedMessage += message.substring(openBrace); // Append remaining message as-is
                break;
            }

            lua->setCommandProcessor(this);
            String luaExpression = message.substring(openBrace + 1, closeBrace);
            auto result = lua->executeExpression(luaExpression, hasReturnValue);

            if (auto const doubleResult = std::get_if<double>(&result)) {
                parsedMessage += String(*doubleResult);
            } else if (auto const stringResult = std::get_if<String>(&result)) {
                parsedMessage += *stringResult;
            }

            startPos = closeBrace + 1;
        }

        return parsedMessage;
    }

    void showHelp()
    {
        auto markupDisplay = std::make_unique<MarkupDisplay::MarkupDisplayComponent>();
        markupDisplay->setURLHandler(this);
        markupDisplay->setFont(Fonts::getDefaultFont());
        markupDisplay->setColour(PlugDataColour::canvasBackgroundColourId, PlugDataColours::levelMeterBackgroundColour);
        markupDisplay->setMarkupString(documentationString);
        markupDisplay->setSize(250, 200);
        markupDisplay->setVisible(true);
        editor->showCalloutBox(std::move(markupDisplay), getScreenBounds().withSizeKeepingCentre(5, 30));
    }

    SmallArray<std::pair<int, String>> executeCommand(pd::Instance* pd, String message) override
    {
        SmallArray<std::pair<int, String>> result;

        message = parseExpressions(message.trim());

        auto argv = StringArray::fromTokens(message, true);

        // Wrapped editor->getCurrentCanvas() to make it easier to log null canvas error
        auto getCurrentCanvas = [this, pd](bool const logError = false) -> Canvas* {
            if (Canvas* cnv = editor->getCurrentCanvas())
                return cnv;

            if (logError)
                pd->logError("No canvas open");

            return nullptr;
        };

        // Post error if argv has only one arg (argv can change during command execution)
        auto isObjectNameProvided = [pd](StringArray const& argv) -> bool {
            if (argv.size() == 1) {
                pd->logError("No object query provided");
                return false;
            }
            return true;
        };

        // Global or canvas message
        if (!argv[0].startsWith(";") && (consoleTargetName == ">" || consoleTargetName == "lua >" || argv[0] == ">" || argv[0] == "deselect" || argv[0] == "clear")) {
            switch (hash(argv[0])) {
            case hash("sel"):
            case hash("select"): {
                if (auto* cnv = getCurrentCanvas(true); isObjectNameProvided(argv) && cnv) {
                    if (argv[1].containsOnly("0123456789")) {
                        int index = argv[1].getIntValue();
                        if (index >= 0 && index < cnv->objects.size()) {
                            // move the window if it needs to be moved
                            editor->highlightSearchTarget(cnv->objects[index]->getPointer(), true);
                            cnv->updateSidebarSelection();
                        } else {
                            result.add({ 1, "Object index out of bounds" });
                        }
                    } else {
                        auto objects = findObjects(cnv, argv[1]);
                        for (auto* object : objects) {
                            cnv->setSelected(object, true);
                            cnv->updateSidebarSelection();
                        }
                        if (objects.empty())
                            pd->logError("No object found for: " + argv[1]);
                        // TODO: fix highlighting!
                        // if(objects.size()) editor->highlightSearchTarget(objects[0]->getPointer(), true);
                    }
                    updateCommandInputTarget();
                }
                break;
            }
            case hash(">"):
            case hash("deselect"): {
                if (auto* cnv = getCurrentCanvas(true)) {
                    cnv->deselectAll();
                    cnv->updateSidebarSelection();
                }
                updateCommandInputTarget();
                break;
            }
            case hash("ls"):
            case hash("list"): {
                if (auto* cnv = getCurrentCanvas(true)) {
                    auto names = getUniqueObjectNames(cnv);
                    for (auto& [name, object] : names) {
                        if (allGuis.contains(object->gui->getType())) {
                            result.add({ 0, name });
                        } else {
                            result.add({ 0, name + ": " + object->gui->getText() });
                        }
                    }
                }
                break;
            }
            case hash("find"):
            case hash("search"): {
                if (auto* cnv = getCurrentCanvas(true); isObjectNameProvided(argv) && cnv) {
                    auto names = getUniqueObjectNames(cnv);
                    for (auto& [name, object] : names) {
                        auto query = argv[1];
                        query = query.trimCharactersAtEnd("*"); // No need for wildcards here
                        auto text = object->gui->getText();
                        if (text.contains(query) || name.contains(query)) {
                            if (allGuis.contains(object->gui->getType())) {
                                result.add({ 0, name });
                            } else {
                                result.add({ 0, name + ": " + object->gui->getText() });
                            }
                        }
                    }
                }
                break;
            }
            case hash("reset"): {
                // Reset lua context
                luas[editor->pd] = std::make_unique<LuaExpressionParser>(editor->pd);
                lua = luas[editor->pd].get();
                break;
            }
            case hash("clear"): {
                commandHistory.clear();
                editor->consolePanel->clear();
                if (auto* cnv = getCurrentCanvas()) {
                    cnv->deselectAll();
                    cnv->updateSidebarSelection();
                }
                updateCommandInputTarget();
                break;
            }
            case hash("cnv"):
            case hash("canvas"): {
                if (auto* cnv = getCurrentCanvas(true)) {
                    auto patchPtr = cnv->patch.getPointer();
                    if (patchPtr && argv.size() == 1 && argv[1].containsOnly("0123456789-e.")) {
                        pd->sendDirectMessage(patchPtr.get(), argv[1].getFloatValue());
                    } else if (patchPtr && argv.size() == 1) {
                        pd->sendDirectMessage(patchPtr.get(), argv[1], { });
                    } else if (patchPtr) {
                        SmallArray<pd::Atom> atoms;
                        for (int i = 2; i < argv.size(); i++) {
                            if (argv[i].containsOnly("0123456789-e.")) {
                                atoms.add(pd::Atom(argv[i].getFloatValue()));
                            } else {
                                atoms.add(pd::Atom(pd->generateSymbol(argv[i])));
                            }
                        }
                        pd->sendDirectMessage(patchPtr.get(), argv[1], std::move(atoms));
                    }
                    cnv->patch.deselectAll();
                }
                break;
            }
            case hash("script"): {
                auto script = pd::Library::findFile(argv[1] + ".lua");
                if (script.existsAsFile()) {
                    lua->executeScript(script.getFullPathName());
                } else {
                    pd->logError("Script not found");
                }
                break;
            }
            case hash("man"): {
                switch (hash(argv[1])) {
                case hash("man"):
                    pd->logMessage("Prints manual for command. Usage: man <command>");
                    break;

                case hash("?"):
                case hash("help"):
                    pd->logMessage(argv[1] + ": Show help");
                    break;

                case hash("script"):
                    pd->logMessage(argv[1] + ": Excute a Lua script from your search path. Usage: script <filename>");
                    break;

                case hash("pd"):
                    pd->logMessage(argv[1] + ": Send a message to pd. Usage: " + argv[1] + " <message>");
                    break;

                case hash("cnv"):
                case hash("canvas"):
                    pd->logMessage(argv[1] + ": Send a message to current canvas. Usage: " + argv[1] + " <message>");
                    break;

                case hash("clear"):
                    pd->logMessage(argv[1] + ": Clear console and command history");
                    break;

                case hash("reset"):
                    pd->logMessage(argv[1] + ": Reset Lua interpreter state");
                    break;

                case hash("sel"):
                case hash("select"):
                    pd->logMessage(argv[1] + ": Select an object by ID or index. After selecting objects, you can send messages to them. Usage: " + argv[1] + " <id> or " + argv[1] + " <index>");
                    break;

                case hash(">"):
                case hash("deselect"):
                    pd->logMessage(argv[1] + ": Deselects all on current canvas");
                    break;

                case hash("ls"):
                case hash("list"):
                    pd->logMessage(argv[1] + ": Print a list of all object IDs on current canvas");
                    break;

                case hash("find"):
                case hash("search"):
                    pd->logMessage(argv[1] + ": Search object IDs on current canvas. Usage: " + argv[1] + " <id>.");
                    break;
                default:
                    pd->logMessage("man: No manual for command: " + argv[1]);
                    break;
                }
                break;
            }
            case hash("?"):
            case hash("help"): {
                showHelp();
                break;
            }
            default: {
                // Match a  "name > message" pattern
                if (argv.size() >= 2 && argv[1] == ">") {
                    auto target = argv[0];
                    if (argv.size() == 2) {
                        if (auto* cnv = getCurrentCanvas(true)) {
                            auto objects = findObjects(cnv, target);
                            for (auto* object : objects) {
                                cnv->setSelected(object, true);
                                cnv->updateSidebarSelection();
                            }
                            if (objects.empty())
                                pd->logError("No object found for: " + argv[1]);
                        }
                        break;
                    }

                    argv.removeRange(0, 2);

                    if (auto* cnv = getCurrentCanvas(true)) {
                        auto objects = findObjects(cnv, target);
                        for (auto* object : objects) {
                            auto objPtr = object->getPointer();
                            if (objPtr && argv.size() && argv[0].containsOnly("0123456789-e.")) {
                                pd->sendDirectMessage(objPtr, argv[0].getFloatValue());
                            } else if (objPtr && argv.size()) {
                                pd->sendDirectMessage(objPtr, argv[0], { });
                            } else if (objPtr) {
                                SmallArray<pd::Atom> atoms;
                                for (int i = 1; i < argv.size(); i++) {
                                    if (argv[i].containsOnly("0123456789-e.")) {
                                        atoms.add(pd::Atom(argv[i].getFloatValue()));
                                    } else {
                                        atoms.add(pd::Atom(pd->generateSymbol(argv[i])));
                                    }
                                }
                                pd->sendDirectMessage(objPtr, argv[0], std::move(atoms));
                            }
                        }
                        if (objects.empty())
                            pd->logError("No object found for: " + argv[1]);
                    }
                }

                if (!argv.size())
                    break;
                argv.getReference(0) = argv[0].trimCharactersAtStart(";");
                SmallArray<pd::Atom> atoms;
                for (int i = 2; i < argv.size(); i++) {
                    if (argv[i].containsOnly("0123456789-e.")) {
                        atoms.add(pd::Atom(argv[i].getFloatValue()));
                    } else {
                        atoms.add(pd::Atom(pd->generateSymbol(argv[i])));
                    }
                }
                pd->lockAudioThread();
                pd->sendMessage(argv[0].toRawUTF8(), argv[1].toRawUTF8(), std::move(atoms));
                pd->unlockAudioThread();
                break;
            }
            }
        } else { // object message
            if (auto* cnv = getCurrentCanvas(true)) {
                for (auto* obj : cnv->getSelectionOfType<Object>()) {
                    if (argv.size() == 1 && argv[0].containsOnly("0123456789-e.")) {
                        pd->sendDirectMessage(obj->getPointer(), argv[0].getFloatValue());
                    } else if (argv.size() == 1) {
                        pd->sendDirectMessage(obj->getPointer(), argv[0], { });
                    } else {
                        SmallArray<pd::Atom> atoms;
                        for (int i = 1; i < argv.size(); i++) {
                            if (argv[i].containsOnly("0123456789-e.")) {
                                atoms.add(pd::Atom(argv[i].getFloatValue()));
                            } else {
                                atoms.add(pd::Atom(pd->generateSymbol(argv[i])));
                            }
                        }
                        pd->sendDirectMessage(obj->getPointer(), argv[0], std::move(atoms));
                    }
                }
            }
        }
        return result;
    }

    ~CommandInput() override
    {
        Desktop::getInstance().removeFocusChangeListener(this);
        onDismiss();
    }

    void globalFocusChanged(Component* focusedComponent) override
    {
        bool const focused = focusedComponent != nullptr
            && (focusedComponent == &commandInput || isParentOf(focusedComponent));

        if (focused == hasInputFocus)
            return;

        hasInputFocus = focused;
        for (auto* btn : helperButtons)
            btn->setVisible(false);

        updateSize(true); // animate on focus change
    }

    void handleURL(String const& url) override // when documentation links or codeblocks are clicked
    {
        commandInput.setText(url);
        commandInput.moveCaretToEnd();
    }

    void paintOverChildren(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        int const inputHeight = std::max(commandInput.getTextHeight() + 4, 30);
        auto const inputRow = bounds.removeFromBottom(inputHeight);

        g.setColour(PlugDataColours::sidebarTextColour);
        g.setFont(Fonts::getSemiBoldFont().withHeight(15));
        g.drawText(consoleTargetName, inputRow.getX() + 9, inputRow.getY(),
            consoleTargetLength, inputRow.getHeight() - 1, Justification::centredLeft);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        g.setColour(PlugDataColours::levelMeterBackgroundColour);
        g.fillRoundedRectangle(bounds.reduced(2, 1).toFloat(), Corners::defaultCornerRadius);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int const inputHeight = std::max(commandInput.getTextHeight() + 4, 30);

        if (hasInputFocus) {
            auto const helperSpace = std::max(0, bounds.getHeight() - inputHeight);
            auto helperBounds = bounds.removeFromTop(helperSpace);
            helperBounds.removeFromLeft(8);
            for (auto* btn : helperButtons) {
                auto const w = CachedStringWidth<14>::calculateStringWidth(btn->getButtonText()) + 15;
                btn->setBounds(helperBounds.removeFromLeft(w).reduced(1, 3));
            }
        }

        auto const clearBounds = bounds.removeFromRight(30).removeFromBottom(inputHeight);
        clearButton.setBounds(clearBounds);
        commandInput.setBounds(bounds.withTrimmedLeft(consoleTargetLength + 4).removeFromBottom(inputHeight));
    }

    void setConsoleTargetName(String const& target)
    {
        consoleTargetName = target + " >";
        if (target == "empty")
            consoleTargetName = ">";
        consoleTargetLength = CachedStringWidth<15>::calculateStringWidth(consoleTargetName) + 4;
        commandInput.setBounds(commandInput.getBounds().withLeft(consoleTargetLength + 4));

        updateHelperCommands();
        repaint();
    }

    void setHistoryCommand()
    {
        if (currentHistoryIndex < 0) {
            commandInput.setText("");
            currentHistoryIndex = -1;
        } else if (currentHistoryIndex < commandHistory.size()) {
            auto const command = commandHistory[currentHistoryIndex];
            if (command.containsChar('\n'))
                setConsoleTargetName("lua");
            else
                updateCommandInputTarget();
            commandInput.setText(command);
        } else {
            currentHistoryIndex = commandHistory.size() - 1;
        }
        commandInput.moveCaretToEnd();
    }

    bool keyPressed(KeyPress const& key, Component*) override
    {
        if (key.getKeyCode() == KeyPress::returnKey && key.getModifiers().isShiftDown()) {
            commandInput.insertTextAtCaret("\n");
            updateSize();
            return true;
        }
        if (key.getKeyCode() == KeyPress::upKey && !commandInput.getText().containsChar('\n')) {
            currentHistoryIndex++;
            setHistoryCommand();
            return true;
        }
        if (key.getKeyCode() == KeyPress::downKey && !commandInput.getText().containsChar('\n')) {
            currentHistoryIndex--;
            setHistoryCommand();
            return true;
        }
        if (key.getKeyCode() == KeyPress::spaceKey) {
            commandInput.insertTextAtCaret(" ");
            return true;
        }
        // Allow delete or backspace key to delete single chars or whole words if ctrl is down
        if (key.getKeyCode() == KeyPress::backspaceKey || key.getKeyCode() == KeyPress::deleteKey) {
            commandInput.deleteBackwards(key.getModifiers().isCtrlDown());
            return true;
        }
        // Use default commandID mappings for other keys
        if (auto const* keyMappings = editor->commandManager.getKeyMappings()) {
            if (auto const commandID = keyMappings->findCommandForKeyPress(key)) {
                editor->commandManager.invokeDirectly(commandID, false);
                return true;
            }
        }
        return false;
    }

private:
    PluginEditor* editor;
    static inline auto luas = UnorderedMap<pd::Instance*, std::unique_ptr<LuaExpressionParser>>();
    LuaExpressionParser* lua;

    int consoleTargetLength = 10;
    String consoleTargetName = ">";

    int currentHistoryIndex = -1;
    static inline std::deque<String> commandHistory;
    static inline String currentCommand;

    TextEditor commandInput;
    SmallIconButton clearButton = SmallIconButton(Icons::ClearText);
    SmallIconButton helpButton = SmallIconButton(Icons::Help);

    OwnedArray<TextButton> helperButtons;
    bool hasInputFocus = false;
    static constexpr int helperRowHeight = 26;

    VBlankAnimatorUpdater animatorUpdater { this };
    Animator sizeAnimator = ValueAnimatorBuilder { }.build();

    static inline StringArray const helperCommands = {
        "help", "man", "ls", "sel", "cnv", "pd"
    };
    static inline StringArray const objectHelperCommands = {
        "deselect",
    };

    static inline String documentationString = {
        "Command input allows you to quickly send commands to objects, pd or the canvas.\n"
        "The following commands are available:\n"
        "\n"
        "- man <command>: print manual for command\n"
        "- list/ls: list all object IDs in the current canvas\n"
        "- search: search for an object ID in current canvas\n"
        "- select/sel <id>: selects an object\n"
        "- deselect: deselect all objects\n"
        "- clear: clears console and command state\n"
        "- reset: clear lua state\n"
        "- canvas: send message to canvas\n"
        "    - canvas obj <x> <y> <name>: create text object\n"
        "    - canvas msg <x> <y> <name>: create message object\n"
        "- script: load lua script from search path\n"
        "- pd: send a message to pd, for example:\n"
        "    - pd dsp <int>: set DSP state\n"
        "    - pd pluginmode: enter plugin mode\n"
        "    - pd quit: quit plugdata\n"
        "- <id> > <message>: sends message to object, for example:\n"
        "    ```tgl_1 > 1```\n to send a 1 to toggle object 1\n"
        "\n"
        "Once you have selected an object, all messages you send become direct messages to that object. So to send a float to a toggle, select it, and enter \"1\". You can deselect an object with \"deselect\", or the shorthand \">\"\n"
        "\n"
        "You can also do dynamic patching with the \"canvas\" command.\n"
        "\n"
        "```canvas obj 20 50 metro 200```\n"
        "\n"
        "This will create a \"metro 200\" object at the 20,50 coordinate.\n"
        "\n"
        "\n"
        "You can also use Lua expressions to generate values, or automate tasks. Lua expressions are written inside brackets. For example, to randomise the colour of a toggle, select a toggle and enter:\n"
        "\n"
        "```color {math.random() * 200}```\n"
        "\n"
        "Lua can also call back to the command input by calling pd.eval(\"your command here\"). For example:\n"
        "\n"
        "```{ pd.eval(\"sel tgl_1\") }```\n"
        "\n"
        "Will select the first toggle from within lua, which you could then send more messages to from Lua\n"
        "\n"
        "To write multi-line lua expressions, leave an open bracket and hit enter, so you could write:\n"
        "\n"
        "```{\n"
        "for i = 1, 20 do\n"
        "    for j = 1, 20 do\n"
        "        pd.eval(\"canvas obj \" .. tostring(i * 28) .. \" \" .. tostring(j * 28) .. \" tgl\")\n"
        "    end\n"
        "end\n"
        "}```\n"
        "\n"
        "To generate a 20x20 grid of toggle objects."
    };

public:
    std::function<void()> onDismiss = [] { };

    static inline UnorderedSet<String> const allAtoms = { "floatbox", "symbolbox", "listbox", "gatom" };

    static inline UnorderedSet<String> const allGuis = { "bng",
        "hsl",
        "vsl",
        "slider",
        "tgl",
        "nbx",
        "vradio",
        "hradio",
        "vu",
        "cnv",
        "keyboard",
        "pic",
        "scope~",
        "function",
        "note",
        "knob",
        "message",
        "comment",
        "canvas",
        "bicoeff",
        "messbox",
        "pad",
        "button" };
};
