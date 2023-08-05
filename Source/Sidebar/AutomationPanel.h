/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"
#include "Object.h"
#include "Utility/PluginParameter.h"
#include "Utility/DraggableNumber.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/ReorderButton.h"
#include "Utility/PropertiesPanel.h"

class AutomationSlider : public Component
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
    AutomationSlider(int idx, Component* parentComponent, PluginProcessor* processor)
        : index(idx)
        , pd(processor)
        , rangeProperty("Range", range, false)
        , modeProperty("Mode", mode, {"Float", "Integer", "Logarithmic", "Exponential"})
    {
        param = dynamic_cast<PlugDataParameter*>(pd->getParameters()[index + 1]);

        addChildComponent(rangeProperty);
        addChildComponent(modeProperty);
        
        range.addListener(this);
        mode.addListener(this);
        
        deleteButton.onClick = [this]() mutable {
            onDelete(this);
        };

        nameLabel.setFont(Font(14));
        nameLabel.addMouseListener(this, false);
        deleteButton.addMouseListener(this, false);
        reorderButton.addMouseListener(this, false);

        nameLabel.setTooltip("Drag to add [param] to canvas");
        deleteButton.setTooltip("Remove parameter");
        settingsButton.setTooltip("Expand settings");

        settingsButton.onClick = [this, parentComponent]() mutable {
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
        nameLabel.setJustificationType(Justification::centredLeft);

        valueLabel.setEditable(true);

        settingsButton.setClickingTogglesState(true);

        bool editable = PlugDataParameter::canDynamicallyAdjustParameters();
        nameLabel.setEditable(editable, editable);
        nameLabel.onEditorShow = [this]() {
            if (auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setInputRestrictions(32, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-");
            }
            lastName = nameLabel.getText(false);
        };

        nameLabel.onTextChange = [this]() {
            dragImage.image = Image();
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

        settingsButton.getProperties().set("Style", "SmallIcon");
        deleteButton.getProperties().set("Style", "SmallIcon");

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(slider);
        addAndMakeVisible(valueLabel);
        
        if(PlugDataParameter::canDynamicallyAdjustParameters())
        {
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
        
        range = Array<var>{min, max};
        
        if(normalisableRange.skew == 4.0f)
        {
            mode = PlugDataParameter::Logarithmic;
        }
        else if(normalisableRange.skew == 0.25f)
        {
            mode = PlugDataParameter::Exponential;
        }
        else {
            mode = normalisableRange.interval == 1.0f ? PlugDataParameter::Integer : PlugDataParameter::Float;
        }
        
        if(mode == PlugDataParameter::Integer)
        {
            valueLabel.setDragMode(DraggableNumber::Integer);
            rangeProperty.getMinimumComponent().setDragMode(DraggableNumber::Integer);
            rangeProperty.getMaximumComponent().setDragMode(DraggableNumber::Integer);
        }
        else {
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

    void lookAndFeelChanged() override
    {
        dragImage.image = Image();
    }

    bool hitTest(int x, int y) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.5f, 3.0f);
        if (bounds.contains(x, y)) {
            return true;
        }
        return false;
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

    void mouseDrag(MouseEvent const& e) override
    {
        if(e.originalComponent == &reorderButton || e.getDistanceFromDragStart() < 5) return;
        
        auto formatedParam = "#X obj 0 0 param " + param->getTitle() + ";";

        deleteButton.setVisible(false);

        if (dragImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToTempImage(formatedParam);
        }

        auto dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        Array<var> paramObjectWithOffset;
        paramObjectWithOffset.add(var(dragImage.offset.getX()));
        paramObjectWithOffset.add(var(dragImage.offset.getY()));
        paramObjectWithOffset.add(var(formatedParam));
        dragContainer->startDragging(paramObjectWithOffset, this, dragImage.image, true, nullptr, nullptr, true);
    }

    void valueChanged(Value& v) override
    {
        if(v.refersToSameSourceAs(range))
        {
            auto min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(range.getValue().getArray()->getReference(1));
            param->setRange(min, max);
            update();
        }
        else if(v.refersToSameSourceAs(mode))
        {
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

    bool isEnabled()
    {
        return param->isEnabled();
    }

    void setEnabled(bool shouldBeEnabled)
    {
        param->setEnabled(shouldBeEnabled);
        param->notifyDAW();
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

        auto buttonsBounds = firstRow.removeFromRight(50).withHeight(25);

        nameLabel.setBounds(firstRow.withTrimmedLeft(4));
        
        if(settingsButton.isVisible()) {
            settingsButton.setBounds(secondRow.removeFromLeft(25));
        }

        slider.setBounds(secondRow.withTrimmedRight(55));
        valueLabel.setBounds(secondRow.removeFromRight(55));

        reorderButton.setBounds(buttonsBounds.removeFromLeft(25));
        deleteButton.setBounds(buttonsBounds.removeFromLeft(25));

    }

    void paint(Graphics& g) override
    {
        slider.setColour(Slider::backgroundColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
        slider.setColour(Slider::trackColourId, findColour(PlugDataColour::sidebarTextColourId));

        nameLabel.setColour(Label::textColourId, findColour(PlugDataColour::sidebarTextColourId));
        valueLabel.setColour(Label::textColourId, findColour(PlugDataColour::sidebarTextColourId));

        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId).withAlpha(0.65f));
        PlugDataLook::fillSmoothedRectangle(g, getLocalBounds().toFloat().reduced(6.0f, 3.0f), Corners::defaultCornerRadius);
    }

    std::function<void(AutomationSlider*)> onDelete = [](AutomationSlider*) {};

    TextButton deleteButton = TextButton(Icons::Clear);
    ExpandButton settingsButton;

    Value range = Value(var(Array<var>{var(0.0f), var(127.0f)}));
    Value mode = Value(var(PlugDataParameter::Float));
        
    PropertiesPanel::RangeComponent rangeProperty;
    PropertiesPanel::ComboComponent modeProperty;
        
    DraggableNumber valueLabel = DraggableNumber(false);

    Label nameLabel;

    String lastName;

    Slider slider;

    ImageWithOffset dragImage;
    ReorderButton reorderButton;

    int index;

    PlugDataParameter* param;
        
    std::unique_ptr<SliderParameterAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationSlider)
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
                PlugDataLook::fillSmoothedRectangle(g, bounds.toFloat(), Corners::defaultCornerRadius);

                colour = findColour(PlugDataColour::sidebarActiveTextColourId);
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

        addParameterButton.onClick = [this, parent]() {
            for (auto* row : rows) {
                if (!row->isEnabled()) {
                    row->setEnabled(true);
                    break;
                }
            }

            resized();
            parent->resized();

            for (auto* row : rows) {
                row->resized();
                row->repaint();
            }

            checkMaxNumParameters();
        };
    }
    
    void mouseDown(MouseEvent const& e) override
    {
        accumulatedOffsetY = { 0, 0 };
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (std::abs(e.getDistanceFromDragStart()) < 5 && !isDragging/* || isDnD*/)
            return;

        isDragging = true;

        if (!draggedItem) {
            if (auto* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
                draggedItem = static_cast<AutomationSlider*>(reorderButton->getParentComponent());
                draggedItem->toFront(false);
                mouseDownPos = draggedItem->getPosition();
                draggedItem->deleteButton.setVisible(false);
            }
        } else {
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
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        if (draggedItem) {
            for (int p = 0; p < PluginProcessor::numParameters; p++) {
                rows[p]->param->setIndex(p);
            }
            isDragging = false;
            draggedItem = nullptr;
            shouldAnimate = true;
            resized();
        }
    }
    
    void updateSliders()
    {
        rows.clear();

        for (int p = 0; p < PluginProcessor::numParameters; p++) {
            auto* slider = rows.add(new AutomationSlider(p, parentComponent, pd));
            addAndMakeVisible(slider);
            
            // TODO: is this safe? Do we need to clear it?
            slider->reorderButton.addMouseListener(this, false);

            slider->onDelete = [this](AutomationSlider* toDelete) {
                std::vector<std::tuple<bool, String, float, float, float>> parameterValues;

                StringArray paramNames;

                for (int i = 0; i < rows.size(); i++) {
                    auto* param = dynamic_cast<PlugDataParameter*>(pd->getParameters()[i + 1]);

                    parameterValues.emplace_back(param->isEnabled(), param->getTitle(), param->getUnscaledValue(), param->getNormalisableRange().start, param->getNormalisableRange().end);

                    paramNames.add(param->getTitle());
                }

                auto toDeleteIdx = rows.indexOf(toDelete);

                auto newParamName = String("param");
                int i = 1;
                while (paramNames.contains(newParamName + String(i))) {
                    i++;
                }
                newParamName += String(i);

                parameterValues.erase(parameterValues.begin() + toDeleteIdx);
                parameterValues.emplace_back(false, newParamName, 0.0f, 0.0f, 1.0f);

                for (int i = 0; i < rows.size(); i++) {
                    auto* param = dynamic_cast<PlugDataParameter*>(pd->getParameters()[i + 1]);

                    auto& [enabled, name, value, min, max] = parameterValues[i];

                    param->setEnabled(enabled);
                    param->setName(name);
                    param->setRange(min, max);
                    param->setUnscaledValueNotifyingHost(value);
                }

                dynamic_cast<PlugDataParameter*>(pd->getParameters()[0])->notifyDAW();

                updateSliders();
            };
        }
        
        std::sort(rows.begin(), rows.end(), [](auto* a, auto* b){
            return a->param->getIndex() < b->param->getIndex();
        });
        
        checkMaxNumParameters();
        parentComponent->resized();
        resized();
    }

    void checkMaxNumParameters()
    {
        addParameterButton.setVisible(getNumEnabled() < PluginProcessor::numParameters);
    }

    void resized() override
    {
        auto& animator = Desktop::getInstance().getAnimator();

        int y = 2;
        int width = getWidth();
        for (int p = 0; p < PluginProcessor::numParameters; p++) {
            if(rows[p]->isEnabled()) {
                int height = rows[p]->getItemHeight();
                if(rows[p] != draggedItem) {
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
        }

        shouldAnimate = false;
        addParameterButton.setBounds(0, y, getWidth(), 28);
    }

    int getNumEnabled() const
    {
        int numEnabled = 0;

        for (int p = 0; p < PluginProcessor::numParameters; p++) {
            numEnabled += rows[p]->isEnabled();
        }

        return numEnabled;
    }

    int getTotalHeight() const
    {
        int y = 28;
        for (int p = 0; p < PluginProcessor::numParameters; p++) {
            y += rows[p]->getItemHeight();
        }

        return y;
    }

    bool isDragging = false;
    SafePointer<AutomationSlider> draggedItem;
    Point<int> mouseDownPos;
    Point<int> accumulatedOffsetY = {0, 0};
    int viewportPosY;
    
    bool shouldAnimate = false;
    
    PluginProcessor* pd;
    Component* parentComponent;
    OwnedArray<AutomationSlider> rows;
    AddParameterButton addParameterButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationComponent)
};

class AutomationPanel : public Component
    , public ScrollBar::Listener {

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

    void updateParameters()
    {
        if (ProjectInfo::isStandalone) {
            
            sliders.updateSliders();
            
            for (int p = 0; p < PluginProcessor::numParameters; p++) {
                auto* param = dynamic_cast<PlugDataParameter*>(pd->getParameters()[p + 1]);
                sliders.rows[p]->slider.setValue(param->getUnscaledValue());
            }
            
        } else {
            sliders.updateSliders();
        }
        
    }
    BouncingViewport viewport;
    AutomationComponent sliders;
    PluginProcessor* pd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationPanel)
};
