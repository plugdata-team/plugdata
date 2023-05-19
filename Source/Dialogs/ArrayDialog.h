/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/PropertiesPanel.h"

struct ArrayDialog : public Component {

public:
    ArrayDialog(Dialog* parent, ArrayDialogCallback callback)
        : cb(callback)
    {
        name = "array1";
        size = 100;
        saveContents = false;
        drawMode = 1;
        yRange = var(Array<var> { var(-1.0f), var(1.0f) });

        addAndMakeVisible(arrayPropertiesPanel);
        addAndMakeVisible(ok);
        addAndMakeVisible(cancel);

        nameProperty = new PropertiesPanel::EditableComponent<String>("Name", name);

        sizeProperty = new PropertiesPanel::EditableComponent<int>("Size", size);
        sizeProperty->setRangeMin(1);

        auto* drawModeProperty = new PropertiesPanel::ComboComponent("Draw mode", drawMode, { "Points", "Polygon", "Bezier Curve" });
        auto* yRangeProperty = new PropertiesPanel::RangeComponent("Y range", yRange);
        auto* saveContentsProperty = new PropertiesPanel::BoolComponent("Save contents", saveContents, { "No", "Yes" });

        arrayPropertiesPanel.setContentWidth(250);
        arrayPropertiesPanel.addSection("Array properties", { nameProperty, sizeProperty, drawModeProperty, yRangeProperty, saveContentsProperty });

        cancel.onClick = [this, parent] {
            MessageManager::callAsync(
                [this, parent]() {
                    cb(0, "", 0, 0, false, { 0, 0 });
                    parent->closeDialog();
                });
        };

        ok.onClick = [this, parent] {
            auto nameStr = name.toString();
            auto sizeInt = getValue<int>(size);

            auto drawModeInt = getValue<int>(drawMode) - 1;
            auto saveContentsBool = getValue<bool>(saveContents);

            auto rangeArray = yRange.getValue().getArray();
            auto yRangePair = std::pair<float, float>(static_cast<float>(rangeArray->getReference(0)), static_cast<float>(rangeArray->getReference(1)));

            // Swap this flag
            if (drawModeInt == 0) {
                drawModeInt = 1;
            } else if (drawModeInt == 1) {
                drawModeInt = 0;
            }

            // Check if input is valid
            if (nameStr.isEmpty()) {
                invalidName = true;
                repaint();
            }
            if (sizeInt < 1) {
                invalidSize = true;
                repaint();
            }
            if (nameStr.isNotEmpty() && sizeInt > 0) {
                MessageManager::callAsync(
                    [this, parent, nameStr, sizeInt, drawModeInt, saveContentsBool, yRangePair]() {
                        cb(1, nameStr, sizeInt, drawModeInt, saveContentsBool, yRangePair);
                        parent->closeDialog();
                    });
            }
        };

        cancel.changeWidthToFitText();
        ok.changeWidthToFitText();
    }

    void paintOverChildren(Graphics& g) override
    {
        if (invalidName) {
            auto invalidArea = getLocalArea(nullptr, nameProperty->getScreenBounds());
            g.setColour(Colours::red);

            Path p;
            p.addRoundedRectangle(invalidArea.getX(), invalidArea.getY(), invalidArea.getWidth(), invalidArea.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
            g.strokePath(p, PathStrokeType(2.0f));
        }

        if (invalidSize) {
            auto invalidArea = getLocalArea(nullptr, sizeProperty->getScreenBounds());
            g.setColour(Colours::red);
            g.drawRoundedRectangle(invalidArea.toFloat(), Corners::windowCornerRadius, 2.0f);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().withTrimmedBottom(50).withTrimmedTop(10);
        arrayPropertiesPanel.setBounds(bounds);

        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);
    }

    ArrayDialogCallback cb;

private:
    PropertiesPanel::EditableComponent<String>* nameProperty;
    PropertiesPanel::EditableComponent<int>* sizeProperty;

    bool invalidName = false;
    bool invalidSize = false;

    PropertiesPanel arrayPropertiesPanel;

    Value name;
    Value size;
    Value drawMode;
    Value yRange;
    Value saveContents;

    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");
};
