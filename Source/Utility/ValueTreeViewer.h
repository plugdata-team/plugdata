

class ValueTreeViewerComponent;
class ValueTreeNodeComponent;
struct ValueTreeOwnerView : public Component
{
    SafePointer<ValueTreeNodeComponent> selectedNode;
    
    std::function<void()> updateView = [](){};
    std::function<void(ValueTree&)> onClick =   [](ValueTree& tree){};
    std::function<void(ValueTree&)> onDragStart = [](ValueTree& tree){};
};

class ValueTreeNodeComponent : public Component
{
public:
    ValueTreeNodeComponent(const ValueTree& node) : valueTreeNode(node)
    {
        // Create subcomponents for each child node
        for (int i = 0; i < valueTreeNode.getNumChildren(); ++i)
        {
            auto* childComponent = nodes.add(new ValueTreeNodeComponent(valueTreeNode.getChild(i)));
            addAndMakeVisible(childComponent);
        }
    }
    
    void update()
    {
        // Compare existing child nodes with current children
        for (int i = 0; i < valueTreeNode.getNumChildren(); ++i)
        {
            ValueTree childNode = valueTreeNode.getChild(i);
            
            // Check if an existing node exists for this child
            ValueTreeNodeComponent* existingNode = nullptr;
            for (auto* node : nodes)
            {
                if (ValueTreeNodeComponent::compareProperties(childNode, node->valueTreeNode))
                {
                    existingNode = node;
                    node->valueTreeNode = childNode;
                    break;
                }
            }
            
            // If an existing node is found, update it; otherwise, create a new node
            if (existingNode != nullptr)
            {
                existingNode->update();
            }
            else
            {
                auto* childComponent = new ValueTreeNodeComponent(childNode);
                nodes.add(childComponent);
                addAndMakeVisible(childComponent);
            }
        }
        
        // Remove any existing nodes that no longer exist in the current children
        for (int i = nodes.size(); --i >= 0;)
        {
            if(!nodes.getUnchecked(i)->valueTreeNode.isAChildOf(valueTreeNode))
            {
                nodes.remove(i);
            }
        }
    }
    
    void paintOpenCloseButton(Graphics& g, Rectangle<float> const& area)
    {
        auto arrowArea = area.reduced(5, 9).translated(4, 0).toFloat();
        Path p;
        
        p.startNewSubPath(0.0f, 0.0f);
        p.lineTo(0.5f, 0.5f);
        p.lineTo(isOpen() ? 1.0f : 0.0f, isOpen() ? 0.0f : 1.0f);
        
        g.setColour(isSelected() ? findColour(PlugDataColour::sidebarActiveTextColourId) : getOwnerView()->findColour(PlugDataColour::sidebarTextColourId));
        g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(arrowArea, true));
    }
    
    bool isSelected()
    {
        return getOwnerView()->selectedNode.getComponent() == this;
    }
    
    ValueTreeOwnerView* getOwnerView()
    {
        return findParentComponentOfClass<ValueTreeOwnerView>();
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if(!isDragging && e.getDistanceFromDragStart() > 10)
        {
            isDragging = true;
            getOwnerView()->onDragStart(valueTreeNode);
        }
    }
    
    bool isOpen() const
    {
        return isOpened || isOpenedBySearch;
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        isDragging = false;
        
        if (e.eventComponent == this && e.mods.isLeftButtonDown())
        {
            if(nodes.size() && e.x < 22) {
                isOpened = !isOpened;
                for (auto* child : nodes)
                    child->setVisible(isOpen());
                
                getOwnerView()->updateView();
                resized();
            }
            else {
                getOwnerView()->selectedNode = this;
                getOwnerView()->repaint();
                if(e.getNumberOfClicks() == 2)
                {
                    getOwnerView()->onClick(valueTreeNode);
                }
            }
        }
    }
    
    void paint(Graphics& g) override
    {
        if(getOwnerView()->selectedNode.getComponent() == this) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().withHeight(25).reduced(2).toFloat(), Corners::defaultCornerRadius);
        }
        
        auto hasChildren = !nodes.isEmpty();
        auto itemBounds = getLocalBounds().removeFromTop(25);
        auto arrowBounds = itemBounds.removeFromLeft(20).toFloat().reduced(1.0f);
        if (isOpen()) {
            arrowBounds = arrowBounds.reduced(1.0f);
        }
        
        if(hasChildren)
        {
            paintOpenCloseButton(g, arrowBounds);
        }
        
        auto& owner = *getOwnerView();
        auto colour = isSelected() ? owner.findColour(PlugDataColour::sidebarActiveTextColourId) : owner.findColour(PlugDataColour::sidebarTextColourId);
        auto hasIcon = valueTreeNode.hasProperty("Icon");
        if(hasIcon)
        {
            Fonts::drawIcon(g, valueTreeNode.getProperty("Icon"), itemBounds.removeFromLeft(22).reduced(2), colour, 12, false);
        }
        
        Fonts::drawFittedText(g, valueTreeNode.getProperty("Name"), itemBounds, colour);
    }
    
    void resized() override
    {
        // Set the bounds of the subcomponents within the current component
        if(isOpen()) {
            auto bounds = getLocalBounds().reduced(8, 1).withTrimmedTop(25);
            
            for (auto* node : nodes)
            {
                if(node->isVisible()) {
                    auto childBounds = bounds.removeFromTop(node->getTotalContentHeight());
                    node->setBounds(childBounds);
                }
            }
        }
    }
    
    int getTotalContentHeight() const
    {
        int totalHeight = isVisible() ? 25 : 0;
        
        if(isOpen()) {
            for (auto* node : nodes)
            {
                totalHeight += node->isVisible() ? node->getTotalContentHeight() : 0;
            }
        }
        
        return totalHeight;
    }
    
    static bool compareProperties(ValueTree oldTree, ValueTree newTree)
    {
        for(int i = 0; i < oldTree.getNumProperties(); i++)
        {
            auto name = oldTree.getPropertyName(i);
            if(!newTree.hasProperty(name) || newTree.getProperty(name) != oldTree.getProperty(name)) return false;
        }
        
        return true;
    }
    
private:
    OwnedArray<ValueTreeNodeComponent> nodes;
    ValueTree valueTreeNode;
    bool isOpened = false;
    bool isOpenedBySearch = false;
    bool isDragging = false;
    friend class ValueTreeViewerComponent;
};

class ValueTreeViewerComponent : public Component
{
public:
    ValueTreeViewerComponent()
    {
        // Add a Viewport to handle scrolling
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, false, false);
        
        contentComponent.setVisible(true);
        contentComponent.updateView = [this](){
            resized();
        };
        
        contentComponent.onClick =   [this](ValueTree& tree){ onClick(tree); };
        contentComponent.onDragStart = [this](ValueTree& tree){ onDragStart(tree); };
        
        addAndMakeVisible(viewport);
    }
    
    void setValueTree(const ValueTree& tree)
    {
        valueTree = tree;
        auto originalViewPos = viewport.getViewPosition();
        
        // Compare existing child nodes with current children
        for (int i = 0; i < valueTree.getNumChildren(); ++i)
        {
            ValueTree childNode = valueTree.getChild(i);
  
            // Check if an existing node exists for this child
            ValueTreeNodeComponent* existingNode = nullptr;
            for (auto* node : nodes)
            {
                if (childNode.isValid() && ValueTreeNodeComponent::compareProperties(childNode, node->valueTreeNode))
                {
                    existingNode = node;
                    node->valueTreeNode = childNode;
                    break;
                }
            }
            
            // If an existing node is found, update it; otherwise, create a new node
            if (existingNode != nullptr)
            {
                existingNode->update();
            }
            else if(childNode.isValid())
            {
                auto* childComponent = new ValueTreeNodeComponent(childNode);
                nodes.add(childComponent);
                contentComponent.addAndMakeVisible(childComponent);
            }
        }
        
        // Remove any existing nodes that no longer exist in the current children
        for (int i = nodes.size(); --i >= 0;)
        {
            if(!nodes.getUnchecked(i)->valueTreeNode.isAChildOf(valueTree))
            {
                nodes.remove(i);
            }
        }

        sortNodes(nodes);
        
        contentComponent.resized();
        resized();
        
        viewport.setViewPosition(originalViewPos);
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(PlugDataColour::sidebarBackgroundColourId));
    }
    
    int getTotalContentHeight() const
    {
        int totalHeight = 0;
        for (auto* node : nodes)
        {
            totalHeight += node->isVisible() ? node->getTotalContentHeight() : 0;
        }
        return totalHeight;
    }
    
    void resized() override
    {
        // Set the bounds of the Viewport within the main component
        auto bounds = getLocalBounds();
        viewport.setBounds(bounds);
        
        contentComponent.setBounds(0, 0, bounds.getWidth(), std::max(getTotalContentHeight(), bounds.getHeight()));
        
        auto scrollbarIndent = viewport.canScrollVertically() ? 8 : 0;
        bounds = bounds.reduced(2, 2).withTrimmedRight(scrollbarIndent).withHeight(getTotalContentHeight());

        for (auto* node : nodes)
        {
            if(node->isVisible()) {
                auto childBounds = bounds.removeFromTop(node->getTotalContentHeight());
                node->setBounds(childBounds);
            }
        }
    }
    
    void setFilterString(const String& toFilter)
    {
        filterString = toFilter;
        
        if(filterString.isEmpty())
        {
            for (auto* topLevelNode : nodes)
            {
                clearSearch(topLevelNode);
            }
            
            resized();
            return;
        }
        

        for (auto* topLevelNode : nodes)
        {
           searchInNode(topLevelNode);
        }

        resized();
    }
    
    std::function<void(ValueTree&)> onClick =   [](ValueTree& tree){};
    std::function<void(ValueTree&)> onDragStart = [](ValueTree& tree){};
    
private:
    
    bool searchInNode(ValueTreeNodeComponent* node)
    {
        // Check if the current node matches the filterString
        bool found = filterString.isEmpty() || node->valueTreeNode.getProperty("Name").toString().containsIgnoreCase(filterString);
        
        for (auto* child : node->nodes)
        {
            found = searchInNode(child) || found;
        }
        
        node->isOpenedBySearch = !node->nodes.isEmpty() && found;
        
        // Set the visibility of the node based on whether it matches the filterString
        node->setVisible(found);

        return found;
    }
    
    void clearSearch(ValueTreeNodeComponent* node)
    {
        node->setVisible(true);
        node->isOpenedBySearch = false;
        for (auto* child : node->nodes)
        {
            clearSearch(child);
        }
    }
    
    static void sortNodes(OwnedArray<ValueTreeNodeComponent>& nodes)
    {
        struct {
            int compareElements (const ValueTreeNodeComponent* a, const ValueTreeNodeComponent* b)
            {
                return a->valueTreeNode.getParent().indexOf(a->valueTreeNode) < b->valueTreeNode.getParent().indexOf(b->valueTreeNode);
            }
        } elementSorter;
        
        nodes.sort(elementSorter, false);
        
        for(auto* childNode : nodes)
        {
            sortNodes(childNode->nodes);
        }
    }
    
    String filterString;
    ValueTreeOwnerView contentComponent;
    OwnedArray<ValueTreeNodeComponent> nodes;
    ValueTree valueTree = ValueTree("Folder");
    BouncingViewport viewport;
};
