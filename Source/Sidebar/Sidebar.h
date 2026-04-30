/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_animation/juce_animation.h>

#include "LookAndFeel.h"
#include "Components/Buttons.h"
#include "Objects/ObjectParameters.h"
#include "Utility/RateReducer.h"

class Console;
class Inspector;
class DocumentationBrowser;
class AutomationPanel;
class SearchPanel;
class PluginProcessor;
class CommandInput;
class Palettes;
class PluginEditor;
class PlugDataParameter;
class Sidebar;

namespace pd {
class Instance;
}

class SidebarSelectorButton final : public TextButton {
public:
    explicit SidebarSelectorButton(String const& icon)
        : TextButton(icon)
    {
    }

    // Right-click handler for "move left/right" context menu
    std::function<void(MouseEvent const&)> onRightClick;

    void mouseDown(MouseEvent const& e) override
    {
        if (e.mods.isPopupMenu() || e.mods.isRightButtonDown()) {
            if (onRightClick)
                onRightClick(e);
            return;
        }

        if (!buttonEnabled)
            return;

        if (!isRealClickEvent(e))
            return;

        numNotifications = 0;
        hasWarning = false;
        TextButton::mouseDown(e);
    }

    void paint(Graphics& g) override
    {
        bool const active = buttonEnabled && (isMouseOver() || isMouseButtonDown() || getToggleState());

        constexpr auto cornerSize = Corners::defaultCornerRadius;

        auto const backgroundColour = active ? PlugDataColours::toolbarHoverColour : Colours::transparentBlack;
        auto const bounds = getLocalBounds().toFloat().reduced(3.0f, 4.0f);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto const font = Fonts::getIconFont().withHeight(13);
        g.setFont(font);
        g.setColour(buttonEnabled ? PlugDataColours::toolbarTextColour : PlugDataColours::toolbarTextColour.withAlpha(0.35f));

        int const yIndent = jmin<int>(4, proportionOfHeight(0.3f));
        int const fontHeight = roundToInt(font.getHeight() * 0.6f);
        int const leftIndent = jmin<int>(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
        int const rightIndent = jmin<int>(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
        int const textWidth = getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);

        if (numNotifications) {
            auto const notificationBounds = getLocalBounds().removeFromBottom(15).removeFromRight(15).translated(-1, -1);
            auto const bubbleColour = hasWarning ? Colours::orange : PlugDataColours::toolbarActiveColour;
            g.setColour(bubbleColour.withAlpha(0.8f));
            g.fillEllipse(notificationBounds.toFloat());
            g.setFont(Font(FontOptions(numNotifications >= 100 ? 8 : 12)));
            g.setColour(bubbleColour.darker(0.6f).contrasting());
            g.drawText(numNotifications > 99 ? String("99+") : String(numNotifications),
                notificationBounds, Justification::centred);
        }
    }

    bool hasWarning = false;
    bool buttonEnabled = true;
    int numNotifications = 0;
};

class Sidebar final : public Component {

public:
    enum SidePanel {
        ConsolePanel = 0,
        DocPanel,
        ParamPanel,
        PatchSearchPanel,
        PalettePanel,
        InspectorPanel,
        NumSidePanels
    };

    enum class Side { Left,
        Right };

    Sidebar(Side side, PluginProcessor* instance, PluginEditor* parent,
        Console* consolePanel,
        DocumentationBrowser* browserPanel,
        AutomationPanel* automationPanel,
        SearchPanel* searchPanel,
        Palettes* palettePanel,
        Inspector* inspector,
        CommandInput* commandInput);

    ~Sidebar() override;

    Side getSide() const { return side; }

    void addPanel(SidePanel panel);
    void removePanel(SidePanel panel);
    bool hasPanel(SidePanel panel) const;
    bool hasAnyPanel() const { return !selectorButtons.empty(); }

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;

    bool hitTest(int, int) override;

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

    void showPanel(SidePanel panelToShow);
    void showSidebar(bool show);
    void updateSearch(bool resetInspector = false);
    bool isHidden() const;

    void clearConsole();
    void updateConsole(int numMessages, bool newWarning);
    void clearSearchOutliner();

    void updateAutomationParameterValue(PlugDataParameter const* param);
    void updateAutomationParameters();

    void setCommandTarget(String const& text);

    void renderButtonsOnCanvas(NVGcontext* ctx);

    void updateCommandInputVisibility();

    static constexpr int dragbarWidth = 6;

    static String panelIdToString(SidePanel id);
    static SidePanel panelFromString(String const& s, SidePanel fallback = ConsolePanel);
    static String sideToString(Side s);
    static Side sideFromString(String const& s, Side fallback = Side::Right);

private:
    int getCommandInputHeight();
    void updateExtraSettingsButton();
    bool areParamObjectsAllValid();
    void updateSelectorButtonStates();
    bool refreshInspectorVisibility(bool allowManualShow);

    Component* getPanelComponent(SidePanel id) const;
    SidebarSelectorButton* getSelectorButton(SidePanel id) const;

    // Build / rebuild the (id, panel*, button*) list and the title strings.
    void rebuildPanelTable();

    // Show right-click context menu for a panel button. `panel` is the panel the button refers to.
    void showButtonContextMenu(SidebarSelectorButton* button, SidePanel panel);

    Side side;

    PluginEditor* editor;

    // Non-owning pointers to shared panel components (owned by PluginEditor).
    Console* consolePanelPtr = nullptr;
    DocumentationBrowser* browserPanelPtr = nullptr;
    AutomationPanel* automationPanelPtr = nullptr;
    SearchPanel* searchPanelPtr = nullptr;
    Palettes* palettePanelPtr = nullptr;
    Inspector* inspectorPtr = nullptr;
    CommandInput* commandInputPtr = nullptr;

    SmallArray<ObjectParameters, 6> lastParameters;
    SmallArray<SafePointer<Component>> lastObjects;

    // Selector buttons created on demand for whichever panels live on this sidebar.
    UnorderedMap<int, std::unique_ptr<SidebarSelectorButton>> selectorButtons;

    std::unique_ptr<Component> extraSettingsButton;
    std::unique_ptr<Component> resetInspectorButton;

    StringArray panelDisplayNames {
        "Console", "Documentation Browser", "Automation Parameters", "Search", "Palettes", "Inspector"
    };

    struct PanelEntry {
        SidePanel id;
        Component* panel;
        SidebarSelectorButton* button;
    };
    SmallArray<PanelEntry> panelTable;

    SidePanel currentPanel = ConsolePanel;
    bool hasCurrentPanel = false;
    bool inspectorAutoShow = false;
    bool inspectorManuallyShown = false;
    bool inspectorHasParameters = false;

    RateReducer rateReducer = RateReducer(45);

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    int lastWidth = 250;

    float sidebarSelectorOffset = 5.0f;
    float sidebarSelectorTarget = 0.0f;
    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder { }
                            .withDurationMs(320)
                            .withEasing(Easings::createEaseInOut())
                            .withValueChangedCallback([this](float const v) {
                                sidebarSelectorOffset = makeAnimationLimits(sidebarSelectorOffset, sidebarSelectorTarget).lerp(v);
                                resized();
                                repaint();
                            })
                            .build();
};
