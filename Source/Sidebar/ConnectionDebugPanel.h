/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"

class Connection;
class PluginEditor;

// Sidebar panel for Detective Mode: pause and step through the messages, set their speed,
// and see the last messages that went through the selected connection
class ConnectionDebugPanel final : public Component
    , private Timer {
public:
    explicit ConnectionDebugPanel(PluginEditor* editor);

    // Shows the messages that went through this connection
    void setConnection(Connection* connection);

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void update();

    class HistoryModel final : public ListBoxModel {
    public:
        explicit HistoryModel(Component& panel)
            : panel(panel)
        {
        }

        int getNumRows() override { return items.size(); }
        void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override;

        StringArray items;

    private:
        Component& panel;
    };

    PluginEditor* editor;
    Component::SafePointer<Connection> connection;

    TextButton pauseButton;
    TextButton stepButton;
    Label speedLabel;
    Slider speedSlider;
    ListBox historyList;
    HistoryModel historyModel { *this };

    String placeholder;
    uint64 lastSequenceNumber = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionDebugPanel)
};
