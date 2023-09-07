#include <JuceHeader.h>
#include "ListBoxObjectItem.h"
#include "ObjectBrowserDialog.h"

ListBoxObjectItem::ListBoxObjectItem(ObjectsListBox* parent, int rowNumber, bool isSelected, std::function<void(bool shouldFade)> dismissDialog)
    : row(rowNumber)
    , rowIsSelected(isSelected)
    , objectsListBox(parent)
    , dismissMenu(dismissDialog)
{
    objectName = parent->objects[rowNumber];
    objectDescription = parent->descriptions[objectName];
}

void ListBoxObjectItem::paint(juce::Graphics& g)
{
    if (rowIsSelected || mouseHover) {
        auto colour = findColour(PlugDataColour::panelActiveBackgroundColourId);
        if (mouseHover && !rowIsSelected)
            colour = colour.withAlpha(0.5f);

        g.setColour(colour);
        g.fillRoundedRectangle(getLocalBounds().reduced(4, 2).toFloat(), Corners::defaultCornerRadius);
        //g.fillRoundedRectangle({ 4.0f, 1.0f, getWidth() - 8.0f, getHeight() - 2.0f }, Corners::defaultCornerRadius);
    }

    auto colour = rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);

    auto textBounds = Rectangle<int>(0, 0, getWidth(), getHeight()).reduced(18, 6);

    Fonts::drawStyledText(g, objectName, textBounds.removeFromTop(textBounds.proportionOfHeight(0.5f)), colour, Bold, 14);

    Fonts::drawText(g, objectDescription, textBounds, colour, 14);
}

bool ListBoxObjectItem::hitTest(int x, int y)
{
    auto bounds = getLocalBounds().reduced(4, 2);
    return bounds.contains(x, y);
}

void ListBoxObjectItem::mouseEnter(MouseEvent const& e)
{
    mouseHover = true;
    repaint();
}

void ListBoxObjectItem::mouseExit(MouseEvent const& e)
{
    mouseHover = false;
    repaint();
}

void ListBoxObjectItem::mouseDown(MouseEvent const& e)
{
    objectsListBox->selectRow(row, true, true);
}

void ListBoxObjectItem::mouseUp(MouseEvent const& e)
{
    if (dragging)
        dismissMenu(false);
}

void ListBoxObjectItem::mouseDrag(MouseEvent const& e)
{
    if (e.getDistanceFromDragStart() < 5)
        return;

    dragging = true;

    auto* dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

    if (dragContainer->isDragAndDropActive())
        return;

    // auto patchWithTheme = substituteThemeColours(objectPatch);

    auto formatedObject = "#X obj 0 0 " + objectName;

    auto scale = 2.0f;
    if (dragImage.image.isNull()) {
        auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
        dragImage = offlineObjectRenderer->patchToTempImage(formatedObject, scale);
    }

    dismissMenu(true);

    Array<var> palettePatchWithOffset;
    palettePatchWithOffset.add(var(dragImage.offset.getX()));
    palettePatchWithOffset.add(var(dragImage.offset.getY()));
    palettePatchWithOffset.add(var(formatedObject));
    dragContainer->startDragging(palettePatchWithOffset, this, ScaledImage(dragImage.image, scale), true, nullptr, nullptr, true);
}

String ListBoxObjectItem::getItemName() const
{
    return objectName;
}

void ListBoxObjectItem::refresh(String name, String description, int rowNumber, bool isSelected)
{
    objectName = name;
    objectDescription = description;
    row = rowNumber;
    rowIsSelected = isSelected;
    repaint();
}