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

struct Inspector : public PropertyPanel
{
    void paint(Graphics& g) override
    {
        g.setColour(findColour(ComboBox::backgroundColourId).darker(0.2f));
        g.fillRect(getLocalBounds().withHeight(getTotalContentHeight()));
    }

    void loadParameters(ObjectParameters& params)
    {
        StringArray names = {"General", "Appearance", "Label", "Extra"};

        clear();

        auto createPanel = [](int type, const String& name, Value* value, Colour bg, std::vector<String>& options) -> PropertyComponent*
        {
            switch (type)
            {
                case tString:
                    return new EditableComponent<String>(name, *value, bg);
                case tFloat:
                    return new EditableComponent<float>(name, *value, bg);
                case tInt:
                    return new EditableComponent<int>(name, *value, bg);
                case tColour:
                    return new ColourComponent(name, *value, bg);
                case tBool:
                    return new BoolComponent(name, *value, bg, options);
                case tCombo:
                    return new ComboComponent(name, *value, bg, options);
                default:
                    return new EditableComponent<String>(name, *value, bg);
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
                    auto bg = idx & 1 ? findColour(ComboBox::backgroundColourId) : findColour(ResizableWindow::backgroundColourId);

                    panels.add(createPanel(type, name, value, bg, options));
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
        Colour bg;

        InspectorProperty(const String& propertyName, Colour background) : PropertyComponent(propertyName, 23), bg(background)
        {
        }

        void paint(Graphics& g) override
        {
            g.fillAll(bg);
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
        }

        void refresh() override{};
    };

    struct ComboComponent : public InspectorProperty
    {
        ComboComponent(const String& propertyName, Value& value, Colour background, std::vector<String> options) : InspectorProperty(propertyName, background)
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
        BoolComponent(const String& propertyName, Value& value, Colour background, std::vector<String> options) : InspectorProperty(propertyName, background)
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
        ColourComponent(const String& propertyName, Value& value, Colour background) : InspectorProperty(propertyName, background), currentColour(value)
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
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(ComboBox::backgroundColourId));

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
        float downValue = 0;

        EditableComponent(String propertyName, Value& value, Colour background) : InspectorProperty(propertyName, background), property(value)
        {
            addAndMakeVisible(label);
            label.setEditable(false, true);
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

        void mouseDown(const MouseEvent& event) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;

            downValue = label.getText().getFloatValue();
        }

        void mouseDrag(const MouseEvent& e) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;

            // Skip for scientific notation
            if (label.getText().contains("e")) return;

            auto const inc = static_cast<float>(-e.getDistanceFromDragStartY()) * 0.5f;
            if (std::abs(inc) < 1.0f) return;

            // Logic for dragging, where the x position decides the precision
            auto currentValue = label.getText();
            if (!currentValue.containsChar('.')) currentValue += '.';
            if (currentValue.getCharPointer()[0] == '-') currentValue = currentValue.substring(1);
            currentValue += "00000";

            // Get position of all numbers
            Array<int> glyphs;
            Array<float> xOffsets;
            label.getFont().getGlyphPositions(currentValue, glyphs, xOffsets);

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
            else
            {
                precision = std::min(precision, 3);
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

            label.setText(newValue, sendNotification);
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

        fb.performLayout(bounds.removeFromBottom(27));
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

        int getNumLines(const String& text)
        {
            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);

            int numLines = 1;

            Array<int> glyphs;
            Array<float> xOffsets;
            font.getGlyphPositions(text, glyphs, xOffsets);

            for (int i = 0; i < xOffsets.size(); i++)
            {
                if ((xOffsets[i] + 12) >= static_cast<float>(getWidth()))
                {
                    for (int j = i + 1; j < xOffsets.size(); j++)
                    {
                        xOffsets.getReference(j) -= xOffsets[i];
                    }
                    numLines++;
                }
            }

            return numLines;
        }

        void mouseDown(const MouseEvent& e) override
        {
            int totalHeight = 0;

            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];

                int numLines = getNumLines(message.first);
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
            g.fillAll(findColour(ComboBox::backgroundColourId));

            int totalHeight = 0;
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();

            bool rowColour = false;

            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];

                int numLines = getNumLines(message.first);
                int height = numLines * 22 + 2;

                const Rectangle<int> r(0, totalHeight, getWidth(), height);

                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors))
                {
                    continue;
                }

                if (rowColour || row == selectedItem)
                {
                    g.setColour(selectedItem == row ? findColour(Slider::thumbColourId) : findColour(ResizableWindow::backgroundColourId));

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
                int numLines = getNumLines(message.first);
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
        static Colour colourWithType(int type)
        {
            if (type == 0)
                return Colours::white;
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

Sidebar::Sidebar(pd::Instance* instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(instance);
    inspector = new Inspector;

    addAndMakeVisible(console);
    addAndMakeVisible(inspector);

    setBounds(getParentWidth() - lastWidth, 40, lastWidth, getParentHeight() - 40);
}

Sidebar::~Sidebar()
{
    delete console;
    delete inspector;
}

void Sidebar::paint(Graphics& g)
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Sidebar
    g.setColour(findColour(ComboBox::backgroundColourId).darker(0.1));
    g.fillRect(getWidth() - sWidth, 0, sWidth, getHeight());
}

void Sidebar::paintOverChildren(Graphics& g)
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Draggable bar
    g.setColour(findColour(ComboBox::backgroundColourId));
    g.fillRect(getWidth() - sWidth, 0, dragbarWidth + 1, getHeight());
}

void Sidebar::resized()
{
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
        // getCurrentCanvas()->checkBounds(); fix this
        draggingSidebar = false;
    }
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
    
    if(!pinned && lastParameters.empty()) {
        hideParameters();
    }
}

bool Sidebar::isPinned(){
    return pinned;
}

void Sidebar::showParameters(ObjectParameters& params)
{
    lastParameters = params;
    inspector->loadParameters(params);

    if(!pinned) {
        inspector->setVisible(true);
        console->setVisible(false);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);
    
    if(!pinned) {
        inspector->setVisible(true);
        console->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    if(!pinned) {
        inspector->setVisible(false);
        console->setVisible(true);
    }
    
    if(pinned) {
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
