/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>

#include <readerwriterqueue.h>
#include "Constants.h"
#include "Objects/AllGuis.h"
#include "Iolet.h"       // Move to impl
#include "Pd/Instance.h" // Move to impl
#include "Pd/MessageListener.h"
#include "Utility/RateReducer.h"
#include "Utility/ModifierKeyListener.h"
#include "Utility/NVGComponent.h"

using PathPlan = std::vector<Point<float>>;

class Canvas;
class PathUpdater;

class Connection : public Component
    , public ComponentListener
    , public ChangeListener
    , public pd::MessageListener 
    , public NVGComponent
{
public:
    int inIdx;
    int outIdx;
    int numSignalChannels = 1;

    WeakReference<Iolet> inlet, outlet;
    WeakReference<Object> inobj, outobj;

    Path toDraw, toDrawLocalSpace;
    // ALEXCONREFACTOR
    Path hitTestPath;
    String lastId;

    std::atomic<int> messageActivity;

    Connection(Canvas* parent, Iolet* start);

    Connection(Canvas* parent, Iolet* start, Iolet* end, t_outconnect* oc);
    ~Connection() override;

    void updateOverlays(int overlay);

    static Path getNonSegmentedPath(Point<float> start, Point<float> end);

    void paintOverChildren(Graphics&) override;

    bool isSegmented() const;
    void setSegmented(bool segmented);
        
    void render(NVGcontext* nvg) override;

    void updatePath();

    void forceUpdate();

    void lookAndFeelChanged() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    bool hitTest(int x, int y) override;

    void resized() override;

    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    Point<float> getStartPoint() const;
    Point<float> getEndPoint() const;

    void reconnect(Iolet* target);

    bool intersects(Rectangle<float> toCheck, int accuracy = 4) const;
    int getClosestLineIdx(Point<float> const& position, PathPlan const& plan);

    void setPointer(t_outconnect* ptr);
    t_outconnect* getPointer();

    t_symbol* getPathState();
    void pushPathState();
    void popPathState();

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<float> start, Point<float> end, Point<float> increment);

    void findPath();

    void applyBestPath();

    bool intersectsObject(Object* object) const;
    bool straightLineIntersectsObject(Line<float> toCheck, Array<Object*>& objects);

    void receiveMessage(t_symbol* symbol, pd::Atom const atoms[8], int numAtoms) override;

    bool isSelected() const;

    StringArray getMessageFormated();
    int getSignalData(t_float* output, int maxChannels);

private:
    void resizeToFit();

    void updateBounds();

    int getMultiConnectNumber();
    int getNumSignalChannels();
    int getNumberOfConnections();
        
    static Point<float> bezierPointAtDistance(const Point<float>& start, const Point<float>& cp1, const Point<float>& cp2, const Point<float>& end, float distance);

    void setSelected(bool shouldBeSelected);

    Array<SafePointer<Connection>> reconnecting;
    Rectangle<float> startReconnectHandle, endReconnectHandle, endCableOrderDisplay;

    std::atomic<bool> selectedFlag = false;
    std::atomic<bool> segmented = false;
    std::atomic<bool> isHovering = false;
    
    PathPlan currentPlan;

    Value locked;
    Value presentationMode;

    bool showDirection = false;
    bool showConnectionOrder = false;
    bool showActiveState = false;

    Canvas* cnv;

    Point<float> previousPStart = Point<float>();

    Point<float> start_;
    Point<float> end_;
    float width_ = 0.0f;
    float height_ = 0.0f;
    Point<float> cp1_;
    Point<float> cp2_;

    Rectangle<int> previousBounds;

    bool generateHitPath;

    int dragIdx = -1;

    float mouseDownPosition = 0;

    pd::WeakReference ptr;

    pd::Atom lastValue[8];
    int lastNumArgs = 0;
    t_symbol* lastSelector = nullptr;

    Colour baseColour;
    Colour dataColour;
    Colour signalColour;
    Colour handleColour;
    Colour shadowColour;

    friend class ConnectionPathUpdater;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};

// Helper class to group connection path changes together into undoable/redoable actions
class ConnectionPathUpdater : public Timer {
    Canvas* canvas;

    moodycamel::ReaderWriterQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>> connectionUpdateQueue = moodycamel::ReaderWriterQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>>(4096);

    void timerCallback() override;

public:
    explicit ConnectionPathUpdater(Canvas* cnv)
        : canvas(cnv)
    {
    }

    void pushPathState(Connection* connection, t_symbol* newPathState)
    {
        connectionUpdateQueue.enqueue({ connection, newPathState });
        startTimer(50);
    }
};
