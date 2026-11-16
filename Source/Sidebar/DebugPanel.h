/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"

class Connection;
class PluginEditor;
class SegmentedControl;
class TraceView;
class ProfilerView;

// Sidebar panel of the Debugger: traces the messages through a connection, and profiles how much of the CPU
// each object takes. They only run, and slow Pd down, while they're showing
class DebugPanel final : public Component
    , private Timer {
public:
    explicit DebugPanel(PluginEditor* editor);
    ~DebugPanel() override;

    // The selected connection, which the trace follows unless another connection is pinned
    void setConnection(Connection* connection);
    Connection* getConnection() const;

    void paint(Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void updateActiveView();

    std::unique_ptr<SegmentedControl> viewSelector;
    std::unique_ptr<TraceView> traceView;
    std::unique_ptr<ProfilerView> profilerView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugPanel)
};
