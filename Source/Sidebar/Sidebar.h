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

class InspectorButton : public Component, public SettableTooltipClient {

public:
    std::function<void()> onClick = [](){};

    explicit InspectorButton(String const& icon) : icon(icon)
    {
        updateTooltip();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        incrementState();

        onClick();
    }

    bool hitTest(int x, int y) override
    {
        if (getLocalBounds().reduced(3,4).contains(Point<int>(x,y)))
            return true;

        return false;
    }

    void mouseEnter(const MouseEvent& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(const MouseEvent& e) override
    {
        isHovering = false;
        repaint();
    }

    void updateTooltip()
    {
        switch(state){
            case InspectorOff:
                setTooltip("Hidden, click to auto show inspector");
                break;
            case InspectorAuto:
                setTooltip("Auto, click to pin inspector");
                break;
            case InspectorPin:
                setTooltip("Pinned, click to hide inspector");
                break;
            default:
                break;
        }
    }

    void incrementState()
    {
        state++;
        state = state % 3;

        updateTooltip();

        repaint();
    }

    bool isInspectorActive()
    {
        return state >= 1;
    }

    bool isInspectorAuto()
    {
        return state == InspectorState::InspectorAuto;
    }

    bool isInspectorPinned()
    {
        return state == InspectorState::InspectorPin;
    }

    void paint(Graphics& g) override
    {
        bool active = isHovering || state == InspectorPin;
        bool stateAuto = state == InspectorAuto;

        auto cornerSize = Corners::defaultCornerRadius;

        auto backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto bounds = getLocalBounds().toFloat().reduced(3.0f, 4.0f);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto font = Fonts::getIconFont().withHeight(13);
        g.setFont(font);
        g.setColour(stateAuto ? findColour(PlugDataColour::objectSelectedOutlineColourId) : findColour(PlugDataColour::toolbarTextColourId));

        int const yIndent = jmin<int>(4, proportionOfHeight(0.3f));
        int const textWidth = getWidth() - 4;

        if (textWidth > 0)
            g.drawFittedText(icon, 2, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);
    }

private:
    enum InspectorState { InspectorOff, InspectorAuto, InspectorPin};
    int state = InspectorAuto;
    String icon;
    bool isHovering = false;
};

class SidebarSelectorButton : public TextButton {
public:
    explicit SidebarSelectorButton(String const& icon)
        : TextButton(icon)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

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

    void showParameters(Array<Component*> comps, SmallArray<ObjectParameters, 6>& params, bool showOnSelect = false);
    void hideParameters();

    bool isShowingBrowser();

    void settingsChanged(String const& name, var const& value) override;

    enum SidePanel { ConsolePan, DocPan, ParamPan, SearchPan, InspectorPan };

    void showPanel(SidePanel panelToShow);

    void showSidebar(bool show);

    void updateSearch();

    bool isHidden() const;

    void clearConsole();
    void updateConsole(int numMessages, bool newWarning);

    void clearSearchOutliner();

    void updateAutomationParameterValue(PlugDataParameter* param);
    void updateAutomationParameters();

    static constexpr int dragbarWidth = 6;

private:
    void updateGeometry();

    void updateExtraSettingsButton();

    PluginProcessor* pd;
    PluginEditor* editor;
    SmallArray<ObjectParameters, 6> lastParameters;
    Array<SafePointer<Component>> lastObjects;

    // Make sure that objects that are displayed still exist when displayed!
    bool areParamObjectsAllValid();

    SidebarSelectorButton consoleButton = SidebarSelectorButton(Icons::Console);
    SidebarSelectorButton browserButton = SidebarSelectorButton(Icons::Documentation);
    SidebarSelectorButton automationButton = SidebarSelectorButton(Icons::Parameters);
    SidebarSelectorButton searchButton = SidebarSelectorButton(Icons::Search);

    Rectangle<int> dividerBounds;

    InspectorButton inspectorButton = InspectorButton(Icons::Settings);

    std::unique_ptr<Component> extraSettingsButton;
    SmallIconButton panelPinButton = SmallIconButton(Icons::Pin);

    std::unique_ptr<Console> consolePanel;
    std::unique_ptr<DocumentationBrowser> browserPanel;
    std::unique_ptr<AutomationPanel> automationPanel;
    std::unique_ptr<SearchPanel> searchPanel;

    std::unique_ptr<Inspector> inspector;
    std::unique_ptr<Component> resetInspectorButton;

    StringArray panelNames = { "Console", "Documentation Browser", "Automation Parameters", "Search" };
    int currentPanel = 0;

    struct PanelAndButton {
        Component* panel;
        SidebarSelectorButton& button;
    };

    // FIXME: Use Tim's new TUCE HeapArray here, but it's not compiling in this use case yet.
    Array<PanelAndButton> panelAndButton;

    enum InspectorMode { InspectorOff, InspectorAuto, InspectorOpen };
    int inspectorMode = InspectorMode::InspectorOff;

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    float dividerFactor = 0.5f;
    bool isDraggingDivider = false;
    int dragOffset = 0;

    int lastWidth = 250;
};
