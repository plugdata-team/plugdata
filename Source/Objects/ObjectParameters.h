#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <utility>
#include "../LookAndFeel.h"

enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool,
    tCombo,
    tRange,
    tFont
};

enum ParameterCategory {
    cDimensions,
    cGeneral,
    cAppearance,
    cLabel,
    cExtra,
};

using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, StringArray, var>;

class ObjectParameters {
public:
    ObjectParameters() = default;

    Array<ObjectParameter> getParameters()
    {
        return objectParameters;
    }

    void addParam(ObjectParameter const& param)
    {
        objectParameters.add(param);
    }

    void resetAll()
    {
        auto& lnf = LookAndFeel::getDefaultLookAndFeel();
        for (auto [name, type, category, value, options, defaultVal] : objectParameters) {
            if (!defaultVal.isVoid()) {
                if (type == tColour) {
                    value->setValue(lnf.findColour(defaultVal).toString());
                } else {
                    value->setValue(defaultVal);
                }
            }
        }
    }

    // ========= overloads for making different types of parameters =========

    void addParamFloat(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tFloat, pCat, pVal, StringArray(), pDefault));
    }

    void addParamInt(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tInt, pCat, pVal, StringArray(), pDefault));
    }

    void addParamBool(String const& pString, ParameterCategory pCat, Value* pVal, StringArray const& pList, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tBool, pCat, pVal, pList, pDefault));
    }

    void addParamString(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tString, pCat, pVal, StringArray(), pDefault));
    }

    void addParamColour(String const& pString, ParameterCategory pCat, Value* pVal, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tColour, pCat, pVal, StringArray(), pDefault));
    }

    void addParamColourFG(Value* pVal)
    {
        objectParameters.add(makeParam("Foreground color", tColour, cAppearance, pVal, StringArray(), PlugDataColour::canvasTextColourId));
    }

    void addParamColourBG(Value* pVal)
    {
        objectParameters.add(makeParam("Background color", tColour, cAppearance, pVal, StringArray(), PlugDataColour::guiObjectBackgroundColourId));
    }

    void addParamColourLabel(Value* pVal)
    {
        objectParameters.add(makeParam("Label color", tColour, cLabel, pVal, StringArray(), PlugDataColour::canvasTextColourId));
    }

    void addParamReceiveSymbol(Value* pVal)
    {
        objectParameters.add(makeParam("Receive Symbol", tString, cGeneral, pVal, StringArray(), ""));
    }

    void addParamSendSymbol(Value* pVal, String const& pDefault = "")
    {
        objectParameters.add(makeParam("Send Symbol", tString, cGeneral, pVal, StringArray(), pDefault));
    }

    void addParamCombo(String const& pString, ParameterCategory pCat, Value* pVal, StringArray const& pStringList, var const& pDefault = var())
    {
        objectParameters.add(makeParam(pString, tCombo, pCat, pVal, pStringList, pDefault));
    }

    void addParamRange(String const& pString, ParameterCategory pCat, Value* pVal, Array<var> const& pDefault = Array<var>())
    {
        objectParameters.add(makeParam(pString, tRange, pCat, pVal, StringArray(), pDefault));
    }

    void addParamFont(String const& pString, ParameterCategory pCat, Value* pVal, String const& pDefault = String())
    {
        objectParameters.add(makeParam(pString, tFont, pCat, pVal, StringArray(), pDefault));
    }
    
    void addParamPosition(Value* positionValue)
    {        
        objectParameters.add(makeParam("Position", tRange, cDimensions, positionValue, StringArray(), var()));
    }
    

private:
    Array<ObjectParameter> objectParameters;

    static ObjectParameter makeParam(String const& pString, ParameterType pType, ParameterCategory pCat, Value* pVal, StringArray const& pStringList, var const& pDefault)
    {
        return std::make_tuple(pString, pType, pCat, pVal, pStringList, pDefault);
    }
};
