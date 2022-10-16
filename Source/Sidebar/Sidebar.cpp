/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"
#include "PluginProcessor.h"
#include "Canvas.h"

#include "Sidebar.h"
#include "Console.h"
#include "Inspector.h"
#include "DocumentBrowser.h"
#include "AutomationPanel.h"

Sidebar::Sidebar(PlugDataAudioProcessor* instance)
    : pd(instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(pd);
    inspector = new Inspector;
    browser = new DocumentBrowser(pd);
    automationPanel = new AutomationPanel(pd);
    
    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    addChildComponent(browser);
    addChildComponent(automationPanel);
    
    browser->setAlwaysOnTop(true);
    browser->addMouseListener(this, true);
    
    browserButton.setTooltip("Open documentation browser");
    browserButton.setConnectedEdges(12);
    browserButton.setName("statusbar:browser");
    browserButton.onClick = [this]()
    {
        showPanel(1);
    };
    browserButton.setClickingTogglesState(true);
    addAndMakeVisible(browserButton);

    automationButton.setTooltip("Open automation panel");
    automationButton.setConnectedEdges(12);
    automationButton.setName("statusbar:browser");
    automationButton.setClickingTogglesState(true);
    automationButton.onClick = [this]()
    {
        showPanel(2);
    };
    addAndMakeVisible(automationButton);
    
    consoleButton.setTooltip("Open automation panel");
    consoleButton.setConnectedEdges(12);
    consoleButton.setName("statusbar:console");
    consoleButton.setClickingTogglesState(true);
    consoleButton.onClick = [this]()
    {
        showPanel(0);
    };
    
    browserButton.setRadioGroupId(1100);
    automationButton.setRadioGroupId(1100);
    consoleButton.setRadioGroupId(1100);
    
    consoleButton.setToggleState(true, dontSendNotification);
    
    addAndMakeVisible(consoleButton);
}

Sidebar::~Sidebar()
{
    browser->removeMouseListener(this);
    delete console;
    delete inspector;
    delete browser;
    delete automationPanel;
}

void Sidebar::paint(Graphics& g)
{
    // Makes sure the theme gets updated
    if (automationPanel)
        automationPanel->viewport.repaint();

    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Sidebar
    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.fillRect(getWidth() - sWidth, 0, sWidth, getHeight() - 28);

    // Draggable bar
    g.setColour(findColour(PlugDataColour::panelTextColourId));
    g.fillRect(getWidth() - sWidth, 0, dragbarWidth + 1, getHeight());

    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
}

void Sidebar::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 28, getWidth(), 28);
    g.drawLine(0.0f, getHeight() - 27.5f, static_cast<float>(getWidth()), getHeight() - 27.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    
    auto tabbarBounds = bounds.removeFromTop(28);
    bounds.removeFromLeft(dragbarWidth);
    
    int buttonWidth = getWidth() / 3;
    
    consoleButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    browserButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    automationButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));

    console->setBounds(bounds);
    inspector->setBounds(bounds);
    browser->setBounds(getLocalBounds().withTrimmedTop(28));
    automationPanel->setBounds(getLocalBounds().withTrimmedTop(28));
}

void Sidebar::mouseDown(MouseEvent const& e)
{
    Rectangle<int> dragBar(0, dragbarWidth, 15, getHeight());
    if (dragBar.contains(e.getPosition()) && !sidebarHidden) {
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
    bool resizeCursor = e.getPosition().getX() < dragbarWidth;
    setMouseCursor(resizeCursor ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
}

void Sidebar::mouseExit(MouseEvent const& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showPanel(int panelToShow)
{
    browser->setVisible(panelToShow == 1);
    automationPanel->setVisible(panelToShow == 2);
    
}

bool Sidebar::isShowingBrowser()
{
    return browser->isVisible();
}


#if PLUGDATA_STANDALONE
void Sidebar::updateAutomationParameters()
{
    if (automationPanel) {
        // Might be called from audio thread
        MessageManager::callAsync([this]() { //automationPanel->updateParameters();
            
        });
    };
};
#endif

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
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);

    if (!pinned) {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    if (!pinned) {
        inspector->setVisible(false);
        console->setVisible(true);
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

void Sidebar::updateConsole()
{
    console->update();
}
