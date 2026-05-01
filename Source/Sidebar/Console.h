/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <utility>
#include "Utility/CachedStringWidth.h"
#include "Components/BouncingViewport.h"
#include "Object.h"
#include "Objects/ObjectBase.h"

class ConsoleSettings final : public Component {
public:
    explicit ConsoleSettings(StackArray<Value, 5>& settingsValues)
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

        int const buttonHeight = buttonBounds.getHeight() / buttons.size();

        for (auto* button : buttons) {
            button->setBounds(buttonBounds.removeFromTop(buttonHeight));
        }
    }

private:
    OwnedArray<TextButton> buttons = {
        new CalloutMenuButton(Icons::Clear, "Clear", false),
        new CalloutMenuButton(Icons::Restore, "Restore", false),
        new CalloutMenuButton(Icons::Message, "Show Messages", true),
        new CalloutMenuButton(Icons::Error, "Show Errors", true),
        new CalloutMenuButton(Icons::AutoScroll, "Autoscroll", true)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleSettings)
};

class Console final : public Component
    , public Value::Listener {

public:
    explicit Console(pd::Instance* pd)
    {
        // Viewport takes ownership
        console = new ConsoleComponent(pd, settingsValues, viewport);

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
    }

    static UnorderedMap<String, Object*> getUniqueObjectNames(Canvas* cnv)
    {
        UnorderedMap<String, Object*> result;
        UnorderedMap<String, int> nameCount;
        for (auto* object : cnv->objects) {
            if (!object->gui)
                continue;

            auto tokens = StringArray::fromTokens(object->gui->getText(), false);
            tokens.removeRange(2, tokens.size() - 2);

            auto uniqueName = tokens.joinIntoString("_");
            auto& found = nameCount[uniqueName];
            found++;

            result[uniqueName + "_" + String(found)] = object;
        }

        return result;
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
        auto const bounds = getLocalBounds();
        viewport.setBounds(bounds);

        auto const width = viewport.canScrollVertically() ? viewport.getWidth() - 5.0f : viewport.getWidth();
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

    class ConsoleComponent final : public Component {
        class UpdateNotification final : public TextButton {
        public:
            explicit UpdateNotification(String const& availableVersion)
                : TextButton("Update available: plugdata " + availableVersion)
            {
                setTooltip("Download the latest plugdata release");
                onClick = [] {
                    URL("https://plugdata.org/download").launchInDefaultBrowser();
                };
            }

            void paint(Graphics& g) override
            {
                auto backgroundColour = PlugDataColours::sidebarBackgroundColour.contrasting(isMouseOver() ? 0.075f : 0.04f);
                if (isDown())
                    backgroundColour = PlugDataColours::sidebarActiveBackgroundColour;

                auto const bounds = getLocalBounds().reduced(0, 1).toFloat().withTrimmedTop(0.5f);
                g.setColour(backgroundColour);
                g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius);

                Fonts::drawFittedText(g, getButtonText(), getLocalBounds().reduced(8, 2).translated(0, -1), PlugDataColours::sidebarTextColour, 1, 0.9f, 14);
            }
        };

        class ConsoleMessage final : public Component {

            ConsoleComponent& console;

        public:
            int idx;

            ConsoleMessage(int const index, ConsoleComponent& parent)
                : console(parent)
                , idx(index)
            {
                parent.addAndMakeVisible(this);
            }

            void mouseDown(MouseEvent const& e) override
            {
                if (e.source.isTouch())
                    return;

                handleClick(e);
            }

            void mouseUp(MouseEvent const& e) override
            {
                if (!e.source.isTouch() || e.mouseWasDraggedSinceMouseDown())
                    return;

                handleClick(e);
            }

            void handleClick(MouseEvent const& e)
            {
                if (!e.mods.isShiftDown() && !e.mods.isCommandDown()) {
                    console.selectedItems.clear();
                }

                auto& [object, message, type, length, repeats] = console.pd->getConsoleMessages()[idx];
                if (e.mods.isPopupMenu()) {

                    PopupMenu menu;
                    menu.addItem("Copy", [this] { console.copySelectionToClipboard(); });
                    menu.addItem("Show origin", object != nullptr, false, [this, target = object] {
                        auto* editor = findParentComponentOfClass<PluginEditor>();
                        editor->highlightSearchTarget(target, true);
                    });
                    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this));
                }

                if (e.mods.isShiftDown()) {
                    int startIdx = console.messages.size();
                    int const endIdx = idx;
                    for (auto const& item : console.selectedItems) {
                        startIdx = std::min(item->idx, startIdx);
                    }
                    for (int i = std::min(startIdx, endIdx); i < std::max(startIdx, endIdx); i++) {
                        console.selectedItems.add_unique(SafePointer(console.messages[i].get()));
                    }
                }

                console.selectedItems.add_unique(SafePointer(this));
                console.repaint();
            }

            void paint(Graphics& g) override
            {
                auto const isSelected = console.selectedItems.contains(this);
                auto const showMessages = getValue<bool>(console.settingsValues[2]);
                auto const showErrors = getValue<bool>(console.settingsValues[3]);

                if (isSelected) {
                    // Draw selected background
                    g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
                    g.fillRoundedRectangle(getLocalBounds().reduced(0, 1).toFloat().withTrimmedTop(0.5f), Corners::defaultCornerRadius);

                    for (auto& item : console.selectedItems) {

                        if (!item.getComponent())
                            return;
                        // Draw connected on top
                        if (item->idx == idx - 1) {
                            g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
                            g.fillRect(getLocalBounds().toFloat().withTrimmedBottom(5));

                            g.setColour(PlugDataColours::outlineColour);
                            g.drawLine(10, 0, getWidth() - 10, 0);
                        }

                        // Draw connected on bottom
                        if (item->idx == idx + 1) {
                            g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
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
                auto const totalLength = length + calculateRepeatOffset(repeats);
                auto const numLines = Console::calculateNumLines(message, totalLength, console.getWidth());

                auto textColour = PlugDataColours::sidebarTextColour;

                if (type == 1)
                    textColour = Colours::orange;
                else if (type == 2)
                    textColour = Colours::red;

                auto bounds = getLocalBounds().reduced(8, 2);
                if (repeats > 1) {

                    auto repeatIndicatorBounds = bounds.removeFromLeft(calculateRepeatOffset(repeats)).toFloat().translated(-4, 0.25);
                    repeatIndicatorBounds = repeatIndicatorBounds.withSizeKeepingCentre(repeatIndicatorBounds.getWidth(), 21);

                    auto circleColour = PlugDataColours::sidebarActiveBackgroundColour;
                    auto const backgroundColour = PlugDataColours::sidebarBackgroundColour;
                    auto const contrast = isSelected ? 1.5f : 0.5f;

                    circleColour = Colour(circleColour.getRed() + (circleColour.getRed() - backgroundColour.getRed()) * contrast,
                        circleColour.getGreen() + (circleColour.getGreen() - backgroundColour.getGreen()) * contrast,
                        circleColour.getBlue() + (circleColour.getBlue() - backgroundColour.getBlue()) * contrast);

                    g.setColour(circleColour);
                    auto const circleBounds = repeatIndicatorBounds.reduced(2);
                    g.fillRoundedRectangle(circleBounds, circleBounds.getHeight() / 2.0f);

                    Fonts::drawText(g, String(repeats), repeatIndicatorBounds, PlugDataColours::sidebarTextColour, 12, Justification::centred);
                }

                // Draw text
                Fonts::drawFittedText(g, message, bounds.translated(0, -1), textColour, numLines, 0.9f, 14);
            }
        };

        StackArray<Value, 5>& settingsValues;
        Viewport& viewport;

        pd::Instance* pd; // instance to get console messages from
    public:
        std::deque<std::unique_ptr<ConsoleMessage>> messages;
        SmallArray<SafePointer<ConsoleMessage>> selectedItems;

        ConsoleComponent(pd::Instance* instance, StackArray<Value, 5>& b, Viewport& v)
            : settingsValues(b)
            , viewport(v)
            , pd(instance)
        {
            setAccessible(false);
            setWantsKeyboardFocus(true);

            auto const availableUpdateVersion = getAvailableUpdateVersion();
            if (availableUpdateVersion.isNotEmpty()) {
                updateNotification = std::make_unique<UpdateNotification>(availableUpdateVersion);
                addAndMakeVisible(*updateNotification);
            }

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
            if (key == KeyPress('a', ModifierKeys::commandModifier, 0)) {
                for (auto& message : messages) {
                    selectedItems.add_unique(SafePointer(message.get()));
                    repaint();
                }
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
        int getTotalHeight() const
        {
            auto const showMessages = getValue<bool>(settingsValues[2]);
            auto const showErrors = getValue<bool>(settingsValues[3]);
            auto totalHeight = 0;

            for (auto& [object, message, type, length, repeats] : pd->getConsoleMessages()) {
                auto const totalLength = length + calculateRepeatOffset(repeats);
                auto const numLines = Console::calculateNumLines(message, totalLength, getWidth());
                auto height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                totalHeight += std::max(0, height);
            }

            return totalHeight + getUpdateNotificationHeight() + 8;
        }

        static int calculateRepeatOffset(int const numRepeats)
        {
            if (numRepeats == 0)
                return 0;

            int const digitCount = static_cast<int>(std::log10(numRepeats)) + 1;
            return digitCount <= 2 ? 21 : 21 + (digitCount - 2) * 10;
        }

        void mouseDown(MouseEvent const& e) override
        {
            if (!isRealClickEvent(e))
                return;

            selectedItems.clear();
            repaint();
        }

        void resized() override
        {
            auto const showMessages = getValue<bool>(settingsValues[2]);
            auto const showErrors = getValue<bool>(settingsValues[3]);

            int totalHeight = 4;
            if (updateNotification) {
                int const rightMargin = viewport.canScrollVertically() ? 13 : 11;
                updateNotification->setBounds(6, totalHeight, getWidth() - rightMargin, updateNotificationHeight);
                totalHeight += updateNotificationHeight;
            }

            for (int row = 0; row < static_cast<int>(pd->getConsoleMessages().size()); row++) {
                if (row >= messages.size())
                    break;

                auto& [object, message, type, length, repeats] = pd->getConsoleMessages()[row];

                auto const totalLength = length + calculateRepeatOffset(repeats);
                auto const numLines = Console::calculateNumLines(message, totalLength, getWidth());
                auto const height = numLines * 13 + 12;

                if ((type == 0 && !showMessages) || (type == 1 && !showErrors))
                    continue;

                int const rightMargin = viewport.canScrollVertically() ? 13 : 11;
                messages[row]->setBounds(6, totalHeight, getWidth() - rightMargin, height);

                totalHeight += height;
            }
        }

        static String normaliseVersionTag(String version)
        {
            version = version.trim().trimCharactersAtStart("vV");

            auto const suffixStart = version.indexOfChar('-');
            if (suffixStart >= 0)
                version = version.substring(0, suffixStart);

            return version.trim();
        }

        static Array<int> getVersionParts(String version)
        {
            Array<int> parts;
            String currentPart;

            version = normaliseVersionTag(version);

            for (int i = 0; i < version.length(); i++) {
                auto const character = version[i];
                if (CharacterFunctions::isDigit(character)) {
                    currentPart += String::charToString(character);
                    continue;
                }

                if (currentPart.isNotEmpty()) {
                    parts.add(currentPart.getIntValue());
                    currentPart.clear();
                }
            }

            if (currentPart.isNotEmpty())
                parts.add(currentPart.getIntValue());

            while (parts.size() < 4) {
                parts.add(0);
            }

            return parts;
        }

        static int compareVersions(String const& lhs, String const& rhs)
        {
            auto const lhsParts = getVersionParts(lhs);
            auto const rhsParts = getVersionParts(rhs);
            auto const numParts = std::max(lhsParts.size(), rhsParts.size());

            for (int i = 0; i < numParts; i++) {
                auto const lhsPart = i < lhsParts.size() ? lhsParts[i] : 0;
                auto const rhsPart = i < rhsParts.size() ? rhsParts[i] : 0;

                if (lhsPart != rhsPart)
                    return lhsPart > rhsPart ? 1 : -1;
            }

            return 0;
        }

        static String getLatestReleaseTag()
        {
            auto const testUpdateVersion = SystemStats::getEnvironmentVariable("PLUGDATA_TEST_UPDATE_VERSION", {}).trim();
            if (testUpdateVersion.isNotEmpty())
                return testUpdateVersion;

            int statusCode = 0;
            auto releaseInfoStream = URL("https://api.github.com/repos/plugdata-team/plugdata/releases/latest")
                                         .createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                                                                .withConnectionTimeoutMs(1000)
                                                                .withStatusCode(&statusCode)
                                                                .withExtraHeaders("User-Agent: plugdata\r\nAccept: application/vnd.github+json\r\n"));

            if (statusCode != 200 || releaseInfoStream == nullptr)
                return {};

            auto releaseInfo = JSON::parse(releaseInfoStream->readEntireStreamAsString());
            auto const latestReleaseVersion = releaseInfo.getProperty("tag_name", {}).toString().trim();

            return latestReleaseVersion.containsIgnoreCase("test") ? String() : latestReleaseVersion;
        }

        static String getAvailableUpdateVersion()
        {
            auto const latestReleaseVersion = normaliseVersionTag(getLatestReleaseTag());
            if (latestReleaseVersion.isEmpty())
                return {};

            return compareVersions(latestReleaseVersion, normaliseVersionTag(PLUGDATA_VERSION)) > 0 ? latestReleaseVersion : String();
        }

        int getUpdateNotificationHeight() const
        {
            return updateNotification ? updateNotificationHeight : 0;
        }

        static constexpr int updateNotificationHeight = 30;
        std::unique_ptr<UpdateNotification> updateNotification;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show console settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [this, settingsCalloutButton] {
            auto consoleSettings = std::make_unique<ConsoleSettings>(settingsValues);
            auto const bounds = settingsCalloutButton->getScreenBounds();
            auto* pluginEditor = findParentComponentOfClass<PluginEditor>();
            pluginEditor->showCalloutBox(std::move(consoleSettings), bounds);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

    static int calculateNumLines(String const& message, int const length, int maxWidth)
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
        }
        if (length == 0)
            return 0;
        return std::max<int>(round(static_cast<float>(length) / maxWidth), 1);
    }

private:
    StackArray<Value, 5> settingsValues;
    ConsoleComponent* console;
    BouncingViewport viewport;
};
