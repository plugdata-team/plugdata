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
    using LuaResult = std::variant<double, std::string>;

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
       void registerFunction(const std::string& name, lua_CFunction func) {
           lua_register(L, name.c_str(), func);
       }

       // Function to execute an expression and return result as LuaResult (either double or string)
       LuaResult executeExpression(const std::string& expression) {
           bool isExpression = (expression.find('=') == std::string::npos);  // Check if it’s an expression or a statement
               std::string luaCode;

               if (isExpression) {
                   luaCode = "return " + expression;  // Wrap expression with "return"
               } else {
                   luaCode = expression;  // Leave statement as-is
               }

               // Run the Lua code and check for errors
               if (luaL_dostring(L, luaCode.c_str()) == LUA_OK) {
                   if (isExpression) {
                       // Handle expression result
                       if (lua_isnumber(L, -1)) {
                           double result = lua_tonumber(L, -1);
                           lua_pop(L, 1);  // Remove result from stack
                           return result;
                       } else if (lua_isstring(L, -1)) {
                           std::string result = lua_tostring(L, -1);
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
       void setGlobalNumber(const std::string& name, double value) {
           lua_pushnumber(L, value);
           lua_setglobal(L, name.c_str());
       }

       // Optional: Add utility to retrieve variables from Lua
       LuaResult getGlobal(const std::string& name) {
           lua_getglobal(L, name.c_str());
           if (lua_isnumber(L, -1)) {
               double value = lua_tonumber(L, -1);
               lua_pop(L, 1);  // Remove number from stack
               return value;
           } else if (lua_isstring(L, -1)) {
               std::string value = lua_tostring(L, -1);
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
            std::string message = lua_tostring(L, 1);
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
    explicit CommandInput(pd::Instance* pd) : lua(pd)
    {
        addAndMakeVisible(commandInput);
    
        commandInput.setBorder({2, 1, 0, 0});
        commandInput.addKeyListener(this);
        commandInput.addMouseListener(this, false);
        commandInput.onReturnKey = [this, pd](){
            sendConsoleMessage(pd, commandInput.getText());
            commandHistory.add(commandInput.getText());
            currentHistoryIndex = 0;
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
        
    void sendConsoleMessage(pd::Instance* pd, String message)
    {
        if(message.contains("<"))
        {
            auto expression = message.fromFirstOccurrenceOf("<", false, false).trim();
            expression = expression.replaceCharacter(';', '\n');
            auto command = message.upToFirstOccurrenceOf("<", false, false);
            
            auto result = lua.executeExpression(expression.toStdString());
            if (auto doubleResult = std::get_if<double>(&result)) {
                message = command + String(*doubleResult);
            } else if (auto stringResult = std::get_if<std::string>(&result)) {
                message = command + String(*stringResult);
            }
        }
        
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
                                pd->logMessage("Selected object " + String(index));
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
                                pd->logMessage("Selected object " + tokens[1]);
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
                        pd->logMessage("Deselected objects");
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
                    // TODO: handle args
                    pd->sendMessage(tokens[0].toRawUTF8(), tokens[1].toRawUTF8(), {});
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
                            pd->sendDirectMessage(obj->getPointer(), tokens[0], std::move(atoms));
                        }
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
        g.setFont(Fonts::getSemiBoldFont().withHeight(14));
        g.drawText(consoleTargetName, 5, 0, consoleTargetLength, getHeight(), Justification::centredLeft);
    }

    void resized() override
    {
        commandInput.setBounds(getLocalBounds().withTrimmedLeft(consoleTargetLength));
    }

    void setConsoleTargetName(String const& target)
    {
        consoleTargetName = target + " >";
        if(target == "empty") consoleTargetName = ">";
        consoleTargetLength = CachedStringWidth<14>::calculateStringWidth(consoleTargetName) + 4;
        commandInput.setBounds(commandInput.getBounds().withLeft(consoleTargetLength));
        repaint();
    }
    
    void setHistoryCommand(int index)
    {
        if(index < 0)
        {
            commandInput.setText("0");
            index = -1;
        }
        else if(index < commandHistory.size()){
            commandInput.setText(commandHistory[index]);
        }
        else {
            index = commandHistory.size();
        }
    }
        
    bool keyPressed (const KeyPress& key, Component*) override
    {
        if(key.getKeyCode() == KeyPress::upKey)
        {
            setHistoryCommand(currentHistoryIndex++);
            return true;
        }
        else if(key.getKeyCode() == KeyPress::downKey)
        {
            setHistoryCommand(currentHistoryIndex--);
            return true;
        }
        return false;
    }
    
private:
    LuaExpressionParser lua;
    int consoleTargetLength = 10;
    String consoleTargetName = ">";
    TextEditor commandInput;
    
    int currentHistoryIndex = -1;
    StringArray commandHistory;
};
