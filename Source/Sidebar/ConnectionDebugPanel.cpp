/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ConnectionDebugPanel.h"

#include "Utility/Fonts.h"
#include "Utility/SettingsFile.h"
#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Connection.h"
#include "DetectiveMode.h"

ConnectionDebugPanel::ConnectionDebugPanel(PluginEditor* e)
    : editor(e)
    , speedLabel("", "Speed")
    , speedSlider(Slider::LinearHorizontal, Slider::TextBoxRight)
{
    pauseButton.setButtonText("Pause");
    pauseButton.onClick = [this] {
        if (auto* detectiveMode = editor->pd->detectiveMode.get()) {
            detectiveMode->setPaused(!detectiveMode->isPaused());
            update();
        }
    };
    addAndMakeVisible(pauseButton);

    stepButton.setButtonText("Step");
    stepButton.setTooltip("Pause, play the current message to the end of its connection, and stop at the next one");
    stepButton.onClick = [this] {
        if (auto* detectiveMode = editor->pd->detectiveMode.get()) {
            detectiveMode->step();
            update();
        }
    };
    addAndMakeVisible(stepButton);

    speedLabel.setFont(Fonts::getDefaultFont().withHeight(14.0f));
    addAndMakeVisible(speedLabel);

    speedSlider.setRange(0.1, 5.0, 0.1);
    speedSlider.getValueObject().referTo(SettingsFile::getInstance()->getPropertyAsValue("detective_speed"));
    speedSlider.setDoubleClickReturnValue(true, 1.0);
    speedSlider.setTextValueSuffix("x");
    speedSlider.setTextBoxStyle(Slider::TextBoxRight, false, 42, 22);
    addAndMakeVisible(speedSlider);

    historyList.setModel(&historyModel);
    historyList.setRowHeight(22);
    historyList.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    addAndMakeVisible(historyList);
}

void ConnectionDebugPanel::setConnection(Connection* newConnection)
{
    if (connection.getComponent() == newConnection)
        return;

    connection = newConnection;
    lastSequenceNumber = 0;
    update();
}

void ConnectionDebugPanel::visibilityChanged()
{
    if (isVisible()) {
        update();
        startTimerHz(10);
    } else {
        stopTimer();
    }
}

void ConnectionDebugPanel::timerCallback()
{
    update();
}

void ConnectionDebugPanel::update()
{
    auto* detectiveMode = editor->pd->detectiveMode.get();
    auto const enabled = detectiveMode && detectiveMode->isEnabled();

    pauseButton.setEnabled(enabled);
    stepButton.setEnabled(enabled);
    pauseButton.setButtonText(enabled && detectiveMode->isPaused() ? "Play" : "Pause");

    auto const* history = enabled && connection ? detectiveMode->getHistory(connection->getPointer()) : nullptr;
    auto const newestSequenceNumber = history && !history->empty() ? history->back().sequenceNumber : 0;

    String newPlaceholder;
    if (!enabled)
        newPlaceholder = "Detective Mode is off";
    else if (!connection)
        newPlaceholder = "Select a connection";
    else if (!newestSequenceNumber)
        newPlaceholder = "No messages yet";

    if (newPlaceholder != placeholder) {
        placeholder = newPlaceholder;
        repaint();
    }

    if (newestSequenceNumber == lastSequenceNumber && historyModel.items.size() == (history ? static_cast<int>(history->size()) : 0))
        return;

    // Newest first
    lastSequenceNumber = newestSequenceNumber;
    historyModel.items.clearQuick();
    if (history) {
        for (auto it = history->rbegin(); it != history->rend(); ++it)
            historyModel.items.add("#" + String(it->sequenceNumber) + "  " + it->toString());
    }
    historyList.updateContent();
    historyList.repaint();
}

void ConnectionDebugPanel::paint(Graphics& g)
{
    g.fillAll(getThemeColours(*this).sidebarBackgroundColour);
}

void ConnectionDebugPanel::paintOverChildren(Graphics& g)
{
    if (placeholder.isEmpty())
        return;

    Fonts::drawText(g, placeholder, historyList.getBounds().withTrimmedTop(16),
        getThemeColours(*this).sidebarTextColour.withAlpha(0.55f), 14, Justification::centredTop);
}

void ConnectionDebugPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    auto buttons = bounds.removeFromTop(28);
    pauseButton.setBounds(buttons.removeFromLeft(buttons.getWidth() / 2).withTrimmedRight(2));
    stepButton.setBounds(buttons.withTrimmedLeft(2));
    bounds.removeFromTop(6);

    auto speed = bounds.removeFromTop(26);
    speedLabel.setBounds(speed.removeFromLeft(48));
    speedSlider.setBounds(speed);
    bounds.removeFromTop(6);

    historyList.setBounds(bounds);
}

void ConnectionDebugPanel::HistoryModel::paintListBoxItem(int const rowNumber, Graphics& g, int const width, int const height, bool)
{
    if (!isPositiveAndBelow(rowNumber, items.size()))
        return;

    Fonts::drawText(g, items[rowNumber], Rectangle<int>(4, 0, width - 8, height), getThemeColours(panel).sidebarTextColour, 14);
}
