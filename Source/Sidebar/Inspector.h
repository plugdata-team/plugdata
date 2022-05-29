/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

struct Inspector : public PropertiesPanel
{
    PropertyComponent* createPanel (int type, const String& name, Value* value, int idx, std::vector<String>& options)
    {
        switch (type)
        {
            case tString:
                return new EditableComponent<String> (name, *value, idx);
            case tFloat:
                return new EditableComponent<float> (name, *value, idx);
            case tInt:
                return new EditableComponent<int> (name, *value, idx);
            case tColour:
                return new ColourComponent (name, *value, idx);
            case tBool:
                return new BoolComponent (name, *value, idx, options);
            case tCombo:
                return new ComboComponent (name, *value, idx, options);
            case tRange:
                return new RangeComponent (name, *value, idx);
            default:
                return new EditableComponent<String> (name, *value, idx);
        }
    }

    void loadParameters (ObjectParameters& params)
    {
        StringArray names = { "General", "Appearance", "Label", "Extra" };

        clear();

        for (int i = 0; i < 4; i++)
        {
            Array<PropertyComponent*> panels;

            int idx = 0;
            for (auto& [name, type, category, value, options] : params)
            {
                if (static_cast<int> (category) == i)
                {
                    panels.add (createPanel (type, name, value, idx, options));
                    idx++;
                }
            }
            if (! panels.isEmpty())
            {
                addSection (names[i], panels);
            }
        }
    }
};
