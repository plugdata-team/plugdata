/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"

struct AutomationSlider : public Component, public Value::Listener {

    PlugDataAudioProcessor* pd;
    
    AutomationSlider(int idx, PlugDataAudioProcessor* processor)
        : index(idx), pd(processor)
    {
        createButton.setName("statusbar:createbutton");

        nameLabel.setText(String(idx + 1), dontSendNotification);

        createButton.onClick = [this]() mutable {
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pd->getActiveEditor())) {
                auto* cnv = editor->getCurrentCanvas();
                if (cnv) {
                    cnv->attachNextObjectToMouse = true;
                    cnv->objects.add(new Object(cnv, "param " + String(index + 1)));
                }
            }
        };
        
        pd->locked.addListener(this);

        slider.setScrollWheelEnabled(false);
        slider.setTextBoxStyle(Slider::NoTextBox, false, 45, 13);

#if PLUGDATA_STANDALONE
        slider.setValue(pd->standaloneParams[index]);
        slider.setRange(0.0f, 1.0f);
        valueLabel.setText(String(pd->standaloneParams[index], 2), dontSendNotification);
        slider.onValueChange = [this]() mutable {
            float value = slider.getValue();
            pd->standaloneParams[index] = value;
            valueLabel.setText(String(value, 2), dontSendNotification);
        };
#else
        slider.onValueChange = [this]() mutable {
            float value = slider.getValue();
            valueLabel.setText(String(value, 2), dontSendNotification);
        };
        auto* param = pd->parameters.getParameter("param" + String(index + 1));
        auto range = param->getNormalisableRange().getRange();
        attachment = std::make_unique<SliderParameterAttachment>(*param, slider, nullptr);
        valueLabel.setText(String(param->getValue(), 2), dontSendNotification);
#endif
        valueLabel.onEditorShow = [this]() mutable {
            if (auto* editor = valueLabel.getCurrentTextEditor()) {
                editor->setInputRestrictions(-1, "0123456789.");
            }
        };

        valueLabel.onEditorHide = [this]() mutable {
            auto* param = pd->parameters.getParameter("param" + String(index + 1));
            param->setValue(valueLabel.getText().getFloatValue());
        };

        valueLabel.setMinimumHorizontalScale(1.0f);
        nameLabel.setMinimumHorizontalScale(1.0f);
        nameLabel.setJustificationType(Justification::centred);

        valueLabel.setEditable(true);

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(slider);
        addAndMakeVisible(valueLabel);
        addAndMakeVisible(createButton);
    }
    
    ~AutomationSlider() {
        pd->locked.removeListener(this);
    }
    
    
    void valueChanged(Value& v) override
    {
        createButton.setEnabled(!static_cast<bool>(v.getValue()));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        nameLabel.setBounds(bounds.removeFromLeft(30).expanded(4, 0));
        slider.setBounds(bounds.removeFromLeft(getWidth() - 100));
        valueLabel.setBounds(bounds.removeFromLeft(30).expanded(4, 0));
        createButton.setBounds(bounds.removeFromLeft(23));
    }

    void paint(Graphics& g) override
    {
        slider.setColour(Slider::backgroundColourId, findColour(index & 1 ? PlugDataColour::panelBackgroundOffsetColourId : PlugDataColour::panelBackgroundColourId));
        slider.setColour(Slider::trackColourId, findColour(PlugDataColour::panelTextColourId));

        auto offColour = findColour(PlugDataColour::panelBackgroundOffsetColourId);
        auto onColour = findColour(PlugDataColour::panelBackgroundColourId);

        g.setColour(index & 1 ? offColour : onColour);
        g.fillRect(getLocalBounds());
    }

    TextButton createButton = TextButton(Icons::Add);

    Label nameLabel;
    Label valueLabel;
    Slider slider;

    int index;

#if !PLUGDATA_STANDALONE
    std::unique_ptr<SliderParameterAttachment> attachment;
#endif
};

struct AutomationComponent : public Component {
    PlugDataAudioProcessor* pd;

    explicit AutomationComponent(PlugDataAudioProcessor* processor)
        : pd(processor)
    {
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++) {
            auto* slider = rows.add(new AutomationSlider(p, processor));
            addAndMakeVisible(slider);
        }
    }

    void resized() override
    {
        int height = 23;
        int y = 0;
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++) {
            auto rect = Rectangle<int>(0, y, getWidth(), height);
            y += height;
            rows[p]->setBounds(rect);
        }
    }

    OwnedArray<AutomationSlider> rows;
};

struct AutomationPanel : public Component
    , public ScrollBar::Listener {
    explicit AutomationPanel(PlugDataAudioProcessor* processor)
        : sliders(processor)
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
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRect(getLocalBounds().withTrimmedLeft(Sidebar::dragbarWidth));
        g.fillRect(getLocalBounds().withTrimmedLeft(Sidebar::dragbarWidth).withHeight(viewport.getY()));

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(15);
        g.drawFittedText("Parameters", 0, 3, getWidth(), viewport.getY(), Justification::centred, 1);

//        g.setColour(findColour(PlugDataColour::outlineColourId));
//        g.drawLine(0, 0.5f, getWidth(), 0.5f);
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds().withTrimmedTop(30).withTrimmedBottom(30).withTrimmedLeft(Sidebar::dragbarWidth));
        sliders.setSize(getWidth(), PlugDataAudioProcessor::numParameters * 23);
    }

#if PLUGDATA_STANDALONE
    void updateParameters()
    {
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++) {
            sliders.rows[p]->slider.setValue(sliders.pd->standaloneParams[p]);
        }
    }
#endif

    Viewport viewport;
    AutomationComponent sliders;
};
