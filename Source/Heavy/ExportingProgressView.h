/*
 // Copyright (c) 2022 Timothy Schoen and Wasted-Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"
#include "../Utility/OSUtils.h"

#if JUCE_LINUX
#    include <unistd.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#endif

#include <z_libpd.h>

class ExportingProgressView : public Component
    , public Thread
    , public Timer {
    TextEditor console;

    ChildProcess* processToMonitor;

public:
    enum ExportState {
        Busy,
        WaitingForUserInput,
        Success,
        Failure,
        NotExporting
    };

    TextButton continueButton = TextButton("Continue");

    ExportState state = NotExporting;

    String userInteractionMessage;
    WaitableEvent userInteractionWait;
    TextButton confirmButton = TextButton("Done!");

    static constexpr int maxLength = 512;
    char processOutput[maxLength];

    ExportingProgressView()
        : Thread("Console thread")
    {
        setVisible(false);
        addChildComponent(continueButton);
        addChildComponent(confirmButton);
        addAndMakeVisible(console);

        continueButton.onClick = [this]() {
            showState(NotExporting);
        };

        confirmButton.onClick = [this]() {
            showState(Busy);
            userInteractionWait.signal();
        };

        console.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
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
            confirmButton.setVisible(state == WaitingForUserInput);
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

    void logToConsole(String text)
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

    // Don't call from message thread!
    void waitForUserInput(String message)
    {
        MessageManager::callAsync([this, message]() mutable {
            userInteractionMessage = message;
            showState(WaitingForUserInput);
            repaint();
        });

        userInteractionWait.wait();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        // TODO: use panel colour IDs?
        if (state == Busy) {
            Fonts::drawStyledText(g, "Exporting...", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

            getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 135, 32, 32);
        } else if (state == Success) {
            Fonts::drawStyledText(g, "Export successful", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        } else if (state == Failure) {
            Fonts::drawStyledText(g, "Exporting failed", 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        } else if (state == WaitingForUserInput) {
            Fonts::drawStyledText(g, userInteractionMessage, 0, 25, getWidth(), 40, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        }
    }

    void resized() override
    {
        console.setBounds(proportionOfWidth(0.1f), 80, proportionOfWidth(0.8f), getHeight() - 172);
        continueButton.setBounds(proportionOfWidth(0.42f), getHeight() - 60, proportionOfWidth(0.12f), 24);
        confirmButton.setBounds(proportionOfWidth(0.42f), getHeight() - 60, proportionOfWidth(0.12f), 24);
    }
};
