/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

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
        auto fb = FlexBox(FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::flexStart, FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexStart);

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

                const Rectangle<int> r(0, totalHeight, getWidth(), height);

                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors))
                {
                    continue;
                }

                auto offColour = findColour(PlugDataColour::canvasColourId);
                auto onColour = offColour.darker(0.04f);
                auto background = rowColour ? offColour : onColour;

                if (row == selectedItem)
                {
                    background = findColour(PlugDataColour::highlightColourId);
                }

                g.setColour(background);
                g.fillRect(r);

                rowColour = !rowColour;

                g.setColour(selectedItem == row ? Colours::white : colourWithType(message.second));
                g.drawFittedText(message.first, r.reduced(4, 0), Justification::centredLeft, numLines, 1.0f);

                totalHeight += height;
            }

            while (totalHeight < viewport.getHeight())
            {
                auto offColour = findColour(PlugDataColour::canvasColourId);
                auto onColour = offColour.darker(0.04f);

                const Rectangle<int> r(0, totalHeight, getWidth(), 24);

                g.setColour(rowColour ? offColour : onColour);
                g.fillRect(r);

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
