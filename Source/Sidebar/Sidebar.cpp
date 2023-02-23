/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"

#include "Sidebar.h"
#include "Console.h"
#include "Inspector.h"
#include "DocumentBrowser.h"
#include "AutomationPanel.h"
#include "SearchPanel.h"

Sidebar::Sidebar(PluginProcessor* instance, PluginEditor* parent)
    : pd(instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(pd);
    inspector = new Inspector;
    browser = new DocumentBrowser(pd);
    automationPanel = new AutomationPanel(pd);
    searchPanel = new SearchPanel(parent);

    inspector->setAlwaysOnTop(true);

    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    addChildComponent(browser);
    addChildComponent(automationPanel);
    addChildComponent(searchPanel);

    browser->addMouseListener(this, true);
    console->addMouseListener(this, true);
    automationPanel->addMouseListener(this, true);
    inspector->addMouseListener(this, true);
    searchPanel->addMouseListener(this, true);

    consoleButton.setTooltip("Open console panel");
    consoleButton.setConnectedEdges(12);
    consoleButton.getProperties().set("Style", "SmallIcon");
    consoleButton.setClickingTogglesState(true);
    consoleButton.onClick = [this]() {
        showPanel(0);
    };

    browserButton.setTooltip("Open documentation browser");
    browserButton.setConnectedEdges(12);
    browserButton.getProperties().set("Style", "SmallIcon");
    browserButton.onClick = [this]() {
        showPanel(1);
    };
    browserButton.setClickingTogglesState(true);
    addAndMakeVisible(browserButton);

    automationButton.setTooltip("Open automation panel");
    automationButton.setConnectedEdges(12);
    automationButton.getProperties().set("Style", "SmallIcon");
    automationButton.setClickingTogglesState(true);
    automationButton.onClick = [this]() {
        showPanel(2);
    };
    addAndMakeVisible(automationButton);

    searchButton.setTooltip("Open search panel");
    searchButton.setConnectedEdges(12);
    searchButton.getProperties().set("Style", "SmallIcon");
    searchButton.setClickingTogglesState(true);
    searchButton.onClick = [this]() {
        showPanel(3);
    };
    addAndMakeVisible(searchButton);

    browserButton.setRadioGroupId(1100);
    automationButton.setRadioGroupId(1100);
    consoleButton.setRadioGroupId(1100);
    searchButton.setRadioGroupId(1100);

    consoleButton.setToggleState(true, dontSendNotification);

    addAndMakeVisible(consoleButton);

    inspector->setVisible(false);
    showPanel(0);
}

Sidebar::~Sidebar()
{
    browser->removeMouseListener(this);
    delete console;
    delete inspector;
    delete browser;
    delete automationPanel;
    delete searchPanel;
}

void Sidebar::paint(Graphics& g)
{
    // Sidebar
    g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
    g.fillRect(0, 0, getWidth(), getHeight() - 30);

    // Background for buttons
    g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
    g.fillRect(0, 0, getWidth(), 26);
}

void Sidebar::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 26, getWidth(), 26);
    g.drawLine(0.0f, getHeight() - 29.5f, static_cast<float>(getWidth()), getHeight() - 29.5f);
    g.drawLine(0.5f, 0, 0.5f, getHeight() - 29.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    auto tabbarBounds = bounds.removeFromTop(26);
    int buttonWidth = getWidth() / 4;

    consoleButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    browserButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    automationButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    searchButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));

    browser->setBounds(bounds);
    console->setBounds(bounds);
    inspector->setBounds(bounds);
    automationPanel->setBounds(bounds);
    searchPanel->setBounds(bounds);
}

void Sidebar::mouseDown(MouseEvent const& e)
{
    Rectangle<int> dragBar(0, dragbarWidth, 15, getHeight());
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
        newWidth = std::clamp(newWidth, 100, std::max(getParentWidth() / 2, 150));

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

    bool resizeCursor = e.getEventRelativeTo(this).getPosition().getX() < dragbarWidth;
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

    console->setVisible(showConsole);

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
}

bool Sidebar::isShowingBrowser()
{
    return browser->isVisible();
}

void Sidebar::updateAutomationParameters()
{
    if (automationPanel) {
        // Might be called from audio thread
        MessageManager::callAsync([this]() {
            automationPanel->updateParameters();
        });
    };
};

void Sidebar::showSidebar(bool show)
{
    sidebarHidden = !show;

    if (!show) {
        lastWidth = getWidth();
        int newWidth = dragbarWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    } else {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
}

void Sidebar::pinSidebar(bool pin)
{
    pinned = pin;

    if (!pinned && lastParameters.empty()) {
        hideParameters();
    }
}

bool Sidebar::isPinned()
{
    return pinned;
}

void Sidebar::showParameters(String const& name, ObjectParameters& params)
{
    lastParameters = params;
    inspector->loadParameters(params);
    inspector->setTitle(name.upToFirstOccurrenceOf(" ", false, false));

    if (!pinned) {
        inspector->setVisible(true);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);

    if (!pinned) {
        inspector->setVisible(true);
        console->setVisible(false);
        browser->setVisible(false);
        searchPanel->setVisible(false);
        automationPanel->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    if (!pinned) {
        inspector->setVisible(false);
    }

    if (pinned) {
        ObjectParameters params = {};
        inspector->loadParameters(params);
    }

    console->deselect();
}

bool Sidebar::isShowingConsole() const
{
    return console->isVisible();
}

void Sidebar::clearConsole()
{
    console->clear();
}

void Sidebar::updateConsole()
{
    console->update();
}

void Sidebar::tabChanged()
{
    searchPanel->clearSearchTargets();
    searchPanel->updateResults();
}
