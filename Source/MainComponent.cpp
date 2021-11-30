#include "MainComponent.h"
#include "Canvas.h"
#include "Edge.h"
#include "Pd/x_libpd_mod_utils.h"
#include "SaveDialog.h"

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
        
        // update GraphOnParent when changing tabs
        for(auto* box : getCurrentCanvas()->findChildrenOfClass<Box>()) {
            if(box->graphics && box->graphics->getGUI().getType() == pd::Gui::Type::GraphOnParent) {
                box->graphics.get()->getCanvas()->synchronise();
            }
            if(box->graphics && box->graphics->getGUI().getType() == pd::Gui::Type::Subpatch) {
                box->updatePorts();
            }
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
        auto new_cnv = ValueTree(Identifiers::canvas);
        new_cnv.setProperty("Title", "Untitled Patcher", nullptr);
        new_cnv.setProperty(Identifiers::is_graph, false, nullptr);
        getState().appendChild(new_cnv, nullptr);
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
        
        menu.addItem (9, "Graph");
        menu.addItem (13, "GraphOnParent");
        menu.addItem (14, "Comment");
        menu.addItem (10, "Canvas");
        
        
        auto callback = [this](int choice) {
            if(choice < 1) return;
            
            auto box = ValueTree(Identifiers::box);
            box.setProperty(Identifiers::box_x, 100, nullptr);
            box.setProperty(Identifiers::box_y, 100, nullptr);
            
            String box_name;
            
            switch (choice) {
                case 1:
                box_name = "nbx";
                break;
                
                case 2:
                box_name = "msg";
                break;
                
                case 3:
                box_name = "bng";
                break;
                
                case 4:
                box_name = "tgl";
                break;
                
                case 5:
                box_name = "hsl";
                break;
                
                case 6:
                box_name = "vsl";
                break;
                
                case 7:
                box_name = "hradio";
                break;
                
                case 8:
                box_name = "vradio";
                break;
                
                case 9:
                box_name = "graph arr1 128";
                break;
                
                case 10:
                box_name = "canvas";
                break;
                    
                case 11:
                box_name = "floatatom";
                break;
                    
                case 12:
                box_name = "symbolatom";
                break;
                    
                case 13:
                box_name = "graph";
                break;
                    
                case 14:
                box_name = "comment";
                break;
                    

            }
            
            box.setProperty(Identifiers::box_name, box_name, nullptr);
            canvas->getState().appendChild(box, nullptr);
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
    
    if(!getState().hasProperty("Canvas")) {
        ValueTree cnv_state("Canvas");
        cnv_state.setProperty("Title", "Untitled Patcher", nullptr);
        cnv_state.setProperty(Identifiers::is_graph, false, nullptr);
        
        getState().appendChild(cnv_state, nullptr);
        
        auto* new_cnv = findChildOfClass<Canvas>(0);
        new_cnv->loadPatch("#N canvas 827 239 527 327 12;");
        pd.m_patch = new_cnv->patch.getPointer();
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
        canvas = new Canvas(tree, this);
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
        //tabbar.clearTabs();
#if JUCE_MODAL_LOOPS_PERMITTED
    openChooser.browseForFileToOpen();
    
    File openedFile = openChooser.getResult();
    
    if(openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
        canvas->loadPatch(openedFile.loadFileAsString());
    }
    
#else
    openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](const FileChooser& f) {
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
    
    getCurrentCanvas()->getState().setProperty("Title", result.getFileName(), nullptr);
    
#else
    saveChooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, to_save, &event](const FileChooser &f) mutable {
        
        File result = saveChooser.getResult();
        
        FileOutputStream ostream(result);
        ostream.writeString(to_save);
        
        getCurrentCanvas()->getState().setProperty("Title", result.getFileName(), nullptr);
    });
    
#endif
    
    getCurrentCanvas()->hasChanged = false;
    
}


void MainComponent::valueTreeChanged() {
    
    pd.setThis();
    
    toolbarButtons[3].setEnabled(libpd_can_undo(static_cast<t_canvas*>(pd.m_patch)));
    toolbarButtons[4].setEnabled(libpd_can_redo(static_cast<t_canvas*>(pd.m_patch)));
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

void MainComponent::addTab(Canvas* cnv)
{
    tabbar.addTab(cnv->getState().getProperty("Title"), findColour(ResizableWindow::backgroundColourId), &cnv->viewport, false);
    
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
    
    auto* tab_button = tabbar.getTabbedButtonBar().getTabButton(tab_idx);
    
    auto* close_button = new TextButton("x");
   
    close_button->onClick = [this, tab_button]() mutable {
        
        int idx = -1;
        for(int i = 0; i < tabbar.getNumTabs(); i++) {
            if(tabbar.getTabbedButtonBar().getTabButton(i) == tab_button) {
                idx = i;
                break;
            }
        }
        if(idx == -1) return;
        
        if(tabbar.getCurrentTabIndex() == idx) {
            tabbar.setCurrentTabIndex(idx == 1 ? idx - 1 : idx + 1);
        }
        tabbar.removeTab(idx);
        
        if(tabbar.getNumTabs() == 1) {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };
    
    close_button->setColour(TextButton::buttonColourId, Colour());
    close_button->setColour(TextButton::buttonOnColourId, Colour());
    close_button->setColour(ComboBox::outlineColourId, Colour());
    close_button->setColour(TextButton::textColourOnId, Colours::white);
    close_button->setColour(TextButton::textColourOffId, Colours::white);
    close_button->setConnectedEdges(12);
    tab_button->setExtraComponent(close_button, TabBarButton::beforeText);
    
    close_button->setVisible(true);
    close_button->setSize(28, 28);
    
    tabbar.repaint();
    
    canvas->setVisible(true);
    canvas->setBounds(0, 0, 1000, 700);
}
