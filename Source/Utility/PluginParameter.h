/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>


// ======================================================================================== //
//                                      PARAMETER                                           //
// ======================================================================================== //

class PlugDataParameter : public RangedAudioParameter
{
public:
    
    PlugDataAudioProcessor& processor;
    
    PlugDataParameter(PlugDataAudioProcessor* p, const String& name, const String& label, const float def) :
    RangedAudioParameter(name.toLowerCase().replaceCharacter(' ', '_'), name, label),
    norm_range(0.0f, 1.0f),
    default_value(def),
    processor(*p)
    {
        value = convertFrom0to1(getDefaultValue());
    }
    
    
    ~PlugDataParameter() {};
    
    void setRange(float min, float max) {
        norm_range = NormalisableRange<float>(min, max);
        notifyDAW();
    }
    
    void setName(const String& newName) {
        name = newName;
        notifyDAW();
    }
    
    String getName(int maximumStringLength) {
        return name.substring(0, maximumStringLength - 1);
    }
    
    void setEnabled(bool shouldBeEnabled) {
        isEnabled = shouldBeEnabled;
        notifyDAW();
    }
    
    NormalisableRange<float> const& getNormalisableRange() const override
    {
        return norm_range;
    }
    
    
    void notifyDAW() {
#if !PLUGDATA_STANDALONE
        const auto details = AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged (true);
        processor.updateHostDisplay (details);
#endif
    }
    
    float getValue() const override
    {
        return convertTo0to1(value);
    }
    
    void setValue(float newValue) override
    {
        value = convertFrom0to1(newValue);
    }
    
    float getDefaultValue() const override
    {
        return default_value;
    }
    
    String getText(float value, int maximumStringLength) const override
    {
        auto const mappedValue = convertFrom0to1(value);
        
        auto const isInterger = norm_range.interval > 0.0 && (std::abs(norm_range.interval - std::floor(norm_range.interval)) < std::numeric_limits<float>::epsilon() && std::abs(norm_range.start - std::floor(norm_range.start))  < std::numeric_limits<float>::epsilon());
        
        if(isInterger)
        {
            return maximumStringLength > 0 ? String(static_cast<int>(mappedValue)).substring(0, maximumStringLength) :  String(static_cast<int>(mappedValue));
        }
        return maximumStringLength > 0 ? String(mappedValue, 2).substring(0, maximumStringLength) :  String(mappedValue, 2);
        
    }
    
    float getValueForText(const String& text) const override
    {
        return convertTo0to1(text.getFloatValue());
    }
    
    
    bool isDiscrete() const override
    {
        return true;
    }
    
    bool isOrientationInverted() const override
    {
        return norm_range.start > norm_range.end;
    }
    
    bool isAutomatable() const override
    {
        return isEnabled;
    }
    
    bool isMetaParameter() const override
    {
        return false;
    }
    
    static void saveStateInformation(XmlElement& xml, Array<AudioProcessorParameter*> const& parameters)
    {
        XmlElement* params = xml.createNewChildElement("params");
        if(params)
        {
            for(int i = 0; i < parameters.size(); ++i)
            {
                params->setAttribute(String("param") + String(i+1), static_cast<double>(parameters[i]->getValue()));
            }
        }
    }

    static void loadStateInformation(XmlElement const& xml, Array<AudioProcessorParameter*> const& parameters)
    {
        XmlElement const* params = xml.getChildByName(juce::StringRef("params"));
        if(params)
        {
            for(int i = 0; i < parameters.size(); ++i)
            {
                const float navalue = static_cast<float>(params->getDoubleAttribute(String("param") + String(i+1),
                                                                                    static_cast<double>(parameters[i]->getValue())));
                parameters[i]->setValueNotifyingHost(navalue);
            }
        }
    }

private:
    std::atomic<float> value;
    NormalisableRange<float> norm_range;
    
    bool isEnabled = false;
    
    float const default_value;
    
    String name;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataParameter)
};


