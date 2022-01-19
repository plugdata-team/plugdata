#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
/* Copyright (c) 2021 Jojo and others. */

/* < https://opensource.org/licenses/BSD-3-Clause > */


class Console : public Logger, public Component, public ListBoxModel, private AsyncUpdater
{

public:
    explicit Console ()
    {
        listBox.setModel (this);
        
        const int h = 22;
        listBox.setMultipleSelectionEnabled (false);
        listBox.setClickingTogglesRowSelection (true);
        listBox.setRowHeight (h);
        listBox.getViewport()->setScrollBarsShown (false, false, true, true);
        
        update(messages, false);
        addAndMakeVisible (listBox);
 
        setOpaque (true);
        setSize (600, 300);
    }
    
    ~Console() override
    {
    }

public:
    void update()
    {
       update(messages, true);
        
        /*
        if (getButtonState (Icons::autoscroll)) {
        //
        const int i = static_cast<int> (messages.size()) - 1;
        
        listBox.scrollToEnsureRowIsOnscreen (jmax (i, 0));
        //
        } */
    }
    

    template <class T>
    void update (T& c, bool updateRows)
    {
        int size = c.size();
        
        if (updateRows) {
            listBox.updateContent();
            listBox.deselectAllRows();
            listBox.repaint();
        }
        
        const bool show = (listBox.getNumRowsOnScreen() < size) ||
                          (size > 0 && listBox.getRowContainingPosition (0, 0) >= size);
        
        listBox.getViewport()->setScrollBarsShown (show, show, true, true);
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
        parseMessages (messages, true, true);
        
        triggerAsyncUpdate();
    }

    void restore()
    {
        std::vector<std::pair<String, int>> m (history.cbegin(), history.cend());
        
        messages.clear();
        logMessageProceed (m);
    }


private:
    void logMessageProceed (std::vector<std::pair<String, int>> m)
    {
        // TODO: message and error selectors
        parseMessages (m, true, true);
        
        messages.insert (messages.cend(), m.cbegin(), m.cend());
        
        triggerAsyncUpdate();
    }


public:
    int getNumRows() override
    {
        return jmax (32, static_cast<int> (messages.size()));
    }

    void paintListBoxItem (int row, Graphics& g, int width, int height, bool isSelected) override
    {
        if (row % 2) { g.fillAll (MainLook::secondBackground); }
        
        if (isPositiveAndBelow (row, messages.size())) {
        //
        const Rectangle<int> r (width, height);
        const auto& e = messages[row];
        
        g.setColour (isSelected ? MainLook::highlightColour
                                : colourWithType (e.second));
        
        //g.setFont (Spaghettis()->getLookAndFeel().getConsoleFont());
        g.drawText (e.first, r.reduced (4, 0), Justification::centredLeft, true);
        //
        }
    }
    
    void listBoxItemClicked (int row, const MouseEvent &) override
    {
        if (isPositiveAndBelow (row, messages.size()) == false) { triggerAsyncUpdate(); }
    }
    
public:
    void paint (Graphics& g) override
    {
        g.fillAll (MainLook::firstBackground);
    }
    
    void resized() override
    {
        listBox.setBounds (getLocalBounds());
        
        update(messages, false);
    }

    void listWasScrolled() override
    {
        update(messages, false);
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
            
            if (!t && showMessages == false)    { return true; }
            else if (t && showErrors == false) { return true; }
            else {
                return false;
            }
        };
        
          m.erase (std::remove_if (m.begin(), m.end(), f), m.end());
        }
    }
    
private:
    ListBox listBox;
    std::deque<std::pair<String, int>> messages;
    std::deque<std::pair<String, int>> history;
    
    TextButton logMessages;
    TextButton logErrors;
    TextButton clearButton;
    TextButton restoreButton;
    TextButton autoscrollButton;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Console)
};

