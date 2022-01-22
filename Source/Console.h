#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
/* Copyright (c) 2021 Jojo and others. */

/* < https://opensource.org/licenses/BSD-3-Clause > */


class Console : public Component, private AsyncUpdater, public ComponentListener
{
    
    
public:
    explicit Console ()
    {

        viewport.addComponentListener(this);
  
        update(messages, false);
        
        setOpaque (true);
        
        viewport.setViewedComponent(this, false);
        viewport.setScrollBarsShown(true, false);
        setVisible(true);
        
        std::vector<String> tooltips = {
            "Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll"
        };
        
        
        std::vector<std::function<void()>> callbacks = {
            [this](){ clear(); },
            [this](){ restore(); },
            [this](){ if (buttons[2].getState()) restore(); else parse(); },
            [this](){ if (buttons[3].getState()) restore(); else parse(); },
            [this](){ if (buttons[4].getState()) { update(); } },
            
        };
        
        int i = 0;
        for(auto& button : buttons) {
            button.setConnectedEdges(12);
            viewport.addAndMakeVisible(button);
            
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
    }
    
    ~Console() override
    {
        for(auto& button : buttons) button.setLookAndFeel(nullptr);
    }
    
    void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) override {
        update();
    }
    
public:
    void update()
    {
        update(messages, true);
        
        setSize(viewport.getWidth(), std::max<int>(totalHeight + 5, viewport.getHeight()));
        
        if (buttons[4].getToggleState() && totalHeight + 5 >= viewport.getHeight()) {
            const int i = static_cast<int> (messages.size()) - 1;
            
            
            viewport.setViewPosition (viewport.getViewPositionX(), getHeight());
        }
        
       
        repaint();
    }
    
    
    
    template <class T>
    void update (T& c, bool updateRows)
    {
        int size = c.size();
        
        if (updateRows) {
            //listBox.updateContent();
            //listBox.deselectAllRows();
            //listBox.repaint();
        }
        
        /*
        const bool show = (listBox.getNumRowsOnScreen() < size) ||
        (size > 0 && listBox.getRowContainingPosition (0, 0) >= size);
         */
        //listBox.getViewport()->setScrollBarsShown (show, show, true, true);
        repaint();
    }
    
    void handleAsyncUpdate() override
    {
        update();
    }
    
    void logMessage (const String& m)
    {
        removeMessagesIfRequired (messages);
        removeMessagesIfRequired (history);
        
        history.push_back({m, 0});
        
        logMessageProceed ({{m, 0}});
    }
    
    void logError (const String& m)
    {
        removeMessagesIfRequired (messages);
        removeMessagesIfRequired (history);
        
        history.push_back({m, 1});
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
        
        for(auto& item : m) is_selected.push_back(false);
        
        triggerAsyncUpdate();
    }
    
    int getNumRows()
    {
        return jmax (32, static_cast<int> (messages.size()));
    }
    
    void paint (Graphics& g) override
    {
        
        auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
        g.setFont(font);
        g.fillAll (MainLook::firstBackground);
        
        totalHeight = 0;
        
        int numEmpty = 0;
        
        for(int row = 0; row < getNumRows(); row++) {
            int height = 24;
            int num_lines = 1;
            
            if (isPositiveAndBelow (row, messages.size())) {
                auto& e = messages[row];
                
                Array<int> glyphs;
                Array<float> xOffsets;
                font.getGlyphPositions(e.first, glyphs, xOffsets);
                
                for(int i = 0; i < xOffsets.size(); i++) {
                    
                    if((xOffsets[i] + 10) >= getWidth()) {
                        height += 22;
                        
                        for(int j = i + 1; j < xOffsets.size(); j++) {
                            xOffsets.getReference(j) -= xOffsets[i];
                        }
                        num_lines++;
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
                
                g.setColour (is_selected[row] ? MainLook::highlightColour
                             : colourWithType (e.second));
                g.drawFittedText (e.first, r.reduced (4, 0), Justification::centredLeft, num_lines, 1.0f);
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
        
        fb.performLayout (viewport.getLocalBounds().removeFromBottom(33).toFloat());
        
        update(messages, false);
        
        
    }
    
    Viewport viewport;

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
        const int maximum_ = 2048;
        const int removed_ = 64;
        
        int size = static_cast<int> (messages.size());
        
        if (size >= maximum_) {
            const int n = nextPowerOfTwo (size - maximum_ + removed_);
            
            jassert (n < size);
            
            messages.erase (messages.cbegin(), messages.cbegin() + n);
        }
    }
    
    template <class T>
    static void parseMessages (T& m, bool showMessages, bool showErrors)
    {
        if (showMessages == false || showErrors == false) {
            auto f = [showMessages, showErrors] (const std::pair<String, bool>& e)
            {
                bool t = e.second;
                
                if (t && showMessages == false)    { return true; }
                else if (!t && showErrors == false) { return true; }
                else {
                    return false;
                }
            };
            
            m.erase (std::remove_if (m.begin(), m.end(), f), m.end());
        }
    }
    

    

    std::deque<std::pair<String, int>> messages;
    std::deque<std::pair<String, int>> history;
    
    std::array<TextButton, 5> buttons = {TextButton(Icons::Clear), TextButton(Icons::Restore),  TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll)};
    
    
    std::vector<bool> is_selected;

    int totalHeight = 0;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console)
};

