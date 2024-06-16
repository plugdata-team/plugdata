/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <utility>
#include "Components/BouncingViewport.h"
#include "Object.h"

class ConsoleSettings : public Component {
public:
    struct ConsoleSettingsButton : public TextButton {
        String const icon;
        String const description;

        ConsoleSettingsButton(String iconString, String descriptionString, bool toggleButton)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
            setClickingTogglesState(toggleButton);
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::toolbarTextColourId);
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14);

            if (getToggleState()) {
                colour = findColour(PlugDataColour::toolbarActiveColourId);
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    explicit ConsoleSettings(std::array<Value, 5>& settingsValues)
    {
        for (auto* button : buttons) {
            addAndMakeVisible(*button);
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

        setSize(150, 135);
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
                : console(parent)
                , idx(index)
            {
                parent.addAndMakeVisible(this);
            }

            void mouseDown(MouseEvent const& e) override
            {
                if (!e.mods.isShiftDown() && !e.mods.isCommandDown()) {
                    console.selectedItems.clear();
                }

                auto& [object, message, type, length, repeats] = console.pd->getConsoleMessages()[idx];
                if (e.mods.isPopupMenu()) {

                    PopupMenu menu;
                    menu.addItem("Copy", [this]() { console.copySelectionToClipboard(); });
                    menu.addItem("Show origin", object != nullptr, false, [this, target = object]() {
                        auto* editor = findParentComponentOfClass<PluginEditor>();
                        editor->highlightSearchTarget(target, true);
                    });
                    menu.showMenuAsync(PopupMenu::Options());
                }

                console.selectedItems.addIfNotAlreadyThere(SafePointer(this));
                console.repaint();
            }

            void paint(Graphics& g) override
            {
                auto isSelected = console.selectedItems.contains(this);
                auto showMessages = getValue<bool>(console.settingsValues[2]);
                auto showErrors = getValue<bool>(console.settingsValues[3]);

                if (isSelected) {
                    // Draw selected background
                    g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                    g.fillRoundedRectangle(getLocalBounds().reduced(0, 1).toFloat().withTrimmedTop(0.5f), Corners::defaultCornerRadius);

                    for (auto& item : console.selectedItems) {

                        if (!item.getComponent())
                            return;
                        // Draw connected on top
                        if (item->idx == idx - 1) {
                            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                            g.fillRect(getLocalBounds().toFloat().withTrimmedBottom(5));

                            g.setColour(findColour(PlugDataColour::outlineColourId));
                            g.drawLine(10, 0, getWidth() - 10, 0);
                        }

                        // Draw connected on bottom
                        if (item->idx == idx + 1) {
                            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                            g.fillRect(getLocalBounds().toFloat().withTrimmedTop(5));
                        }
                    }
                }

                // Get console message
                auto& [object, message, type, length, repeats] = console.pd->getConsoleMessages()[idx];

                // Check if message type should be visible
                if ((type == 0 && !showMessages) || (type == 1 && !showErrors)) {
                    return;
                }

                // Approximate number of lines from string length and current width
                auto totalLength = length + calculateRepeatOffset(repeats);
                auto numLines = Console::calculateNumLines(message, totalLength, console.getWidth());

                auto textColour = findColour(PlugDataColour::sidebarTextColourId);

                if (type == 1)
                    textColour = Colours::orange;
                else if (type == 2)
                    textColour = Colours::red;

                auto bounds = getLocalBounds().reduced(8, 2);
                if (repeats > 1) {

                    auto repeatIndicatorBounds = bounds.removeFromLeft(calculateRepeatOffset(repeats)).toFloat().translated(-4, 0.25);
                    repeatIndicatorBounds = repeatIndicatorBounds.withSizeKeepingCentre(repeatIndicatorBounds.getWidth(), 21);

                    auto circleColour = findColour(PlugDataColour::sidebarActiveBackgroundColourId);
                    auto backgroundColour = findColour(PlugDataColour::sidebarBackgroundColourId);
                    auto contrast = isSelected ? 1.5f : 0.5f;

                    circleColour = Colour(circleColour.getRed() + (circleColour.getRed() - backgroundColour.getRed()) * contrast,
                        circleColour.getGreen() + (circleColour.getGreen() - backgroundColour.getGreen()) * contrast,
                        circleColour.getBlue() + (circleColour.getBlue() - backgroundColour.getBlue()) * contrast);

                    g.setColour(circleColour);
                    auto circleBounds = repeatIndicatorBounds.reduced(2);
                    g.fillRoundedRectangle(circleBounds, circleBounds.getHeight() / 2.0f);

                    Fonts::drawText(g, String(repeats), repeatIndicatorBounds, findColour(PlugDataColour::sidebarTextColourId), 12, Justification::centred);
                }

                // Draw text
                Fonts::drawFittedText(g, message, bounds.translated(0, -1), textColour, numLines, 0.9f, 14);
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
            setAccessible(false);
            setWantsKeyboardFocus(true);
            repaint();
        }

        void focusLost(FocusChangeType cause) override
        {
            selectedItems.clear();
            repaint();
        }

        void copySelectionToClipboard()
        {
            String textToCopy;
            for (auto& item : selectedItems) {
                if (!item.getComponent())
                    continue;
                textToCopy += std::get<1>(pd->getConsoleMessages()[item->idx]) + "\n";
            }

            SystemClipboard::copyTextToClipboard(textToCopy.trimEnd());
        }

        bool keyPressed(KeyPress const& key) override
        {
            // Copy from console
            if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
                copySelectionToClipboard();
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

            for (auto& [object, message, type, length, repeats] : pd->getConsoleMessages()) {
                auto totalLength = length + calculateRepeatOffset(repeats);
                auto numLines = Console::calculateNumLines(message, totalLength, getWidth());
                auto height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            return totalHeight + 8;
        }

        static int calculateRepeatOffset(int numRepeats)
        {
            if (numRepeats == 0)
                return 0;

            int digitCount = static_cast<int>(std::log10(numRepeats)) + 1;
            return digitCount <= 2 ? 21 : 21 + ((digitCount - 2) * 10);
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

            int totalHeight = 4;
            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                if (row >= messages.size())
                    break;

                auto& [object, message, type, length, repeats] = pd->getConsoleMessages()[row];

                auto totalLength = length + calculateRepeatOffset(repeats);
                auto numLines = Console::calculateNumLines(message, totalLength, getWidth());
                auto height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                int rightMargin = viewport.canScrollVertically() ? 13 : 11;
                messages[row]->setBounds(6, totalHeight, getWidth() - rightMargin, height);

                totalHeight += height;
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show console settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [this, settingsCalloutButton]() {
            auto consoleSettings = std::make_unique<ConsoleSettings>(settingsValues);
            auto bounds = settingsCalloutButton->getScreenBounds();
            CallOutBox::launchAsynchronously(std::move(consoleSettings), bounds, nullptr);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

    static int calculateNumLines(String& message, int length, int maxWidth)
    {
        maxWidth -= 38.0f;
        if (message.containsAnyOf("\n\r") && message.containsNonWhitespaceChars()) {
            int numLines = 0;
            for (auto line : StringArray::fromLines(message)) {
                numLines++;
                int lineWidth = CachedStringWidth<14>::calculateSingleLineWidth(line);
                while (lineWidth > maxWidth && numLines < 64) {
                    lineWidth -= maxWidth;
                    numLines++;
                }
            }
            return numLines;
        } else {
            if (length == 0)
                return 0;
            return std::max<int>(round(static_cast<float>(length) / maxWidth), 1);
        }

        return 1;
    }

private:
    std::array<Value, 5> settingsValues;
    ConsoleComponent* console;
    BouncingViewport viewport;
};
