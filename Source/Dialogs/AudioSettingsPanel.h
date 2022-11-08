/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


struct DAWAudioSettings : public Component, public Value::Listener {
    explicit DAWAudioSettings(AudioProcessor& p)
        : processor(p)
    {
        addAndMakeVisible(latencyNumberBox);
        addAndMakeVisible(tailLengthNumberBox);
        addAndMakeVisible(nativeDialogToggle);
        
        dynamic_cast<DraggableNumber*>(latencyNumberBox.label.get())->setMinimum(64);
        
        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        auto& settingsTree = dynamic_cast<PlugDataAudioProcessor&>(p).settingsTree;
        
        if(!settingsTree.hasProperty("NativeDialog")) {
            settingsTree.setProperty("NativeDialog", true, nullptr);
        }
        
        tailLengthValue.referTo(proc->tailLength);
        nativeDialogValue.referTo(settingsTree.getPropertyAsValue("NativeDialog", nullptr));
        
        tailLengthValue.addListener(this);
        latencyValue.addListener(this);
        nativeDialogValue.addListener(this);
        
        latencyValue = proc->getLatencySamples();
        
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        latencyNumberBox.setBounds(bounds.removeFromTop(23));
        tailLengthNumberBox.setBounds(bounds.removeFromTop(23));
        nativeDialogToggle.setBounds(bounds.removeFromTop(23));
    }
    
    
    void valueChanged(Value& v) override
    {
        if(v.refersToSameSourceAs(latencyValue)) {
            processor.setLatencySamples(static_cast<int>(latencyValue.getValue()));
        }
    }

    AudioProcessor& processor;
    
    Value latencyValue;
    Value tailLengthValue;
    Value nativeDialogValue;
    
    PropertiesPanel::EditableComponent<int> latencyNumberBox = PropertiesPanel::EditableComponent<int>("Latency (samples)", latencyValue);
    PropertiesPanel::EditableComponent<float> tailLengthNumberBox = PropertiesPanel::EditableComponent<float>("Tail Length (seconds)", tailLengthValue);
    PropertiesPanel::BoolComponent nativeDialogToggle = PropertiesPanel::BoolComponent("Use Native Dialog", tailLengthValue,  {"No", "Yes"});
};
