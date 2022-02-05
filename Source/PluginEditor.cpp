/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"
#include "Dialogs.h"
#include "Edge.h"
#include "x_libpd_mod_utils.h"

//==============================================================================

PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p, Console* debugConsole)
    : AudioProcessorEditor(&p)
    , pd(p)
    , levelmeter(p.parameters, p.meterSource)
{
    addKeyListener(this);
    setWantsKeyboardFocus(true);
    
    LookAndFeel::setDefaultLookAndFeel(&mainLook);
    
    console = debugConsole;
    console->setLookAndFeel(&statusbarLook);
    levelmeter.setLookAndFeel(&statusbarLook);
    
    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedComponent::outlineColourId, MainLook::firstBackground);

    setLookAndFeel(&mainLook);

    addAndMakeVisible(levelmeter);

    tabbar.onTabChange = [this](int idx) {
        Edge::connectingEdge = nullptr;

        if (idx == -1)
            return;

        // update GraphOnParent when changing tabs
        for (auto* box : getCurrentCanvas()->boxes) {
            if (box->graphics && box->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
                auto* cnv = box->graphics.get()->getCanvas();
                if (cnv)
                    cnv->synchronise();
            }
            if (box->graphics && (box->graphics->getGUI().getType() == pd::Type::Subpatch || box->graphics->getGUI().getType() == pd::Type::GraphOnParent)) {
                box->updatePorts();
            }
        }

        auto* cnv = getCurrentCanvas();
        if (cnv->patch.getPointer()) {
            cnv->patch.setCurrent();
        }

        cnv->checkBounds();

        updateValues();
    };
    
    addAndMakeVisible(tabbar);
    addAndMakeVisible(console);
    addChildComponent(inspector);

    bypassButton.setTooltip("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setConnectedEdges(12);
    bypassButton.setLookAndFeel(&statusbarLook);
    addAndMakeVisible(bypassButton);
    bypassButton.onClick = [this]() {
        pd.setBypass(!bypassButton.getToggleState());
    };

    bypassButton.setToggleState(true, dontSendNotification);
    
    lockButton.setTooltip("Lock");
    lockButton.setClickingTogglesState(true);
    lockButton.setConnectedEdges(12);
    lockButton.setLookAndFeel(&statusbarLook);
    lockButton.onClick = [this]() {
        pd.locked = lockButton.getToggleState();
        
        for(auto& cnv : canvases) {
            cnv->deselectAll();
            for(auto& connection : cnv->connections) {
                if(connection->isSelected) {
                    connection->isSelected = false;
                    connection->repaint();
                }
            }
        }
        
        lockButton.setButtonText(pd.locked ? Icons::Lock : Icons::Unlock);

        sendChangeMessage();
    };
    addAndMakeVisible(lockButton);
    
    lockButton.setButtonText(pd.locked ? Icons::Lock : Icons::Unlock);
    lockButton.setToggleState(pd.locked, dontSendNotification);

    connectionStyleButton.setTooltip("Enable segmented connections");
    connectionStyleButton.setClickingTogglesState(true);
    connectionStyleButton.setConnectedEdges(12);
    connectionStyleButton.setLookAndFeel(&statusbarLook);
    connectionStyleButton.onClick = [this]() {
        pd.settingsTree.setProperty(Identifiers::connectionStyle, connectionStyleButton.getToggleState(), nullptr);
        
        connectionPathfind.setEnabled(connectionStyleButton.getToggleState());
        
        for(auto& cnv : canvases) {
            for (auto* connection : cnv->connections) {
                connection->resized();
                connection->repaint();
            }
        }
    };
    
    bool defaultConnectionStyle = (bool)pd.settingsTree.getProperty(Identifiers::connectionStyle);
    connectionStyleButton.setToggleState(defaultConnectionStyle, sendNotification);
    addAndMakeVisible(connectionStyleButton);
    
    connectionPathfind.setTooltip("Find best connection path");
    connectionPathfind.setConnectedEdges(12);
    connectionPathfind.setLookAndFeel(&statusbarLook);
    connectionPathfind.onClick = [this]() {
        for(auto& c : getCurrentCanvas()->connections) {
            if(c->isSelected) {
                c->applyPath(c->findPath());
            }
        }
    };
    connectionPathfind.setEnabled(defaultConnectionStyle);
    addAndMakeVisible(connectionPathfind);

    addAndMakeVisible(zoomLabel);
    zoomLabel.setText("100%", dontSendNotification);
    zoomLabel.setFont(Font(11));
    
    zoomIn.setTooltip("Zoom In");
    zoomIn.setConnectedEdges(12);
    zoomIn.setLookAndFeel(&statusbarLook);
    zoomIn.onClick = [this]() {
        zoom(true);
    };
    addAndMakeVisible(zoomIn);
    
    
    zoomOut.setTooltip("Zoom Out");
    zoomOut.setConnectedEdges(12);
    zoomOut.setLookAndFeel(&statusbarLook);
    zoomOut.onClick = [this]() {
        zoom(false);
    };
    addAndMakeVisible(zoomOut);
    
    for (auto& button : toolbarButtons) {
        button.setLookAndFeel(&toolbarLook);
        button.setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // New button
    toolbarButtons[0].setTooltip("New Project");
    toolbarButtons[0].onClick = [this]() {
        auto createFunc = [this]() {
            tabbar.clearTabs();
            
            canvases.clear();
            
            pd.loadPatch(pd.defaultPatch);
            pd.getPatch().setTitle("Untitled Patcher");
            
            auto* cnv = canvases.getFirst();
            mainCanvas = cnv;
        };
        
        if (pd.isDirty()) {
            SaveDialog::show(this, [this, createFunc](int result) {
                if (result == 2) {
                    saveProject([this, createFunc]() mutable {
                        createFunc();
                    });
                } else if (result == 1) {
                    createFunc();
                }
            });
        } else {
            createFunc();
        }
    };

    // Open button
    toolbarButtons[1].setTooltip("Open Project");
    toolbarButtons[1].onClick = [this]() {
        openProject();
    };

    // Save button
    toolbarButtons[2].setTooltip("Save Project");
    toolbarButtons[2].onClick = [this]() {
        saveProject();
    };
    
    // Save Ad button
    toolbarButtons[3].setTooltip("Save Project as");
    toolbarButtons[3].onClick = [this]() {
        saveProjectAs();
    };


    //  Undo button
    toolbarButtons[4].setTooltip("Undo");
    toolbarButtons[4].onClick = [this]() {
        getCurrentCanvas()->undo();
    };

    // Redo button
    toolbarButtons[5].setTooltip("Redo");
    toolbarButtons[5].onClick = [this]() {
        getCurrentCanvas()->redo();
    };

    // New object button
    toolbarButtons[6].setTooltip("Create Object");
    toolbarButtons[6].onClick = [this]() {
        showNewObjectMenu();
    };


    // Show settings
    toolbarButtons[7].setTooltip("Settings");
    toolbarButtons[7].onClick = [this]() {
        // By initialising after the first click we prevent it possibly hanging because audio hasn't started yet
        if(!settingsDialog) {
            // Initialise settings dialog for DAW and standalone
            if (pd.wrapperType == AudioPluginInstance::wrapperType_Standalone) {
                auto pluginHolder = StandalonePluginHolder::getInstance();
                settingsDialog.reset(new SettingsDialog(resources.get(), pd, &pluginHolder->deviceManager, pd.settingsTree, [this]() {
                    pd.updateSearchPaths();
                }));
            } else {
                settingsDialog.reset(new SettingsDialog(resources.get(), pd, nullptr, pd.settingsTree, [this]() {
                    pd.updateSearchPaths();
                }));
            }
        }

        addChildComponent(settingsDialog.get());
        
        
        settingsDialog->setVisible(true);
        settingsDialog->setBounds(getLocalBounds().withSizeKeepingCentre(600, 460));
        settingsDialog->toFront(false);
        settingsDialog->resized();
    };

    // Hide sidebar
    hideButton.setTooltip("Hide Sidebar");
    hideButton.setLookAndFeel(&toolbarLook);
    hideButton.setClickingTogglesState(true);
    hideButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hideButton.setConnectedEdges(12);
    hideButton.onClick = [this]() {
        sidebarHidden = hideButton.getToggleState();
        hideButton.setButtonText(sidebarHidden ? Icons::Show : Icons::Hide);

        repaint();
        resized();
    };

    addAndMakeVisible(hideButton);

    // window size limits
    restrainer.setSizeLimits(600, 300, 4000, 4000);
    resizer.reset(new ResizableCornerComponent(this, &restrainer));
    addAndMakeVisible(resizer.get());

    setSize(pd.lastUIWidth, pd.lastUIHeight);
}

PlugDataPluginEditor::~PlugDataPluginEditor()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    console->setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
    hideButton.setLookAndFeel(nullptr);
    lockButton.setLookAndFeel(nullptr);
    connectionStyleButton.setLookAndFeel(nullptr);
    connectionPathfind.setLookAndFeel(nullptr);
    levelmeter.setLookAndFeel(nullptr);

    for (auto& button : toolbarButtons) {
        button.setLookAndFeel(nullptr);
    }
}


void PlugDataPluginEditor::showNewObjectMenu()
{
    PopupMenu menu;
    
    menu.addItem(16, "Empty Object");
    menu.addSeparator();
    
    
    menu.addItem(1, "Numbox");
    menu.addItem(2, "Message");
    menu.addItem(3, "Bang");
    menu.addItem(4, "Toggle");
    menu.addItem(5, "Horizontal Slider");
    menu.addItem(6, "Vertical Slider");
    menu.addItem(7, "Horizontal Radio");
    menu.addItem(8, "Vertical Radio");

    menu.addSeparator();

    menu.addItem(9, "Float Atom"); // 11
    menu.addItem(10, "Symbol Atom"); // 12

    menu.addSeparator();

    menu.addItem(11, "Array"); // 9
    menu.addItem(12, "GraphOnParent"); // 13
    menu.addItem(13, "Comment"); // 14
    menu.addItem(14, "Canvas"); // 10
    menu.addSeparator();
    menu.addItem(15, "Keyboard");
   

    auto callback = [this](int choice) {
        if (choice < 1)
            return;

        String boxName;

        switch (choice) {
        case 1:
            boxName = "nbx";
            break;

        case 2:
            boxName = "msg";
            break;

        case 3:
            boxName = "bng";
            break;

        case 4:
            boxName = "tgl";
            break;

        case 5:
            boxName = "hsl";
            break;

        case 6:
            boxName = "vsl";
            break;

        case 7:
            boxName = "hradio";
            break;

        case 8:
            boxName = "vradio";
            break;
        case 9:
            boxName = "floatatom";
            break;

        case 10:
            boxName = "symbolatom";
            break;
                
        case 11: {
            ArrayDialog::show(this, [this](int result, String name, String size) {
                if (result) {
                    auto* cnv = getCurrentCanvas();
                    auto* box = new Box(cnv, "graph " + name + " " + size, cnv->lastMousePos);
                    cnv->boxes.add(box);
                }
            });
            return;
        }
        case 12:
            boxName = "graph";
            break;

        case 13:
            boxName = "comment";
            break;
        case 14:
            boxName = "cnv";
            break;
        case 15:
            boxName = "keyboard";
            break;
        }
        auto* cnv = getCurrentCanvas();
       
        cnv->boxes.add(new Box(cnv, boxName,  cnv->viewport->getViewArea().getCentre()));
    };

    menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(toolbarButtons[5]).withParentComponent(this), ModalCallbackFunction::create(callback));
}

//==============================================================================
void PlugDataPluginEditor::paint(Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    auto baseColour = MainLook::firstBackground;
    auto highlightColour = MainLook::highlightColour;

    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);

    // Sidebar
    g.setColour(baseColour.darker(0.1));
    g.fillRect(getWidth() - sWidth, dragbarWidth, sWidth + 10, getHeight() - toolbarHeight);

    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight - 4);

    g.setColour(highlightColour);
    g.drawRoundedRectangle({ -4.0f, toolbarHeight - 6.0f, (float)getWidth() + 9, 20.0f }, 12.0, 4.0);

    // Make sure we cant see the bottom half of the rounded rectangle
    g.setColour(baseColour);
    g.fillRect(0, toolbarHeight - 4, getWidth(), toolbarHeight + 16);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbarHeight, getWidth(), statusbarHeight);

    // Draggable bar
    g.setColour(baseColour);
    g.fillRect(getWidth() - sWidth, dragbarWidth, statusbarHeight, getHeight() - (toolbarHeight - statusbarHeight));
}

void PlugDataPluginEditor::resized()
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);

    int sContentWidth = sWidth - dragbarWidth;

    int sbarY = toolbarHeight - 4;

    console->setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    console->toFront(false);

    inspector.setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    inspector.toFront(false);

    tabbar.setBounds(0, sbarY, getWidth() - sWidth, getHeight() - sbarY - statusbarHeight);
    tabbar.toFront(false);

    
    int idx = 0;
    int toolbarPosition = 0;
    
    FlexBox fb;
    fb.flexWrap = FlexBox::Wrap::noWrap;
    fb.justifyContent = FlexBox::JustifyContent::flexStart;
    fb.alignContent = FlexBox::AlignContent::flexStart;
    fb.flexDirection = FlexBox::Direction::row;
    
    for (int b = 0; b < 9; b++) {
        auto& button = toolbarButtons[b];
        
        auto item = FlexItem(button).withMinWidth(50.0f).withMinHeight(8.0f).withMaxWidth(80.0f);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
        
        if(b == 4 || b == 6) {
            auto item = FlexItem(seperators[b == 4]).withMinWidth(1.0f).withMaxWidth(50.0f);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add (item);
        }
        
        fb.items.add (item);
    }
    
   
    Rectangle<float> toolbarBounds = {5.0f, 0.0f, getWidth() - sWidth + 60.0f, (float)toolbarHeight};
    if(sidebarHidden) toolbarBounds.setWidth(getWidth() - 50.0f);
    
    fb.performLayout(toolbarBounds);
    

    // hide when they fall off the screen
    for (int b = 0; b < 8; b++) {
        toolbarButtons[b].setVisible((toolbarButtons[b].getBounds().getCentreX()) < getWidth() - sidebarWidth);
    }


    hideButton.setBounds(std::min(getWidth() - sWidth, getWidth() - 80), 0, 70, toolbarHeight);

    lockButton.setBounds(8, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    
    connectionStyleButton.setBounds(43, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    connectionPathfind.setBounds(70, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    
    zoomLabel.setBounds(110, getHeight() - statusbarHeight + 1, statusbarHeight * 2, statusbarHeight);
    zoomIn.setBounds(150, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    zoomOut.setBounds(178, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    
    bypassButton.setBounds(getWidth() - sWidth - 40, getHeight() - statusbarHeight, statusbarHeight, statusbarHeight);
    
    levelmeter.setBounds(getWidth() - sWidth - 150, getHeight() - statusbarHeight, 100, statusbarHeight);

    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    resizer->toFront(false);

    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();
}

bool PlugDataPluginEditor::keyPressed(const KeyPress& key, Component* originatingComponent)
{
    auto* cnv = getCurrentCanvas();
    
    // cmd-e
    if (key.getModifiers().isCommandDown() && key.isKeyCode(69)) {
        lockButton.triggerClick();
        return true;
    }
    
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.isKeyCode(89)) {
        for(auto& connection : cnv->connections) {
            if(connection->isSelected) {
                connection->applyPath(connection->findPath());
            }
        }
        return true;
    }

    // Zoom in
    if (key.isKeyCode(61) && key.getModifiers().isCommandDown()) {
        zoom(true);
        
        return true;
    }
    // Zoom out
    if (key.isKeyCode(45) && key.getModifiers().isCommandDown()) {
        zoom(false);
        return true;
    }

    if (pd.locked)
        return false;

    // Key shortcuts for creating objects
    if (key.getTextCharacter() == 'n') {
        cnv->boxes.add(new Box(cnv, "", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'c' && !key.getModifiers().isCommandDown()) {
        cnv->boxes.add(new Box(cnv, "comment", cnv->lastMousePos));
        return true;
    }
    if (key.isKeyCode(65) && key.getModifiers().isCommandDown()) {
        for (auto* child : cnv->boxes) {
            cnv->setSelected(child, true);
        }
        for (auto* child : cnv->connections) {
            child->isSelected = true;
            child->repaint();
        }
        return true;
    }
    if (key.getTextCharacter() == 'b') {
        cnv->boxes.add(new Box(cnv, "bng", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'm') {
        cnv->boxes.add(new Box(cnv, "msg", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'i') {
        cnv->boxes.add(new Box(cnv, "nbx", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 'f') {
        cnv->boxes.add(new Box(cnv, "floatatom", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 't') {
        cnv->boxes.add(new Box(cnv, "tgl", cnv->lastMousePos));
        return true;
    }
    if (key.getTextCharacter() == 's') {
        cnv->boxes.add(new Box(cnv, "vsl", cnv->lastMousePos));
        return true;
    }

    if (key.getKeyCode() == KeyPress::backspaceKey) {
        cnv->removeSelection();
        return true;
    }
    // cmd-c
    if (key.getModifiers().isCommandDown() && key.isKeyCode(67)) {
        cnv->copySelection();
        return true;
    }
    // cmd-v
    if (key.getModifiers().isCommandDown() && key.isKeyCode(86)) {
        cnv->pasteSelection();
        return true;
    }
    // cmd-x
    if (key.getModifiers().isCommandDown() && key.isKeyCode(88)) {
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    // cmd-d
    if (key.getModifiers().isCommandDown() && key.isKeyCode(68)) {
        cnv->duplicateSelection();
        return true;
    }

    // cmd-shift-z
    if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && key.isKeyCode(90)) {
        cnv->redo();
        return true;
    }
    // cmd-z
    if (key.getModifiers().isCommandDown() && key.isKeyCode(90)) {
        cnv->undo();
        return true;
    }

    return false;
}

void PlugDataPluginEditor::mouseDown(const MouseEvent& e)
{
    Rectangle<int> drag_bar(getWidth() - sidebarWidth, dragbarWidth, sidebarWidth, getHeight() - toolbarHeight);
    if (drag_bar.contains(e.getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = sidebarWidth;
    } else {
        draggingSidebar = false;
    }
}

void PlugDataPluginEditor::mouseDrag(const MouseEvent& e)
{
    if (draggingSidebar) {
        sidebarWidth = dragStartWidth - e.getDistanceFromDragStartX();
        repaint();
        resized();
    }
}

void PlugDataPluginEditor::mouseUp(const MouseEvent& e)
{
    if(draggingSidebar) {
        getCurrentCanvas()->checkBounds();
        draggingSidebar = false;
    }
    
}


void PlugDataPluginEditor::openProject()
{

    auto openFunc = [this](const FileChooser& f) {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
            tabbar.clearTabs();
            pd.loadPatch(openedFile);
        }
    };

    if (pd.isDirty()) {

        SaveDialog::show(this, [this, openFunc](int result) {
            if (result == 2) {
                saveProject([this, openFunc]() mutable {
                    openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
                    
                });
            } else if (result != 0) {
                openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
            }
        });
    } else {
        openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
    }
}


void PlugDataPluginEditor::saveProjectAs(std::function<void()> nestedCallback)
{
    saveChooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, nestedCallback](const FileChooser& f) mutable {
        
        File result = saveChooser.getResult();

        if(result.getFullPathName().isNotEmpty()) {
            result.deleteFile();
        
            pd.savePatch(result);
        }

        nestedCallback();
    });
}

void PlugDataPluginEditor::saveProject(std::function<void()> nestedCallback)
{
    if(pd.getCurrentFile().existsAsFile()) {
        getCurrentCanvas()->pd->savePatch();
        nestedCallback();
    }
    else {
        saveProjectAs(nestedCallback);
    }
}


void PlugDataPluginEditor::updateValues()
{
    auto* cnv = getCurrentCanvas();
    
    if(!cnv) return;
    
    for (auto& box : cnv->boxes) {
        if (box->graphics && box->isShowing()) {
            box->graphics->updateValue();
        }
    }

    updateUndoState();

}

void PlugDataPluginEditor::updateUndoState()
{
    pd.setThis();

    toolbarButtons[6].setEnabled(!pd.locked);

    if (getCurrentCanvas() && getCurrentCanvas()->patch.getPointer() && !pd.locked) {
        getCurrentCanvas()->patch.setCurrent();

        auto* cnv = getCurrentCanvas()->patch.getPointer();
        
        pd.canvasLock.lock();
        bool canUndo = libpd_can_undo(cnv);
        bool canRedo = libpd_can_redo(cnv);
        pd.canvasLock.unlock();
        
        toolbarButtons[4].setEnabled(canUndo);
        toolbarButtons[5].setEnabled(canRedo);
        
    } else {
        toolbarButtons[4].setEnabled(false);
        toolbarButtons[5].setEnabled(false);
    }
}

Canvas* PlugDataPluginEditor::getCurrentCanvas()
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PlugDataPluginEditor::getMainCanvas()
{
    return mainCanvas;
}


Canvas* PlugDataPluginEditor::getCanvas(int idx)
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx))) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }

    return nullptr;
}

void PlugDataPluginEditor::addTab(Canvas* cnv)
{
    tabbar.addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);

    int tabIdx = tabbar.getNumTabs() - 1;
    tabbar.setCurrentTabIndex(tabIdx);

    if (tabbar.getNumTabs() > 1) {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    } else {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }

    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);

    auto* closeButton = new TextButton(Icons::Clear);

    closeButton->onClick = [this, tabButton]() mutable {
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for (int i = 0; i < tabbar.getNumTabs(); i++) {
            if (tabbar.getTabbedButtonBar().getTabButton(i) == tabButton) {
                idx = i;
                break;
            }
        }

        if (idx == -1)
            return;

        if (tabbar.getCurrentTabIndex() == idx) {
            tabbar.setCurrentTabIndex(0, false);
        }

        auto* cnv = getCanvas(idx);
        canvases.removeObject(cnv);
        tabbar.removeTab(idx);

        tabbar.setCurrentTabIndex(0, true);

        if (tabbar.getNumTabs() == 1) {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };

    closeButton->setLookAndFeel(&statusbarLook);
    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setColour(TextButton::textColourOnId, Colours::white);
    closeButton->setColour(TextButton::textColourOffId, Colours::white);
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);

    closeButton->setVisible(tabIdx != 0);
    closeButton->setSize(28, 28);

    tabbar.repaint();

    cnv->setVisible(true);
}

void PlugDataPluginEditor::zoom(bool zoomIn)
{
    transform = transform.scaled(zoomIn ? 1.1f : 1.0f / 1.1f);
    for(auto& cnv : canvases) cnv->setTransform(transform);
    
    int scale = ((std::abs (transform.mat00) + std::abs (transform.mat11)) / 2.0f) * 100.0f;
    zoomLabel.setText(String(scale) + "%", dontSendNotification);
    getCurrentCanvas()->checkBounds();
}
