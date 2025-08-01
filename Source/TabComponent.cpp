#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Canvas.h"
#include "Sidebar/Sidebar.h"
#include "Dialogs/Dialogs.h"
#include "Utility/Autosave.h"
#include "Components/ObjectDragAndDrop.h"
#include "NVGSurface.h"
#include "PluginMode.h"
#include "Standalone/PlugDataWindow.h"

TabComponent::TabComponent(PluginEditor* editor)
    : editor(editor)
    , pd(editor->pd)
{
    for (int i = 0; i < tabbars.size(); i++) {
        addChildComponent(newTabButtons[i]);
        newTabButtons[i].onClick = [this, i] {
            activeSplitIndex = i;
            newPatch();
        };

        addChildComponent(tabOverflowButtons[i]);
        tabOverflowButtons[i].onClick = [this, i] {
            showHiddenTabsMenu(i);
        };
    }

    addMouseListener(this, true);

    // Dequeue messages to "pd" symbol to make sure the "pluginmode" message always arrives earlier than the tab update.
    // Without this, FL Studio (and possibly others) will fail to init the pluginmode theme!
    editor->pd->triggerAsyncUpdate();

    triggerAsyncUpdate();
}

TabComponent::~TabComponent()
{
    sendTabUpdateToVisibleCanvases();
    clearCanvases();
}

Canvas* TabComponent::newPatch()
{
    return openPatch(pd::Instance::defaultPatch);
}

Canvas* TabComponent::openPatch(const URL& path)
{
    auto const patchFile = path.getLocalFile();

    for (auto* editor : pd->getEditors()) {
        for (auto* cnv : editor->getCanvases()) {
            if (cnv->patch.getCurrentFile() == patchFile) {
                pd->logError("Patch is already open");
                editor->getTopLevelComponent()->toFront(true);
                editor->getTabComponent().showTab(cnv, cnv->patch.splitViewIndex);
                editor->getTabComponent().setActiveSplit(cnv);
                return cnv;
            }
        }
    }

    auto const patch = pd->loadPatch(path);
    
    // If we're opening a temp file, assume it's dirty upon opening
    // This is so that you can recover an autosave without directly overewriting it, but still be prompted to save if you close the autosaved patch
    if(path.getLocalFile().getParentDirectory() == File::getSpecialLocation(File::tempDirectory))
    {
        if(auto p = patch->getPointer()) {
            canvas_dirty(p.get(), 1.0f);
        }
    }
    return openPatch(patch, true);
}

Canvas* TabComponent::openPatch(String const& patchContent)
{
    auto const patch = pd->loadPatch(patchContent);
    patch->setUntitled();
    return openPatch(patch);
}

Canvas* TabComponent::openPatch(pd::Patch::Ptr existingPatch, bool const warnIfAlreadyOpen)
{
    if (!existingPatch)
        return nullptr;

    // Check if subpatch is already opened
    for (auto* editor : pd->getEditors()) {
        for (auto* cnv : editor->getCanvases()) {
            if (cnv->patch == *existingPatch) {
                if (warnIfAlreadyOpen)
                    pd->logError("Patch is already open");
                editor->getTopLevelComponent()->toFront(true);
                editor->getTabComponent().showTab(cnv, cnv->patch.splitViewIndex);
                editor->getTabComponent().setActiveSplit(cnv);
                return cnv;
            }
        }
    }

    pd->patches.add_unique(existingPatch, [](auto const& ptr1, auto const& ptr2) {
        return *ptr1 == *ptr2;
    });

    existingPatch->splitViewIndex = activeSplitIndex;
    existingPatch->windowIndex = editor->editorIndex;
    if (existingPatch->openInPluginMode) {
        triggerAsyncUpdate();
        return nullptr;
    }

    auto* cnv = canvases.add(new Canvas(editor, existingPatch));

    auto const patchTitle = existingPatch->getTitle();
    // Open help files and references in Locked Mode
    if (patchTitle.contains("-help") || patchTitle.equalsIgnoreCase("reference"))
        cnv->locked.setValue(true);

    showTab(cnv, activeSplitIndex);
    cnv->restoreViewportState();

    triggerAsyncUpdate();
    tabVisibilityMessageUpdater.triggerAsyncUpdate();

    static bool alreadyOpeningInNewWindow = false;
    if (canvases.size() > 1 && !alreadyOpeningInNewWindow && ProjectInfo::isStandalone && SettingsFile::getInstance()->getProperty<bool>("open_patches_in_window")) {
        alreadyOpeningInNewWindow = true;
        cnv = createNewWindow(cnv);
        alreadyOpeningInNewWindow = false;
    }

    return cnv;
}

void TabComponent::openPatch()
{
    Dialogs::showOpenDialog([this](URL resultURL) {
        auto result = resultURL.getLocalFile();
        if (result.exists() && result.getFileExtension().equalsIgnoreCase(".pd")) {
            editor->pd->autosave->checkForMoreRecentAutosave(result, editor, [this, result, resultURL](File patchFile, File patchPath) {
                auto* cnv = openPatch(URL(patchFile));
                if(cnv)
                {
                    cnv->patch.setCurrentFile(URL(patchPath));
                }
                SettingsFile::getInstance()->addToRecentlyOpened(patchPath);
            });
        }
    },
        true, false, "*.pd", "Patch", this);
}

void TabComponent::moveToLeftSplit(TabBarButtonComponent* tab)
{
    if (tab->parent != this) // Move to another window
    {
        if (tab->parent->tabbars[0].contains(tab)) {
            auto const patch = tab->cnv->refCountedPatch;
            patch->windowIndex = editor->editorIndex;

            auto* oldTabbar = tab->parent;
            oldTabbar->canvases.removeObject(tab->cnv);
            oldTabbar->tabbars[0].removeObject(tab);

            auto* cnv = canvases.add(new Canvas(editor, patch));
            tabbars[0].add(new TabBarButtonComponent(cnv, this));
            showTab(cnv, 0);

            cnv->restoreViewportState();
            triggerAsyncUpdate();
            oldTabbar->triggerAsyncUpdate();
        } else if (tab->parent->tabbars[1].contains(tab)) {
            auto const patch = tab->cnv->refCountedPatch;
            patch->windowIndex = editor->editorIndex;

            auto* oldTabbar = tab->parent;
            oldTabbar->canvases.removeObject(tab->cnv);
            oldTabbar->tabbars[1].removeObject(tab);

            auto* cnv = canvases.add(new Canvas(editor, patch));
            tabbars[0].add(new TabBarButtonComponent(cnv, this));
            showTab(cnv, 0);

            cnv->restoreViewportState();
            triggerAsyncUpdate();
            oldTabbar->triggerAsyncUpdate();
        }
        return;
    }

    if (tabbars[1].size() && splits[1] && tabbars[1].indexOf(tab) >= 0) {
        tabbars[0].add(tabbars[1].removeAndReturn(tabbars[1].indexOf(tab))); // Move tab to left tabbar
        if (tabbars[1].size())
            showTab(tabbars[1][0]->cnv, 1); // Show first tab of right tabbar, if there are any tabs left
        else
            showTab(nullptr, 1); // If no tabs are left on right tabbar

        showTab(tab->cnv, 0);                                                                    // Show moved tab on left split
    } else if (tabbars[0].size() > 1 && splits[0] && !splits[1] && tabbars[0].indexOf(tab) >= 0) // If we try to create a left splits when there are no splits open
    {
        showTab(tab->cnv, 0); // Show dragged tab on left split

        // Move all other tabs to right split
        for (int i = tabbars[0].size() - 1; i >= 0; i--) {
            if (tab != tabbars[0][i]) {
                tabbars[0][i]->cnv->patch.splitViewIndex = 1;        // Save split index
                tabbars[1].insert(0, tabbars[0].removeAndReturn(i)); // Move to other split
            }
        }

        showTab(tabbars[1][0]->cnv, 1); // Show first tab of right split
    }
}

void TabComponent::moveToRightSplit(TabBarButtonComponent* tab)
{
    if (tab->parent != this) // Move to another window
    {
        if (tab->parent->tabbars[0].contains(tab)) {
            auto const patch = tab->cnv->refCountedPatch;
            patch->windowIndex = editor->editorIndex;

            auto* oldTabbar = tab->parent;
            oldTabbar->canvases.removeObject(tab->cnv);
            oldTabbar->tabbars[0].removeObject(tab);

            auto* cnv = canvases.add(new Canvas(editor, patch));
            cnv->restoreViewportState();
            tabbars[1].add(new TabBarButtonComponent(cnv, this));
            showTab(cnv, 1);

            triggerAsyncUpdate();
            oldTabbar->triggerAsyncUpdate();
        } else if (tab->parent->tabbars[1].contains(tab)) {
            auto const patch = tab->cnv->refCountedPatch;
            patch->windowIndex = editor->editorIndex;

            auto* oldTabbar = tab->parent;
            oldTabbar->canvases.removeObject(tab->cnv);
            oldTabbar->tabbars[1].removeObject(tab);

            auto* cnv = canvases.add(new Canvas(editor, patch));
            cnv->restoreViewportState();
            tabbars[1].add(new TabBarButtonComponent(cnv, this));
            showTab(cnv, 1);

            triggerAsyncUpdate();
            oldTabbar->triggerAsyncUpdate();
        }
        return;
    }

    if ((tabbars[0].size() > 1 || splits[1]) && splits[0] && tabbars[0].indexOf(tab) >= 0) {
        tabbars[1].add(tabbars[0].removeAndReturn(tabbars[0].indexOf(tab))); // Move tab to right tabbar
        if (tabbars[0].size())
            showTab(tabbars[0][0]->cnv, 0); // Show first tab of left tabbar
        showTab(tab->cnv, 1);               // Show the moved tab on right tabbar
    }
}

void TabComponent::nextTab()
{
    auto const splitIndex = activeSplitIndex && splits[1];
    auto const& tabbar = tabbars[splitIndex];
    auto oldTabIndex = 0;
    for (int i = 0; i < tabbar.size(); i++) {
        if (tabbar[i]->cnv == splits[splitIndex]) {
            oldTabIndex = i;
        }
    }

    auto const newTabIndex = oldTabIndex + 1;
    showTab(newTabIndex < tabbar.size() ? tabbar[newTabIndex]->cnv : tabbar[0]->cnv, splitIndex);
}

void TabComponent::previousTab()
{
    auto const splitIndex = activeSplitIndex && splits[1];
    auto const& tabbar = tabbars[splitIndex];
    auto oldTabIndex = 0;
    for (int i = 0; i < tabbar.size(); i++) {
        if (tabbar[i]->cnv == splits[splitIndex]) {
            oldTabIndex = i;
        }
    }

    auto const newTabIndex = oldTabIndex - 1;
    showTab(newTabIndex >= 0 ? tabbar[newTabIndex]->cnv : tabbar[tabbar.size() - 1]->cnv, splitIndex);
}

void TabComponent::createNewWindowFromTab(Component* draggedTab)
{
    auto const* tab = dynamic_cast<TabBarButtonComponent*>(draggedTab);
    if (!tab)
        return;
    if(canvases.size() > 1) {
        createNewWindow(tab->cnv);
    }
}

Canvas* TabComponent::createNewWindow(Canvas* cnv)
{
    if (!ProjectInfo::isStandalone)
        return nullptr;

    auto* newEditor = new PluginEditor(*pd);
    auto* newWindow = ProjectInfo::createNewWindow(newEditor);
    auto const* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent());

    pd->openedEditors.add(newEditor);

    newWindow->addToDesktop(window->getDesktopWindowStyleFlags());
    newWindow->setVisible(true);

    auto const patch = cnv->refCountedPatch;
    closeTab(cnv);

    patch->windowIndex = newEditor->editorIndex;

    auto* newCanvas = newEditor->getTabComponent().openPatch(patch);
    newCanvas->restoreViewportState();

    newWindow->setTopLeftPosition(Desktop::getInstance().getMousePosition() - Point<int>(500, 60));
    newWindow->toFront(true);
    newEditor->nvgSurface.detachContext();

    if (SettingsFile::getInstance()->getProperty<bool>("open_patches_in_window")) {
        auto const patchBounds = newCanvas->patch.getBounds() * (SettingsFile::getInstance()->getProperty<float>("default_zoom") / 100.0f);
        auto const screenBounds = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
        auto const windowBounds = screenBounds.withSizeKeepingCentre(patchBounds.getWidth() + newEditor->sidebar->getWidth() + 30, patchBounds.getHeight() + 94);
        newEditor->getTopLevelComponent()->setBounds(windowBounds);
    }

    return newCanvas;
}

void TabComponent::openInPluginMode(pd::Patch::Ptr patch)
{
    patch->openInPluginMode = true;
    triggerAsyncUpdate();
}

// Deleting a canvas can lead to subpatches of that canvas being deleted as well
// This means that clearing all elements from the canvases array by calling 'clear()' is unsafe
// instead, we must check if they still exist before deleting
void TabComponent::clearCanvases()
{
    SmallArray<SafePointer<Canvas>, 16> safeCanvases;
    for (int i = canvases.size() - 1; i >= 0; i--) {
        safeCanvases.add(canvases[i]);
    }

    for (auto safeCnv : safeCanvases) {
        if (safeCnv)
            canvases.removeObject(safeCnv.getComponent());
    }
}

void TabComponent::updateNow()
{
    handleAsyncUpdate();
}

void TabComponent::handleAsyncUpdate()
{
    if (canvases.isEmpty() && pd->getEditors().size() > 1) {
        auto* pdInstance = pd; // Copy pd because we might self-destruct
        pdInstance->openedEditors.removeObject(editor);
        auto const* editor = pdInstance->openedEditors.getFirst();
        if (auto* topLevel = editor->getTopLevelComponent())
            topLevel->toFront(true);
        return;
    }

    pd->setThis();

    auto const editorIndex = editor->editorIndex;

    // save the patch from the canvases that were the two splits
    for (int i = 0; i < splits.size(); i++) {
        if (splits[i]) {
            lastSplitPatches[i] = &splits[i]->patch;
        }
    }
    if (getCurrentCanvas())
        lastActiveCanvas = getCurrentCanvas()->patch.getUncheckedPointer();

    for (auto* cnv : getCanvases()) {
        cnv->saveViewportState();
    }

    tabbars[0].clear();
    tabbars[1].clear();

    // Check if there is a patch that should be in plugin mode for this tabComponents editor
    // If so, we create a new pluginmode object, and delete all canvases from this editor
    if (auto patchInPluginMode = pd->findPatchInPluginMode(editor->editorIndex)) {
        if (patchInPluginMode->windowIndex == editorIndex) {
            // Initialise plugin mode
            clearCanvases();
            if (!editor->isInPluginMode() || editor->pluginMode->getPatch()->getPointer().get() != patchInPluginMode->getUncheckedPointer()) {
                editor->pluginMode = std::make_unique<PluginMode>(editor, patchInPluginMode);
            }
            editor->pluginMode->updateSize();
            editor->parentSizeChanged(); // hack to force the window title buttons to hide
            return;
        }
        // if the editor is in pluginmode
    } else if (editor->isInPluginMode()) {
        editor->pluginMode.reset(nullptr);
    }

    // First, remove canvases that no longer exist
    for (int i = canvases.size() - 1; i >= 0; i--) {
        bool exists = false;
        {
            for (auto& patch : pd->patches) {
                if (canvases[i]->patch == *patch && canvases[i]->patch.windowIndex == editorIndex) {
                    exists = true;
                }
            }
        }
        if (!exists) {
            canvases.remove(i);
        }
    }

    for (auto& patch : pd->patches) {
        if (patch->windowIndex != editorIndex)
            continue;

        Canvas* cnv = nullptr;
        for (auto* canvas : canvases) {
            if (canvas->patch == *patch) {
                cnv = canvas;
            }
        }

        if (!cnv) {
            cnv = canvases.add(new Canvas(editor, patch));
            resized();
            cnv->restoreViewportState();
        }

        // Create tab buttons
        auto* newTabButton = new TabBarButtonComponent(cnv, this);
        tabbars[patch->splitViewIndex == 1].add(newTabButton);
        addAndMakeVisible(newTabButton);
    }

    closeEmptySplits();

    // Show welcome panel if there are no tabs
    if (tabbars[0].size() == 0 && tabbars[1].size() == 0) {
        editor->showWelcomePanel(true);
        editor->nvgSurface.renderAll();
        editor->resized();
        editor->parentSizeChanged();
    } else {
        editor->showWelcomePanel(false);
        editor->resized();
        editor->parentSizeChanged();
    }

    resized(); // Update tab and canvas layout

    for (auto* cnv : getCanvases()) {
        cnv->restoreViewportState();
    }

    // Show plugin mode tab after closing pluginmode
    for (int i = 0; i < tabbars.size(); i++) {
        for (auto* canvas : getCanvases()) {
            if (!tabbars[i].isEmpty() && &canvas->patch == lastSplitPatches[i]) {
                showTab(canvas, i);
                break;
            }
        }
    }
    if (lastActiveCanvas) {
        for (auto* cnv : getCanvases()) {
            if (cnv->patch.getUncheckedPointer() == lastActiveCanvas) {
                setActiveSplit(cnv);
                break;
            }
        }
    }

    editor->updateCommandStatus();
    sendTabUpdateToVisibleCanvases();
    repaint();
}

void TabComponent::closeEmptySplits()
{
    if (!tabbars[0].size() && tabbars[1].size()) // Check if split can be closed
    {
        // Move all tabs to left split
        for (int i = tabbars[1].size() - 1; i >= 0; i--) {
            tabbars[1][i]->cnv->patch.splitViewIndex = 0;        // Save split index
            tabbars[0].insert(0, tabbars[1].removeAndReturn(i)); // Move to other split
        }

        showTab(tabbars[0][0]->cnv, 0);
    }
    if (tabbars[0].size() && !splits[0]) {
        showTab(tabbars[0][0]->cnv, 0);
    }
    if (tabbars[1].size() && !splits[1]) {
        showTab(tabbars[1][0]->cnv, 1);
    }
    if (!tabbars[1].size() && splits[1]) // Check if right split is valid
    {
        showTab(nullptr, 1);
    }
    if (!tabbars[0].size() && splits[0]) // Check if left split is valid
    {
        showTab(nullptr, 0);
    }
    // Check for tabs that are shown inside the wrong split, dragging tabs can cause that
    for (int i = 0; i < tabbars.size(); i++) {
        for (auto const* tab : tabbars[i]) {
            if (tab->cnv == splits[!i] && tabbars[!i].size()) {
                showTab(tabbars[!i][0]->cnv, !i);
                break;
            }
        }
    }
}

void TabComponent::showTab(Canvas* cnv, int const splitIndex)
{
    if (cnv == splits[splitIndex] && cnv && cnv->getParentComponent()) {
        return;
    }

    if (cnv && cnv != getCurrentCanvas()) {
        cnv->deselectAll();
        editor->sidebar->updateSearch(true);
    }

    if (splits[splitIndex] && splits[splitIndex] != splits[!splitIndex]) {
        splits[splitIndex]->saveViewportState();
        removeChildComponent(splits[splitIndex]->viewport.get());
    }

    splits[splitIndex] = cnv;

    if (cnv) {
        addAndMakeVisible(cnv->viewport.get());
        cnv->setVisible(true);
        cnv->grabKeyboardFocus();
        cnv->patch.splitViewIndex = splitIndex;
        activeSplitIndex = splitIndex;
    }

    resized();
    repaint();

    editor->nvgSurface.invalidateAll();

    tabVisibilityMessageUpdater.triggerAsyncUpdate();

    editor->sidebar->hideParameters();
    editor->sidebar->clearSearchOutliner();
    editor->updateCommandStatus();

    addLastShownTab(cnv, splitIndex);
}

void TabComponent::TabVisibilityMessageUpdater::handleAsyncUpdate()
{
    parent->sendTabUpdateToVisibleCanvases();
}

Canvas* TabComponent::getCurrentCanvas()
{
    if (editor->pluginMode) {
        return editor->pluginMode->getCanvas();
    }

    return activeSplitIndex && splits[1] ? splits[1] : splits[0];
}

SmallArray<Canvas*> TabComponent::getCanvases()
{
    SmallArray<Canvas*> allCanvases;
    allCanvases.reserve(canvases.size());
    for (auto& canvas : canvases)
        allCanvases.add(canvas);
    return allCanvases;
}

void TabComponent::renderArea(NVGcontext* nvg, Rectangle<int> area)
{
    if (splits[0]) {
        NVGScopedState scopedState(nvg);
        nvgScissor(nvg, 0, 0, splits[1] ? splitSize - 3 : getWidth(), getHeight());
        splits[0]->performRender(nvg, area);
    }
    if (splits[1]) {
        NVGScopedState scopedState(nvg);
        nvgTranslate(nvg, splitSize + 3, 0);
        nvgScissor(nvg, 0, 0, getWidth() - (splitSize + 3), getHeight());
        splits[1]->performRender(nvg, area.translated(-(splitSize + 3), 0));
    }

    if (!splitDropBounds.isEmpty()) {
        nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::dataColourId).withAlpha(0.1f)));
        nvgFillRect(nvg, splitDropBounds.getX(), splitDropBounds.getY(), splitDropBounds.getWidth(), splitDropBounds.getHeight());
    }

    if (splits[1]) {
        nvgFillColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::canvasBackgroundColourId)));
        nvgFillRect(nvg, splitSize - 3, 0, 6, getHeight());

        auto const activeSplitBounds = activeSplitIndex ? Rectangle<int>(splitSize, 0, getWidth() - splitSize, getHeight() - 31) : Rectangle<int>(0, 0, splitSize, getHeight() - 31);

        nvgStrokeWidth(nvg, 3.0f);
        nvgStrokeColor(nvg, NVGComponent::convertColour(findColour(PlugDataColour::objectSelectedOutlineColourId).withAlpha(0.25f)));
        nvgStrokeRect(nvg, activeSplitBounds.getX(), activeSplitBounds.getY(), activeSplitBounds.getWidth(), activeSplitBounds.getHeight());
    }
}

void TabComponent::mouseDown(MouseEvent const& e)
{
    auto const localPos = e.getEventRelativeTo(this).getPosition();
    if (localPos.x > splitSize - 3 && localPos.x < splitSize + 3) {
        draggingSplitResizer = true;
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    } else if (splits[1] && localPos.x > splitSize) {
        setActiveSplit(splits[1]);
    } else {
        setActiveSplit(splits[0]);
    }
}

void TabComponent::mouseUp(MouseEvent const& e)
{
    draggingSplitResizer = false;
    setMouseCursor(MouseCursor::NormalCursor);
}

void TabComponent::mouseDrag(MouseEvent const& e)
{
    auto const localPos = e.getEventRelativeTo(this).getPosition();
    if (draggingSplitResizer) {
        splitProportion = std::clamp(getWidth() / static_cast<float>(jmax(0, localPos.x)), 1.25f, 5.0f);
        splitSize = getWidth() / splitProportion;
        resized();
    }
}

void TabComponent::mouseMove(MouseEvent const& e)
{
    auto const localPos = e.getEventRelativeTo(this).getPosition();
    if (localPos.x > splitSize - 3 && localPos.x < splitSize + 3) {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
}

void TabComponent::resized()
{
    auto const isSplit = splits[1] != nullptr;
    auto bounds = getLocalBounds();
    auto tabbarBounds = bounds.removeFromTop(30);
    auto& animator = Desktop::getInstance().getAnimator();

    splitSize = getWidth() / splitProportion;

    for (int i = 0; i < tabbars.size(); i++) {
        auto& tabButtons = tabbars[i];
        auto splitBounds = tabbarBounds.removeFromLeft(isSplit && i == 0 ? splitSize : getWidth());
        newTabButtons[i].setBounds(splitBounds.removeFromLeft(30));

        auto const totalWidth = splitBounds.getWidth();
        auto tabWidth = splitBounds.getWidth() / std::max(1, tabButtons.size());
        constexpr auto minTabWidth = 75;

        if (minTabWidth * tabButtons.size() > totalWidth) {
            int tabsThatFit = totalWidth / 150;
            tabWidth = (totalWidth - 30) / std::max(1, std::min(tabButtons.size(), tabsThatFit));
        }

        bool wasOverflown = false;

        for (auto* tabButton : tabButtons) {
            if (tabWidth > splitBounds.getWidth()) {
                wasOverflown = true;
            }
            if (wasOverflown) {
                tabButton->setVisible(false);
                continue;
            }
            tabButton->setVisible(true);

            auto targetBounds = splitBounds.removeFromLeft(tabWidth);
            if (tabButton->isDragging) {
                tabButton->setSize(tabWidth, 30);
                if (splits[1]) {
                    tabButton->dragger.updateMouseDownPosition(tabButton->getLocalBounds().getCentre());
                }
                continue; // We reserve space for it, but don't set the bounds to create a ghost tab
            }

            if (draggingOverTabbar) {
                animator.animateComponent(tabButton, targetBounds, 1.0f, 200, false, 3.0, 0.0);
            } else {
                tabButton->setBounds(targetBounds);
            }
        }

        tabOverflowButtons[i].setVisible(wasOverflown);
        tabOverflowButtons[i].setBounds(splitBounds.removeFromRight(wasOverflown ? 30 : 0));
    }

    // go over all canvases in each split (a split is simply a pointer to the active canvas)
    // use the active canvas viewports dimensions (the one that is assigned to the split) for resizing each canvas
    for (int i = 0; i < splits.size(); i++) {
        if (splits[i]) {
            auto const splitBounds = bounds.removeFromLeft(isSplit && splits[i] == splits[0] ? splitSize - 3 : getWidth());
            for (auto const* tab : tabbars[i]) {
                if (auto const canvas = tab->cnv) {
                    canvas->viewport->setBounds(splitBounds);
                }
            }
            bounds.removeFromLeft(6); // For split resizer
        }
    }

    newTabButtons[0].setVisible(!tabbars[0].isEmpty());
    newTabButtons[1].setVisible(!tabbars[1].isEmpty());
}

void TabComponent::askToCloseTab(Canvas* cnv)
{
    MessageManager::callAsync([_cnv = SafePointer(cnv), _editor = SafePointer(editor), _this = SafePointer(this)]() mutable {
        // Don't show save dialog, if patch is still open in another view
        if (_editor && _cnv && _cnv->patch.isDirty()) {
            Dialogs::showAskToSaveDialog(
                &_editor->openedDialog, _editor, _cnv->patch.getTitle(),
                [_cnv, _this](int const result) mutable {
                    if (!_cnv || !_this)
                        return;
                    if (result == 2)
                        _cnv->save([_cnv, _this]() mutable { _this->closeTab(_cnv); });
                    else if (result == 1)
                        _this->closeTab(_cnv);
                },
                0, true);
        } else if (_this && _cnv) {
            _this->closeTab(_cnv);
        }
    });
}

void TabComponent::closeTab(Canvas* cnv)
{
    auto const patch = cnv->refCountedPatch;

    editor->sidebar->hideParameters();
    editor->sidebar->clearSearchOutliner();

    patch->setVisible(false);

    auto const* tab = [this, cnv] {
        for (auto& tabbar : tabbars) {
            for (auto* tab : tabbar) {
                if (tab->cnv == cnv) {
                    return tab;
                }
            }
        }
        return static_cast<TabBarButtonComponent*>(nullptr);
    }();

    cnv->setCachedComponentImage(nullptr); // Clear nanovg invalidation listener, just to be sure

    if (splits[0] == cnv) {
        if (auto* lastCnv = getLastShownTab(cnv, 0)) {
            showTab(lastCnv, 0);
        } else if (tabbars[0].indexOf(tab) >= 1) {
            showTab(tabbars[0][tabbars[0].indexOf(tab) - 1]->cnv, 0);
        }
    }
    if (splits[1] == cnv) {
        if (auto* lastCnv = getLastShownTab(cnv, 1)) {
            showTab(lastCnv, 1);
        } else if (tabbars[1].indexOf(tab) >= 1) {
            showTab(tabbars[1][tabbars[1].indexOf(tab) - 1]->cnv, 1);
        }
    }

    canvases.removeObject(cnv);
    pd->patches.remove_one(patch, [](auto const& ptr1, auto const& ptr2) {
        return *ptr1 == *ptr2;
    });

    pd->updateObjectImplementations();

    triggerAsyncUpdate();
}

void TabComponent::addLastShownTab(Canvas* tab, int const split)
{
    if (lastShownTabs[split].contains(tab))
        lastShownTabs[split].remove_one(tab);

    while (lastShownTabs[split].size() >= 12)
        lastShownTabs[split].remove_at(0);

    lastShownTabs[split].add(tab);
}

Canvas* TabComponent::getLastShownTab(Canvas* current, int const split)
{
    Canvas* lastShownTab = nullptr;
    for (auto it = lastShownTabs[split].rbegin(); it != lastShownTabs[split].rend(); ++it) {
        lastShownTab = *it;
        if (lastShownTab == current)
            continue;

        auto const index = std::distance(it, lastShownTabs[split].rend()) - 1;
        if (lastShownTab) {
            lastShownTabs[split].remove_range(index, lastShownTabs[split].size() - 1);
            break;
        }
    }
    return lastShownTab;
}

void TabComponent::sendTabUpdateToVisibleCanvases()
{
    for (auto* editorWindow : pd->getEditors()) {
        for (auto* cnv : editorWindow->getTabComponent().getVisibleCanvases()) {
            cnv->tabChanged();
        }
    }
}

void TabComponent::closeAllTabs(bool quitAfterComplete, Canvas* patchToExclude, std::function<void()> afterComplete)
{
    if (!canvases.size()) {
        afterComplete();
        if (quitAfterComplete) {
            JUCEApplication::quit();
        }
        return;
    }
    if (patchToExclude && canvases.size() == 1) {
        afterComplete();
        return;
    }

    auto canvas = SafePointer<Canvas>(canvases.getLast());
    auto patch = canvas->refCountedPatch;

    auto deleteFunc = [this, canvas, quitAfterComplete, patchToExclude, afterComplete] {
        if (canvas && !(patchToExclude && canvas == patchToExclude)) {
            closeTab(canvas);
        } else if (canvas == patchToExclude) {
            canvases.move(canvases.indexOf(patchToExclude), 0);
        }

        closeAllTabs(quitAfterComplete, patchToExclude, afterComplete);
    };

    if (canvas) {
        MessageManager::callAsync([this, canvas, patch, deleteFunc]() mutable {
            if (patch->isDirty()) {
                Dialogs::showAskToSaveDialog(
                    &editor->openedDialog, editor, patch->getTitle(),
                    [canvas, deleteFunc](int const result) mutable {
                        if (!canvas)
                            return;
                        if (result == 2)
                            canvas->save([deleteFunc]() mutable { deleteFunc(); });
                        else if (result == 1)
                            deleteFunc();
                    },
                    0, true);
            } else {
                deleteFunc();
            }
        });
    }
}

void TabComponent::setActiveSplit(Canvas* cnv)
{
    if (cnv != splits[activeSplitIndex]) {
        if (auto const otherCanvas = splits[activeSplitIndex])
            otherCanvas->deselectAll();

        activeSplitIndex = cnv == splits[1] ? 1 : 0;
        editor->nvgSurface.invalidateAll();

        editor->sidebar->updateSearch(true);
    }
}

Canvas* TabComponent::getCanvasAtScreenPosition(Point<int> screenPosition)
{
    auto const localPoint = getLocalPoint(nullptr, screenPosition);
    if (!getLocalBounds().contains(localPoint))
        return nullptr;

    auto const split = localPoint.x < splitSize || !splits[1];
    return split ? splits[0] : splits[1];
}

TabComponent::VisibleCanvasArray TabComponent::getVisibleCanvases()
{
    VisibleCanvasArray result;
    if (auto* split = splits[0].get())
        result.add(reinterpret_cast<Canvas*>(split));
    if (auto* split = splits[1].get())
        result.add(reinterpret_cast<Canvas*>(split));
    return result;
}

bool TabComponent::isInterestedInDragSource(SourceDetails const& dragSourceDetails)
{
    return dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get()) || dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get());
}

void TabComponent::itemDropped(SourceDetails const& dragSourceDetails)
{
    if (dynamic_cast<ObjectDragAndDrop*>(dragSourceDetails.sourceComponent.get())) {
        auto const screenPosition = localPointToGlobal(dragSourceDetails.localPosition);

        auto* cnv = getCanvasAtScreenPosition(screenPosition);
        if (!cnv)
            return;

        auto const mousePos = cnv->getLocalPoint(this, dragSourceDetails.localPosition) - cnv->canvasOrigin;

        // Extract the VarArray from the var
        auto const patchWithSize = *dragSourceDetails.description.getArray();
        auto const patchSize = Point<int>(patchWithSize[0], patchWithSize[1]);
        auto const patchData = patchWithSize[2].toString();
        auto const patchName = patchWithSize[3].toString();

        cnv->dragAndDropPaste(patchData, mousePos, patchSize.x, patchSize.y, patchName);
        setActiveSplit(cnv);
        return;
    }

    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        if (getLocalBounds().removeFromRight(getWidth() - splitSize).contains(dragSourceDetails.localPosition) && (dragSourceDetails.localPosition.y > 30 || splits[1])) // Dragging to right split
        {
            moveToRightSplit(tab);
        } else if (dragSourceDetails.localPosition.y > 30) {
            moveToLeftSplit(tab);
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
    if (auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get())) {
        tab->setVisible(false);
    }
    splitDropBounds = Rectangle<int>();
    draggingOverTabbar = false;
    editor->nvgSurface.invalidateAll();
}

void TabComponent::saveTabPositions()
{
    auto editors = pd->getEditors();

    SmallArray<std::pair<pd::Patch::Ptr, int>> sortedPatches;
    for (auto* editor : pd->getEditors()) {
        auto& tabbar = editor->getTabComponent();
        for (int i = 0; i < tabbar.tabbars.size(); i++) {
            for (int j = 0; j < tabbar.tabbars[i].size(); j++) {
                if (auto* cnv = tabbar.tabbars[i][j]->cnv.getComponent()) {
                    cnv->patch.splitViewIndex = i;
                    cnv->patch.windowIndex = editor->editorIndex;
                    sortedPatches.add({ cnv->refCountedPatch, j });
                }
            }
        }
    }

    sortedPatches.sort([](auto const& a, auto const& b) {
        if (a.first->windowIndex != b.first->windowIndex)
            return a.first->windowIndex < b.first->windowIndex;

        if (a.first->splitViewIndex != b.first->splitViewIndex)
            return a.first->splitViewIndex < b.first->splitViewIndex;

        return a.second < b.second;
    });

    int i = 0;
    for (auto& [patch, tabIdx] : sortedPatches) {

        if (i >= pd->patches.size())
            break;

        pd->patches[i] = patch;
        i++;
    }
}

void TabComponent::itemDragMove(SourceDetails const& dragSourceDetails)
{
    auto* tab = dynamic_cast<TabBarButtonComponent*>(dragSourceDetails.sourceComponent.get());
    if (!tab)
        return;

    Rectangle<int> const oldSplitDropBounds = splitDropBounds;

    if (!splits[1]) {
        splitProportion = 2;
        splitSize = getWidth() / 2;
    }

    if (getLocalBounds().removeFromTop(30).contains(dragSourceDetails.localPosition)) // Dragging over tabbar
    {
        draggingOverTabbar = true;
        splitDropBounds = Rectangle<int>();
        tab->setVisible(true);

        auto const centreX = tab->getBounds().getCentreX();
        auto const tabBarWidth = splits[1] ? getWidth() / 2 : getWidth();
        int const hoveredSplit = splits[1] && centreX > splitSize;

        // Calculate target tab index based on tabbar width and tab centre position
        auto const targetTabPos = tabBarWidth / std::max(1, tabbars[hoveredSplit].size());
        auto const tabPos = (centreX - (hoveredSplit ? tabBarWidth : 0)) / targetTabPos;

        auto const oldPos = tabbars[hoveredSplit].indexOf(tab);
        if (oldPos != tabPos) {
            if (oldPos != -1) { // Dragging inside same tabbar
                tabbars[hoveredSplit].move(oldPos, tabPos);
                resized();
            } else if (splits[1]) { // Dragging to another tabbar
                tabbars[hoveredSplit].insert(tabPos, tabbars[!hoveredSplit].removeAndReturn(tabbars[!hoveredSplit].indexOf(tab)));
                resized();
            }
        }
    } else if (getLocalBounds().removeFromRight(getWidth() - splitSize).contains(dragSourceDetails.localPosition)) // Dragging over right split
    {
        draggingOverTabbar = false;
        splitDropBounds = getLocalBounds().removeFromRight(getWidth() - splitSize);
        tab->setVisible(false);
    } else { // Dragging over left split
        draggingOverTabbar = false;
        splitDropBounds = getLocalBounds().removeFromLeft(splitSize);
        tab->setVisible(false);
    }

    if (splitDropBounds != oldSplitDropBounds) {
        editor->nvgSurface.invalidateAll(); // Repaint for updated split drop bounds
    }
}

void TabComponent::showHiddenTabsMenu(int const splitIndex)
{

    class HiddenTabMenuItem : public PopupMenu::CustomComponent {
        String tabTitle;
        SafePointer<Canvas> cnv;

    public:
        TabComponent& tabbar;

        HiddenTabMenuItem(SafePointer<Canvas> canvas, String const& text, TabComponent& tabs)
            : tabTitle(text)
            , cnv(canvas)
            , tabbar(tabs)
        {
            closeTabButton.setButtonText(Icons::Clear);
            closeTabButton.addMouseListener(this, false);
            closeTabButton.onClick = [this]() mutable {
                tabbar.askToCloseTab(cnv.getComponent());
            };
            addChildComponent(closeTabButton);
        }

        void resized() override
        {
            closeTabButton.setBounds(getWidth() - 26, -2, 26, 26);
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 150;
            idealHeight = 24;
        }

        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;

            if (e.originalComponent == &closeTabButton)
                return;

            tabbar.showTab(cnv, cnv->patch.splitViewIndex);
            triggerMenuItem();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            closeTabButton.setVisible(true);
        }

        void mouseExit(MouseEvent const& e) override
        {
            closeTabButton.setVisible(false);
        }

        void paint(Graphics& g) override
        {

            if (tabbar.getVisibleCanvases().contains(cnv)) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            } else if (isItemHighlighted()) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId).interpolatedWith(findColour(PlugDataColour::popupMenuBackgroundColourId), 0.4f));
            } else {
                g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
            }

            g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::defaultCornerRadius);

            auto const area = getLocalBounds().reduced(4, 1).toFloat();

            auto const font = Font(14);

            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
            g.setFont(font);
            g.drawText(tabTitle.trim(), area.reduced(4, 0), Justification::centred, false);
        }

        SmallIconButton closeTabButton;
    };

    PopupMenu m;
    auto const& tabbar = tabbars[splitIndex];
    for (int i = 0; i < tabbar.size(); ++i) {
        auto* tab = tabbar[i];
        if (!tab->isVisible()) {
            auto title = tab->cnv->patch.getTitle();
            m.addCustomItem(i + 1, std::make_unique<HiddenTabMenuItem>(tab->cnv, title, *this), nullptr, title);
        }
    }

    m.showMenuAsync(PopupMenu::Options()
            .withDeletionCheck(*this)
            .withTargetComponent(&tabOverflowButtons[splitIndex]));
}
