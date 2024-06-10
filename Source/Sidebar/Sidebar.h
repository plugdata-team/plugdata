/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "LookAndFeel.h"
#include "Components/Buttons.h"
#include "Objects/ObjectParameters.h"
#include "Utility/SettingsFile.h"
#include "NVGSurface.h"

class Console;
class Inspector;
class DocumentationBrowser;
class AutomationPanel;
class SearchPanel;
class PluginProcessor;

namespace pd {
class Instance;
}

class SidebarSelectorButton : public TextButton {
public:
    explicit SidebarSelectorButton(String const& icon)
        : TextButton(icon)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        numNotifications = 0;
        hasWarning = false;
        TextButton::mouseDown(e);
    }

    void paint(Graphics& g) override
    {
        bool active = isMouseOver() || isMouseButtonDown() || getToggleState();

        auto cornerSize = Corners::defaultCornerRadius;

        auto backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto bounds = getLocalBounds().toFloat().reduced(3.0f, 4.0f);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto font = Fonts::getIconFont().withHeight(13);
        g.setFont(font);
        g.setColour(findColour(PlugDataColour::toolbarTextColourId));

        int const yIndent = jmin<int>(4, proportionOfHeight(0.3f));

        int const fontHeight = roundToInt(font.getHeight() * 0.6f);
        int const leftIndent = jmin<int>(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
        int const rightIndent = jmin<int>(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
        int const textWidth = getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);

        if (numNotifications) {
            auto notificationBounds = getLocalBounds().removeFromBottom(15).removeFromRight(15).translated(-1, -1);
            auto bubbleColour = hasWarning ? Colours::orange : findColour(PlugDataColour::toolbarActiveColourId);
            g.setColour(bubbleColour.withAlpha(0.8f));
            g.fillEllipse(notificationBounds.toFloat());
            g.setFont(Font(numNotifications >= 100 ? 8 : 12));
            g.setColour(bubbleColour.darker(0.6f).contrasting());
            g.drawText(numNotifications > 99 ? String("99+") : String(numNotifications), notificationBounds, Justification::centred);
        }
    }

    bool hasWarning = false;
    int numNotifications = 0;
};

class PluginEditor;
class PlugDataParameter;
class Sidebar : public Component
    , public SettingsFileListener {

public:
    explicit Sidebar(PluginProcessor* instance, PluginEditor* parent);

    ~Sidebar() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void showParameters(String const& name, Array<ObjectParameters>& params);
    void hideParameters();

    bool isShowingBrowser();

    void propertyChanged(String const& name, var const& value) override;

    void showPanel(int panelToShow);

    void showSidebar(bool show);

    void pinSidebar(bool pin);

    bool isPinned() const;
    bool isHidden() const;

    void clearConsole();
    void updateConsole(int numMessages, bool newWarning);

    void clearSearchOutliner();

    void updateAutomationParameterValue(PlugDataParameter* param);
    void updateAutomationParameters();

    static constexpr int dragbarWidth = 6;

private:
    void updateExtraSettingsButton();

    PluginProcessor* pd;
    PluginEditor* editor;
    Array<ObjectParameters> lastParameters;

    SidebarSelectorButton consoleButton = SidebarSelectorButton(Icons::Console);
    SidebarSelectorButton browserButton = SidebarSelectorButton(Icons::Documentation);
    SidebarSelectorButton automationButton = SidebarSelectorButton(Icons::Parameters);
    SidebarSelectorButton searchButton = SidebarSelectorButton(Icons::Search);

    std::unique_ptr<Component> extraSettingsButton;
    SmallIconButton panelPinButton = SmallIconButton(Icons::Pin);

    std::unique_ptr<Console> console;
    std::unique_ptr<Inspector> inspector;
    std::unique_ptr<DocumentationBrowser> browser;
    std::unique_ptr<AutomationPanel> automationPanel;
    std::unique_ptr<SearchPanel> searchPanel;

    StringArray panelNames = { "Console", "Documentation Browser", "Automation Parameters", "Search" };
    int currentPanel = 0;

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    bool pinned = false;

    int lastWidth = 250;
};
