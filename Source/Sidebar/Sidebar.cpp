/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Pd/Instance.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"

#include "Components/SearchEditor.h"
#include "Components/WelcomePanel.h"
#include "Utility/NVGGraphicsContext.h"
#include "Sidebar.h"
#include "Console.h"
#include "Inspector.h"
#include "CommandInput.h"
#include "DocumentationBrowser.h"
#include "AutomationPanel.h"
#include "SearchPanel.h"
#include "Palettes.h"

String Sidebar::panelIdToString(SidePanel id)
{
    switch (id) {
    case ConsolePanel:
        return "console";
    case DocPanel:
        return "doc";
    case ParamPanel:
        return "param";
    case PatchSearchPanel:
        return "search";
    case PalettePanel:
        return "palette";
    case InspectorPanel:
        return "inspector";
    default:
        return "console";
    }
}

Sidebar::SidePanel Sidebar::panelFromString(String const& s, SidePanel fallback)
{
    if (s == "console")
        return ConsolePanel;
    if (s == "doc")
        return DocPanel;
    if (s == "param")
        return ParamPanel;
    if (s == "search")
        return PatchSearchPanel;
    if (s == "palette")
        return PalettePanel;
    if (s == "inspector")
        return InspectorPanel;
    return fallback;
}

String Sidebar::sideToString(Side s)
{
    return s == Side::Left ? "left" : "right";
}

Sidebar::Side Sidebar::sideFromString(String const& s, Side fallback)
{
    if (s == "left")
        return Side::Left;
    if (s == "right")
        return Side::Right;
    return fallback;
}

Sidebar::Sidebar(Side sideIn, PluginProcessor* instance, PluginEditor* parent,
    Console* console,
    DocumentationBrowser* browser,
    AutomationPanel* automation,
    SearchPanel* search,
    Palettes* palette,
    Inspector* insp,
    CommandInput* cmdInput)
    : side(sideIn)
    , editor(parent)
    , consolePanelPtr(console)
    , browserPanelPtr(browser)
    , automationPanelPtr(automation)
    , searchPanelPtr(search)
    , palettePanelPtr(palette)
    , inspectorPtr(insp)
    , commandInputPtr(cmdInput)
{
    updater.addAnimator(animator);
    rebuildPanelTable();
    resized();
}

Sidebar::~Sidebar()
{
    // Make sure shared panels lose us as parent, in case PluginEditor outlives us.
    for (auto const& entry : panelTable) {
        if (entry.panel && entry.panel->getParentComponent() == this)
            removeChildComponent(entry.panel);
    }
    if (inspectorPtr && inspectorPtr->getParentComponent() == this)
        removeChildComponent(inspectorPtr);
    if (commandInputPtr && commandInputPtr->getParentComponent() == this)
        removeChildComponent(commandInputPtr);
}

void Sidebar::addPanel(SidePanel panel)
{
    if (hasPanel(panel))
        return;

    auto* panelComp = getPanelComponent(panel);
    if (!panelComp)
        return;

    // Reparent the panel to this sidebar
    if (panelComp->getParentComponent() && panelComp->getParentComponent() != this)
        panelComp->getParentComponent()->removeChildComponent(panelComp);
    addChildComponent(panelComp);
    panelComp->addMouseListener(this, true);

    // Pick the appropriate icon
    String icon;
    switch (panel) {
    case ConsolePanel:
        icon = Icons::Console;
        break;
    case DocPanel:
        icon = Icons::Documentation;
        break;
    case ParamPanel:
        icon = Icons::Parameters;
        break;
    case PatchSearchPanel:
        icon = Icons::Search;
        break;
    case PalettePanel:
        icon = Icons::Palette;
        break;
    default:
        break;
    }

    auto button = std::make_unique<SidebarSelectorButton>(icon);
    button->setConnectedEdges(12);
    button->setClickingTogglesState(true);
    button->setTooltip("Open " + panelDisplayNames[panel].toLowerCase() + " panel");
    button->onClick = [this, panel] { showPanel(panel); };
    button->onRightClick = [this, panel, b = button.get()](MouseEvent const&) { showButtonContextMenu(b, panel); };

    addAndMakeVisible(button.get());
    selectorButtons[panel] = std::move(button);

    if (panel == ConsolePanel) {
        if (inspectorPtr) {
            // Reparent inspector to this sidebar
            if (inspectorPtr->getParentComponent() && inspectorPtr->getParentComponent() != this)
                inspectorPtr->getParentComponent()->removeChildComponent(inspectorPtr);
            addChildComponent(inspectorPtr);
            inspectorPtr->addMouseListener(this, true);
            inspectorPtr->setAlwaysOnTop(true);
        }
    }

    rebuildPanelTable();

    // If this is the first panel on this sidebar, make it active.
    if (!hasCurrentPanel && panel != InspectorPanel) {
        currentPanel = panel;
        hasCurrentPanel = true;
        if (auto* btn = getSelectorButton(panel))
            btn->setToggleState(true, dontSendNotification);
        if (auto* p = getPanelComponent(panel))
            p->setVisible(!sidebarHidden);
    }

    updateExtraSettingsButton();
    resized();
    repaint();
}

void Sidebar::removePanel(SidePanel panel)
{
    if (!hasPanel(panel))
        return;

    if (panel == InspectorPanel) {
        if (inspectorPtr && inspectorPtr->getParentComponent() == this) {
            inspectorPtr->removeMouseListener(this);
            removeChildComponent(inspectorPtr);
        }
    } else {
        auto* panelComp = getPanelComponent(panel);
        if (panelComp && panelComp->getParentComponent() == this) {
            panelComp->removeMouseListener(this);
            removeChildComponent(panelComp);
        }
        selectorButtons.erase(panel);

        // If we just removed the currently-displayed panel, fall back to the first available.
        if (hasCurrentPanel && currentPanel == panel) {
            hasCurrentPanel = false;
            if (!selectorButtons.empty()) {
                // Pick the first remaining panel by enum order
                for (int i = 0; i < NumSidePanels; ++i) {
                    if (selectorButtons.count(i)) {
                        currentPanel = static_cast<SidePanel>(i);
                        hasCurrentPanel = true;
                        if (auto* btn = getSelectorButton(currentPanel))
                            btn->setToggleState(true, dontSendNotification);
                        if (auto* p = getPanelComponent(currentPanel))
                            p->setVisible(!sidebarHidden);
                        break;
                    }
                }
            }
        }
    }

    rebuildPanelTable();
    updateExtraSettingsButton();
    resized();
    repaint();
}

bool Sidebar::hasPanel(SidePanel panel) const
{
    if(panel == InspectorPanel)
        panel = ConsolePanel; // for now, these share a selector button

    return selectorButtons.count(panel) > 0;
}

void Sidebar::rebuildPanelTable()
{
    panelTable.clear();
    for (int i = 0; i < InspectorPanel; ++i) {
        auto id = static_cast<SidePanel>(i);
        if (selectorButtons.count(i)) {
            panelTable.add({ id, getPanelComponent(id), selectorButtons[i].get() });
        }
    }
}

void Sidebar::showButtonContextMenu(SidebarSelectorButton* button, SidePanel panel)
{
    PopupMenu menu;
    auto const otherSide = side == Side::Left ? Side::Right : Side::Left;
    auto const moveLabel = String("Move to ") + (otherSide == Side::Left ? "left" : "right") + " sidebar";
    menu.addItem(1, moveLabel);

    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(button),
        [this, panel, otherSide](int const result) {
            if (result == 1 && editor) {
                editor->movePanelToSide(panel, otherSide);
            }
        });
}

void Sidebar::paint(Graphics& g)
{
    if (sidebarHidden || !hasAnyPanel())
        return;

    g.setColour(PlugDataColours::sidebarBackgroundColour);
    g.fillRect(0, 30, getWidth(), getHeight() - 12);
    g.fillRoundedRectangle(0.0f, 30.0f, getWidth(), getHeight() - 30.0f, Corners::windowCornerRadius);

    String panelName = hasCurrentPanel && currentPanel < panelDisplayNames.size()
        ? panelDisplayNames[currentPanel]
        : String();

    if (inspectorPtr && inspectorPtr->isVisible())
        panelName = "Inspector: " + inspectorPtr->getTitle();

    Fonts::drawStyledText(g, panelName,
        Rectangle<int>(0, 0, getWidth(), 30),
        PlugDataColours::toolbarTextColour, Bold, 15, Justification::centred);

    /*
    if (inspectorPtr) {
        auto inspectorPos = Point<int>(0, getHeight() - getCommandInputHeight());
        g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
        g.fillRect(inspectorPos.x, inspectorPos.y, getWidth(), 30);
        auto title = inspectorPtr->getTitle();
        if (lastParameters.empty())
            title = "empty";
        Fonts::drawStyledText(g, "Inspector: " + title,
            Rectangle<int>(inspectorPos.x, inspectorPos.y + 5, getWidth(), 20),
            PlugDataColours::toolbarTextColour, Bold, 15, Justification::centred);
    } */
}

void Sidebar::paintOverChildren(Graphics& g)
{
    if (!hasAnyPanel())
        return;

    g.setColour(PlugDataColours::toolbarOutlineColour);

    if(side == Side::Left) {
        g.drawLine(0.5f, 30, 0.5f, getHeight() + 0.5f);
    }
    else {
        g.drawLine(getWidth() - 0.5f, 30, getWidth() - 0.5f, getHeight() + 0.5f);
    }
    g.drawLine(0, 30, getWidth(), 30);
}

int Sidebar::getCommandInputHeight()
{
    return commandInputPtr && commandInputPtr->isVisible() && commandInputPtr->getParentComponent() == this
        ? commandInputPtr->getHeight() + 16
        : 0;
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    if (bounds.getWidth() == 0)
        return;

    auto buttonBarBounds = (side == Side::Right)
        ? bounds.removeFromRight(42).reduced(0, 1).translated(-6, 0)
        : bounds.removeFromLeft(42).reduced(0, 1).translated(6, 0);

    buttonBarBounds.translate(side == Side::Right ? -sidebarSelectorOffset : sidebarSelectorOffset, 0);

    int totalH = 0;
    for (auto const& _ : panelTable)
        totalH += 30 + 8;

    buttonBarBounds = buttonBarBounds.withSizeKeepingCentre(30, totalH);

    for (auto const& entry : panelTable) {
        if (entry.button) {
            entry.button->setBounds(buttonBarBounds.removeFromTop(30));
            buttonBarBounds.removeFromTop(8);
        }
    }

    auto extraButtonBounds = side == Side::Right ? Rectangle<int>(0, 0, 30, 30) : Rectangle<int>(getWidth() - 60, 0, 30, 30);

    if (extraSettingsButton)
        extraSettingsButton->setBounds(extraButtonBounds);

    if (commandInputPtr && commandInputPtr->isVisible()
        && commandInputPtr->getParentComponent() == this) {
        commandInputPtr->setBounds(getLocalBounds()
                .removeFromBottom(getCommandInputHeight())
                .reduced(8)
                .withTrimmedLeft(side == Side::Left ? 30 : 0)
                .withTrimmedRight(side == Side::Right ? 30 : 0));
    }

    bounds.removeFromTop(30);

    if (inspectorPtr && inspectorPtr->getParentComponent() == this && inspectorPtr->isVisible())
        inspectorPtr->setBounds(bounds);
    if (resetInspectorButton)
        resetInspectorButton->setVisible(false);


    for (auto const& entry : panelTable) {
        if (entry.panel && entry.panel->getParentComponent() == this)
            entry.panel->setBounds(bounds);
    }
}

bool Sidebar::hitTest(int x, int y)
{
    Rectangle<int> buttonBounds;
    for (auto const& entry : panelTable) {
        if (entry.button && entry.button->isVisible())
            buttonBounds = buttonBounds.getUnion(entry.button->getBounds());
    }
    return !isHidden() || buttonBounds.contains(x, y);
}

void Sidebar::mouseDown(MouseEvent const& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    // Drag bar is on the *inner* edge — the edge that faces the canvas.
    Rectangle<int> dragBar = side == Side::Right
        ? Rectangle<int>(0, 0, dragbarWidth, getHeight() - 30)
        : Rectangle<int>(getWidth() - dragbarWidth, 0, dragbarWidth, getHeight() - 30);

    if (dragBar.contains(e.getEventRelativeTo(this).getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = getWidth();
    } else {
        draggingSidebar = false;
    }
}

void Sidebar::mouseDrag(MouseEvent const& e)
{
    if (draggingSidebar) {
        if (rateReducer.tooFast())
            return;

        // For right sidebar: dragging right shrinks (negative dx grows).
        // For left sidebar: dragging right grows.
        int delta = side == Side::Right ? -e.getDistanceFromDragStartX()
                                        : e.getDistanceFromDragStartX();
        int newWidth = dragStartWidth + delta;
        newWidth = std::clamp(newWidth, 230, std::max(getParentWidth() / 2, 150));

        if (side == Side::Right) {
            setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        } else {
            setBounds(0, getY(), newWidth, getHeight());
        }
        getParentComponent()->resized();
    }
}

void Sidebar::mouseUp(MouseEvent const& /*e*/)
{
    if (draggingSidebar) {
        // Persist new width
        SettingsFile::getInstance()->setProperty(
            side == Side::Right ? "right_sidebar_width" : "left_sidebar_width",
            getWidth());
    }
    draggingSidebar = false;
}

void Sidebar::mouseMove(MouseEvent const& e)
{
    if (sidebarHidden)
        return;

    auto const pos = e.getEventRelativeTo(this).getPosition();

    bool const onDragBar = side == Side::Right
        ? (pos.getX() < dragbarWidth)
        : (pos.getX() > getWidth() - dragbarWidth);
    bool const resizeCursor = onDragBar && pos.getY() < getHeight() - 30;

    if (resizeCursor)
        e.originalComponent->setMouseCursor(MouseCursor::LeftRightResizeCursor);
    else {
        e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
    }
}

void Sidebar::mouseExit(MouseEvent const& e)
{
    e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showPanel(SidePanel const panelToShow)
{
    // Toggle off if clicking the already-active panel
    if (hasCurrentPanel && panelToShow == currentPanel
        && !sidebarHidden && !editor->welcomePanel->isVisible()
        && panelToShow != InspectorPanel) {
        for (auto const& entry : panelTable)
            if (entry.button)
                entry.button->setToggleState(false, dontSendNotification);

        showSidebar(false);
        return;
    }

    if (panelToShow != InspectorPanel)
        showSidebar(true);

    auto setPanelVis = [this](Component const* panel, SidePanel const panelEnum) {
        for (auto const& entry : panelTable) {
            if (entry.panel == panel) {
                entry.panel->setVisible(true);
                entry.panel->setInterceptsMouseClicks(true, true);
                entry.panel->resized();
                if (entry.button)
                    entry.button->setToggleState(true, dontSendNotification);
                currentPanel = panelEnum;
                hasCurrentPanel = true;
            } else {
                if (entry.panel) {
                    entry.panel->setVisible(false);
                    entry.panel->setInterceptsMouseClicks(false, false);
                }
                if (entry.button)
                    entry.button->setToggleState(false, dontSendNotification);
            }
            if (inspectorPtr && inspectorPtr->isVisible()) {
                inspectorPtr->setVisible(false);
            }
        }
    };

    // CommandInput is shown for Console or Inspector. It belongs to whichever
    // sidebar is currently displaying that panel.
    auto const wantsCmd = (panelToShow == ConsolePanel || panelToShow == InspectorPanel);
    if (commandInputPtr) {
        if (wantsCmd && hasPanel(panelToShow)) {
            if (commandInputPtr->getParentComponent() != this) {
                if (commandInputPtr->getParentComponent())
                    commandInputPtr->getParentComponent()->removeChildComponent(commandInputPtr);
                addAndMakeVisible(commandInputPtr);
            } else {
                commandInputPtr->setVisible(true);
            }
        } else if (commandInputPtr->getParentComponent() == this) {
            commandInputPtr->setVisible(false);
        }
    }

    switch (panelToShow) {
    case ConsolePanel:
        if (hasPanel(ConsolePanel))
            setPanelVis(consolePanelPtr, ConsolePanel);
        break;
    case DocPanel:
        if (hasPanel(DocPanel)) {
            setPanelVis(browserPanelPtr, DocPanel);
            if (browserPanelPtr)
                browserPanelPtr->grabKeyboardFocus();
        }
        break;
    case ParamPanel:
        if (hasPanel(ParamPanel))
            setPanelVis(automationPanelPtr, ParamPanel);
        break;
    case PatchSearchPanel:
        if (hasPanel(PatchSearchPanel)) {
            setPanelVis(searchPanelPtr, PatchSearchPanel);
            if (searchPanelPtr)
                searchPanelPtr->grabFocus();
        }
        break;
    case PalettePanel:
        if (hasPanel(PalettePanel))
            setPanelVis(palettePanelPtr, PalettePanel);
        break;
    case InspectorPanel:
        if (hasPanel(InspectorPanel) && !sidebarHidden && inspectorPtr) {
            auto const isVisible = !inspectorPtr->isEmpty();
            if (!areParamObjectsAllValid())
                clearInspector();
            if (isVisible) {
                inspectorPtr->loadParameters(lastParameters);
            }
            inspectorPtr->setVisible(isVisible);
        }
        break;
    default:
        break;
    }

    updateExtraSettingsButton();
    resized();
    repaint();
}

void Sidebar::clearInspector()
{
    lastParameters.clear();
    if (inspectorPtr) {
        inspectorPtr->loadParameters(lastParameters);
        inspectorPtr->setTitle("empty");
    }
}

bool Sidebar::areParamObjectsAllValid()
{
    for (auto const& obj : lastObjects)
        if (!obj)
            return false;
    return true;
}

bool Sidebar::isShowingBrowser() const
{
    return hasPanel(DocPanel) && browserPanelPtr && browserPanelPtr->isVisible();
}

bool Sidebar::isShowingSearch() const
{
    return hasPanel(PatchSearchPanel) && searchPanelPtr && searchPanelPtr->isVisible();
}

void Sidebar::updateAutomationParameterValue(PlugDataParameter const* param)
{
    if (!hasPanel(ParamPanel))
        return;
    if (ProjectInfo::isStandalone && automationPanelPtr)
        automationPanelPtr->updateParameterValue(param);
}

void Sidebar::updateAutomationParameters()
{
    if (!hasPanel(ParamPanel))
        return;
    if (automationPanelPtr)
        automationPanelPtr->triggerAsyncUpdate();
}

void Sidebar::setCommandTarget(String const& text)
{
    if (commandInputPtr && commandInputPtr->getParentComponent() == this)
        commandInputPtr->setConsoleTargetName(text);
}

void Sidebar::showSidebar(bool const show)
{
    if (!hasAnyPanel())
        return;

    sidebarHidden = !show;
    sidebarSelectorTarget = show ? 0.0f : 5.0f;
    animator.start();

    SettingsFile::getInstance()->setProperty(side == Side::Right ? "right_sidebar_hidden" : "left_sidebar_hidden", !show);

    if (!show) {
        lastWidth = getWidth();
        constexpr int newWidth = 48;
        if (side == Side::Right)
            setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        else
            setBounds(0, getY(), newWidth, getHeight());

        if (extraSettingsButton)
            extraSettingsButton->setVisible(false);
        if (resetInspectorButton)
            resetInspectorButton->setVisible(false);
        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));
    } else {
        setCachedComponentImage(nullptr);
        int const newWidth = lastWidth;
        if (side == Side::Right)
            setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        else
            setBounds(0, getY(), newWidth, getHeight());

        if (inspectorPtr && inspectorPtr->isVisible() && inspectorPtr->getParentComponent() == this)
            inspectorPtr->showParameters();

        if (extraSettingsButton)
            extraSettingsButton->setVisible(true);
    }

    editor->resized();
}

bool Sidebar::isHidden() const
{
    return sidebarHidden;
}

void Sidebar::forceShowParameters(SmallArray<Component*>& objects, SmallArray<ObjectParameters, 6>& params)
{
    showParameters(objects, params, true);
}

void Sidebar::showParameters(SmallArray<Component*>& objects, SmallArray<ObjectParameters, 6>& params, bool const showOnSelect)
{
    lastObjects.clear();
    for (auto const obj : objects)
        lastObjects.add(obj);
    lastParameters = params;

    auto const activeParams = inspectorPtr ? inspectorPtr->loadParameters(params) : false;

    auto name = String("empty");
    if (objects.size() == 1) {
        auto const obj = dynamic_cast<Object*>(objects[0]);
        name = dynamic_cast<Canvas*>(objects[0]) ? "canvas" : (obj ? obj->getType(false) : "");
    } else if (objects.size() > 1) {
        name = "(" + String(objects.size()) + " selected)";
    }
    if (inspectorPtr)
        inspectorPtr->setTitle(name);

    auto const haveParams = showOnSelect && params.not_empty() && activeParams;
    if (!sidebarHidden && !haveParams && hasCurrentPanel && currentPanel == ConsolePanel) {
        if (auto* btn = getSelectorButton(ConsolePanel)) {
            btn->numNotifications = 0;
            btn->repaint();
        }
    }

    if (inspectorPtr)
        inspectorPtr->setVisible(haveParams);

    updateExtraSettingsButton();
    resized();
    repaint();
}

void Sidebar::hideParameters()
{
    if (inspectorPtr)
        inspectorPtr->setVisible(false);

    if (consolePanelPtr && hasPanel(ConsolePanel))
        consolePanelPtr->deselect();

    updateExtraSettingsButton();
    resized();
    repaint();
}

void Sidebar::renderButtonsOnCanvas(NVGcontext* nvg)
{
    if (!hasAnyPanel())
        return;

    Graphics g(*editor->getNanoLLGC());

    auto totalHeight = 0;
    for (auto const& _ : panelTable) {
        totalHeight += 38;
    }

    auto b = editor->nvgSurface.getLocalArea(this, getLocalBounds()).withSizeKeepingCentre(36, totalHeight).translated(side == Side::Right ? -8 : 8, -4);

    StackShadow::drawShadowForRect(g, b.reduced(3.0f), 10, Corners::largeCornerRadius, 0.4f, 1);

    g.setColour(PlugDataColours::toolbarBackgroundColour);
    g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

    g.setColour(PlugDataColours::toolbarOutlineColour);
    g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);

    auto drawButton = [&](Component* button) {
        auto pos = editor->nvgSurface.getLocalPoint(button, Point<int>(0, 0));
        g.saveState();
        g.addTransform(AffineTransform::translation(pos));
        button->paintEntireComponent(g, true);
        g.restoreState();
    };

    for (auto const& entry : panelTable) {
        if (entry.button)
            drawButton(entry.button);
    }
}

void Sidebar::updateSearch(bool const resetInspector)
{
    if (hasPanel(PatchSearchPanel) && searchPanelPtr)
        searchPanelPtr->updateResults();
    if (!areParamObjectsAllValid() || resetInspector)
        clearInspector();
}

void Sidebar::setActiveSearchItem(void const* objPtr)
{
    if (hasPanel(PatchSearchPanel) && searchPanelPtr)
        searchPanelPtr->patchTree.makeNodeActive(objPtr);
}

void Sidebar::updateExtraSettingsButton()
{
    if (!isHidden() && inspectorPtr) {
        resetInspectorButton = inspectorPtr->getExtraSettingsComponent();
        extraSettingsButton.reset(nullptr);
    } else {
        resetInspectorButton.reset(nullptr);
    }

    if (resetInspectorButton) {
        addChildComponent(resetInspectorButton.get());
        resetInspectorButton->setVisible(!isHidden());
    }

    if (isHidden()) {
        if (resetInspectorButton)
            resetInspectorButton->setVisible(false);
        extraSettingsButton.reset(nullptr);
        return;
    }

    if (hasPanel(ConsolePanel) && consolePanelPtr && consolePanelPtr->isVisible())
        extraSettingsButton = consolePanelPtr->getExtraSettingsComponent();
    else if (hasPanel(DocPanel) && browserPanelPtr && browserPanelPtr->isVisible())
        extraSettingsButton = browserPanelPtr->getExtraSettingsComponent();
    else if (hasPanel(PatchSearchPanel) && searchPanelPtr && searchPanelPtr->isVisible())
        extraSettingsButton = searchPanelPtr->getExtraSettingsComponent();
    else {
        extraSettingsButton.reset(nullptr);
        return;
    }

    if (extraSettingsButton) {
        addChildComponent(extraSettingsButton.get());
        extraSettingsButton->setVisible(!isHidden()
            && inspectorPtr && inspectorPtr->isVisible());
    }
}

void Sidebar::clearConsole()
{
    if (hasPanel(ConsolePanel) && consolePanelPtr)
        consolePanelPtr->clear();
}

void Sidebar::updateConsole(int const numMessages, bool const newWarning)
{
    if (!hasPanel(ConsolePanel))
        return;

    auto* btn = getSelectorButton(ConsolePanel);
    bool const consoleHasFocus = hasCurrentPanel && currentPanel == ConsolePanel;

    if (!consoleHasFocus || sidebarHidden
        || (inspectorPtr && inspectorPtr->isVisible())) {
        if (btn) {
            btn->numNotifications += numMessages;
            btn->hasWarning = btn->hasWarning || newWarning;
            btn->repaint();
        }
    } else if (btn) {
        btn->numNotifications = 0;
        btn->repaint();
    }

    if (consolePanelPtr)
        consolePanelPtr->update();
}

void Sidebar::clearSearchOutliner()
{
    if (hasPanel(PatchSearchPanel) && searchPanelPtr)
        searchPanelPtr->clear();
}

Component* Sidebar::getPanelComponent(SidePanel id) const
{
    switch (id) {
    case ConsolePanel:
        return consolePanelPtr;
    case DocPanel:
        return browserPanelPtr;
    case ParamPanel:
        return automationPanelPtr;
    case PatchSearchPanel:
        return searchPanelPtr;
    case PalettePanel:
        return palettePanelPtr;
    case InspectorPanel:
        return inspectorPtr;
    default:
        return nullptr;
    }
}

SidebarSelectorButton* Sidebar::getSelectorButton(SidePanel id) const
{
    auto it = selectorButtons.find(id);
    return it == selectorButtons.end() ? nullptr : it->second.get();
}
