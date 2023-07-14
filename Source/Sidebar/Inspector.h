/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/PropertiesPanel.h"

class InspectorPanel : public PropertiesPanel
{

};

class Inspector : public Component {

    InspectorPanel panel;
    String title;
    TextButton resetButton;
    ObjectParameters properties;

public:
    Inspector()
    {
        panel.setTitleHeight(20);
        panel.setTitleAlignment(PropertiesPanel::AlignWithPropertyName);
        panel.setDrawShadowAndOutline(false);
        addAndMakeVisible(panel);
        lookAndFeelChanged();
    }
    
    void lookAndFeelChanged() override
    {
        panel.setSeparatorColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        panel.setPanelColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId).withAlpha(0.6f));
    }

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
    }

    void resized() override
    {
        panel.setBounds(getLocalBounds());
        resetButton.setTopLeftPosition(getLocalBounds().withTrimmedRight(23).getRight(), 0);
        
        panel.setContentWidth(getWidth() - 16);
    }

    void setTitle(String const& name)
    {
        title = name;
    }
    
    String getTitle()
    {
        return title;
    }

    static PropertiesPanel::Property* createPanel(int type, String const& name, Value* value, StringArray& options)
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
            Array<PropertiesPanel::Property*> panels;
            int idx = 0;
            for (auto& [name, type, category, value, options, defaultVal] : params.getParameters()) {
                if (static_cast<int>(category) == i) {
                    auto newPanel = createPanel(type, name, value, options);
                    newPanel->setPreferredHeight(26);
                    panels.add(newPanel);
                    idx++;
                }
            }
            if (!panels.isEmpty()) {
                panel.addSection(names[i], panels);
            }
        }
    }
    
    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* resetButton = new TextButton(Icons::Reset);
        resetButton->getProperties().set("Style", "SmallIcon");
        resetButton->setTooltip("Reset to default");
        resetButton->setSize(23, 23);
        resetButton->onClick = [this]() {
            properties.resetAll();
        };
        
        return std::unique_ptr<TextButton>(resetButton);
    }
};
