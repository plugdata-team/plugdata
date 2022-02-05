/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"



class ConsoleComponent : public Component, private AsyncUpdater, public ComponentListener
{
    
    std::array<TextButton, 5>& buttons;
    Viewport& viewport;
    
public:
    ConsoleComponent(std::array<TextButton, 5>& b, Viewport& v) : buttons(b), viewport(v)
    {
        
        update();
        
        //setOpaque (true);
        
    }

    void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) override {
        setSize(viewport.getWidth(), getHeight());
        repaint();
    }
    
public:
    
    void update()
    {
        repaint();
        setSize(viewport.getWidth(), std::max<int>(totalHeight + 25, viewport.getHeight()));
        
        if (buttons[4].getToggleState()) {
            viewport.setViewPositionProportionately(0.0f, 1.0f);
        }
    }
    
    void handleAsyncUpdate() override
    {
        update();
    }
    
    void logMessage (const String& m)
    {
        removeMessagesIfRequired (messages);
        removeMessagesIfRequired (history);
        
        history.emplace_back(m, 0);
        
        logMessageProceed ({{m, 0}});
    }
    
    void logError (const String& m)
    {
        removeMessagesIfRequired (messages);
        removeMessagesIfRequired (history);
        
        history.emplace_back(m, 1);
        logMessageProceed ({{m, 1}});
    }
    
    void clear()
    {
        messages.clear();
        triggerAsyncUpdate();
    }
    
    void parse()
    {
        
        parseMessages (messages, buttons[2].getToggleState(), buttons[3].getToggleState());
        
        triggerAsyncUpdate();
    }
    
    void restore()
    {
        std::vector<std::pair<String, int>> m (history.cbegin(), history.cend());
        
        messages.clear();
        logMessageProceed (m);
    }
    
    
    void logMessageProceed (std::vector<std::pair<String, int>> m)
    {
        parseMessages (m, buttons[2].getToggleState(), buttons[3].getToggleState());
        
        messages.insert (messages.cend(), m.cbegin(), m.cend());
        
        for(auto& item : m) isSelected.push_back(false);
        
        triggerAsyncUpdate();
    }

    void paint (Graphics& g) override
    {
        
        auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
        g.setFont(font);
        g.fillAll (MainLook::firstBackground);
        
        totalHeight = 0;
        
        int numEmpty = 0;
        
        for(int row = 0; row < jmax(32, static_cast<int>(messages.size())); row++) {
            int height = 24;
            int numLines = 1;
            
            if (isPositiveAndBelow (row, messages.size())) {
                auto& e = messages[row];
                
                Array<int> glyphs;
                Array<float> xOffsets;
                font.getGlyphPositions(e.first, glyphs, xOffsets);
                
                for(int i = 0; i < xOffsets.size(); i++) {
                    
                    if((xOffsets[i] + 10) >= static_cast<float>(getWidth())) {
                        height += 22;
                        
                        for(int j = i + 1; j < xOffsets.size(); j++) {
                            xOffsets.getReference(j) -= xOffsets[i];
                        }
                        numLines++;
                    }
                }
            }
            
            const Rectangle<int> r (0, totalHeight, getWidth(), height);
            
            if (row % 2) {
                g.setColour (MainLook::secondBackground);
                g.fillRect (r);
            }
            
            if (isPositiveAndBelow (row, messages.size())) {
                const auto& e = messages[row];
                
                g.setColour (isSelected[row] ? MainLook::highlightColour
                                             : colourWithType (e.second));
                g.drawFittedText (e.first, r.reduced (4, 0), Justification::centredLeft, numLines, 1.0f);
            }
            else {
                numEmpty++;
            }
            
            totalHeight += height;
        }
        
        totalHeight -= numEmpty * 24;
        
    }
    
    void resized() override
    {

        repaint();
        
        
    }
    
private:
    static Colour colourWithType (int type)
    {
        auto c = Colours::red;
        
        if (type == 0)       { c = Colours::white; }
        else if (type == 1)  { c = Colours::orange;  }
        else if (type == 2) { c = Colours::red; }
        
        return c;
    }
    
    static void removeMessagesIfRequired (std::deque<std::pair<String, int>>& messages)
    {
        const int maximum = 2048;
        const int removed = 64;
        
        int size = static_cast<int> (messages.size());
        
        if (size >= maximum) {
            const int n = nextPowerOfTwo (size - maximum + removed);
            
            jassert (n < size);
            
            messages.erase (messages.cbegin(), messages.cbegin() + n);
        }
    }
    
    template <class T>
    static void parseMessages (T& m, bool showMessages, bool showErrors)
    {
        if (!showMessages || !showErrors) {
            auto f = [showMessages, showErrors] (const std::pair<String, bool>& e)
            {
                bool t = e.second;
                
                if ((t && !showMessages) || (!t && !showErrors)) {
                    return true;
                }
                else {
                    return false;
                }
            };
            
            m.erase (std::remove_if (m.begin(), m.end(), f), m.end());
        }
    }
    
    std::deque<std::pair<String, int>> messages;
    std::deque<std::pair<String, int>> history;
    
    std::vector<bool> isSelected;
    
    int totalHeight = 0;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConsoleComponent)
};


struct Console : public Component
{
    
    Console() {

        // Viewport takes ownership
        console = new ConsoleComponent(buttons, viewport);
        
        addComponentListener(console);
        
        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);
        console->setVisible(true);
        
        
        addAndMakeVisible(viewport);
        
        std::vector<String> tooltips = {
            "Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll"
        };
        
        
        std::vector<std::function<void()>> callbacks = {
            [this](){ console->clear(); },
            [this](){ console->restore(); },
            [this](){ if (buttons[2].getState()) console->restore(); else console->parse(); },
            [this](){ if (buttons[3].getState()) console->restore(); else console->parse(); },
            [this](){ if (buttons[4].getState()) { console->update(); } },
            
        };
        
        int i = 0;
        for(auto& button : buttons) {
            button.setConnectedEdges(12);
            addAndMakeVisible(button);
            
            button.onClick = callbacks[i];
            button.setTooltip(tooltips[i]);
            
            i++;
        }
        
        buttons[2].setClickingTogglesState(true);
        buttons[3].setClickingTogglesState(true);
        buttons[4].setClickingTogglesState(true);
        
        buttons[2].setToggleState(true, sendNotification);
        buttons[3].setToggleState(true, sendNotification);
        buttons[4].setToggleState(true, sendNotification);
        

        resized();
    }
    
    ~Console() override {
        removeComponentListener(console);
    }
    
    void logMessage (const String& m)
    {
        console->logMessage(m);
    }
    
    void logError (const String& m)
    {
        console->logError(m);
    }
    
    void resized() override {
        
        FlexBox fb;
        fb.flexWrap = FlexBox::Wrap::noWrap;
        fb.justifyContent = FlexBox::JustifyContent::flexStart;
        fb.alignContent = FlexBox::AlignContent::flexStart;
        fb.flexDirection = FlexBox::Direction::row;
        
        for (auto& b : buttons) {
            auto item = FlexItem(b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add (item);
        }
        
        auto bounds = getLocalBounds().toFloat();
        
        fb.performLayout (bounds.removeFromBottom(27));
        viewport.setBounds(bounds.toNearestInt());
    }
    
    ConsoleComponent* console;
    Viewport viewport;
    
    std::array<TextButton, 5> buttons = {TextButton(Icons::Clear), TextButton(Icons::Restore),  TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll)};
};
