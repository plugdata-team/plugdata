#include "MainComponent.h"
#include "Canvas.h"
#include "Edge.h"
#include "Pd/x_libpd_mod_utils.h"
#include "Dialogs.h"

//==============================================================================

// Print std::cout and std::cerr to console when in debug mode
#if JUCE_DEBUG
#define LOG_STDOUT true
#else
#define LOG_STDOUT false
#endif

MainComponent::MainComponent() : ValueTreeObject(ValueTree("Main")), console(LOG_STDOUT, LOG_STDOUT), pd(&console)
{
    setSize(1000, 700);
    
    LookAndFeel::setDefaultLookAndFeel(&mainLook);
    
    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, MainLook::background_1);
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, MainLook::background_1);
    tabbar.setColour(TabbedComponent::outlineColourId, MainLook::background_1);
    
    //player.reset(nullptr);
    //set_remote(false);
    
    setLookAndFeel(&mainLook);
    
    tabbar.onTabChange = [this](int idx) {
        Edge::connectingEdge = nullptr;
        
        if(idx == -1) return;
        
        // update GraphOnParent when changing tabs
        for(auto* box : getCurrentCanvas()->findChildrenOfClass<Box>()) {
            if(box->graphics && box->graphics->getGUI().getType() == pd::Gui::Type::GraphOnParent) {
                box->graphics.get()->getCanvas()->synchronise();
            }
            if(box->graphics && (box->graphics->getGUI().getType() == pd::Gui::Type::Subpatch || box->graphics->getGUI().getType() == pd::Gui::Type::GraphOnParent)) {
                box->updatePorts();
            }
        }
        
        auto* cnv = getCurrentCanvas();
        if(cnv->patch.getPointer()) {
            cnv->patch.setCurrent();
        }

        
        valueTreeChanged();
    };
    
    startButton.setClickingTogglesState(true);
    startButton.setConnectedEdges(12);
    startButton.setLookAndFeel(&statusbarLook);
    addAndMakeVisible(startButton);

    
    addAndMakeVisible(tabbar);
    rebuildObjects();
    
    addAndMakeVisible(&console);
    

    startButton.onClick = [this]() {
        pd.setBypass(!startButton.getToggleState());
    };
    
    startButton.setToggleState(true, dontSendNotification);
    
    for(auto& button : toolbarButtons) {
        button.setLookAndFeel(&toolbarLook);
        button.setConnectedEdges(12);
        addAndMakeVisible(button);
    }
    
    // New button
    toolbarButtons[0].onClick = [this]() {
        ValueTreeObject::removeAllChildren();
        tabbar.clearTabs();
        
        auto canvas = ValueTree(Identifiers::canvas);
        canvas.setProperty("Title", "Untitled Patcher", nullptr);
        canvas.setProperty(Identifiers::isGraph, false, nullptr);
        appendChild(canvas);
        
        mainCanvas = findChildOfClass<Canvas>(0);
        mainCanvas->loadPatch(defaultPatch);
    };
    
    // Open button
    toolbarButtons[1].onClick = [this]() {
        openProject();
    };
    
    // Save button
    toolbarButtons[2].onClick = [this]() {
        saveProject();
    };
    
    toolbarButtons[3].onClick = [this]() {
       
        getCurrentCanvas()->undo();
    };
    
    toolbarButtons[4].onClick = [this]() {
        getCurrentCanvas()->redo();
    };
    
    toolbarButtons[5].onClick = [this]() {
        PopupMenu menu;
        menu.addItem (1, "Numbox");
        menu.addItem (2, "Message");
        menu.addItem (3, "Bang");
        menu.addItem (4, "Toggle");
        menu.addItem (5, "Horizontal Slider");
        menu.addItem (6, "Vertical Slider");
        menu.addItem (7, "Horizontal Radio");
        menu.addItem (8, "Vertical Radio");
        
        menu.addSeparator();
        
        menu.addItem (11, "Float Atom");
        menu.addItem (12, "Symbol Atom");
        
        menu.addSeparator();
        
        menu.addItem (9, "Array");
        menu.addItem (13, "GraphOnParent");
        menu.addItem (14, "Comment");
        menu.addItem (10, "Canvas");
        
        
        auto callback = [this](int choice) {
            if(choice < 1) return;
            
            auto box = ValueTree(Identifiers::box);
            box.setProperty(Identifiers::boxX, 100, nullptr);
            box.setProperty(Identifiers::boxY, 100, nullptr);
            
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
                
                case 9: {
                    String size;
                    String name;
                    
                    ArrayDialog arrayDialog(&size, &name);
                    
                    arrayDialog.setBounds(getScreenX()  + (getWidth() / 2.) - 200., getScreenY() + toolbarHeight - 3, 300, 200);
                    
                    auto result = arrayDialog.runModalLoop();
                    if(result == 1) {
                        boxName = "graph " + name + " " + size;
                    }
                    else {
                        return;
                    }
                    break;
                }
                
               
                
                case 10:
                boxName = "canvas";
                break;
                    
                case 11:
                boxName = "floatatom";
                break;
                    
                case 12:
                boxName = "symbolatom";
                break;
                    
                case 13:
                boxName = "graph";
                break;
                    
                case 14:
                boxName = "comment";
                break;
                    

            }
            
            box.setProperty(Identifiers::boxName, boxName, nullptr);
            getCurrentCanvas()->appendChild(box);
        };
        
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent (toolbarButtons[5]), ModalCallbackFunction::create(callback));
    };
    
    hideButton.setLookAndFeel(&toolbarLook);
    hideButton.setClickingTogglesState(true);
    hideButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hideButton.setConnectedEdges(12);
    
    hideButton.onClick = [this](){
        sidebarHidden = hideButton.getToggleState();
        hideButton.setButtonText(sidebarHidden ? CharPointer_UTF8("\xef\x81\x93") : CharPointer_UTF8("\xef\x81\x94"));
        
        repaint();
        resized();
    };
    
    addAndMakeVisible(hideButton);
    
    if(!hasProperty("Canvas")) {
        ValueTree cnv_state("Canvas");
        cnv_state.setProperty("Title", "Untitled Patcher", nullptr);
        cnv_state.setProperty(Identifiers::isGraph, false, nullptr);
        
        appendChild(cnv_state);
        
        mainCanvas = findChildOfClass<Canvas>(0);
        mainCanvas->loadPatch(defaultPatch);
        pd.m_patch = mainCanvas->patch.getPointer();
        
    }
}

MainComponent::~MainComponent()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    startButton.setLookAndFeel(nullptr);
    hideButton.setLookAndFeel(nullptr);
    
    for(auto& button : toolbarButtons) {
        button.setLookAndFeel(nullptr);
    }
}


ValueTreeObject* MainComponent::factory(const Identifier& id, const ValueTree& tree)
{
    if(id == Identifiers::canvas) {
        auto* canvas = new Canvas(tree, this);
        addTab(canvas);

        return static_cast<ValueTreeObject*>(canvas);
    }
    
    return nullptr;
}


//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    
    auto base_colour = MainLook::background_1;
    auto highlight_colour = MainLook::highlight_colour;
    
    int s_width = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);

    
    // Sidebar
    g.setColour(base_colour.darker(0.1));
    g.fillRect(getWidth() - s_width, dragbarWidth, s_width + 10, getHeight() - toolbarHeight);
    
    // Toolbar background
    g.setColour(base_colour);
    g.fillRect(0, 0, getWidth(), toolbarHeight - 4);
    
    g.setColour(highlight_colour);
    g.drawRoundedRectangle({-4.0f, 39.0f, (float)getWidth() + 9, 20.0f}, 12.0, 4.0);
    
    // Make sure we cant see the bottom half of the rounded rectangle
    g.setColour(base_colour);
    g.fillRect(0, toolbarHeight - 4, getWidth(), toolbarHeight + 16);
    
    // Statusbar background
    g.setColour(base_colour);
    g.fillRect(0, getHeight() - statusbarHeight, getWidth(), statusbarHeight);
    
    // Draggable bar
    g.setColour(base_colour);
    g.fillRect(getWidth() - s_width, dragbarWidth, statusbarHeight, getHeight() - (toolbarHeight - statusbarHeight));
    
    
}

void MainComponent::resized()
{
    int s_width = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);
    
    int s_content_width = s_width - dragbarWidth;
    
    int sbar_y = toolbarHeight - 4;
    
    console.setBounds(getWidth() - s_content_width, sbar_y + 2, s_content_width, getHeight() - sbar_y);
    console.toFront(false);
    
    tabbar.setBounds(0, sbar_y, getWidth() - s_width, getHeight() - sbar_y - statusbarHeight);
    tabbar.toFront(false);
    
    startButton.setBounds(getWidth() - s_width - 40, getHeight() - 27, 27, 27);
    
    
    int jump_positions[2] = {3, 5};
    int idx = 0;
    int toolbar_position = 0;
    for(auto& button : toolbarButtons) {
        int spacing = (25 * (idx >= jump_positions[0])) +  (25 * (idx >= jump_positions[1])) + 10;
        button.setBounds(toolbar_position + spacing, 0, 70, toolbarHeight);
        toolbar_position += 70;
        idx++;
    }
    
    hideButton.setBounds(std::min(getWidth() - s_width, getWidth() - 80), 0, 70, toolbarHeight);
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::mouseDown(const MouseEvent &e) {
    Rectangle<int> drag_bar(getWidth() - sidebarWidth, dragbarWidth, sidebarWidth, getHeight() - toolbarHeight);
    if(drag_bar.contains(e.getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = sidebarWidth;
    }
    else {
        draggingSidebar = false;
    }
    
   
}

void MainComponent::mouseDrag(const MouseEvent &e) {
    if(draggingSidebar) {
        sidebarWidth = dragStartWidth - e.getDistanceFromDragStartX();
        repaint();
        resized();
    }
}

void MainComponent::mouseUp(const MouseEvent &e) {
    draggingSidebar = false;
}


void MainComponent::openProject() {
    
    int result = 2;
    
    if(getCurrentCanvas()->changed()) {
        SaveDialog dialog;

        dialog.setBounds(getScreenX()  + (getWidth() / 2.) - 200., getScreenY() + toolbarHeight - 3, 400, 130);
        
        result = dialog.runModalLoop();
        
        // TODO: the async calling is killing me here
        if(result == 1) {
            saveProject();
        }
    }
    
    if(result != 0) {
        // Close all tabs
#if JUCE_MODAL_LOOPS_PERMITTED
    openChooser.browseForFileToOpen();
    
    File openedFile = openChooser.getResult();
    
    if(openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
        
        ValueTreeObject::removeAllChildren();
        tabbar.clearTabs();
        
        auto canvas = ValueTree(Identifiers::canvas);
        canvas.setProperty("Title", openedFile.getFileName(), nullptr);
        canvas.setProperty(Identifiers::isGraph, false, nullptr);
        appendChild(canvas);
        
        mainCanvas = findChildOfClass<Canvas>(0);
        mainCanvas->loadPatch(openedFile.loadFileAsString());
    }
    
#else
    openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](const FileChooser& f) {
        
          ValueTreeObject::removeAllChildren();
          tabbar.clearTabs();
        
        
          File openedFile = f.getResult();
          if(openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
              canvas->loadPatch(openedFile.loadFileAsString());
          }
    });
    
#endif
        

    }
}

void MainComponent::saveProject() {
    
    WaitableEvent event;
    
    auto to_save = pd.getCanvasContent();
    
#if JUCE_MODAL_LOOPS_PERMITTED
    saveChooser.browseForFileToSave(true);
    
    File result = saveChooser.getResult();
    
    FileOutputStream ostream(result);
    ostream.writeString(to_save);
    
    getCurrentCanvas()->setProperty("Title", result.getFileName());
    
#else
    saveChooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, to_save, &event](const FileChooser &f) mutable {
        
        File result = saveChooser.getResult();
        
        FileOutputStream ostream(result);
        ostream.writeString(to_save);
        
        getCurrentCanvas()->setProperty("Title", result.getFileName(), nullptr);
    });
    
#endif
    
    getCurrentCanvas()->hasChanged = false;
    
}

void MainComponent::timerCallback() {
    for(auto* cnv : findChildrenOfClass<Canvas>(true)) {
        for(auto& box : cnv->findChildrenOfClass<Box>()) {
            if(box->graphics) {
                box->graphics->updateValue();
            }
        }
    }
}

void MainComponent::valueTreeChanged() {
    
    pd.setThis();
    
    //toolbarButtons[3].setEnabled(libpd_can_undo(getCurrentCanvas()->patch.getPointer()));
    //toolbarButtons[4].setEnabled(libpd_can_redo(getCurrentCanvas()->patch.getPointer()));
}

Canvas* MainComponent::getCurrentCanvas()
{
    if(auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if(auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* MainComponent::getMainCanvas() {
    return mainCanvas;
}

Canvas* MainComponent::getCanvas(int idx) {
    if(auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx))) {
        if(auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    
    return nullptr;
}


void MainComponent::addTab(Canvas* cnv)
{
    tabbar.addTab(cnv->getProperty("Title"), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);
    
    int tab_idx = tabbar.getNumTabs() - 1;
    tabbar.setCurrentTabIndex(tab_idx);
    
    if(tabbar.getNumTabs() > 1) {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    }
    else {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }
    
    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tab_idx);
    
    auto* closeButton = new TextButton("x");
   
    closeButton->onClick = [this, tabButton]() mutable {
        
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for(int i = 0; i < tabbar.getNumTabs(); i++) {
            if(tabbar.getTabbedButtonBar().getTabButton(i) == tabButton) {
                idx = i;
                break;
            }
        }
        
        if(idx == -1) return;
        
        if(tabbar.getCurrentTabIndex() == idx) {
            tabbar.setCurrentTabIndex(0, false);
        }
        
        auto* cnv = getCanvas(idx);
        
        removeChild(cnv->getState());
        tabbar.removeTab(idx);
        
        tabbar.setCurrentTabIndex(0, true);
        
        if(tabbar.getNumTabs() == 1) {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };
    
    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setColour(TextButton::textColourOnId, Colours::white);
    closeButton->setColour(TextButton::textColourOffId, Colours::white);
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);
    
    closeButton->setVisible(tab_idx != 0);
    closeButton->setSize(28, 28);
    
    tabbar.repaint();
    
    cnv->setVisible(true);
    cnv->setBounds(0, 0, 1000, 700);
}
