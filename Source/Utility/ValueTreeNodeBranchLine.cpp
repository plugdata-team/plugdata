//
// Created by alexmitchell on 17/01/24.
//

#include "ValueTreeNodeBranchLine.h"
#include "ValueTreeViewer.h"

void ValueTreeNodeBranchLine::mouseUp(MouseEvent const& e)
{
    // single click to collapse directory / node
    if (e.getNumberOfClicks() == 1) {
        node->closeNode();
        auto nodePos = node->getPositionInViewport();
        auto* viewerComponent = node->findParentComponentOfClass<ValueTreeViewerComponent>();
        auto mousePosInViewport = e.getEventRelativeTo(&viewerComponent->getViewport()).getPosition().getY();

        // option to set the position of the collapsed node to top of view (not at mouse)
        //viewerComponent->getViewport().setViewPosition(0, nodePos);

        viewerComponent->getViewport().setViewPosition(0, nodePos - mousePosInViewport + (node->getHeight() * 0.5f));
        viewerComponent->repaint();
    }
}