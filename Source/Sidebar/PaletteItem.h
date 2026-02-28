#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "Pd/Instance.h"
#include "Components/ObjectDragAndDrop.h"
#include "Components/Buttons.h"

class PluginEditor;
class PaletteDraggableList;
class ReorderButton;
class PaletteItem final : public ObjectDragAndDrop {
public:
    PaletteItem(PluginEditor* e, PaletteDraggableList* parent, ValueTree tree);
    ~PaletteItem() override;

    void paint(Graphics& g) override;
    void resized() override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    String getObjectString() override;

    void lookAndFeelChanged() override;

    String getPatchStringName() override;

    bool hitTest(int x, int y) override;

    void deleteItem();

    void animateToPosition(Rectangle<int> targetBounds);
    void cancelAnimation(Rectangle<int> targetBounds);
    Rectangle<int> getTargetBounds();

    static bool isSubpatchOrAbstraction(String const& patchAsString);

    static std::pair<SmallArray<bool>, SmallArray<bool>> countIolets(String const& patchAsString);

    ValueTree itemTree;

    Label nameLabel;
    SmallIconButton deleteButton = SmallIconButton(Icons::Clear);

    std::unique_ptr<ReorderButton> reorderButton;

    PluginEditor* editor;
    PaletteDraggableList* paletteComp;
    String paletteName, palettePatch;
    bool isSubpatch;
    SmallArray<bool> inlets, outlets;

    Rectangle<int> animationStartBounds, animationEndBounds;
    VBlankAnimatorUpdater updater { this };
    Animator animator = ValueAnimatorBuilder {}
                            .withEasing(Easings::createEaseInOut())
                            .withDurationMs(260)
                            .withValueChangedCallback([this](float v) {
                                auto start = std::make_tuple(animationStartBounds.getX(), animationStartBounds.getY(), animationStartBounds.getWidth(), animationStartBounds.getHeight());
                                auto end = std::make_tuple(animationEndBounds.getX(), animationEndBounds.getY(), animationEndBounds.getWidth(), animationEndBounds.getHeight());
                                auto const [x, y, w, h] = makeAnimationLimits(start, end).lerp(v);
                                setBounds(x, y, w, h);
                            })
                            .build();

private:
    void setIsItemDragged(bool isActive);
    bool isItemDragged = false;
};
