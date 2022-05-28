/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"
#include "PluginProcessor.h"

#include "Sidebar.h"
#include "Console.h"
#include "Inspector.h"
#include "DocumentBrowser.h"
#include "AutomationPanel.h"

Sidebar::Sidebar(PlugDataAudioProcessor* instance) : pd(instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(pd);
    inspector = new Inspector;
    browser = new DocumentBrowser(pd);

    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    addChildComponent(browser);

    browser->setAlwaysOnTop(true);
    browser->addMouseListener(this, true);

    setBounds(getParentWidth() - lastWidth, 40, lastWidth, getParentHeight() - 40);
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
    if (automationPanel) automationPanel->viewport.repaint();

    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());

    // Sidebar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, sWidth, getHeight() - 28);

    // Draggable bar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, dragbarWidth + 1, getHeight());

    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(dragbarWidth);

    console->setBounds(bounds);
    inspector->setBounds(bounds);
    browser->setBounds(getLocalBounds());
    if (automationPanel) automationPanel->setBounds(getLocalBounds().withTop(getHeight() - 300));
}

void Sidebar::mouseDown(const MouseEvent& e)
{
    Rectangle<int> dragBar(0, dragbarWidth, getWidth(), getHeight());
    if (dragBar.contains(e.getPosition()) && !sidebarHidden)
    {
        draggingSidebar = true;
        dragStartWidth = getWidth();
    }
    else
    {
        draggingSidebar = false;
    }
}

void Sidebar::mouseDrag(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        int newWidth = dragStartWidth - e.getDistanceFromDragStartX();
        newWidth = std::min(newWidth, getParentWidth() / 2);

        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    }
}

void Sidebar::mouseUp(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        // getCurrentCanvas()->checkBounds(); fix this
        draggingSidebar = false;
    }
}

void Sidebar::mouseMove(const MouseEvent& e)
{
    if (e.getPosition().getX() < dragbarWidth)
    {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void Sidebar::mouseExit(const MouseEvent& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
}

void Sidebar::showBrowser(bool show)
{
    browser->setVisible(show);
    pinned = show;
    if (show)
    {
        browser->grabKeyboardFocus();
    }
}

bool Sidebar::isShowingBrowser()
{
    return browser->isVisible();
};

void Sidebar::showAutomationPanel(bool show)
{
    if (show)
    {
        automationPanel = new AutomationPanel(pd);
        addAndMakeVisible(automationPanel);
        automationPanel->setAlwaysOnTop(true);
        automationPanel->toFront(false);
    }
    else
    {
        delete automationPanel;
        automationPanel = nullptr;
    }

    resized();
}

#if PLUGDATA_STANDALONE
void Sidebar::updateParameters()
{
    if (automationPanel)
    {
        // Might be called from audio thread
        MessageManager::callAsync([this]() { automationPanel->sliders.updateParameters(); });
    };
};
#endif

void Sidebar::showSidebar(bool show)
{
    sidebarHidden = !show;

    if (!show)
    {
        lastWidth = getWidth();
        int newWidth = dragbarWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
    else
    {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
}

void Sidebar::pinSidebar(bool pin)
{
    pinned = pin;

    if (!pinned && lastParameters.empty())
    {
        hideParameters();
    }
}

bool Sidebar::isPinned()
{
    return pinned;
}

void Sidebar::showParameters(ObjectParameters& params)
{
    lastParameters = params;
    inspector->loadParameters(params);

    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);

    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    if (!pinned)
    {
        inspector->setVisible(false);
        console->setVisible(true);
    }

    if (pinned)
    {
        ObjectParameters params = {};
        inspector->loadParameters(params);
    }

    console->deselect();
}

bool Sidebar::isShowingConsole() const noexcept
{
    return console->isVisible();
}

void Sidebar::updateConsole()
{
    console->update();
}
