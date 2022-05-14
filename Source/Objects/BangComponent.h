
struct BangComponent : public GUIComponent
{
    uint32_t lastBang = 0;
    
    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);
    
    bool bangState = false;
    
    BangComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        bangInterrupt = static_cast<t_bng*>(gui.getPointer())->x_flashtime_break;
        bangHold = static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold;
        
        initialise(newObject);
    }
    
    void checkBoxBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, box->getWidth());
        if (size != box->getHeight() || size != box->getWidth())
        {
            box->setSize(size, size);
        }
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        startEdition();
        setValueOriginal(1);
        stopEdition();
        update();
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(box->findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
        
        const auto bounds = getLocalBounds().reduced(1).toFloat();
        const auto width = std::max(bounds.getWidth(), bounds.getHeight());
        
        const float circleOuter = 80.f * (width * 0.01f);
        const float circleThickness = std::max(width * 0.06f, 1.5f);
        
        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);
        
        g.setColour(bangState ? findColour(TextButton::buttonOnColourId) : Colours::transparentWhite);
        
        g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
    }
    
    void update() override
    {
        if (getValueOriginal() > std::numeric_limits<float>::epsilon())
        {
            bangState = true;
            repaint();
            
            auto currentTime = Time::getCurrentTime().getMillisecondCounter();
            auto timeSinceLast = currentTime - lastBang;
            
            int holdTime = bangHold.getValue();
            
            if (timeSinceLast < static_cast<int>(bangHold.getValue()) * 2)
            {
                holdTime = timeSinceLast / 2;
            }
            if (holdTime < bangInterrupt)
            {
                holdTime = bangInterrupt.getValue();
            }
            
            lastBang = currentTime;
            
            auto deletionChecker = SafePointer<Component>(this);
            Timer::callAfterDelay(holdTime,
                                  [deletionChecker, this]() mutable
                                  {
                // First check if this object still exists
                if (!deletionChecker) return;
                
                if(bangState) {
                    bangState = false;
                    repaint();
                }
            });
        }
    }
    
    ObjectParameters defineParameters() override
    {
        return {
            {"Interrupt", tInt, cGeneral, &bangInterrupt, {}},
            {"Hold", tInt, cGeneral, &bangHold, {}},
        };
    }
    
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(bangInterrupt))
        {
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold))
        {
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold = bangHold.getValue();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};
