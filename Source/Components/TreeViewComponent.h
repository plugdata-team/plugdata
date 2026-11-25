/*
// Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Fonts.h"
#include "Utility/CachedStringWidth.h"
#include "Utility/NVGGraphicsContext.h"
#include "Components/BouncingViewport.h"
#include "Utility/SettingsFile.h"

// Nodes, properties and string data each live in one flat array. clear() keeps their capacity,
// so rebuilding a tree that's no larger than before doesn't allocate.
class TreeViewContent {
    static constexpr uint32 none = ~0u;

    struct NodeData {
        int64 id;
        uint32 firstChild = none, lastChild = none, nextSibling = none, firstProperty = none;
    };

    struct Property {
        hash32 name;
        uint32 next;
        int64 value; // integer, pointer, or offset of a null-terminated string
    };

public:
    struct PropertyName {
        consteval PropertyName(char const* name)
            : hash(::hash(name))
        {
        }

        hash32 hash;
    };

    // Handle to a node, valid until its tree is reassigned or cleared
    class Node {
    public:
        Node() = default;

        bool isValid() const { return tree != nullptr; }
        int64 getId() const { return tree ? data().id : 0; }

        Node addChild(int64 const id) const
        {
            auto const child = static_cast<uint32>(tree->nodes.size());
            tree->nodes.add({ id });
            auto& parent = data();
            (parent.lastChild == none ? parent.firstChild : tree->nodes[parent.lastChild].nextSibling) = child;
            parent.lastChild = child;
            return { tree, child };
        }

        bool hasProperty(PropertyName const name) const { return find(name) != nullptr; }

        // Takes integers, bools, pointers and strings; read them back as the type they were set with
        template<typename T>
        void setProperty(PropertyName const name, T const& value) const
        {
            int64 v;
            if constexpr (std::is_integral_v<T>)
                v = static_cast<int64>(value);
            else if constexpr (std::is_same_v<T, String>)
                v = tree->addString(value.toRawUTF8(), value.getNumBytesAsUTF8());
            else if constexpr (std::is_same_v<T, SmallString>)
                v = tree->addString(value.data(), value.length());
            else if constexpr (std::is_convertible_v<T const&, char const*>)
                v = tree->addString(value, std::strlen(value));
            else
                v = reinterpret_cast<int64>(value);

            if (auto* property = find(name)) {
                property->value = v;
            } else {
                tree->properties.add({ name.hash, data().firstProperty, v });
                data().firstProperty = static_cast<uint32>(tree->properties.size() - 1);
            }
        }

        template<typename T>
        T getProperty(PropertyName const name) const
        {
            auto const* property = find(name);
            if constexpr (std::is_same_v<T, String>)
                return property ? String::fromUTF8(tree->strings.data() + property->value) : String();
            else if constexpr (std::is_same_v<T, char const*>)
                return property ? tree->strings.data() + property->value : "";
            else if constexpr (std::is_pointer_v<T>)
                return property ? reinterpret_cast<T>(property->value) : nullptr;
            else
                return property ? static_cast<T>(property->value) : T();
        }

        struct Iterator {
            TreeViewContent* tree;
            uint32 index;

            Node operator*() const { return { tree, index }; }
            bool operator!=(Iterator const& other) const { return index != other.index; }
            Iterator& operator++()
            {
                index = tree->nodes[index].nextSibling;
                return *this;
            }
        };

        Iterator begin() const { return { tree, tree ? data().firstChild : none }; }
        Iterator end() const { return { tree, none }; }

    private:
        friend class TreeViewContent;

        Node(TreeViewContent* tree, uint32 const index)
            : tree(tree)
            , index(index)
        {
        }

        NodeData& data() const { return tree->nodes[index]; }

        Property* find(PropertyName const name) const
        {
            if (tree) {
                for (auto i = data().firstProperty; i != none; i = tree->properties[i].next) {
                    if (tree->properties[i].name == name.hash)
                        return &tree->properties[i];
                }
            }
            return nullptr;
        }

        TreeViewContent* tree = nullptr;
        uint32 index = 0;
    };

    TreeViewContent()
    {
        clear();
    }

    Node getRoot() { return { this, 0 }; }

    void clear()
    {
        nodes.clear();
        properties.clear();
        strings.clear();
        nodes.add({ 0 });
    }

private:
    int64 addString(char const* text, size_t const length)
    {
        auto const offset = static_cast<int64>(strings.size());
        strings.append(text, text + length);
        strings.add('\0');
        return offset;
    }

    SmallArray<NodeData, 64> nodes;
    SmallArray<Property, 512> properties;
    SmallArray<char, 4096> strings;
};

class TreeViewComponent;
class TreeViewNodeComponent;
struct TreeViewOwnerView final : public Component {
    SafePointer<TreeViewNodeComponent> selectedNode;

    std::function<void()> updateView = [] { };
    std::function<void(TreeViewContent::Node const&)> onReturn = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onClick = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onSelect = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onDragStart = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onRightClick = [](TreeViewContent::Node const&) { };
};

class TreeViewNodeComponent final : public Component {
    class TreeViewNodeBranchLine final : public Component
        , public TooltipClient {
    public:
        explicit TreeViewNodeBranchLine(TreeViewNodeComponent* node)
            : node(node)
        {
        }

        String getTooltip() override
        {
            if (node->treeNode.hasProperty("Name"))
                return node->tooltipPrepend + " " + node->treeNode.getProperty<String>("Name");

            return { };
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);

            auto const colour = isHover && !node->isOpenInSearchMode() ? colours.objectSelectedOutlineColour : colours.panelTextColour.withAlpha(0.25f);

            g.setColour(colour);
            g.drawVerticalLine(4.0f, 0.0f, getHeight() - 3.0f);
            g.fillEllipse(Rectangle<float>(2.0f, getHeight() - 5.0f, 5.0f, 5.0f));
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
            if (!isRealClickEvent(e))
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


    private:
        TreeViewNodeComponent* node;
        bool isHover = false;
    };

public:
    TreeViewNodeComponent(TreeViewContent::Node const& node, int const index, TreeViewNodeComponent* parentNode, String const& prepend)
        : treeNode(node)
        , id(node.getId())
        , index(index)
        , parent(parentNode)
        , tooltipPrepend(prepend.isEmpty() ? String("(Parent)") : prepend)
    {
        nodeBranchLine = std::make_unique<TreeViewNodeBranchLine>(this);
        addAndMakeVisible(nodeBranchLine.get());
        nodeBranchLine->setAlwaysOnTop(true);

        syncNodes(nodes, treeNode, *this, this, tooltipPrepend);
    }

    void update(TreeViewContent::Node const& node, int const newIndex)
    {
        treeNode = node;
        index = newIndex;
        syncNodes(nodes, treeNode, *this, this, tooltipPrepend);
    }

    // Reuses the components whose id is still in the tree, so open branches and selection survive a refresh
    static void syncNodes(OwnedArray<TreeViewNodeComponent>& nodes, TreeViewContent::Node const& treeNode, Component& container, TreeViewNodeComponent* parent, String const& prepend)
    {
        OwnedArray<TreeViewNodeComponent> previousNodes;
        previousNodes.swapWith(nodes);

        UnorderedMap<int64, int> previousIndices;
        for (int i = 0; i < previousNodes.size(); i++)
            previousIndices.emplace(previousNodes[i]->id, i);

        int index = 0;
        for (auto child : treeNode) {
            auto const previous = previousIndices.find(child.getId());
            if (auto* node = previous != previousIndices.end() ? previousNodes[previous->second] : nullptr) {
                previousNodes.set(previous->second, nullptr, false);
                nodes.add(node)->update(child, index++);
            } else {
                auto* newNode = nodes.add(new TreeViewNodeComponent(child, index++, parent, prepend));
                container.addChildComponent(newNode);
                newNode->setVisible(!parent || parent->isOpen());
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

        g.setColour(getThemeColours(*this).sidebarTextColour);
        g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(arrowArea, true));
    }

    bool isSelected() const
    {
        if (auto const node = getOwnerView()->selectedNode)
            return node.getComponent() == this;

        return false;
    }

    TreeViewOwnerView* getOwnerView() const
    {
        return findParentComponentOfClass<TreeViewOwnerView>();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!isDragging && e.getDistanceFromDragStart() > 10) {
            isDragging = true;
            getOwnerView()->onDragStart(treeNode);
        }
    }

    bool isOpen() const
    {
        return isOpened || isOpenedBySearch;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.eventComponent == this && e.mods.isRightButtonDown())
            getOwnerView()->onRightClick(treeNode);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!isRealClickEvent(e))
            return;

        isDragging = false;

        if (e.eventComponent == this && e.mods.isLeftButtonDown() && getScreenBounds().contains(e.getScreenPosition())) {
            if (nodes.size() && e.x < 22) {
                toggleNodeOpenClosed();
            } else {
                getOwnerView()->selectedNode = this;
                getOwnerView()->repaint();

                if (e.getNumberOfClicks() == 1) {
                    getOwnerView()->onSelect(treeNode);
                } else {
                    getOwnerView()->onClick(treeNode);
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
        auto const& colours = getThemeColours(*this);

        // Either show single selection or multi-selection
        if (getOwnerView()->selectedNode ? isSelected() : treeNode.getProperty<bool>("Selected")) {
            g.setColour(colours.sidebarActiveBackgroundColour);
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

        auto colour = colours.sidebarTextColour;
        auto* nvg = dynamic_cast<NVGGraphicsContext*>(&g.getInternalContext());
        auto drawText = [&g, nvg](String const& text, Rectangle<int> bounds, Colour textColour, bool useEllipsis = true) {
            g.setColour(textColour);
            if (nvg != nullptr)
                nvg->drawText(text, bounds.toFloat(), Justification::centredLeft, useEllipsis);
            else
                g.drawText(text, bounds, Justification::centredLeft, useEllipsis);
        };

        if (treeNode.hasProperty("Icon")) {
            auto iconColour = colour;
            if (treeNode.hasProperty("IconColour"))
                iconColour = Colour(treeNode.getProperty<uint32>("IconColour"));
            g.setFont(Fonts::getIconFont().withHeight(12));
            drawText(treeNode.getProperty<String>("Icon"), itemBounds.removeFromLeft(22).reduced(2), iconColour, false);
        }

        // Don't allow multi-line for viewer items, so remove all newlines in the string
        auto nameText = treeNode.getProperty<String>("Name").replaceCharacters("\n", " ");

        auto nameLength = CachedStringWidth<15>::calculateStringWidth(nameText);
        g.setFont(Fonts::getDefaultFont().withHeight(15));
        drawText(nameText, itemBounds.removeFromLeft(nameLength + 2), colour);

        constexpr auto tagCornerRadius = Corners::defaultCornerRadius * 0.7f;

#ifdef SHOW_PD_SUBPATCH_SYMBOL
        if (treeNode.hasProperty("PDSymbol")) {
            // draw generic symbol label tag
            //  ╭──────────╮
            //  │  symbol  │
            //  ╰──────────╯
            auto sendSymbolText = treeNode.getProperty<String>("PDSymbol");

            auto length = CachedStringWidth<15>::calculateStringWidth(sendSymbolText);
            auto sendColour = colours.objectSelectedOutlineColour.withRotatedHue(0.25);
            g.setColour(sendColour.withAlpha(0.2f));
            auto tagBounds = itemBounds.removeFromLeft(length).translated(4, 0).reduced(0, 5).expanded(2, 0);
            g.fillRoundedRectangle(tagBounds.toFloat(), Corners::defaultCornerRadius * 0.8f);
            drawText(sendSymbolText, tagBounds.translated(2, 0), sendColour);
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
            if (treeNode.hasProperty("ReceiveSymbol")) {
                auto receiveSymbolText = (treeNode.hasProperty("ReceiveObject") ? "" : "r: ") + treeNode.getProperty<String>("ReceiveSymbol");

                auto length = CachedStringWidth<15>::calculateStringWidth(receiveSymbolText);
                auto recColour = colours.objectSelectedOutlineColour;
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
                drawText(receiveSymbolText, tagBounds.translated(2 + tagBounds.getHeight() * 0.5f, 0).toNearestIntEdges(), recColour);
                itemBounds.translate(16, 0);
            }
            // draw send symbol label tag
            //  ╭────────── a
            //  │          │ \
            //  │  symbol  │  b
            //  │          │ /
            //  ╰────────── c
            if (treeNode.hasProperty("SendSymbol")) {
                auto sendSymbolText = (treeNode.hasProperty("SendObject") ? "" : "s: ") + treeNode.getProperty<String>("SendSymbol");

                auto length = CachedStringWidth<15>::calculateStringWidth(sendSymbolText);
                auto sendColour = colours.objectSelectedOutlineColour.withRotatedHue(0.5f);
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
                drawText(sendSymbolText, tagBounds.translated(2, 0).toNearestIntEdges(), sendColour);
            }
        }

        int rightOffset = 0;
        auto rightBounds = getLocalBounds().removeFromTop(25);

        if (showIndex && treeNode.hasProperty("Index")) {
            rightOffset += 4;
            auto rightText = String(treeNode.getProperty<int>("Index"));
            auto const rightTextWidth = CachedStringWidth<15>::calculateStringWidth(rightText);
            if (itemBounds.getWidth() - rightTextWidth >= rightOffset) {
                drawText(rightText, rightBounds.removeFromRight(rightTextWidth + 4), colour.withAlpha(0.5f));
            }
        }

        if (showXYpos && treeNode.hasProperty("RightText")) {
            rightOffset += 8;
            auto rightText = treeNode.getProperty<String>("RightText");
            auto rightTextWidth = CachedStringWidth<15>::calculateStringWidth(rightText);
            if (itemBounds.getWidth() - rightTextWidth >= rightOffset) {
                drawText(rightText, rightBounds.removeFromRight(rightTextWidth + 4), colour.withAlpha(0.5f));
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

    TreeViewContent::Node treeNode;

    bool isOpenInSearchMode() const { return isOpenedBySearch; }

private:
    int64 const id;
    int index;
    TreeViewNodeComponent* parent;
    String const tooltipPrepend;
    SafePointer<TreeViewNodeComponent> previous, next;
    OwnedArray<TreeViewNodeComponent> nodes;
    bool isOpened = false;
    bool isOpenedBySearch = false;
    bool isDragging = false;

    std::unique_ptr<TreeViewNodeBranchLine> nodeBranchLine;

    bool showXYpos = false;
    bool showIndex = false;

    friend class TreeViewComponent;
};


class TreeViewComponent final : public Component
    , public KeyListener
    , public SettingsFileListener {
public:
    explicit TreeViewComponent(String prepend = String())
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

        contentComponent.onReturn = [this](TreeViewContent::Node const& node) { onReturn(node); };
        contentComponent.onClick = [this](TreeViewContent::Node const& node) { onClick(node); };
        contentComponent.onSelect = [this](TreeViewContent::Node const& node) { onSelect(node); };
        contentComponent.onDragStart = [this](TreeViewContent::Node const& node) { onDragStart(node); };
        contentComponent.onRightClick = [this](TreeViewContent::Node const& node) { onRightClick(node); };

        addAndMakeVisible(viewport);
    }

    void makeNodeActive(void const* objPtr)
    {
        for (auto* node : nodes) {
            if (node->id == reinterpret_cast<int64>(objPtr)) {
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
        // Locate the object in the tree, and set it as the selected node
        if (obj) {
            for (auto const& node : nodes) {
                if (node->id == reinterpret_cast<int64>(obj)) {
                    contentComponent.selectedNode = node;
                    return;
                }
            }
        }
        contentComponent.selectedNode = nullptr;
    }

    TreeViewContent::Node getSelectedNode() const
    {
        if (contentComponent.selectedNode)
            return contentComponent.selectedNode->treeNode;

        return { };
    }

    void setValueTree(TreeViewContent const& newTree)
    {
        tree = newTree;
        auto const originalViewPos = viewport.getViewPosition();

        TreeViewNodeComponent::syncNodes(nodes, tree.getRoot(), contentComponent, nullptr, tooltipPrepend);
        sortNodes(nodes, sortLayerOrder);

        setShowIndex(nodes);
        setShowXYPosAllNodes(nodes);

        TreeViewNodeComponent* previous = nullptr;
        linkNodes(nodes, previous);

        contentComponent.resized();
        contentComponent.repaint();
        resized();

        viewport.setViewPosition(originalViewPos);
    }

    void clearValueTree()
    {
        setValueTree({ });
    }

    void setSortDir(bool const sortDir)
    {
        sortLayerOrder = sortDir;
        sortNodes(nodes, sortLayerOrder);
        resizeAllNodes(nodes);
    }

    void setShowIndex(OwnedArray<TreeViewNodeComponent>& nodes)
    {
        for (auto* node : nodes) {
            node->showIndex = showIndex;
            node->repaint();
            setShowIndex(node->nodes);
        }
    }

    void setShowXYPosAllNodes(OwnedArray<TreeViewNodeComponent>& nodes)
    {
        for (auto* node : nodes) {
            node->showXYpos = showXYPos;
            node->repaint();
            setShowXYPosAllNodes(node->nodes);
        }
    }

    void resizeAllNodes(OwnedArray<TreeViewNodeComponent>& nodes)
    {
        resized();

        for (auto* node : nodes) {
            node->resized();
            resizeAllNodes(node->nodes);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(getThemeColours(*this).sidebarBackgroundColour);
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

        auto resizeNodes = [bounds](TreeViewNodeComponent* node) mutable {
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
                onSelect(contentComponent.selectedNode->treeNode);
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
                onSelect(contentComponent.selectedNode->treeNode);
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
                onReturn(contentComponent.selectedNode->treeNode);
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

    std::function<void(TreeViewContent::Node const&)> onReturn = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onClick = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onSelect = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onDragStart = [](TreeViewContent::Node const&) { };
    std::function<void(TreeViewContent::Node const&)> onRightClick = [](TreeViewContent::Node const&) { };

private:
    static void linkNodes(OwnedArray<TreeViewNodeComponent>& nodes, TreeViewNodeComponent*& previous)
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

    bool searchInNode(TreeViewNodeComponent* node)
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
            auto findProperty = [node, token](TreeViewContent::PropertyName const propertyName, bool const containsToken = false, bool const isStrict = false) -> bool {
                if (node->treeNode.hasProperty(propertyName)) {
                    if (containsToken) {
                        auto const name = CharPointer_UTF8(node->treeNode.getProperty<char const*>(propertyName));
                        return (isStrict ? token.length() == static_cast<int>(name.length()) : true) && CharacterFunctions::indexOfIgnoreCase(name, token.getCharPointer()) >= 0;
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

    static void clearSearch(TreeViewNodeComponent* node)
    {
        node->setVisible(true);
        node->isOpenedBySearch = false;
        for (auto* child : node->nodes) {
            clearSearch(child);
        }
    }

    static void sortNodes(OwnedArray<TreeViewNodeComponent>& nodes, bool const sortDown)
    {
        struct {
            bool sortDirection = false;

            int compareElements(TreeViewNodeComponent const* a, TreeViewNodeComponent const* b) const
            {
                auto const firstIdx = a->index;
                auto const secondIdx = b->index;
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
    TreeViewOwnerView contentComponent;
    OwnedArray<TreeViewNodeComponent> nodes;
    TreeViewContent tree;
    BouncingViewport viewport;
    bool sortLayerOrder = false;
    bool showXYPos = false;
    bool showIndex = false;
};
