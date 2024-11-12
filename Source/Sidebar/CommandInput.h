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

class LuaExpressionParser {
    public:
    using LuaResult = std::variant<double, String>;
    
    // Constructor: initialize Lua
    LuaExpressionParser(pd::Instance* pd) :pd(pd) {
        L = luaL_newstate();
        luaL_openlibs(L);  // Load Lua standard libraries
        
        luaopen_math(L);
        
        // Create the global "pd" table and set up "pd.post"
        lua_newtable(L);  // Create a new table for "pd"
        
        // Push "this" as light userdata so it can be used as an upvalue in luaPost
        lua_pushlightuserdata(L, pd);
        
        // Register luaPost function with "this" as an upvalue
        lua_pushcclosure(L, LuaExpressionParser::luaPost, 1);  // 1 indicates the number of upvalues
        lua_setfield(L, -2, "post");  // Sets pd.post in the table
        
        lua_setglobal(L, "pd");  // Set the table as the global "pd"
    }
    
    // Destructor: close Lua
    ~LuaExpressionParser() {
        if (L) {
            lua_close(L);
        }
    }
    
    // Define a function to register custom C++ functions for Lua (if needed)
    void registerFunction(const String& name, lua_CFunction func) {
        lua_register(L, name.toRawUTF8(), func);
    }
    
    // Function to execute an expression and return result as LuaResult (either double or string)
    LuaResult executeExpression(const String& expression) {
        bool isExpression = !expression.containsChar('=');  // Check if it’s an expression or a statement
        String luaCode;
        
        if (isExpression) {
            luaCode = "return " + expression;  // Wrap expression with "return"
        } else {
            luaCode = expression;  // Leave statement as-is
        }
        
        // Run the Lua code and check for errors
        if (luaL_dostring(L, luaCode.toRawUTF8()) == LUA_OK) {
            if (isExpression) {
                // Handle expression result
                if (lua_isnumber(L, -1)) {
                    double result = lua_tonumber(L, -1);
                    lua_pop(L, 1);  // Remove result from stack
                    return result;
                } else if (lua_isstring(L, -1)) {
                    String result = lua_tostring(L, -1);
                    lua_pop(L, 1);  // Remove result from stack
                    return result;
                } else {
                    pd->logError("Error: Expression did not return a number or string.");
                    lua_pop(L, 1);  // Remove unexpected result
                    return "";  // Default to empty string if not a number or string
                }
            } else {
                // If it's a statement, we could return success or fetch a specific variable if needed
                return "";  // Or, alternatively, return an empty result if you don’t need feedback
            }
        } else {
            const char* error = lua_tostring(L, -1);
            pd->logError("Lua error: " + String::fromUTF8(error));
            lua_pop(L, 1);  // Remove error message from stack
            return "";  // Return empty string on error
        }
    }
    
    // Optional: Add utility to set variables in Lua
    void setGlobalNumber(const String& name, double value) {
        lua_pushnumber(L, value);
        lua_setglobal(L, name.toRawUTF8());
    }
    
    // Optional: Add utility to retrieve variables from Lua
    LuaResult getGlobal(const String& name) {
        lua_getglobal(L, name.toRawUTF8());
        if (lua_isnumber(L, -1)) {
            double value = lua_tonumber(L, -1);
            lua_pop(L, 1);  // Remove number from stack
            return value;
        } else if (lua_isstring(L, -1)) {
            String value = lua_tostring(L, -1);
            lua_pop(L, 1);  // Remove string from stack
            return value;
        } else {
            pd->logError("Error: Global variable '" + String(name) + "' is not a number or string");
            lua_pop(L, 1);  // Remove unexpected type
            return "";  // Default to empty string if not a number or string
        }
    }
    
    static int luaPost(lua_State* L) {
        // Retrieve the LuaWrapper instance (via userdata, upvalues, etc.)
        pd::Instance* pd = reinterpret_cast<pd::Instance*>(lua_touserdata(L, lua_upvalueindex(1)));
        
        if (lua_isstring(L, 1)) {
            String message = lua_tostring(L, 1);
            pd->logMessage(message);  // Use Pd instance to post message
        } else {
            pd->logError("pd.post requires a string argument");
        }
        return 0;
    }
    
    private:
    lua_State* L;  // Lua state
    pd::Instance* pd;
};

class CommandInput : public Component, public KeyListener
{
public:
    explicit CommandInput(PluginEditor* editor) : editor(editor), lua(editor->pd)
    {
        addAndMakeVisible(commandInput);
        
        commandInput.setBorder({3, 3, 0, 0});
        commandInput.addKeyListener(this);
        commandInput.addMouseListener(this, false);
        commandInput.setFont(Fonts::getDefaultFont().withHeight(15));
        commandInput.onReturnKey = [this, pd = editor->pd](){
            sendConsoleMessage(pd, commandInput.getText());
            if(!commandInput.isEmpty()) {
                commandHistory.push_front(commandInput.getText());
                currentHistoryIndex = -1;
            }
            commandInput.clear();
        };
        
        
        commandInput.onFocusLost = [this](){
            repaint();
        };
        
        lookAndFeelChanged();
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        repaint();
    }
    
    void lookAndFeelChanged() override
    {
        commandInput.setColour(TextEditor::backgroundColourId,findColour(PlugDataColour::sidebarBackgroundColourId));
        commandInput.setColour(TextEditor::outlineColourId,findColour(PlugDataColour::sidebarBackgroundColourId));
        commandInput.setColour(TextEditor::focusedOutlineColourId,findColour(PlugDataColour::sidebarBackgroundColourId));
    }
    
    UnorderedMap<String, Object*> getUniqueObjectNames(Canvas* cnv)
    {
        UnorderedMap<String, Object*> result;
        UnorderedMap<String, int> nameCount;
        for(auto* object : cnv->objects)
        {
            if(!object->gui) continue;
            
            auto tokens = StringArray::fromTokens(object->gui->getText(), false);
            tokens.removeRange(2, tokens.size() - 2);
            
            auto uniqueName = tokens.joinIntoString("_");
            auto& found = nameCount[uniqueName];
            found++;
            
            result[uniqueName + "_" + String(found)] = object;
        }
        
        return result;
    }
    
    String parseExpressions(const String& message) {
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
                parsedMessage += message.substring(openBrace);  // Append remaining message as-is
                break;
            }
            
            String luaExpression = message.substring(openBrace + 1, closeBrace);
            auto result = lua.executeExpression(luaExpression);
            
            if (auto doubleResult = std::get_if<double>(&result)) {
                parsedMessage += String(*doubleResult);
            } else if (auto stringResult = std::get_if<String>(&result)) {
                parsedMessage += *stringResult;
            }
            
            startPos = closeBrace + 1;
        }
        
        return parsedMessage;
    }
    
    void sendConsoleMessage(pd::Instance* pd, String message)
    {
        message = parseExpressions(message);
        
        auto* editor = findParentComponentOfClass<PluginEditor>();
        auto tokens = StringArray::fromTokens(message, true);
        
        if(!tokens[0].startsWith(";") && (consoleTargetName == ">" || tokens[0] == ">" || tokens[0] == "deselect")) // Global or canvas message
        {
            if(tokens[0] == ">") {
                tokens.remove(0);
                if(tokens.size() == 0)
                {
                    tokens.add("deselect");
                }
            }
            
            auto selector = hash(tokens[0]);
            switch(selector)
            {
                case hash("select"):
                {
                    if(auto* cnv = editor->getCurrentCanvas())
                    {
                        if(tokens[1].containsOnly("0123456789"))
                        {
                            int index = tokens[1].getIntValue();
                            if(index >= 0 && index < cnv->objects.size())
                            {
                                cnv->setSelected(cnv->objects[index], true);
                                cnv->updateSidebarSelection();
                            }
                            else {
                                pd->logError("Object index out of bounds");
                            }
                        }
                        else {
                            auto names = getUniqueObjectNames(cnv);
                            if(names.contains(tokens[1])) {
                                auto* object = names[tokens[1]];
                                cnv->setSelected(object, true);
                                cnv->updateSidebarSelection();
                            }
                        }
                    }
                    break;
                }
                case hash("deselect"):
                {
                    if(auto* cnv = editor->getCurrentCanvas())
                    {
                        cnv->deselectAll();
                        cnv->updateSidebarSelection();
                    }
                    break;
                }
                case hash("list"):
                {
                    if(auto* cnv = editor->getCurrentCanvas())
                    {
                        auto names = getUniqueObjectNames(cnv);
                        for(auto& [name, object] : names)
                        {
                            pd->logMessage(name + ": " + object->gui->getText());
                        }
                    }
                    break;
                }
                case hash("search"): {
                    if(auto* cnv = editor->getCurrentCanvas())
                    {
                        auto names = getUniqueObjectNames(cnv);
                        for(auto& [name, object] : names)
                        {
                            auto text = object->gui->getText();
                            if(text.contains(tokens[1]) || name.contains(tokens[1])) {
                                pd->logMessage(name + ": " + object->gui->getText());
                            }
                        }
                    }
                    
                    break;
                }
                default: {
                    SmallArray<pd::Atom> atoms;
                    for(int i = 2; i < tokens.size(); i++)
                    {
                        if(tokens[i].containsOnly("0123456789-e."))
                        {
                            atoms.add(pd::Atom(tokens[i].getFloatValue()));
                        }
                        else {
                            atoms.add(pd::Atom(pd->generateSymbol(tokens[i])));
                        }
                    }
                    pd->sendMessage(tokens[0].toRawUTF8(), tokens[1].toRawUTF8(), std::move(atoms));
                    break;
                }
            }
        }
        else { // object message
            if(auto* cnv = editor->getCurrentCanvas())
            {
                for(auto* obj : cnv->getSelectionOfType<Object>())
                {
                    if(tokens.size() == 1 && tokens[0].containsOnly("0123456789-e."))
                    {
                        pd->sendDirectMessage(obj->getPointer(), tokens[0].getFloatValue());
                    }
                    else if (tokens.size() == 1) {
                        pd->sendDirectMessage(obj->getPointer(), tokens[0], {});
                    }
                    else {
                        SmallArray<pd::Atom> atoms;
                        for(int i = 1; i < tokens.size(); i++)
                        {
                            if(tokens[i].containsOnly("0123456789-e."))
                            {
                                atoms.add(pd::Atom(tokens[i].getFloatValue()));
                            }
                            else {
                                atoms.add(pd::Atom(pd->generateSymbol(tokens[i])));
                            }
                        }
                        pd->sendDirectMessage(obj->getPointer(), tokens[0], std::move(atoms));
                    }
                    
                }
            }
        }
    }
    
    ~CommandInput() override = default;
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 0, getWidth(), 0);
        
        g.setColour(commandInput.hasKeyboardFocus(false) ? findColour(PlugDataColour::dataColourId) : findColour(PlugDataColour::sidebarTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15));
        g.drawText(consoleTargetName, 7, 0, consoleTargetLength, getHeight() - 4, Justification::centredLeft);
    }
    
    void resized() override
    {
        commandInput.setBounds(getLocalBounds().withTrimmedLeft(consoleTargetLength));
    }
    
    void setConsoleTargetName(String const& target)
    {
        consoleTargetName = target + " >";
        if(target == "empty") consoleTargetName = ">";
        consoleTargetLength = CachedStringWidth<15>::calculateStringWidth(consoleTargetName) + 4;
        commandInput.setBounds(commandInput.getBounds().withLeft(consoleTargetLength));
        repaint();
    }
    
    void setHistoryCommand()
    {
        if(currentHistoryIndex < 0)
        {
            commandInput.setText("");
            currentHistoryIndex = -1;
        }
        else if(currentHistoryIndex < commandHistory.size()){
            commandInput.setText(commandHistory[currentHistoryIndex]);
        }
        else {
            currentHistoryIndex = commandHistory.size() - 1;
        }
    }
    
    bool keyPressed (const KeyPress& key, Component*) override
    {
        if(key.getKeyCode() == KeyPress::upKey)
        {
            currentHistoryIndex++;
            setHistoryCommand();
            return true;
        }
        else if(key.getKeyCode() == KeyPress::downKey)
        {
            currentHistoryIndex--;
            setHistoryCommand();
            return true;
        }
        return false;
    }
    
    private:
    PluginEditor* editor;
    LuaExpressionParser lua;
    int consoleTargetLength = 10;
    String consoleTargetName = ">";
    TextEditor commandInput;
    
    int currentHistoryIndex = -1;
    std::deque<String> commandHistory;
};
