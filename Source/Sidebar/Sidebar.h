/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "LookAndFeel.h"
#include "Components/Buttons.h"
#include "Objects/ObjectParameters.h"
#include "Utility/SettingsFile.h"
#include "Utility/RateReducer.h"

class Console;
class Inspector;
class DocumentationBrowser;
class AutomationPanel;
class SearchPanel;
class PluginProcessor;
class CommandInput;

namespace pd {
class Instance;
}

class InspectorButton final : public Component
    , public SettableTooltipClient {

public:
    std::function<void()> onClick = [] { };

    explicit InspectorButton(String const& icon)
        : icon(icon)
    {
        updateTooltip();
    }

    void showIndicator(bool const toShow)
    {
        if (showingIndicator != toShow) {
            showingIndicator = toShow;
            repaint();
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        incrementState();

        onClick();
    }

    bool hitTest(int const x, int const y) override
    {
        if (getLocalBounds().reduced(3, 4).contains(Point<int>(x, y)))
            return true;

        return false;
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

    void updateTooltip()
    {
        switch (state) {
        case InspectorOff:
            setTooltip("Inspector hidden, click to auto show");
            break;
        case InspectorAuto:
            setTooltip("Inspector auto, click to pin");
            break;
        case InspectorPin:
            setTooltip("Inspector pinned, click to hide");
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

    void setAuto()
    {
        state = InspectorState::InspectorAuto;
        updateTooltip();
        repaint();
    }

    bool isInspectorActive() const
    {
        return state >= 1;
    }

    bool isInspectorAuto() const
    {
        return state == InspectorState::InspectorAuto;
    }

    bool isInspectorPinned() const
    {
        return state == InspectorState::InspectorPin;
    }

    void paint(Graphics& g) override
    {
        auto const selCol = findColour(PlugDataColour::objectSelectedOutlineColourId);

        bool const active = isHovering || state == InspectorPin;
        bool const stateAuto = state == InspectorAuto;

        constexpr auto cornerSize = Corners::defaultCornerRadius;

        auto const backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto const bounds = getLocalBounds().toFloat().reduced(3.0f, 4.0f);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto const font = Fonts::getIconFont().withHeight(13);
        g.setFont(font);
        g.setColour(stateAuto ? selCol : findColour(PlugDataColour::toolbarTextColourId));

        int const yIndent = jmin<int>(4, proportionOfHeight(0.3f));
        int const textWidth = getWidth() - 4;

        if (textWidth > 0)
            g.drawFittedText(icon, 2, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);

        if (state == InspectorOff) {
            auto const b = getLocalBounds().toFloat().reduced(10.5f).translated(-0.5f, 0.5f);
            Path strikeThrough;
            strikeThrough.startNewSubPath(b.getBottomRight());
            strikeThrough.lineTo(b.getTopLeft());
            auto const front = strikeThrough;

            // back stroke
            auto const bgCol = findColour(PlugDataColour::panelBackgroundColourId);
            g.setColour(active ? bgCol.overlaidWith(backgroundColour) : bgCol);
            PathStrokeType strokeType(3.0f, PathStrokeType::JointStyle::mitered, PathStrokeType::EndCapStyle::rounded);
            strikeThrough.applyTransform(AffineTransform::translation(-0.7f, -0.7f));
            g.strokePath(strikeThrough, strokeType);

            // front stroke
            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
            strokeType.setStrokeThickness(1.5f);
            g.strokePath(front, strokeType);
        }
        if (showingIndicator) {
            g.setColour(selCol);
            g.fillEllipse(Rectangle<float>(0, 0, 5, 5).withCentre(getLocalBounds().reduced(8).toFloat().getBottomRight()));
        }
    }

private:
    enum InspectorState { InspectorOff,
        InspectorAuto,
        InspectorPin };
    int state = InspectorAuto;
    bool showingIndicator = false;
    String icon;
    bool isHovering = false;
};

class SidebarSelectorButton final : public TextButton {
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
        bool const active = isMouseOver() || isMouseButtonDown() || getToggleState();

        constexpr auto cornerSize = Corners::defaultCornerRadius;

        auto const backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto const bounds = getLocalBounds().toFloat().reduced(3.0f, 4.0f);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto const font = Fonts::getIconFont().withHeight(13);
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
            auto const notificationBounds = getLocalBounds().removeFromBottom(15).removeFromRight(15).translated(-1, -1);
            auto const bubbleColour = hasWarning ? Colours::orange : findColour(PlugDataColour::toolbarActiveColourId);
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
class Sidebar final : public Component
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

    void forceShowParameters(SmallArray<Component*>& objects, SmallArray<ObjectParameters, 6>& params);
    void showParameters(SmallArray<Component*>& objects, SmallArray<ObjectParameters, 6>& params, bool showOnSelect = false);
    void hideParameters();

    void setActiveSearchItem(void const* objPtr);

    void clearInspector();

    bool isShowingBrowser() const;
    bool isShowingSearch() const;

    void settingsChanged(String const& name, var const& value) override;

    enum SidePanel { ConsolePan,
        DocPan,
        ParamPan,
        SearchPan,
        InspectorPan };

    void showPanel(SidePanel panelToShow);

    void showSidebar(bool show);

    void updateSearch(bool resetInspector = false);

    bool isHidden() const;

    void clearConsole();
    void updateConsole(int numMessages, bool newWarning);

    void clearSearchOutliner();

    void updateAutomationParameterValue(PlugDataParameter const* param);
    void updateAutomationParameters();

    static constexpr int dragbarWidth = 6;

private:
    void updateExtraSettingsButton();

    PluginProcessor* pd;
    PluginEditor* editor;
    SmallArray<ObjectParameters, 6> lastParameters;
    SmallArray<SafePointer<Component>> lastObjects;

    // Make sure that objects that are displayed still exist when displayed!
    bool areParamObjectsAllValid();

    SidebarSelectorButton consoleButton = SidebarSelectorButton(Icons::Console);
    SidebarSelectorButton browserButton = SidebarSelectorButton(Icons::Documentation);
    SidebarSelectorButton automationButton = SidebarSelectorButton(Icons::Parameters);
    SidebarSelectorButton searchButton = SidebarSelectorButton(Icons::Search);

    Rectangle<int> dividerBounds;

    InspectorButton inspectorButton = InspectorButton(Icons::Wrench);

    std::unique_ptr<Component> extraSettingsButton;

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

    SmallArray<PanelAndButton> panelAndButton;

    RateReducer rateReducer = RateReducer(45);

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    float dividerFactor = 0.5f;
    bool isDraggingDivider = false;
    int dragOffset = 0;

    int lastWidth = 250;
};
