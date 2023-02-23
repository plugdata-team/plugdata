#include "SplitView.h"
#include "Canvas.h"
#include "PluginEditor.h"

class SplitViewResizer : public Component
{
    
public:
    
    std::function<void(int)> onMove = [](int){};
    
    SplitViewResizer() {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
        setAlwaysOnTop(true);
    }
    
private:
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(getX() + width / 2, getY(), getX() + width / 2, getBottom());
    }

    void mouseDown(const MouseEvent& e) override
    {
        dragStartWidth = getX();
    }

    void mouseDrag(const MouseEvent& e) override
    {
        
        int newX = std::clamp<int>(dragStartWidth + e.getDistanceFromDragStartX(), getParentComponent()->getWidth() * 0.25f, getParentComponent()->getWidth() * 0.75f);
        setTopLeftPosition(newX, 0);
        onMove(newX + width / 2);
    }

    void parentSizeChanged() override
    {
        auto parentBounds = getParentComponent()->getBounds();
        int newX = parentBounds.getWidth() / 2;
        
        if(!isVisible()) setTopLeftPosition(newX - width / 2, 0);
        setSize(width, parentBounds.getHeight());
        onMove(newX);
    }
    
    int dragStartWidth = 0;
    bool draggingSplitview = false;
    const int width = 6;
};

SplitView::SplitView(PluginEditor* parent) : editor(parent)
{
    auto* resizer = new SplitViewResizer();
    resizer->onMove = [this](int x){
        splitViewWidth = x;
        resized();
        
        if(auto* cnv = getLeftTabbar()->getCurrentCanvas()) {
            cnv->checkBounds();
        }
        if(auto* cnv = getRightTabbar()->getCurrentCanvas()) {
            cnv->checkBounds();
        }
    };
    addChildComponent(resizer);
    
    splitViewResizer.reset(resizer);
    
    int i = 0;
    for(auto& tabbar : splits) {
                
        tabbar.newTab = [this, i]() {
            splitFocusIndex = i;
            editor->newProject();
        };
        
        tabbar.openProject = [this, i]() {
            splitFocusIndex = i;
            editor->openProject();
        };
        

        tabbar.onTabChange = [this, i, &tabbar](int idx) {
            splitFocusIndex = i;
            auto* cnv = tabbar.getCurrentCanvas();
            
            if (!cnv || idx == -1 || editor->pd->isPerformingGlobalSync)
                return;
            
            editor->sidebar.tabChanged();
            
            // update GraphOnParent when changing tabs
            for (auto* object : cnv->objects) {
                if (!object->gui)
                    continue;
                if (auto* graphCnv = object->gui->getCanvas())
                    graphCnv->synchronise();
            }
            
            if (cnv->patch.getPointer()) {
                cnv->patch.setCurrent();
            }
            
            cnv->synchronise();
            cnv->tabChanged();
            cnv->updateDrawables();
            
            if(auto* splitCnv = splits[1 - i].getCurrentCanvas()) {
                
                for (auto* object : splitCnv->objects) {
                    if (!object->gui)
                        continue;
                    if (auto* graphCnv = object->gui->getCanvas())
                        graphCnv->synchronise();
                }
                
                splitCnv->synchronise();
                splitCnv->tabChanged();
            }
  
            editor->updateCommandStatus();
        };
        
        tabbar.rightClick = [this, &tabbar, i](int tabIndex, String const& tabName) {
            PopupMenu tabMenu;
            
            bool enabled = true;
            if(i == 0 && !splitView) enabled = getLeftTabbar()->getNumTabs() > 1;
            tabMenu.addItem(i == 0 ? "Split Right" : "Split Left", enabled, false, [this, tabIndex, &tabbar, i]() {
                
                if (auto* cnv = tabbar.getCanvas(tabIndex)) {
                    splitCanvasView(cnv, i == 0);
                }
                
                closeEmptySplits();
            });
            // Show the popup menu at the mouse position
            tabMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withParentComponent(editor->pd->getActiveEditor()));
        };
        
        tabbar.setOutline(0);
        addAndMakeVisible(tabbar);
        i++;
    }
}


void SplitView::setSplitEnabled(bool splitEnabled) {
    
    splitView = splitEnabled;
    splitFocusIndex = splitEnabled;
    
    splitViewResizer->setVisible(splitEnabled);
    resized();
}

void SplitView::resized() {
    auto b = getLocalBounds();
    auto splitWidth = splitView ? splitViewWidth : getWidth();

    getRightTabbar()->setBounds(b.removeFromRight(getWidth() - splitWidth));
    getLeftTabbar()->setBounds(b);
    
    if(auto* cnv = getLeftTabbar()->getCurrentCanvas()) {
        cnv->checkBounds();
    }
    if(auto* cnv = getRightTabbar()->getCurrentCanvas()) {
        cnv->checkBounds();
    }
}

void SplitView::setFocus(Canvas* cnv) {
    splitFocusIndex = cnv->getTabbar() == getRightTabbar();
}

void SplitView::closeEmptySplits()
{
    if (!splits[1].getNumTabs()) {
        // Disable splitview if all splitview tabs are closed
        setSplitEnabled(false);
    }
    if (splitView && !splits[0].getNumTabs()) {
        
        // move all tabs over to the other side
        for(int i = splits[1].getNumTabs() - 1; i >= 0; i--)
        {
            splitCanvasView(splits[1].getCanvas(i), i);
        }
        
        setSplitEnabled(false);
    }
}

void SplitView::splitCanvasView(Canvas* cnv, bool splitViewFocus) {
        
    auto* patch = &cnv->patch;
    auto* editor = cnv->editor;
    bool deleteOnClose = cnv->closePatchAlongWithCanvas;
    bool locked = static_cast<bool>(cnv->locked.getValue());
    
    editor->closeTab(cnv, true);
    
    // Closing the tab deletes the canvas, so we clone it
    auto* canvasCopy = new Canvas(editor, *patch, deleteOnClose, nullptr);
    canvasCopy->locked = locked;
    
    setSplitEnabled(true);
    splitFocusIndex = splitViewFocus;
    editor->addTab(canvasCopy);
    editor->canvases.add(canvasCopy);
}

TabComponent* SplitView::getActiveTabbar() {
    return &splits[splitFocusIndex];
}

TabComponent* SplitView::getLeftTabbar()
{
    return &splits[0];
}

TabComponent* SplitView::getRightTabbar()
{
    return &splits[1];
}
