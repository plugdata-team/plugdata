/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct Console : public Component {
    explicit Console(pd::Instance* instance)
    {
        // Viewport takes ownership
        console = new ConsoleComponent(instance, buttons, viewport);

        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);

        console->setVisible(true);

        addAndMakeVisible(viewport);

        std::vector<String> tooltips = { "Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll" };

        std::vector<std::function<void()>> callbacks = {
            [this]() { console->clear(); },
            [this]() { console->restore(); },
            [this]() { console->update(); },
            [this]() { console->update(); },
            [this]() { console->update(); },

        };

        int i = 0;
        for (auto& button : buttons) {
            button.setName("statusbar:console");
            button.setConnectedEdges(12);
            addAndMakeVisible(button);

            button.onClick = callbacks[i];
            button.setTooltip(tooltips[i]);

            i++;
        }

        for (int n = 2; n < 5; n++) {
            buttons[n].setClickingTogglesState(true);
            buttons[n].setToggleState(true, dontSendNotification);
        }

        resized();
    }

    ~Console() override
    {
    }

    void resized() override
    {
        auto fb = FlexBox(FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::flexStart, FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexStart);

        for (auto& b : buttons) {
            auto item = FlexItem(b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }

        auto bounds = getLocalBounds().toFloat();

        fb.performLayout(bounds.removeFromBottom(28));
        
        viewport.setBounds(bounds.toNearestInt());
        console->setSize(viewport.getWidth(), std::max<int>(console->getTotalHeight(), viewport.getHeight()));
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

    void paint(Graphics& g) override
    {
        // Draw background if we don't have enough messages to fill the panel
        int h = 24;
        int y = console->getTotalHeight();
        int idx = static_cast<int>(console->messages.size());
        while (y < console->getHeight()) {

            if (y + h > console->getHeight()) {
                h = console->getHeight() - y;
            }

            idx++;
            y += h;
        }
    }

    struct ConsoleComponent : public Component {
        struct ConsoleMessage : public Component {
            int idx;
            ConsoleComponent& console;

            ConsoleMessage(int index, ConsoleComponent& parent)
                : idx(index)
                , console(parent)
            {

                parent.addAndMakeVisible(this);
            }

            Colour colourWithType(int type)
            {
                if (type == 0)
                    return findColour(PlugDataColour::panelTextColourId);
                else if (type == 1)
                    return Colours::orange;
                else
                    return Colours::red;
            }

            void mouseDown(MouseEvent const& e)
            {
                console.selectedItem = idx;
                console.repaint();
            }

            void paint(Graphics& g)
            {
                auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
                g.setFont(font);

                bool isSelected = console.selectedItem == idx;
                bool showMessages = console.buttons[2].getToggleState();
                bool showErrors = console.buttons[3].getToggleState();

                // Draw background
                auto background = findColour(PlugDataColour::panelBackgroundColourId);

                g.setColour(isSelected ? findColour(PlugDataColour::panelActiveBackgroundColourId) : background);
                g.fillRoundedRectangle(getLocalBounds().reduced(4, 2).toFloat(), 4.0f);

                // Get console message
                auto& [message, type, length] = console.pd->getConsoleMessages()[idx];

                // Check if message type should be visible
                if ((type == 1 && !showMessages) || (type == 0 && !showErrors)) {
                    return;
                }

                // Approximate number of lines from string length and current width
                int numLines = getNumLines(console.getWidth(), length);

                // Draw text
                g.setColour(isSelected ? findColour(PlugDataColour::panelActiveTextColourId) : colourWithType(type));
                g.drawFittedText(message, getLocalBounds().reduced(4, 0).withTrimmedLeft(6), Justification::centredLeft, numLines, 1.0f);
            }
        };

        std::deque<std::unique_ptr<ConsoleMessage>> messages;

        std::array<TextButton, 5>& buttons;
        Viewport& viewport;

        pd::Instance* pd; // instance to get console messages from

        ConsoleComponent(pd::Instance* instance, std::array<TextButton, 5>& b, Viewport& v)
            : buttons(b)
            , viewport(v)
            , pd(instance)
        {
            setWantsKeyboardFocus(true);
            repaint();
        }

        void focusLost(FocusChangeType cause) override
        {
            selectedItem = -1;
        }

        bool keyPressed(KeyPress const& key) override
        {
            if (isPositiveAndBelow(selectedItem, pd->getConsoleMessages().size())) {
                // Copy console item
                if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
                    SystemClipboard::copyTextToClipboard(std::get<0>(pd->getConsoleMessages()[selectedItem]));
                    return true;
                }
            }
            return false;
        }

        void update()
        {
            while (messages.size() > pd->getConsoleMessages().size() || messages.size() >= 800) {
                messages.pop_front();

                // Make sure we don't trigger a repaint for all messages when the console is full
                for (int row = 0; row < static_cast<int>(messages.size()); row++) {
                    messages[row]->idx--;
                }
            }

            while (messages.size() < pd->getConsoleMessages().size()) {
                messages.push_back(std::make_unique<ConsoleMessage>(messages.size(), *this));
            }

            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();

            int totalHeight = 0;
            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                auto [message, type, length] = pd->getConsoleMessages()[row];
                int numLines = getNumLines(getWidth(), length);
                int height = numLines * 22 + 2;

                if (messages[row]->idx != row) {
                    messages[row]->idx = row;
                    messages[row]->repaint();
                }

                if ((type == 1 && !showMessages) || (length == 0 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            setSize(viewport.getWidth(), std::max<int>(getTotalHeight(), viewport.getHeight()));
            resized();

            if (buttons[4].getToggleState()) {
                viewport.setViewPositionProportionately(0.0f, 1.0f);
            }
        }

        void clear()
        {
            pd->getConsoleHistory().insert(pd->getConsoleHistory().end(), pd->getConsoleMessages().begin(), pd->getConsoleMessages().end());
            pd->getConsoleMessages().clear();
            update();
        }

        void restore()
        {
            pd->getConsoleMessages().insert(pd->getConsoleMessages().begin(), pd->getConsoleHistory().begin(), pd->getConsoleHistory().end());
            pd->getConsoleHistory().clear();
            update();
        }

        // Get total height of messages, also taking multi-line messages into account
        int getTotalHeight()
        {
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();

            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
            int totalHeight = 0;

            for (auto& [message, type, length] : pd->getConsoleMessages()) {
                int numLines = getNumLines(getWidth(), length);
                int height = numLines * 22 + 2;

                if ((type == 1 && !showMessages) || (length == 0 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            return totalHeight;
        }

        void resized() override
        {
            int totalHeight = 2;
            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                if (row >= messages.size())
                    break;

                auto& [message, type, length] = pd->getConsoleMessages()[row];

                int numLines = getNumLines(getWidth(), length);
                int height = numLines * 22 + 4;

                messages[row]->setBounds(0, totalHeight, getWidth(), height);

                totalHeight += height;
            }
        }

        int selectedItem = -1;

    private:
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };

private:
    ConsoleComponent* console;
    Viewport viewport;

    int pendingUpdates = 0;

    std::array<TextButton, 5> buttons = { TextButton(Icons::ClearLarge), TextButton(Icons::Restore), TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll) };
};
