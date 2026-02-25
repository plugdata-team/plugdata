#pragma once
/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

class ObjectInfoPanel final : public Component {
    struct CategoryPanel final : public Component {
        CategoryPanel(String const& name, SmallArray<std::pair<String, String>> const& content)
            : categoryName(name)
            , panelContent(content)
        {
        }

        void paint(Graphics& g) override
        {
            g.setColour(PlugDataColours::outlineColour);
            g.drawLine(0, 1, getWidth(), 0);

            Fonts::drawStyledText(g, categoryName, getLocalBounds().toFloat().removeFromTop(24).translated(2, 0), PlugDataColours::panelTextColour, FontStyle::Bold, 14.0f);

            float totalHeight = 24;
            for (int i = 0; i < panelContent.size(); i++) {
                auto const textHeight = layouts[i].getHeight();

                auto bounds = Rectangle<float>(36.0f, totalHeight + 6.0f, getWidth() - 48.0f, textHeight);
                auto const nameWidth = std::max(Fonts::getStringWidthInt(panelContent[i].first, Fonts::getSemiBoldFont()), 64);

                Fonts::drawStyledText(g, panelContent[i].first, bounds.removeFromLeft(nameWidth), PlugDataColours::panelTextColour, FontStyle::Semibold, 13.5f);

                layouts[i].draw(g, bounds);

                g.setColour(PlugDataColours::outlineColour);
                g.drawLine(36.0f, totalHeight, getWidth() - 24.0f, totalHeight);

                totalHeight += textHeight + 12;
            }
        }

        void recalculateLayout(int const width)
        {
            layouts.clear();

            int totalHeight = 24;
            for (auto const& [name, description] : panelContent) {
                auto const nameWidth = std::max(Fonts::getStringWidthInt(name, Fonts::getSemiBoldFont()), 64);

                AttributedString str;

                auto lines = StringArray::fromLines(description);

                // Draw anything between () as bold
                for (auto const& line : lines) {
                    if (line.contains("(") && line.contains(")")) {
                        auto const type = line.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
                        auto const description = line.fromFirstOccurrenceOf(")", false, false);
                        str.append(type + ":", Fonts::getSemiBoldFont().withHeight(13.5f), PlugDataColours::panelTextColour);

                        str.append(description + "\n", Font(FontOptions(13.5f)), PlugDataColours::panelTextColour);
                    } else {
                        str.append(line, Font(FontOptions(13.5f)), PlugDataColours::panelTextColour);
                    }
                }

                TextLayout layout;
                layout.createLayout(str, width - (nameWidth + 64.0f));
                layouts.add(layout);
                totalHeight += layout.getHeight() + 12;
            }

            setSize(width, totalHeight);
        }

        String const categoryName;
        SmallArray<std::pair<String, String>> const panelContent;
        SmallArray<TextLayout> layouts;
    };

public:
    ObjectInfoPanel()
    {
        categoriesViewport.setViewedComponent(&categoriesHolder, false);
        addAndMakeVisible(categoriesViewport);
        categoriesViewport.setScrollBarsShown(true, false, false, false);
        categoriesHolder.setVisible(true);
    }

    void showObject(ValueTree const& objectInfo)
    {
        categories.clear();

        SmallArray<std::pair<String, String>> inletInfo;
        SmallArray<std::pair<String, String>> outletInfo;

        auto ioletDescriptions = objectInfo.getChildWithName("iolets");
        for (auto iolet : ioletDescriptions) {
            if (iolet.getType() == Identifier("inlet")) {
                auto tooltip = iolet.getProperty("tooltip").toString();
                inletInfo.add({ String(inletInfo.size() + 1), tooltip });
            } else {
                auto tooltip = iolet.getProperty("tooltip").toString();
                outletInfo.add({ String(outletInfo.size() + 1), tooltip });
            }
        }

        if (inletInfo.size()) {
            categories.add(new CategoryPanel("Inlets", inletInfo));
        }
        if (outletInfo.size()) {
            categories.add(new CategoryPanel("Outlets", outletInfo));
        }

        SmallArray<std::pair<String, String>> argsInfo;

        auto arguments = objectInfo.getChildWithName("arguments");
        for (auto argument : arguments) {
            auto type = argument.getProperty("type").toString();
            auto desc = argument.getProperty("description").toString();
            auto def = argument.getProperty("default").toString();
            String argumentDescription;

            argumentDescription += type.isNotEmpty() ? "(" + type + ") " : "";
            argumentDescription += desc;
            argumentDescription += def.isNotEmpty() && !desc.contains("(default") ? " (default: " + def + ")" : "";
            argsInfo.add({ String(argsInfo.size() + 1), argumentDescription });
        }

        if (argsInfo.size()) {
            categories.add(new CategoryPanel("Arguments", argsInfo));
        }

        SmallArray<std::pair<String, String>> methodsInfo;

        auto methods = objectInfo.getChildWithName("methods");
        for (auto method : methods) {
            auto type = method.getProperty("type").toString();
            auto description = method.getProperty("description").toString();
            methodsInfo.add({ type, description });
        }

        if (methodsInfo.size()) {
            categories.add(new CategoryPanel("Methods", methodsInfo));
        }

        SmallArray<std::pair<String, String>> flagsInfo;

        auto flags = objectInfo.getChildWithName("flags");

        for (auto flag : flags) {
            auto name = flag.getProperty("name").toString().trim();
            auto description = flag.getProperty("description").toString();
            auto def = flag.getProperty("default").toString();

            if (def.isNotEmpty())
                description += " (default: " + def + ")";

            if (!name.startsWith("-"))
                name = "- " + name;

            flagsInfo.add({ name, description });
        }

        if (flagsInfo.size()) {
            categories.add(new CategoryPanel("Flags", flagsInfo));
        }

        for (auto* category : categories) {
            categoriesHolder.addAndMakeVisible(category);
        }
    }

    void resized() override
    {
        categoriesViewport.setBounds(getLocalBounds());

        int totalHeight = 24;
        for (auto* category : categories) {
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

class ObjectReferenceDialog final : public Component {

public:
    ObjectReferenceDialog(PluginEditor const* editor, bool const showBackButton)
        : library(*editor->pd->objectLibrary)
    {
        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);

        if (showBackButton) {
            addAndMakeVisible(backButton);
        }

        backButton.onClick = [this] {
            setVisible(false);
        };

        addAndMakeVisible(objectInfoPanel);
    }

    void resized() override
    {
        backButton.setBounds(2, 0, 40, 40);

        auto const rightPanelBounds = getLocalBounds().withTrimmedTop(40).removeFromRight(getLocalBounds().proportionOfWidth(0.65f)).reduced(20, 0);

        objectInfoPanel.setBounds(rightPanelBounds);
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto const titlebarBounds = getLocalBounds().removeFromTop(40).toFloat();

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(p);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(40, 0.0f, getWidth());

        if (objectName.isEmpty())
            return;

        auto leftPanelBounds = getLocalBounds().withTrimmedRight(getLocalBounds().proportionOfWidth(0.65f));

        g.drawVerticalLine(leftPanelBounds.getRight(), 40.0f, getHeight() - 40.0f);

        auto infoBounds = leftPanelBounds.withTrimmedBottom(100).withTrimmedTop(140).withTrimmedLeft(5).reduced(10);
        auto const objectDisplayBounds = leftPanelBounds.removeFromTop(140);

        Fonts::drawStyledText(g, "Object Reference:  " + objectName, getLocalBounds().removeFromTop(35).translated(0, 4), PlugDataColours::panelTextColour, Bold, 16, Justification::centred);

        auto const colour = PlugDataColours::panelTextColour;

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray const infoNames = { "Categories:", "Origin:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray const infoText = { categories, origin, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

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
            auto const questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            Fonts::drawText(g, "?", questionMarkBounds, colour, 40, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> const objectRect)
    {
        constexpr int ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max<int>(inlets.size(), outlets.size());
        int const textWidth = Fonts::getStringWidthInt(objectName, 15);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto const outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(PlugDataColours::objectOutlineColour);
        g.drawRoundedRectangle(outlineBounds, Corners::objectCornerRadius, 1.0f);

        auto const textBounds = outlineBounds.reduced(2.0f);
        Fonts::drawText(g, objectName, textBounds.toNearestInt(), PlugDataColours::panelTextColour, 15, Justification::centred);

        auto const themeTree = SettingsFile::getInstance()->getCurrentTheme();

        auto squareIolets = static_cast<bool>(themeTree.getProperty("square_iolets"));

        auto drawIolet = [squareIolets](Graphics& g, Rectangle<float> const bounds, bool const type) mutable {
            g.setColour(type ? PlugDataColours::signalColour : PlugDataColours::dataColour);

            if (squareIolets) {
                g.fillRect(bounds);

                g.setColour(PlugDataColours::objectOutlineColour);
                g.drawRect(bounds, 1.0f);
            } else {

                g.fillEllipse(bounds);

                g.setColour(PlugDataColours::objectOutlineColour);
                g.drawEllipse(bounds, 1.0f);
            }
        };

        auto const ioletBounds = outlineBounds.reduced(8, 0);

        for (int i = 0; i < inlets.size(); i++) {
            auto inletBounds = Rectangle<int>();

            int const total = inlets.size();
            float const yPosition = ioletBounds.getY() + 1 - ioletSize / 2.0f;

            if (total == 1 && i == 0) {
                int const xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

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
                int const xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

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

        bool const valid = name.isNotEmpty();

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            repaint();
            return;
        }

        bool hasUnknownInletLayout = false;
        bool hasUnknownOutletLayout = false;

        auto const objectInfo = library.getObjectInfo(name);
        if (!objectInfo.isValid())
            return;

        auto const ioletDescriptions = objectInfo.getChildWithName("iolets");
        for (auto iolet : ioletDescriptions) {
            auto const variable = iolet.getProperty("variable").toString() == "1";

            if (iolet.getType() == Identifier("inlet")) {
                if (variable)
                    hasUnknownInletLayout = true;
                auto tooltip = iolet.getProperty("tooltip").toString();
                inlets.add(tooltip.contains("(signal)"));
            } else {
                if (variable)
                    hasUnknownOutletLayout = true;
                auto tooltip = iolet.getProperty("tooltip").toString();
                outlets.add(tooltip.contains("(signal)"));
            }
        }

        unknownInletLayout = hasUnknownInletLayout;
        unknownOutletLayout = hasUnknownOutletLayout;

        objectName = name;
        categories = "";
        origin = "";

        auto const categoriesTree = objectInfo.getChildWithName("categories");

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
        objectInfoPanel.resized();
    }

    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    SmallArray<bool> inlets;
    SmallArray<bool> outlets;

    ObjectInfoPanel objectInfoPanel;

    MainToolbarButton backButton = MainToolbarButton(Icons::Back);

    String categories;
    String origin;
    String description;

    pd::Library& library;
};
