/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <utility>
extern "C"
{
#include <pd-lua/lua/lua.h>
#include <pd-lua/lua/lauxlib.h>
#include <pd-lua/lua/lualib.h>
}

#include "Components/BouncingViewport.h"
#include "Object.h"
#include "PluginEditor.h"
#include "Objects/ObjectBase.h"
#include "Sidebar/Sidebar.h"

class CommandProcessor
{
public:
    virtual SmallArray<std::pair<int, String>> executeCommand(pd::Instance* pd, String message) = 0;
};

class LuaExpressionParser {
public:
    using LuaResult = std::variant<double, String>;

    // Constructor: initialize Lua
    LuaExpressionParser(pd::Instance* pd)
        : pd(pd)
    {
        L = luaL_newstate();
        luaL_openlibs(L); // Load Lua standard libraries

        luaopen_math(L);

        // Create the global "pd" table and set up "pd.post"
        lua_newtable(L); // Create a new table for "pd"

        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, LuaExpressionParser::luaPost, 1);  // 1 upvalue for "pd"
        lua_setfield(L, -2, "post");  // Sets pd.post in the table

        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, LuaExpressionParser::luaEval, 1); // 1 upvalue for "LuaExpressionParser"
        lua_setfield(L, -2, "eval"); // Sets pd.eval in the table

        lua_setglobal(L, "pd");  // Set the table as the global "pd"
    }

    // Destructor: close Lua
    ~LuaExpressionParser()
    {
        if (L) {
            lua_close(L);
        }
    }

    // Function to execute an expression and return result as LuaResult (either double or string)
    LuaResult executeExpression(String const& expression)
    {
        UnorderedSet<String> luaKeywords = {"if", "for", "while", "function", "local"};

        auto trimmedExpression = expression.trim();
        auto firstWord = trimmedExpression.upToFirstOccurrenceOf(" ", false, false);

        // Determine if the expression is a block by checking the first word
        bool isBlock = luaKeywords.contains(firstWord);
        
        String luaCode = "local __eval = function()";
        if(!isBlock) luaCode += "\nreturn ";
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
            } else if (lua_isstring(L, -1)) {
                String result = lua_tostring(L, -1);
                lua_pop(L, 1);  // Remove result from stack
                return result;
            } else {
                lua_pop(L, 1); // Remove nil result
                return "";     // Default to empty string if not a number or string
            }
        } else {
            char const* error = lua_tostring(L, -1);
            pd->logError("Lua error: " + String::fromUTF8(error));
            lua_pop(L, 1); // Remove error message from stack
            return "";     // Return empty string on error
        }
    }

    static int luaPost(lua_State* L)
    {
        // Retrieve the LuaWrapper instance (via userdata, upvalues, etc.)
        auto* pd = reinterpret_cast<LuaExpressionParser*>(lua_touserdata(L, lua_upvalueindex(1)))->pd;

        if (lua_isstring(L, 1)) {
            String message = lua_tostring(L, 1);
            pd->logMessage(message); // Use Pd instance to post message
        } else {
            pd->logError("pd.post requires a string argument");
        }
        return 0;
    }
    
    static int luaEval(lua_State* L)
    {
        auto* parser = reinterpret_cast<LuaExpressionParser*>(lua_touserdata(L, lua_upvalueindex(1)));
        
        if (lua_isstring(L, 1)) {
            String command = lua_tostring(L, 1);
            auto result = parser->commandInput->executeCommand(parser->pd, command);  // Execute the command
            
            // Create a Lua table to store result messages
            lua_newtable(L); // Creates an empty table on the stack
            
            int index = 1;
            for (const auto& [type, string] : result) {
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

class CommandInput final : public Component
    , public KeyListener, public CommandProcessor {
public:
    CommandInput(PluginEditor* editor) : editor(editor)
    {
        if(!luas.contains(editor->pd))
        {
            luas[editor->pd] = std::make_unique<LuaExpressionParser>(editor->pd);
        }
        lua = luas[editor->pd].get();
        
        updateCommandInputTarget();

        commandInput.onTextChange = [this](){
            updateSize();
        };
        
        commandInput.onFocusLost = [editor](){
            if(editor->calloutArea && editor->calloutArea->isOnDesktop()) {
                editor->calloutArea->removeFromDesktop();
            }
        };

        commandInput.onReturnKey = [this, pd = editor->pd]() {
            auto text = commandInput.getText();
            
            if(countBraces(text) > 0)
            {
                commandInput.setMultiLine(true);
                commandInput.insertTextAtCaret("\n");
                setConsoleTargetName("lua");
                updateSize();
                return;
            }
            
            auto result = executeCommand(pd, text);
            for(auto& [type, message] : result)
            {
                if(type == 0)
                {
                    pd->logMessage(message);
                }
                else {
                    pd->logError(message);
                }
            }
            auto isUniqueCommand = commandHistory.empty() ? true : commandHistory.front() != commandInput.getText();
            if(!commandInput.isEmpty() && isUniqueCommand) {
                commandHistory.push_front(commandInput.getText());
                currentHistoryIndex = -1;
            }
            commandInput.clear();
            commandInput.setMultiLine(false);
            updateCommandInputTarget();
            updateClearButtonTooltip();
            updateSize();
        };

        addAndMakeVisible(commandInput);
        addAndMakeVisible(clearButton);

        updateClearButtonTooltip();

        clearButton.setWantsKeyboardFocus(false);
        clearButton.onClick = [this](){
            commandHistory.clear();
            commandInput.clear();
            commandInput.setMultiLine(false);
            updateCommandInputTarget();
            updateClearButtonTooltip();
            updateSize();
        };

        commandInput.setBorder({3, 3, 0, 0});
        commandInput.addKeyListener(this);
        commandInput.addMouseListener(this, false);
        commandInput.setFont(Fonts::getDefaultFont().withHeight(15));

        commandInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        commandInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        commandInput.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    }
        
    void updateSize()
    {
        auto width = std::clamp(commandInput.getTextWidth() + consoleTargetLength + 30, getWidth(), 400);
        setSize(width, commandInput.getTextHeight() + 12);
    }
        
    int countBraces(String const& text)
    {
        int braceCount = 0;

        for (int i = 0; i < text.length(); ++i)
        {
            juce_wchar currentChar = text[i];

            if (currentChar == '{')
            {
                ++braceCount;  // Increment for each '{'
            }
            else if (currentChar == '}')
            {
                --braceCount;  // Decrement for each '}'
            }
        }

        return braceCount;
    }

    void updateClearButtonTooltip()
    {
        clearButton.setTooltip("Clear command history: " + String(commandHistory.size()));
    }

    void updateCommandInputTarget()
    {
        auto name = String("empty");
        if(auto* cnv = editor->getCurrentCanvas()) {
            auto objects = cnv->getSelectionOfType<Object>();
            if (objects.size() == 1) {
                name = objects[0]->getType();
            } else if (objects.size() > 1){
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
                    auto isGui = allGuis.contains(tokens[0]);
                    auto numArgs = std::min(isGui ? 1 : 2, tokens.size());
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
        
    SmallArray<Object*> findObjects(Canvas* cnv, String const& name)
    {
        SmallArray<Object*> found;
        auto names = getUniqueObjectNames(cnv);
        if(name.endsWith("*"))
        {
            auto wildcard = name.upToLastOccurrenceOf("*", false, false);
            for(auto [name, ptr] : names)
            {
                if(name.contains(wildcard))
                {
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

        while (startPos < message.length()) {
            auto openBrace = message.indexOf(startPos, "{");

            if (openBrace == -1) {
                parsedMessage += message.substring(startPos);
                break;
            }

            parsedMessage += message.substring(startPos, openBrace);

            auto closeBrace = message.indexOf(openBrace, "}");
            if (closeBrace == -1) {
                editor->pd->logError("Unmatched '{' in expression.");
                parsedMessage += message.substring(openBrace); // Append remaining message as-is
                break;
            }

            lua->setCommandProcessor(this);
            String luaExpression = message.substring(openBrace + 1, closeBrace);
            auto result = lua->executeExpression(luaExpression);

            if (auto doubleResult = std::get_if<double>(&result)) {
                parsedMessage += String(*doubleResult);
            } else if (auto stringResult = std::get_if<String>(&result)) {
                parsedMessage += *stringResult;
            }

            startPos = closeBrace + 1;
        }

        return parsedMessage;
    }

    SmallArray<std::pair<int, String>> executeCommand(pd::Instance* pd, String message) override
    {
        SmallArray<std::pair<int, String>> result;
        
        message = parseExpressions(message);

        auto tokens = StringArray::fromTokens(message, true);

        // Global or canvas message
        if (!tokens[0].startsWith(";") && (consoleTargetName == ">" || consoleTargetName == "lua >" || tokens[0] == ">" || tokens[0] == "deselect" || tokens[0] == "clear"))
        {
            auto selector = hash(tokens[0]);
            switch (selector) {
            case hash("sel"):
            case hash("select"): {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    if (tokens[1].containsOnly("0123456789")) {
                        int index = tokens[1].getIntValue();
                        if (index >= 0 && index < cnv->objects.size()) {
                            cnv->setSelected(cnv->objects[index], true);
                            cnv->updateSidebarSelection();
                        } else {
                            result.add({1, "Object index out of bounds"});
                        }
                    } else {
                        for(auto* object : findObjects(cnv, tokens[1])) {
                            cnv->setSelected(object, true);
                            cnv->updateSidebarSelection();
                        }
                    }
                }
                updateCommandInputTarget();
                break;
            }
            case hash(">"):
            case hash("deselect"): {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    cnv->deselectAll();
                    cnv->updateSidebarSelection();
                }
                updateCommandInputTarget();
                break;
            }
            case hash("ls"):
            case hash("list"): {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    auto names = getUniqueObjectNames(cnv);
                    for (auto& [name, object] : names) {
                        if (allGuis.contains(object->gui->getType())) {
                            result.add({0, name});
                        } else {
                            result.add({0, name + ": " + object->gui->getText()});
                        }
                    }
                }
                break;
            }
            case hash("find"):
            case hash("search"): {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    auto names = getUniqueObjectNames(cnv);
                    for (auto& [name, object] : names) {
                        auto query = tokens[1];
                        query = query.trimCharactersAtEnd("*"); // No need for wildcards here
                        auto text = object->gui->getText();
                        if (text.contains(query) || name.contains(query)) {
                            if (allGuis.contains(object->gui->getType())) {
                                result.add({0, name});
                            } else {
                                result.add({0, name + ": " + object->gui->getText()});
                            }
                        }
                    }
                }

                break;
            }
            case hash("clear"): {
                // Reset lua context
                luas[editor->pd] = std::make_unique<LuaExpressionParser>(editor->pd);
                lua = luas[editor->pd].get();
                
                editor->sidebar->clearConsole();
                if (auto* cnv = editor->getCurrentCanvas()) {
                    cnv->deselectAll();
                    cnv->updateSidebarSelection();
                }
                updateCommandInputTarget();
                break;
            }
            case hash("cnv"):
            case hash("canvas"):
            {
                if(auto* cnv = editor->getCurrentCanvas())
                {
                    auto patchPtr = cnv->patch.getPointer();
                    if (patchPtr && tokens.size() == 1 && tokens[1].containsOnly("0123456789-e.")) {
                        pd->sendDirectMessage(patchPtr.get(), tokens[1].getFloatValue());
                    } else if (patchPtr && tokens.size() == 1) {
                        pd->sendDirectMessage(patchPtr.get(), tokens[1], {});
                    } else if(patchPtr) {
                        SmallArray<pd::Atom> atoms;
                        for (int i = 2; i < tokens.size(); i++) {
                            if (tokens[i].containsOnly("0123456789-e.")) {
                                atoms.add(pd::Atom(tokens[i].getFloatValue()));
                            } else {
                                atoms.add(pd::Atom(pd->generateSymbol(tokens[i])));
                            }
                        }
                        pd->sendDirectMessage(patchPtr.get(), tokens[1], std::move(atoms));
                    }
                    cnv->patch.deselectAll();
                }
            }
            default: {
                if(!tokens.size()) break;
                tokens.getReference(0) = tokens[0].trimCharactersAtStart(";");
                SmallArray<pd::Atom> atoms;
                for (int i = 2; i < tokens.size(); i++) {
                    if (tokens[i].containsOnly("0123456789-e.")) {
                        atoms.add(pd::Atom(tokens[i].getFloatValue()));
                    } else {
                        atoms.add(pd::Atom(pd->generateSymbol(tokens[i])));
                    }
                }
                pd->sendMessage(tokens[0].toRawUTF8(), tokens[1].toRawUTF8(), std::move(atoms));
                break;
            }
            }
        } else { // object message
            if (auto* cnv = editor->getCurrentCanvas()) {
                for (auto* obj : cnv->getSelectionOfType<Object>()) {
                    if (tokens.size() == 1 && tokens[0].containsOnly("0123456789-e.")) {
                        pd->sendDirectMessage(obj->getPointer(), tokens[0].getFloatValue());
                    } else if (tokens.size() == 1) {
                        pd->sendDirectMessage(obj->getPointer(), tokens[0], {});
                    } else {
                        SmallArray<pd::Atom> atoms;
                        for (int i = 1; i < tokens.size(); i++) {
                            if (tokens[i].containsOnly("0123456789-e.")) {
                                atoms.add(pd::Atom(tokens[i].getFloatValue()));
                            } else {
                                atoms.add(pd::Atom(pd->generateSymbol(tokens[i])));
                            }
                        }
                        pd->sendDirectMessage(obj->getPointer(), tokens[0], std::move(atoms));
                    }
                    
                }
            }
        }
                    
       return result;
    }

    ~CommandInput() override = default;

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 0, getWidth(), 0);

        g.setColour(findColour(PlugDataColour::dataColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15));
        g.drawText(consoleTargetName, 7, 0, consoleTargetLength, getHeight() - 4, Justification::centredLeft);
    }
        
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::levelMeterBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(2, 2).toFloat(), Corners::defaultCornerRadius);
    }

    void resized() override
    {
        commandInput.setBounds(getLocalBounds().withTrimmedLeft(consoleTargetLength).withTrimmedRight(30));
        auto buttonBounds = getLocalBounds().removeFromRight(30);
        clearButton.setBounds(buttonBounds);
    }

    void setConsoleTargetName(String const& target)
    {
        consoleTargetName = target + " >";
        if (target == "empty")
            consoleTargetName = ">";
        consoleTargetLength = CachedStringWidth<15>::calculateStringWidth(consoleTargetName) + 4;
        commandInput.setBounds(commandInput.getBounds().withLeft(consoleTargetLength));
        repaint();
    }

    void setHistoryCommand()
    {
        if (currentHistoryIndex < 0) {
            commandInput.setText("");
            currentHistoryIndex = -1;
        } else if (currentHistoryIndex < commandHistory.size()) {
            auto command = commandHistory[currentHistoryIndex];
            auto isMultiLine = command.containsChar('\n');
            commandInput.setMultiLine(isMultiLine);
            if(isMultiLine) setConsoleTargetName("lua");
            else updateCommandInputTarget();
            commandInput.setText(command);
        } else {
            currentHistoryIndex = commandHistory.size() - 1;
        }
    }

    bool keyPressed(KeyPress const& key, Component*) override
    {
        if(commandInput.isMultiLine()) return false;
        
        if (key.getKeyCode() == KeyPress::upKey) {
            currentHistoryIndex++;
            setHistoryCommand();
            return true;
        } else if (key.getKeyCode() == KeyPress::downKey) {
            currentHistoryIndex--;
            setHistoryCommand();
            return true;
        }
        return false;
    }

    static std::deque<String>& getCommandHistory()
    {
        return commandHistory;
    }

    static void setCommandHistory(StringArray& commands)
    {
        commandHistory.clear();

        for (auto& command : commands){
            commandHistory.push_back(command);
        }
    };

    private:
    PluginEditor* editor;
    static inline UnorderedMap<pd::Instance*, std::unique_ptr<LuaExpressionParser>> luas = UnorderedMap<pd::Instance*, std::unique_ptr<LuaExpressionParser>>();
    LuaExpressionParser* lua;
    
    int consoleTargetLength = 10;
    String consoleTargetName = ">";

    int currentHistoryIndex = -1;
    static inline std::deque<String> commandHistory;

    TextEditor commandInput;
    SmallIconButton clearButton = SmallIconButton(Icons::ClearText);

    static inline UnorderedSet<String> allAtoms = { "floatbox", "symbolbox", "listbox", "gatom" };

    static inline UnorderedSet<String> allGuis = { "bng",
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
                                                   "button"};
};
