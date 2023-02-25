#include "SplitView.h"
#include "Canvas.h"
#include "PluginEditor.h"

class SplitViewResizer : public Component
{
public:
    
    static inline constexpr int width = 6;
    std::function<void(int)> onMove = [](int){};

    SplitViewResizer()
    {
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
    
    int dragStartWidth = 0;
    bool draggingSplitview = false;
};

SplitView::SplitView(PluginEditor* parent) : editor(parent)
{
    auto* resizer = new SplitViewResizer();
    resizer->onMove = [this](int x){
        splitViewWidth = static_cast<float>(x) / getWidth();
        resized();

        if(auto* cnv = getLeftTabbar()->getCurrentCanvas()) {
            cnv->checkBounds();
        }
        if(auto* cnv = getRightTabbar()->getCurrentCanvas()) {
            cnv->checkBounds();
        }
        
        editor->updateSplitOutline();
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
            cnv->tabChanged();
            
            if(auto* splitCnv = splits[1 - i].getCurrentCanvas()) {
                splitCnv->tabChanged();
            }

            editor->updateCommandStatus();
        };
        
        tabbar.onFocusGrab = [this, &tabbar](){
            if(auto* cnv = tabbar.getCurrentCanvas()) {
                setFocus(cnv);
            }
        };
        
        tabbar.onTabMoved = [this](){
            editor->pd->savePatchTabPositions();
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


void SplitView::setSplitEnabled(bool splitEnabled)
{
    splitView = splitEnabled;
    splitFocusIndex = splitEnabled;

    splitViewResizer->setVisible(splitEnabled);
    resized();
}

bool SplitView::isSplitEnabled()
{
    return splitView;
}

void SplitView::resized()
{
    auto b = getLocalBounds();
    auto splitWidth = splitView ? splitViewWidth * getWidth() : getWidth();

    getRightTabbar()->setBounds(b.removeFromRight(getWidth() - splitWidth));
    getLeftTabbar()->setBounds(b);

    if(auto* cnv = getLeftTabbar()->getCurrentCanvas()) {
        cnv->checkBounds();
    }
    if(auto* cnv = getRightTabbar()->getCurrentCanvas()) {
        cnv->checkBounds();
    }
    
    int splitResizerWidth = SplitViewResizer::width;
    int halfSplitWidth = splitResizerWidth / 2;
    splitViewResizer->setBounds(splitWidth - halfSplitWidth, 0, splitResizerWidth, getHeight());
}

void SplitView::setFocus(Canvas* cnv)
{
    splitFocusIndex = cnv->getTabbar() == getRightTabbar();
    
    if(auto* cnv = getLeftTabbar()->getCurrentCanvas()) {
        cnv->repaint();
    }
    if(auto* cnv = getRightTabbar()->getCurrentCanvas()) {
        cnv->repaint();
    }

    editor->updateSplitOutline();
}

int SplitView::getCurrentSplitIndex()
{
    return splitFocusIndex;
}

bool SplitView::hasFocus(Canvas* cnv)
{
    if ((cnv->getTabbar() == getRightTabbar()) == splitFocusIndex)
        return true;
    else
        return false;
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

void SplitView::splitCanvasesAfterIndex(int idx, bool direction)
{
    Array<Canvas*> splitCanvases;
    
    // Two loops to make sure we don't edit the order during the first loop
    for(int i = 0; i < idx && i >= 0; i++) {
        splitCanvases.add(editor->canvases[i]);
    }
    for(auto* cnv : splitCanvases)
    {
        splitCanvasView(cnv, direction);
    }
}
void SplitView::splitCanvasView(Canvas* cnv, bool splitViewFocus)
{
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
    editor->updateSplitOutline();
    editor->pd->savePatchTabPositions();
}

TabComponent* SplitView::getActiveTabbar()
{
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
