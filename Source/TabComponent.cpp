#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Canvas.h"
#include "Sidebar/Sidebar.h"
#include "Dialogs/Dialogs.h"
#include "Utility/Autosave.h"
#include "Components/ObjectDragAndDrop.h"
#include "NVGSurface.h"

TabComponent::TabComponent(PluginEditor* editor) : editor(editor), pd(editor->pd)
{
    for(int i = 0; i < tabbars.size(); i++)
    {
        addChildComponent(newTabButtons[i]);
        newTabButtons[i].onClick = [this, i](){
            activeSplitIndex = i;
            newPatch();
        };
    }
    
    addMouseListener(this, true);
}

Canvas* TabComponent::newPatch()
{
    return openPatch(pd::Instance::defaultPatch);
}

Canvas* TabComponent::openPatch(const URL& path)
{
    auto patchFile = path.getLocalFile();
    for (auto* cnv : canvases) {
        if (cnv->patch.getCurrentFile() == patchFile) {
            pd->logError("Patch is already open");
            showTab(cnv);
            return cnv;
        }
    }
    
    auto patch = pd->loadPatch(path, editor);
    return openPatch(patch);
}

Canvas* TabComponent::openPatch(const String& patchContent)
{
    auto patch = pd->loadPatch(patchContent, editor);
    patch->setUntitled();
    return openPatch(patch);
}

Canvas* TabComponent::openPatch(pd::Patch::Ptr existingPatch)
{
    // Check if subpatch is already opened
    for (auto* cnv : canvases) {
        if (cnv->patch == *existingPatch) {
            pd->logError("Patch is already open");
            showTab(cnv);
            return cnv;
        }
    }
    
    pd->patches.addIfNotAlreadyThere(existingPatch);
    auto* cnv = canvases.add(new Canvas(editor, existingPatch));
    
    auto patchTitle = existingPatch->getTitle();
    // Open help files and references in Locked Mode
    if (patchTitle.contains("-help") || patchTitle.equalsIgnoreCase("reference"))
        cnv->locked.setValue(true);
    
    existingPatch->splitViewIndex = activeSplitIndex;
    
    update();
    pd->titleChanged();
    
    showTab(cnv, activeSplitIndex);
    closeEmptySplits();
    
    cnv->jumpToOrigin();
    
    return cnv;
}

void TabComponent::openPatch()
{
    Dialogs::showOpenDialog([this](URL resultURL) {
        auto result = resultURL.getLocalFile();
        if (result.exists() && result.getFileExtension().equalsIgnoreCase(".pd")) {
            editor->autosave->checkForMoreRecentAutosave(result, [this, result, resultURL]() {
                openPatch(resultURL);
                SettingsFile::getInstance()->addToRecentlyOpened(result);
            });
        }
    },
        true, false, "*.pd", "Patch", this);
}

void TabComponent::nextTab() {
    
    auto splitIndex = activeSplitIndex && splits[1];
    auto& tabbar = tabbars[splitIndex];
    auto oldTabIndex = 0;
    for(int i = 0; i < tabbar.size(); i++)
    {
        if(tabbar[i]->cnv == splits[splitIndex])
        {
            oldTabIndex = i;
        }
    }
    
    auto newTabIndex = oldTabIndex + 1;
    if(newTabIndex < tabbar.size())
    {
        showTab(tabbar[newTabIndex]->cnv);
    }
    else {
        showTab(tabbar[0]->cnv);
    }
}

void TabComponent::previousTab() {
    auto splitIndex = activeSplitIndex && splits[1];
    auto& tabbar = tabbars[splitIndex];
    auto oldTabIndex = 0;
    for(int i = 0; i < tabbar.size(); i++)
    {
        if(tabbar[i]->cnv == splits[splitIndex])
        {
            oldTabIndex = i;
        }
    }
    
    auto newTabIndex = oldTabIndex - 1;
    if(newTabIndex >= 0)
    {
        showTab(tabbar[newTabIndex]->cnv);
    }
    else {
        showTab(tabbar[tabbar.size() - 1]->cnv);
    }
}


void TabComponent::update()
{
    tabbars[0].clear();
    tabbars[1].clear();
    
    // Load all patches from pd patch array
    for(auto& patch : pd->patches)
    {
        Canvas* cnv = nullptr;
        for(auto* canvas : canvases)
        {
            if(canvas->patch == *patch)
            {
                cnv = canvas;
            }
        }
        
        if(!cnv)  {
            cnv = canvases.add(new Canvas(editor, patch));
            cnv->jumpToOrigin();
        }
        
        // Create tab buttons
        auto* newTabButton = new TabBarButtonComponent(cnv, this);
        tabbars[patch->splitViewIndex == 1].add(newTabButton);
        addAndMakeVisible(newTabButton);
    }

    closeEmptySplits();
    
    resized(); // Update tab and canvas layout
}

void TabComponent::closeEmptySplits()
{
    if(!tabbars[0].size() && tabbars[1].size()) // Check if split can be closed
    {
        // Move all tabs to left split
        for(int i = tabbars[1].size() - 1; i >= 0; i--)
        {
            tabbars[1][i]->cnv->patch.splitViewIndex = 0; // Save split index
            tabbars[0].insert(0, tabbars[1].removeAndReturn(i)); // Move to other split
        }
        
        showTab(tabbars[0][0]->cnv, 0);
    }
    if(tabbars[0].size() && !splits[0]) {
        showTab(tabbars[0][0]->cnv, 0);
    }
    if(tabbars[1].size() && !splits[1])
    {
        showTab(tabbars[1][0]->cnv, 1);
    }
    if(!tabbars[1].size() && splits[1]) // Check if right split is valid
    {
        showTab(nullptr, 1);
    }
    if(!tabbars[0].size() && splits[0]) // Check if left split is valid
    {
        showTab(nullptr, 0);
    }
    // Check for tabs that are shown inside the wrong split, dragging tabs can cause that
    for(int i = 0; i < tabbars.size(); i++)
    {
        for(auto* tab : tabbars[i])
        {
            if(tab->cnv == splits[!i] && tabbars[!i].size())
            {
                showTab(tabbars[!i][0]->cnv, !i);
                break;
            }
        }
    }
}

void TabComponent::showTab(Canvas* cnv, int splitIndex)
{
    if(splits[splitIndex]) removeChildComponent(splits[splitIndex]->viewport.get());
    
    splits[splitIndex] = cnv;
    
    if(cnv) {
        addAndMakeVisible(cnv->viewport.get());
        cnv->setVisible(true);
        cnv->grabKeyboardFocus();
        cnv->patch.splitViewIndex = splitIndex;
        activeSplitIndex = splitIndex;
    }
    
    resized();
    repaint();
    
    editor->nvgSurface.invalidateAll();
}

Canvas* TabComponent::getCurrentCanvas()
{
    return activeSplitIndex && splits[1] ? splits[1] : splits[0];
}

Array<Canvas*> TabComponent::getCanvases()
{
    Array<Canvas*> canvas;
    canvas.addArray(canvases);
    return canvas;
}

void TabComponent::renderArea(NVGcontext* nvg, Rectangle<int> area)
{
    if(splits[0])
    {
        nvgSave(nvg);
        nvgScissor(nvg, 0, 0, splits[1] ? (splitSize - 3) : getWidth(), getHeight());
        splits[0]->performRender(nvg, area);
        nvgRestore(nvg);
    }
    if(splits[1])
    {
        nvgSave(nvg);
        nvgTranslate(nvg, splitSize + 3, 0);
        nvgScissor(nvg, 0, 0, getWidth() - (splitSize + 3), getHeight());
        
        splits[1]->performRender(nvg, area.translated(-(splitSize + 3), 0));
        nvgRestore(nvg);
    }
    
    if(!splitDropBounds.isEmpty())
    {
        nvgBeginPath(nvg);
        nvgRect(nvg, splitDropBounds.getX(), splitDropBounds.getY(), splitDropBounds.getWidth(), splitDropBounds.getHeight());
        nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::dataColourId).withAlpha(0.1f)));
        nvgFill(nvg);
    }
    
    if(splits[1])
    {
        nvgBeginPath(nvg);
        nvgRect(nvg, splitSize - 3, 0, 6, getHeight());
        nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::canvasBackgroundColourId)));
        nvgFill(nvg);
        
        auto activeSplitBounds = splits[activeSplitIndex]->viewport->getBounds().translated(0, -30);
        nvgBeginPath(nvg);
        nvgRect(nvg, activeSplitBounds.getX(), activeSplitBounds.getY(), activeSplitBounds.getWidth(), activeSplitBounds.getHeight());
        nvgStrokeWidth(nvg, 3.0f);
        nvgStrokeColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.25f)));
        nvgStroke(nvg);
    }
}

void TabComponent::mouseDown(const MouseEvent& e)
{
    if(e.eventTime.getMillisecondCounter() == lastMouseTime) return;
    lastMouseTime = e.eventTime.getMillisecondCounter();
    
    auto localPos = e.getEventRelativeTo(this).getPosition();
    if(localPos.x > splitSize - 3 && localPos.x < splitSize + 3)
    {
        draggingSplitResizer = true;
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else if(splits[1] && localPos.x > splitSize)
    {
        setActiveSplit(splits[1]);
    }
    else {
        setActiveSplit(splits[0]);
    }
}

void TabComponent::mouseUp(const MouseEvent& e)
{
    draggingSplitResizer = false;
    e.eventComponent->setMouseCursor(MouseCursor::NormalCursor);
}

void TabComponent::mouseDrag(const MouseEvent& e)
{
    if(e.eventTime.getMillisecondCounter() == lastMouseTime) return;
    lastMouseTime = e.eventTime.getMillisecondCounter();
    
    auto localPos = e.getEventRelativeTo(this).getPosition();
    if(draggingSplitResizer) {
        splitSize = localPos.x;
        resized();
    }
}

void TabComponent::mouseMove(const MouseEvent& e)
{
    if(e.eventTime.getMillisecondCounter() == lastMouseTime) return;
    lastMouseTime = e.eventTime.getMillisecondCounter();
    
    auto localPos = e.getEventRelativeTo(this).getPosition();
    if(localPos.x > splitSize - 3 && localPos.x < splitSize + 3)
    {
        e.eventComponent->setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else {
        e.eventComponent->setMouseCursor(MouseCursor::NormalCursor);
    }
}


void TabComponent::parentSizeChanged()
{
    // TODO: keep split proportion?
    splitSize = getWidth() / 2;
}

void TabComponent::resized()
{
    auto isSplit = splits[1] != nullptr;
    auto bounds = getLocalBounds();
    auto tabbarBounds = bounds.removeFromTop(30);
    auto& animator = Desktop::getInstance().getAnimator();
    
    for(int i = 0; i < tabbars.size(); i++)
    {
        auto& tabButtons = tabbars[i];
        auto splitBounds = tabbarBounds.removeFromLeft((isSplit && i == 0) ? splitSize : getWidth());
        newTabButtons[i].setBounds(splitBounds.removeFromLeft(30));
        auto tabWidth = splitBounds.getWidth() / std::max(1, tabButtons.size());
        
        for(auto* tabButton : tabButtons)
        {
            auto targetBounds = splitBounds.removeFromLeft(tabWidth);
            if(tabButton->isDragging)  {
                tabButton->setSize(tabWidth, 30);
                if(splits[1]) {
                    tabButton->dragger.updateMouseDownPosition(tabButton->getLocalBounds().getCentre());
                }
                continue; // We reserve space for it, but don't set the bounds to create a ghost tab
            }
            
            if(draggingOverTabbar)
            {
                animator.animateComponent(tabButton, targetBounds,  1.0f, 200, false, 3.0, 0.0);
            }
            else {
                tabButton->setBounds(targetBounds);
            }
        }
    }
    
    for(auto& split : splits)
    {
        if(split)
        {
            split->viewport->setBounds(bounds.removeFromLeft((isSplit && split == splits[0]) ? (splitSize - 3) : getWidth()));
            bounds.removeFromLeft(6); // For split resizer
        }
    }
    
    newTabButtons[0].setVisible(!tabbars[0].isEmpty());
    newTabButtons[1].setVisible(!tabbars[1].isEmpty());
}

void TabComponent::closeTab(Canvas* cnv) {
    auto patch = cnv->refCountedPatch;

    editor->sidebar->hideParameters();
    editor->sidebar->clearSearchOutliner();

    patch->setVisible(false);
    
    cnv->setCachedComponentImage(nullptr); // Clear nanovg invalidation listener, just to be sure
    canvases.removeObject(cnv);
    
    pd->patches.removeFirstMatchingValue(patch);
    pd->updateObjectImplementations();
    
    update();
}



void TabComponent::closeAllTabs() {
    editor->closeAllTabs();
}

void TabComponent::setActiveSplit(Canvas* cnv) {
    if(cnv != splits[activeSplitIndex]) {
        activeSplitIndex = cnv == splits[1] ? 1 : 0;
        editor->nvgSurface.invalidateAll();
    }
}

Canvas* TabComponent::getCanvasAtScreenPosition(Point<int> screenPosition)
{
    auto x = getLocalPoint(nullptr, screenPosition).x;
    auto split = x < splitSize || !splits[1];
    return split ? splits[0] : splits[1];
    // TODO: return nullptr if OOB
}

Array<Canvas*> TabComponent::getVisibleCanvases()
{
    Array<Canvas*> result;
    if(splits[0]) result.add(splits[0]);
    if(splits[1]) result.add(splits[1]);
    return result;
}


bool TabComponent::isInterestedInDragSource(SourceDetails const& dragSourceDetails)
{
    if (dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) 
    {
        return true;
    }
    
    if(dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get())) 
    {
        return true;
    }
    
    return false;
}

void TabComponent::itemDropped(SourceDetails const& dragSourceDetails)
{
    if(auto* objectDnD = dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get()))
    {
        auto screenPosition = localPointToGlobal(dragSourceDetails.localPosition);
        
        auto* cnv = getCanvasAtScreenPosition(screenPosition);
        if (!cnv)
            return;

        auto mousePos = (cnv->getLocalPoint(this, dragSourceDetails.localPosition) - cnv->canvasOrigin);

        // Extract the array<var> from the var
        auto patchWithSize = *dragSourceDetails.description.getArray();
        auto patchSize = Point<int>(patchWithSize[0], patchWithSize[1]);
        auto patchData = patchWithSize[2].toString();
        auto patchName = patchWithSize[3].toString();

        cnv->dragAndDropPaste(patchData, mousePos, patchSize.x, patchSize.y, patchName);
        setActiveSplit(cnv);
        return;
    }
    
    auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    
    auto showCanvas = [this](Canvas* cnv, int splitIndex){
        if(splits[splitIndex]) removeChildComponent(splits[splitIndex]->viewport.get());
        splits[splitIndex] = cnv;
        if(cnv) {
            addAndMakeVisible(cnv->viewport.get());
            cnv->setVisible(true);
            cnv->grabKeyboardFocus();
            cnv->patch.splitViewIndex = splitIndex; // Save split index
            activeSplitIndex = splitIndex;
            editor->nvgSurface.invalidateAll();
        }
    };
    
    if(getLocalBounds().removeFromRight(getWidth() - splitSize).contains(dragSourceDetails.localPosition)) // Dragging to right split
    {
        if(tabbars[0].size() && splits[0] && tabbars[0].indexOf(tab) >= 0 && (dragSourceDetails.localPosition.y > 30 || splits[1]))
        {
            tabbars[1].add(tabbars[0].removeAndReturn(tabbars[0].indexOf(tab))); // Move tab to right tabbar
            if(tabbars[0].size()) showCanvas(tabbars[0][0]->cnv, 0); // Show first tab of left tabbar
            showCanvas(tab->cnv, 1); // Show the moved tab on right tabbar
            resized();
        }
    }
    else {
        if(tabbars[1].size() && splits[1] && tabbars[1].indexOf(tab) >= 0) {
            tabbars[0].add(tabbars[1].removeAndReturn(tabbars[1].indexOf(tab)));  // Move tab to left tabbar
            if(tabbars[1].size()) showCanvas(tabbars[1][0]->cnv, 1); // Show first tab of right tabbar, if there are any tabs left
            else showCanvas(nullptr, 1); // If no tabs are left on right tabbar
            
            showCanvas(tab->cnv, 0); // Show moved tab in left tabbar
            resized();
        }
        else if(tabbars[0].size() > 1 && splits[0] && !splits[1] && tabbars[0].indexOf(tab) >= 0 && (dragSourceDetails.localPosition.y > 30)) // If we try to create a left splits when there are no splits open
        {
            showCanvas(tab->cnv, 0);  // Show dragged tab on left split
            
            // Move all other tabs to right split
            for(int i = tabbars[0].size() - 1; i >= 0; i--)
            {
                if(tab != tabbars[0][i]) {
                    tabbars[0][i]->cnv->patch.splitViewIndex = 1; // Save split index
                    tabbars[1].insert(0, tabbars[0].removeAndReturn(i)); // Move to other split
                }
            }

            showCanvas(tabbars[1][0]->cnv, 1); // Show first tab of right split
            resized();
        }
    }

    closeEmptySplits();
    saveTabPositions();
    
    draggingOverTabbar = false;
    splitDropBounds = Rectangle<int>();
    editor->nvgSurface.invalidateAll();
}

void TabComponent::itemDragEnter(SourceDetails const& dragSourceDetails)
{
    itemDragMove(dragSourceDetails);
}

void TabComponent::itemDragExit(SourceDetails const& dragSourceDetails)
{
    auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    tab->setVisible(false);
    splitDropBounds = Rectangle<int>();
    draggingOverTabbar = false;
}

void TabComponent::saveTabPositions()
{
    Array<std::tuple<pd::Patch::Ptr, int>> sortedPatches;
    for(int i = 0; i < tabbars.size(); i++) {
        for(int j = 0; j < tabbars[i].size(); j++)
        {
            if(auto* cnv = tabbars[i][j]->cnv.getComponent()) {
                cnv->patch.splitViewIndex = i;
                sortedPatches.add({ cnv->refCountedPatch, j });
            }
        }
    }

    std::sort(sortedPatches.begin(), sortedPatches.end(), [](auto const& a, auto const& b) {
        auto& [patchA, idxA] = a;
        auto& [patchB, idxB] = b;

        if (patchA->splitViewIndex == patchB->splitViewIndex)
            return idxA < idxB;

        return patchA->splitViewIndex < patchB->splitViewIndex;
    });

    pd->patches.getLock().enter();
    int i = 0;
    for (auto& [patch, tabIdx] : sortedPatches) {

        if (i >= pd->patches.size())
            break;

        pd->patches.set(i, patch);
        i++;
    }
    pd->patches.getLock().exit();
}

void TabComponent::itemDragMove(SourceDetails const& dragSourceDetails)
{
    auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    if(!tab) return;
    
    Rectangle<int> oldSplitDropBounds = splitDropBounds;
    
    if(getLocalBounds().removeFromTop(30).contains(dragSourceDetails.localPosition)) // Dragging over tabbar
    {
        draggingOverTabbar = true;
        splitDropBounds = Rectangle<int>();
        tab->setVisible(true);
        
        if(tab->parent != this)
        {
            // TODO: split refactor, handle drag to other window!
        }
        
        auto centreX = tab->getBounds().getCentreX();
        auto tabBarWidth = splits[1] ? getWidth() /  2 : getWidth();
        int hoveredSplit = splits[1] && centreX > splitSize;
        
        // Calculate target tab index based on tabbar width and tab centre position
        auto targetTabPos = tabBarWidth / std::max(1, tabbars[hoveredSplit].size());
        auto tabPos = (centreX - (hoveredSplit ? tabBarWidth : 0)) / targetTabPos;
        
        auto oldPos = tabbars[hoveredSplit].indexOf(tab);
        if(oldPos != tabPos) {
            if(oldPos != -1) { // Dragging inside same tabbar
                tabbars[hoveredSplit].move(oldPos, tabPos);
                resized();
            }
            else if(splits[1]) { // Dragging to another tabbar
                tabbars[hoveredSplit].insert(tabPos, tabbars[!hoveredSplit].removeAndReturn(tabbars[!hoveredSplit].indexOf(tab)));
                resized();
            }
        }
    }
    else if(getLocalBounds().removeFromRight(getWidth() - splitSize).contains(dragSourceDetails.localPosition)) // Dragging over right split
    {
        draggingOverTabbar = false;
        splitDropBounds = getLocalBounds().removeFromRight(splitSize);
        tab->setVisible(false);
    }
    else { // Dragging over left split
        draggingOverTabbar = false;
        splitDropBounds = getLocalBounds().removeFromLeft(splitSize);
        tab->setVisible(false);
    }
    
    if(splitDropBounds != oldSplitDropBounds)
    {
        // Repaint for updated split drop bounds
        editor->nvgSurface.invalidateAll();
    }
}
