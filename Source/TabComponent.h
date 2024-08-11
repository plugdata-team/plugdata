#pragma once

#include "Utility/ZoomableDragAndDropContainer.h"
#include "PluginProcessor.h"

class PluginMode;
class TabComponent : public Component
    , public DragAndDropTarget
    , public AsyncUpdater {
    class TabBarButtonComponent;

public:
    explicit TabComponent(PluginEditor* editor);

    ~TabComponent();
    
    Canvas* newPatch();

    Canvas* openPatch(const URL& path);
    Canvas* openPatch(String const& patchContent);
    Canvas* openPatch(pd::Patch::Ptr existingPatch);
    void openPatch();

    void openInPluginMode(pd::Patch::Ptr patch);

    void renderArea(NVGcontext* nvg, Rectangle<int> bounds);

    void nextTab();
    void previousTab();

    void askToCloseTab(Canvas* cnv);
    void closeTab(Canvas* cnv);
    void showTab(Canvas* cnv, int splitIndex = 0);
    void setActiveSplit(Canvas* cnv);

    void closeAllTabs(
        bool quitAfterComplete = false, Canvas* patchToExclude = nullptr, std::function<void()> afterComplete = []() {});
    void createNewWindow(Component* draggedTab);

    Canvas* getCurrentCanvas();
    Canvas* getCanvasAtScreenPosition(Point<int> screenPosition);

    Array<Canvas*> getCanvases();
    Array<Canvas*> getVisibleCanvases();

private:
    void clearCanvases();
    void handleAsyncUpdate() override;

    void sendTabUpdateToVisibleCanvases();

    void resized() override;

    void moveToLeftSplit(TabComponent::TabBarButtonComponent* tab);
    void moveToRightSplit(TabComponent::TabBarButtonComponent* tab);

    void saveTabPositions();
    void closeEmptySplits();

    bool isInterestedInDragSource(SourceDetails const& dragSourceDetails) override;
    void itemDropped(SourceDetails const& dragSourceDetails) override;
    void itemDragEnter(SourceDetails const& dragSourceDetails) override;
    void itemDragExit(SourceDetails const& dragSourceDetails) override;
    void itemDragMove(SourceDetails const& dragSourceDetails) override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;

    void showHiddenTabsMenu(int splitIndex);

    class TabBarButtonComponent : public Component {

        struct TabDragConstrainer : public ComponentBoundsConstrainer {
            explicit TabDragConstrainer(TabComponent* parent)
                : parent(parent)
            {
            }
            void checkBounds(Rectangle<int>& bounds, Rectangle<int> const&, Rectangle<int> const& limits, bool, bool, bool, bool) override
            {
                bounds = bounds.withPosition(std::clamp(bounds.getX(), 30, parent->getWidth() - bounds.getWidth()), 0);
            }

            TabComponent* parent;
        };

        class CloseTabButton : public SmallIconButton {

            using SmallIconButton::SmallIconButton;

            void paint(Graphics& g) override
            {
                auto font = Fonts::getIconFont().withHeight(12);
                g.setFont(font);

                if (!isEnabled()) {
                    g.setColour(Colours::grey);
                } else if (getToggleState()) {
                    g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
                } else if (isMouseOver()) {
                    g.setColour(findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f));
                } else {
                    g.setColour(findColour(PlugDataColour::toolbarTextColourId));
                }

                int const yIndent = jmin(4, proportionOfHeight(0.3f));
                int const cornerSize = jmin(getHeight(), getWidth()) / 2;

                int const fontHeight = roundToInt(font.getHeight() * 0.6f);
                int const leftIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
                int const rightIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
                int const textWidth = getWidth() - leftIndent - rightIndent;

                if (textWidth > 0)
                    g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);
            }
        };

    public:
        TabBarButtonComponent(Canvas* cnv, TabComponent* parent)
            : cnv(cnv)
            , parent(parent)
            , tabDragConstrainer(parent)
        {
            closeButton.onClick = [cnv = SafePointer(cnv), parent]() {
                if (cnv)
                    parent->askToCloseTab(cnv);
            };
            closeButton.addMouseListener(this, false);
            closeButton.setSize(28, 28);
            addAndMakeVisible(closeButton);
            setRepaintsOnMouseActivity(true);
        }

        void paint(Graphics& g) override
        {
            auto mouseOver = isMouseOver();
            auto active = isActive();
            if (active) {
                g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
            } else if (mouseOver) {
                g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId).interpolatedWith(findColour(PlugDataColour::toolbarBackgroundColourId), 0.4f));
            } else {
                g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
            }

            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4.5f), Corners::defaultCornerRadius);

            auto area = getLocalBounds().reduced(4, 1).toFloat();

            // Use a gradient to make it fade out when it gets near to the close button
            auto fadeX = (mouseOver || active) ? area.getRight() - 25 : area.getRight() - 8;
            auto textColour = findColour(PlugDataColour::toolbarTextColourId);
            g.setGradientFill(ColourGradient(textColour, fadeX - 18, area.getY(), Colours::transparentBlack, fadeX, area.getY(), false));

            if (cnv) {
                auto text = cnv->patch.getTitle() + (cnv->patch.isDirty() ? String("*") : String());

                g.setFont(Fonts::getCurrentFont().withHeight(14.0f));
                g.drawText(text, area.reduced(4, 0), Justification::centred, false);
            }
        }

        void resized() override
        {
            closeButton.setCentrePosition(getLocalBounds().getCentre().withX(getWidth() - 15).translated(0, 1));
        }

        ScaledImage generateTabBarButtonImage() const
        {
            if (!cnv)
                return {};

            auto scale = 2.0f;
            // we calculate the best size for the tab DnD image
            auto text = cnv->patch.getTitle();
            Font font(Fonts::getCurrentFont());
            auto length = font.getStringWidth(text) + 32;
            auto const boundsOffset = 10;

            // we need to expand the bounds, but reset the position to top left
            // then we offset the mouse drag by the same amount
            // this is to allow area for the shadow to render correctly
            auto textBounds = Rectangle<int>(0, 0, length, 28);
            auto bounds = textBounds.expanded(boundsOffset).withZeroOrigin();
            auto image = Image(Image::PixelFormat::ARGB, bounds.getWidth() * scale, bounds.getHeight() * scale, true);
            auto g = Graphics(image);
            g.addTransform(AffineTransform::scale(scale));
            Path path;
            path.addRoundedRectangle(bounds.reduced(10), 5.0f);
            StackShadow::renderDropShadow(g, path, Colour(0, 0, 0).withAlpha(0.2f), 6, { 0, 1 }, scale);
            g.setOpacity(1.0f);

            g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
            g.fillRoundedRectangle(textBounds.withPosition(10, 10).reduced(2).toFloat(), Corners::defaultCornerRadius);

            g.setColour(findColour(PlugDataColour::toolbarTextColourId));

            g.setFont(font);
            g.drawText(text, textBounds.withPosition(10, 10), Justification::centred, false);

            return ScaledImage(image, scale);
        }

        void mouseDown(MouseEvent const& e) override
        {
            if (e.mods.isPopupMenu() && cnv) {
                PopupMenu tabMenu;

#if JUCE_MAC
                String revealTip = "Reveal in Finder";
#elif JUCE_WINDOWS
                String revealTip = "Reveal in Explorer";
#else
                String revealTip = "Reveal in file browser";
#endif

                bool canReveal = cnv->patch.getCurrentFile().existsAsFile();

                tabMenu.addItem(revealTip, canReveal, false, [this]() {
                    cnv->patch.getCurrentFile().revealToUser();
                });

                tabMenu.addSeparator();

                PopupMenu parentPatchMenu;

                if (auto patch = cnv->patch.getPointer()) {
                    auto* parentPatch = patch.get();
                    while ((parentPatch = parentPatch->gl_owner)) {
                        parentPatchMenu.addItem(String::fromUTF8(parentPatch->gl_name->s_name), [this, parentPatch]() {
                            auto* pdInstance = dynamic_cast<pd::Instance*>(parent->pd);
                            parent->openPatch(new pd::Patch(pd::WeakReference(parentPatch, pdInstance), pdInstance, false));
                        });
                    }
                }

                tabMenu.addSubMenu("Parent patches", parentPatchMenu, parentPatchMenu.getNumItems());

                tabMenu.addSeparator();

                auto splitIndex = parent->splits[1] && parent->tabbars[1].contains(this);
                auto canSplitTab = parent->splits[1] || parent->tabbars[splitIndex].size() > 1;
                tabMenu.addItem("Split left", canSplitTab, false, [this]() {
                    parent->moveToLeftSplit(this);
                    parent->closeEmptySplits();
                    parent->saveTabPositions();
                });
                tabMenu.addItem("Split right", canSplitTab, false, [this]() {
                    parent->moveToRightSplit(this);
                    parent->closeEmptySplits();
                    parent->saveTabPositions();
                });

                tabMenu.addSeparator();

                tabMenu.addItem("Close patch", true, false, [this]() {
                    parent->closeTab(cnv);
                });

                tabMenu.addItem("Close all other patches", true, false, [this]() {
                    parent->closeAllTabs(false, cnv);
                });

                tabMenu.addItem("Close all patches", true, false, [this]() {
                    parent->closeAllTabs(false);
                });

                // Show the popup menu at the mouse position
                tabMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1));
            } else if (cnv && e.originalComponent == this) {
                toFront(false);
                parent->showTab(cnv, parent->tabbars[1].contains(this));
                dragger.startDraggingComponent(this, e);
            }
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if (e.getDistanceFromDragStart() > 10 && !isDragging) {
                isDragging = true;
                auto dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

                tabImage = generateTabBarButtonImage();
                dragContainer->startDragging(1, this, tabImage, tabImage, true, nullptr);
            } else if (parent->draggingOverTabbar) {
                dragger.dragComponent(this, e, &tabDragConstrainer);
            }
        }

        void mouseUp(MouseEvent const& e) override
        {
            isDragging = false;
            setVisible(true);
            parent->resized(); // call resized so the dropped tab will animate into its correct position
        }

        bool isActive() const
        {
            return cnv && (parent->splits[0] == cnv || parent->splits[1] == cnv);
        }

        // close button, etc.
        SafePointer<Canvas> cnv;
        TabComponent* parent;
        ScaledImage tabImage;
        bool isDragging = false;
        ComponentDragger dragger;
        TabDragConstrainer tabDragConstrainer;

        CloseTabButton closeButton = CloseTabButton(Icons::Clear);
    };

    std::array<MainToolbarButton, 2> newTabButtons = { MainToolbarButton(Icons::Add), MainToolbarButton(Icons::Add) };
    std::array<MainToolbarButton, 2> tabOverflowButtons = { MainToolbarButton(Icons::ThinDown), MainToolbarButton(Icons::ThinDown) };

    std::array<OwnedArray<TabBarButtonComponent>, 2> tabbars;
    std::array<SafePointer<Canvas>, 2> splits = { nullptr, nullptr };

    std::array<pd::Patch*, 2> lastSplitPatches { nullptr, nullptr };
    t_glist* lastActiveCanvas = nullptr;

    bool draggingOverTabbar = false;
    bool draggingSplitResizer = false;
    Rectangle<int> splitDropBounds;

    float splitProportion = 2;
    int splitSize = 0;
    int activeSplitIndex = 0;

    OwnedArray<Canvas, CriticalSection> canvases;

    PluginEditor* editor;
    PluginProcessor* pd;
};
