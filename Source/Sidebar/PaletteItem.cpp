#include "PaletteItem.h"
#include "Palettes.h"
#include "Constants.h"
#include "Utility/StackShadow.h"
#include "Components/ColourPicker.h"

#include <array>

namespace {
std::array<Colour, 9> getPaletteItemColours()
{
    return {
        PlugDataColours::textObjectBackgroundColour,
        Colour::fromRGB(255, 232, 194),
        Colour::fromRGB(255, 247, 191),
        Colour::fromRGB(219, 242, 198),
        Colour::fromRGB(198, 238, 223),
        Colour::fromRGB(203, 232, 255),
        Colour::fromRGB(217, 220, 255),
        Colour::fromRGB(235, 215, 255),
        Colour::fromRGB(255, 216, 238)
    };
}

class PaletteItemColourPicker final : public Component {
public:
    PaletteItemColourPicker(PluginEditor* e, Component* topLevel, ValueTree item, Rectangle<int> colourPickerBounds, std::function<void()> colourChanged)
        : itemTree(item)
        , editor(e)
        , topLevelComponent(topLevel)
        , pickerBounds(colourPickerBounds)
        , onColourChanged(std::move(colourChanged))
    {
        setSize(168, 76);
    }

    void paint(Graphics& g) override
    {
        auto const colours = getPaletteItemColours();
        auto const hasCustomColour = itemTree.hasProperty("BgColor");
        auto const selectedColour = hasCustomColour
            ? Colour::fromString(itemTree.getProperty("BgColor").toString())
            : Colours::transparentBlack;
        auto const tick = getLookAndFeel().getTickShape(0.6f);

        for (int i = 0; i < static_cast<int>(colours.size()); i++) {
            auto const circleBounds = getCircleBounds(i);
            auto const colour = colours[static_cast<size_t>(i)];

            g.setColour(colour);
            g.fillEllipse(circleBounds);

            g.setColour(PlugDataColours::outlineColour);
            g.drawEllipse(circleBounds, 1.0f);

            if ((!hasCustomColour && i == 0) || (hasCustomColour && selectedColour.toString() == colour.toString())) {
                auto const tickBounds = circleBounds.getSmallestIntegerContainer().reduced(6);
                g.setColour(colour.contrasting(0.8f));
                g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.toFloat(), false));
            }
        }

        auto const buttonBounds = getPickerButtonBounds();
        g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
        g.fillEllipse(buttonBounds);
        g.setColour(PlugDataColours::outlineColour);
        g.drawEllipse(buttonBounds, 1.0f);
        Fonts::drawIcon(g, Icons::Eyedropper, buttonBounds.getSmallestIntegerContainer(), PlugDataColours::sidebarTextColour, 12);
    }

    void mouseUp(MouseEvent const& e) override
    {
        auto const colours = getPaletteItemColours();
        for (int i = 0; i < static_cast<int>(colours.size()); i++) {
            if (getCircleBounds(i).contains(e.position)) {
                if (i == 0)
                    itemTree.removeProperty("BgColor", nullptr);
                else
                    itemTree.setProperty("BgColor", colours[static_cast<size_t>(i)].toString(), nullptr);
                onColourChanged();
                closeCalloutBox();
                return;
            }
        }

        if (getPickerButtonBounds().contains(e.position)) {
            auto const colour = itemTree.hasProperty("BgColor")
                ? Colour::fromString(itemTree.getProperty("BgColor").toString())
                : PlugDataColours::textObjectBackgroundColour;
            auto* editorToUse = editor;
            auto* topLevelToUse = topLevelComponent.getComponent();
            auto const bounds = pickerBounds;
            auto item = itemTree;
            auto colourChanged = onColourChanged;
            closeCalloutBox();
            MessageManager::callAsync([editorToUse, topLevelToUse, bounds, item, colour, colourChanged]() mutable {
                ColourPicker::getInstance()->show(editorToUse, topLevelToUse, false, colour, bounds, [item, colourChanged](Colour const c) mutable {
                    item.setProperty("BgColor", c.toString(), nullptr);
                    colourChanged();
                });
            });
        }
    }

private:
    Rectangle<float> getCircleBounds(int const index) const
    {
        constexpr int columns = 5;
        constexpr float diameter = 24.0f;
        constexpr float gap = 8.0f;
        constexpr float left = 12.0f;
        constexpr float top = 10.0f;

        auto const row = index / columns;
        auto const column = index % columns;
        return { left + column * (diameter + gap), top + row * (diameter + gap), diameter, diameter };
    }

    Rectangle<float> getPickerButtonBounds() const
    {
        return getCircleBounds(static_cast<int>(getPaletteItemColours().size()));
    }

    void closeCalloutBox()
    {
        if (auto* callout = findParentComponentOfClass<CallOutBox>())
            callout->dismiss();
    }

    ValueTree itemTree;
    PluginEditor* editor;
    SafePointer<Component> topLevelComponent;
    Rectangle<int> pickerBounds;
    std::function<void()> onColourChanged;
};
}

PaletteItem::PaletteItem(PluginEditor* e, PaletteList* parent, ValueTree tree)
    : itemTree(tree)
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

    nameLabel.onEditorShow = [this] {
        if (auto* editor = nameLabel.getCurrentTextEditor()) {
            editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setJustification(Justification::centred);
        }
    };

    nameLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(nameLabel);

    reorderButton = std::make_unique<ReorderButton>();
    reorderButton->addMouseListener(this, false);

    addChildComponent(reorderButton.get());

    deleteButton.setTooltip("Delete item");
    deleteButton.setSize(25, 25);
    deleteButton.onClick = [this] {
        deleteItem();
    };
    deleteButton.addMouseListener(this, false);

    addChildComponent(deleteButton);

    isSubpatch = isSubpatchOrAbstraction(palettePatch);
    if (isSubpatch) {
        auto const [fst, snd] = countIolets(palettePatch);
        inlets = fst;
        outlets = snd;
    }

    setViewportIgnoreDragFlag(true);

    updater.addAnimator(animator);
    lookAndFeelChanged();
    updateTextColour();
}

PaletteItem::~PaletteItem()
{
    if (paletteComp)
        removeMouseListener(paletteComp);
}

void PaletteItem::lookAndFeelChanged()
{
    nameLabel.setFont(Fonts::getCurrentFont());
    updateTextColour();
}

Colour PaletteItem::getBackgroundColour() const
{
    if (itemTree.hasProperty("BgColor"))
        return Colour::fromString(itemTree.getProperty("BgColor").toString());

    return PlugDataColours::textObjectBackgroundColour;
}

void PaletteItem::updateTextColour()
{
    auto const textColour = getBackgroundColour().contrasting(0.8f);
    nameLabel.setColour(Label::textColourId, textColour);
    nameLabel.setColour(Label::textWhenEditingColourId, textColour);
}

void PaletteItem::showColourPicker()
{
    if (editor) {
        auto const pickerBounds = getScreenBounds();
        auto colourPicker = std::make_unique<PaletteItemColourPicker>(editor, getTopLevelComponent(), itemTree, pickerBounds, [_this = SafePointer(this)] {
            if (_this) {
                _this->updateTextColour();
                _this->repaint();
            }
        });
        editor->showCalloutBox(std::move(colourPicker), pickerBounds);
    }
}

bool PaletteItem::hitTest(int const x, int const y)
{
    auto hit = false;
    auto const bounds = getLocalBounds().reduced(16.0f, 4.0f).toFloat();

    if (bounds.contains(x, y)) {
        hit = true;
    }

    return hit;
}

void PaletteItem::setIsItemDragged(bool const isActive)
{
    if (isItemDragged != isActive) {
        isItemDragged = isActive;
        repaint();
    }
}

void PaletteItem::paint(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(16.0f, 4.0f).toFloat();
    auto const backgroundColour = getBackgroundColour();

    if (isItemDragged) {
        Path dropShadowPath;
        dropShadowPath.addRoundedRectangle(bounds.reduced(4.0f), 5.0f);
        auto dropShadowColour = PlugDataColours::objectSelectedOutlineColour;
        StackShadow::drawShadowForPath(g, hash("palette_item"), dropShadowPath, 8, dropShadowColour.withAlpha(0.5f));
    }
    auto outlineColour = isItemDragged ? PlugDataColours::objectSelectedOutlineColour : PlugDataColours::objectOutlineColour;

    if (!isSubpatch) {
        auto lineBounds = bounds.reduced(2.5f);

        SmallArray<float> dashLength = { 5.0f, 5.0f };

        Path dashedRect;
        dashedRect.addRoundedRectangle(lineBounds, 5.0f);

        PathStrokeType dashedStroke(0.5f);
        dashedStroke.createDashedStroke(dashedRect, dashedRect, dashLength.data(), 2);

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(lineBounds, 5.0f);

        g.setColour(outlineColour);
        g.strokePath(dashedRect, dashedStroke);
        return;
    }

    auto inletCount = inlets.size();
    auto outletCount = outlets.size();

    auto inletSize = inletCount > 0 ? (bounds.getWidth() - 24 * 2) / inletCount * 0.5f : 0.0f;
    auto outletSize = outletCount > 0 ? (bounds.getWidth() - 24 * 2) / outletCount * 0.5f : 0.0f;

    auto ioletRadius = 5.0f;
    auto inletRadius = jmin(ioletRadius, inletSize);
    auto outletRadius = jmin(ioletRadius, outletSize);
    auto cornerRadius = 5.0f;

    int x = bounds.getX() + 8;

    auto lineBounds = bounds.reduced(ioletRadius / 2);

    Path p;
    p.startNewSubPath(x, lineBounds.getY());

    auto ioletStroke = PathStrokeType(1.0f);
    SmallArray<std::tuple<Path, Colour>, 8> ioletPaths;

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
            inletBounds = Rectangle<float>(bounds.getX() + ratio * i + 24, yPosition, inletRadius, ioletRadius);
        }

        inletArc.startNewSubPath(inletBounds.getCentre().translated(-inletRadius, 0.0f));

        constexpr auto fromRadians = MathConstants<float>::pi * 1.5f;
        constexpr auto toRadians = MathConstants<float>::pi * 0.5f;

        // p.addCentredArc(inletBounds.getCentreX(), inletBounds.getCentreY(), inletRadius, inletRadius, 0.0f, fromRadians, toRadians, false);
        inletArc.addCentredArc(inletBounds.getCentreX(), inletBounds.getCentreY(), inletRadius, inletRadius, 0.0f, fromRadians, toRadians, false);

        auto inletColour = inlets[i] ? PlugDataColours::signalColour : PlugDataColours::dataColour;
        ioletPaths.add(std::tuple<Path, Colour>(inletArc, inletColour));
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
            outletBounds = Rectangle<float>(bounds.getX() + ratio * i + 24, yPosition, outletRadius, ioletRadius);
        }

        outletArc.startNewSubPath(outletBounds.getCentre().translated(outletRadius, 0.0f).getX(), lineBounds.getBottom());

        constexpr auto fromRadians = MathConstants<float>::pi * -0.5f;
        constexpr auto toRadians = MathConstants<float>::pi * 0.5f;

        // p.addCentredArc(outletBounds.getCentreX(), lineBounds.getBottom(), outletRadius, outletRadius, 0, fromRadians, toRadians, false);
        outletArc.addCentredArc(outletBounds.getCentreX(), lineBounds.getBottom(), outletRadius, outletRadius, 0.0f, fromRadians, toRadians, false);

        auto outletColour = outlets[i] ? PlugDataColours::signalColour : PlugDataColours::dataColour;
        ioletPaths.add(std::tuple<Path, Colour>(outletArc, outletColour));
    }

    p.lineTo(lineBounds.getBottomLeft().translated(cornerRadius, 0));

    p.quadraticTo(lineBounds.getBottomLeft(), lineBounds.getBottomLeft().translated(0, -cornerRadius));

    p.lineTo(lineBounds.getTopLeft().translated(0, cornerRadius));
    p.quadraticTo(lineBounds.getTopLeft(), lineBounds.getTopLeft().translated(cornerRadius, 0));
    p.closeSubPath();

    g.setColour(backgroundColour);
    g.fillPath(p);

    g.setColour(outlineColour);
    g.strokePath(p, PathStrokeType(1.0f));

    // draw all the iolet paths on top of the border
    for (auto& [path, colour] : ioletPaths) {
        g.setColour(colour);
        g.fillPath(path);
    }
}

void PaletteItem::mouseDown(MouseEvent const& e)
{
    if (e.mods.isRightButtonDown()) {
        showColourPicker();
        return;
    }
    
    if (!isRealClickEvent(e))
        return;

    if (reorderButton.get() == e.originalComponent) {
        setIsItemDragged(true);
    }
}

void PaletteItem::mouseDrag(MouseEvent const& e)
{
    if (!isRealClickEvent(e))
        return;

    if (e.originalComponent == this && e.getDistanceFromDragStart() > 4) {
        if (auto* editor = findParentComponentOfClass<PluginEditor>()) {
            ObjectDragAndDrop::attachToMouse(editor, palettePatch);
        }
    }
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
    auto const componentCentre = getLocalBounds().getCentre().getY();
    reorderButton->setCentrePosition(30, componentCentre);
    deleteButton.setCentrePosition(getLocalBounds().getRight() - 30, componentCentre);
}

void PaletteItem::deleteItem()
{
    auto parentTree = itemTree.getParent();

    if (!parentTree.isValid())
        return;

    // remove the item via async as we are also deleting this instance
    MessageManager::callAsync([this, parentTree, itemTree = this->itemTree, _paletteComp = SafePointer(paletteComp)]() mutable {
        parentTree.removeChild(itemTree, nullptr);
        if (_paletteComp) {
            _paletteComp->items.removeObject(this);
            _paletteComp->resized();
        }
    });
}

void PaletteItem::animateToPosition(Rectangle<int> targetBounds)
{
    animationStartBounds = getBounds();
    animationEndBounds = targetBounds;
    animator.start();
}

void PaletteItem::cancelAnimation(Rectangle<int> targetBounds)
{
    animator.complete();
    setBounds(targetBounds);
}

Rectangle<int> PaletteItem::getTargetBounds()
{
    return animator.isComplete() ? getBounds() : animationEndBounds;
}

void PaletteItem::mouseUp(MouseEvent const& e)
{
    if (nameLabel.getBounds().contains(e.getEventRelativeTo(&nameLabel).getPosition()) && !e.mouseWasDraggedSinceMouseDown() && e.getNumberOfClicks() >= 2) {
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
    for (int i = lines.size() - 1; i >= 0; i--) {
        if (lines[i].startsWith("#A")) {
            lines.remove(i);
        }
    }
    return lines.size() == 1 || (lines[0].startsWith("#N canvas") && lines[lines.size() - 1].startsWith("#X restore"));
}

std::pair<SmallArray<bool>, SmallArray<bool>> PaletteItem::countIolets(String const& patchAsString)
{

    StackArray<SmallArray<std::pair<bool, Point<int>>>, 2> iolets;
    auto& [inlets, outlets] = iolets.data_;
    int canvasDepth = patchAsString.startsWith("#N canvas") ? -1 : 0;

    auto isObject = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] != "connect" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto isStartingCanvas = [](StringArray const& tokens) {
        return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
    };

    auto isEndingCanvas = [](StringArray const& tokens) {
        return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
    };

    auto countIolet = [&inlets = iolets[0], &outlets = iolets[1]](StringArray const& tokens) {
        auto position = Point<int>(tokens[2].getIntValue(), tokens[3].getIntValue());
        auto const name = tokens[4];
        if (name == "inlet")
            inlets.add({ false, position });
        if (name == "outlet")
            outlets.add({ false, position });
        if (name == "inlet~")
            inlets.add({ true, position });
        if (name == "outlet~")
            outlets.add({ true, position });
    };

    auto lines = StringArray::fromLines(patchAsString.trim());

    for (int i = lines.size() - 1; i >= 0; i--) {
        if (lines[i].startsWith("#A")) {
            lines.remove(i);
        }
    }

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

    auto ioletSortFunc = [](std::pair<bool, Point<int>> const& a, std::pair<bool, Point<int>> const& b) {
        auto const& [typeA, positionA] = a;
        auto const& [typeB, positionB] = b;

        if (positionA.x == positionB.x) {
            return positionA.y < positionB.y;
        }

        return positionA.x < positionB.x;
    };

    inlets.sort(ioletSortFunc);
    outlets.sort(ioletSortFunc);

    auto result = std::pair<SmallArray<bool>, SmallArray<bool>>();

    for (auto& [type, position] : inlets) {
        result.first.add(type);
    }
    for (auto& [type, position] : outlets) {
        result.second.add(type);
    }

    return result;
}
