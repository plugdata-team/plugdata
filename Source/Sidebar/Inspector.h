/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

class Inspector : public Component {

    PropertiesPanel panel;
    String title;

public:
    Inspector()
    {
        addAndMakeVisible(panel);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(getLocalBounds().withTrimmedBottom(30));

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().removeFromBottom(30).toFloat(), Corners::windowCornerRadius);

        Fonts::drawText(g, title, getLocalBounds().removeFromTop(23), findColour(PlugDataColour::sidebarTextColourId), 15, Justification::centred);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, 23, getWidth(), 23);
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().withTrimmedTop(28));
    }

    void setTitle(String const& name)
    {
        title = name;
    }

    PropertyComponent* createPanel(int type, String const& name, Value* value, std::vector<String>& options)
    {
        switch (type) {
        case tString:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        case tFloat:
            return new PropertiesPanel::EditableComponent<float>(name, *value);
        case tInt:
            return new PropertiesPanel::EditableComponent<int>(name, *value);
        case tColour:
            return new PropertiesPanel::ColourComponent(name, *value);
        case tBool:
            return new PropertiesPanel::BoolComponent(name, *value, options);
        case tCombo:
            return new PropertiesPanel::ComboComponent(name, *value, options);
        case tRange:
            return new PropertiesPanel::RangeComponent(name, *value);
        default:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        }
    }

    void loadParameters(ObjectParameters& params)
    {
        StringArray names = { "General", "Appearance", "Label", "Extra" };

        panel.clear();

        for (int i = 0; i < 4; i++) {
            Array<PropertyComponent*> panels;

            int idx = 0;
            for (auto& [name, type, category, value, options] : params) {
                if (static_cast<int>(category) == i) {
                    panels.add(createPanel(type, name, value, options));
                    idx++;
                }
            }
            if (!panels.isEmpty()) {
                panel.addSection(names[i], panels);
            }
        }
    }
};
