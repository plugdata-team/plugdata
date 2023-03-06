#pragma once
/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ObjectReferenceDialog : public Component {

public:
    ObjectReferenceDialog(PluginEditor* editor, bool showBackButton)
        : library(editor->pd->objectLibrary)
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

        addAndMakeVisible(rightSideInfo);
        rightSideInfo.setReadOnly(true);
        rightSideInfo.setMultiLine(true);
        rightSideInfo.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        rightSideInfo.setFont(Font(15));

        backButton.getProperties().set("Style", "LargeIcon");
    }

    void resized() override
    {
        backButton.setBounds(2, 0, 55, 55);

        auto buttonBounds = getLocalBounds().removeFromBottom(80).reduced(30, 0).translated(0, -30);
        buttonBounds.removeFromTop(10);

        auto rightPanelBounds = getLocalBounds().withTrimmedLeft(getLocalBounds().proportionOfWidth(0.4f));
        rightPanelBounds = rightPanelBounds.withTrimmedTop(20).withTrimmedBottom(20).reduced(20);

        rightSideInfo.setBounds(rightPanelBounds);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), PlugDataLook::windowCornerRadius);

        if (objectName.isEmpty())
            return;

        auto leftPanelBounds = getLocalBounds().withTrimmedRight(getLocalBounds().proportionOfWidth(0.6f));

        auto infoBounds = leftPanelBounds.withTrimmedBottom(100).withTrimmedTop(100).withTrimmedLeft(5).reduced(10);
        auto objectDisplayBounds = leftPanelBounds.removeFromTop(140);

        PlugDataLook::drawStyledText(g, "Reference: " + objectName, getLocalBounds().removeFromTop(35).translated(0, 4), findColour(PlugDataColour::panelTextColourId), Bold, 16, Justification::centred);

        auto colour = findColour(PlugDataColour::panelTextColourId);

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray infoNames = { "Categories:", "Origin:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray infoText = { categories, origin, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

        for (int i = 0; i < infoNames.size(); i++) {
            auto localBounds = infoBounds.removeFromTop(25);
            PlugDataLook::drawText(g, infoNames[i], localBounds.removeFromLeft(90), colour, 15, Justification::topLeft);
            PlugDataLook::drawText(g, infoText[i], localBounds, colour, 15, Justification::topLeft);
        }

        auto descriptionBounds = infoBounds.removeFromTop(25);
        PlugDataLook::drawText(g, "Description: ", descriptionBounds.removeFromLeft(90), colour, 15, Justification::topLeft);

        PlugDataLook::drawFittedText(g, description, descriptionBounds.withHeight(180), colour, 10, 0.9f, 15, Justification::topLeft);

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, objectDisplayBounds);
        } else {
            auto questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            PlugDataLook::drawText(g, "?", questionMarkBounds, colour, 40, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> objectRect)
    {
        int const ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max(inlets.size(), outlets.size());
        int const textWidth = Font(15).getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, PlugDataLook::objectCornerRadius, 1.0f);

        auto textBounds = outlineBounds.reduced(2.0f);
        PlugDataLook::drawText(g, objectName, textBounds.toNearestInt(), findColour(PlugDataColour::panelTextColourId), 15, Justification::centred);

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

    void showObject(String name)
    {
        bool valid = name.isNotEmpty();

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            inlets.clear();
            outlets.clear();
            repaint();
            return;
        }

        auto const ioletDescriptions = library.getIoletDescriptions()[name];
        auto const& inletDescriptions = ioletDescriptions[0];
        auto const& outletDescriptions = ioletDescriptions[1];
        auto methods = library.getMethods()[name];

        inlets.resize(inletDescriptions.size());
        outlets.resize(outletDescriptions.size());

        bool hasUnknownInletLayout = false;

        for (int i = 0; i < inlets.size(); i++) {
            inlets[i] = inletDescriptions[i].first.contains("(signal)");
            if (inletDescriptions[i].second)
                hasUnknownInletLayout = true;
        }

        bool hasUnknownOutletLayout = false;
        for (int i = 0; i < outlets.size(); i++) {
            outlets[i] = outletDescriptions[i].first.contains("(signal)");
            if (outletDescriptions[i].second)
                hasUnknownOutletLayout = true;
        }

        unknownInletLayout = hasUnknownInletLayout;
        unknownOutletLayout = hasUnknownOutletLayout;

        objectName = name;
        categories = "";
        origin = "";

        // Inverse lookup :(
        for (auto const& [cat, objects] : library.getObjectCategories()) {
            if (pd::Library::objectOrigins.contains(cat) && objects.contains(name)) {
                origin = cat;
            } else if (objects.contains(name)) {
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

        description = library.getObjectDescriptions()[name];

        if (description.isEmpty()) {
            description = "No description available";
        }

        setVisible(true);

        String rightSideInfoText;

        auto arguments = library.getArguments()[name];

        if (arguments.size())
            rightSideInfoText += "Arguments:";

        int numArgs = 1;

        for (auto [type, description, defaultValue] : arguments) {
            rightSideInfoText += "\n" + String(numArgs) + ": ";
            rightSideInfoText += type.isNotEmpty() ? "(" + type + ") " : "";
            rightSideInfoText += description;
            rightSideInfoText += defaultValue.isNotEmpty() && !description.contains("(default") ? " (default: " + defaultValue + ")" : "";

            numArgs++;
        }

        if (inletDescriptions.size())
            rightSideInfoText += "\n\nInlets:";

        int numIn = 1;
        for (auto [description, type] : inletDescriptions) {
            description = description.replace("\n", "\n      ");
            rightSideInfoText += "\n" + String(numIn) + ":\n    " + description;

            numIn++;
        }

        if (outletDescriptions.size())
            rightSideInfoText += "\n\nOutlets:";

        int numOut = 1;
        for (auto [description, type] : outletDescriptions) {
            description = description.replace("\n", "\n    ");
            rightSideInfoText += "\n" + String(numOut) + ":\n    " + description;

            numOut++;
        }

        if (methods.size())
            rightSideInfoText += "\n\nMethods:";

        int numMethods = 1;
        for (auto [type, description] : methods) {
            rightSideInfoText += "\n" + String(numMethods) + ": ";
            rightSideInfoText += type.isNotEmpty() ? "(" + type + ") " : "";
            rightSideInfoText += description;
            numMethods++;
        }

        rightSideInfo.setText(rightSideInfoText);
    }

    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;

    TextEditor rightSideInfo;

    TextButton backButton = TextButton(Icons::Back);

    String categories;
    String origin;
    String description;

    PluginEditor* editor;

    pd::Library& library;
};
