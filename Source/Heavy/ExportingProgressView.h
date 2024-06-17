/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"
#include "Utility/OSUtils.h"

#include <z_libpd.h>

class ExportingProgressView : public Component
    , public Thread
    , public Timer {
    TextEditor console;

    ChildProcess* processToMonitor;

public:
    enum ExportState {
        Busy,
        Success,
        Failure,
        NotExporting
    };

    TextButton continueButton = TextButton("Continue");

    ExportState state = NotExporting;

    String userInteractionMessage;

    static constexpr int maxLength = 512;
    char processOutput[maxLength];

    ExportingProgressView()
        : Thread("Console thread")
    {
        setVisible(false);
        addChildComponent(continueButton);
        addAndMakeVisible(console);

        continueButton.onClick = [this]() {
            showState(NotExporting);
        };

        console.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        console.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        console.setScrollbarsShown(true);
        console.setMultiLine(true);
        console.setReadOnly(true);
        console.setWantsKeyboardFocus(true);

        // To ensure custom LnF got assigned...
        MessageManager::callAsync([this]() {
            console.setFont(Fonts::getMonospaceFont());
        });
    }

    // For the spinning animation
    void timerCallback() override
    {
        repaint();
    }

    void run() override
    {
        while (processToMonitor && !threadShouldExit()) {
            int len = processToMonitor->readProcessOutput(processOutput, maxLength);
            if (len)
                logToConsole(String::fromUTF8(processOutput, len));

            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 100);
        }
    }

    void monitorProcessOutput(ChildProcess* process)
    {
        startTimer(20);
        processToMonitor = process;
        startThread();
    }

    void flushConsole()
    {
        while (processToMonitor) {
            int len = processToMonitor->readProcessOutput(processOutput, maxLength);
            if (!len)
                break;

            logToConsole(String::fromUTF8(processOutput, len));
        }
    }

    void stopMonitoring()
    {
        flushConsole();
        stopThread(-1);
        stopTimer();
    }

    void showState(ExportState newState)
    {
        state = newState;

        MessageManager::callAsync([this]() {
            setVisible(state < NotExporting);
            continueButton.setVisible(state >= Success);
            if (state == Busy)
                console.setText("");
            if (console.isShowing()) {
                console.grabKeyboardFocus();
            }

            resized();
            repaint();
        });
    }

    void logToConsole(String const& text)
    {

        if (text.isNotEmpty()) {
            MessageManager::callAsync([_this = SafePointer(this), text]() {
                if (!_this)
                    return;

                _this->console.setText(_this->console.getText() + text);
                _this->console.moveCaretToEnd();
                _this->console.setScrollToShowCursor(true);
            });
        }
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds();

        Path background;
        background.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);

        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillPath(background);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(background, PathStrokeType(1.0f));

        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRoundedRectangle(console.getBounds().expanded(2).toFloat(), Corners::defaultCornerRadius);

        if (state == Busy) {
            Fonts::drawStyledText(g, "Exporting...", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

            getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        } else if (state == Success) {
            Fonts::drawStyledText(g, "Export successful", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        } else if (state == Failure) {
            Fonts::drawStyledText(g, "Exporting failed", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        }
    }

    void resized() override
    {
        console.setBounds(proportionOfWidth(0.05f), 80, proportionOfWidth(0.9f), getHeight() - 172);
        continueButton.setBounds(proportionOfWidth(0.42f), getHeight() - 60, proportionOfWidth(0.12f), 24);
    }
};
