#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <utility>
#include "Utility/SmallVector.h"
#include "LookAndFeel.h"

enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool,
    tCombo,
    tRangeFloat,
    tRangeInt,
    tFont,
    tCustom
};

enum ParameterCategory {
    cDimensions,
    cGeneral,
    cAppearance,
    cLabel
};

class PropertiesPanelProperty;
using CustomPanelCreateFn = std::function<PropertiesPanelProperty*(void)>;
using InteractionFn = std::function<void(bool)>;

using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, StringArray, var, CustomPanelCreateFn, InteractionFn>;

class ObjectParameters {
public:
    ObjectParameters() = default;

    SmallVector<ObjectParameter> getParameters()
    {
        return objectParameters;
    }

    void addParam(ObjectParameter const& param)
    {
        objectParameters.push_back(param);
    }

    void resetAll()
    {
        auto& lnf = LookAndFeel::getDefaultLookAndFeel();
        for (auto [name, type, category, value, options, defaultVal, customComponent, onInteractionFn] : objectParameters) {
            if (!defaultVal.isVoid()) {
                if (type == tColour) {
                    value->setValue(lnf.findColour(defaultVal).toString());
                } else if (defaultVal.isArray() && defaultVal.getArray()->isEmpty()) {
                    return;
                } else {
                    value->setValue(defaultVal);
                }
            }
        }
    }

    // ========= overloads for making different types of parameters =========

    void addParamFloat(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.push_back(makeParam(pString, tFloat, pCat, pVal, StringArray(), pDefault));
    }

    void addParamInt(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var(), InteractionFn onInteractionFn = nullptr)
    {
        objectParameters.push_back(makeParam(pString, tInt, pCat, pVal, StringArray(), pDefault, nullptr, onInteractionFn));
    }

    void addParamBool(String const& pString, ParameterCategory pCat, Value* pVal, StringArray const& pList, var const& pDefault = var())
    {
        objectParameters.push_back(makeParam(pString, tBool, pCat, pVal, pList, pDefault));
    }

    void addParamString(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.push_back(makeParam(pString, tString, pCat, pVal, StringArray(), pDefault));
    }

    void addParamColour(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.push_back(makeParam(pString, tColour, pCat, pVal, StringArray(), pDefault));
    }

    void addParamColourFG(Value* pVal)
    {
        objectParameters.push_back(makeParam("Foreground color", tColour, cAppearance, pVal, StringArray(), PlugDataColour::canvasTextColourId));
    }

    void addParamColourBG(Value* pVal)
    {
        objectParameters.push_back(makeParam("Background color", tColour, cAppearance, pVal, StringArray(), PlugDataColour::guiObjectBackgroundColourId));
    }

    void addParamColourLabel(Value* pVal)
    {
        objectParameters.push_back(makeParam("Label color", tColour, cLabel, pVal, StringArray(), PlugDataColour::canvasTextColourId));
    }

    void addParamReceiveSymbol(Value* pVal)
    {
        objectParameters.push_back(makeParam("Receive Symbol", tString, cGeneral, pVal, StringArray(), ""));
    }

    void addParamSendSymbol(Value* pVal, String const& pDefault = "")
    {
        objectParameters.push_back(makeParam("Send Symbol", tString, cGeneral, pVal, StringArray(), pDefault));
    }

    void addParamCombo(String const& pString, ParameterCategory pCat, Value* pVal, StringArray const& pStringList, var const& pDefault = var())
    {
        objectParameters.push_back(makeParam(pString, tCombo, pCat, pVal, pStringList, pDefault));
    }

    void addParamRange(String const& pString, ParameterCategory pCat, Value* pVal, Array<var> const& pDefault = Array<var>())
    {
        objectParameters.push_back(makeParam(pString, tRangeFloat, pCat, pVal, StringArray(), pDefault));
    }

    void addParamFont(String const& pString, ParameterCategory pCat, Value* pVal, String const& pDefault = String())
    {
        objectParameters.push_back(makeParam(pString, tFont, pCat, pVal, StringArray(), pDefault));
    }

    void addParamPosition(Value* positionValue)
    {
        objectParameters.push_back(makeParam("Position", tRangeInt, cDimensions, positionValue, StringArray(), var()));
    }

    void addParamSize(Value* sizeValue, bool singleDimension = false)
    {
        objectParameters.push_back(makeParam("Size", singleDimension ? tInt : tRangeInt, cDimensions, sizeValue, StringArray(), var()));
    }

    void addParamCustom(CustomPanelCreateFn customComponentFn)
    {
        objectParameters.push_back(makeParam("", tCustom, cGeneral, nullptr, StringArray(), var(), customComponentFn));
    }

private:
    SmallVector<ObjectParameter> objectParameters;

    static ObjectParameter makeParam(String const& pString, ParameterType pType, ParameterCategory pCat, Value* pVal, StringArray const& pStringList, var const& pDefault, CustomPanelCreateFn customComponentFn = nullptr, InteractionFn onInteractionFn = nullptr)
    {
        return std::make_tuple(pString, pType, pCat, pVal, pStringList, pDefault, customComponentFn, onInteractionFn);
    }
};
