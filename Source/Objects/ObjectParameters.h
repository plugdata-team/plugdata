#pragma once

#include <juce_data_structures/juce_data_structures.h>
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
    cGeneral,
    cAppearance,
    cLabel,
    cExtra
};

using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, std::vector<String>, var>;

class ObjectParameters {
public:
    ObjectParameters(){};

    Array<ObjectParameter> getParameters()
    {
        return objectParameters;
    }

    void addParam(ObjectParameter param)
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

    void addParamFloat(String pString, ParameterCategory pCat, Value* pVal, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tFloat, pCat, pVal, std::vector<String>(), pDefault));
    }

    void addParamInt(String pString, ParameterCategory pCat, Value* pVal, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tInt, pCat, pVal, std::vector<String>(), pDefault));
    }

    void addParamBool(String pString, ParameterCategory pCat, Value* pVal, std::vector<String> pList, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tBool, pCat, pVal, pList, pDefault));
    }

    void addParamString(String pString, ParameterCategory pCat, Value* pVal, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tString, pCat, pVal, std::vector<String>(), pDefault));
    }

    void addParamColour(String pString, ParameterCategory pCat, Value* pVal, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tColour, pCat, pVal, std::vector<String>(), pDefault));
    }

    void addParamColourFG(Value* pVal)
    {
        objectParameters.add(makeParam("Foreground color", tColour, cAppearance, pVal, std::vector<String>(), PlugDataColour::canvasTextColourId));
    }

    void addParamColourBG(Value* pVal)
    {
        objectParameters.add(makeParam("Foreground color", tColour, cAppearance, pVal, std::vector<String>(), PlugDataColour::guiObjectBackgroundColourId));
    }

    void addParamColourLabel(Value* pVal)
    {
        objectParameters.add(makeParam("Label color", tColour, cLabel, pVal, std::vector<String>(), PlugDataColour::canvasTextColourId));
    }

    void addParamReceiveSymbol(Value* pVal)
    {
        objectParameters.add(makeParam("Receive Symbol", tString, cGeneral, pVal, std::vector<String>(), ""));
    }

    void addParamSendSymbol(Value* pVal, String pDefault = "")
    {
        objectParameters.add(makeParam("Send Symbol", tString, cGeneral, pVal, std::vector<String>(), pDefault));
    }

    void addParamCombo(String pString, ParameterCategory pCat, Value* pVal, std::vector<String> pStringList, var pDefault = var())
    {
        objectParameters.add(makeParam(pString, tCombo, pCat, pVal, pStringList, pDefault));
    }

    void addParamRange(String pString, ParameterCategory pCat, Value* pVal, Array<var> pDefault = Array<var>())
    {
        objectParameters.add(makeParam(pString, tRange, pCat, pVal, std::vector<String>(), pDefault));
    }

    void addParamFont(String pString, ParameterCategory pCat, Value* pVal, String pDefault = String())
    {
        objectParameters.add(makeParam(pString, tFont, pCat, pVal, std::vector<String>(), pDefault));
    }

private:
    Array<ObjectParameter> objectParameters;
    
    ObjectParameter makeParam(String pString, ParameterType pType, ParameterCategory pCat, Value* pVal, std::vector<String> pStringList, var pDefault)
    {
        return std::make_tuple(pString, pType, pCat, pVal, pStringList, pDefault);
    }
};
