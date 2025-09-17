/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Utility/SeqLock.h"

class PlugDataParameter final : public RangedAudioParameter {
public:
    enum Mode {
        Float = 1,
        Integer,
        Logarithmic,
        Exponential
    };

    PluginProcessor& processor;

    PlugDataParameter(PluginProcessor* p, String const& defaultName, float const def, bool const enabled, int const idx, float const minimum, float const maximum)
        : RangedAudioParameter(ParameterID(defaultName, 1), defaultName, AudioProcessorParameterWithIDAttributes())
        , processor(*p)
        , defaultValue(def)
        , index(idx)
        , enabled(enabled)
        , rangeStart(minimum)
        , rangeEnd(maximum)
        , rangeInterval(0)
        , mode(Float)
    {
        value = NormalisableRange<float>(rangeStart, rangeEnd, rangeInterval, rangeSkew).convertFrom0to1(getDefaultValue());

        setName(defaultName);
    }

    ~PlugDataParameter() override = default;

    int getNumSteps() const override
    {
        auto const range = getNormalisableRange();
        if(mode == Integer)
        {
            return (range.end - range.start) + 1;
        }
        
        return static_cast<int>((range.end - range.start) / std::numeric_limits<float>::epsilon()) + 1;
    }

    void setRange(float const min, float const max)
    {
        rangeStart = min;
        rangeEnd = max;
    }

    void setMode(Mode const newMode, bool const notify = true)
    {
        mode = newMode;
        if (newMode == Logarithmic) {
            rangeSkew = 4.0f;
            rangeInterval = 0.0f;
        } else if (newMode == Exponential) {
            rangeSkew = 0.25f;
            rangeInterval = 0.0f;
        } else if (newMode == Float) {
            rangeSkew = 1.0f;
            rangeInterval = 0.0f;
        } else if (newMode == Integer) {
            rangeSkew = 1.0f;
            rangeStart = std::floor(rangeStart);
            rangeEnd = std::floor(rangeEnd);
            rangeInterval = 1.0f;
            setValue(getValue());
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

    void setName(SmallString const& newName)
    {
        StackArray<char, 128> name = {};
        std::copy_n(newName.data(), newName.length(), name.data());
        parameterName.store(name);
    }

    String getName(int const maximumStringLength) const override
    {
        auto const name = getTitle().toString();
        if (!isEnabled() && canDynamicallyAdjustParameters()) {
            return ("(DISABLED) " + name).substring(0, maximumStringLength - 1);
        }

        return name.substring(0, maximumStringLength - 1);
    }

    SmallString getTitle() const
    {
        return SmallString(parameterName.load().data());
    }

    void setEnabled(bool const shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
    }

    NormalisableRange<float> const& getNormalisableRange() const override
    {
        // Have to do this because RangedAudioParameter forces us to return a reference...
        const_cast<PlugDataParameter*>(this)->normalisableRangeRet = NormalisableRange<float>(rangeStart, rangeEnd, rangeInterval, rangeSkew);
        return normalisableRangeRet;
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

    void setUnscaledValueNotifyingHost(float const newValue)
    {
        auto const range = getNormalisableRange();
        auto const oldValue = value.load();
        value = std::clamp(newValue, range.start, range.end);
        sendValueChangedMessageToListeners(getValue());
        valueChanged = valueChanged || (oldValue != value);
    }

    float getValue() const override
    {
        auto const range = getNormalisableRange();
        return range.convertTo0to1(value);
    }

    void setValue(float const newValue) override
    {
        auto const range = getNormalisableRange();
        auto const oldValue = value.load();
        value = range.convertFrom0to1(newValue);
        valueChanged = valueChanged || (oldValue != value);
    }
    
    void setDefaultValue(float newDefaultValue)
    {
        defaultValue = newDefaultValue;
        if(enabled && !loadedFromDAW)
        {
            setValue(newDefaultValue);
        }
    }
    
    void clearLoadedFromDAWFlag()
    {
        loadedFromDAW = false;
    }

    float getDefaultValue() const override
    {
        return defaultValue;
    }

    String getText(float const value, int const maximumStringLength) const override
    {
        auto const range = getNormalisableRange();
        auto const mappedValue = range.convertFrom0to1(value);

        return maximumStringLength > 0 ? String(mappedValue).substring(0, maximumStringLength) : String(mappedValue, 6);
    }

    float getValueForText(String const& text) const override
    {
        auto const range = getNormalisableRange();
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

    AtomicValue<float>* getValuePointer()
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

            auto const* param = dynamic_cast<PlugDataParameter*>(parameters[i]);

            auto* paramXml = new XmlElement("PARAM");

            paramXml->setAttribute("id", String("param") + String(i));

            paramXml->setAttribute(String("name"), param->getTitle().toString());
            paramXml->setAttribute(String("min"), param->getNormalisableRange().start);
            paramXml->setAttribute(String("max"), param->getNormalisableRange().end);
            paramXml->setAttribute(String("enabled"), param->enabled);

            paramXml->setAttribute(String("value"), static_cast<double>(param->getValue()));
            paramXml->setAttribute(String("index"), param->index);
            paramXml->setAttribute(String("mode"), param->mode);

            xml.addChildElement(paramXml);
        }
    }

    static void loadStateInformation(XmlElement const& xml, Array<AudioProcessorParameter*> const& parameters)
    {
        if (auto const* volumeParam = xml.getChildByAttribute("id", "volume")) {
            auto const navalue = static_cast<float>(volumeParam->getDoubleAttribute(String("value"),
                parameters[0]->getValue()));

            parameters[0]->setValueNotifyingHost(navalue);
        }

        for (int i = 1; i < parameters.size(); i++) {
            auto* param = dynamic_cast<PlugDataParameter*>(parameters[i]);

            auto const xmlParam = xml.getChildByAttribute("id", "param" + String(i));

            if (!xmlParam)
                continue;

            auto const navalue = static_cast<float>(xmlParam->getDoubleAttribute(String("value"),
                param->getValue()));

            auto name = "param" + String(i);
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
            param->setChanged();
            param->setEnabled(enabled);
            param->loadedFromDAW = enabled;
        }
    }

    bool wasChanged() const
    {
        return valueChanged;
    }

    void setUnchanged()
    {
        valueChanged = false;
    }

    void setChanged()
    {
        valueChanged = true;
    }

    float getGestureState() const
    {
        return gestureState;
    }

    void setIndex(int const idx)
    {
        index = idx;
    }

    int getIndex()
    {
        return index;
    }

    void setGestureState(float const v)
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
    float defaultValue;
    bool loadedFromDAW = false;
    
    AtomicValue<bool> valueChanged;
    AtomicValue<float> gestureState = 0.0f;
    AtomicValue<int> index;
    AtomicValue<float> value;
    AtomicValue<bool> enabled = false;

    AtomicValue<float> rangeStart = 0;
    AtomicValue<float> rangeEnd = 1;
    AtomicValue<float> rangeInterval = 0;
    AtomicValue<float> rangeSkew = 1;

    AtomicValue<StackArray<char, 128>> parameterName;
    NormalisableRange<float> normalisableRangeRet;

    Mode mode;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugDataParameter)
};
