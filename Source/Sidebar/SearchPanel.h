/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


#include <m_pd.h>
#include <m_imp.h>
#include <x_libpd_extra_utils.h>

class SearchPanel : public Component
, public ListBoxModel
, public ScrollBar::Listener
, public KeyListener {
public:
    SearchPanel(PlugDataPluginEditor* pluginEditor) : editor(pluginEditor)
    {
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();
        
        listBox.getViewport()->setScrollBarsShown(true, false, false, false);
        
        input.setName("sidebar::searcheditor");
        
        input.onTextChange = [this]() {
            updateResults();
        };
        
        input.addKeyListener(this);
        listBox.addKeyListener(this);
        
        closeButton.setName("statusbar:clearsearch");
        closeButton.onClick = [this]() {
            clearSearchTargets();
            input.clear();
            input.repaint();
        };
        
        closeButton.setAlwaysOnTop(true);
        
        addAndMakeVisible(closeButton);
        addAndMakeVisible(listBox);
        addAndMakeVisible(input);
        
        listBox.addMouseListener(this, true);
        
        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 3, 1 });
        
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        
        listBox.getViewport()->getVerticalScrollBar().addListener(this);
        
        setWantsKeyboardFocus(true);

    }
    
    void mouseDown(const MouseEvent& e) override{
        MessageManager::callAsync([this](){
            updateSelection();
        });
    }
    
    // Divert up/down key events from text editor to the listbox
    bool keyPressed (const KeyPress &key, Component *originatingComponent) override
    {
        MessageManager::callAsync([this](){
            updateSelection();
        });
        
        if(originatingComponent == &listBox) {
            return false;
        }
        if(key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey)) {
            listBox.keyPressed(key);
            
            return true;
        }
    }
    
    void updateSelection() {
        int row = listBox.getSelectedRow();
        if(isPositiveAndBelow(row, searchResult.size())) {
            auto [name, prefix, object, ptr] = searchResult[row];
            
            if(auto* cnv = editor->getCurrentCanvas()) {
                highlightSearchTarget(object);
            }
        }
        
    }
    
    void highlightSearchTarget(Object* target)
    {
        auto* cnv = editor->getCurrentCanvas();
        if(!cnv) return;
        
        for(auto* object : cnv->objects) {
            bool wasSearchTarget = object->isSearchTarget;
            object->isSearchTarget = object == target;
            
            if(wasSearchTarget != object->isSearchTarget) {
                object->repaint();
            }
        }
        
        if(auto* viewport = cnv->viewport) {
            auto pos = target->getPosition();
            
            pos.x -= viewport->getWidth() / 2.0f;
            pos.y -= viewport->getHeight() / 2.0f;
            
            viewport->setViewPosition(pos);
        }
    }

    void clearSearchTargets()
    {
        searchResult.clear();
        listBox.updateContent();
        
        for(auto* cnv : editor->canvases) {
            for(auto* object : cnv->objects) {
                bool wasSearchTarget = object->isSearchTarget;
                object->isSearchTarget = false;
                
                if(wasSearchTarget != object->isSearchTarget && cnv->isShowing()) {
                    object->repaint();
                }
            }
        }
    }
    
    void visibilityChanged() override
    {
        if(!isVisible()) {
            clearSearchTargets();
        }
    }
    
    
    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, 29, getWidth(), 29);
        
        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 30));
        g.setColour(findColour(PlugDataColour::sidebarTextColourId));
        
        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);
    }
    
    String reducePrefixLength(String name, String prefix)
    {
        int maxWidth = getWidth() - 15;
        if(Font().getStringWidth(name + prefix) > maxWidth) {
            prefix = prefix.upToFirstOccurrenceOf("->", true, true) + " ... " + prefix.fromLastOccurrenceOf("->", true, true);
        }
        
        /*
        if(Font().getStringWidth(name + prefix) > maxWidth) {
            prefix = prefix.upToFirstOccurenceOf("->", true, true);
        } */
        
        return prefix;
    }
    
    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if(!isShowing()) return;
        
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(4, 2, w - 8, h - 4, Constants::smallCornerRadius);
        }
        
        g.setColour(rowIsSelected ? findColour(PlugDataColour::sidebarActiveTextColourId) : findColour(ComboBox::textColourId));
        
        const auto& [name, prefix, object, ptr] = searchResult[rowNumber];
        
        auto shortenedPrefix = reducePrefixLength(name, prefix);
        
        Point<int> position;
        auto [x, y] = object->getPosition();
        auto text = shortenedPrefix + name + " (" + String(x) + ", " + String(y) + ")";
        
        g.setFont(Font());
        g.drawText(text, 12, 0, w - 8, h, Justification::centredLeft, true);
    }
    
    int getNumRows() override
    {
        return searchResult.size();
    }
    
    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        
        return nullptr;
    }
    
    
    void updateResults()
    {
        String query = input.getText();
        
        searchResult.clear();
        auto* cnv = editor->getCurrentCanvas();
        
        if (query.isEmpty() || !cnv) {
            clearSearchTargets();
            return;
        }
        
        searchResult = searchRecursively(cnv, cnv->patch, query);
        
        listBox.updateContent();
        
        if (listBox.getSelectedRow() == -1) {
            listBox.selectRow(0, true, true);
            updateSelection();
        }
    }
    
    void grabFocus() {
        input.grabKeyboardFocus();
    }
    
    
    static Array<std::tuple<String, String, Object*, void*>> searchRecursively(Canvas* topLevelCanvas, pd::Patch& patch, const String& query, Object* topLevelObject = nullptr, String prefix = "") {
        
        auto* instance = patch.instance;
        
        Array<std::tuple<String, String, Object*, void*>> result;
        
        for(auto* object : patch.getObjects()) {
            
            auto addObject = [&query, &result, &prefix](const String& text, Object* object, void* ptr) {
                
                // Insert in front if the query matches a whole word
                if (text.containsWholeWordIgnoreCase(query)) {
                    result.insert(0, {text, prefix, object, ptr});
                    return true;
                }
                // Insert in back if it contains the query
                else if (text.containsIgnoreCase(query)) {
                    result.add({text, prefix, object, ptr});
                    return true;
                }
                
                return false;
            };
            
            Object* topLevel = topLevelObject;
            if(topLevelCanvas) {
                for(auto* obj : topLevelCanvas->objects) {
                    if(obj->getPointer() == object) {
                        topLevel = obj;
                    }
                }
            }
            
            auto className = String::fromUTF8(libpd_get_object_class_name(object));
            
            if(className == "canvas" || className == "graph") {
                auto patch = pd::Patch(object, instance);
                
                char* objectText;
                int len;
                libpd_get_object_text(object, &objectText, &len);
                
                addObject(String::fromUTF8(objectText, len), topLevel, object);
                
                // TODO: find a better way to handle this
                auto objTextStr = String::fromUTF8(objectText, len);
                if(len > 25)  {
                    objTextStr = objTextStr.upToFirstOccurrenceOf(" ", true, false);
                }
                
                result.addArray(searchRecursively(nullptr, patch, query, topLevel, prefix +  objTextStr + "-> "));
            }
            else {
                
                bool isGui = !libpd_is_text_object(object);
                
                // If it's a gui add the class name
                if(isGui) {
                    addObject(className, topLevel, object);
                }
                // If it's a text object, message or comment, add the text
                else {
                    char* objectText;
                    int len;
                    libpd_get_object_text(object, &objectText, &len);
                    addObject(String::fromUTF8(objectText, len), topLevel, object);
                }
            }
        }
        
        return result;
    }


    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(28);
        
        tableBounds.removeFromTop(4);
        
        input.setBounds(inputBounds);
        closeButton.setBounds(inputBounds.removeFromRight(30));
        listBox.setBounds(tableBounds);
    }
    
private:
    ListBox listBox;
    
    Array<std::tuple<String, String, Object*, void*>> searchResult;
    TextEditor input;
    TextButton closeButton = TextButton(Icons::Clear);
    
    PlugDataPluginEditor* editor;
};
