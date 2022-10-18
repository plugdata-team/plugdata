/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PluginEditor.h"

#include "LookAndFeel.h"

#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/Dialogs.h"


bool wantsNativeDialog() {
#if PLUGDATA_STANDALONE
    return true;
#endif
    
    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
    File settingsFile = homeDir.getChildFile("Settings.xml");
    
    
    auto settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());
    
    if(!settingsTree.hasProperty("NativeDialog")) {
        return true;
    }
    
    return static_cast<bool>(settingsTree.getProperty("NativeDialog"));
}





PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p) : AudioProcessorEditor(&p), pd(p), statusbar(p), sidebar(&p)
{
    toolbarButtons = {new TextButton(Icons::Open), new TextButton(Icons::Save),     new TextButton(Icons::SaveAs), new TextButton(Icons::Undo),
                      new TextButton(Icons::Redo), new TextButton(Icons::Add),  new TextButton(Icons::Settings), new TextButton(Icons::Hide),   new TextButton(Icons::Pin)};
    
    
#if PLUGDATA_ROUNDED
    setResizable(true, false);
#else
    setResizable(true, true);
#endif

    tooltipWindow->setOpaque(false);
    tooltipWindow->setLookAndFeel(&pd.lnf.get());

    addKeyListener(getKeyMappings());

    pd.locked.addListener(this);
    pd.settingsTree.addListener(this);

    setWantsKeyboardFocus(true);
    registerAllCommandsForTarget(this);

    for (auto& seperator : seperators)
    {
        addChildComponent(&seperator);
    }

    auto keymap = p.settingsTree.getChildWithName("Keymap");
    if (keymap.isValid())
    {
        auto xmlStr = keymap.getProperty("keyxml").toString();
        auto elt = XmlDocument(xmlStr).getDocumentElement();

        if (elt)
        {
            getKeyMappings()->restoreFromXml(*elt);
        }
    }
    else
    {
        p.settingsTree.appendChild(ValueTree("Keymap"), nullptr);
    }

    theme.referTo(pd.settingsTree.getPropertyAsValue("Theme", nullptr));
    theme.addListener(this);
    
    zoomScale.referTo(pd.settingsTree.getPropertyAsValue("Zoom", nullptr));
    zoomScale.addListener(this);
    
    addAndMakeVisible(statusbar);

    tabbar.newTab = [this]()
    {
        newProject();
    };
    
    
    tabbar.openProject = [this]()
    {
        openProject();
    };
    
    tabbar.onTabChange = [this](int idx)
    {
        if (idx == -1) return;

        // update GraphOnParent when changing tabs
        for (auto* object : getCurrentCanvas()->objects)
        {
            if (!object->gui) continue;
            if (auto* cnv = object->gui->getCanvas()) cnv->synchronise();
        }

        auto* cnv = getCurrentCanvas();
        if (cnv->patch.getPointer())
        {
            cnv->patch.setCurrent();
        }

        cnv->synchronise();
        cnv->updateGuiValues();
        cnv->updateDrawables();
        cnv->updateGuiParameters();
        
        updateCommandStatus();
        
        pd.lastTab = idx;
    };

    tabbar.setOutline(0);
    addAndMakeVisible(tabbar);
    addAndMakeVisible(sidebar);

    for (auto* button : toolbarButtons)
    {
        button->setName("toolbar:button");
        button->setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // Open button
    toolbarButton(Open)->setTooltip("Open Project");
    toolbarButton(Open)->onClick = [this]() { openProject(); };

    // Save button
    toolbarButton(Save)->setTooltip("Save Project");
    toolbarButton(Save)->onClick = [this]() { saveProject(); };

    // Save Ad button
    toolbarButton(SaveAs)->setTooltip("Save Project as");
    toolbarButton(SaveAs)->onClick = [this]() { saveProjectAs(); };

    //  Undo button
    toolbarButton(Undo)->setTooltip("Undo");
    toolbarButton(Undo)->onClick = [this]() { getCurrentCanvas()->undo(); };

    // Redo button
    toolbarButton(Redo)->setTooltip("Redo");
    toolbarButton(Redo)->onClick = [this]() { getCurrentCanvas()->redo(); };

    // New object button
    toolbarButton(Add)->setTooltip("Create Object");
    toolbarButton(Add)->onClick = [this]() { Dialogs::showObjectMenu(this, toolbarButton(Add)); };

    // Show settings
    toolbarButton(Settings)->setTooltip("Settings");
    toolbarButton(Settings)->onClick = [this]()
    {
#ifdef PLUGDATA_STANDALONE
            // Initialise settings dialog for DAW and standalone
            auto* pluginHolder = StandalonePluginHolder::getInstance();
            Dialogs::createSettingsDialog(pd, &pluginHolder->deviceManager, toolbarButton(Settings), pd.settingsTree);
#else
            Dialogs::createSettingsDialog(pd, nullptr, toolbarButton(Settings), pd.settingsTree);
#endif
    };

    // Hide sidebar
    toolbarButton(Hide)->setTooltip("Hide Sidebar");
    toolbarButton(Hide)->setName("toolbar:hide");
    toolbarButton(Hide)->setClickingTogglesState(true);
    toolbarButton(Hide)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Hide)->setConnectedEdges(12);
    toolbarButton(Hide)->onClick = [this]()
    {
        bool show = !toolbarButton(Hide)->getToggleState();
        sidebar.showSidebar(show);
        toolbarButton(Hide)->setButtonText(show ? Icons::Hide : Icons::Show);

        toolbarButton(Pin)->setVisible(show);

        repaint();
        resized();
    };

    // Hide pin sidebar panel
    toolbarButton(Pin)->setTooltip("Pin Panel");
    toolbarButton(Pin)->setName("toolbar:pin");
    toolbarButton(Pin)->setClickingTogglesState(true);
    toolbarButton(Pin)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Pin)->setConnectedEdges(12);
    toolbarButton(Pin)->onClick = [this]() { sidebar.pinSidebar(toolbarButton(Pin)->getToggleState()); };

    addAndMakeVisible(toolbarButton(Hide));
    
    sidebar.setSize(250, pd.lastUIHeight - 40);
    setSize(pd.lastUIWidth, pd.lastUIHeight);
    
    // Set minimum bounds
    setResizeLimits(835, 305, 999999, 999999);
    
    tabbar.toFront(false);
    sidebar.toFront(false);
    
    // Make sure existing console messages are processed
    sidebar.updateConsole();
    updateCommandStatus();
    
    addChildComponent(zoomLabel);
    
    // Initialise zoom factor
    valueChanged(zoomScale);
}
PlugDataPluginEditor::~PlugDataPluginEditor()
{
    auto keymap = pd.settingsTree.getChildWithName("Keymap");
    if (keymap.isValid())
    {
        keymap.setProperty("keyxml", getKeyMappings()->createXml(true)->toString(), nullptr);
    }

    setConstrainer(nullptr);

    pd.settingsTree.removeListener(this);
    pd.locked.removeListener(this);
    zoomScale.removeListener(this);
    theme.removeListener(this);
}

void PlugDataPluginEditor::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    
    auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);

#if PLUGDATA_ROUNDED
    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 10, getWidth(), toolbarHeight - 9);
    g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), toolbarHeight, 6.0f);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight() - 10);
    g.fillRoundedRectangle(0.0f, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight(), 6.0f);

#else
    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight());
#endif
    
    int roundedOffset = PLUGDATA_ROUNDED;

    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, toolbarHeight + roundedOffset, static_cast<float>(getWidth()), toolbarHeight + roundedOffset, 1.0f);
}

void PlugDataPluginEditor::resized()
{
    int roundedOffset = PLUGDATA_ROUNDED;
    
    sidebar.setBounds(getWidth() - sidebar.getWidth(), toolbarHeight + roundedOffset, sidebar.getWidth(), getHeight() - toolbarHeight - roundedOffset);
    
    tabbar.setBounds(0, toolbarHeight + roundedOffset, (getWidth() - sidebar.getWidth()) + 1, getHeight() - toolbarHeight - (statusbar.getHeight() + roundedOffset));


    statusbar.setBounds(0, getHeight() - statusbar.getHeight(), getWidth() - sidebar.getWidth(), statusbar.getHeight());

    auto fb = FlexBox(FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::flexStart, FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexStart);

    for (int b = 0; b < 9; b++)
    {
        auto* button = toolbarButtons[b];

        auto item = FlexItem(*button).withMinWidth(80.0f).withMinHeight(8.0f).withMaxWidth(120.0f);
        item.flexGrow = 1.0f;
        item.flexShrink = 1.0f;

        if (b == 3 || b == 5)
        {
            auto separator = FlexItem(seperators[b == 4]).withMinWidth(1.0f).withMaxWidth(12.0f);
            separator.flexGrow = 1.0f;
            separator.flexShrink = 1.0f;
            fb.items.add(separator);
        }

        fb.items.add(item);
    }

    Rectangle<float> toolbarBounds = {5.0f, 0.0f, getWidth() - sidebar.getWidth() + 60.0f, static_cast<float>(toolbarHeight)};
    if (toolbarButton(Hide)->getToggleState()) toolbarBounds.setWidth(getWidth() - 50.0f);

    fb.performLayout(toolbarBounds);

    // hide when they fall off the screen
    for (int b = 0; b < 8; b++)
    {
        toolbarButtons[b]->setVisible((toolbarButtons[b]->getBounds().getCentreX()) < getWidth() - sidebar.getWidth());
    }
#ifdef PLUGDATA_STANDALONE
    int offset = 150;
#else
    int offset = 80;
#endif

    int pinPosition = getWidth() - std::max(sidebar.getWidth() - 40, offset);
    int hidePosition = toolbarButton(Hide)->getToggleState() ? pinPosition : pinPosition - 70;

    toolbarButton(Hide)->setBounds(hidePosition, 0, 70, toolbarHeight);
    toolbarButton(Pin)->setBounds(pinPosition, 0, 70, toolbarHeight);

    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();
    
    zoomLabel.setTopLeftPosition(5, statusbar.getY() - 28);
    zoomLabel.setSize(50, 23);
    
    if (auto* cnv = getCurrentCanvas())
    {
        cnv->checkBounds();
    }
    
    repaint();

}

void PlugDataPluginEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
    }
}

void PlugDataPluginEditor::mouseMagnify(const MouseEvent& e, float scrollFactor)
{
    auto* cnv = getCurrentCanvas();
    
    if(!cnv) return;
    
    auto* viewport = getCurrentCanvas()->viewport;

    auto event = e.getEventRelativeTo(viewport);

    auto oldMousePos = cnv->getLocalPoint(this, e.getPosition());

    float value = static_cast<float>(zoomScale.getValue());
    
    // Apply and limit zoom
    value = std::clamp(value * scrollFactor, 0.5f, 2.0f);

    zoomScale = value;

    auto newMousePos = cnv->getLocalPoint(this, e.getPosition());

    viewport->setViewPosition(viewport->getViewPositionX() + (oldMousePos.x - newMousePos.x), viewport->getViewPositionY() + (oldMousePos.y - newMousePos.y));
}

#ifdef PLUGDATA_STANDALONE
void PlugDataPluginEditor::mouseDown(const MouseEvent& e)
{
    if (e.getPosition().getY() < toolbarHeight)
    {
        auto* window = getTopLevelComponent();
        windowDragger.startDraggingComponent(window, e.getEventRelativeTo(window));
    }
}

void PlugDataPluginEditor::mouseDrag(const MouseEvent& e)
{
    auto* window = getTopLevelComponent();
    windowDragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
}
#endif

void PlugDataPluginEditor::newProject() {
    auto* patch = pd.loadPatch(pd::Instance::defaultPatch);
    patch->setTitle("Untitled Patcher");
}

void PlugDataPluginEditor::openProject()
{
    auto openFunc = [this](const FileChooser& f)
    {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd"))
        {
            pd.settingsTree.setProperty("LastChooserPath", openedFile.getParentDirectory().getFullPathName(), nullptr);

            pd.loadPatch(openedFile);
        }
    };

    openChooser = std::make_unique<FileChooser>("Choose file to open", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd", wantsNativeDialog());

    openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
}

void PlugDataPluginEditor::saveProjectAs(const std::function<void()>& nestedCallback)
{
    saveChooser = std::make_unique<FileChooser>("Select a save file", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd", wantsNativeDialog());

    saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
                             [this, nestedCallback](const FileChooser& f) mutable
                             {
                                 File result = saveChooser->getResult();

                                 if (result.getFullPathName().isNotEmpty())
                                 {
                                     pd.settingsTree.setProperty("LastChooserPath", result.getParentDirectory().getFullPathName(), nullptr);

                                     result.deleteFile();
                                     result = result.withFileExtension(".pd");

                                     getCurrentCanvas()->patch.savePatch(result);
                                 }

                                 nestedCallback();
                             });
}

void PlugDataPluginEditor::saveProject(const std::function<void()>& nestedCallback)
{
    for (auto* patch : pd.patches)
    {
        patch->deselectAll();
    }

    if (getCurrentCanvas()->patch.getCurrentFile().existsAsFile())
    {
        getCurrentCanvas()->patch.savePatch();
        nestedCallback();
    }
    else
    {
        saveProjectAs(nestedCallback);
    }
}

Canvas* PlugDataPluginEditor::getCurrentCanvas()
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent()))
    {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()))
        {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PlugDataPluginEditor::getCanvas(int idx)
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx)))
    {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()))
        {
            return cnv;
        }
    }

    return nullptr;
}

void PlugDataPluginEditor::addTab(Canvas* cnv, bool deleteWhenClosed)
{
    tabbar.addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);
    

    int tabIdx = tabbar.getNumTabs() - 1;

    tabbar.setCurrentTabIndex(tabIdx);
    tabbar.setTabBackgroundColour(tabIdx, Colours::transparentBlack);

    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);

    auto* closeButton = new TextButton(Icons::Clear);

    closeButton->onClick = [this, tabButton, deleteWhenClosed]() mutable
    {
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for (int i = 0; i < tabbar.getNumTabs(); i++)
        {
            if (tabbar.getTabbedButtonBar().getTabButton(i) == tabButton)
            {
                idx = i;
                break;
            }
        }

        if (idx == -1) return;

        auto deleteFunc = [this, deleteWhenClosed, idx]() mutable
        {
            auto* cnv = getCanvas(idx);

            if (!cnv)
            {
                tabbar.removeTab(idx);
                return;
            }

            auto* patch = &cnv->patch;

            if (deleteWhenClosed)
            {
                patch->close();
            }

            canvases.removeObject(cnv);
            tabbar.removeTab(idx);
            pd.patches.removeObject(patch);

            tabbar.setCurrentTabIndex(tabbar.getNumTabs() - 1, true);
            updateCommandStatus();
        };

        MessageManager::callAsync(
            [this, deleteFunc, idx]() mutable
            {
                auto cnv = SafePointer(getCanvas(idx));
                if (cnv && cnv->patch.isDirty())
                {
                    Dialogs::showSaveDialog(&openedDialog, this, cnv->patch.getTitle(),
                                            [this, deleteFunc, cnv](int result) mutable
                                            {
                                                if(!cnv) return;
                                                if (result == 2)
                                                {
                                                    saveProject([deleteFunc]() mutable { deleteFunc(); });
                                                }
                                                else if (result == 1)
                                                {
                                                    deleteFunc();
                                                }
                                            });
                }
                else if(cnv)
                {
                    deleteFunc();
                }
            });
    };

    closeButton->setName("tab:close");
    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);
    closeButton->setSize(28, 28);

    tabbar.repaint();

    cnv->setVisible(true);
    
    updateCommandStatus();
}

void PlugDataPluginEditor::valueChanged(Value& v)
{
    // Update undo state when locking/unlocking
    if (v.refersToSameSourceAs(pd.locked))
    {
        updateCommandStatus();
    }
    // Update zoom
    else if (v.refersToSameSourceAs(zoomScale))
    {
        float scale = static_cast<float>(v.getValue());
        
        if(scale == 0)  {
            zoomScale = 1.0f;
        }
        
        transform = AffineTransform().scaled(scale);
        for (auto& canvas : canvases)
        {
            if (!canvas->isGraph)
            {
                canvas->hideSuggestions();
                canvas->setTransform(transform);
            }
        }
        if(auto* cnv = getCurrentCanvas()) {
            cnv->checkBounds();
        }
        
        zoomLabel.setZoomLevel(scale);
    }
    // Update theme
    else if (v.refersToSameSourceAs(theme))
    {
        pd.setTheme(static_cast<bool>(theme.getValue()));
    }
}

void PlugDataPluginEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    pd.settingsChangedInternally = true;
    startTimer(300);
}
void PlugDataPluginEditor::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
    pd.settingsChangedInternally = true;
    startTimer(300);
}
void PlugDataPluginEditor::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    pd.settingsChangedInternally = true;
    startTimer(300);
}

void PlugDataPluginEditor::timerCallback()
{
    // Save settings to file whenever valuetree state changes
    // Use timer to group changes together
    pd.saveSettings();
    stopTimer();
}

void PlugDataPluginEditor::updateCommandStatus()
{
    if (auto* cnv = getCurrentCanvas())
    {
        // Update connection style button
        bool allSegmented = true;
        bool allNotSegmented = true;
        bool hasSelection = false;
        
        
        bool isDragging = cnv->didStartDragging && !cnv->isDraggingLasso && statusbar.locked == var(false);
        for (auto& connection : cnv->getSelectionOfType<Connection>())
        {
            allSegmented = allSegmented && connection->isSegmented();
            allNotSegmented = allNotSegmented && !connection->isSegmented();
            hasSelection = true;
        }

        statusbar.connectionStyleButton->setEnabled(!isDragging && hasSelection && (allSegmented || allNotSegmented));
        statusbar.connectionPathfind->setEnabled(!isDragging && hasSelection && allSegmented);
        statusbar.connectionStyleButton->setToggleState(!isDragging && hasSelection && allSegmented, dontSendNotification);
        
        statusbar.lockButton->setEnabled(!isDragging);
        statusbar.zoomIn->setEnabled(!isDragging);
        statusbar.zoomOut->setEnabled(!isDragging);

        auto* patchPtr = cnv->patch.getPointer();
        if (!patchPtr) return;
        
        auto deletionCheck = SafePointer(this);

        // First on pd's thread, get undo status
        pd.enqueueFunction(
            [this, patchPtr, isDragging, deletionCheck]() mutable
            {
                if(!deletionCheck) return;
                
                canUndo = libpd_can_undo(patchPtr) && !isDragging && pd.locked == var(false);
                canRedo = libpd_can_redo(patchPtr) && !isDragging && pd.locked == var(false);

                // Set button enablement on message thread
                MessageManager::callAsync(
                    [this, deletionCheck]() mutable
                    {
                        if(!deletionCheck) return;
                        
                        toolbarButton(Undo)->setEnabled(canUndo);
                        toolbarButton(Redo)->setEnabled(canRedo);

                        // Application commands need to be updated when undo state changes
                        commandStatusChanged();
                    });
            });
        
        statusbar.lockButton->setEnabled(true);
        statusbar.presentationButton->setEnabled(true);
        statusbar.gridButton->setEnabled(true);
        
        toolbarButton(Save)->setEnabled(true);
        toolbarButton(SaveAs)->setEnabled(true);
        toolbarButton(Add)->setEnabled(pd.locked == var(false));
    }
    else {
        statusbar.connectionStyleButton->setEnabled(false);
        statusbar.connectionPathfind->setEnabled(false);
        statusbar.lockButton->setEnabled(false);
        statusbar.zoomIn->setEnabled(false);
        statusbar.zoomOut->setEnabled(false);
        
        statusbar.lockButton->setEnabled(false);
        statusbar.presentationButton->setEnabled(false);
        statusbar.gridButton->setEnabled(false);
        
        toolbarButton(Save)->setEnabled(false);
        toolbarButton(SaveAs)->setEnabled(false);
        toolbarButton(Undo)->setEnabled(false);
        toolbarButton(Redo)->setEnabled(false);
        toolbarButton(Add)->setEnabled(false);
        
        
    }
}

ApplicationCommandTarget* PlugDataPluginEditor::getNextCommandTarget()
{
    return this;
}

void PlugDataPluginEditor::getAllCommands(Array<CommandID>& commands)
{
    // Add all command IDs
    for (int n = NewProject; n < NumItems; n++)
    {
        commands.add(n);
    }
}

void PlugDataPluginEditor::getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result)
{
    bool hasBoxSelection = false;
    bool hasSelection = false;
    bool isDragging = false;
    bool hasCanvas = true;
    
    if (auto* cnv = getCurrentCanvas())
    {
        auto selectedBoxes = cnv->getSelectionOfType<Object>();
        auto selectedConnections = cnv->getSelectionOfType<Connection>();

        hasBoxSelection = !selectedBoxes.isEmpty();
        hasSelection = hasBoxSelection || !selectedConnections.isEmpty();
        isDragging = cnv->didStartDragging && !cnv->isDraggingLasso && statusbar.locked == var(false);
    }
    else {
        hasCanvas = false;
    }

    switch (commandID)
    {
        case CommandIDs::NewProject:
        {
            result.setInfo("New Project", "Create a new project", "General", 0);
            result.addDefaultKeypress(84, ModifierKeys::commandModifier);
            break;
        }
        case CommandIDs::OpenProject:
        {
            result.setInfo("Open Project", "Open a new project", "General", 0);
            break;
        }
        case CommandIDs::SaveProject:
        {
            result.setInfo("Save Project", "Save project at current location", "General", 0);
            result.addDefaultKeypress(83, ModifierKeys::commandModifier);
            result.setActive(hasCanvas);
            break;
        }
        case CommandIDs::SaveProjectAs:
        {
            result.setInfo("Save Project As", "Save project in chosen location", "General", 0);
            result.addDefaultKeypress(83, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas);
            break;
        }
        case CommandIDs::CloseTab:
        {
            result.setInfo("Close tab", "Close currently opened tab", "General", 0);
            result.addDefaultKeypress(87, ModifierKeys::commandModifier);
            result.setActive(hasCanvas);
            break;
        }
        case CommandIDs::Undo:
        {
            result.setInfo("Undo", "Undo action", "General", 0);
            result.addDefaultKeypress(90, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && canUndo);

            break;
        }

        case CommandIDs::Redo:
        {
            result.setInfo("Redo", "Redo action", "General", 0);
            result.addDefaultKeypress(90, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && canRedo);
            break;
        }
        case CommandIDs::Lock:
        {
            result.setInfo("Lock", "Lock patch", "Edit", 0);
            result.addDefaultKeypress(69, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && !static_cast<bool>(statusbar.presentationMode.getValue()));
            break;
        }
        case CommandIDs::ConnectionPathfind:
        {
            result.setInfo("Tidy connection", "Find best path for connection", "Edit", 0);
            result.addDefaultKeypress(89, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging);
            break;
        }
        case CommandIDs::ConnectionStyle:
        {
            result.setInfo("Connection style", "Set connection style", "Edit", 0);
            result.setActive(hasCanvas && !isDragging && statusbar.connectionStyleButton->isEnabled());
            break;
        }
        case CommandIDs::ZoomIn:
        {
            result.setInfo("Zoom in", "Zoom in", "Edit", 0);
            result.addDefaultKeypress(61, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging);
            break;
        }
        case CommandIDs::ZoomOut:
        {
            result.setInfo("Zoom out", "Zoom out", "Edit", 0);
            result.addDefaultKeypress(45, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging);
            break;
        }
        case CommandIDs::ZoomNormal:
        {
            result.setInfo("Zoom 100%", "Revert zoom to 100%", "Edit", 0);
            result.addDefaultKeypress(33, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging);
            break;
        }
        case CommandIDs::Copy:
        {
            result.setInfo("Copy", "Copy", "Edit", 0);
            result.addDefaultKeypress(67, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && pd.locked == var(false) && hasBoxSelection && !isDragging);
            break;
        }
        case CommandIDs::Cut:
        {
            result.setInfo("Cut", "Cut selection", "Edit", 0);
            result.addDefaultKeypress(88, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && pd.locked == var(false) && hasSelection && !isDragging);
            break;
        }
        case CommandIDs::Paste:
        {
            result.setInfo("Paste", "Paste", "Edit", 0);
            result.addDefaultKeypress(86, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && pd.locked == var(false) && !isDragging);
            break;
        }
        case CommandIDs::Delete:
        {
            result.setInfo("Delete", "Delete selection", "Edit", 0);
            result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys::noModifiers);
            result.addDefaultKeypress(KeyPress::deleteKey, ModifierKeys::noModifiers);

            result.setActive(hasCanvas && !isDragging && pd.locked == var(false) && hasSelection);
            break;
        }
        case CommandIDs::Encapsulate:
        {
            result.setInfo("Encapsulate", "Encapsulate objects", "Edit", 0);
            result.addDefaultKeypress(69, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false) && hasSelection);
            break;
        }
        case CommandIDs::Duplicate:
        {
            
            result.setInfo("Duplicate", "Duplicate selection", "Edit", 0);
            result.addDefaultKeypress(68, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false) && hasBoxSelection);
            break;
        }
        case CommandIDs::SelectAll:
        {
            result.setInfo("Select all", "Select all objects and connections", "Edit", 0);
            result.addDefaultKeypress(65, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::ShowBrowser:
        {
            result.setInfo("Show Browser", "Open documentation browser panel", "Edit", 0);
            result.addDefaultKeypress(66, ModifierKeys::commandModifier);
            result.setActive(true);
            break;
        }

        case CommandIDs::NewObject:
        {
            result.setInfo("New Object", "Create new object", "Objects", 0);
            result.addDefaultKeypress(49, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewComment:
        {
            result.setInfo("New Comment", "Create new comment", "Objects", 0);
            result.addDefaultKeypress(53, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewBang:
        {
            result.setInfo("New Bang", "Create new bang", "Objects", 0);
            result.addDefaultKeypress(66, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewMessage:
        {
            result.setInfo("New Message", "Create new message", "Objects", 0);
            result.addDefaultKeypress(50, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewToggle:
        {
            result.setInfo("New Toggle", "Create new toggle", "Objects", 0);
            result.addDefaultKeypress(84, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewNumbox:
        {
            result.setInfo("New Number", "Create new number object", "Objects", 0);
            result.addDefaultKeypress(51, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            
            break;
        }
        case CommandIDs::NewNumboxTilde:
        {
            result.setInfo("New Numbox~", "Create new numbox~ object", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            
            break;
        }
        case CommandIDs::NewOscilloscope:
        {
            result.setInfo("New Oscilloscope", "Create new oscilloscope object", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            
            break;
        }
        case CommandIDs::NewFunction:
        {
            result.setInfo("New Function", "Create new function object", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            
            break;
        }
        case CommandIDs::NewFloatAtom:
        {
            result.setInfo("New Floatatom", "Create new floatatom", "Objects", 0);
            result.addDefaultKeypress(70, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewSymbolAtom:
        {
            result.setInfo("New Symbolatom", "Create new symbolatom", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewListAtom:
        {
            result.setInfo("New Listatom", "Create new listatom", "Objects", 0);
            result.addDefaultKeypress(52, ModifierKeys::commandModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVerticalSlider:
        {
            result.setInfo("New Vertical Slider", "Create new vertical slider", "Objects", 0);
            result.addDefaultKeypress(86, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewHorizontalSlider:
        {
            result.setInfo("New Horizontal Slider", "Create new horizontal slider", "Objects", 0);
            result.addDefaultKeypress(74, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVerticalRadio:
        {
            result.setInfo("New Vertical Radio", "Create new vertical radio", "Objects", 0);
            result.addDefaultKeypress(68, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewHorizontalRadio:
        {
            result.setInfo("New Horizontal Radio", "Create new horizontal radio", "Objects", 0);
            result.addDefaultKeypress(73, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewArray:
        {
            result.setInfo("New Array", "Create new array", "Objects", 0);
            result.addDefaultKeypress(65, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewGraphOnParent:
        {
            result.setInfo("New GraphOnParent", "Create new graph on parent", "Objects", 0);
            result.addDefaultKeypress(71, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewCanvas:
        {
            result.setInfo("New Canvas", "Create new canvas object", "Objects", 0);
            result.addDefaultKeypress(67, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewKeyboard:
        {
            result.setInfo("New Keyboard", "Create new keyboard", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVUMeterObject:
        {
            result.setInfo("New VU Meter", "Create new VU meter", "Objects", 0);
            result.addDefaultKeypress(85, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        case CommandIDs::NewButton:
        {
            result.setInfo("New Button", "Create new button", "Objects", 0);
            result.setActive(hasCanvas && !isDragging && pd.locked == var(false));
            break;
        }
        default:
            break;
    }
}

bool PlugDataPluginEditor::perform(const InvocationInfo& info)
{
    auto* cnv = getCurrentCanvas();

    auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->getMouseXYRelative() - Point<int>(Object::margin, Object::margin));

    switch (info.commandID)
    {
        case CommandIDs::NewProject:
        {
            newProject();
            return true;
        }
        case CommandIDs::OpenProject:
        {
            openProject();
            return true;
        }
        case CommandIDs::SaveProject:
        {
            saveProject();
            return true;
        }
        case CommandIDs::SaveProjectAs:
        {
            saveProjectAs();
            return true;
        }
        case CommandIDs::CloseTab:
        {
            if(tabbar.getNumTabs() == 0) return true;
            
            int currentIdx = tabbar.getCurrentTabIndex();
            auto* closeButton = dynamic_cast<TextButton*>(tabbar.getTabbedButtonBar().getTabButton(currentIdx)->getExtraComponent());
            
            // Virtually click the close button
            closeButton->triggerClick();
            
            return true;
        }
        case CommandIDs::Copy:
        {
            cnv->copySelection();
            return true;
        }
        case CommandIDs::Paste:
        {
            cnv->pasteSelection();
            return true;
        }
        case CommandIDs::Cut:
        {
            cnv->copySelection();
            cnv->removeSelection();
            return true;
        }
        case CommandIDs::Delete:
        {
            cnv->removeSelection();
            return true;
        }
        case CommandIDs::Duplicate:
        {
            cnv->duplicateSelection();
            return true;
        }
        case CommandIDs::Encapsulate:
        {
            cnv->encapsulateSelection();
            return true;
        }
        case CommandIDs::SelectAll:
        {
            for (auto* object : cnv->objects)
            {
                cnv->setSelected(object, true);
            }
            for (auto* con : cnv->connections)
            {
                cnv->setSelected(con, true);
            }
            return true;
        }
        case CommandIDs::ShowBrowser:
        {
            sidebar.showPanel(sidebar.isShowingBrowser() ? 1 : 0);
            return true;
        }
        case CommandIDs::Lock:
        {
            statusbar.lockButton->triggerClick();
            return true;
        }
        case CommandIDs::ConnectionPathfind:
        {
            statusbar.connectionStyleButton->setToggleState(true, sendNotification);
            for (auto* con : cnv->connections)
            {
                if (cnv->isSelected(con))
                {
                    con->findPath();
                    con->updatePath();
                }
            }

            return true;
        }
        case CommandIDs::ZoomIn:
        {
            float newScale = static_cast<float>(zoomScale.getValue()) + 0.1f;
            zoomScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;
            
            return true;
        }
        case CommandIDs::ZoomOut:
        {
            float newScale = static_cast<float>(zoomScale.getValue()) - 0.1f;
            zoomScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;
            
            return true;
        }
        case CommandIDs::ZoomNormal:
        {
            zoomScale = 1.0f;
            return true;
        }
        case CommandIDs::Undo:
        {
            cnv->undo();
            return true;
        }
        case CommandIDs::Redo:
        {
            cnv->redo();
            return true;
        }
        case CommandIDs::NewArray:
        {
            
            Dialogs::showArrayDialog(&openedDialog, this,
                                     [this](int result, const String& name, const String& size)
                                     {
                                         if (result)
                                         {
                                             auto* cnv = getCurrentCanvas();
                                             auto* object = new Object(cnv, "graph " + name + " " + size, cnv->viewport->getViewArea().getCentre());
                                             cnv->objects.add(object);
                                         }
                                     });
            return true;
        }

        default:
        {
            const StringArray objectNames = {"", "comment", "bng", "msg", "tgl", "nbx", "vsl", "hsl", "vradio", "hradio", "floatatom", "symbolatom", "listbox", "array", "graph", "cnv", "keyboard", "vu", "button", "numbox~", "oscope~", "function"};

            jassert(objectNames.size() == CommandIDs::NumItems - CommandIDs::NewObject);

            int idx = static_cast<int>(info.commandID) - CommandIDs::NewObject;
            if (isPositiveAndBelow(idx, objectNames.size()))
            {
                cnv->objects.add(new Object(cnv, objectNames[idx], lastPosition));
                return true;
            }

            return false;
        }
    }
}
