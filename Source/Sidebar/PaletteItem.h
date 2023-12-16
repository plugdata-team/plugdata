#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include "Pd/Instance.h"
#include "Components/ObjectDragAndDrop.h"
#include "Components/Buttons.h"

class PluginEditor;
class PaletteDraggableList;
class ReorderButton;
class PaletteItem : public ObjectDragAndDrop {
public:
    PaletteItem(PluginEditor* e, PaletteDraggableList* parent, ValueTree tree);
    ~PaletteItem();

    void paint(Graphics& g) override;
    void resized() override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    String getObjectString() override;

    String getPatchStringName() override;

    bool hitTest(int x, int y) override;

    void deleteItem();

    bool isSubpatchOrAbstraction(String const& patchAsString);

    Image patchToTempImage(String const& patch);

    std::pair<std::vector<bool>, std::vector<bool>> countIolets(String const& patchAsString);

    ValueTree itemTree;

    Label nameLabel;
    SmallIconButton deleteButton;

    std::unique_ptr<ReorderButton> reorderButton;

    PluginEditor* editor;
    PaletteDraggableList* paletteComp;
    String paletteName, palettePatch;
    bool isSubpatch;
    std::vector<bool> inlets, outlets;

private:
    void setIsItemDragged(bool isActive);
    bool isItemDragged = false;
};
