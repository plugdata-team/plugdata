#pragma once

class GlobalMouseListener : public MouseListener {
    Component* target;

public:
    explicit GlobalMouseListener(Component* targetComponent = nullptr)
        : target(targetComponent)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }

    ~GlobalMouseListener() override
    {
        Desktop::getInstance().removeGlobalMouseListener(this);
    }

    std::function<void(MouseEvent const& e)> globalMouseDown = [](MouseEvent const&) { };
    std::function<void(MouseEvent const& e)> globalMouseUp = [](MouseEvent const&) { };
    std::function<void(MouseEvent const& e)> globalMouseDrag = [](MouseEvent const&) { };
    std::function<void(MouseEvent const& e)> globalMouseMove = [](MouseEvent const&) { };

    void mouseDown(MouseEvent const& e) override
    {
        globalMouseDown(target ? e.getEventRelativeTo(target) : e);
    }

    void mouseUp(MouseEvent const& e) override
    {
        globalMouseUp(target ? e.getEventRelativeTo(target) : e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        globalMouseDrag(target ? e.getEventRelativeTo(target) : e);
    }

    void mouseMove(MouseEvent const& e) override
    {
        globalMouseMove(target ? e.getEventRelativeTo(target) : e);
    }
};
