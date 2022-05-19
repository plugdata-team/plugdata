/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Canvas.h"

struct AutomationComponent : public Component
{
    PlugDataAudioProcessor* pd;

    AutomationComponent(PlugDataAudioProcessor* processor) : pd(processor)
    {
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++)
        {
            auto* slider = sliders.add(new Slider());
            auto* label = labels.add(new Label());
            auto* button = createButtons.add(new TextButton(Icons::Add));

            button->setName("statusbar:createbutton");

            String name = "param" + String(p + 1);
            label->setText(name, dontSendNotification);
            label->attachToComponent(slider, true);

            slider->setScrollWheelEnabled(false);
            slider->setTextBoxStyle(Slider::TextBoxRight, false, 45, 13);
            

#if PLUGDATA_STANDALONE
            slider->onValueChange = [this, slider, p]() mutable
            {
                float value = slider->getValue();
                pd->standaloneParams[p] = value;
            };
#endif

            button->onClick = [this, name]() mutable
            {
                if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(pd->getActiveEditor()))
                {
                    auto* cnv = editor->getCurrentCanvas();
                    if (cnv)
                    {
                        cnv->attachNextObjectToMouse = true;
                        cnv->boxes.add(new Box(cnv, "r " + name));
                    }
                }
            };

            addAndMakeVisible(label);
            addAndMakeVisible(slider);
            addAndMakeVisible(button);

#if PLUGDATA_STANDALONE
            sliders[p]->setValue(pd->standaloneParams[p]);
            sliders[p]->setRange(0.0f, 1.0f);
#else
            auto* param = pd->parameters.getParameter("param" + String(p + 1));
            auto range = param->getNormalisableRange().getRange();
            
            slider->setNormalisableRange(NormalisableRange<double>(range.getStart(), range.getEnd()));
            slider->setValue(param->getValue());
            attachments.add(new SliderParameterAttachment(*param, *slider, nullptr));
#endif
        }
    }
    
#if PLUGDATA_STANDALONE
    void updateParameters() {
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++)
        {
            sliders[p]->setValue(pd->standaloneParams[p]);
        }
    }
#endif
    
    void paint(Graphics& g) override
    {
        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++)
        {

            
            sliders[p]->setColour(Slider::backgroundColourId, findColour(p & 1 ? PlugDataColour::toolbarColourId : PlugDataColour::canvasColourId));
            sliders[p]->setColour(Slider::trackColourId, findColour(PlugDataColour::textColourId));

            auto offColour = findColour(PlugDataColour::canvasColourId);
            auto onColour = offColour.darker(0.04f);
            
            g.setColour(p & 1 ? offColour : onColour);
            g.fillRect(0, sliders[p]->getY(), getWidth(), sliders[p]->getHeight());
        }
    }

    void resized() override
    {
        auto fb = FlexBox(FlexBox::Direction::column, FlexBox::Wrap::noWrap, FlexBox::AlignContent::flexStart, FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexStart);

        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++)
        {
            auto item = FlexItem(*sliders[p]).withMinHeight(19.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }

        auto bounds = getLocalBounds().withTrimmedLeft(55).withTrimmedRight(40).toFloat();

        fb.performLayout(bounds);

        for (int p = 0; p < PlugDataAudioProcessor::numParameters; p++)
        {
            createButtons[p]->setBounds(sliders[p]->getBounds().withX(getWidth() - 40).withWidth(30));
        }
    }

    OwnedArray<TextButton> createButtons;
    OwnedArray<Label> labels;
    OwnedArray<Slider> sliders;
    
#if !PLUGDATA_STANDALONE
    OwnedArray<SliderParameterAttachment> attachments;
#endif
};

struct AutomationPanel : public Component, public ScrollBar::Listener
{
    AutomationPanel(PlugDataAudioProcessor* processor) : sliders(processor)
    {
        viewport.setViewedComponent(&sliders, false);
        viewport.setScrollBarsShown(true, false, false, false);
        viewport.setBufferedToImage(true);

        viewport.getVerticalScrollBar().addListener(this);

        addAndMakeVisible(viewport);
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(getLocalBounds().withTrimmedLeft(Sidebar::dragbarWidth));
        g.fillRect(getLocalBounds().withTrimmedLeft(Sidebar::dragbarWidth).withHeight(viewport.getY()));

        g.setColour(findColour(PlugDataColour::textColourId));
        g.setFont(15);
        g.drawFittedText("Parameters", 0, 3, getWidth(), viewport.getY(), Justification::centred, 1);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 0.5f, getWidth(), 0.5f);
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds().withTrimmedTop(30).withTrimmedBottom(30).withTrimmedLeft(Sidebar::dragbarWidth));
        sliders.setSize(getWidth(), 10000);
    }

    Viewport viewport;
    AutomationComponent sliders;
};
