/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"
#include "PluginProcessor.h"

#include "Sidebar.h"

// MARK: Document Browser

class DocumentBrowser : public Component
{
public:

    DocumentBrowser(PlugDataAudioProcessor* processor) :
    filter("*.pd", "*", "pure-data files"),
    updateThread("browserThread"),
    directory(&filter, updateThread),
    fileList(directory),
    pd(processor)
    {
        directory.setDirectory(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData"), true, true);

        updateThread.startThread();
        directory.refresh();
        
        addAndMakeVisible(fileList);
        
        fileList.addMouseListener(this, true);
        // use createSymbolicLink on drag/drop
    }
    

    ~DocumentBrowser() {
        fileList.removeMouseListener(this);
        updateThread.stopThread(1000);
    }
    
    void mouseDoubleClick(const MouseEvent& e) override
    {
        auto file = fileList.getSelectedFile();
        
        if(file.existsAsFile()) {
            pd->loadPatch(file);
        }
        
    }

    void resized() override {
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 28));
        
    }
    
    void paint(Graphics& g) override {
        g.fillAll(findColour(PlugDataColour::toolbarColourId));
    }
    
private:
    WildcardFileFilter filter;
    TimeSliceThread updateThread;
    DirectoryContentsList directory;
    FileTreeComponent fileList;
    
    PlugDataAudioProcessor* pd;

};


// MARK: Inspector

struct Inspector : public PropertyPanel
{
    void paint(Graphics& g) override
    {
        
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(getLocalBounds().withHeight(getTotalContentHeight()));
    }

    void loadParameters(ObjectParameters& params)
    {
        StringArray names = {"General", "Appearance", "Label", "Extra"};

        clear();

        auto createPanel = [](int type, const String& name, Value* value, int idx, std::vector<String>& options) -> PropertyComponent*
        {
            switch (type)
            {
                case tString:
                    return new EditableComponent<String>(name, *value, idx);
                case tFloat:
                    return new EditableComponent<float>(name, *value, idx);
                case tInt:
                    return new EditableComponent<int>(name, *value, idx);
                case tColour:
                    return new ColourComponent(name, *value, idx);
                case tBool:
                    return new BoolComponent(name, *value, idx, options);
                case tCombo:
                    return new ComboComponent(name, *value, idx, options);
                default:
                    return new EditableComponent<String>(name, *value, idx);
            }
        };

        for (int i = 0; i < 4; i++)
        {
            Array<PropertyComponent*> panels;

            int idx = 0;
            for (auto& [name, type, category, value, options] : params)
            {
                if (static_cast<int>(category) == i)
                {
                    panels.add(createPanel(type, name, value, idx, options));
                    idx++;
                }
            }
            if (!panels.isEmpty())
            {
                addSection(names[i], panels);
            }
        }
    }

    struct InspectorProperty : public PropertyComponent
    {
        int idx;

        InspectorProperty(const String& propertyName, int i) : PropertyComponent(propertyName, 23), idx(i)
        {
        }

        void paint(Graphics& g) override
        {
            auto bg = idx & 1 ? findColour(PlugDataColour::toolbarColourId) : findColour(PlugDataColour::canvasColourId);
            
            g.fillAll(bg);
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
        }

        void refresh() override{};
    };

    struct ComboComponent : public InspectorProperty
    {
        ComboComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            for (int n = 0; n < options.size(); n++)
            {
                comboBox.addItem(options[n], n + 1);
            }

            comboBox.setName("inspector:combo");
            comboBox.getSelectedIdAsValue().referTo(value);

            addAndMakeVisible(comboBox);
        }

        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        ComboBox comboBox;
    };

    struct BoolComponent : public InspectorProperty
    {
        BoolComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setConnectedEdges(12);

            toggleButton.getToggleStateValue().referTo(value);
            toggleButton.setButtonText(static_cast<bool>(value.getValue()) ? options[1] : options[0]);

            toggleButton.setName("inspector:toggle");

            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this, value, options]() mutable { toggleButton.setButtonText(toggleButton.getToggleState() ? options[1] : options[0]); };
        }

        void resized() override
        {
            toggleButton.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        TextButton toggleButton;
    };

    struct ColourComponent : public InspectorProperty, public ChangeListener
    {
        ColourComponent(const String& propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), currentColour(value)
        {
            String strValue = currentColour.toString();
            if (strValue.length() > 2)
            {
                button.setButtonText(String("#") + strValue.substring(2).toUpperCase());
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
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(PlugDataColour::toolbarColourId));

                colourSelector->setCurrentColour(Colour::fromString(currentColour.toString()));

                CallOutBox::launchAsynchronously(std::move(colourSelector), button.getScreenBounds(), nullptr);
            };
        }

        void updateColour()
        {
            auto colour = Colour::fromString(currentColour.toString());

            button.setColour(TextButton::buttonColourId, colour);
            button.setColour(TextButton::buttonOnColourId, colour.brighter());

            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;

            // make sure text is readable
            button.setColour(TextButton::textColourOffId, textColour);
            button.setColour(TextButton::textColourOnId, textColour);

            button.setButtonText(String("#") + currentColour.toString().substring(2).toUpperCase());
        }

        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            auto* cs = dynamic_cast<ColourSelector*>(source);

            auto colour = cs->getCurrentColour();
            currentColour = colour.toString();

            updateColour();
        }

        ~ColourComponent() override = default;

        void resized() override
        {
            button.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }

       private:
        TextButton button;
        Value& currentColour;
    };

    template <typename T>
    struct EditableComponent : public InspectorProperty
    {
        Label label;
        Value& property;

        float dragValue = 0.0f;
        bool shift = false;
        int decimalDrag = 0;

        Point<float> lastDragPos;

        EditableComponent(String propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), property(value)
        {
            addAndMakeVisible(label);
            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);

            label.setFont(Font(14));

            label.onEditorShow = [this]()
            {
                auto* editor = label.getCurrentTextEditor();

                if constexpr (std::is_floating_point<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789.-");
                }
                else if constexpr (std::is_integral<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789-");
                }
            };
        }

        void mouseDown(const MouseEvent& e) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;
            if (label.isBeingEdited()) return;

            shift = e.mods.isShiftDown();
            dragValue = label.getText().getFloatValue();

            lastDragPos = e.position;

            const auto textArea = label.getBorderSize().subtractedFrom(label.getBounds());

            GlyphArrangement glyphs;
            glyphs.addFittedText(label.getFont(), label.getText(), textArea.getX(), 0., textArea.getWidth(), getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());

            double decimalX = getWidth();
            for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
            {
                auto const& glyph = glyphs.getGlyph(i);
                if (glyph.getCharacter() == '.')
                {
                    decimalX = glyph.getRight();
                }
            }

            bool isDraggingDecimal = e.x > decimalX;

            if constexpr (std::is_integral<T>::value) isDraggingDecimal = false;

            decimalDrag = isDraggingDecimal ? 6 : 0;

            if (isDraggingDecimal)
            {
                GlyphArrangement decimalsGlyph;
                static const String decimalStr("000000");

                decimalsGlyph.addFittedText(label.getFont(), decimalStr, decimalX, 0, getWidth(), getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());

                for (int i = 0; i < decimalsGlyph.getNumGlyphs(); ++i)
                {
                    auto const& glyph = decimalsGlyph.getGlyph(i);
                    if (e.x <= glyph.getRight())
                    {
                        decimalDrag = i + 1;
                        break;
                    }
                }
            }
        }

        void mouseUp(const MouseEvent& e) override
        {
            setMouseCursor(MouseCursor::NormalCursor);
            updateMouseCursor();
        }

        void mouseDrag(const MouseEvent& e) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;
            if (label.isBeingEdited()) return;

            setMouseCursor(MouseCursor::NoCursor);
            updateMouseCursor();

            const int decimal = decimalDrag + e.mods.isShiftDown();
            const float increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
            const float deltaY = e.y - lastDragPos.y;
            lastDragPos = e.position;

            dragValue += increment * -deltaY;

            // truncate value and set
            double newValue = dragValue;

            if (decimal > 0)
            {
                const int sign = (newValue > 0) ? 1 : -1;
                unsigned int ui_temp = (newValue * std::pow(10, decimal)) * sign;
                newValue = (((double)ui_temp) / std::pow(10, decimal) * sign);
            }
            else
            {
                newValue = static_cast<int64_t>(newValue);
            }

            label.setText(formatNumber(newValue), NotificationType::sendNotification);
        }

        String formatNumber(float value)
        {
            String text;
            text << value;
            if (!text.containsChar('.')) text << '.';
            return text;
        }

        void resized() override
        {
            label.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
    };
};

// MARK: Console
struct Console : public Component
{
    explicit Console(pd::Instance* instance)
    {
        // Viewport takes ownership
        console = new ConsoleComponent(instance, buttons, viewport);

        addComponentListener(console);

        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);
        
        console->setVisible(true);

        addAndMakeVisible(viewport);

        std::vector<String> tooltips = {"Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll"};

        std::vector<std::function<void()>> callbacks = {
            [this]() { console->clear(); }, [this]() { console->restore(); }, [this]() {
                console->update(); }, [this]() {
                console->update(); }, [this]() {
                console->update(); },

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

        for (int n = 2; n < 5; n++)
        {
            buttons[n].setClickingTogglesState(true);
            buttons[n].setToggleState(true, sendNotification);
        }

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

        fb.performLayout(bounds.removeFromBottom(28));
        viewport.setBounds(bounds.toNearestInt());
        console->resized();
    }

    void update()
    {
        console->update();
    }

    void deselect()
    {
        console->selectedItem = -1;
        repaint();
    }

    struct ConsoleComponent : public Component, public ComponentListener
    {
        std::array<TextButton, 5>& buttons;
        Viewport& viewport;

        pd::Instance* pd;  // instance to get console messages from

        ConsoleComponent(pd::Instance* instance, std::array<TextButton, 5>& b, Viewport& v) : buttons(b), viewport(v), pd(instance)
        {
            setWantsKeyboardFocus(true);
            update();
        }

        void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override
        {
            setSize(viewport.getWidth(), getHeight());
            repaint();
        }

        void focusLost(FocusChangeType cause) override
        {
            selectedItem = -1;
        }

        bool keyPressed(const KeyPress& key) override
        {
            if (isPositiveAndBelow(selectedItem, pd->consoleMessages.size()))
            {
                // Copy console item
                if (key == KeyPress('c', ModifierKeys::commandModifier, 0))
                {
                    SystemClipboard::copyTextToClipboard(pd->consoleMessages[selectedItem].first);
                    return true;
                }
            }

            return false;
        }

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
            pd->consoleHistory.insert(pd->consoleHistory.end(), pd->consoleMessages.begin(), pd->consoleMessages.end());

            pd->consoleMessages.clear();

            update();
        }

        void restore()
        {
            pd->consoleMessages.insert(pd->consoleMessages.begin(), pd->consoleHistory.begin(), pd->consoleHistory.end());
            pd->consoleHistory.clear();
            update();
        }

        void mouseDown(const MouseEvent& e) override
        {
            int totalHeight = 0;

            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];

                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;

                const Rectangle<int> r(0, totalHeight, getWidth(), height);

                if (r.contains(e.getPosition() + viewport.getViewPosition()))
                {
                    selectedItem = row;
                    repaint();
                    break;
                }

                totalHeight += height;
            }
        }

        void paint(Graphics& g) override
        {
            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
            g.setFont(font);
            g.fillAll(findColour(PlugDataColour::toolbarColourId));
            
            int totalHeight = 0;
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();

            bool rowColour = false;

            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];

                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;

                const Rectangle<int> r(2, totalHeight, getWidth(), height);

                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors))
                {
                    continue;
                }

                if (rowColour || row == selectedItem)
                {
                    g.setColour(selectedItem == row ? findColour(PlugDataColour::highlightColourId) : findColour(ResizableWindow::backgroundColourId));

                    g.fillRect(r);
                }

                rowColour = !rowColour;

                g.setColour(selectedItem == row ? Colours::white : colourWithType(message.second));
                g.drawFittedText(message.first, r.reduced(4, 0), Justification::centredLeft, numLines, 1.0f);

                totalHeight += height;
            }

            while (totalHeight < viewport.getHeight())
            {
                if (rowColour)
                {
                    const Rectangle<int> r(0, totalHeight, getWidth(), 24);
                    
                    g.setColour(findColour(ResizableWindow::backgroundColourId));
                    g.fillRect(r);
                }
                rowColour = !rowColour;
                totalHeight += 24;
            }
        }

        // Get total height of messages, also taking multi-line messages into account
        int getTotalHeight()
        {
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();

            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
            int totalHeight = 0;

            int numEmpty = 0;

            for (auto& message : pd->consoleMessages)
            {
                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;

                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors)) continue;

                totalHeight += height;
            }

            totalHeight -= numEmpty * 24;

            return totalHeight;
        }

        void resized() override
        {
            update();
        }

        int selectedItem = -1;

       private:
        Colour colourWithType(int type)
        {
            if (type == 0)
                return findColour(ComboBox::textColourId);
            else if (type == 1)
                return Colours::orange;
            else
                return Colours::red;
        }

       private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };
    


   private:
    ConsoleComponent* console;
    Viewport viewport;

    std::array<TextButton, 5> buttons = {TextButton(Icons::Clear), TextButton(Icons::Restore), TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll)};
};

Sidebar::Sidebar(PlugDataAudioProcessor* instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(instance);
    inspector = new Inspector;
    browser = new DocumentBrowser(instance);

    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    addChildComponent(browser);
    
    browser->setAlwaysOnTop(true);

    setBounds(getParentWidth() - lastWidth, 40, lastWidth, getParentHeight() - 40);
}

Sidebar::~Sidebar()
{
    delete console;
    delete inspector;
    delete browser;
}

void Sidebar::paint(Graphics& g)
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Sidebar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, sWidth, getHeight() - 28);
    
}

void Sidebar::paintOverChildren(Graphics& g)
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Draggable bar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, dragbarWidth + 1, getHeight());
    
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(1, 0, 1, getHeight() - 27.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(dragbarWidth);

    console->setBounds(bounds);
    inspector->setBounds(bounds);
    browser->setBounds(bounds);
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
        newWidth = std::min(newWidth, getParentWidth() / 2);

        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    }
}

void Sidebar::mouseUp(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        // getCurrentCanvas()->checkBounds(); fix this
        draggingSidebar = false;
    }
}

void Sidebar::mouseMove(const MouseEvent& e)
{
    if(e.getPosition().getX() < dragbarWidth) {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void Sidebar::mouseExit(const MouseEvent& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showBrowser(bool show)
{
    browser->setVisible(show);
}

void Sidebar::showSidebar(bool show)
{
    sidebarHidden = !show;

    if (!show)
    {
        lastWidth = getWidth();
        int newWidth = dragbarWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
    else
    {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
}

void Sidebar::pinSidebar(bool pin)
{
    pinned = pin;

    if (!pinned && lastParameters.empty())
    {
        hideParameters();
    }
}

bool Sidebar::isPinned()
{
    return pinned;
}

void Sidebar::showParameters(ObjectParameters& params)
{
    lastParameters = params;
    inspector->loadParameters(params);

    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);

    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    
    if (!pinned)
    {
        inspector->setVisible(false);
        console->setVisible(true);
    }

    if (pinned)
    {
        ObjectParameters params = {};
        inspector->loadParameters(params);
    }

    console->deselect();
}

bool Sidebar::isShowingConsole() const noexcept
{
    return console->isVisible();
}

void Sidebar::updateConsole()
{
    console->update();
}
