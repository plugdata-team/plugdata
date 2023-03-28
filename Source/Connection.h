/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>

#include <concurrentqueue.h> // Move to impl
#include "Constants.h"
#include "Iolet.h"           // Move to impl
#include "Pd/MessageListener.h"
#include "Utility/RateReducer.h"
#include "Utility/ModifierKeyListener.h"

using PathPlan = std::vector<Point<float>>;

class Canvas;
class PathUpdater;

class Connection : public Component
    , public ComponentListener
    , public Value::Listener
    , public pd::MessageListener
    , public SettableTooltipClient
{
public:
    int inIdx;
    int outIdx;

    SafePointer<Iolet> inlet, outlet;
    SafePointer<Object> inobj, outobj;

    Path toDraw;
    String lastId;

    Connection(Canvas* parent, Iolet* start, Iolet* end, void* oc);
    ~Connection() override;

    void updateOverlays(int overlay);

    static void renderConnectionPath(Graphics& g,
                                     Canvas* cnv,
                                     Path connectionPath,
                                     bool isSignal,
                                     bool isMouseOver = false,
                                     bool showDirection = false,
                                     bool showConnectionOrder = false,
                                     bool isSelected = false,
                                     Point<int> mousePos = { 0, 0 },
                                     bool isHovering = false,
                                     int connections = 0,
                                     int connectionNum = 0);

    static Path getNonSegmentedPath(Point<float> start, Point<float> end);

    void paint(Graphics&) override;

    bool isSegmented();
    void setSegmented(bool segmented);

    void updatePath();

    void lookAndFeelChanged() override;

    bool hitTest(int x, int y) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    Point<float> getStartPoint();
    Point<float> getEndPoint();

    void reconnect(Iolet* target);

    bool intersects(Rectangle<float> toCheck, int accuracy = 4) const;
    int getClosestLineIdx(Point<float> const& position, PathPlan const& plan);

    String getId() const;

    void setPointer(void* ptr);
    void* getPointer();

    t_symbol* getPathState();
    void pushPathState();
    void popPathState();

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<float> start, Point<float> end, Point<float> increment);

    void findPath();

    bool intersectsObject(Object* object);
    bool straightLineIntersectsObject(Line<float> toCheck, Array<Object*>& objects);

    void receiveMessage(String const& name, int argc, t_atom* argv) override;

private:
    bool wasSelected = false;
    bool segmented = false;

    Array<SafePointer<Connection>> reconnecting;

    Rectangle<float> startReconnectHandle, endReconnectHandle, endCableOrderDisplay;

    int getMultiConnectNumber();
    int getNumberOfConnections();

    PathPlan currentPlan;

    Value locked;
    Value presentationMode;

    bool showDirection = false;
    bool showConnectionOrder = false;

    Canvas* cnv;

    Point<float> origin, offset;

    int dragIdx = -1;

    float mouseDownPosition = 0;
    bool isHovering = false;

    void valueChanged(Value& v) override;

    struct t_fake_outconnect {
        void* oc_next;
        t_pd* oc_to;
        t_symbol* outconnect_path_data;
    };

    t_fake_outconnect* ptr;

    friend class ConnectionPathUpdater;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};

// TODO: hide behind Connection interface to reduce includes!
class ConnectionBeingCreated : public Component {
    SafePointer<Iolet> iolet;
    Component* cnv;
    Path connectionPath;

public:
    ConnectionBeingCreated(Iolet* target, Component* canvas)
        : iolet(target)
        , cnv(canvas)
    {

        // Only listen for mouse-events on canvas and the original iolet
        setInterceptsMouseClicks(false, true);
        cnv->addMouseListener(this, true);
        iolet->addMouseListener(this, false);

        cnv->addAndMakeVisible(this);

        setAlwaysOnTop(true);
    }

    ~ConnectionBeingCreated()
    {
        cnv->removeMouseListener(this);
        iolet->removeMouseListener(this);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        mouseMove(e);
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (rateReducer.tooFast())
            return;

        auto ioletPoint = cnv->getLocalPoint((Component*)iolet->object, iolet->getBounds().getCentre());
        auto cursorPoint = cnv->getLocalPoint(nullptr, e.getScreenPosition());

        auto& startPoint = iolet->isInlet ? cursorPoint : ioletPoint;
        auto& endPoint = iolet->isInlet ? ioletPoint : cursorPoint;

        connectionPath = Connection::getNonSegmentedPath(startPoint.toFloat(), endPoint.toFloat());

        auto bounds = connectionPath.getBounds().getSmallestIntegerContainer().expanded(3);

        // Make sure we have minimal bounds, expand slightly to take line thickness into account
        setBounds(bounds);

        // Remove bounds offset from path, because we've already set our origin by setting component bounds
        connectionPath.applyTransform(AffineTransform::translation(-bounds.getX(), -bounds.getY()));

        repaint();
        iolet->repaint();
    }

    void paint(Graphics& g) override
    {
        if (!iolet) {
            jassertfalse; // shouldn't happen
            return;
        }
        Connection::renderConnectionPath(g, (Canvas*)cnv, connectionPath, iolet->isSignal, true);
    }

    Iolet* getIolet()
    {
        return iolet;
    }

    RateReducer rateReducer = RateReducer(90);
};

// Helper class to group connection path changes together into undoable/redoable actions
class ConnectionPathUpdater : public Timer {
    Canvas* canvas;

    moodycamel::ConcurrentQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>> connectionUpdateQueue = moodycamel::ConcurrentQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>>(4096);

    void timerCallback() override;

public:
    ConnectionPathUpdater(Canvas* cnv)
        : canvas(cnv) {};

    void pushPathState(Connection* connection, t_symbol* newPathState)
    {
        connectionUpdateQueue.enqueue({ connection, newPathState });
        startTimer(50);
    }
};
