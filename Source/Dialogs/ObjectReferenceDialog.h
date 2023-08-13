#pragma once
/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ObjectInfoPanel : public Component
{
    struct CategoryPanel : public Component
    {
        CategoryPanel(const String& name, const Array<std::pair<String, String>>& content) : categoryName(name), panelContent(content)
        {
        }
        
        
        void paint(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawLine(0, 1, getWidth(), 0);
            
            Fonts::drawStyledText(g, categoryName, getLocalBounds().toFloat().removeFromTop(24), findColour(PlugDataColour::panelTextColourId), FontStyle::Bold, 14.0f);
            
            float totalHeight = 24;
            for(int i = 0; i < panelContent.size(); i++)
            {
                auto textHeight = layouts[i].getHeight();
                auto bounds = Rectangle<float>(24.0f, totalHeight + 6.0f, getWidth() - 48.0f, textHeight);
                
                Fonts::drawStyledText(g, panelContent[i].first, bounds.removeFromLeft(128), findColour(PlugDataColour::panelTextColourId), FontStyle::Semibold, 13.5f);
                
                layouts[i].draw(g, bounds);
                
   
                g.setColour(findColour(PlugDataColour::outlineColourId));
                g.drawLine(24.0f, totalHeight, getWidth() - 24.0f, totalHeight);
                
                totalHeight += textHeight + 12;
            }
        }

        
        void recalculateLayout(int width)
        {
            layouts.clear();
            
            int totalHeight = 24;
            for(const auto& [name, description] : panelContent)
            {
                AttributedString str;
                str.append(description, Font(13.5f), findColour(PlugDataColour::panelTextColourId));
                TextLayout layout;
                layout.createLayout(str, width - 192.0f);
                layouts.add(layout);
                totalHeight += layout.getHeight() + 12;
            }
            
            setSize(width, totalHeight);
        }
        
        const String categoryName;
        const Array<std::pair<String, String>> panelContent;
        Array<TextLayout> layouts;
    };
    
public:
    
    ObjectInfoPanel()
    {
        categoriesViewport.setViewedComponent(&categoriesHolder, false);
        addAndMakeVisible(categoriesViewport);
        categoriesViewport.setScrollBarsShown(true, false, false, false);
        categoriesHolder.setVisible(true);
    }
    
    
    void showObject(ValueTree objectInfo)
    {
        categories.clear();
        
        Array<std::pair<String, String>> inletInfo;
        Array<std::pair<String, String>> outletInfo;
        
        auto ioletDescriptions = objectInfo.getChildWithName("iolets");
        for (auto iolet : ioletDescriptions) {
            auto variable = iolet.getProperty("variable").toString() == "1";

            if (iolet.getType() == Identifier("inlet")) {
                auto tooltip = iolet.getProperty("tooltip").toString();
                inletInfo.add({String(inletInfo.size() + 1), tooltip});
            } else {
                auto tooltip = iolet.getProperty("tooltip").toString();
                outletInfo.add({String(outletInfo.size() + 1), tooltip});
            }
        }
        
        if(inletInfo.size()) {
            categories.add(new CategoryPanel("Inlets", inletInfo));
        }
        if(outletInfo.size()) {
            categories.add(new CategoryPanel("Outlets", outletInfo));
        }


        Array<std::pair<String, String>> argsInfo;

        auto arguments = objectInfo.getChildWithName("arguments");
        for (auto argument : arguments) {
            auto type = argument.getProperty("type").toString();
            auto desc = argument.getProperty("description").toString();
            auto def = argument.getProperty("default").toString();
            String argumentDescription;
            
            argumentDescription += type.isNotEmpty() ? "(" + type + ") " : "";
            argumentDescription += desc;
            argumentDescription += def.isNotEmpty() && !desc.contains("(default") ? " (default: " + def + ")" : "";
            argsInfo.add({String(argsInfo.size() + 1), argumentDescription});
        }

        if(argsInfo.size()) {
            categories.add(new CategoryPanel("Arguments", argsInfo));
        }
        
        Array<std::pair<String, String>> methodsInfo;
        
        auto methods = objectInfo.getChildWithName("methods");
        for (auto method : methods) {
            auto type = method.getProperty("type").toString();
            auto description = method.getProperty("description").toString();
            methodsInfo.add({type, description});
        }
        
        if(methodsInfo.size()) {
            categories.add(new CategoryPanel("Methods", methodsInfo));
        }

        Array<std::pair<String, String>> flagsInfo;
        
        auto flags = objectInfo.getChildWithName("flags");

        for (auto flag : flags) {
            auto name = flag.getProperty("name").toString().trim();
            auto description = flag.getProperty("description").toString();

            if (!name.startsWith("-"))
                name = "- " + name;

            flagsInfo.add({name, description});
        }
        
        if(flagsInfo.size()) {
            categories.add(new CategoryPanel("Flags", flagsInfo));
        }
        
        
        for(auto* category : categories)
        {
            categoriesHolder.addAndMakeVisible(category);
        }
    }
    
    void resized() override
    {
        categoriesViewport.setBounds(getLocalBounds());
        
        int totalHeight = 0;
        for(auto* category : categories)
        {
            category->recalculateLayout(getWidth());
            category->setTopLeftPosition(0, totalHeight);
            totalHeight += category->getHeight();
        }
        
        categoriesHolder.setSize(getWidth(), totalHeight);
    }
    
    BouncingViewport categoriesViewport;
    Component categoriesHolder;
    OwnedArray<CategoryPanel> categories;
};

class ObjectReferenceDialog : public Component {

public:
    ObjectReferenceDialog(PluginEditor* editor, bool showBackButton)
        : library(*editor->pd->objectLibrary)
    {
        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);

        if (showBackButton) {
            addAndMakeVisible(backButton);
        }

        backButton.onClick = [this]() {
            setVisible(false);
        };

        backButton.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        backButton.setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        backButton.getProperties().set("Style", "LargeIcon");
        
        addAndMakeVisible(objectInfoPanel);
    }

    void resized() override
    {
        backButton.setBounds(8, 6, 42, 48);

        auto buttonBounds = getLocalBounds().removeFromBottom(80).reduced(30, 0).translated(0, -30);
        buttonBounds.removeFromTop(10);

        auto rightPanelBounds = getLocalBounds().removeFromRight(getLocalBounds().proportionOfWidth(0.7f)).reduced(20, 40);

        objectInfoPanel.setBounds(rightPanelBounds);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        if (objectName.isEmpty())
            return;

        auto leftPanelBounds = getLocalBounds().withTrimmedRight(getLocalBounds().proportionOfWidth(0.7f));

        auto infoBounds = leftPanelBounds.withTrimmedBottom(100).withTrimmedTop(100).withTrimmedLeft(5).reduced(10);
        auto objectDisplayBounds = leftPanelBounds.removeFromTop(140);

        Fonts::drawStyledText(g, "Reference: " + objectName, getLocalBounds().removeFromTop(35).translated(0, 4), findColour(PlugDataColour::panelTextColourId), Bold, 16, Justification::centred);

        auto colour = findColour(PlugDataColour::panelTextColourId);

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray infoNames = { "Categories:", "Origin:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray infoText = { categories, origin, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

        for (int i = 0; i < infoNames.size(); i++) {
            auto localBounds = infoBounds.removeFromTop(25);
            Fonts::drawText(g, infoNames[i], localBounds.removeFromLeft(90), colour, 15, Justification::topLeft);
            Fonts::drawText(g, infoText[i], localBounds, colour, 15, Justification::topLeft);
        }

        auto descriptionBounds = infoBounds.removeFromTop(25);
        Fonts::drawText(g, "Description: ", descriptionBounds.removeFromLeft(90), colour, 15, Justification::topLeft);

        Fonts::drawFittedText(g, description, descriptionBounds.withHeight(180), colour, 10, 0.9f, 15, Justification::topLeft);

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, objectDisplayBounds);
        } else {
            auto questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            Fonts::drawText(g, "?", questionMarkBounds, colour, 40, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> objectRect)
    {
        int const ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max<int>(inlets.size(), outlets.size());
        int const textWidth = Font(15).getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, Corners::objectCornerRadius, 1.0f);

        auto textBounds = outlineBounds.reduced(2.0f);
        Fonts::drawText(g, objectName, textBounds.toNearestInt(), findColour(PlugDataColour::panelTextColourId), 15, Justification::centred);

        auto themeTree = SettingsFile::getInstance()->getCurrentTheme();

        auto squareIolets = static_cast<bool>(themeTree.getProperty("square_iolets"));

        auto drawIolet = [this, squareIolets](Graphics& g, Rectangle<float> bounds, bool type) mutable {
            g.setColour(type ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));

            if (squareIolets) {
                g.fillRect(bounds);

                g.setColour(findColour(PlugDataColour::objectOutlineColourId));
                g.drawRect(bounds, 1.0f);
            } else {

                g.fillEllipse(bounds);

                g.setColour(findColour(PlugDataColour::objectOutlineColourId));
                g.drawEllipse(bounds, 1.0f);
            }
        };

        auto ioletBounds = outlineBounds.reduced(8, 0);

        for (int i = 0; i < inlets.size(); i++) {
            auto inletBounds = Rectangle<int>();

            int const total = inlets.size();
            float const yPosition = ioletBounds.getY() + 1 - ioletSize / 2.0f;

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

                inletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);
            } else if (total > 1) {
                float const ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);

                inletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }

            drawIolet(g, inletBounds.toFloat(), inlets[i]);
        }

        for (int i = 0; i < outlets.size(); i++) {

            auto outletBounds = Rectangle<int>();
            int const total = outlets.size();
            float const yPosition = ioletBounds.getBottom() - ioletSize / 2.0f;

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

                outletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);

            } else if (total > 1) {
                float const ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
                outletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }

            drawIolet(g, outletBounds.toFloat(), outlets[i]);
        }
    }

    void showObject(String const& name)
    {
        inlets.clear();
        outlets.clear();

        bool valid = name.isNotEmpty();

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            repaint();
            return;
        }

        bool hasUnknownInletLayout = false;
        bool hasUnknownOutletLayout = false;

        auto objectInfo = library.getObjectInfo(name);
        auto ioletDescriptions = objectInfo.getChildWithName("iolets");
        for (auto iolet : ioletDescriptions) {
            auto variable = iolet.getProperty("variable").toString() == "1";

            if (iolet.getType() == Identifier("inlet")) {
                if (variable)
                    hasUnknownInletLayout = true;
                auto tooltip = iolet.getProperty("tooltip").toString();
                inlets.push_back(tooltip.contains("(signal)"));
            } else {
                if (variable)
                    hasUnknownOutletLayout = true;
                auto tooltip = iolet.getProperty("tooltip").toString();
                outlets.push_back(tooltip.contains("(signal)"));
            }
        }

        unknownInletLayout = hasUnknownInletLayout;
        unknownOutletLayout = hasUnknownOutletLayout;

        objectName = name;
        categories = "";
        origin = "";

        auto categoriesTree = objectInfo.getChildWithName("categories");

        for (auto category : categoriesTree) {
            auto cat = category.getProperty("name").toString();
            if (pd::Library::objectOrigins.contains(cat)) {
                origin = cat;
            } else {
                categories += cat + ", ";
            }
        }

        if (categories.isEmpty()) {
            categories = "Unknown";
        } else {
            categories = categories.dropLastCharacters(2);
        }

        if (origin.isEmpty()) {
            origin = "Unknown";
        }

        description = objectInfo.getProperty("description");

        if (description.isEmpty()) {
            description = "No description available";
        }

        setVisible(true);
        
        objectInfoPanel.showObject(objectInfo);
        
    }

    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;

    ObjectInfoPanel objectInfoPanel;

    TextButton backButton = TextButton(Icons::Back);

    String categories;
    String origin;
    String description;

    pd::Library& library;
};
