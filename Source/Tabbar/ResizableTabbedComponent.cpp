/*
 // Copyright (c) 2023 Alex Mitchell 2021-2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "ResizableTabbedComponent.h"
#include "PluginEditor.h"
#include "SplitViewResizer.h"
#include "PluginProcessor.h"
#include "Components/SearchEditor.h"
#include "Sidebar/Palettes.h"
#include "Sidebar/DocumentBrowser.h"
#include "Tabbar.h"
#include "TabBarButtonComponent.h"

#include "Components/ObjectDragAndDrop.h"

#define ENABLE_SPLITS_DROPZONE_DEBUGGING 0

ResizableTabbedComponent::ResizableTabbedComponent(PluginEditor* editor, TabComponent* mainTabComponent)
    : editor(editor)
{
    if (mainTabComponent != nullptr)
        tabComponent.reset(mainTabComponent);
    else
        tabComponent = std::make_unique<TabComponent>(editor);

    addAndMakeVisible(*tabComponent);

    setInterceptsMouseClicks(true, true);
    addMouseListener(this, true);
}

ResizableTabbedComponent::~ResizableTabbedComponent()
{
    removeMouseListener(this);
}

bool ResizableTabbedComponent::isInterestedInDragSource(SourceDetails const& dragSourceDetails)
{
    auto windowTab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    auto draggedObject = dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get());

    // TODO: not implemented yet
    auto docBrowserItem = dynamic_cast<DocumentBrowserViewBase*>(dragSourceDetails.sourceComponent.get());

    if (windowTab || docBrowserItem || draggedObject)
        return true;
    return false;
}

void ResizableTabbedComponent::itemDropped(SourceDetails const& dragSourceDetails)
{
    isDragAndDropOver = false;
    repaint();

    if (auto windowTab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        switch (activeZone) {
        case DropZones::Right:
            splitMode = Split::SplitMode::Horizontal;
            moveTabToNewSplit(dragSourceDetails);
            break;
        case DropZones::Left:
            splitMode = Split::SplitMode::Horizontal;
            moveTabToNewSplit(dragSourceDetails);
            break;
        case DropZones::Top:
            moveTabToNewSplit(dragSourceDetails);
            // splitMode = Split::SplitMode::Vertical;
            break;
        case DropZones::Bottom:
            moveTabToNewSplit(dragSourceDetails);
            // splitMode = Split::SplitMode::Vertical;
            break;
        case DropZones::TabBar:
            splitMode = Split::SplitMode::None;
            break;
        case DropZones::Centre:
            moveTabToNewSplit(dragSourceDetails);
            break;
        }
    } else if (dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get())) {
        if (!tabComponent)
            return;

        auto cnv = tabComponent->getCurrentCanvas();
        if (!cnv)
            return;

        auto mousePos = (cnv->getLocalPoint(this, dragSourceDetails.localPosition) - cnv->canvasOrigin);

        // Extract the array<var> from the var
        auto patchWithSize = *dragSourceDetails.description.getArray();
        auto patchSize = Point<int>(patchWithSize[0], patchWithSize[1]);
        auto patchData = patchWithSize[2].toString();

        cnv->dragAndDropPaste(patchData, mousePos, patchSize.x, patchSize.y);
    } else if (auto docBrowserItem = dynamic_cast<DocumentBrowserItem*>(dragSourceDetails.sourceComponent.get())) {
        // ALEX TODO: not implemented
        // we also have an issue that the DocumentBrowserItem is a TreeViewItem which is a Juce class that uses DragAndDropContainer!
        // so we wil need to somehow get that using ZoomableDragAndDropContainer?
        // browser->pd->loadPatch(file);
        // SettingsFile::getInstance()->addToRecentlyOpened(file);
    }
}

void ResizableTabbedComponent::moveToSplit(ResizableTabbedComponent* targetSplit, Canvas* canvas)
{
    if (!targetSplit) {
        createNewSplit(DropZones::Right, canvas);
    } else {
        if (auto* sourceTabBar = canvas->getTabbar()) {
            auto sourceTabIndex = canvas->getTabIndex();
            sourceTabBar->removeTab(sourceTabIndex);
            sourceTabBar->setCurrentTabIndex(sourceTabIndex > (sourceTabBar->getNumTabs() - 1) ? sourceTabIndex - 1 : sourceTabIndex);
        }

        auto tabTitle = canvas->patch.getTitle();
        targetSplit->getTabComponent()->addTab(tabTitle, canvas->viewport.get(), 0);
        canvas->viewport->setVisible(true);

        targetSplit->resized();
        targetSplit->getTabComponent()->resized();
    }
}
void ResizableTabbedComponent::createNewSplit(DropZones activeZone, Canvas* canvas)
{
    if (auto* sourceTabBar = canvas->getTabbar()) {
        auto sourceTabIndex = canvas->getTabIndex();
        sourceTabBar->removeTab(sourceTabIndex);
        sourceTabBar->setCurrentTabIndex(sourceTabIndex > (sourceTabBar->getNumTabs() - 1) ? sourceTabIndex - 1 : sourceTabIndex);
    }

    auto* newSplit = new ResizableTabbedComponent(editor);
    SplitViewResizer* resizer;

    // depending on if the dropzone is left or right we have to use the opposite resizer
    if (activeZone == DropZones::Right) {
        // connect resizers (if they exist) to the new split and / or replace the existing resizer of existing split
        resizer = new SplitViewResizer(this, newSplit, Split::SplitMode::Horizontal, 1);
        newSplit->resizerLeft = resizer;
        newSplit->resizerRight = resizerRight;

        // if we have a right resizer, that means we are inserting a split between two existing splits
        // so update the split that the right resizer has with the new split
        if (resizerRight) {
            resizerRight->splits[0] = newSplit;
        }
        resizerRight = resizer;
    } else if (activeZone == DropZones::Left) {
        resizer = new SplitViewResizer(this, newSplit, Split::SplitMode::Horizontal, 0);
        newSplit->resizerRight = resizer;
        newSplit->resizerLeft = resizerLeft;

        if (resizerLeft) {
            resizerLeft->splits[1] = newSplit;
        }
        resizerLeft = resizer;
    } else {
        return;
    }

    // update the bounds of the new and existing split using the resizer factors
    newSplit->setBoundsWithFactors(getParentComponent()->getLocalBounds());
    setBoundsWithFactors(getParentComponent()->getLocalBounds());

    // add the split to the splitview and make it active (both owned arrays of SplitView)
    editor->splitView.addSplit(newSplit);
    editor->splitView.addResizer(resizer);

    auto tabTitle = canvas->patch.getTitle();
    newSplit->getTabComponent()->addTab(tabTitle, canvas->viewport.get(), 0);
    canvas->viewport->setVisible(true);
    canvas->moveToWindow(editor);

    newSplit->resized();
    newSplit->getTabComponent()->resized();
}

void ResizableTabbedComponent::moveTabToNewSplit(SourceDetails const& dragSourceDetails)
{
    // get the dragging tab
    auto* sourceTabButton = static_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    int sourceTabIndex = sourceTabButton->getIndex();
    auto sourceTabContent = sourceTabButton->getTabComponent();
    int sourceNumTabs = sourceTabContent->getNumTabs();
    bool shouldDelete = (sourceNumTabs - 1) == 0 ? true : false;
    bool dropZoneCentre = (activeZone == DropZones::Centre) ? true : false;

    if (dropZoneCentre) {
        auto* tabCanvas = sourceTabContent->getCanvas(sourceTabIndex);
        auto tabTitle = tabCanvas->patch.getTitle();
        auto newTabIdx = tabComponent->getNumTabs();
        tabComponent->addTab(tabTitle, sourceTabContent->getCanvas(sourceTabIndex)->viewport.get(), newTabIdx);
        tabComponent->setCurrentTabIndex(newTabIdx);

        editor->splitView.setFocus(this);
        tabCanvas->moveToWindow(editor);
        sourceTabContent->removeTab(sourceTabIndex);
        sourceTabContent->setCurrentTabIndex(sourceTabIndex > (sourceTabContent->getNumTabs() - 1) ? sourceTabIndex - 1 : sourceTabIndex);
        for (auto* split : editor->splitView.splits) {
            split->setBoundsWithFactors(getParentComponent()->getLocalBounds());
        }
    } else if (activeZone == DropZones::Left || activeZone == DropZones::Right) {
        createNewSplit(static_cast<DropZones>(activeZone), sourceTabContent->getCanvas(sourceTabIndex));
    }

    if (shouldDelete) {
        editor->splitView.setFocus(this);
        editor->splitView.removeSplit(sourceTabContent);
        for (auto* split : editor->splitView.splits) {
            split->setBoundsWithFactors(getParentComponent()->getLocalBounds());
        }
    }

    // set all current canvas viewports to visible, (if they already are this shouldn't do anything)
    for (auto* split : editor->splitView.splits) {
        if (auto tabComponent = split->getTabComponent()) {
            auto tabIndex = tabComponent->getCurrentTabIndex();
            if (tabIndex < 0 && tabComponent->getNumTabs() > 0) {
                tabComponent->setCurrentTabIndex(0);
            }
            if (auto cnv = tabComponent->getCanvas(tabIndex)) {
                cnv->viewport->setVisible(true);
                split->resized();
                split->getTabComponent()->resized();
            }
        }
    }

    editor->pd->savePatchTabPositions();
}

String ResizableTabbedComponent::getZoneName(int zone)
{
    // for console debugging
    String zoneName;
    switch (zone) {
    case DropZones::Left:
        zoneName = "left";
        break;
    case DropZones::Top:
        zoneName = "top";
        break;
    case DropZones::Right:
        zoneName = "right";
        break;
    case DropZones::Bottom:
        zoneName = "bottom";
        break;
    case DropZones::Centre:
        zoneName = "centre";
        break;
    case DropZones::TabBar:
        zoneName = "tab bar";
        break;
    }
    return zoneName;
}

int ResizableTabbedComponent::findZoneFromSource(SourceDetails const& dragSourceDetails)
{
    for (auto const& [zone, dropZone] : dropZones) {
        if (dropZone.contains(dragSourceDetails.localPosition.toFloat())) {
            return zone;
        }
    }
    return -1;
}

void ResizableTabbedComponent::mouseDown(MouseEvent const& e)
{
    editor->splitView.setFocus(this);
}

void ResizableTabbedComponent::setBoundsWithFactors(Rectangle<int> bounds)
{
    if (resizerLeft)
        leftFactor = resizerLeft->resizerPosition;
    else
        leftFactor = leftFactorDefault;

    if (resizerRight)
        rightFactor = resizerRight->resizerPosition;
    else
        rightFactor = rightFactorDefault;

    if (resizerTop)
        topFactor = resizerTop->resizerPosition;
    else
        topFactor = topFactorDefault;

    if (resizerBottom)
        bottomFactor = resizerBottom->resizerPosition;
    else
        bottomFactor = bottomFactorDefault;

    auto leftPointX = bounds.getWidth() * leftFactor;
    auto leftPointY = bounds.getHeight() * topFactor;
    auto width = (bounds.getWidth() * rightFactor) - leftPointX;
    auto height = (bounds.getHeight() * bottomFactor) - leftPointY;

    auto factorBounds = Rectangle<float>(leftPointX, leftPointY, width, height);
    setBounds(factorBounds.toNearestIntEdges());
}

void ResizableTabbedComponent::resized()
{
    auto bounds = getLocalBounds();
    if (oldBounds == bounds)
        return;

    oldBounds = bounds;

    updateDropZones();

    tabComponent->setBounds(getLocalBounds());
}

void ResizableTabbedComponent::paintOverChildren(Graphics& g)
{
#if (ENABLE_SPLITS_DROPZONE_DEBUGGING == 1)
    for (auto const& [zone, path] : dropZones) {
        static Random rng;
        uint8 R = rng.nextInt(255);
        uint8 G = rng.nextInt(255);
        uint8 B = rng.nextInt(255);
        g.setColour(Colour(R, G, B, (uint8)0x50));
        g.fillPath(path);
    }
#endif

    g.setColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.15f));

    if (isDragAndDropOver) {
        Rectangle<int> highlight;
        switch (activeZone) {
        case DropZones::Left:
            highlight = splitBoundsLeft;
            break;
        case DropZones::Top:
            // highlight = splitBoundsTop;
            if (editor->splitView.splits.size() > 1) {
                highlight = splitBoundsFull;
            }
            break;
        case DropZones::Right:
            highlight = splitBoundsRight;
            break;
        case DropZones::Bottom:
            // highlight = splitBoundsBottom;
            if (editor->splitView.splits.size() > 1) {
                highlight = splitBoundsFull;
            }
            break;
        case DropZones::Centre:
        case DropZones::TabBar:
            if (editor->splitView.splits.size() > 1) {
                highlight = getLocalBounds();
            }
            break;
        };

        g.fillRect(highlight);
    };
}

void ResizableTabbedComponent::itemDragEnter(SourceDetails const& dragSourceDetails)
{
    editor->splitView.setFocus(this);
    // if we are dragging a tabbar, update the highlight split
    if (dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        isDragAndDropOver = true;
        repaint();
    }
}

void ResizableTabbedComponent::itemDragExit(SourceDetails const& dragSourceDetails)
{
    if (dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        isDragAndDropOver = false;
        repaint();
    }
}

void ResizableTabbedComponent::itemDragMove(SourceDetails const& dragSourceDetails)
{
    // if we are dragging a tabbed window or from the document browser
    if (auto sourceTabButton = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        auto sourceTabContent = sourceTabButton->getTabComponent();
        int sourceNumTabs = sourceTabContent->getNumTabs();

        auto zone = findZoneFromSource(dragSourceDetails);

        if (editor->splitView.canSplit() && sourceNumTabs > 1) {
            if (activeZone != zone) {
                activeZone = zone;
                repaint();
                // std::cout << "dragging over: " << getZoneName(zone) << std::endl;
            }
        } else if (sourceTabButton->getTabComponent() != tabComponent.get()) {
            auto foundZone = zone == DropZones::TabBar ? DropZones::None : DropZones::Centre;
            if (activeZone != foundZone) {
                activeZone = foundZone;
                repaint();
            }
        } else if (sourceTabButton->getTabComponent() == tabComponent.get()) {
            if (activeZone != DropZones::None) {
                activeZone = DropZones::None;
                repaint();
            }
        }
    }
}

void ResizableTabbedComponent::updateDropZones()
{
    auto objectBounds = getLocalBounds();

    auto vHalf = objectBounds.getHeight() * 0.5f;
    auto hHalf = objectBounds.getWidth() * 0.5f;
    splitBoundsTop = objectBounds.withBottom(vHalf);
    splitBoundsBottom = objectBounds.withTop(vHalf);
    splitBoundsRight = objectBounds.withLeft(hHalf);
    splitBoundsLeft = objectBounds.withRight(hHalf);
    splitBoundsFull = objectBounds;

    Rectangle<float> innerRect;
    auto localBounds = objectBounds.toFloat();
    auto tabbarBounds = localBounds.removeFromTop(tabComponent->getTabBarDepth());
    auto canvasBounds = localBounds.withTop(tabComponent->getTabBarDepth());
    auto bounds = canvasBounds.reduced(canvasBounds.getWidth() * 0.25f, canvasBounds.getHeight() * 0.25f);
    innerRect.setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());

    /*
    DROP ZONE ARRANGEMENT
    ┌─────────────────────────┐
    │3                       0│
    │ \                     / │
    │  \        TOP        /  │
    │   \                 /   │
    │    ┌───────────────┐    │
    │    │7             4│    │
    │ L  │   TAB CENTRE  │ R  │
    │    │6             5│    │
    │    └───────────────┘    │
    │   /                 \   │
    │  /      BOTTOM       \  │
    │ /                     \ │
    │2                       1│
    └─────────────────────────┘
    */

    auto point_0 = canvasBounds.getTopRight();
    auto point_1 = canvasBounds.getBottomRight();
    auto point_2 = canvasBounds.getBottomLeft();
    auto point_3 = canvasBounds.getTopLeft();
    auto point_4 = innerRect.getTopRight();
    auto point_5 = innerRect.getBottomRight();
    auto point_6 = innerRect.getBottomLeft();
    auto point_7 = innerRect.getTopLeft();

    Path zoneLeft, zoneTop, zoneRight, zoneBottom, zoneTabCentre, zoneTab;

    zoneLeft.addQuadrilateral(point_3.x, point_3.y, point_7.x, point_7.y, point_6.x, point_6.y, point_2.x, point_2.y);
    zoneTop.addQuadrilateral(point_3.x, point_3.y, point_0.x, point_0.y, point_4.x, point_4.y, point_7.x, point_7.y);
    zoneRight.addQuadrilateral(point_4.x, point_4.y, point_0.x, point_0.y, point_1.x, point_1.y, point_5.x, point_5.y);
    zoneBottom.addQuadrilateral(point_6.x, point_6.y, point_5.x, point_5.y, point_1.x, point_1.y, point_2.x, point_2.y);
    zoneTabCentre.addRectangle(innerRect);
    zoneTab.addRectangle(tabbarBounds);

    dropZones[0] = std::tuple<DropZones, Path>(DropZones::Left, zoneLeft);
    dropZones[1] = std::tuple<DropZones, Path>(DropZones::Top, zoneTop);
    dropZones[2] = std::tuple<DropZones, Path>(DropZones::Right, zoneRight);
    dropZones[3] = std::tuple<DropZones, Path>(DropZones::Bottom, zoneBottom);
    dropZones[4] = std::tuple<DropZones, Path>(DropZones::Centre, zoneTabCentre);
    dropZones[5] = std::tuple<DropZones, Path>(DropZones::TabBar, zoneTab);
}

TabComponent* ResizableTabbedComponent::getTabComponent()
{
    return tabComponent.get();
}
