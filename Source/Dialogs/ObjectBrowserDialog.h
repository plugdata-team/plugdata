/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
struct CategoriesListBox : public ListBox, public ListBoxModel {
    
    StringArray categories = {"All", "Audio", "More"};
    
    CategoriesListBox() {
        
        setOutlineThickness(0);
        setRowHeight(25);
        
        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);
        
    }
    
    int getNumRows() override
    {
        return categories.size();
    }
    
    void selectedRowsChanged (int row) override
    {
        changeCallback(categories[row]);
    }
    
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({4.0f, 1.0f, width - 8.0f, height - 2.0f}, Constants::defaultCornerRadius);
        }
        
        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));
        
        g.setFont(Font(15));
        
        g.drawText(categories[rowNumber], 12, 0, width - 9, height, Justification::centredLeft, true);
    }
    
    void initialise(StringArray newCategories) {
        categories = newCategories;
        updateContent();
    }
    
    std::function<void(const String&)> changeCallback;
};

struct ObjectsListBox : public ListBox, public ListBoxModel {
    
    ObjectsListBox(pd::Library& library) {
        setOutlineThickness(0);
        setRowHeight(50);
        
        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);
        
        descriptions = library.getObjectDescriptions();
    }
    
    int getNumRows() override
    {
        return objects.size();
    }
    
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto objectName = objects[rowNumber];
        auto objectDescription = descriptions[objectName];
        
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({4.0f, 1.0f, width - 8.0f, height - 2.0f}, Constants::defaultCornerRadius);
        }
        
        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));
        
        
        g.setFont(lnf->boldFont.withHeight(14));
        
        auto textBounds = Rectangle<int>(0, 0, width, height).reduced(18, 6);
        
        g.drawText(objectName, textBounds.removeFromTop(textBounds.proportionOfHeight(0.5f)), Justification::centredLeft, true);
        
        g.setFont(lnf->defaultFont.withHeight(14));
        
        g.drawText(objectDescription, textBounds, Justification::centredLeft, true);
    }
    
    void selectedRowsChanged(int row) override
    {
        changeCallback(objects[row]);
    }
    
    void showObjects(StringArray objectsToShow)
    {
        objects = objectsToShow;
        updateContent();
    }
    
    std::unordered_map<String, String> descriptions;
    StringArray objects;
    std::function<void(const String&)> changeCallback;
};

struct ObjectViewer : public Component
{
    ObjectViewer(pd::Library& objectLibrary) : library(objectLibrary) {
        addChildComponent(openHelp);
        addChildComponent(createObject);
        
        createObject.setConnectedEdges(Button::ConnectedOnBottom);
        openHelp.setConnectedEdges(Button::ConnectedOnTop);
        
        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);
    }
    
    void resized() override {
        auto buttonBounds = getLocalBounds().removeFromBottom(50).reduced(60, 0).translated(0, -30);
        createObject.setBounds(buttonBounds.removeFromTop(25));
        openHelp.setBounds(buttonBounds.translated(0, -1));
    }
    
    void paint(Graphics& g) override
    {
        const auto font = Font(15.0f);
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(5, 10, 5, getHeight() - 20);
        
        if(objectName.isEmpty()) return;
        
        auto infoBounds = getLocalBounds().withTrimmedBottom(100).reduced(20);
        auto objectDisplayBounds = infoBounds.removeFromTop(300).reduced(60);
        
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if(!lnf) return;
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(16.0f));
        g.drawText(objectName, getLocalBounds().removeFromTop(100).translated(0, 20), Justification::centred);
        

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText("Category:", infoBounds.removeFromTop(25), Justification::left);
        g.drawText("Description:", infoBounds.removeFromTop(25), Justification::left);
        

        if(unknownLayout) {
            return;
        }
        
        const int ioletSize = 8;
        
        
        const int ioletWidth = (ioletSize + 5) * std::max(inlets.size(), outlets.size());
        const int textWidth = font.getStringWidth(objectName);
        const int width = std::max(ioletWidth, textWidth) + 14;
        
        auto outlineBounds = objectDisplayBounds.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, Constants::objectCornerRadius, 1.0f);
        
        auto textBounds = outlineBounds.reduced(2.0f);
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText(objectName, textBounds, Justification::centred);
        
       
        
        auto ioletBounds = outlineBounds.reduced(10, 0);
        
        for(int i = 0; i < inlets.size(); i++) {
            auto inletBounds = Rectangle<int>();
            
            const int total = inlets.size();
            const float yPosition = ioletBounds.getY() + 1 - ioletSize / 2.0f;
            
            if (total == 1 && i == 0)
            {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();
                
                inletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);
            }
            else if (total > 1)
            {
                const float ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
                
                inletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }
            g.setColour(inlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(inletBounds.toFloat());
            
            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(inletBounds.toFloat(), 1.0f);
        }
        
        for(int i = 0; i < outlets.size(); i++) {
            
            auto outletBounds = Rectangle<int>();
            const int total = outlets.size();
            const float  yPosition = ioletBounds.getBottom() - ioletSize / 2.0f;
            
            if (total == 1 && i == 0)
            {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();
                
                outletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);
                
            }
            else if (total > 1)
            {
                const float ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
                outletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }
            
            g.setColour(outlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(outletBounds.toFloat());
            
            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(outletBounds.toFloat(), 1.0f);
        }

    }
    
    void showObject(String name)
    {
        bool valid = name.isNotEmpty();
        createObject.setVisible(valid);
        openHelp.setVisible(valid);
        
        if(!valid)  {
            objectName = "";
            unknownLayout = false;
            inlets.clear();
            outlets.clear();
            repaint();
            return;
        }
        
        auto inletDescriptions = library.getInletDescriptions()[name];
        auto outletDescriptions = library.getOutletDescriptions()[name];
                
        inlets.resize(inletDescriptions.size());
        outlets.resize(outletDescriptions.size());
        
        bool hasUnknownLayout = false;
        
        for(int i = 0; i < inlets.size(); i++) {
            inlets[i] = inletDescriptions[i].first.contains("(signal)");
            if(inletDescriptions[i].second) hasUnknownLayout = true;
        }
        for(int i = 0; i < outlets.size(); i++) {
            outlets[i] = outletDescriptions[i].first.contains("(signal)");
            if(outletDescriptions[i].second) hasUnknownLayout = true;
        }
        
        unknownLayout = hasUnknownLayout;
        objectName = name;
        
        repaint();
    }
    
    bool unknownLayout = false;
    
    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;
    
    
    TextButton openHelp = TextButton("Show Help");
    TextButton createObject = TextButton("Create Object");
    
    pd::Library& library;
};

struct ObjectBrowserDialog : public Component {
    ObjectBrowserDialog(Component* pluginEditor, Dialog* parent) :
    editor(dynamic_cast<PluginEditor*>(pluginEditor)),
    objectsList(editor->pd->objectLibrary),
    objectViewer(editor->pd->objectLibrary)
    {
        objectsByCategory = editor->pd->objectLibrary.getObjectCategories();
        
        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectsList);
        addAndMakeVisible(objectViewer);
        
        objectsByCategory["All"] = StringArray();
        
        StringArray categories;
        for(auto [category, objects] : objectsByCategory)
        {
            objects.sort(true);
            objectsByCategory["All"].addArray(objects);
            categories.add(category);
        }
        
        objectsByCategory["All"].sort(true);
        categories.sort(true);
        categories.move(categories.indexOf("All"), 0);
        
        categoriesList.changeCallback = [this](const String& category){
            objectsList.showObjects(objectsByCategory[category]);
        };
        
        objectsList.changeCallback = [this](const String& object){
            objectViewer.showObject(object);
        };
        
        categoriesList.initialise(categories);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        categoriesList.setBounds(b.removeFromLeft(150));
        objectsList.setBounds(b.removeFromLeft(300));
        objectViewer.setBounds(b);
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Constants::windowCornerRadius);
    }
    
private:
    
    PluginEditor* editor;
    CategoriesListBox categoriesList;
    ObjectsListBox objectsList;
    
    ObjectViewer objectViewer;
    
    std::unordered_map<String, StringArray> objectsByCategory;
    std::unordered_map<String, String> objectDescriptions;

};
