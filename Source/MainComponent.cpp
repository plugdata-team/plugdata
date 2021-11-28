#include "MainComponent.h"
#include "Canvas.h"
#include "Edge.h"
#include "Pd/x_libpd_mod_utils.h"

//==============================================================================
MainComponent::MainComponent() : ValueTreeObject(ValueTree("Main")), console(true, true), pd(&console)
{
    setSize(1000, 700);
    
    LookAndFeel::setDefaultLookAndFeel(&main_look);
    
    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, Colour(25, 25, 25));
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, Colour(25, 25, 25));
    tabbar.setColour(TabbedComponent::outlineColourId, Colour(25, 25, 25));
    
    //player.reset(nullptr);
    //set_remote(false);
    
    setLookAndFeel(&main_look);
    
    tabbar.on_tab_change = [this](int idx) {
        Edge::connecting_edge = nullptr;
        triggerChange();
    };
    
    start_button.setClickingTogglesState(true);
    start_button.setConnectedEdges(12);
    start_button.setLookAndFeel(&statusbar_look);
    addAndMakeVisible(start_button);

    
    addAndMakeVisible(tabbar);
    rebuildObjects();
    
    addAndMakeVisible(&console);
    

    start_button.onClick = [this]() {
        pd.setBypass(!start_button.getToggleState());
    };
    
    start_button.setToggleState(true, dontSendNotification);
    
    for(auto& button : toolbar_buttons) {
        button.setLookAndFeel(&toolbar_look);
        button.setConnectedEdges(12);
        addAndMakeVisible(button);
    }
    
    // New button
    toolbar_buttons[0].onClick = [this]() {
        auto new_cnv = ValueTree("Canvas");
        new_cnv.setProperty("Title", "Untitled Patcher", nullptr);
        getState().appendChild(new_cnv, nullptr);
    };
    
    // Open button
    toolbar_buttons[1].onClick = [this]() {
        open_project();
    };
    
    // Save button
    toolbar_buttons[2].onClick = [this]() {
        save_project();
    };
    
    toolbar_buttons[3].onClick = [this]() {
        get_current_canvas()->undo();
    };
    
    toolbar_buttons[4].onClick = [this]() {
        get_current_canvas()->redo();
    };
    
    toolbar_buttons[5].onClick = [this]() {
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
            }
            
            box.setProperty(Identifiers::box_name, box_name, nullptr);
            canvas->getState().appendChild(box, nullptr);
        };
        
        
        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent (toolbar_buttons[5]), ModalCallbackFunction::create(callback));

        
    };
    
    


    
    hide_button.setLookAndFeel(&toolbar_look);
    hide_button.setClickingTogglesState(true);
    hide_button.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hide_button.setConnectedEdges(12);
    
    hide_button.onClick = [this](){
        sidebar_hidden = hide_button.getToggleState();
        hide_button.setButtonText(sidebar_hidden ? CharPointer_UTF8("\xef\x81\x93") : CharPointer_UTF8("\xef\x81\x94"));
        
        repaint();
        resized();
    };
    
    addAndMakeVisible(hide_button);
    
    
    if(!getState().hasProperty("Canvas")) {
        ValueTree cnv_state("Canvas");
        cnv_state.setProperty("Title", "Untitled Patcher", nullptr);
        getState().appendChild(cnv_state, nullptr);
    }
}

MainComponent::~MainComponent()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    start_button.setLookAndFeel(nullptr);
    hide_button.setLookAndFeel(nullptr);
    
    for(auto& button : toolbar_buttons) {
        button.setLookAndFeel(nullptr);
    }
}


ValueTreeObject* MainComponent::factory(const Identifier& id, const ValueTree& tree)
{
    if(id == Identifiers::canvas) {
        canvas = new Canvas(tree, this);
        add_tab(canvas);

        return static_cast<ValueTreeObject*>(canvas);
    }
    
    return nullptr;
}


//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    
    auto base_colour = Colour(25, 25, 25);
    auto highlight_colour = Colour (0xff42a2c8).darker(0.2);
    
    int s_width = sidebar_hidden ? dragbar_width : std::max(dragbar_width, sidebar_width);

    
    // Sidebar
    g.setColour(base_colour.darker(0.1));
    g.fillRect(getWidth() - s_width, dragbar_width, s_width, getHeight() - toolbar_height);
    
    // Toolbar background
    g.setColour(base_colour);
    g.fillRect(0, 0, getWidth(), toolbar_height - 4);
    
    g.setColour(highlight_colour);
    g.drawRoundedRectangle({-4.0f, 39.0f, (float)getWidth() + 9, 20.0f}, 10.0, 4.0);
    
    // Statusbar background
    g.setColour(base_colour);
    g.fillRect(0, getHeight() - statusbar_height, getWidth(), statusbar_height);
    
    // Draggable bar
    g.setColour(base_colour);
    g.fillRect(getWidth() - s_width, dragbar_width, statusbar_height, getHeight() - (toolbar_height - 5));
    
    
}

void MainComponent::resized()
{
    int s_width = sidebar_hidden ? dragbar_width : std::max(dragbar_width, sidebar_width);
    
    int s_content_width = s_width - dragbar_width;
    
    int sbar_y = toolbar_height - 4;
    
    console.setBounds(getWidth() - s_content_width, sbar_y + 2, s_content_width, getHeight() - sbar_y);
    console.toFront(false);
    
    tabbar.setBounds(0, sbar_y, getWidth() - s_width, getHeight() - sbar_y - statusbar_height);
    tabbar.toFront(false);
    
    start_button.setBounds(getWidth() - s_width - 40, getHeight() - 30, 30, 30);
    
    
    int jump_positions[2] = {3, 5};
    int idx = 0;
    int toolbar_position = 0;
    for(auto& button : toolbar_buttons) {
        int spacing = (25 * (idx >= jump_positions[0])) +  (25 * (idx >= jump_positions[1])) + 10;
        button.setBounds(toolbar_position + spacing, 0, 70, toolbar_height);
        toolbar_position += 70;
        idx++;
    }
    
    
    hide_button.setBounds(std::min(getWidth() - s_width, getWidth() - 80), 0, 70, toolbar_height);
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::mouseDown(const MouseEvent &e) {
    Rectangle<int> drag_bar(getWidth() - sidebar_width, dragbar_width, sidebar_width, getHeight() - toolbar_height);
    if(drag_bar.contains(e.getPosition()) && !sidebar_hidden) {
        dragging_sidebar = true;
        drag_start_width = sidebar_width;
    }
    else {
        dragging_sidebar = false;
    }
    
   
}

void MainComponent::mouseDrag(const MouseEvent &e) {
    if(dragging_sidebar) {
        sidebar_width = drag_start_width - e.getDistanceFromDragStartX();
        repaint();
        resized();
    }
}

void MainComponent::mouseUp(const MouseEvent &e) {
    dragging_sidebar = false;
}

void MainComponent::pd_synchonize() {
    auto objects = pd.getPatch().getObjects();
    
    std::vector<pd::Object> found_objects;
    std::vector<pd::Object> new_objects;
    
    for(auto& object : objects) {
        bool found = false;
        
        for(auto& box : canvas->findChildrenOfClass<Box>()) {
            if(object.getPointer() == box->pd_object) {
                found_objects.push_back(object);
                found = true;
                break;
            }
        }
        
        if(!found) {
            new_objects.push_back(object);
        }
    }

    
    
}

void MainComponent::open_project() {
    open_chooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this](const FileChooser& f) {
          File openedfile = f.getResult();
          if(openedfile.exists() && openedfile.getFileExtension().equalsIgnoreCase(".pd")) {
              try
              {
                  for(auto& box : canvas->findChildrenOfClass<Box>()) {
                      box->remove_box();
                  }
                  
                  libpd_canvas_doclear(static_cast<t_canvas*>(pd.m_patch));
                  
                  StringArray lines;
                  openedfile.readLines(lines);
                  
                  Array<Box*> boxes;
                  
                  for(auto& line : lines) {
                      
                      auto args = TokenString(line.toStdString(), ' ', '\"');
                      
                      if(!args.size()) return;
                      
                      if(args[0] == std::string("#N")) {
                          continue;
                      }
                      else if(args[0] == std::string("#X")) {
                        if(args[1] == std::string("obj")) {
                            
                            auto x = std::stoi(args[2]);
                            auto y = std::stoi(args[3]);
                            
                            String name;
                            for(int i = 4; i < args.size(); i++) {
                                name += String(args[i] + " ");
                            }
                            
                            name = name.dropLastCharacters(2); // remove excess space and semicolon
                            
                            auto gui_simplify = [](String& target, const String& selector) {
                                if(target.startsWith(selector)) {
                                    target = selector;
                                }
                            };
                            
                            gui_simplify(name, "bng");
                            gui_simplify(name, "tgl");
                            gui_simplify(name, "nbx");
                            //gui_simplify(name, "msg"); // not sure
                            gui_simplify(name, "hsl");
                            gui_simplify(name, "vsl");
                            gui_simplify(name, "hradio");
                            gui_simplify(name, "vradio");
  
                            
                            auto box = ValueTree(Identifiers::box);
                   
                            canvas->getState().appendChild(box, nullptr);
                            box.setProperty(Identifiers::box_name, name, nullptr);
                            box.setProperty(Identifiers::box_x, x, nullptr);
                            box.setProperty(Identifiers::box_y, y, nullptr);
                            
                            
                            continue;
                        }
                        else if(args[1] == std::string("msg")) {
                            auto x = std::stoi(args[2]);
                            auto y = std::stoi(args[3]);
                            
                            String message;
                            for(int i = 4; i < args.size(); i++) {
                                message += String(args[i] + " ");
                            }
                            
                            message = message.dropLastCharacters(2);
                            message.removeCharacters("\\");
                            
                            auto box = ValueTree(Identifiers::box);
                            canvas->getState().appendChild(box, nullptr);
                            box.setProperty(Identifiers::box_name, "msg " + message, nullptr);
                            box.setProperty(Identifiers::box_x, x, nullptr);
                            box.setProperty(Identifiers::box_y, y, nullptr);
                            
                        }
                        else if(args[1] == std::string("connect")) {
                            int start_box = std::stoi(args[4]);
                            int inlet = std::stoi(args[5]);
                            int end_box = std::stoi(args[2]);
                            int outlet = std::stoi(args[3]);
                            
                            auto boxes = canvas->findChildrenOfClass<Box>();
                            
                            auto start_edges = boxes[start_box]->findChildrenOfClass<Edge>();
                            auto end_edges = boxes[end_box]->findChildrenOfClass<Edge>();
                            
                            auto new_connection = ValueTree(Identifiers::connection);
                            
                            String start_id = start_edges[inlet]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
                            
                            String end_id = end_edges[outlet + boxes[end_box]->total_in]->ValueTreeObject::getState().getProperty(Identifiers::edge_id);
                            
                            new_connection.setProperty(Identifiers::start_id, start_id, nullptr);
                            new_connection.setProperty(Identifiers::end_id, end_id, nullptr);
                            
                            canvas->getState().appendChild(new_connection, nullptr);
                            
                        }
                      }
                      
                  }
                  
              }
              catch (...)
              {
                  std::cout << "Failed to open project" << std::endl;
              }
          }
    
    });
}

void MainComponent::save_project() {
    
    auto to_save = pd.getCanvasContent();
    save_chooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, to_save](const FileChooser &f) mutable {
        File result = f.getResult();
        
        FileOutputStream ostream(result);
        ostream.writeString(to_save);
        
        get_current_canvas()->getState().setProperty("Title", result.getFileName(), nullptr);
    });
}


void MainComponent::triggerChange() {
    if(!get_current_canvas())  {
        toolbar_buttons[3].setEnabled(false);
        toolbar_buttons[4].setEnabled(false);
        return;
    }
    
    toolbar_buttons[3].setEnabled(get_current_canvas()->undo_manager.canUndo());
    toolbar_buttons[4].setEnabled(get_current_canvas()->undo_manager.canRedo());
}

Canvas* MainComponent::get_current_canvas()
{
    if(auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if(auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    return nullptr;
}

void MainComponent::add_tab(Canvas* cnv)
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
