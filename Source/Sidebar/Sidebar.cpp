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
}

void Sidebar::paint(Graphics& g)
{
    // Makes sure the theme gets updated
    if (automationPanel)
        automationPanel->viewport.repaint();

    // Sidebar
    g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
    g.fillRect(0, 0, getWidth(), getHeight() - 28);
    
    // Background for buttons
    g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
    g.fillRect(0, 0, getWidth(), 28);
    
    // Draggable bar
    g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
    g.fillRect(0.0f, 28.0f, 5.0f, getHeight() - 55.0f);
}

void Sidebar::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0, 0, getWidth(), 0);
    g.drawLine(0, 28, getWidth(), 28);
    g.drawLine(0.0f, getHeight() - 27.5f, static_cast<float>(getWidth()), getHeight() - 27.5f);
    g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    
    auto tabbarBounds = bounds.removeFromTop(28);
    
    int buttonWidth = getWidth() / 3;
    
    consoleButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    browserButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));
    automationButton.setBounds(tabbarBounds.removeFromLeft(buttonWidth));

    browser->setBounds(bounds);
    
    bounds.removeFromLeft(dragbarWidth);
    
    console->setBounds(bounds);
    inspector->setBounds(bounds);
    
    automationPanel->setBounds(bounds);
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
    e.originalComponent->setMouseCursor(resizeCursor ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
}

void Sidebar::mouseExit(MouseEvent const& e)
{
    e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showPanel(int panelToShow)
{
    bool showBrowser = panelToShow == 1;
    bool showAutomation = panelToShow == 2;
    
    browser->setVisible(showBrowser);
    browser->setInterceptsMouseClicks(showBrowser, showBrowser);
    
    auto buttons = std::vector<TextButton*>{&consoleButton, &browserButton, &automationButton};
    
    for(int i = 0; i < buttons.size(); i++) {
        buttons[i]->setToggleState(i == panelToShow, dontSendNotification);
    }
    
    automationPanel->setVisible(showAutomation);
    automationPanel->setInterceptsMouseClicks(showAutomation, showAutomation);

    if(auto* editor =  dynamic_cast<PlugDataPluginEditor*>(pd->getActiveEditor())) {
        editor->toolbarButton(PlugDataPluginEditor::Pin)->setEnabled(panelToShow == 0);
    };

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
