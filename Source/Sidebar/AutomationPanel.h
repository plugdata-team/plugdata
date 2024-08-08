/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"
#include "Object.h"
#include "Utility/PluginParameter.h"
#include "Components/DraggableNumber.h"
#include "Components/Buttons.h"
#include "Components/PropertiesPanel.h"
#include "Components/ObjectDragAndDrop.h"

class AutomationItem : public ObjectDragAndDrop
    , public Value::Listener {

    class ExpandButton : public TextButton {
        void paint(Graphics& g) override
        {
            auto isOpen = getToggleState();
            auto mouseOver = isMouseOver();
            auto area = getLocalBounds().reduced(5).toFloat();

            Path p;
            p.startNewSubPath(0.0f, 0.0f);
            p.lineTo(0.5f, 0.5f);
            p.lineTo(isOpen ? 1.0f : 0.0f, isOpen ? 0.0f : 1.0f);

            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(mouseOver ? 0.7f : 1.0f));
            g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(area.translated(3, 0).reduced(area.getWidth() / 4, area.getHeight() / 4), true));
        }
    };

    PluginProcessor* pd;

public:
    AutomationItem(PlugDataParameter* parameter, Component* parentComponent, PluginProcessor* processor)
        : ObjectDragAndDrop(parentComponent->findParentComponentOfClass<PluginEditor>())
        , pd(processor)
        , rangeProperty("Range", range, false)
        , modeProperty("Mode", mode, { "Float", "Integer", "Logarithmic", "Exponential" })
        , param(parameter)
    {
        addMouseListener(parentComponent, true);

        addChildComponent(rangeProperty);
        addChildComponent(modeProperty);

        range.addListener(this);
        mode.addListener(this);

        deleteButton.onClick = [this]() mutable {
            onDelete(this);
        };

        nameLabel.setFont(Font(14));
        nameLabel.setInterceptsMouseClicks(false, false);

        deleteButton.setSize(25, 25);
        reorderButton.setSize(25, 25);

        deleteButton.addMouseListener(this, false);
        reorderButton.addMouseListener(this, false);

        nameLabel.setTooltip("Drag to add [param] to canvas");
        deleteButton.setTooltip("Remove parameter");
        settingsButton.setTooltip("Expand settings");

        settingsButton.onClick = [this]() mutable {
            bool toggleState = settingsButton.getToggleState();

            rangeProperty.setVisible(toggleState);
            modeProperty.setVisible(toggleState);

            getParentComponent()->resized();
        };

        auto& minimumComponent = rangeProperty.getMinimumComponent();
        auto& maximumComponent = rangeProperty.getMaximumComponent();

        minimumComponent.dragEnd = [this, &maximumComponent]() {
            double minimum = static_cast<int>((*range.getValue().getArray())[0]);
            double maximum = param->getNormalisableRange().end;

            valueLabel.setMinimum(minimum);
            valueLabel.setMaximum(maximum);
            valueLabel.setValue(std::clamp(valueLabel.getValue(), minimum, maximum));

            maximumComponent.setMinimum(minimum + 0.000001);

            // make sure min is always smaller than max
            minimum = std::min(minimum, maximum - 0.000001);

            param->setRange(minimum, maximum);
            param->notifyDAW();
            update();
        };

        maximumComponent.dragEnd = [this, &minimumComponent]() {
            double minimum = param->getNormalisableRange().start;
            double maximum = static_cast<int>((*range.getValue().getArray())[1]);

            valueLabel.setMinimum(minimum);
            valueLabel.setMaximum(maximum);
            valueLabel.setValue(std::clamp(valueLabel.getValue(), minimum, maximum));

            minimumComponent.setMaximum(maximum);

            // make sure max is always bigger than min
            maximum = std::max(maximum, minimum + 0.000001);

            param->setRange(minimum, maximum);
            param->notifyDAW();
            update();
        };

        slider.setScrollWheelEnabled(false);
        slider.setTextBoxStyle(Slider::NoTextBox, false, 45, 13);

        if (ProjectInfo::isStandalone) {
            valueLabel.setText(String(param->getUnscaledValue(), 2), dontSendNotification);
            slider.setValue(param->getUnscaledValue(), dontSendNotification);
            slider.onValueChange = [this]() mutable {
                float value = slider.getValue();
                param->setUnscaledValueNotifyingHost(value);
                valueLabel.setText(String(value, 2), dontSendNotification);
            };
        } else {
            slider.onValueChange = [this]() mutable {
                float value = slider.getValue();
                valueLabel.setText(String(value, 2), dontSendNotification);
            };

            attachment = std::make_unique<SliderParameterAttachment>(*param, slider, nullptr);
            valueLabel.setText(String(param->getValue(), 2), dontSendNotification);
        }

        valueLabel.onValueChange = [this](float newValue) mutable {
            auto minimum = param->getNormalisableRange().start;
            auto maximum = param->getNormalisableRange().end;

            auto value = std::clamp(newValue, minimum, maximum);

            valueLabel.setText(String(value, 2), dontSendNotification);
            param->setUnscaledValueNotifyingHost(value);
            slider.setValue(value, dontSendNotification);
        };

        valueLabel.setMinimumHorizontalScale(1.0f);
        valueLabel.setJustificationType(Justification::centred);

        nameLabel.setMinimumHorizontalScale(1.0f);
        nameLabel.setJustificationType(Justification::centred);

        valueLabel.setEditable(true);

        settingsButton.setClickingTogglesState(true);

        nameLabel.onEditorShow = [this]() {
            if (auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
                editor->setJustification(Justification::centred);
                editor->setInputRestrictions(32, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-");
            }
            lastName = nameLabel.getText(false);
        };

        nameLabel.onTextChange = [this]() {
            resetDragAndDropImage();
        };

        nameLabel.onEditorHide = [this]() {
            StringArray allNames;
            for (auto* param : pd->getParameters()) {
                allNames.add(dynamic_cast<PlugDataParameter*>(param)->getTitle());
            }

            auto newName = nameLabel.getText(true);
            auto character = newName[0];

            // Check if name is valid
            if ((character == '_' || character == '-'
                    || (character >= 'a' && character <= 'z')
                    || (character >= 'A' && character <= 'Z'))
                && newName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-") && !allNames.contains(newName) && newName.isNotEmpty()) {
                param->setName(nameLabel.getText(true));
                param->notifyDAW();
            } else {
                nameLabel.setText(lastName, dontSendNotification);
            }
        };

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(slider);
        addAndMakeVisible(valueLabel);

        if (PlugDataParameter::canDynamicallyAdjustParameters()) {
            addAndMakeVisible(settingsButton);
        }

        addChildComponent(reorderButton);
        addChildComponent(deleteButton);

        update();
    }

    void update()
    {
        lastName = param->getTitle();
        nameLabel.setText(lastName, dontSendNotification);

        auto normalisableRange = param->getNormalisableRange();

        auto& min = normalisableRange.start;
        auto& max = normalisableRange.end;

        range = Array<var> { min, max };

        if (normalisableRange.skew == 4.0f) {
            mode = PlugDataParameter::Logarithmic;
        } else if (normalisableRange.skew == 0.25f) {
            mode = PlugDataParameter::Exponential;
        } else {
            mode = normalisableRange.interval == 1.0f ? PlugDataParameter::Integer : PlugDataParameter::Float;
        }

        if (mode == PlugDataParameter::Integer) {
            valueLabel.setDragMode(DraggableNumber::Integer);
            rangeProperty.getMinimumComponent().setDragMode(DraggableNumber::Integer);
            rangeProperty.getMaximumComponent().setDragMode(DraggableNumber::Integer);
        } else {
            valueLabel.setDragMode(DraggableNumber::Regular);
            rangeProperty.getMinimumComponent().setDragMode(DraggableNumber::Regular);
            rangeProperty.getMaximumComponent().setDragMode(DraggableNumber::Regular);
        }

        valueLabel.setMinimum(min);
        valueLabel.setMaximum(max);

        rangeProperty.getMaximumComponent().setMinimum(min + 0.000001f);
        rangeProperty.getMinimumComponent().setMaximum(max);

        auto doubleRange = NormalisableRange<double>(normalisableRange.start, normalisableRange.end, normalisableRange.interval, normalisableRange.skew);

        if (ProjectInfo::isStandalone) {
            slider.setValue(param->getUnscaledValue());
            slider.setNormalisableRange(doubleRange);
            valueLabel.setText(String(param->getUnscaledValue(), 2), dontSendNotification);
        } else {
            slider.setNormalisableRange(doubleRange);
        }
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().toFloat().reduced(4.5f, 3.0f).contains(x, y);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (&reorderButton == e.originalComponent)
            setIsReordering(true);
        else
            setIsReordering(false);
    }

    void mouseUp(MouseEvent const& e) override
    {
        bool isEditable = PlugDataParameter::canDynamicallyAdjustParameters();
        bool isOverNameLable = nameLabel.getBounds().contains(e.getEventRelativeTo(&nameLabel).getPosition());

        if (e.mods.isRightButtonDown() && !ProjectInfo::isStandalone) {
            auto* pluginEditor = findParentComponentOfClass<PluginEditor>();
            if (auto* hostContext = pluginEditor->getHostContext()) {
                hostContextMenu = hostContext->getContextMenuForParameter(param);
                if (hostContextMenu) {
                    auto menuPosition = pluginEditor->getMouseXYRelative();
                    hostContextMenu->showNativeMenu(menuPosition);
                }
            }
            return;
        }

        if (isEditable && isOverNameLable && !e.mouseWasDraggedSinceMouseDown() && e.getNumberOfClicks() >= 2) {
            nameLabel.showEditor();
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        // Make sure this isn't coming from the listened object
        deleteButton.setVisible(true);
        reorderButton.setVisible(true);
    }

    void mouseExit(MouseEvent const& e) override
    {
        deleteButton.setVisible(false);
        reorderButton.setVisible(false);
    }

    String getObjectString() override
    {
        return "#X obj 0 0 param " + param->getTitle() + ";";
    }

    String getPatchStringName() override
    {
        return "param object";
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(range)) {
            auto min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(range.getValue().getArray()->getReference(1));
            param->setRange(min, max);
            update();
        } else if (v.refersToSameSourceAs(mode)) {
            param->setMode(static_cast<PlugDataParameter::Mode>(getValue<int>(mode)));
            update();
        }
    }

    int getItemHeight()
    {
        if (param->isEnabled()) {
            return settingsButton.getToggleState() ? 110.0f : 56.0f;
        } else {
            return 0.0f;
        }
    }

    // TOOD: hides non-virtual function!
    bool isEnabled() const
    {
        return param->isEnabled();
    }

    void resized() override
    {
        bool settingsVisible = settingsButton.getToggleState();

        auto bounds = getLocalBounds().reduced(6, 2);

        int rowHeight = 26;

        auto firstRow = bounds.removeFromTop(rowHeight);
        auto secondRow = bounds.removeFromTop(rowHeight);

        if (settingsVisible) {

            rangeProperty.setBounds(bounds.removeFromTop(rowHeight));
            modeProperty.setBounds(bounds.removeFromTop(rowHeight));
        }

        nameLabel.setBounds(firstRow.reduced(25, 0));
        auto componentCentre = firstRow.getCentre().getY();
        reorderButton.setCentrePosition(firstRow.getTopLeft().getX() + 12, componentCentre);
        deleteButton.setCentrePosition(firstRow.getRight() - 12, componentCentre);

        if (settingsButton.isVisible()) {
            settingsButton.setBounds(secondRow.removeFromLeft(25));
        }

        slider.setBounds(secondRow.withTrimmedRight(55));
        valueLabel.setBounds(secondRow.removeFromRight(55));
    }

    void paint(Graphics& g) override
    {
        slider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
        slider.setColour(Slider::trackColourId, findColour(PlugDataColour::sidebarTextColourId));

        nameLabel.setColour(Label::textColourId, findColour(PlugDataColour::sidebarTextColourId));
        valueLabel.setColour(Label::textColourId, findColour(PlugDataColour::sidebarTextColourId));

        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f, 3.0f), Corners::defaultCornerRadius);
    }

    std::function<void(AutomationItem*)> onDelete = [](AutomationItem*) {};
    std::unique_ptr<HostProvidedContextMenu> hostContextMenu;

    SmallIconButton deleteButton = SmallIconButton(Icons::Clear);
    ExpandButton settingsButton;

    Value range = Value(var(Array<var> { var(0.0f), var(127.0f) }));
    Value mode = Value(var(PlugDataParameter::Float));

    PropertiesPanel::RangeComponent rangeProperty;
    PropertiesPanel::ComboComponent modeProperty;

    DraggableNumber valueLabel = DraggableNumber(false);

    Label nameLabel;

    String lastName;

    Slider slider;

    ReorderButton reorderButton;

    PlugDataParameter* param;

    std::unique_ptr<SliderParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationItem)
};

class AlphaAnimator : public Timer {
public:
    AlphaAnimator() = default;

    void fadeIn(Component* component, double duration)
    {
        animate(component, duration, 0.0f, 1.0f);
    }

    void fadeOut(Component* component, double duration)
    {
        animate(component, duration, 1.0f, 0.0f);
    }

private:
    void animate(Component* component, double duration, float startAlpha, float endAlpha)
    {
        componentToAnimate = component;
        animationSteps = static_cast<int>(duration / timerInterval);
        currentStep = 0;
        startAlphaValue = startAlpha;
        endAlphaValue = endAlpha;

        startTimer(timerInterval); // Start the timer
    }

    void timerCallback() override
    {
        ++currentStep;

        if (currentStep >= animationSteps) {
            componentToAnimate->setAlpha(endAlphaValue);
            stopTimer();
        } else {
            float alpha = startAlphaValue + (endAlphaValue - startAlphaValue) * static_cast<float>(currentStep) / static_cast<float>(animationSteps);
            componentToAnimate->setAlpha(alpha);
        }
    }

    Component* componentToAnimate = nullptr;
    int animationSteps = 0;
    int currentStep = 0;
    float startAlphaValue = 0.0f;
    float endAlphaValue = 0.0f;

    int const timerInterval = 16; // 60 FPS timer interval

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlphaAnimator)
};

class DraggedItemDropShadow : public Component
    , public ComponentListener {
public:
    DraggedItemDropShadow()
    {
        setAlpha(0.0f);
    }

    void activate(AutomationItem* item)
    {
        if (automationItem != item) {
            if (automationItem)
                automationItem->removeComponentListener(this);
            automationItem = item;
        }

        toFront(false);
        automationItem->addComponentListener(this);
        setBounds(automationItem->getBounds().expanded(8, 4));
        animator.fadeIn(this, 300);
    }

    void deActivate()
    {
        animator.fadeOut(this, 300);
    }

    void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override
    {
        if (&component == automationItem) {
            setBounds(component.getBounds().expanded(8, 4));
        }
    }

    void paint(Graphics& g) override
    {
        auto rect = getLocalBounds().reduced(14, 7);
        Path shadowPath;
        shadowPath.addRoundedRectangle(rect, Corners::defaultCornerRadius);
        StackShadow::renderDropShadow(g, shadowPath, Colours::black.withAlpha(0.3f), 7);
    }

private:
    SafePointer<AutomationItem> automationItem = nullptr;
    AlphaAnimator animator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DraggedItemDropShadow)
};

class AutomationComponent : public Component {

    class AddParameterButton : public Component {

        bool mouseIsOver = false;

    public:
        std::function<void()> onClick = []() {};

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().reduced(5, 2);
            auto textBounds = bounds;
            auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto colour = findColour(PlugDataColour::sidebarTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
            }

            Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
            Fonts::drawText(g, "Add new parameter", textBounds, colour, 14);
        }

        bool hitTest(int x, int y) override
        {
            if (getLocalBounds().reduced(5, 2).contains(x, y)) {
                return true;
            }
            return false;
        }

        void mouseEnter(MouseEvent const& e) override
        {
            mouseIsOver = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            mouseIsOver = false;
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            onClick();
        }
    };

public:
    explicit AutomationComponent(PluginProcessor* processor, Component* parent)
        : pd(processor)
        , parentComponent(parent)
    {
        updateSliders();

        addAndMakeVisible(addParameterButton);
        addAndMakeVisible(draggedItemDropShadow);

        addParameterButton.onClick = [this, parent]() {
            for (auto* param : getParameters()) {
                if (!param->isEnabled()) {
                    param->setEnabled(true);
                    param->setName(getNewParameterName());
                    param->setIndex(rows.size());
                    param->notifyDAW();
                    break;
                }
            }

            resized();
            parent->resized();
            updateSliders();
        };
    }

    void mouseDown(MouseEvent const& e) override
    {
        accumulatedOffsetY = { 0, 0 };

        if (auto* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
            draggedItem = static_cast<AutomationItem*>(reorderButton->getParentComponent());
            draggedItemDropShadow.activate(draggedItem);
            draggedItem->toFront(false);
            mouseDownPos = draggedItem->getPosition();
            draggedItem->deleteButton.setVisible(false);
            draggedItem->reorderButton.setVisible(false);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (draggedItem) {
            for (int p = 0; p < rows.size(); p++) {
                rows[p]->param->setIndex(p);
            }
            draggedItem = nullptr;
            shouldAnimate = true;
            draggedItemDropShadow.deActivate();
            resized();
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!draggedItem || std::abs(e.getDistanceFromDragStart()) < 5)
            return;

        // autoscroll the viewport when we are close. to. the. edge.
        auto viewport = findParentComponentOfClass<BouncingViewport>();
        if (viewport->autoScroll(0, viewport->getLocalPoint(nullptr, e.getScreenPosition()).getY(), 0, 5)) {
            beginDragAutoRepeat(20);
        }

        auto dragPos = mouseDownPos.translated(0, e.getDistanceFromDragStartY());
        auto autoScrollOffset = Point<int>(0, viewportPosY - viewport->getViewPositionY());
        accumulatedOffsetY += autoScrollOffset;
        draggedItem->setTopLeftPosition(dragPos - accumulatedOffsetY);
        viewportPosY -= autoScrollOffset.getY();

        int idx = rows.indexOf(draggedItem);
        if (idx > 0 && draggedItem->getBounds().getCentreY() < rows[idx - 1]->getBounds().getCentreY() && rows[idx - 1]->isEnabled()) {
            rows.swap(idx, idx - 1);
            shouldAnimate = true;
            resized();
        } else if (idx < rows.size() - 1 && draggedItem->getBounds().getCentreY() > rows[idx + 1]->getBounds().getCentreY() && rows[idx + 1]->isEnabled()) {
            rows.swap(idx, idx + 1);
            shouldAnimate = true;
            resized();
        }
    }

    String getNewParameterName()
    {
        StringArray takenNames;
        for (auto* row : rows) {
            if (row->isEnabled()) {
                takenNames.add(row->param->getTitle());
            }
        }

        auto newParamName = String("param");
        int i = 1;
        while (takenNames.contains(newParamName + String(i))) {
            i++;
        }

        return newParamName + String(i);
    }

    Array<PlugDataParameter*> getParameters()
    {
        Array<PlugDataParameter*> params;
        for (auto* param : pd->getParameters())
            params.add(dynamic_cast<PlugDataParameter*>(param));

        params.remove(0);

        return params;
    }

    void updateSliders()
    {
        rows.clear();

        for (auto* param : getParameters()) {
            if (param->isEnabled()) {
                auto* slider = rows.add(new AutomationItem(param, parentComponent, pd));
                addAndMakeVisible(slider);

                slider->reorderButton.addMouseListener(this, false);

                slider->onDelete = [this](AutomationItem* toDelete) {
                    StringArray paramNames;

                    for (auto* param : getParameters()) {
                        if (param != toDelete->param) {
                            paramNames.add(param->getTitle());
                        }
                    }

                    auto toDeleteIdx = rows.indexOf(toDelete);
                    for (int i = toDeleteIdx; i < rows.size(); i++) {
                        rows[i]->param->setIndex(rows[i]->param->getIndex() - 1);
                    }

                    auto newParamName = String("param");
                    int i = 1;
                    while (paramNames.contains(newParamName + String(i))) {
                        i++;
                    }
                    newParamName += String(i);

                    toDelete->param->setEnabled(false);
                    toDelete->param->setName(newParamName);
                    toDelete->param->setValue(0.0f);
                    toDelete->param->setRange(0.0f, 1.0f);
                    toDelete->param->setMode(PlugDataParameter::Float);
                    toDelete->param->notifyDAW();

                    updateSliders();
                };
            }
        }

        std::sort(rows.begin(), rows.end(), [](auto* a, auto* b) {
            return a->param->getIndex() < b->param->getIndex();
        });

        addParameterButton.toFront(false);

        checkMaxNumParameters();
        parentComponent->resized();
        resized();
    }

    void checkMaxNumParameters()
    {
        addParameterButton.setVisible(rows.size() < PluginProcessor::numParameters);
    }

    void resized() override
    {
        auto& animator = Desktop::getInstance().getAnimator();

        int y = 2;
        int width = getWidth();
        for (int p = 0; p < rows.size(); p++) {
            int height = rows[p]->getItemHeight();
            if (rows[p] != draggedItem) {
                auto bounds = Rectangle<int>(0, y, width, height);
                if (shouldAnimate) {
                    animator.animateComponent(rows[p], bounds, 1.0f, 200, false, 3.0f, 0.0f);
                } else {
                    animator.cancelAnimation(rows[p], false);
                    rows[p]->setBounds(bounds);
                }
            }
            y += height;
        }

        shouldAnimate = false;
        addParameterButton.setBounds(0, y, getWidth(), 28);
    }

    int getTotalHeight() const
    {
        int y = 30;
        for (int p = 0; p < rows.size(); p++) {
            y += rows[p]->getItemHeight();
        }

        return y;
    }

    SafePointer<AutomationItem> draggedItem;
    DraggedItemDropShadow draggedItemDropShadow;
    Point<int> mouseDownPos;
    Point<int> accumulatedOffsetY = { 0, 0 };
    int viewportPosY = 0;

    bool shouldAnimate = false;

    PluginProcessor* pd;
    Component* parentComponent;
    OwnedArray<AutomationItem> rows;
    AddParameterButton addParameterButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationComponent)
};

class AutomationPanel : public Component
    , public ScrollBar::Listener
    , public AsyncUpdater {

public:
    explicit AutomationPanel(PluginProcessor* processor)
        : sliders(processor, this)
        , pd(processor)
    {
        viewport.setViewedComponent(&sliders, false);
        viewport.setScrollBarsShown(true, false, false, false);

        viewport.getVerticalScrollBar().addListener(this);

        setWantsKeyboardFocus(false);
        viewport.setWantsKeyboardFocus(false);

        addAndMakeVisible(viewport);
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(getLocalBounds());
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds());

        sliders.setSize(getWidth(), std::max(sliders.getTotalHeight(), viewport.getMaximumVisibleHeight()));
    }

    void updateParameterValue(PlugDataParameter* changedParameter)
    {
        for (int p = 0; p < sliders.rows.size(); p++) {
            auto* param = sliders.rows[p]->param;
            auto& slider = sliders.rows[p]->slider;
            if (changedParameter == param && slider.getThumbBeingDragged() == -1) {
                slider.setValue(param->getUnscaledValue());
                break;
            }
        }
    }

    void handleAsyncUpdate() override
    {
        sliders.updateSliders();
    }
    BouncingViewport viewport;
    AutomationComponent sliders;
    PluginProcessor* pd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationPanel)
};
