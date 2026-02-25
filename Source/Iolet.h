// Copyright (c) 2021-2025 Timothy Schoen
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

class Connection;
class Object;
class Canvas;
struct NVGcontext;

class Iolet final : public Component
    , public SettableTooltipClient
    , public Value::Listener
    , public SettingsFileListener
    , public NVGComponent {
public:
    Object* object;
    Canvas* cnv;

    Iolet(Object* parent, bool isInlet);
    ~Iolet() override;

    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    Iolet* getNextIolet();

    bool hitTest(int x, int y) override;

    void render(NVGcontext* nvg) override;

    void valueChanged(Value& v) override;
    void settingsChanged(String const& name, var const& value) override;

    static Iolet* findNearestIolet(Canvas* cnv, Point<int> position, bool inlet, Object const* boxToExclude = nullptr);

    void createConnection();

    void setHidden(bool hidden);

    SmallArray<Connection*> getConnections() const;

    Rectangle<int> getCanvasBounds() const;

    uint16 ioletIdx;
    bool isInlet : 1;
    bool isSignal : 1;
    bool isGemState : 1;
    bool isTargeted : 1 = false;

private:
    bool const insideGraph : 1;
    bool isSymbolIolet : 1 = false;
    bool locked : 1 = false;
    bool commandLocked : 1 = false;
    bool presentationMode : 1 = false;
    bool patchDownwardsOnly : 1 = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Iolet)
    JUCE_DECLARE_WEAK_REFERENCEABLE(Iolet)
};
