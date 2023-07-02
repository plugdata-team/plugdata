#include <utility>
#include "Utility/BouncingViewport.h"
/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ConsoleSettings : public Component {
public:
    struct ConsoleSettingsButton : public TextButton {
        const String icon;
        const String description;

        ConsoleSettingsButton(String iconString, String descriptionString, bool toggleButton)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
            setClickingTogglesState(toggleButton);
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::toolbarTextColourId);

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14);

            if (isMouseOver()) {
                colour = colour.brighter(0.4f);
            }
            if (getToggleState()) {
                colour = findColour(PlugDataColour::toolbarActiveColourId);
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(6), colour, 14, false);
        }
    };

    explicit ConsoleSettings(std::array<Value, 5>& settingsValues)
    {
        auto i = 0;
        for (auto* button : buttons) {
            addAndMakeVisible(*button);
            i++;
        }

        for (int i = 0; i < buttons.size(); i++) {

            if (buttons[i]->getClickingTogglesState()) {
                // For toggle buttons, assign the button state to the Value
                buttons[i]->getToggleStateValue().referTo(settingsValues[i]);
            } else {
                // For action buttons, just trigger the Value on an off to send a change message
                buttons[i]->onClick = [settingsValues, i]() mutable {
                    settingsValues[i] = !getValue<bool>(settingsValues[i]);
                };
            }
        }

        setSize(150, 150);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int buttonHeight = buttonBounds.getHeight() / buttons.size();

        for (auto* button : buttons) {
            button->setBounds(buttonBounds.removeFromTop(buttonHeight));
        }
    }

private:
    OwnedArray<TextButton> buttons = {
        new ConsoleSettingsButton(Icons::Clear, "Clear", false),
        new ConsoleSettingsButton(Icons::Restore, "Restore", false),
        new ConsoleSettingsButton(Icons::Message, "Show Messages", true),
        new ConsoleSettingsButton(Icons::Error, "Show Errors", true),
        new ConsoleSettingsButton(Icons::AutoScroll, "Autoscroll", true)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleSettings)
};

class Console : public Component
    , public Value::Listener {

public:
    explicit Console(pd::Instance* instance)
    {
        // Viewport takes ownership
        console = new ConsoleComponent(instance, settingsValues, viewport);

        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);

        console->setVisible(true);

        addAndMakeVisible(viewport);

        for (auto& settingsValue : settingsValues) {
            settingsValue.addListener(this);
        }

        // Show messages, show errors and autoscroll should be enabled by default
        settingsValues[2] = true;
        settingsValues[3] = true;
        settingsValues[4] = true;

        resized();
    }

    ~Console() override = default;

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(settingsValues[0])) {
            console->clear();
        } else if (v.refersToSameSourceAs(settingsValues[1])) {
            console->restore();
        } else {
            update();
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        viewport.setBounds(bounds);

        auto width = viewport.canScrollVertically() ? viewport.getWidth() - 5.0f : viewport.getWidth();
        console->setSize(width, std::max<int>(console->getTotalHeight(), viewport.getHeight()));
    }

    void clear()
    {
        console->clear();
    }

    void update()
    {
        console->update();
        resized();
        repaint();
    }

    void deselect()
    {
        console->selectedItems.clear();
        repaint();
    }

    class ConsoleComponent : public Component {
        class ConsoleMessage : public Component {

            ConsoleComponent& console;

        public:
            int idx;

            ConsoleMessage(int index, ConsoleComponent& parent)
                : idx(index)
                , console(parent)
            {

                parent.addAndMakeVisible(this);
            }

            void mouseDown(MouseEvent const& e)
            {
                if (!e.mods.isShiftDown() && !e.mods.isCommandDown()) {
                    console.selectedItems.clear();
                }

                console.selectedItems.addIfNotAlreadyThere(SafePointer(this));
                console.repaint();
            }

            void paint(Graphics& g)
            {
                auto isSelected = console.selectedItems.contains(this);
                auto showMessages = getValue<bool>(console.settingsValues[2]);
                auto showErrors = getValue<bool>(console.settingsValues[3]);

                if (isSelected) {
                    // Draw selected background
                    g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                    PlugDataLook::fillSmoothedRectangle(g, getLocalBounds().reduced(6, 1).toFloat(), Corners::defaultCornerRadius);

                    for (auto& item : console.selectedItems) {

                        if (!item.getComponent())
                            return;
                        // Draw connected on top
                        if (item->idx == idx - 1) {
                            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                            g.fillRect(getLocalBounds().reduced(6, 0).toFloat().withTrimmedBottom(5));

                            g.setColour(findColour(PlugDataColour::outlineColourId));
                            g.drawLine(10, 0, getWidth() - 10, 0);
                        }

                        // Draw connected on bottom
                        if (item->idx == idx + 1) {
                            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                            g.fillRect(getLocalBounds().reduced(6, 0).toFloat().withTrimmedTop(5));
                        }
                    }
                }

                // Get console message
                auto& [message, type, length] = console.pd->getConsoleMessages()[idx];

                // Check if message type should be visible
                if ((type == 0 && !showMessages) || (type == 1 && !showErrors)) {
                    return;
                }

                // Approximate number of lines from string length and current width
                auto numLines = StringUtils::getNumLines(console.getWidth(), length);

                auto textColour = findColour(isSelected ? PlugDataColour::sidebarActiveTextColourId : PlugDataColour::sidebarTextColourId);

                if (type == 1)
                    textColour = Colours::orange;
                else if (type == 2)
                    textColour = Colours::red;

                // Draw text
                Fonts::drawFittedText(g, message, getLocalBounds().reduced(14, 2), textColour, numLines, 0.9f, 14);
            }
        };

        std::array<Value, 5>& settingsValues;
        Viewport& viewport;

        pd::Instance* pd; // instance to get console messages from
    public:
        std::deque<std::unique_ptr<ConsoleMessage>> messages;
        Array<SafePointer<ConsoleMessage>> selectedItems;

        ConsoleComponent(pd::Instance* instance, std::array<Value, 5>& b, Viewport& v)
            : settingsValues(b)
            , viewport(v)
            , pd(instance)
        {
            setWantsKeyboardFocus(true);
            repaint();
        }

        void focusLost(FocusChangeType cause) override
        {
            selectedItems.clear();
            repaint();
        }

        bool keyPressed(KeyPress const& key) override
        {
            // Copy from console
            if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
                String textToCopy;
                for (auto& item : selectedItems) {
                    if (!item.getComponent())
                        continue;
                    textToCopy += std::get<0>(pd->getConsoleMessages()[item->idx]) + "\n";
                }

                textToCopy.trimEnd();
                SystemClipboard::copyTextToClipboard(textToCopy);
                return true;
            }

            return false;
        }

        void update()
        {
            while (messages.size() > pd->getConsoleMessages().size() || messages.size() >= 800) {
                messages.pop_front();

                // Make sure we don't trigger a repaint for all messages when the console is full
                for (auto const& message : messages) {
                    message->idx--;
                }
            }

            while (messages.size() < pd->getConsoleMessages().size()) {
                messages.push_back(std::make_unique<ConsoleMessage>(messages.size(), *this));
            }

            auto showMessages = getValue<bool>(settingsValues[2]);
            auto showErrors = getValue<bool>(settingsValues[3]);

            int totalHeight = 0;
            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                auto [message, type, length] = pd->getConsoleMessages()[row];
                auto numLines = StringUtils::getNumLines(getWidth(), length);
                auto height = numLines * 13 + 12;

                if (messages[row]->idx != row) {
                    messages[row]->idx = row;
                    messages[row]->repaint();
                }

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            setSize(getWidth(), std::max<int>(getTotalHeight(), viewport.getHeight()));
            resized();

            if (getValue<bool>(settingsValues[4])) {
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
            auto showMessages = getValue<bool>(settingsValues[2]);
            auto showErrors = getValue<bool>(settingsValues[3]);
            auto totalHeight = 0;

            for (auto& [message, type, length] : pd->getConsoleMessages()) {
                auto numLines = StringUtils::getNumLines(getWidth(), length);
                auto height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            return totalHeight;
        }

        void mouseDown(MouseEvent const& e) override
        {
            selectedItems.clear();
            repaint();
        }

        void resized() override
        {
            auto showMessages = getValue<bool>(settingsValues[2]);
            auto showErrors = getValue<bool>(settingsValues[3]);

            int totalHeight = 2;
            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                if (row >= messages.size())
                    break;

                auto& [message, type, length] = pd->getConsoleMessages()[row];

                auto numLines = StringUtils::getNumLines(getWidth(), length);
                auto height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                messages[row]->setBounds(0, totalHeight, getWidth(), height);

                totalHeight += height;
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };

    void showCalloutBox(Rectangle<int> bounds, PluginEditor* editor)
    {
        auto consoleSettings = std::make_unique<ConsoleSettings>(settingsValues);
        CallOutBox::launchAsynchronously(std::move(consoleSettings), bounds, editor);
    }

private:
    std::array<Value, 5> settingsValues;
    ConsoleComponent* console;
    BouncingViewport viewport;

    int pendingUpdates = 0;
};
