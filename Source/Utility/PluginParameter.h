/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PlugDataParameter : public RangedAudioParameter {
public:
    enum Mode {
        Float = 1,
        Integer,
        Logarithmic,
        Exponential
    };

    PluginProcessor& processor;

    PlugDataParameter(PluginProcessor* p, String const& defaultName, float const def, bool enabled, int idx, float minimum, float maximum)
        : RangedAudioParameter(ParameterID(defaultName, 1), defaultName, AudioProcessorParameterWithIDAttributes())
        , processor(*p)
        , defaultValue(def)
        , index(idx)
        , enabled(enabled)
        , parameterName(defaultName)
        , normalisableRange(minimum, maximum, 0.000001f)
        , mode(Float)
    {
        value = normalisableRange.convertFrom0to1(getDefaultValue());
    }

    ~PlugDataParameter() override = default;

    int getNumSteps() const override
    {
        auto range = getNormalisableRange();
        return (static_cast<int>((range.end - range.start) / 0.000001f) + 1);
    }

    void setRange(float min, float max)
    {
        ScopedLock lock(rangeLock);
        normalisableRange.start = min;
        normalisableRange.end = max;
    }

    void setMode(Mode newMode, bool notify = true)
    {
        ScopedLock lock(rangeLock);

        mode = newMode;
        if (newMode == Logarithmic) {
            normalisableRange.skew = 4.0f;
            normalisableRange.interval = 0.000001f;
        } else if (newMode == Exponential) {
            normalisableRange.skew = 0.25f;
            normalisableRange.interval = 0.000001f;
        } else if (newMode == Float) {
            normalisableRange.skew = 1.0f;
            normalisableRange.interval = 0.000001f;
        } else if (newMode == Integer) {
            normalisableRange.skew = 1.0f;
            normalisableRange.start = std::floor(normalisableRange.start);
            normalisableRange.end = std::floor(normalisableRange.end);
            normalisableRange.interval = 1.0f;
            setValue(std::floor(getValue()));
        }

        if (notify)
            notifyDAW();
    }

    // Reports whether the current DAW/format can deal with dynamic
    static bool canDynamicallyAdjustParameters()
    {
        // We can add more DAWs or formats here if needed
        return PluginHostType::getPluginLoadedAs() != AudioProcessor::wrapperType_LV2;
    }

    void setName(String const& newName)
    {
        ScopedLock lock(nameLock);
        parameterName = newName;
    }

    String getName(int maximumStringLength) const override
    {
        auto name = getTitle();
        if (!isEnabled() && canDynamicallyAdjustParameters()) {
            return ("(DISABLED) " + name).substring(0, maximumStringLength - 1);
        }

        return name.substring(0, maximumStringLength - 1);
    }

    String getTitle() const
    {
        ScopedLock lock(nameLock);
        return parameterName;
    }

    void setEnabled(bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
    }

    NormalisableRange<float> const& getNormalisableRange() const override
    {
        ScopedLock lock(rangeLock);
        return normalisableRange;
    }

    void notifyDAW()
    {
        if (!ProjectInfo::isStandalone) {
            processor.sendParameterInfoChangeMessage();
        }
    }

    float getUnscaledValue() const
    {
        return value;
    }

    void setUnscaledValueNotifyingHost(float newValue)
    {
        auto range = getNormalisableRange();
        value = std::clamp(newValue, range.start, range.end);
        sendValueChangedMessageToListeners(getValue());
    }

    float getValue() const override
    {
        auto range = getNormalisableRange();
        return range.convertTo0to1(value);
    }

    void setValue(float newValue) override
    {
        auto range = getNormalisableRange();
        value = range.convertFrom0to1(newValue);
    }

    float getDefaultValue() const override
    {
        return defaultValue;
    }

    String getText(float value, int maximumStringLength) const override
    {
        auto range = getNormalisableRange();
        auto const mappedValue = range.convertFrom0to1(value);

        return maximumStringLength > 0 ? String(mappedValue).substring(0, maximumStringLength) : String(mappedValue, 6);
    }

    float getValueForText(String const& text) const override
    {
        auto range = getNormalisableRange();
        return range.convertTo0to1(text.getFloatValue());
    }

    bool isDiscrete() const override
    {
        return mode == Integer;
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

    std::atomic<float>* getValuePointer()
    {
        return &value;
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
            paramXml->setAttribute(String("min"), param->getNormalisableRange().start);
            paramXml->setAttribute(String("max"), param->getNormalisableRange().end);
            paramXml->setAttribute(String("enabled"), static_cast<int>(param->enabled));

            paramXml->setAttribute(String("value"), static_cast<double>(param->getValue()));
            paramXml->setAttribute(String("index"), param->index);
            paramXml->setAttribute(String("mode"), static_cast<int>(param->mode));

            xml.addChildElement(paramXml);
        }
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
            int index = i;
            Mode mode = Float;

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
            if (xmlParam->hasAttribute("index")) {
                index = xmlParam->getIntAttribute("index");
            }
            if (xmlParam->hasAttribute("mode")) {
                mode = static_cast<Mode>(xmlParam->getIntAttribute("mode"));
            }

            param->setRange(min, max);
            param->setName(name);
            param->setIndex(index);
            param->setMode(mode, false);
            param->setValue(navalue);
            param->setEnabled(enabled);
        }
    }

    void setLastValue(float v)
    {
        lastValue = v;
    }

    float getLastValue() const
    {
        return lastValue;
    }

    float getGestureState() const
    {
        return gestureState;
    }

    void setIndex(int idx)
    {
        index = idx;
    }

    int getIndex()
    {
        return index;
    }

    void setGestureState(float v)
    {
        if (!ProjectInfo::isStandalone) {
            // Send new value to DAW
            if (v) {
                beginChangeGesture();
            } else {
                endChangeGesture();
            }
        }

        gestureState = v;
    }

private:
    float lastValue = 0.0f;
    float const defaultValue;

    // TODO: do they all need to be atomic?
    std::atomic<float> gestureState = 0.0f;
    std::atomic<int> index;
    std::atomic<float> value;
    std::atomic<bool> enabled = false;

    CriticalSection nameLock;
    String parameterName;

    CriticalSection rangeLock;
    NormalisableRange<float> normalisableRange;

    Mode mode;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataParameter)
};
