#include "PaletteItem.h"
#include "Palettes.h"
#include "Constants.h"
#include "Utility/StackShadow.h"

PaletteItem::PaletteItem(PluginEditor* e, PaletteDraggableList* parent, ValueTree tree)
    : ObjectDragAndDrop(e)
    , itemTree(tree)
    , editor(e)
    , paletteComp(parent)
{
    addMouseListener(paletteComp, true);

    paletteName = itemTree.getProperty("Name");
    palettePatch = itemTree.getProperty("Patch");

    nameLabel.setText(paletteName, dontSendNotification);
    nameLabel.setInterceptsMouseClicks(false, false);
    nameLabel.onTextChange = [this]() mutable {
        paletteName = nameLabel.getText();
        itemTree.setProperty("Name", paletteName, nullptr);
    };

    nameLabel.onEditorShow = [this]() {
        if (auto* editor = nameLabel.getCurrentTextEditor()) {
            editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setJustification(Justification::centred);
        }
    };

    nameLabel.setJustificationType(Justification::centred);
    // nameLabel.addMouseListener(this, false);

    addAndMakeVisible(nameLabel);

    reorderButton = std::make_unique<ReorderButton>();
    reorderButton->addMouseListener(this, false);

    addChildComponent(reorderButton.get());

    deleteButton.setTooltip("Delete item");
    deleteButton.setSize(25, 25);
    deleteButton.onClick = [this]() {
        deleteItem();
    };
    deleteButton.addMouseListener(this, false);

    addChildComponent(deleteButton);

    isSubpatch = isSubpatchOrAbstraction(palettePatch);
    if (isSubpatch) {
        auto iolets = countIolets(palettePatch);
        inlets = iolets.first;
        outlets = iolets.second;
    }

    lookAndFeelChanged();
}

PaletteItem::~PaletteItem()
{
    if (paletteComp)
        removeMouseListener(paletteComp);
}

void PaletteItem::lookAndFeelChanged()
{
    nameLabel.setFont(Fonts::getCurrentFont());
}

bool PaletteItem::hitTest(int x, int y)
{
    auto hit = false;
    auto bounds = getLocalBounds().reduced(16.0f, 4.0f).toFloat();

    if (bounds.contains(x, y)) {
        hit = true;
    }

    return hit;
}

void PaletteItem::setIsItemDragged(bool isActive)
{
    if (isItemDragged != isActive) {
        isItemDragged = isActive;
        repaint();
    }
}

void PaletteItem::paint(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(16.0f, 4.0f).toFloat();

    if (isItemDragged) {
        Path dropShadowPath;
        dropShadowPath.addRoundedRectangle(bounds.reduced(4.0f), 5.0f);
        auto dropShadowColour = findColour(PlugDataColour::objectSelectedOutlineColourId);
        StackShadow::renderDropShadow(g, dropShadowPath, dropShadowColour.withAlpha(0.5f), 7);
    }
    auto outlineColour = isItemDragged ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId;

    if (!isSubpatch) {
        auto lineBounds = bounds.reduced(2.5f);

        std::vector<float> dashLength = { 5.0f, 5.0f };

        juce::Path dashedRect;
        dashedRect.addRoundedRectangle(lineBounds, 5.0f);

        juce::PathStrokeType dashedStroke(0.5f);
        dashedStroke.createDashedStroke(dashedRect, dashedRect, dashLength.data(), 2);

        g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
        g.fillRoundedRectangle(lineBounds, 5.0f);

        g.setColour(findColour(outlineColour));
        g.strokePath(dashedRect, dashedStroke);
        return;
    }

    auto inletCount = inlets.size();
    auto outletCount = outlets.size();

    auto inletSize = inletCount > 0 ? ((bounds.getWidth() - (24 * 2)) / inletCount) * 0.5f : 0.0f;
    auto outletSize = outletCount > 0 ? ((bounds.getWidth() - (24 * 2)) / outletCount) * 0.5f : 0.0f;

    auto ioletRadius = 5.0f;
    auto inletRadius = jmin(ioletRadius, inletSize);
    auto outletRadius = jmin(ioletRadius, outletSize);
    auto cornerRadius = 5.0f;

    int x = bounds.getX() + 8;

    auto lineBounds = bounds.reduced(ioletRadius / 2);

    Path p;
    p.startNewSubPath(x, lineBounds.getY());

    auto ioletStroke = PathStrokeType(1.0f);
    std::vector<std::tuple<Path, Colour>> ioletPaths;

    for (int i = 0; i < inlets.size(); i++) {
        Path inletArc;
        auto inletBounds = Rectangle<float>();
        int const total = inlets.size();
        float const yPosition = bounds.getY();

        if (total == 1 && i == 0) {
            int xPosition = getWidth() < 40 ? bounds.getCentreX() - inletRadius / 2.0f : bounds.getX();
            inletBounds = Rectangle<float>(xPosition + 24, yPosition, inletRadius, ioletRadius);

        } else if (total > 1) {
            float const ratio = (bounds.getWidth() - inletRadius - 48) / static_cast<float>(total - 1);
            inletBounds = Rectangle<float>((bounds.getX() + ratio * i) + 24, yPosition, inletRadius, ioletRadius);
        }

        inletArc.startNewSubPath(inletBounds.getCentre().translated(-inletRadius, 0.0f));

        auto const fromRadians = MathConstants<float>::pi * 1.5f;
        auto const toRadians = MathConstants<float>::pi * 0.5f;

        // p.addCentredArc(inletBounds.getCentreX(), inletBounds.getCentreY(), inletRadius, inletRadius, 0.0f, fromRadians, toRadians, false);
        inletArc.addCentredArc(inletBounds.getCentreX(), inletBounds.getCentreY(), inletRadius, inletRadius, 0.0f, fromRadians, toRadians, false);

        auto inletColour = inlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId);
        ioletPaths.push_back(std::tuple<Path, Colour>(inletArc, inletColour));
    }

    p.lineTo(lineBounds.getTopRight().translated(-cornerRadius, 0));

    p.quadraticTo(lineBounds.getTopRight(), lineBounds.getTopRight().translated(0, cornerRadius));

    p.lineTo(lineBounds.getBottomRight().translated(0, -cornerRadius));

    p.quadraticTo(lineBounds.getBottomRight(), lineBounds.getBottomRight().translated(-cornerRadius, 0));

    for (int i = outlets.size() - 1; i >= 0; i--) {
        Path outletArc;
        auto outletBounds = Rectangle<float>();
        int const total = outlets.size();
        float const yPosition = bounds.getBottom() - outletRadius;

        if (total == 1 && i == 0) {
            int xPosition = getWidth() < 40 ? bounds.getCentreX() - outletRadius / 2.0f : bounds.getX();
            outletBounds = Rectangle<float>(xPosition + 24, yPosition, outletRadius, ioletRadius);

        } else if (total > 1) {
            float const ratio = (bounds.getWidth() - outletRadius - 48) / static_cast<float>(total - 1);
            outletBounds = Rectangle<float>((bounds.getX() + ratio * i) + 24, yPosition, outletRadius, ioletRadius);
        }

        outletArc.startNewSubPath(outletBounds.getCentre().translated(outletRadius, 0.0f).getX(), lineBounds.getBottom());

        auto const fromRadians = MathConstants<float>::pi * -0.5f;
        auto const toRadians = MathConstants<float>::pi * 0.5f;

        // p.addCentredArc(outletBounds.getCentreX(), lineBounds.getBottom(), outletRadius, outletRadius, 0, fromRadians, toRadians, false);
        outletArc.addCentredArc(outletBounds.getCentreX(), lineBounds.getBottom(), outletRadius, outletRadius, 0.0f, fromRadians, toRadians, false);

        auto outletColour = outlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId);
        ioletPaths.push_back(std::tuple<Path, Colour>(outletArc, outletColour));
    }

    p.lineTo(lineBounds.getBottomLeft().translated(cornerRadius, 0));

    p.quadraticTo(lineBounds.getBottomLeft(), lineBounds.getBottomLeft().translated(0, -cornerRadius));

    p.lineTo(lineBounds.getTopLeft().translated(0, cornerRadius));
    p.quadraticTo(lineBounds.getTopLeft(), lineBounds.getTopLeft().translated(cornerRadius, 0));
    p.closeSubPath();

    g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
    g.fillPath(p);

    g.setColour(findColour(outlineColour));
    g.strokePath(p, PathStrokeType(1.0f));

    // draw all the iolet paths on top of the border
    for (auto& [path, colour] : ioletPaths) {
        g.setColour(colour);
        g.fillPath(path);
    }
}

void PaletteItem::mouseDown(MouseEvent const& e)
{
    if (reorderButton.get() == e.originalComponent)
        setIsReordering(true);
    else
        setIsReordering(false);
}

void PaletteItem::mouseEnter(MouseEvent const& e)
{
    reorderButton->setVisible(true);
    deleteButton.setVisible(true);
}

void PaletteItem::mouseExit(MouseEvent const& e)
{
    reorderButton->setVisible(false);
    deleteButton.setVisible(false);
}

void PaletteItem::resized()
{
    nameLabel.setBounds(getLocalBounds().reduced(16, 4));
    auto componentCentre = getLocalBounds().getCentre().getY();
    reorderButton->setCentrePosition(30, componentCentre);
    deleteButton.setCentrePosition(getLocalBounds().getRight() - 30, componentCentre);
}

String PaletteItem::getObjectString()
{
    return palettePatch;
}

String PaletteItem::getPatchStringName()
{
    return paletteName + String(" palette");
}

void PaletteItem::deleteItem()
{
    auto parentTree = itemTree.getParent();

    if (!parentTree.isValid())
        return;

    // remove the item via async as we are also deleting this instance
    // we also need to resize the list component, but from it's parent component
    // and _also?_ the list component? ¯\_(ツ)_/¯
    MessageManager::callAsync([this, parentTree, itemTree = this->itemTree, _paletteComp = SafePointer(paletteComp)]() mutable {
        parentTree.removeChild(itemTree, nullptr);
        auto paletteComponent = findParentComponentOfClass<PaletteComponent>();
        if (_paletteComp) {
            _paletteComp->items.removeObject(this);
            paletteComponent->resized();
            _paletteComp->resized();
        }
    });
}

void PaletteItem::mouseUp(MouseEvent const& e)
{
    if (nameLabel.getBounds().contains((e.getEventRelativeTo(&nameLabel).getPosition())) && !e.mouseWasDraggedSinceMouseDown() && e.getNumberOfClicks() >= 2) {
        nameLabel.showEditor();
    } else if (e.mouseWasDraggedSinceMouseDown()) {
        getParentComponent()->resized();
        setIsItemDragged(false);
    }
    if (paletteComp->isItemShowingMenu) {
        paletteComp->addMouseListener(this, false);
        paletteComp->isItemShowingMenu = false;
    }
}

bool PaletteItem::isSubpatchOrAbstraction(String const& patchAsString)
{
    auto lines = StringArray::fromLines(patchAsString.trim());
    return lines.size() == 1 || (lines[0].startsWith("#N canvas") && lines[lines.size() - 1].startsWith("#X restore"));
}

std::pair<std::vector<bool>, std::vector<bool>> PaletteItem::countIolets(String const& patchAsString)
{

    std::array<std::vector<std::pair<bool, Point<int>>>, 2> iolets;
    auto& [inlets, outlets] = iolets;
    int canvasDepth = patchAsString.startsWith("#N canvas") ? -1 : 0;

    auto isObject = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isStartingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
    };

    auto isEndingCanvas = [](StringArray& tokens) {
        return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto countIolet = [&inlets = iolets[0], &outlets = iolets[1]](StringArray& tokens) {
        auto position = Point<int>(tokens[2].getIntValue(), tokens[3].getIntValue());
        auto name = tokens[4];
        if (name == "inlet")
            inlets.push_back({ false, position });
        if (name == "outlet")
            outlets.push_back({ false, position });
        if (name == "inlet~")
            inlets.push_back({ true, position });
        if (name == "outlet~")
            outlets.push_back({ true, position });
    };

    auto lines = StringArray::fromLines(patchAsString);

    // In case the patch contains a single object, we need to use a different method to find the number and kind inlets and outlets
    if (lines.size() == 1) {
        return OfflineObjectRenderer::countIolets(lines[0]);
    }

    for (auto& line : lines) {

        line = line.upToLastOccurrenceOf(";", false, false);

        auto tokens = StringArray::fromTokens(line, true);

        if (isStartingCanvas(tokens)) {
            canvasDepth++;
        }

        if (canvasDepth == 0 && isObject(tokens)) {
            countIolet(tokens);
        }

        if (isEndingCanvas(tokens)) {
            canvasDepth--;
        }
    }

    auto ioletSortFunc = [](std::pair<bool, Point<int>>& a, std::pair<bool, Point<int>>& b) {
        auto& [typeA, positionA] = a;
        auto& [typeB, positionB] = b;

        if (positionA.x == positionB.x) {
            return positionA.y < positionB.y;
        }

        return positionA.x < positionB.x;
    };

    std::sort(inlets.begin(), inlets.end(), ioletSortFunc);
    std::sort(outlets.begin(), outlets.end(), ioletSortFunc);

    auto result = std::pair<std::vector<bool>, std::vector<bool>>();

    for (auto& [type, position] : inlets) {
        result.first.push_back(type);
    }
    for (auto& [type, position] : outlets) {
        result.second.push_back(type);
    }

    return result;
}
