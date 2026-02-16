/*
// Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Fonts.h"
#include "Components/BouncingViewport.h"

class ValueTreeViewerComponent;
class ValueTreeNodeComponent;
struct ValueTreeOwnerView final : public Component {
    SafePointer<ValueTreeNodeComponent> selectedNode;

    std::function<void()> updateView = [] { };
    std::function<void(ValueTree&)> onReturn = [](ValueTree&) { };
    std::function<void(ValueTree&)> onClick = [](ValueTree&) { };
    std::function<void(ValueTree&)> onSelect = [](ValueTree&) { };
    std::function<void(ValueTree&)> onDragStart = [](ValueTree&) { };
    std::function<void(ValueTree&)> onRightClick = [](ValueTree&) { };
};

class ValueTreeNodeComponent final : public Component {
    class ValueTreeNodeBranchLine final : public Component
        , public SettableTooltipClient {
    public:
        explicit ValueTreeNodeBranchLine(ValueTreeNodeComponent* node)
            : node(node)
        {
        }

        void paint(Graphics& g) override
        {
            if (!treeLine.isEmpty()) {
                auto const colour = isHover && !node->isOpenInSearchMode() ? findColour(PlugDataColour::objectSelectedOutlineColourId) : findColour(PlugDataColour::panelTextColourId).withAlpha(0.25f);

                g.reduceClipRegion(treeLineImage, AffineTransform());
                g.fillAll(colour);
            }
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isHover = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isHover = false;
            repaint();
        }

        void mouseMove(MouseEvent const& e) override
        {
            if (!isHover) {
                isHover = true;
                repaint();
            }
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;
            
            // double click to collapse directory / node
            if (e.getNumberOfClicks() == 2) {
                node->toggleNodeOpenClosed();
                auto const nodePos = node->getPositionInViewport();
                auto* viewport = node->findParentComponentOfClass<Viewport>();
                auto const mousePosInViewport = e.getEventRelativeTo(viewport).getPosition().getY();

                viewport->setViewPosition(0, nodePos - mousePosInViewport + node->getHeight() * 0.5f);
                viewport->repaint();
            }
        }

        void resized() override
        {
            treeLine.clear();

            if (getParentComponent()->isVisible()) {
                // create a line to show the current branch
                auto const b = getLocalBounds();
                auto const lineEnd = Point<float>(4.0f, b.getHeight() - 3.0f);
                treeLine.startNewSubPath(4.0f, 0.0f);
                treeLine.lineTo(lineEnd);

                if (!b.isEmpty()) {
                    treeLineImage = Image(Image::PixelFormat::ARGB, b.getWidth(), b.getHeight(), true);
                    Graphics treeLineG(treeLineImage);
                    treeLineG.setColour(Colours::white);
                    treeLineG.strokePath(treeLine, PathStrokeType(1.0f));
                    auto const ballEnd = Rectangle<float>(0, 0, 5, 5).withCentre(lineEnd);
                    treeLineG.fillEllipse(ballEnd);
                }
            }
        }

    private:
        ValueTreeNodeComponent* node;
        Path treeLine;
        Image treeLineImage;
        bool isHover = false;
    };

public:
    ValueTreeNodeComponent(ValueTree const& node, ValueTreeNodeComponent* parentNode, String const& prepend = String())
        : valueTreeNode(node)
        , parent(parentNode)
    {
        nodeBranchLine = std::make_unique<ValueTreeNodeBranchLine>(this);
        addAndMakeVisible(nodeBranchLine.get());
        nodeBranchLine->setAlwaysOnTop(true);

        if (valueTreeNode.hasProperty("Name")) {
            auto tooltipPrepend = prepend;
            if (tooltipPrepend.isEmpty())
                tooltipPrepend = "(Parent)";
            nodeBranchLine->setTooltip(tooltipPrepend + " " + valueTreeNode.getProperty("Name").toString());
        }

        // Create subcomponents for each child node
        for (int i = 0; i < valueTreeNode.getNumChildren(); ++i) {
            auto* childComponent = nodes.add(new ValueTreeNodeComponent(valueTreeNode.getChild(i), this, prepend));
            addChildComponent(childComponent);
        }
    }

    void update()
    {
        // Compare existing child nodes with current children
        for (auto const& childNode : valueTreeNode) {
            // Check if an existing node exists for this child
            ValueTreeNodeComponent* existingNode = nullptr;
            for (auto* node : nodes) {
                if (ValueTreeNodeComponent::compareProperties(childNode, node->valueTreeNode)) {
                    existingNode = node;
                    node->valueTreeNode = childNode;
                    break;
                }
            }

            // If an existing node is found, update it; otherwise, create a new node
            if (existingNode != nullptr) {
                existingNode->update();
            } else {
                auto* childComponent = new ValueTreeNodeComponent(childNode, this);
                nodes.add(childComponent);
                addAndMakeVisible(childComponent);
            }
        }

        // Remove any existing nodes that no longer exist in the current children
        for (int i = nodes.size(); --i >= 0;) {
            if (!nodes.getUnchecked(i)->valueTreeNode.isAChildOf(valueTreeNode)) {
                nodes.remove(i);
            }
        }
    }

    void paintOpenCloseButton(Graphics& g, Rectangle<float> const& area) const
    {
        auto const arrowArea = area.reduced(5, 9).translated(4, 0).toFloat();
        Path p;

        p.startNewSubPath(0.0f, 0.0f);
        p.lineTo(0.5f, 0.5f);
        p.lineTo(isOpen() ? 1.0f : 0.0f, isOpen() ? 0.0f : 1.0f);

        g.setColour(getOwnerView()->findColour(PlugDataColour::sidebarTextColourId));
        g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(arrowArea, true));
    }

    bool isSelected() const
    {
        if (auto const node = getOwnerView()->selectedNode)
            return node.getComponent() == this;

        return false;
    }

    ValueTreeOwnerView* getOwnerView() const
    {
        return findParentComponentOfClass<ValueTreeOwnerView>();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!isDragging && e.getDistanceFromDragStart() > 10) {
            isDragging = true;
            getOwnerView()->onDragStart(valueTreeNode);
        }
    }

    bool isOpen() const
    {
        return isOpened || isOpenedBySearch;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.eventComponent == this && e.mods.isRightButtonDown())
            getOwnerView()->onRightClick(valueTreeNode);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;
        
        isDragging = false;

        if (e.eventComponent == this && e.mods.isLeftButtonDown() && getScreenBounds().contains(e.getScreenPosition())) {
            if (nodes.size() && e.x < 22) {
                toggleNodeOpenClosed();
            } else {
                getOwnerView()->selectedNode = this;
                getOwnerView()->repaint();

                if (e.getNumberOfClicks() == 1) {
                    getOwnerView()->onSelect(valueTreeNode);
                } else {
                    getOwnerView()->onClick(valueTreeNode);
                }
            }
        }
    }

    void makeActive()
    {
        getOwnerView()->selectedNode = this;
        getOwnerView()->repaint();
        getOwnerView()->grabKeyboardFocus();
    }

    void toggleNodeOpenClosed()
    {
        isOpened = !isOpened;
        for (auto* child : nodes) {
            child->setVisible(isOpen());
        }

        getOwnerView()->updateView();
        resized();
    }

    void paint(Graphics& g) override
    {
        auto font = Font(FontOptions(15));
        // Either show single selection or multi-selection
        if (getOwnerView()->selectedNode ? isSelected() : valueTreeNode.getProperty("Selected") == var(true)) {
            auto const highlightCol = findColour(PlugDataColour::sidebarActiveBackgroundColourId);
            g.setColour(isSelected() ? highlightCol.brighter(0.2f) : highlightCol);
            g.fillRoundedRectangle(getLocalBounds().withHeight(25).reduced(2).toFloat(), Corners::defaultCornerRadius);
        }

        auto hasChildren = !nodes.isEmpty();
        auto itemBounds = getLocalBounds().removeFromTop(25);
        auto arrowBounds = itemBounds.removeFromLeft(20).toFloat().reduced(1.0f);
        if (isOpen()) {
            arrowBounds = arrowBounds.reduced(1.0f);
        }

        if (hasChildren) {
            paintOpenCloseButton(g, arrowBounds);
        }

        auto colour = getOwnerView()->findColour(PlugDataColour::sidebarTextColourId);

        if (valueTreeNode.hasProperty("Icon")) {
            auto iconColour = colour;
            if (valueTreeNode.hasProperty("IconColour"))
                iconColour = Colour::fromString(valueTreeNode.getProperty("IconColour").toString());
            Fonts::drawIcon(g, valueTreeNode.getProperty("Icon"), itemBounds.removeFromLeft(22).reduced(2), iconColour, 12, false);
        }

        // Don't allow multi-line for viewer items, so remove all newlines in the string
        auto nameText = valueTreeNode.getProperty("Name").toString().replaceCharacters("\n", " ");
        auto nameLength = Fonts::getStringWidth(nameText, font);
        Fonts::drawFittedText(g, nameText, itemBounds.removeFromLeft(nameLength), colour);

        constexpr auto tagCornerRadius = Corners::defaultCornerRadius * 0.7f;

#ifdef SHOW_PD_SUBPATCH_SYMBOL
        if (valueTreeNode.hasProperty("PDSymbol")) {
            // draw generic symbol label tag
            //  ╭──────────╮
            //  │  symbol  │
            //  ╰──────────╯
            auto sendSymbolText = valueTreeNode.getProperty("PDSymbol").toString();
            auto length = Fonts.getStringWidthInt(sendSymbolText, 15);
            auto sendColour = findColour(PlugDataColour::objectSelectedOutlineColourId).withRotatedHue(0.25);
            g.setColour(sendColour.withAlpha(0.2f));
            auto tagBounds = itemBounds.removeFromLeft(length).translated(4, 0).reduced(0, 5).expanded(2, 0);
            g.fillRoundedRectangle(tagBounds.toFloat(), Corners::defaultCornerRadius * 0.8f);
            Fonts::drawFittedText(g, sendSymbolText, tagBounds.translated(2, 0), sendColour);
            itemBounds.translate(8, 0);
        } else
#endif
        {
            // draw receive symbol label tag
            //  a──b───────────╮
            //   \ │           │
            //     c   symbol  │
            //   / │           │
            //  e──d───────────╯
            if (valueTreeNode.hasProperty("ReceiveSymbol")) {
                auto receiveSymbolText = (valueTreeNode.hasProperty("ReceiveObject") ? "" : "r: ") + valueTreeNode.getProperty("ReceiveSymbol").toString();
                auto length = Fonts::getStringWidth(receiveSymbolText, 15);
                auto recColour = findColour(PlugDataColour::objectSelectedOutlineColourId);
                g.setColour(recColour.withAlpha(0.2f));
                auto tagBounds = itemBounds.removeFromLeft(length).translated(4, 0).reduced(0, 5).expanded(2, 0).toFloat();
                Path flag;
                auto const a = tagBounds.getTopLeft().toFloat();
                auto const b = Point<float>(tagBounds.getX() + tagBounds.getHeight() * 0.5f, tagBounds.getY());
                auto const c = Point<float>(tagBounds.getX() + tagBounds.getHeight() * 0.5f, tagBounds.getCentreY());
                auto const d = Point<float>(tagBounds.getX() + tagBounds.getHeight() * 0.5f, tagBounds.getBottom());
                auto const e = tagBounds.getBottomLeft().toFloat();
                flag.startNewSubPath(a);
                flag.lineTo(b);
                flag.lineTo(c);
                flag.closeSubPath();
                flag.startNewSubPath(c);
                flag.lineTo(d);
                flag.lineTo(e);
                flag.closeSubPath();
                flag.addRoundedRectangle(b.getX(), b.getY(), tagBounds.getWidth(), tagBounds.getHeight(), tagCornerRadius, tagCornerRadius, false, true, false, true);
                g.fillPath(flag);
                Fonts::drawFittedText(g, receiveSymbolText, tagBounds.translated(2 + tagBounds.getHeight() * 0.5f, 0).toNearestIntEdges(), recColour);
                itemBounds.translate(16, 0);
            }
            // draw send symbol label tag
            //  ╭────────── a
            //  │          │ \
            //  │  symbol  │  b
            //  │          │ /
            //  ╰────────── c
            if (valueTreeNode.hasProperty("SendSymbol")) {
                auto sendSymbolText = (valueTreeNode.hasProperty("SendObject") ? "" : "s: ") + valueTreeNode.getProperty("SendSymbol").toString();
                auto length = Fonts::getStringWidth(sendSymbolText, font);
                auto sendColour = findColour(PlugDataColour::objectSelectedOutlineColourId).withRotatedHue(0.5f);
                g.setColour(sendColour.withAlpha(0.2f));
                auto tagBounds = itemBounds.removeFromLeft(length).translated(4, 0).reduced(0, 5).expanded(2, 0).toFloat();
                Path flag;
                auto const a = tagBounds.getTopRight().toFloat();
                auto const b = Point<float>(tagBounds.getRight() + tagBounds.getHeight() * 0.5f, tagBounds.getCentreY());
                auto const c = tagBounds.getBottomRight().toFloat();
                flag.startNewSubPath(a);
                flag.lineTo(b);
                flag.lineTo(c);
                flag.closeSubPath();
                flag.addRoundedRectangle(tagBounds.getX(), tagBounds.getY(), tagBounds.getWidth(), tagBounds.getHeight(), tagCornerRadius, tagCornerRadius, true, false, true, false);
                g.fillPath(flag);
                Fonts::drawFittedText(g, sendSymbolText, tagBounds.translated(2, 0).toNearestIntEdges(), sendColour);
            }
        }

        int rightOffset = 0;
        auto rightBounds = getLocalBounds().removeFromTop(25);

        if (showIndex && valueTreeNode.hasProperty("Index")) {
            rightOffset += 4;
            auto rightText = valueTreeNode.getProperty("Index").toString();
            if (itemBounds.getWidth() - Fonts::getStringWidth(rightText, font) >= rightOffset) {
                Fonts::drawFittedText(g, rightText, rightBounds.removeFromRight(Fonts::getStringWidth(rightText, font) + 4), colour.withAlpha(0.5f), Justification::topLeft);
            }
        }

        if (showXYpos && valueTreeNode.hasProperty("RightText")) {
            rightOffset += 8;
            auto rightText = valueTreeNode.getProperty("RightText").toString();
            if (itemBounds.getWidth() - Fonts::getStringWidth(rightText, font) >= rightOffset) {
                Fonts::drawFittedText(g, rightText, rightBounds.removeFromRight(Fonts::getStringWidth(rightText, font) + 4), colour.withAlpha(0.5f), Justification::topLeft);
            }
        }
    }

    void resized() override
    {
        // Set the bounds of the subcomponents within the current component
        if (isOpen()) {
            nodeBranchLine->setVisible(true);
            nodeBranchLine->setBounds(getLocalBounds().withLeft(10).withRight(18).withTrimmedBottom(10).withTop(20));

            auto bounds = getLocalBounds().withTrimmedLeft(8).withTrimmedTop(25);

            for (auto* node : nodes) {
                if (node->isVisible()) {
                    auto const childBounds = bounds.removeFromTop(node->getTotalContentHeight());
                    node->setBounds(childBounds);
                }
            }
        } else {
            nodeBranchLine->setVisible(false);
        }
    }

    int getTotalContentHeight() const
    {
        int totalHeight = isVisible() ? 25 : 0;

        if (isOpen()) {
            for (auto const* node : nodes) {
                totalHeight += node->isVisible() ? node->getTotalContentHeight() : 0;
            }
        }

        return totalHeight;
    }

    static bool compareProperties(ValueTree const& oldTree, ValueTree const& newTree)
    {
        for (int i = 0; i < oldTree.getNumProperties(); i++) {
            auto name = oldTree.getPropertyName(i);
            if (!newTree.hasProperty(name) || newTree.getProperty(name) != oldTree.getProperty(name))
                return false;
        }

        return true;
    }

    int getPositionInViewport() const
    {
        auto const* node = this;
        int posInParent = 0;
        while (node) {
            posInParent += node->getPosition().getY();
            node = node->parent;
        }
        return posInParent;
    }

    ValueTree valueTreeNode;

    bool isOpenInSearchMode() const { return isOpenedBySearch; }

private:
    ValueTreeNodeComponent* parent;
    SafePointer<ValueTreeNodeComponent> previous, next;
    OwnedArray<ValueTreeNodeComponent> nodes;
    bool isOpened = false;
    bool isOpenedBySearch = false;
    bool isDragging = false;

    std::unique_ptr<ValueTreeNodeBranchLine> nodeBranchLine;

    bool showXYpos = false;
    bool showIndex = false;

    friend class ValueTreeViewerComponent;
};

#include "SettingsFile.h"

class ValueTreeViewerComponent final : public Component
    , public KeyListener
    , public SettingsFileListener {
public:
    explicit ValueTreeViewerComponent(String prepend = String())
        : tooltipPrepend(std::move(prepend))
    {
        if (tooltipPrepend == "(Subpatch)") { // FIXME: this is horrible
            sortLayerOrder = SettingsFile::getInstance()->getProperty<bool>("search_order");
            showXYPos = SettingsFile::getInstance()->getProperty<bool>("search_xy_show");
            showIndex = SettingsFile::getInstance()->getProperty<bool>("search_index_show");
        }

        // Add a Viewport to handle scrolling
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, false, false);
        viewport.addKeyListener(this);

        contentComponent.setVisible(true);
        contentComponent.updateView = [this] {
            resized();
        };

        contentComponent.onReturn = [this](ValueTree& tree) { onReturn(tree); };
        contentComponent.onClick = [this](ValueTree& tree) { onClick(tree); };
        contentComponent.onSelect = [this](ValueTree& tree) { onSelect(tree); };
        contentComponent.onDragStart = [this](ValueTree& tree) { onDragStart(tree); };
        contentComponent.onRightClick = [this](ValueTree& tree) { onRightClick(tree); };

        addAndMakeVisible(viewport);
    }

    void makeNodeActive(void const* objPtr)
    {
        for (auto* node : nodes) {
            if (reinterpret_cast<void*>(static_cast<int64>(node->valueTreeNode.getProperty("Object"))) == objPtr) {
                node->makeActive();
            }
        }
    }

    void settingsChanged(String const& name, var const& value) override
    {
        if (tooltipPrepend != "(Subpatch)") // FIXME: again this is a horrible way to find out what we are!
            return;

        if (name == "search_order") {
            setSortDir(value);
        } else if (name == "search_xy_show") {
            auto const showXY = static_cast<bool>(value);
            if (showXYPos != showXY) {
                showXYPos = showXY;
                setShowXYPosAllNodes(nodes);
            }
        } else if (name == "search_index_show") {
            auto const showXY = static_cast<bool>(value);
            if (showIndex != showXY) {
                showIndex = showXY;
                setShowIndex(nodes);
            }
        }
    }

    void setSelectedNode(void const* obj)
    {
        // Locate the object in the value tree, and set it as the selected node
        if (obj) {
            for (auto const& node : nodes) {
                if (reinterpret_cast<void*>(static_cast<int64>(node->valueTreeNode.getProperty("Object"))) == obj) {
                    contentComponent.selectedNode = node;
                    return;
                }
            }
        }
        contentComponent.selectedNode = nullptr;
    }

    void* getSelectedNodeObject() const
    {
        if (contentComponent.selectedNode)
            return reinterpret_cast<void*>(static_cast<int64>(contentComponent.selectedNode->valueTreeNode.getProperty("Object")));

        return nullptr;
    }

    void setValueTree(ValueTree const& tree)
    {
        valueTree = tree;
        auto const originalViewPos = viewport.getViewPosition();

        // Compare existing child nodes with current children
        for (int i = 0; i < valueTree.getNumChildren(); ++i) {
            ValueTree childNode = valueTree.getChild(i);

            // Check if an existing node exists for this child
            ValueTreeNodeComponent* existingNode = nullptr;
            for (auto* node : nodes) {
                if (childNode.isValid() && ValueTreeNodeComponent::compareProperties(childNode, node->valueTreeNode)) {
                    existingNode = node;
                    node->valueTreeNode = childNode;
                    break;
                }
            }

            // If an existing node is found, update it; otherwise, create a new node
            if (existingNode != nullptr) {
                existingNode->update();
            } else if (childNode.isValid()) {
                auto* childComponent = new ValueTreeNodeComponent(childNode, nullptr, tooltipPrepend);
                nodes.add(childComponent);
                contentComponent.addAndMakeVisible(childComponent);
            }
        }

        // Remove any existing nodes that no longer exist in the current children
        for (int i = nodes.size(); --i >= 0;) {
            if (!nodes.getUnchecked(i)->valueTreeNode.isAChildOf(valueTree)) {
                nodes.remove(i);
            }
        }

        sortNodes(nodes, sortLayerOrder);

        setShowIndex(nodes);
        setShowXYPosAllNodes(nodes);

        ValueTreeNodeComponent* previous = nullptr;
        linkNodes(nodes, previous);

        contentComponent.resized();
        resized();

        viewport.setViewPosition(originalViewPos);
    }

    void clearValueTree()
    {
        ValueTree const emptyTree;
        setValueTree(emptyTree);
    }

    ValueTree& getValueTree()
    {
        return valueTree;
    }

    void setSortDir(bool const sortDir)
    {
        sortLayerOrder = sortDir;
        sortNodes(nodes, sortLayerOrder);
        resizeAllNodes(nodes);
    }

    void setShowIndex(OwnedArray<ValueTreeNodeComponent>& nodes)
    {
        for (auto* node : nodes) {
            node->showIndex = showIndex;
            node->repaint();
            setShowIndex(node->nodes);
        }
    }

    void setShowXYPosAllNodes(OwnedArray<ValueTreeNodeComponent>& nodes)
    {
        for (auto* node : nodes) {
            node->showXYpos = showXYPos;
            node->repaint();
            setShowXYPosAllNodes(node->nodes);
        }
    }

    void resizeAllNodes(OwnedArray<ValueTreeNodeComponent>& nodes)
    {
        resized();

        for (auto* node : nodes) {
            node->resized();
            resizeAllNodes(node->nodes);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(PlugDataColour::sidebarBackgroundColourId));
    }

    int getTotalContentHeight() const
    {
        int totalHeight = 0;
        for (auto const* node : nodes) {
            totalHeight += node->isVisible() ? node->getTotalContentHeight() : 0;
        }
        return totalHeight;
    }

    void resized() override
    {
        auto const originalViewPos = viewport.getViewPosition();

        // Set the bounds of the Viewport within the main component
        auto bounds = getLocalBounds();
        viewport.setBounds(bounds);

        contentComponent.setBounds(0, 0, bounds.getWidth(), std::max(getTotalContentHeight(), bounds.getHeight()));

        auto const scrollbarIndent = viewport.canScrollVertically() ? 8 : 0;
        bounds = bounds.reduced(2, 0).withTrimmedRight(scrollbarIndent).withHeight(getTotalContentHeight() + 4).withTrimmedTop(4);

        auto resizeNodes = [bounds](ValueTreeNodeComponent* node) mutable {
            if (node->isVisible()) {
                auto const childBounds = bounds.removeFromTop(node->getTotalContentHeight());
                node->setBounds(childBounds);
            }
        };

        for (auto* node : nodes) {
            resizeNodes(node);
        }

        viewport.setViewPosition(originalViewPos);
    }

    bool keyPressed(KeyPress const& key, Component* component) override
    {
        auto const previousSel = contentComponent.selectedNode;
        if (key.getKeyCode() == KeyPress::upKey) {
            // Traverse previous nodes
            while (contentComponent.selectedNode && contentComponent.selectedNode->previous) {
                contentComponent.selectedNode = contentComponent.selectedNode->previous;

                // Skip nodes that are not visible or whose parent is closed
                while (contentComponent.selectedNode && contentComponent.selectedNode->parent && !contentComponent.selectedNode->parent->isOpen()) {
                    contentComponent.selectedNode = contentComponent.selectedNode->previous;
                }

                // If the current node is visible, break
                if (contentComponent.selectedNode && contentComponent.selectedNode->isShowing()) {
                    break;
                }
            }

            if (contentComponent.selectedNode && contentComponent.selectedNode->isShowing()) {
                onSelect(contentComponent.selectedNode->valueTreeNode);
                contentComponent.repaint();
                resized();
                scrollToShowSelection();
            } else {
                contentComponent.selectedNode = previousSel;
            }
            return true;
        }
        if (key.getKeyCode() == KeyPress::downKey) {
            // Traverse next nodes
            while (contentComponent.selectedNode && contentComponent.selectedNode->next) {
                contentComponent.selectedNode = contentComponent.selectedNode->next;

                // Skip nodes that are not visible or whose parent is closed
                while (contentComponent.selectedNode && contentComponent.selectedNode->parent && !contentComponent.selectedNode->parent->isOpen()) {
                    contentComponent.selectedNode = contentComponent.selectedNode->next;
                }

                // If the current node is visible, break
                if (contentComponent.selectedNode && contentComponent.selectedNode->isShowing()) {
                    break;
                }
            }

            if (contentComponent.selectedNode && contentComponent.selectedNode->isShowing()) {
                onSelect(contentComponent.selectedNode->valueTreeNode);
                contentComponent.repaint();
                resized();
                scrollToShowSelection();
            } else {
                contentComponent.selectedNode = previousSel;
            }
            return true;
        }

        if (key.getKeyCode() == KeyPress::rightKey) {
            if (contentComponent.selectedNode && contentComponent.selectedNode->nodes.size())
                contentComponent.selectedNode->toggleNodeOpenClosed();
            return true;
        }
        if (key.getKeyCode() == KeyPress::leftKey) {
            if (contentComponent.selectedNode && contentComponent.selectedNode->nodes.size() && contentComponent.selectedNode->isOpen())
                contentComponent.selectedNode->toggleNodeOpenClosed();
            return true;
        }
        if (key.getKeyCode() == KeyPress::returnKey) {
            if (contentComponent.selectedNode && contentComponent.selectedNode->parent != nullptr)
                onReturn(contentComponent.selectedNode->valueTreeNode);
            return true;
        }

        return false;
    }

    void scrollToShowSelection()
    {
        if (auto const* selection = contentComponent.selectedNode.getComponent()) {
            auto const viewBounds = viewport.getViewArea();
            auto const selectionBounds = contentComponent.getLocalArea(selection, selection->getLocalBounds());

            if (selectionBounds.getY() < viewBounds.getY()) {
                viewport.setViewPosition(0, selectionBounds.getY());
            } else if (selectionBounds.getBottom() > viewBounds.getBottom()) {
                viewport.setViewPosition(0, selectionBounds.getY() - (viewBounds.getHeight() - 25));
            }
        }
    }

    Viewport& getViewport()
    {
        return viewport;
    }

    void setFilterString(String const& toFilter)
    {
        filterString = toFilter;
        filterNodes();
    }

    void filterNodes()
    {
        if (filterString.isEmpty()) {
            for (auto* topLevelNode : nodes) {
                clearSearch(topLevelNode);
            }

            resized();
            return;
        }

        for (auto* topLevelNode : nodes) {
            searchInNode(topLevelNode);
        }

        resized();
    }

    std::function<void(ValueTree&)> onReturn = [](ValueTree&) { };
    std::function<void(ValueTree&)> onClick = [](ValueTree&) { };
    std::function<void(ValueTree&)> onSelect = [](ValueTree&) { };
    std::function<void(ValueTree&)> onDragStart = [](ValueTree&) { };
    std::function<void(ValueTree&)> onRightClick = [](ValueTree&) { };

private:
    static void linkNodes(OwnedArray<ValueTreeNodeComponent>& nodes, ValueTreeNodeComponent*& previous)
    {
        // Iterate over direct children
        for (auto* node : nodes) {
            if (previous) {
                node->previous = previous;
                previous->next = node;
            }

            // Recursively iterate over grandchildren
            linkNodes(node->nodes, node);

            // Set previous to the last child processed
            previous = node;
        }
    }

    bool searchInNode(ValueTreeNodeComponent* node)
    {
        // Check if the current node matches the filterString
        int found = 0;
        StringArray searchTokens;
        searchTokens.addTokens(filterString, " ", "\"");
        for (auto& token : searchTokens) {
            auto isStrict = false;
            // Modify token to deal with quotation mark restriction
            if (token[0] == '"' && token.getLastCharacter() == '"') {
                token = token.substring(1).dropLastCharacters(1);
                // Should only be used for object search only ATM
                isStrict = true;
            }

            auto searchObjectNameOnly = false;

            // Modify token to deal limit search to object names only (not flags / text)
            if (token.length() > 7) {
                auto objectSearch = token.substring(0, 7);
                if (objectSearch.containsWholeWordIgnoreCase("object:")) {
                    searchObjectNameOnly = true;
                    token = token.substring(7);
                }
            }

            // NOTE! lambda capture needs to happen here, after we have modified token & isStrict to implement search restrictions
            auto findProperty = [node, token](String const& propertyName, bool const containsToken = false, bool const isStrict = false) -> bool {
                if (node->valueTreeNode.hasProperty(propertyName)) {
                    if (containsToken) {
                        auto const name = node->valueTreeNode.getProperty(propertyName).toString();
                        // std::cout << "token: " << token << " name: " << name << std::endl;
                        return (isStrict ? token.length() == name.length() : true) && name.containsIgnoreCase(token);
                    }
                    return true;
                }
                return false;
            };

            if (token.isEmpty() ||
                // search the object name only if user prepends search string with "object:<to-search-for>", otherwise search full name + args
                (searchObjectNameOnly ? findProperty("ObjectName", true, isStrict) : findProperty("Name", true)) ||
                // search over the send/receive tags
                findProperty("SendSymbol", true) || findProperty("ReceiveSymbol", true) ||
                // return all nodes that have send with the keyword: "send"
                (token == "send" && (findProperty("SendSymbol") || findProperty("SendObject"))) ||
                // return all nodes that have receive with the keyword: "send"
                (token == "receive" && (findProperty("ReceiveSymbol") || findProperty("ReceiveObject"))) ||
                // return all nodes that have send or recieve when keyword is "symbols"
                (token == "symbols" && (findProperty("SendSymbol") || findProperty("SendObject") || findProperty("ReceiveSymbol") || findProperty("ReceiveObject"))) ||
                // Deal with alias objects here:
                // t == trigger
                // v == value
                // i == int
                // f == float
                (token == "trigger" && findProperty("TriggerObject")) || (token == "value" && findProperty("ValueObject")) || (token == "int" && findProperty("IntObject")) || (token == "float" && findProperty("FloatObject"))) {
                found++;
            }
        }

        // attempt at implementing an 'and' search, all search text tokens need to be true
        found = searchTokens.size() == found;

        for (auto* child : node->nodes) {
            // We can't return early because searchInNode has side effects
            found = searchInNode(child) || found;
        }

        node->isOpenedBySearch = !node->nodes.isEmpty() && found;

        // Set the visibility of the node based on whether it matches the filterString
        node->setVisible(found);

        return found;
    }

    static void clearSearch(ValueTreeNodeComponent* node)
    {
        node->setVisible(true);
        node->isOpenedBySearch = false;
        for (auto* child : node->nodes) {
            clearSearch(child);
        }
    }

    static void sortNodes(OwnedArray<ValueTreeNodeComponent>& nodes, bool const sortDown)
    {
        struct {
            bool sortDirection = false;

            int compareElements(ValueTreeNodeComponent const* a, ValueTreeNodeComponent const* b) const
            {
                auto const firstIdx = a->valueTreeNode.getParent().indexOf(a->valueTreeNode);
                auto const secondIdx = b->valueTreeNode.getParent().indexOf(b->valueTreeNode);
                if (firstIdx > secondIdx)
                    return sortDirection ? -1 : 1;
                if (firstIdx < secondIdx)
                    return sortDirection ? 1 : -1;
                return 0;
            }
        } elementComparator;

        elementComparator.sortDirection = sortDown;

        nodes.sort(elementComparator, false);

        for (auto* childNode : nodes) {
            sortNodes(childNode->nodes, sortDown);
        }
    }

    String filterString;
    String tooltipPrepend;
    ValueTreeOwnerView contentComponent;
    OwnedArray<ValueTreeNodeComponent> nodes;
    ValueTree valueTree = ValueTree("Folder");
    BouncingViewport viewport;
    bool sortLayerOrder = false;
    bool showXYPos = false;
    bool showIndex = false;
};
