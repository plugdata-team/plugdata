/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"

#include "Sidebar.h"
// MARK: Inspector


struct Inspector : public Component
{
    int expanded = 0;
    
    Inspector()
    {
        for(auto& table : tables)  {
            panel.addPanel(-1, table.getTable(), false);
            panel.setCustomPanelHeader(table.getTable(), table.getHeader(), false);
        }
        
        loadData({});
        
        addAndMakeVisible(panel);
        
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().withWidth(getWidth() + 2));
    }
    
    void expand(int index) {
        expanded = index;
        panel.expandPanelFully(panel.getPanel(index), true);
    }

    void deselect()
    {
        loadData({});
    }

    void loadData(const ObjectParameters& params)
    {
        for(auto& table : tables) table.loadData(params);
        expand(0);
    }
    
    
    struct ParametersHeader : public Component
    {
        int index;
        Inspector* inspector;
        
        ParametersHeader(Inspector* parent, int idx) : index(idx), inspector(parent) {
        }
        
        void drawArrow(Graphics& g, bool closed, Rectangle<int> r) {
            
            const float side = juce::jmin (r.getWidth(), r.getHeight()) * 0.65f;
            const float x    = static_cast<float> (r.getCentreX());
            const float y    = static_cast<float> (r.getCentreY());
            const float h    = closed ? side * 0.5f : side * 0.25f;
            const float w    = closed ? side * 0.25f : side * 0.5f;
            
            juce::Path path;
            path.startNewSubPath (x - w, y - h);
            
            if(closed) {
                path.lineTo (x + w, y);
                path.lineTo (x - w, y + h);
            }
            else
            {
                path.lineTo (x, y + h);
                path.lineTo (x + w, y - h);
            }

            g.strokePath (path, juce::PathStrokeType (2.0f));
        }
        
        void paint (Graphics& g) override
        {
            Rectangle<int> b (getLocalBounds().withTrimmedBottom (2));
            
            g.setColour(findColour(ResizableWindow::backgroundColourId));
            g.fillRoundedRectangle (b.toFloat(), 3.0f);
            
            Rectangle<int> arrow (b.removeFromLeft (25));
            
            arrow = arrow.reduced(5);
            
            g.setColour(Colours::white);

            drawArrow(g, inspector->expanded == index, arrow);
            
            String name;
            if(index == cGeneral) name = "General";
            else if(index == cAppearance) name = "Appearance";
            else if(index == cLabel) name = "Label";
            else name = "Extra";
    
            g.setColour (Colours::white);
            g.drawText (name, b.reduced (4, 0), Justification::centredLeft, true);
        }

        void mouseUp (const MouseEvent& e) override
        {
            if (!e.mouseWasDraggedSinceMouseDown()) { inspector->expand(index); }
        }
    };
    

    struct InspectorListbox : public Component, public TableListBoxModel {
        
        
        Inspector* inspector;
        
        ParameterCategory id;
        ParametersHeader header;

        InspectorListbox(Inspector* parent, ParameterCategory paramId) : inspector(parent), id(paramId), header(parent, static_cast<int>(paramId)) {
            table.setModel(this);

            table.getHeader().addColumn("Name", 1, 50, 30, -1, TableHeaderComponent::notSortable);
            table.getHeader().addColumn("Value", 2, 80, 30, -1, TableHeaderComponent::notSortable);
            table.setHeaderHeight(0);
            
            setColour(ListBox::textColourId, Colours::white);
            setColour(ListBox::outlineColourId, Colours::white);

            table.getHeader().setStretchToFitActive(true);

            table.getHeader().setColour(TableHeaderComponent::textColourId, Colours::white);
            table.getHeader().setColour(TableHeaderComponent::backgroundColourId, findColour(Slider::thumbColourId));

            table.setMultipleSelectionEnabled(true);
            
        }
        
        TableListBox* getTable() {
            return &table;
        }
        
        Component* getHeader() {
            return &header;
        }
        
        
        void loadData(ObjectParameters parameters) {
            
            items.clear();
            indices.clear();
            
            auto& [params, cb] = parameters;
            
            callback = cb;
            for(int n = 0; n < params.size(); n++) {
                
                auto& [name, type, category, value, options] = params[n];
                if(category != id) continue;
                
                items.push_back(params[n]);
                indices.push_back(n);
            }
            
            
            inspector->panel.setPanelHeaderSize(getTable(), items.empty() ? 0 : 23);
            
            table.updateContent();
        }
        
        // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
        int getNumRows() override
        {
            return static_cast<int>(items.size());
        }

        // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
        void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override
        {
            if (rowIsSelected)
            {
                g.fillAll(findColour(Slider::thumbColourId));
            }
            else
            {
                g.fillAll((row % 2) ? findColour(ComboBox::backgroundColourId) : findColour(ResizableWindow::backgroundColourId));
            }
        }
        

        // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
        // components.
        void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
        {
            g.setColour(Colours::white);
            g.setFont(font);

            if (rowNumber < items.size())
            {
                const auto& [name, type, category, ptr, options] = items[rowNumber];
                g.drawText(name, 6, 0, width - 4, height, Justification::centredLeft, true);
            }

            g.setColour(Colours::black.withAlpha(0.2f));
            g.fillRect(width - 1, 0, 1, height);
        }

        // Sorting is disabled
        void sortOrderChanged(int newSortColumnId, bool isForwards) override
        {
        }

        // This is overloaded from TableListBoxModel, and must update any custom components that we're using
        Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
        {
            delete existingComponentToUpdate;

            // Draw names regularly
            if (columnId == 1) return nullptr;

            auto& [name, type, category, ptr, options] = items[rowNumber];

            switch (type)
            {
                case tString:
                    return new EditableComponent<String>(callback, static_cast<String*>(ptr), indices[rowNumber]);
                case tFloat:
                    return new EditableComponent<float>(callback, static_cast<float*>(ptr), indices[rowNumber]);
                case tInt:
                    return new EditableComponent<int>(callback, static_cast<int*>(ptr), indices[rowNumber]);
                case tColour:
                    return new ColourComponent(callback, static_cast<String*>(ptr), indices[rowNumber]);
                case tBool:
                    return new ToggleComponent(callback, static_cast<bool*>(ptr), indices[rowNumber], options);
                case tCombo:
                    return new ComboComponent(callback, static_cast<int*>(ptr), indices[rowNumber], options);
            }

            // for any other column, just return 0, as we'll be painting these columns directly.
            jassert(existingComponentToUpdate == nullptr);
            return nullptr;
        }

        // This is overloaded from TableListBoxModel, and should choose the best width for the specified
        // column.
        int getColumnAutoSizeWidth(int columnId) override
        {
            if (columnId == 1)
            {
                return getWidth() / 3;  // Name column
            }
            else
            {
                return static_cast<int>(getWidth() * 1.5f);  // Value column
            }
        }
        
        std::vector<int> indices;
        std::vector<ObjectParameter> items;  // Currently loaded items in inspector
        std::function<void(int)> callback;   // Callback when param changes, with int argument
        
        
        TableListBox table;  // the table component itself
        Font font;

    };
    
    struct ComboComponent : public Component
    {
        ComboComponent(std::function<void(int)> cb, int* value, int rowIdx, std::vector<String> items) : callback(std::move(cb)), row(rowIdx)
        {
            
            for(int n = 0; n < items.size(); n++) {
                comboBox.addItem(items[n], n + 1);
            }
            
            comboBox.setName("inspector:combo");
            comboBox.setSelectedId((*value) + 1);
  
            addAndMakeVisible(comboBox);

            comboBox.onChange = [this, value]()
            {
                *value = comboBox.getSelectedId() - 1;
                callback(row);
            };
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds());
        }

       private:
        std::function<void(int)> callback;
        int row;
        ComboBox comboBox;
        
    };
    
    struct ToggleComponent : public Component
    {
        ToggleComponent(std::function<void(int)> cb, bool* value, int rowIdx, std::vector<String> options) : callback(std::move(cb)), row(rowIdx)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setToggleState(*value, sendNotification);
            toggleButton.setButtonText((*value) ? options[1] : options[0]);
            toggleButton.setConnectedEdges(12);
            
            toggleButton.setName("inspector:toggle");

            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this, value, options]() mutable
            {
                *value = toggleButton.getToggleState();
                toggleButton.setButtonText((*value) ? options[1] : options[0]);
                callback(row);
            };
        }

        void resized() override
        {
            toggleButton.setBounds(getLocalBounds());
        }

       private:
        std::function<void(int)> callback;
        int row;
        TextButton toggleButton;
    };

    struct ColourComponent : public Component, public ChangeListener
    {
        ColourComponent(std::function<void(int)> cb, String* value, int rowIdx) : callback(std::move(cb)), currentColour(value), row(rowIdx)
        {
            if (value && value->length() > 2)
            {
                button.setButtonText(String("#") + (*value).substring(2));
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            updateColour();

            button.onClick = [this]()
            {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(ComboBox::backgroundColourId));

                colourSelector->setCurrentColour(Colour::fromString(*currentColour));

                CallOutBox::launchAsynchronously(std::move(colourSelector), button.getScreenBounds(), nullptr);
            };
        }

        void updateColour()
        {
            auto colour = Colour::fromString(*currentColour);

            button.setColour(TextButton::buttonColourId, colour);
            button.setColour(TextButton::buttonOnColourId, colour.brighter());

            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;

            // make sure text is readable
            button.setColour(TextButton::textColourOffId, textColour);
            button.setColour(TextButton::textColourOnId, textColour);

            button.setButtonText(String("#") + (*currentColour).substring(2));
        }

        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            auto* cs = dynamic_cast<ColourSelector*>(source);

            auto colour = cs->getCurrentColour();
            *currentColour = colour.toString();

            updateColour();
            callback(row);
        }

        ~ColourComponent() override = default;

        void resized() override
        {
            button.setBounds(getLocalBounds());
        }

       private:
        std::function<void(int)> callback;
        TextButton button;
        String* currentColour;

        int row;
    };

    template <typename T>
    struct EditableComponent : public Label
    {
        float downValue;
        
        EditableComponent(std::function<void(int)> cb, T* value, int rowIdx) : callback(std::move(cb)), row(rowIdx)
        {
            setEditable(false, true);

            setText(String(*value), dontSendNotification);

            onTextChange = [this, value]()
            {
                if constexpr (std::is_floating_point<T>::value)
                {
                    *value = getText().getFloatValue();
                }
                else if constexpr (std::is_integral<T>::value)
                {
                    *value = getText().getIntValue();
                }
                else
                {
                    *value = getText();
                }

                callback(row);
            };
        }

        TextEditor* createEditorComponent() override
        {
            auto* editor = Label::createEditorComponent();

            if constexpr (std::is_floating_point<T>::value)
            {
                editor->setInputRestrictions(0, "0123456789.-");
            }
            else if constexpr (std::is_integral<T>::value)
            {
                editor->setInputRestrictions(0, "0123456789-");
            }

            return editor;
        }
        
        void mouseDown(const MouseEvent& event) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;
            
            downValue = getText().getFloatValue();
        }
        
        void mouseDrag(const MouseEvent& e) override {
            
            if constexpr (!std::is_arithmetic<T>::value) return;
            
            Label::mouseDrag(e);
        
            auto const inc = static_cast<float>(-e.getDistanceFromDragStartY()) * 0.5f;
            if (std::abs(inc) < 1.0f) return;

            // Logic for dragging, where the x position decides the precision
            auto currentValue = getText();
            if (!currentValue.containsChar('.')) currentValue += '.';
            if (currentValue.getCharPointer()[0] == '-') currentValue = currentValue.substring(1);
            currentValue += "00000";

            // Get position of all numbers
            Array<int> glyphs;
            Array<float> xOffsets;
            getFont().getGlyphPositions(currentValue, glyphs, xOffsets);

            // Find the number closest to the mouse down
            auto position = static_cast<float>(e.getMouseDownX() - 4);
            auto precision = static_cast<int>(std::lower_bound(xOffsets.begin(), xOffsets.end(), position) - xOffsets.begin());
            precision -= currentValue.indexOfChar('.');

            // I case of integer dragging
            if (precision <= 0)
            {
                precision = 0;
            }
            else
            {
                // Offset for the decimal point character
                precision -= 1;
            }
            
            if constexpr (std::is_integral<T>::value)
            {
                precision = 0;
            }

            // Calculate increment multiplier
            float multiplier = powf(10.0f, static_cast<float>(-precision));

            // Calculate new value as string
            auto newValue = String(downValue + inc * multiplier, precision);

            if (precision == 0) newValue = newValue.upToFirstOccurrenceOf(".", true, false);

            if constexpr (std::is_integral<T>::value)
            {
                newValue = newValue.upToFirstOccurrenceOf(".", false, false);
            }
            
            setText(newValue, sendNotification);
            
    
        
        }

       private:
        std::function<void(int)> callback;

        int row;
    };

   private:
    
    ConcertinaPanel panel;
    std::array<InspectorListbox, 4> tables = {InspectorListbox(this, cGeneral), InspectorListbox(this, cAppearance), InspectorListbox(this, cLabel), InspectorListbox(this, cExtra)};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Inspector)
};

// MARK: Console


struct ConsoleComponent : public Component, public ComponentListener
{
    std::array<TextButton, 5>& buttons;
    Viewport& viewport;
    
    std::vector<std::pair<String, int>> messages;
    std::vector<std::pair<String, int>> history;

    ConsoleComponent(std::array<TextButton, 5>& b, Viewport& v) : buttons(b), viewport(v)
    {
        update();
    }

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override
    {
        setSize(viewport.getWidth(), getHeight());
        repaint();
    }

   public:
    
    void update()
    {
        repaint();
        setSize(viewport.getWidth(), std::max<int>(getTotalHeight(), viewport.getHeight()));

        if (buttons[4].getToggleState())
        {
            viewport.setViewPositionProportionately(0.0f, 1.0f);
        }
    }


    void clear()
    {
        messages.clear();
    }


    void mouseDown(const MouseEvent& e) override {
        // TODO: implement selecting and copying comments
    }

    void paint(Graphics& g) override
    {
        auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
        g.setFont(font);
        g.fillAll(findColour(ComboBox::backgroundColourId));

        
        int totalHeight = 0;

        int numEmpty = 0;

        bool showMessages = buttons[2].getToggleState();
        bool showErrors = buttons[3].getToggleState();
        
        for (int row = 0; row < jmax(32, static_cast<int>(messages.size())); row++)
        {
            int height = 24;
            int numLines = 1;

            if (isPositiveAndBelow(row, messages.size()))
            {
                auto& e = messages[row];
                
                if((e.second == 1 && !showMessages) || (e.second == 2 && !showErrors)) continue;

                Array<int> glyphs;
                Array<float> xOffsets;
                font.getGlyphPositions(e.first, glyphs, xOffsets);

                for (int i = 0; i < xOffsets.size(); i++)
                {
                    if ((xOffsets[i] + 10) >= static_cast<float>(getWidth()))
                    {
                        height += 22;

                        for (int j = i + 1; j < xOffsets.size(); j++)
                        {
                            xOffsets.getReference(j) -= xOffsets[i];
                        }
                        numLines++;
                    }
                }
            }

            const Rectangle<int> r(0, totalHeight, getWidth(), height);

            if (row % 2 || row == selectedItem)
            {
                g.setColour(selectedItem == row ? findColour(Slider::thumbColourId) : findColour(ResizableWindow::backgroundColourId));
                
                g.fillRect(r);
            }

            if (isPositiveAndBelow(row, messages.size()))
            {
                const auto& e = messages[row];

                g.setColour(selectedItem == row ? Colours::white : colourWithType(e.second));
                g.drawFittedText(e.first, r.reduced(4, 0), Justification::centredLeft, numLines, 1.0f);
            }
            else
            {
                numEmpty++;
            }

            totalHeight += height;
        }

        totalHeight -= numEmpty * 24;
    }

    // Get total height of messages, also taking multi-line messages into account
    // TODO: pre-calculate the number of lines in messages!!
    int getTotalHeight()
    {
        auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
        int totalHeight = 0;

        int numEmpty = 0;

        for (int row = 0; row < jmax(32, static_cast<int>(messages.size())); row++)
        {
            int height = 24;
            int numLines = 1;

            if (isPositiveAndBelow(row, messages.size()))
            {
                auto& e = messages[row];

                Array<int> glyphs;
                Array<float> xOffsets;
                font.getGlyphPositions(e.first, glyphs, xOffsets);

                for (int i = 0; i < xOffsets.size(); i++)
                {
                    if ((xOffsets[i] + 10) >= static_cast<float>(getWidth()))
                    {
                        height += 22;

                        for (int j = i + 1; j < xOffsets.size(); j++)
                        {
                            xOffsets.getReference(j) -= xOffsets[i];
                        }
                        numLines++;
                    }
                }
            }

            if (!isPositiveAndBelow(row, messages.size()))
            {
                numEmpty++;
            }

            totalHeight += height;
        }

        totalHeight -= numEmpty * 24;

        return totalHeight;
    }

    void resized() override
    {
        update();
    }

   private:
    static Colour colourWithType(int type)
    {
        auto c = Colours::red;

        if (type == 0)
        {
            c = Colours::white;
        }
        else if (type == 1)
        {
            c = Colours::orange;
        }
        else if (type == 2)
        {
            c = Colours::red;
        }

        return c;
    }

    static void removeMessagesIfRequired(std::deque<std::pair<String, int>>& messages)
    {
        const int maximum = 2048;
        const int removed = 64;

        int size = static_cast<int>(messages.size());

        if (size >= maximum)
        {
            const int n = nextPowerOfTwo(size - maximum + removed);

            jassert(n < size);

            messages.erase(messages.cbegin(), messages.cbegin() + n);
        }
    }

    template <class T>
    static void parseMessages(T& m, bool showMessages, bool showErrors)
    {
        if (!showMessages || !showErrors)
        {
            auto f = [showMessages, showErrors](const std::pair<String, bool>& e)
            {
                bool t = e.second;

                if ((t && !showMessages) || (!t && !showErrors))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            };

            m.erase(std::remove_if(m.begin(), m.end(), f), m.end());
        }
    }

    int selectedItem = -1;

   private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
};

struct Console : public Component
{
    Console()
    {
        // Viewport takes ownership
        console = new ConsoleComponent(buttons, viewport);

        addComponentListener(console);

        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);
        console->setVisible(true);

        addAndMakeVisible(viewport);

        std::vector<String> tooltips = {"Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll"};

        std::vector<std::function<void()>> callbacks = {
            [this]() { /*console->clear();*/ },
            [this]() { /* console->restore(); */ },
            [this]()
            {
                repaint();
                /*
                if (buttons[2].getState())
                    console->restore();
                else
                    console->parse(); */
            },
            [this]()
            {
                repaint();
                /*
                if (buttons[3].getState())
                    console->restore();
                else
                    console->parse(); */
            },
            [this]()
            {
                if (buttons[4].getState())
                {
                    console->update();
                }
            },

        };

        int i = 0;
        for (auto& button : buttons)
        {
            button.setName("statusbar:console");
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

    ~Console() override
    {
        removeComponentListener(console);
    }
    


    void resized() override
    {
        FlexBox fb;
        fb.flexWrap = FlexBox::Wrap::noWrap;
        fb.justifyContent = FlexBox::JustifyContent::flexStart;
        fb.alignContent = FlexBox::AlignContent::flexStart;
        fb.flexDirection = FlexBox::Direction::row;

        for (auto& b : buttons)
        {
            auto item = FlexItem(b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }

        auto bounds = getLocalBounds().toFloat();

        fb.performLayout(bounds.removeFromBottom(27));
        viewport.setBounds(bounds.toNearestInt());
    }

    ConsoleComponent* console;
    Viewport viewport;

    std::array<TextButton, 5> buttons = {TextButton(Icons::Clear), TextButton(Icons::Restore), TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll)};
};


Sidebar::Sidebar(pd::Instance* instance) : pd(instance) {
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console;
    inspector = new Inspector;
    
    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    
    setBounds(getParentWidth() - lastWidth, 40, lastWidth, getParentHeight() - 65);
}

Sidebar::~Sidebar() {
    delete console;
    delete inspector;
}

void Sidebar::paint(Graphics& g) {
    
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    auto baseColour = findColour(ComboBox::backgroundColourId);
    
    // Sidebar
    g.setColour(baseColour.darker(0.1));
    g.fillRect(getWidth() - sWidth, dragbarWidth, sWidth + 10, getHeight());

    // Draggable bar
    g.setColour(baseColour);
    g.fillRect(getWidth() - sWidth, dragbarWidth, 25, getHeight());
}

void Sidebar::resized() {
    
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(dragbarWidth);
    
    console->setBounds(bounds);
    inspector->setBounds(bounds);
}

void Sidebar::mouseDown(const MouseEvent& e)
{
    Rectangle<int> dragBar(0, dragbarWidth, getWidth(), getHeight());
    if (dragBar.contains(e.getPosition()) && !sidebarHidden)
    {
        draggingSidebar = true;
        dragStartWidth = getWidth();
    }
    else
    {
        draggingSidebar = false;
    }
}

void Sidebar::mouseDrag(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        int newWidth = dragStartWidth - e.getDistanceFromDragStartX();
        
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    }
}

void Sidebar::mouseUp(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        //getCurrentCanvas()->checkBounds(); fix this
        draggingSidebar = false;
    }
}

void Sidebar::showSidebar(bool show) {
    sidebarHidden = !show;
    
    if(!show)
    {
        lastWidth = getWidth();
        int newWidth = dragbarWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
    else {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
}

void Sidebar::showParameters(ObjectParameters& params) {
    lastParameters = params;
    inspector->loadData(params);
    inspector->setVisible(true);
    console->setVisible(false);
}
void Sidebar::showParameters() {
    inspector->loadData(lastParameters);
    inspector->setVisible(true);
    console->setVisible(false);
}
void Sidebar::hideParameters() {
    inspector->setVisible(false);
    console->setVisible(true);
}

bool Sidebar::isShowingConsole() const noexcept {
    return console->isVisible();
}


void Sidebar::updateConsole()
{
    console->console->messages = pd->consoleMessages;
    console->repaint();
}

