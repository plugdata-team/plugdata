/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

class Connection;
class Object;
class Canvas;

class Iolet : public Component
    , public SettableTooltipClient
    , public Value::Listener {
public:
    Object* object;

    Iolet(Object* parent, bool isInlet);

    void paint(Graphics&) override;

    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    bool hitTest(int x, int y) override;

    void valueChanged(Value& v) override;

    static Iolet* findNearestIolet(Canvas* cnv, Point<int> position, bool inlet, Object* boxToExclude = nullptr);

    void createConnection();

    void clearConnections();
    Array<Connection*> getConnections();

    Rectangle<int> getCanvasBounds();

    int ioletIdx;
    bool isInlet;
    bool isSignal;

    bool isTargeted = false;

    Canvas* cnv;
private:
    Value locked;
    Value presentationMode;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Iolet)
};
