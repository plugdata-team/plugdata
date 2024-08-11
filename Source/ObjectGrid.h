/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Utility/SettingsFile.h"
#include "NVGSurface.h"

class Object;
class Canvas;
struct ObjectGrid : public SettingsFileListener
    , public Timer {

    int gridSize = 20;

    explicit ObjectGrid(Canvas* cnv);

    Point<int> performResize(Object* toDrag, Point<int> dragOffset, Rectangle<int> newResizeBounds);
    Point<int> performMove(Object* toDrag, Point<int> dragOffset);

    void clearIndicators(bool fast);

    void render(NVGcontext* nvg);

private:
    enum Side {
        Left,
        Right,
        Top,
        Bottom,
        VerticalCentre,
        HorizontalCentre,
    };

    void timerCallback() override;

    void propertyChanged(String const& name, var const& value) override;

    static Array<Object*> getSnappableObjects(Object* draggedObject);

    void setIndicator(int idx, Line<int> line, float lineScale);

    static Line<int> getObjectIndicatorLine(Side side, Rectangle<int> b1, Rectangle<int> b2);

    static constexpr int objectTolerance = 6;
    static constexpr int connectionTolerance = 9;

    Line<int> lines[2];
    float lineAlpha[2] = {};
    float lineTargetAlpha[2] = {};
    float lineAlphaMultiplier[2] = {};
    Canvas* cnv;

    int gridType;
    bool gridEnabled;
};
