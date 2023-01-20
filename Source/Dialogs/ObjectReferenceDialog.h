#pragma once
/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct ObjectReferenceDialog : public Component {

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
        rightSideInfo.setFont(Font(15.0f));

        backButton.setName("toolbar:backbutton");
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

        auto const font = Font(15.0f);

        if (objectName.isEmpty())
            return;

        auto leftPanelBounds = getLocalBounds().withTrimmedRight(getLocalBounds().proportionOfWidth(0.6f));

        auto infoBounds = leftPanelBounds.withTrimmedBottom(100).withTrimmedTop(100).withTrimmedLeft(5).reduced(10);
        auto objectDisplayBounds = leftPanelBounds.removeFromTop(140);

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if (!lnf)
            return;

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(16.0f));
        g.drawText("Reference: " + objectName, getLocalBounds().removeFromTop(35).translated(0, 4), Justification::centred);

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray infoNames = { "Category:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray infoText = { category, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

        for (int i = 0; i < infoNames.size(); i++) {
            auto localBounds = infoBounds.removeFromTop(25);
            g.drawText(infoNames[i], localBounds.removeFromLeft(90), Justification::topLeft);
            g.drawText(infoText[i], localBounds, Justification::topLeft);
        }

        auto descriptionBounds = infoBounds.removeFromTop(25);
        g.drawText("Description: ", descriptionBounds.removeFromLeft(90), Justification::topLeft);

        g.drawFittedText(description, descriptionBounds.withHeight(180), Justification::topLeft, 5, 1.0f);

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, objectDisplayBounds);
        } else {
            auto questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            g.setFont(Font(40));
            g.drawText("?", questionMarkBounds, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> objectRect)
    {
        auto const font = Font(15.0f);

        int const ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max(inlets.size(), outlets.size());
        int const textWidth = font.getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, PlugDataLook::objectCornerRadius, 1.0f);

        auto textBounds = outlineBounds.reduced(2.0f);
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText(objectName, textBounds, Justification::centred);

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
            g.setColour(inlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(inletBounds.toFloat());

            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(inletBounds.toFloat(), 1.0f);
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

            g.setColour(outlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(outletBounds.toFloat());

            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(outletBounds.toFloat(), 1.0f);
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

        inletDescriptions = library.getInletDescriptions()[name];
        outletDescriptions = library.getOutletDescriptions()[name];

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
        category = "";

        // Inverse lookup :(
        for (auto const& [cat, objects] : library.getObjectCategories()) {
            if (objects.contains(name)) {
                category = cat;
            }
        }

        if (category.isEmpty())
            category = "Unknown";

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

        rightSideInfo.setText(rightSideInfoText);
    }

    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;

    Array<std::pair<String, bool>> inletDescriptions;
    Array<std::pair<String, bool>> outletDescriptions;

    TextEditor rightSideInfo;

    TextButton backButton = TextButton(Icons::Back);

    String category;
    String description;

    pd::Library& library;
};
