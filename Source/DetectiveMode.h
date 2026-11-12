/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <deque>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/SettingsFile.h"
#include "Pd/Instance.h"
#include "Pd/ConnectionTraceCollector.h"

class PluginProcessor;
class Canvas;
class Connection;
struct NVGcontext;

// Detective Mode shows the messages that Pd sends through connections as bubbles travelling along the cables,
// one at a time, in the order Pd sends them.
// Pd records the messages with its lock held (see ConnectionTraceCollector). Everything else here runs on the message
// thread, render() included: like all canvas drawing, it records nanovg commands that the render thread only replays.
class DetectiveMode final : public SettingsFileListener
    , private Timer {
public:
    explicit DetectiveMode(PluginProcessor* processor);
    ~DetectiveMode() override;

    bool isEnabled() const { return traceCollector != nullptr; }

    // While paused, the message being shown stays where it is
    void setPaused(bool shouldPause);
    bool isPaused() const { return paused; }

    // Pauses, plays the message being shown to the end of its cable, and stops at the start of the next one
    void step();

    // Draws the message being shown, if it's going through one of this canvas's connections
    void render(Canvas const* cnv, NVGcontext* nvg, Rectangle<int> invalidRegion) const;

    struct Message {
        uint64 sequenceNumber = 0;
        t_symbol* selector = nullptr;
        SmallArray<pd::Atom> atoms;
        bool truncated = false; // only the first atoms were recorded

        // Limiting the length keeps huge lists from filling the canvas, -1 shows everything
        String toString(int maxLength = -1) const;
    };

    // The last messages that went through a connection, oldest first, or nullptr if there were none
    std::deque<Message> const* getHistory(t_outconnect* connection) const;

    // Pd only sends messages to plugdata while debugging is on: for the connection tooltips, and for Detective Mode
    static void updatePdDebugging();

private:
    struct Bubble {
        Component::SafePointer<Canvas> canvas;
        Component::SafePointer<Connection> connection;

        TextLayout layout;
        int layoutWidth = -1;
        Colour layoutColour;

        // Where it was last drawn, in canvas coordinates
        Rectangle<float> box;
        Point<float> dot;
        float cableWidth = 0.0f;
        Rectangle<int> area;
    };

    void setEnabled(bool shouldBeEnabled);
    void settingsChanged(String const& name, var const& value) override;
    void timerCallback() override;

    void collectMessages();
    bool startNextMessage();
    void stopMessage();
    void updateBubbles();
    bool updateBubble(Bubble& bubble);

    // The connections open in any editor, by Pd connection
    UnorderedMap<t_outconnect*, SmallArray<std::pair<Canvas*, Connection*>, 1>> const& getOpenConnections();

    PluginProcessor* pd;
    std::unique_ptr<pd::ConnectionTraceCollector> traceCollector;

    UnorderedMap<t_outconnect*, SmallArray<std::pair<Canvas*, Connection*>, 1>> openConnections;
    bool openConnectionsAreCurrent = false;

    struct PendingMessage {
        t_outconnect* connection;
        Message message;
    };
    std::deque<PendingMessage> pending;
    UnorderedMap<t_outconnect*, std::deque<Message>> history;

    std::optional<Message> current;
    String currentText;
    SmallArray<Bubble, 2> bubbles;
    float progress = 0.0f; // 0 at the outlet, 1 at the inlet

    bool paused = false;
    bool stepping = false;
    double lastTickMs = 0.0;
    Value speed;

    static constexpr float messageDurationMs = 400.0f;
    static constexpr size_t maxPendingMessages = 4096;
    static constexpr size_t maxHistory = 64;
    static constexpr int maxBubbleTextLength = 300;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetectiveMode)
};
