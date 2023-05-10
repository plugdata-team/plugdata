/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

class Inspector : public Component {

    PropertiesPanel panel;
    String title;
    TextButton resetButton;
    ObjectParameters properties;

public:
    Inspector()
    {
        resetButton.setButtonText(Icons::Reset);
        resetButton.getProperties().set("Style", "LargeIcon");
        resetButton.setTooltip("Reset to default");
        resetButton.setSize(23,23);

        addAndMakeVisible(resetButton);
        resetButton.onClick = [this](){
            for (auto [name, type, category, value, options, defaultVal] : properties) {
                if (!defaultVal.isVoid())
                    value->setValue(defaultVal);
            }
        };

        addAndMakeVisible(panel);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(getLocalBounds().withTrimmedBottom(30));

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().removeFromBottom(30).toFloat(), Corners::windowCornerRadius);

        Fonts::drawText(g, title, getLocalBounds().removeFromTop(23), findColour(PlugDataColour::sidebarTextColourId), 15, Justification::centred);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 23, getWidth(), 23);
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds().withTrimmedTop(28));
        resetButton.setTopLeftPosition(getLocalBounds().withTrimmedRight(23).getRight(), 0);
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
        case tFont:
            return new PropertiesPanel::FontComponent(name, *value);
        default:
            return new PropertiesPanel::EditableComponent<String>(name, *value);
        }
    }

    void loadParameters(ObjectParameters& params)
    {
        properties = params;

        StringArray names = { "General", "Appearance", "Label", "Extra" };

        panel.clear();

        for (int i = 0; i < 4; i++) {
            Array<PropertyComponent*> panels;
            int idx = 0;
            for (auto& [name, type, category, value, options, defaultVal] : params) {
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
