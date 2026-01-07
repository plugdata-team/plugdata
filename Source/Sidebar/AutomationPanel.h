/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Canvas.h"
#include "Object.h"
#include "Utility/PluginParameter.h"
#include "Components/DraggableNumber.h"
#include "Components/Buttons.h"
#include "Components/PropertiesPanel.h"
#include "Components/ObjectDragAndDrop.h"
#include "Objects/ObjectBase.h"

class AutomationItem final : public ObjectDragAndDrop
    , public Value::Listener {

    class ParamNameErrorCallout final : public Component {
        Label textLabel;
        Label textLabel2;

        std::function<void()> onDismiss = [] { };

    public:
        ParamNameErrorCallout(String const& text, std::function<void()> const& onDismiss)
            : textLabel("error title", text)
            , textLabel2("info", "(Click outside to dismiss)")
            , onDismiss(onDismiss)
        {
            addAndMakeVisible(&textLabel);
            addAndMakeVisible(&textLabel2);

            textLabel.setJustificationType(Justification::centred);
            textLabel2.setJustificationType(Justification::centred);

            auto const bestWidthText = textLabel.getFont().getStringWidth(textLabel.getText());
            auto const bestWidthErrorNameText = textLabel2.getFont().getStringWidth(textLabel2.getText());

            auto const bestWidth = jmax(bestWidthText, bestWidthErrorNameText);
            setSize(bestWidth + 8, 2 * 24);
        }

        ~ParamNameErrorCallout() override
        {
            onDismiss();
        }

        void resized() override
        {
            textLabel.setBounds(4, 4, getWidth() - 8, 16);
            textLabel2.setBounds(4, 28, getWidth() - 8, 16);
        }
    };

    class RenameAllOccurancesCallout final : public Component {
        TextButton confirmButton = TextButton("Yes");
        TextButton dismissButton = TextButton("No");
        Label textLabel = Label("RenameLabel", "Update param in open patches?");

    public:
        std::function<void()> onYes = [] { };
        std::function<void()> onNo = [] { };

        RenameAllOccurancesCallout()
        {
            setSize(190, 60);

            addAndMakeVisible(&confirmButton);
            addAndMakeVisible(&dismissButton);
            addAndMakeVisible(&textLabel);

            textLabel.setJustificationType(Justification::centred);

            confirmButton.onClick = [this] { onYes(); };
            dismissButton.onClick = [this] { onNo(); };
        }

        void resized() override
        {
            // Set the bounds for textLabel
            textLabel.setBounds(4, 4, getWidth() - 8, 16);

            // Calculate the width of the buttons
            int constexpr buttonWidth = 40;
            int constexpr buttonHeight = 20;

            // Calculate the horizontal spacing between the buttons
            int constexpr totalButtonWidth = buttonWidth * 2 + 10; // 10 pixels gap between buttons

            // Center position for the buttons
            int const centerX = (getWidth() - totalButtonWidth) / 2;

            // Set bounds for the buttons
            dismissButton.setBounds(centerX, 35, buttonWidth, buttonHeight);
            confirmButton.setBounds(centerX + buttonWidth + 10, 35, buttonWidth, buttonHeight);
        }
    };

    class ExpandButton final : public TextButton {
        void paint(Graphics& g) override
        {
            auto const isOpen = getToggleState();
            auto const mouseOver = isMouseOver();
            auto const area = getLocalBounds().reduced(5).toFloat();

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
            bool const toggleState = settingsButton.getToggleState();

            rangeProperty.setVisible(toggleState);
            modeProperty.setVisible(toggleState);

            if (auto* parent = getParentComponent()) {
                parent->resized();
            }
        };

        auto& minimumComponent = rangeProperty.getMinimumComponent();
        auto& maximumComponent = rangeProperty.getMaximumComponent();

        minimumComponent.dragEnd = [this, &maximumComponent] {
            double minimum = static_cast<int>((*range.getValue().getArray())[0]);
            double const maximum = param->getNormalisableRange().end;

            valueLabel.setMinimum(minimum);
            valueLabel.setMaximum(maximum);
            valueLabel.setValue(std::clamp(valueLabel.getValue(), minimum, maximum));

            maximumComponent.setMinimum(minimum + 0.000001f);

            // make sure min is always smaller than max
            minimum = std::min(minimum, maximum - 0.000001f);

            param->setRange(minimum, maximum);
            param->notifyDAW();
            update();
        };

        maximumComponent.dragEnd = [this, &minimumComponent] {
            double const minimum = param->getNormalisableRange().start;
            double maximum = static_cast<int>((*range.getValue().getArray())[1]);

            valueLabel.setMinimum(minimum);
            valueLabel.setMaximum(maximum);
            valueLabel.setValue(std::clamp(valueLabel.getValue(), minimum, maximum));

            minimumComponent.setMaximum(maximum);

            // make sure max is always bigger than min
            maximum = std::max(maximum, minimum + 0.000001f);

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
                float const value = slider.getValue();
                param->setUnscaledValueNotifyingHost(value);
                valueLabel.setText(String(value, 2), dontSendNotification);
            };
        } else {
            slider.onValueChange = [this]() mutable {
                float const value = slider.getValue();
                valueLabel.setText(String(value, 2), dontSendNotification);
            };

            attachment = std::make_unique<SliderParameterAttachment>(*param, slider, nullptr);
            valueLabel.setText(String(param->getValue(), 2), dontSendNotification);
        }

        valueLabel.onValueChange = [this](float const newValue) mutable {
            auto const minimum = param->getNormalisableRange().start;
            auto const maximum = param->getNormalisableRange().end;

            auto const value = std::clamp(newValue, minimum, maximum);

            valueLabel.setText(String(value, 2), dontSendNotification);
            param->setUnscaledValueNotifyingHost(value);
            slider.setValue(value, dontSendNotification);
        };

        valueLabel.setMinimumHorizontalScale(1.0f);

        nameLabel.setMinimumHorizontalScale(1.0f);
        nameLabel.setJustificationType(Justification::centred);

        valueLabel.setEditableOnClick(true);

        settingsButton.setClickingTogglesState(true);

        nameLabel.onEditorShow = [this] {
            if (auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
                editor->setJustification(Justification::centred);
            }
            lastName = nameLabel.getText(false);
        };

        nameLabel.onTextChange = [this] {
            resetDragAndDropImage();
        };

        nameLabel.onEditorHide = [this] {
            // If name hasn't been changed do nothing
            auto const newName = nameLabel.getText(true);
            if (lastName == newName)
                return;

            StringArray allNames;
            for (auto* param : pd->getParameters()) {
                allNames.add(dynamic_cast<PlugDataParameter*>(param)->getTitle().toString());
            }

            auto const character = newName[0];

            bool const startsWithCorrectChar = character == '_' || character == '-'
                || (character >= 'a' && character <= 'z')
                || (character >= 'A' && character <= 'Z');
            bool const correctCharacters = newName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-");
            bool const uniqueName = !allNames.contains(newName);
            bool const notEmptyName = newName.isNotEmpty();
            bool const tooLongName = newName.length() >= 127;

            // Check if name is valid
            if (startsWithCorrectChar && correctCharacters && uniqueName && notEmptyName) {
                param->setName(newName);

                auto findParamsWithLastName = [this, newName] {
                    SmallArray<pd::WeakReference> paramObjectsToChange;

                    pd->lockAudioThread();
                    // Find [param] object, and update it's param name to new name
                    for (auto* cnv = pd_getcanvaslist(); cnv; cnv = cnv->gl_next) {
                        std::function<void(t_glist*)> searchInsideCanvas = [&](t_glist* cnv) -> void {
                            for (t_gobj* y = cnv->gl_list; y; y = y->g_next) {
                                if (pd_class(&y->g_pd) == canvas_class) {
                                    auto const canvas = reinterpret_cast<t_canvas*>(y);
                                    if (String(canvas->gl_name->s_name) == "param.pd") {
                                        auto const binName = canvas->gl_obj.te_binbuf;
                                        t_atom* atoms;
                                        if (int const argc = binbuf_getnatom(binName); argc > 1) {
                                            atoms = binbuf_getvec(binName);
                                            if (atoms[1].a_type == A_SYMBOL) {
                                                if (String(atom_getsymbol(&atoms[1])->s_name) == lastName) {
                                                    paramObjectsToChange.add(pd::WeakReference(canvas, pd));
                                                }
                                            }
                                        }
                                    }
                                    // Yes, also search inside the param.pd - in case someone put a param inside that!
                                    searchInsideCanvas(canvas);
                                }
                            }
                        };
                        searchInsideCanvas(cnv);
                    }
                    pd->unlockAudioThread();

                    return paramObjectsToChange;
                };

                auto const paramsWithLastName = findParamsWithLastName();

                if (paramsWithLastName.size() == 0)
                    return;

                // Launch a dialog to ask if the user wishes to rename all occurrences in patch
                auto paramRenameDialog = std::make_unique<RenameAllOccurancesCallout>();
                auto* rawDialogPointer = paramRenameDialog.get();
                auto& callOutBox = CallOutBox::launchAsynchronously(std::move(paramRenameDialog), nameLabel.getScreenBounds(), nullptr);

                SafePointer<CallOutBox> callOutBoxSafePtr(&callOutBox);
                rawDialogPointer->onNo = [callOutBoxSafePtr] {
                    if (callOutBoxSafePtr)
                        callOutBoxSafePtr->dismiss();
                };

                rawDialogPointer->onYes = [this, paramsWithLastName, newName, callOutBoxSafePtr] {
                    auto const name = "param " + newName;
                    for (auto objReference : paramsWithLastName) {
                        if (auto obj = objReference.get<t_canvas>()) {
                            pd::Interface::renameObject(obj->gl_owner, &obj->gl_obj.te_g, name.toRawUTF8(), name.getNumBytesAsUTF8());
                        }
                    }

                    for (auto const& editor : pd->getEditors()) {
                        for (auto const canvas : editor->getCanvases()) {
                            canvas->synchronise();
                        }
                    }

                    if (callOutBoxSafePtr)
                        callOutBoxSafePtr->dismiss();
                };

                param->notifyDAW();
            } else {
                String errorText;
                if (!notEmptyName)
                    errorText = "Name can't be empty";
                else if (!uniqueName)
                    errorText = "Name is not unique";
                else if (!startsWithCorrectChar)
                    errorText = "Name can't start with spaces, numbers or symbols";
                else if (!correctCharacters)
                    errorText = "Name can't contain spaces or symbols";
                else if (tooLongName)
                    errorText = "Name needs to be shorter than 127 characters";

                auto onDismiss = [this] {
                    nameLabel.setText(lastName, dontSendNotification);
                };

                auto paramError = std::make_unique<ParamNameErrorCallout>(errorText, onDismiss);
                CallOutBox::launchAsynchronously(std::move(paramError), nameLabel.getScreenBounds(), nullptr);
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
        lastName = String::fromUTF8(param->getTitle().data());
        nameLabel.setText(lastName, dontSendNotification);

        auto const normalisableRange = param->getNormalisableRange();

        auto const& min = normalisableRange.start;
        auto const& max = normalisableRange.end;

        range = VarArray { min, max };

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

        auto const doubleRange = NormalisableRange<double>(normalisableRange.start, normalisableRange.end, normalisableRange.interval, normalisableRange.skew);

        if (ProjectInfo::isStandalone) {
            slider.setValue(param->getUnscaledValue());
            slider.setNormalisableRange(doubleRange);
            valueLabel.setText(String(param->getUnscaledValue(), 2), dontSendNotification);
        } else {
            slider.setNormalisableRange(doubleRange);
        }
    }

    bool hitTest(int const x, int const y) override
    {
        return getLocalBounds().toFloat().reduced(4.5f, 3.0f).contains(x, y);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (&reorderButton == e.originalComponent)
            setIsReordering(true);
        else
            setIsReordering(false);
    }

    void mouseUp(MouseEvent const& e) override
    {
        bool const isEditable = PlugDataParameter::canDynamicallyAdjustParameters();
        bool const isOverNameLable = nameLabel.getBounds().contains(e.getEventRelativeTo(&nameLabel).getPosition());

        if (e.mods.isRightButtonDown() && !ProjectInfo::isStandalone) {
            auto const* pluginEditor = findParentComponentOfClass<PluginEditor>();
            if (auto const* hostContext = pluginEditor->getHostContext()) {
                hostContextMenu = hostContext->getContextMenuForParameter(param);
                if (hostContextMenu) {
                    auto const menuPosition = pluginEditor->getMouseXYRelative();
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
        return "#X obj 0 0 param " + param->getTitle().toString() + ";";
    }

    String getPatchStringName() override
    {
        return "param object";
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(range)) {
            auto const min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto const max = static_cast<float>(range.getValue().getArray()->getReference(1));
            param->setRange(min, max);
            update();
        } else if (v.refersToSameSourceAs(mode)) {
            param->setMode(static_cast<PlugDataParameter::Mode>(getValue<int>(mode)));
            update();
        }
    }

    int getItemHeight() const
    {
        if (param->isEnabled()) {
            return settingsButton.getToggleState() ? 110.0f : 56.0f;
        }
        return 0.0f;
    }

    // TOOD: hides non-virtual function!
    bool isParameterEnabled() const
    {
        return param->isEnabled();
    }

    void resized() override
    {
        bool const settingsVisible = settingsButton.getToggleState();

        auto bounds = getLocalBounds().reduced(6, 2);

        constexpr int rowHeight = 26;

        auto const firstRow = bounds.removeFromTop(rowHeight);
        auto secondRow = bounds.removeFromTop(rowHeight);

        if (settingsVisible) {

            rangeProperty.setBounds(bounds.removeFromTop(rowHeight));
            modeProperty.setBounds(bounds.removeFromTop(rowHeight));
        }

        nameLabel.setBounds(firstRow.reduced(25, 0));
        auto const componentCentre = firstRow.getCentre().getY();
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

    std::function<void(AutomationItem*)> onDelete = [](AutomationItem*) { };
    std::unique_ptr<HostProvidedContextMenu> hostContextMenu;

    SmallIconButton deleteButton = SmallIconButton(Icons::Clear);
    ExpandButton settingsButton;

    Value range = Value(var(VarArray { var(0.0f), var(127.0f) }));
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

class AlphaAnimator final : public Timer {
public:
    AlphaAnimator() = default;

    void fadeIn(Component* component, double const duration)
    {
        animate(component, duration, 0.0f, 1.0f);
    }

    void fadeOut(Component* component, double const duration)
    {
        animate(component, duration, 1.0f, 0.0f);
    }

private:
    void animate(Component* component, double const duration, float const startAlpha, float const endAlpha)
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
            float const alpha = startAlphaValue + (endAlphaValue - startAlphaValue) * static_cast<float>(currentStep) / static_cast<float>(animationSteps);
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

class DraggedItemDropShadow final : public Component
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
        auto const rect = getLocalBounds().reduced(14, 7);
        Path shadowPath;
        shadowPath.addRoundedRectangle(rect, Corners::defaultCornerRadius);
        StackShadow::renderDropShadow(hash("automation_item"), g, shadowPath, Colours::black.withAlpha(0.3f), 7);
    }

private:
    SafePointer<AutomationItem> automationItem = nullptr;
    AlphaAnimator animator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DraggedItemDropShadow)
};

class AutomationComponent final : public Component {

    class AddParameterButton final : public Component {

        bool mouseIsOver = false;

    public:
        std::function<void()> onClick = [] { };

        void paint(Graphics& g) override
        {
            auto const bounds = getLocalBounds().reduced(5, 2);
            auto textBounds = bounds;
            auto const iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto const colour = findColour(PlugDataColour::sidebarTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
            }

            Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
            Fonts::drawText(g, "Add new parameter", textBounds, colour, 14);
        }

        bool hitTest(int const x, int const y) override
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

        addParameterButton.onClick = [this, parent] {
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
            pd->updateEnabledParameters();
            updateSliders();
        };
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        accumulatedOffsetY = { 0, 0 };

        if (auto const* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
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
        auto const viewport = findParentComponentOfClass<BouncingViewport>();
        if (viewport->autoScroll(0, viewport->getLocalPoint(nullptr, e.getScreenPosition()).getY(), 0, 5)) {
            beginDragAutoRepeat(20);
        }

        auto const dragPos = mouseDownPos.translated(0, e.getDistanceFromDragStartY());
        auto const autoScrollOffset = Point<int>(0, viewportPosY - viewport->getViewPositionY());
        accumulatedOffsetY += autoScrollOffset;
        draggedItem->setTopLeftPosition(dragPos - accumulatedOffsetY);
        viewportPosY -= autoScrollOffset.getY();

        int const idx = rows.indexOf(draggedItem);
        if (idx > 0 && draggedItem->getBounds().getCentreY() < rows[idx - 1]->getBounds().getCentreY() && rows[idx - 1]->isParameterEnabled()) {
            rows.swap(idx, idx - 1);
            shouldAnimate = true;
            resized();
        } else if (idx < rows.size() - 1 && draggedItem->getBounds().getCentreY() > rows[idx + 1]->getBounds().getCentreY() && rows[idx + 1]->isParameterEnabled()) {
            rows.swap(idx, idx + 1);
            shouldAnimate = true;
            resized();
        }
    }

    String getNewParameterName()
    {
        StringArray takenNames;
        for (auto const* row : rows) {
            if (row->isParameterEnabled()) {
                takenNames.add(row->param->getTitle().toString());
            }
        }

        auto const newParamName = String("param");
        int i = 1;
        while (takenNames.contains(newParamName + String(i))) {
            i++;
        }

        return newParamName + String(i);
    }

    HeapArray<PlugDataParameter*> getParameters() const
    {
        auto allParameters = pd->getParameters();

        HeapArray<PlugDataParameter*> params;
        params.reserve(allParameters.size() - 1);

        bool first = true;
        for (auto* param : allParameters) {
            if (first) {
                first = false;
                continue;
            }
            params.add(dynamic_cast<PlugDataParameter*>(param));
        }

        return params;
    }

    void updateSliders()
    {
        rows.clear();

        for (auto* param : pd->getEnabledParameters()) {
            if (param->getTitle() == "volume")
                continue;

            auto* slider = rows.add(new AutomationItem(param, parentComponent, pd));
            addAndMakeVisible(slider);

            slider->reorderButton.addMouseListener(this, false);

            slider->onDelete = [this](AutomationItem const* toDelete) {
                StringArray paramNames;

                for (auto const* param : getParameters()) {
                    if (param != toDelete->param) {
                        paramNames.add(param->getTitle().toString());
                    }
                }

                auto const toDeleteIdx = rows.indexOf(toDelete);
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
                toDelete->param->setDefaultValue(0.0f);
                toDelete->param->setRange(0.0f, 1.0f);
                toDelete->param->setMode(PlugDataParameter::Float);
                toDelete->param->notifyDAW();

                pd->updateEnabledParameters();
                updateSliders();
            };
        }

        std::ranges::sort(rows, [](auto* a, auto* b) {
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
        int const width = getWidth();
        for (int p = 0; p < rows.size(); p++) {
            int const height = rows[p]->getItemHeight();
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

class AutomationPanel final : public Component
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

    void updateParameterValue(PlugDataParameter const* changedParameter)
    {
        for (int p = 0; p < sliders.rows.size(); p++) {
            auto const* param = sliders.rows[p]->param;
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
