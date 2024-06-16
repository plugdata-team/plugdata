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
    console = std::make_unique<Console>(pd);
    inspector = std::make_unique<Inspector>();
    browser = std::make_unique<DocumentationBrowser>(pd);
    automationPanel = std::make_unique<AutomationPanel>(pd);
    searchPanel = std::make_unique<SearchPanel>(parent);

    inspector->setAlwaysOnTop(true);

    addAndMakeVisible(console.get());
    addChildComponent(inspector.get());
    addChildComponent(browser.get());
    addChildComponent(automationPanel.get());
    addChildComponent(searchPanel.get());

    browser->addMouseListener(this, true);
    console->addMouseListener(this, true);
    automationPanel->addMouseListener(this, true);
    inspector->addMouseListener(this, true);
    searchPanel->addMouseListener(this, true);

    consoleButton.setTooltip("Open console panel");
    consoleButton.setConnectedEdges(12);
    consoleButton.setClickingTogglesState(true);
    consoleButton.onClick = [this]() {
        showPanel(0);
    };

    browserButton.setTooltip("Open documentation browser");
    browserButton.setConnectedEdges(12);
    browserButton.onClick = [this]() {
        showPanel(1);
    };
    browserButton.setClickingTogglesState(true);
    addAndMakeVisible(browserButton);

    automationButton.setTooltip("Open automation panel");
    automationButton.setConnectedEdges(12);
    automationButton.setClickingTogglesState(true);
    automationButton.onClick = [this]() {
        showPanel(2);
    };
    addAndMakeVisible(automationButton);

    searchButton.setTooltip("Open search panel");
    searchButton.setConnectedEdges(12);
    searchButton.setClickingTogglesState(true);
    searchButton.onClick = [this]() {
        showPanel(3);
    };
    addAndMakeVisible(searchButton);

    panelPinButton.setTooltip("Pin panel");
    panelPinButton.setConnectedEdges(12);
    panelPinButton.setClickingTogglesState(true);
    panelPinButton.onClick = [this]() {
        pinSidebar(panelPinButton.getToggleState());
    };
    addAndMakeVisible(panelPinButton);

    browserButton.setRadioGroupId(hash("sidebar_button"));
    automationButton.setRadioGroupId(hash("sidebar_button"));
    consoleButton.setRadioGroupId(hash("sidebar_button"));
    searchButton.setRadioGroupId(hash("sidebar_button"));

    consoleButton.setToggleState(true, dontSendNotification);

    addAndMakeVisible(consoleButton);

    inspector->setVisible(false);
    currentPanel = 0;
    updateExtraSettingsButton();
    resized();
}

Sidebar::~Sidebar()
{
    browser->removeMouseListener(this);
}

void Sidebar::paint(Graphics& g)
{
    if (!sidebarHidden) {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(0, 30, getWidth(), getHeight());

        if (inspector->isVisible()) {
            Fonts::drawStyledText(g, "Inspector: " + inspector->getTitle(), Rectangle<int>(0, 0, getWidth() - 30, 30), findColour(PlugDataColour::toolbarTextColourId), Bold, 15, Justification::centred);
        } else {
            Fonts::drawStyledText(g, panelNames[currentPanel], Rectangle<int>(0, 0, getWidth(), 30), findColour(PlugDataColour::toolbarTextColourId), Bold, 15, Justification::centred);
        }
    }
}

void Sidebar::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0.5f, 30, 0.5f, getHeight() + 0.5f);
    if (!sidebarHidden) {
        g.drawLine(0, 30, getWidth(), 30);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
        g.drawLine(getWidth() - 30, 30, getWidth() - 30, getHeight() + 0.5f);
    }
}

void Sidebar::propertyChanged(String const& name, var const& value)
{
    if (name == "centre_sidepanel_buttons") {
        resized();
    }
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    auto buttonBarBounds = bounds.removeFromRight(30).reduced(0, 1);

    if (SettingsFile::getInstance()->getProperty<bool>("centre_sidepanel_buttons")) {
        buttonBarBounds = buttonBarBounds.withSizeKeepingCentre(30, 144);
    }

    consoleButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    browserButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    automationButton.setBounds(buttonBarBounds.removeFromTop(30));
    buttonBarBounds.removeFromTop(8);
    searchButton.setBounds(buttonBarBounds.removeFromTop(30));

    auto panelTitleBarBounds = bounds.removeFromTop(30).withTrimmedRight(-30);

    if (extraSettingsButton)
        extraSettingsButton->setBounds(panelTitleBarBounds.removeFromLeft(30));
    panelPinButton.setBounds(panelTitleBarBounds.removeFromRight(30));

    browser->setBounds(bounds);
    console->setBounds(bounds);
    inspector->setBounds(bounds);
    automationPanel->setBounds(bounds);
    searchPanel->setBounds(bounds);
}

void Sidebar::mouseDown(MouseEvent const& e)
{
    Rectangle<int> dragBar(0, 0, dragbarWidth, getHeight() - 30);
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
        int newWidth = dragStartWidth - e.getDistanceFromDragStartX();
        newWidth = std::clamp(newWidth, 230, std::max(getParentWidth() / 2, 150));

        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    }
}

void Sidebar::mouseUp(MouseEvent const& e)
{
    if (draggingSidebar) {
        draggingSidebar = false;
    }
}

void Sidebar::mouseMove(MouseEvent const& e)
{

    bool resizeCursor = e.getEventRelativeTo(this).getPosition().getX() < dragbarWidth && e.getEventRelativeTo(this).getPosition().getY() < getHeight() - 30;
    e.originalComponent->setMouseCursor(resizeCursor ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
}

void Sidebar::mouseExit(MouseEvent const& e)
{
    e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showPanel(int panelToShow)
{
    bool showConsole = panelToShow == 0;
    bool showBrowser = panelToShow == 1;
    bool showAutomation = panelToShow == 2;
    bool showSearch = panelToShow == 3;

    if (panelToShow == currentPanel && !sidebarHidden) {

        consoleButton.setToggleState(false, dontSendNotification);
        browserButton.setToggleState(false, dontSendNotification);
        automationButton.setToggleState(false, dontSendNotification);
        searchButton.setToggleState(false, dontSendNotification);

        showSidebar(false);
        return;
    }

    showSidebar(true);

    console->setVisible(showConsole);
    if (showConsole)
        console->resized();

    browser->setVisible(showBrowser);
    browser->setInterceptsMouseClicks(showBrowser, showBrowser);

    auto buttons = std::vector<TextButton*> { &consoleButton, &browserButton, &automationButton, &searchButton };

    for (int i = 0; i < buttons.size(); i++) {
        buttons[i]->setToggleState(i == panelToShow, dontSendNotification);
    }

    automationPanel->setVisible(showAutomation);
    automationPanel->setInterceptsMouseClicks(showAutomation, showAutomation);

    bool searchWasVisisble = searchPanel->isVisible();
    searchPanel->setVisible(showSearch);
    if (showSearch && !searchWasVisisble)
        searchPanel->grabFocus();
    searchPanel->setInterceptsMouseClicks(showSearch, showSearch);

    hideParameters();

    currentPanel = panelToShow;

    updateExtraSettingsButton();

    repaint();
}

bool Sidebar::isShowingBrowser()
{
    return browser->isVisible();
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

void Sidebar::pinSidebar(bool pin)
{
    pinned = pin;

    if (!pinned && lastParameters.isEmpty()) {
        hideParameters();
    }
}

bool Sidebar::isPinned() const
{
    return pinned;
}

bool Sidebar::isHidden() const
{
    return sidebarHidden;
}

void Sidebar::showParameters(String const& name, Array<ObjectParameters>& params)
{
    lastParameters = params;
    inspector->loadParameters(params);
    inspector->setTitle(name);

    if (!pinned) {
        inspector->setVisible(true);
    }

    updateExtraSettingsButton();
    repaint();
}

void Sidebar::updateExtraSettingsButton()
{
    if (inspector->isVisible()) {
        extraSettingsButton = inspector->getExtraSettingsComponent();
    } else if (console->isVisible()) {
        extraSettingsButton = console->getExtraSettingsComponent();
    } else if (browser->isVisible()) {
        extraSettingsButton = browser->getExtraSettingsComponent();
    } else if (searchPanel->isVisible()) {
        extraSettingsButton = searchPanel->getExtraSettingsComponent();
    } else {
        extraSettingsButton.reset(nullptr);
        return;
    }

    addChildComponent(extraSettingsButton.get());
    extraSettingsButton->setVisible(!isHidden());
    resized();
}

void Sidebar::hideParameters()
{
    if (!pinned) {
        inspector->setVisible(false);
    }

    if (pinned) {
        Array<ObjectParameters> params = {};
        inspector->loadParameters(params);
    }

    console->deselect();
    updateExtraSettingsButton();

    repaint();
}

void Sidebar::clearConsole()
{
    console->clear();
}

void Sidebar::updateConsole(int numMessages, bool newWarning)
{
    if (currentPanel != 0 || sidebarHidden) {
        consoleButton.numNotifications += numMessages;
        consoleButton.hasWarning = consoleButton.hasWarning || newWarning;
        consoleButton.repaint();
    } else {
        consoleButton.numNotifications = 0;
    }

    console->update();
}

void Sidebar::clearSearchOutliner()
{
    searchPanel->clear();
}
