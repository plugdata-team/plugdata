/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PlugDataParameter : public RangedAudioParameter {
public:
    PluginProcessor& processor;

    PlugDataParameter(PluginProcessor* p, String const& defaultName, float const def, bool enabled)
        : RangedAudioParameter(ParameterID(defaultName, 1), defaultName, defaultName)
        , range(0.0f, 1.0f, 0.000001f)
        , defaultValue(def)
        , processor(*p)
        , enabled(enabled)
        , name(defaultName)
    {
        value = range.convertFrom0to1(getDefaultValue());
    }

    ~PlugDataParameter() {};

    int getNumSteps() const override
    {
        return (static_cast<int>((range.end - range.start) / 0.000001f) + 1);
    }

    void setRange(float min, float max)
    {
        range = NormalisableRange<float>(min, max, 0.000001f);
    }

    void setName(String const& newName)
    {
        name = newName;
    }

    String getName(int maximumStringLength) const override
    {
        if (!isEnabled()) {
            return ("(DISABLED) " + name).substring(0, maximumStringLength - 1);
        }

        return name.substring(0, maximumStringLength - 1);
    }

    String getTitle()
    {
        return name;
    }

    void setEnabled(bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
    }

    NormalisableRange<float> const& getNormalisableRange() const override
    {
        return range;
    }

    void notifyDAW()
    {
        if (!ProjectInfo::isStandalone) {
            auto const details = AudioProcessorListener::ChangeDetails {}.withParameterInfoChanged(true);
            processor.updateHostDisplay(details);
        }
    }

    float getUnscaledValue() const
    {
        return value;
    }

    void setUnscaledValueNotifyingHost(float newValue)
    {
        value = std::clamp(newValue, range.start, range.end);
        sendValueChangedMessageToListeners(getValue());
    }

    float getValue() const override
    {
        return range.convertTo0to1(value);
    }

    void setValue(float newValue) override
    {
        value = range.convertFrom0to1(newValue);
    }

    float getDefaultValue() const override
    {
        return defaultValue;
    }

    String getText(float value, int maximumStringLength) const override
    {
        auto const mappedValue = range.convertFrom0to1(value);

        return maximumStringLength > 0 ? String(mappedValue).substring(0, maximumStringLength) : String(mappedValue, 6);
    }

    float getValueForText(String const& text) const override
    {
        return range.convertTo0to1(text.getFloatValue());
    }

    bool isDiscrete() const override
    {
        return false;
    }

    bool isOrientationInverted() const override
    {
        return false;
    }

    bool isEnabled() const
    {
        return enabled;
    }

    bool isAutomatable() const override
    {
        return true;
    }

    bool isMetaParameter() const override
    {
        return false;
    }

    static void saveStateInformation(XmlElement& xml, Array<AudioProcessorParameter*> const& parameters)
    {
        auto* volumeXml = new XmlElement("PARAM");
        volumeXml->setAttribute("id", "volume");
        volumeXml->setAttribute("value", parameters[0]->getValue());
        xml.addChildElement(volumeXml);

        for (int i = 1; i < parameters.size(); i++) {

            auto* param = dynamic_cast<PlugDataParameter*>(parameters[i]);

            auto* paramXml = new XmlElement("PARAM");

            paramXml->setAttribute("id", String("param") + String(i));

            paramXml->setAttribute(String("name"), param->getTitle());
            paramXml->setAttribute(String("min"), param->range.start);
            paramXml->setAttribute(String("max"), param->range.end);
            paramXml->setAttribute(String("enabled"), static_cast<int>(param->enabled));

            paramXml->setAttribute(String("value"), static_cast<double>(param->getValue()));

            xml.addChildElement(paramXml);
        }
    }

    std::atomic<float>* getValuePointer()
    {
        return &value;
    }

    static void loadStateInformation(XmlElement const& xml, Array<AudioProcessorParameter*> const& parameters)
    {
        auto* volumeParam = xml.getChildByAttribute("id", "volume");
        if (volumeParam) {
            auto const navalue = static_cast<float>(volumeParam->getDoubleAttribute(String("value"),
                static_cast<double>(parameters[0]->getValue())));

            parameters[0]->setValueNotifyingHost(navalue);
        }

        for (int i = 1; i < parameters.size(); i++) {
            auto* param = dynamic_cast<PlugDataParameter*>(parameters[i]);

            auto xmlParam = xml.getChildByAttribute("id", "param" + String(i));

            if (!xmlParam)
                continue;

            auto const navalue = static_cast<float>(xmlParam->getDoubleAttribute(String("value"),
                static_cast<double>(param->getValue())));

            String name = "param" + String(i);
            float min = 0.0f, max = 1.0f;
            bool enabled = true;

            // Check for these values, they may not be there in legacy versions
            if (xmlParam->hasAttribute("name")) {
                name = xmlParam->getStringAttribute(String("name"));
            }
            if (xmlParam->hasAttribute("min")) {
                min = xmlParam->getDoubleAttribute("min");
            }
            if (xmlParam->hasAttribute("max")) {
                max = xmlParam->getDoubleAttribute("max");
            }
            if (xmlParam->hasAttribute("enabled")) {
                enabled = xmlParam->getIntAttribute("enabled");
            }

            param->setEnabled(enabled);
            param->setRange(min, max);
            param->setName(name);
            param->setValueNotifyingHost(navalue);
            param->notifyDAW();
        }
    }

    void setLastValue(float v)
    {
        lastValue = v;
    }

    float getLastValue()
    {
        return lastValue;
    }

    float getGestureState()
    {
        return gestureState;
    }

    void setGestureState(float v)
    {

        if (!ProjectInfo::isStandalone) {
            // Send new value to DAW
            v ? beginChangeGesture() : endChangeGesture();
        }

        gestureState = v;
    }

private:
    float lastValue = 0.0f;
    float gestureState = 0.0f;
    float const defaultValue;

    std::atomic<float> value;
    NormalisableRange<float> range;
    String name;
    std::atomic<bool> enabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataParameter)
};
