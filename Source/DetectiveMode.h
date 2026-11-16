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
#include "Pd/MessageListener.h"

class PluginProcessor;
class Canvas;
class Connection;
struct NVGcontext;

// Message animation shows the messages that Pd sends through connections as bubbles travelling along the cables,
// one at a time, in the order Pd sends them. Each event (a click, a clock, MIDI...) sets off a chain of messages,
// and a source that sends faster than its chains can be shown only keeps its newest chain waiting.
// It's an overlay: it plays on the canvases that show it in their current mode
class DetectiveMode final : public SettingsFileListener
    , private pd::RawMessageListener
    , private Timer {
public:
    explicit DetectiveMode(PluginProcessor* processor);
    ~DetectiveMode() override;

    bool isEnabled() const { return enabled; }

    // While paused, the message being shown stays where it is
    void setPaused(bool shouldPause);
    bool isPaused() const { return paused; }

    // Pauses, plays the message being shown to the end of its cable, and stops at the start of the next one
    void step();

    // Draws the message being shown, if it's going through one of this canvas's connections
    void render(Canvas const* cnv, NVGcontext* nvg, Rectangle<int> invalidRegion) const;

    struct Message {
        t_symbol* selector = nullptr;
        SmallArray<pd::Atom> atoms;
        bool truncated = false; // only the first atoms were recorded

        Message() = default;
        Message(t_symbol* symbol, SmallArray<pd::Atom> const& messageAtoms);

        // Limiting the length keeps huge lists from filling the canvas, -1 shows everything
        String toString(int maxLength = -1) const;
    };

    // The "Debug visualization" switch of the overlay settings: Pd only reports messages and activity while it's on
    static bool isDebuggingEnabled();

    // An option under that switch, which is off while the switch is
    static bool isVisualizationEnabled(String const& option);

    // The message animation overlay is on in at least one mode
    static bool isAnimationRequested();

    // Applies the switch to Pd
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
        float scale = 1.0f; // canvas units per screen pixel
        bool isBelowDot = false;
        Rectangle<int> area;
    };

    struct PendingMessage {
        void* connection;
        Message message;
    };

    // The messages that one event set off, in the order Pd sent them
    struct Chain {
        uint64 id;
        void* source; // the outlet that started it
        std::deque<PendingMessage> messages;
    };

    void setEnabled(bool shouldBeEnabled);
    void settingsChanged(String const& name, var const& value) override;
    void receiveRawMessage(void* target, t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override;
    void timerCallback() override;

    void clearPending();
    void addPendingMessage(void* connection, Message message);
    void startNextMessage();
    void stopMessage();
    void updateBubbles();
    bool updateBubble(Bubble& bubble);

    // All open views of a Pd connection: its patch can be open in more than one tab or window
    SmallArray<Connection*, 2> findConnections(void* connection) const;

    PluginProcessor* pd;
    bool enabled = false;

    std::deque<Chain> pending; // oldest first, the first one can be playing
    size_t numPending = 0;
    bool isPlayingChain = false;

    // Pd sends this before the first message of a chain
    t_symbol* chainSymbol;
    void* chainSource = nullptr;
    uint64 chainCounter = 0;

    std::optional<Message> current;
    String currentText;
    SmallArray<Bubble, 2> bubbles;
    float progress = 0.0f; // 0 at the outlet, 1 at the inlet
    float durationMs = minDurationMs;

    bool paused = false;
    bool stepping = false;
    double lastTickMs = 0.0;
    Value speed;

    static constexpr float minDurationMs = 400.0f;
    static constexpr float maxDurationMs = 1200.0f;
    static constexpr float durationPerPixelMs = 2.5f;
    static constexpr size_t maxPendingMessages = 4096;
    static constexpr int maxAtoms = 32;
    static constexpr int maxBubbleTextLength = 300;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetectiveMode)
};
