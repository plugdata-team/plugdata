/*
 // Copyright (c) 2021-2022 Timothy Schoen.
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
#include "Sidebar.h"
#include "Console.h"
#include "Inspector.h"
#include "DocumentationBrowser.h"
#include "AutomationPanel.h"
#include "SearchPanel.h"

Sidebar::Sidebar(PluginProcessor* instance, PluginEditor* parent)
    : pd(instance)
    , editor(parent)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    consolePanel = std::make_unique<Console>(pd);
    browserPanel = std::make_unique<DocumentationBrowser>(pd);
    automationPanel = std::make_unique<AutomationPanel>(pd);
    searchPanel = std::make_unique<SearchPanel>(parent);

    inspector = std::make_unique<Inspector>();

    addAndMakeVisible(consolePanel.get());
    addChildComponent(browserPanel.get());
    addChildComponent(automationPanel.get());
    addChildComponent(searchPanel.get());

    addChildComponent(inspector.get());

    browserPanel->addMouseListener(this, true);
    consolePanel->addMouseListener(this, true);
    automationPanel->addMouseListener(this, true);
    inspector->addMouseListener(this, true);
    searchPanel->addMouseListener(this, true);

    consoleButton.setTooltip("Open console panel");
    consoleButton.setConnectedEdges(12);
    consoleButton.setClickingTogglesState(true);
    consoleButton.onClick = [this]() {
        showPanel(SidePanel::ConsolePan);
    };

    browserButton.setTooltip("Open documentation browser");
    browserButton.setConnectedEdges(12);
    browserButton.onClick = [this]() {
        showPanel(SidePanel::DocPan);
    };
    browserButton.setClickingTogglesState(true);
    addAndMakeVisible(browserButton);

    automationButton.setTooltip("Open automation panel");
    automationButton.setConnectedEdges(12);
    automationButton.setClickingTogglesState(true);
    automationButton.onClick = [this]() {
        showPanel(SidePanel::ParamPan);
    };
    addAndMakeVisible(automationButton);

    searchButton.setTooltip("Open search panel");
    searchButton.setConnectedEdges(12);
    searchButton.setClickingTogglesState(true);
    searchButton.onClick = [this]() {
        showPanel(SidePanel::SearchPan);
    };
    addAndMakeVisible(searchButton);

    consoleButton.setToggleState(true, dontSendNotification);

    addAndMakeVisible(consoleButton);

    inspectorButton.onClick = [this]() {
        showPanel(SidePanel::InspectorPan);
    };

    addAndMakeVisible(inspectorButton);

    panelAndButton = {  PanelAndButton{consolePanel.get(), consoleButton},
                        PanelAndButton{browserPanel.get(), browserButton},
                        PanelAndButton{automationPanel.get(), automationButton},
                        PanelAndButton{searchPanel.get(), searchButton} };

    inspector->setVisible(false);
    currentPanel = SidePanel::ConsolePan;
    updateExtraSettingsButton();

    resized();
}

Sidebar::~Sidebar()
{
    browserPanel->removeMouseListener(this);
}

void Sidebar::paint(Graphics& g)
{
    if (!sidebarHidden) {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(0, 30, getWidth(), getHeight());
        auto panelName = panelNames[currentPanel];
        if (inspectorButton.isInspectorAuto() && inspector->isVisible())
            panelName = "Inspector: " + inspector->getTitle();
        Fonts::drawStyledText(g, panelName, Rectangle<int>(0, 0, getWidth() - 30, 30), findColour(PlugDataColour::toolbarTextColourId), Bold, 15, Justification::centred);

        if (inspector->isVisible() && inspectorButton.isInspectorPinned()) {
            auto inpectorPos = Point<int>(0, dividerFactor * (getHeight()));
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRect(inpectorPos.x, inpectorPos.y, getWidth() - 30, 30);
            auto inspectorTitle = inspector->getTitle();
            if (lastParameters.empty())
                inspectorTitle = "empty";
            Fonts::drawStyledText(g, "Inspector: " + inspectorTitle, Rectangle<int>(inpectorPos.x, inpectorPos.y + 5, getWidth() - 30, 20), findColour(PlugDataColour::toolbarTextColourId), Bold, 15, Justification::centred);
        }
    }
}

void Sidebar::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0.5f, 30, 0.5f, getHeight() + 0.5f);

    g.drawLine(dividerBounds.getX() + 4, dividerBounds.getCentreY(), dividerBounds.getRight() - 4, dividerBounds.getCentreY());

    if (!sidebarHidden) {
        g.drawLine(0, 30, getWidth(), 30);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
        g.drawLine(getWidth() - 30, 30, getWidth() - 30, getHeight() + 0.5f);
    }
}

void Sidebar::settingsChanged(String const& name, var const& value)
{
    if (name == "centre_sidepanel_buttons") {
        resized();
    }
}

void Sidebar::resized()
{
    updateGeometry();
}

void Sidebar::updateGeometry() {
    auto bounds = getLocalBounds();

    if (bounds.getWidth() == 0)
        return;

    auto buttonBarBounds = bounds.removeFromRight(30).reduced(0, 1);

    if (SettingsFile::getInstance()->getProperty<bool>("centre_sidepanel_buttons")) {
        buttonBarBounds = buttonBarBounds.withSizeKeepingCentre(30, 144 + 30 + 8 + 30);
    }

    consoleButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    browserButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    automationButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    searchButton.setBounds(buttonBarBounds.removeFromTop(30));

    dividerBounds = buttonBarBounds.removeFromTop(20);

    inspectorButton.setBounds(buttonBarBounds.removeFromTop(30));

    auto panelTitleBarBounds = bounds.removeFromTop(30).withTrimmedRight(-30).removeFromLeft(30);

    if (extraSettingsButton){
        extraSettingsButton->setBounds(panelTitleBarBounds);
    }

    auto dividerPos = getHeight() * (1.0f - dividerFactor);

    // We need to give the inspector bounds to start with - even if it's not visible
    inspector->setBounds(bounds);

    if (inspector->isVisible()) {
        if (inspectorButton.isInspectorAuto()) {
            inspector->setBounds(bounds);
            std::cout << "tbb: " << panelTitleBarBounds.toString() << std::endl;
            if (extraSettingsButton)
                extraSettingsButton->setVisible(false);
            if (resetInspectorButton) {
                resetInspectorButton->setBounds(panelTitleBarBounds);
                resetInspectorButton->setVisible(true);
            }
        } else {
            auto bottomB = bounds.removeFromBottom(dividerPos);
            auto resetB = bottomB.removeFromTop(30);
            inspector->setBounds(bottomB);
            auto resetBounds = resetB.removeFromLeft(30);
            if (extraSettingsButton)
                extraSettingsButton->setVisible(true);
            if (resetInspectorButton) {
                resetInspectorButton->setBounds(resetBounds);
                resetInspectorButton->setVisible(true);
            }
        }
    } else {
        if (resetInspectorButton)
            resetInspectorButton->setVisible(false);
    }

    browserPanel->setBounds(bounds);
    consolePanel->setBounds(bounds);
    automationPanel->setBounds(bounds);
    searchPanel->setBounds(bounds);
}

void Sidebar::mouseDown(MouseEvent const& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    Rectangle<int> dragBar(0, 0, dragbarWidth, getHeight() - 30);

    if (dragBar.contains(e.getEventRelativeTo(this).getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = getWidth();
    } else {
        draggingSidebar = false;
    }

    dragOffset = static_cast<float>(e.getEventRelativeTo(this).y) - (dividerFactor * getHeight());
}

void Sidebar::mouseDrag(MouseEvent const& e)
{
    if (draggingSidebar) {
        int newWidth = dragStartWidth - e.getDistanceFromDragStartX();
        newWidth = std::clamp(newWidth, 230, std::max(getParentWidth() / 2, 150));

        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    } else if (isDraggingDivider) {
        auto newDividerY = static_cast<float>(jlimit(30, getHeight() - 30, e.getEventRelativeTo(this).getPosition().y - dragOffset));
        dividerFactor = newDividerY / getHeight();
        resized();
        repaint();
    }
}

void Sidebar::mouseUp(MouseEvent const& e)
{
    draggingSidebar = false;
    isDraggingDivider = false;
}

void Sidebar::mouseMove(MouseEvent const& e)
{
    bool resizeCursor = e.getEventRelativeTo(this).getPosition().getX() < dragbarWidth && e.getEventRelativeTo(this).getPosition().getY() < getHeight() - 30;

    auto pos = e.getEventRelativeTo(this).getPosition();
    bool resizeVertical = pos.x > 5 && pos.y > dividerFactor * (getHeight()) + 3 && pos.y < dividerFactor * getHeight() + 30 - 6;

    isDraggingDivider = false;

    if (resizeCursor)
        e.originalComponent->setMouseCursor(MouseCursor::LeftRightResizeCursor);
    else if (inspectorButton.isInspectorPinned() && resizeVertical) {
        isDraggingDivider = true;
        e.originalComponent->setMouseCursor(MouseCursor::UpDownResizeCursor);
    }
    else
        e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::mouseExit(MouseEvent const& e)
{
    e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showPanel(SidePanel panelToShow)
{
    if (panelToShow == currentPanel && !sidebarHidden) {
        for (auto panel : panelAndButton) {
            panel.button.setToggleState(false, dontSendNotification);
        }

        showSidebar(false);
        return;
    }

    if (panelToShow != SidePanel::InspectorPan)
        showSidebar(true);

    // Set one of the panels to active, and the rest to inactive
    auto setPanelVis = [this](Component* panel, SidePanel panelEnum) {
        for (auto pb : panelAndButton) {
            if (pb.panel == panel) {
                pb.panel->setVisible(true);
                pb.panel->setInterceptsMouseClicks(true, true);
                pb.panel->resized();
                pb.button.setToggleState(true, dontSendNotification);
                currentPanel = panelEnum;
            }
            else {
                pb.panel->setVisible(false);
                pb.panel->setInterceptsMouseClicks(false, false);
                pb.button.setToggleState(false, dontSendNotification);
            }
            if (inspector->isVisible() && !inspectorButton.isInspectorActive())
                inspector->setVisible(false);
        }
    };

    switch(panelToShow){
        case SidePanel::ConsolePan:
            setPanelVis(consolePanel.get(), SidePanel::ConsolePan);
            break;
        case SidePanel::DocPan:
            setPanelVis(browserPanel.get(), SidePanel::DocPan);
            break;
        case SidePanel::ParamPan:
            setPanelVis(automationPanel.get(), SidePanel::ParamPan);
            break;
        case SidePanel::SearchPan:
            setPanelVis(searchPanel.get(), SidePanel::SearchPan);
            searchPanel->grabFocus();
            break;
        case SidePanel::InspectorPan:
            if (!sidebarHidden) {
                auto isVisible = inspectorButton.isInspectorPinned() || (inspectorButton.isInspectorAuto() && lastParameters.not_empty());
                if (!areParamObjectsAllValid()) {
                    inspector->setInspectorEmpty();
                }
                inspector->setVisible(isVisible);
            }
            break;
        default:
            break;
    }

    updateExtraSettingsButton();

    updateGeometry();

    repaint();
}

bool Sidebar::areParamObjectsAllValid()
{
    for (const auto& obj : lastObjects) {
        if (!obj)
            return false;
    }
    return true;
}

bool Sidebar::isShowingBrowser()
{
    return browserPanel->isVisible();
}

void Sidebar::updateAutomationParameterValue(PlugDataParameter* param)
{
    if (ProjectInfo::isStandalone && automationPanel) {
        automationPanel->updateParameterValue(param);
    }
}

void Sidebar::updateAutomationParameters()
{
    if (automationPanel) {
        automationPanel->triggerAsyncUpdate();
    }
}

void Sidebar::showSidebar(bool show)
{
    sidebarHidden = !show;

    if (!show) {
        lastWidth = getWidth();
        int newWidth = 30;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        if (extraSettingsButton)
            extraSettingsButton->setVisible(false);
    } else {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());

        if (inspector->isVisible()) {
            inspector->showParameters();
        }
        if (extraSettingsButton)
            extraSettingsButton->setVisible(true);
    }

    editor->resized();
}

bool Sidebar::isHidden() const
{
    return sidebarHidden;
}

void Sidebar::forceShowParameters(Array<Component*> objects, SmallArray<ObjectParameters, 6>& params)
{
    if (!inspectorButton.isInspectorActive()) {
        inspectorButton.setAuto();
    }
    showParameters(objects, params, true);
}

void Sidebar::showParameters(Array<Component*> objects, SmallArray<ObjectParameters, 6>& params, bool showOnSelect)
{
    lastObjects.clear();
    for (auto obj : objects)
        lastObjects.add(obj);

    lastParameters = params;
    auto activeParams = inspector->loadParameters(params);

    auto name = String("empty");

    if (objects.size() == 1) {
        auto obj = dynamic_cast<Object*>(objects.getFirst());
        name = dynamic_cast<Canvas*>(objects.getFirst()) ? "canvas" : obj ? obj->getType(false) : "";
    } else if (objects.size() > 1){
        name = "(" + String(objects.size()) + " selected)";
    }
    inspector->setTitle(name);

    bool isVis = (inspectorButton.isInspectorAuto() && params.not_empty() && showOnSelect && activeParams) || inspectorButton.isInspectorPinned();

    // Reset console notifications if the inspector is not visible and console is
    if (!isVis && currentPanel == SidePanel::ConsolePan) {
        consoleButton.numNotifications = 0;
        consoleButton.repaint();
    }

    inspector->setVisible(isVis);

    inspectorButton.showIndicator(!isVis && showOnSelect && params.not_empty() && activeParams);

    updateExtraSettingsButton();

    updateGeometry();

    repaint();
}

void Sidebar::updateSearch(bool resetInspector)
{
    searchPanel->updateResults();
    if (!areParamObjectsAllValid() || resetInspector) {
        inspector->setInspectorEmpty();
    }
}

void Sidebar::updateExtraSettingsButton()
{
    if (inspector->isVisible()) {
        resetInspectorButton = inspector->getExtraSettingsComponent();
        extraSettingsButton.reset(nullptr);
    } else
        resetInspectorButton.reset(nullptr);

    if (consolePanel->isVisible()) {
        extraSettingsButton = consolePanel->getExtraSettingsComponent();
    } else if (browserPanel->isVisible()) {
        extraSettingsButton = browserPanel->getExtraSettingsComponent();
    } else if (searchPanel->isVisible()) {
        extraSettingsButton = searchPanel->getExtraSettingsComponent();
    } else {
        extraSettingsButton.reset(nullptr);
        return;
    }

    if (extraSettingsButton) {
        addChildComponent(extraSettingsButton.get());
        extraSettingsButton->setVisible(!isHidden() && !(inspectorButton.isInspectorAuto() && inspector->isVisible()));
    }
    if (resetInspectorButton) {
        addChildComponent(resetInspectorButton.get());
        resetInspectorButton->setVisible(!isHidden());
    }
}

void Sidebar::hideParameters()
{
    if (inspectorButton.isInspectorAuto()) {
        inspector->setVisible(false);
    }

    consolePanel->deselect();
    updateExtraSettingsButton();

    updateGeometry();

    repaint();
}

void Sidebar::clearConsole()
{
    consolePanel->clear();
}

void Sidebar::updateConsole(int numMessages, bool newWarning)
{
    if (currentPanel != 0 || sidebarHidden || (inspector->isVisible() && inspectorButton.isInspectorAuto())) {
        consoleButton.numNotifications += numMessages;
        consoleButton.hasWarning = consoleButton.hasWarning || newWarning;
        consoleButton.repaint();
    } else {
        consoleButton.numNotifications = 0;
        consoleButton.repaint();
    }

    consolePanel->update();
}

void Sidebar::clearSearchOutliner()
{
    searchPanel->clear();
}
