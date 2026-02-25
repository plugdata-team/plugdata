/*
 // Copyright (c) 2021-2025 Timothy Schoen
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
#include "Utility/NVGUtils.h"
#include "LookAndFeel.h"

using PathPlan = SmallArray<Point<float>>;

class Canvas;
class Connection final : public DrawablePath
    , public ComponentListener
    , public ChangeListener
    , public pd::MessageListener
    , public NVGComponent
    , public MultiTimer {
public:
    int inIdx;
    int outIdx;
    int numSignalChannels = 1;

    WeakReference<Iolet> inlet, outlet;
    WeakReference<Object> inobj, outobj;

    Path toDrawLocalSpace;
    String lastId;

    Connection(Canvas* parent, Iolet* start, Iolet* end, t_outconnect* oc);
    ~Connection() override;

    static Path getNonSegmentedPath(Point<float> start, Point<float> end);

    bool isSegmented() const;
    void setSegmented(bool segmented);

    bool intersectsRectangle(Rectangle<int> rectToIntersect) const;

    void render(NVGcontext* nvg) override;
    void renderConnectionOrder(NVGcontext* nvg) const;

    void updatePath();

    void updateReconnectHandle();

    void forceUpdate();

    void lookAndFeelChanged() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    bool hitTest(int x, int y) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    Point<float> getStartPoint() const;
    Point<float> getEndPoint() const;

    void reconnect(Iolet const* target);

    bool intersects(Rectangle<float> toCheck, int accuracy = 4) const;
    int getClosestLineIdx(Point<float> const& position, PathPlan const& plan) const;

    void setPointer(t_outconnect* ptr);
    t_outconnect* getPointer() const;

    t_symbol* getPathState() const;
    void pushPathState(bool force = false);

    void popPathState();

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    // Pathfinding
    int findLatticePaths(PathPlan& bestPath, PathPlan& pathStack, Point<float> start, Point<float> end, Point<float> increment);

    void findPath();

    void applyBestPath();

    bool straightLineIntersectsObject(Line<float> toCheck, SmallArray<Object*>& objects) const;

    void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override;

    bool isSelected() const;

    bool isMouseHovering() const { return isHovering; }

    StringArray getMessageFormated() const;

private:
    enum Timer { StopAnimation,
        Animation };

    void timerCallback(int ID) override;

    void animate();

    int getMultiConnectNumber() const;
    int getNumSignalChannels() const;
    int getNumberOfConnections() const;

    NVGcolor getConnectionColour() const;

    void setSelected(bool shouldBeSelected);

    void pathChanged() override;

    float getPathWidth() const;

    SmallArray<SafePointer<Connection>> reconnecting;
    Rectangle<float> startReconnectHandle, endReconnectHandle;

    PathPlan currentPlan;

    Value locked;
    Value presentationMode;

    NVGcolor handleColour;
    NVGcolor shadowColour;
    NVGcolor outlineColour;

    NVGcolor textColour;

    RectangleList<int> clipRegion;

    enum CableType { DataCable,
        GemCable,
        SignalCable };
    CableType cableType;

    Canvas* cnv;

    Point<float> previousPStart = Point<float>();

    int dragIdx = -1;

    float mouseDownPosition = 0;

    NVGCachedPath cachedPath;
    pd::WeakReference ptr;

    SmallArray<pd::Atom> lastValue;
    t_symbol* lastSelector = nullptr;

    float offset = 0.0f;
    float pathLength = 0.0f;

    PlugDataLook::ConnectionStyle connectionStyle = PlugDataLook::ConnectionStyleDefault;
    bool selectedFlag : 1 = false;
    bool wasSelected : 1 = false;
    bool segmented : 1 = false;
    bool isHovering : 1 = false;
    bool isInStartReconnectHandle : 1 = false;
    bool isInEndReconnectHandle : 1 = false;

    friend class ConnectionPathUpdater;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Connection)
};

class ConnectionBeingCreated final : public DrawablePath
    , public NVGComponent {
    SafePointer<Iolet> iolet;
    Component* cnv;
    Point<float> lastMousePos;

public:
    ConnectionBeingCreated(Iolet* target, Component* canvas)
        : NVGComponent(this)
        , iolet(target)
        , cnv(canvas)
    {
        setStrokeThickness(5.0f);

        // Only listen for mouse-events on canvas and the original iolet
        setInterceptsMouseClicks(false, true);
        cnv->addMouseListener(this, true);
        iolet->addMouseListener(this, false);

        cnv->addAndMakeVisible(this);
        cnv->repaint();

        setAlwaysOnTop(true);
        setAccessible(false); // TODO: implement accessibility. We disable default, since it makes stuff slow on macOS
    }

    ~ConnectionBeingCreated() override
    {
        cnv->removeMouseListener(this);
        if (iolet)
            iolet->removeMouseListener(this);
    }

    void pathChanged() override
    {
        strokePath.clear();
        strokePath = path;
        setBoundsToEnclose(getDrawableBounds().expanded(3));
        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (rateReducer.tooFast())
            return;

        updatePosition(e.getEventRelativeTo(cnv).position);
        scrollViewport(cnv, e);
    }

    void mouseMove(MouseEvent const& e) override
    {
        if (rateReducer.tooFast())
            return;

        updatePosition(e.getEventRelativeTo(cnv).position);
        scrollViewport(cnv, e);
    }

    void updatePosition(Point<float> const cursorPoint)
    {
        if (!iolet)
            return;

        auto const ioletPoint = cnv->getLocalPoint(reinterpret_cast<Component*>(iolet->object), iolet->getBounds().toFloat().getCentre());
        auto const& startPoint = iolet->isInlet ? cursorPoint : ioletPoint;
        auto const& endPoint = iolet->isInlet ? ioletPoint : cursorPoint;

        lastMousePos = cursorPoint;
        auto const connectionPath = Connection::getNonSegmentedPath(startPoint.toFloat(), endPoint.toFloat());
        setPath(connectionPath);

        repaint();
        iolet->repaint();
    }

    static void scrollViewport(Component* cnvComp, MouseEvent const& e);

    void render(NVGcontext* nvg) override
    {
        auto const shadowColour = findColour(PlugDataColour::canvasBackgroundColourId).contrasting(0.06f).withAlpha(0.24f);

        NVGScopedState scopedState(nvg);
        setJUCEPath(nvg, getPath());

        auto const connectionStyle = PlugDataLook::getConnectionStyle();
        float cableThickness;
        switch (connectionStyle) {
        case PlugDataLook::ConnectionStyleVanilla:
            cableThickness = iolet->isSignal ? 4.5f : 2.5f;
            break;
        case PlugDataLook::ConnectionStyleThin:
            cableThickness = 3.0f;
            break;
        default:
            cableThickness = 4.5f;
            break;
        }

        nvgStrokeWidth(nvg, cableThickness);

        if (iolet && iolet->isSignal && connectionStyle != PlugDataLook::ConnectionStyleVanilla) {
            auto const lineColour = cnv->findColour(PlugDataColour::signalColourId).brighter(0.6f);
            auto dashColor = convertColour(shadowColour);
            dashColor.a = 255;
            dashColor.r *= 0.4f;
            dashColor.g *= 0.4f;
            dashColor.b *= 0.4f;
            nvgStrokePaint(nvg, nvgDoubleStroke(nvg, convertColour(lineColour), convertColour(shadowColour), dashColor, 2.5f, false, false, 0.0f));
            nvgStroke(nvg);
        } else {
            auto const lineColour = cnv->findColour(PlugDataColour::dataColourId).brighter(0.6f);
            nvgStrokePaint(nvg, nvgDoubleStroke(nvg, convertColour(lineColour), convertColour(shadowColour), convertColour(Colours::transparentBlack), 0.0f, false, false, 0.0f));
            nvgStroke(nvg);
        }

        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGBAf(0.6f, 0.6f, 0.6f, 0.7f));
        nvgCircle(nvg, lastMousePos.x, lastMousePos.y, 3.5f);
        nvgFill(nvg);
    }

    void toNextIolet()
    {
        if (!iolet)
            return;

        iolet->removeMouseListener(this);
        iolet->isTargeted = false;
        iolet->repaint();

        iolet = iolet->getNextIolet();
        iolet->addMouseListener(this, false);
        iolet->isTargeted = true;
        iolet->repaint();

        updatePosition(cnv->getMouseXYRelative().toFloat());
    }

    Iolet* getIolet()
    {
        return iolet;
    }

    RateReducer rateReducer = RateReducer(90);
};

// Helper class to group connection path changes together into undoable/redoable actions
class ConnectionPathUpdater final : public Timer {
    Canvas* canvas;

    moodycamel::ReaderWriterQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>> connectionUpdateQueue = moodycamel::ReaderWriterQueue<std::pair<Component::SafePointer<Connection>, t_symbol*>>(4096);

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

    void timerCallback() override;
};
