#pragma once

#include <JuceHeader.h>

#include "Pd/Instance.h"
#include "Utility/OfflineObjectRenderer.h"

class PluginEditor;
class PaletteDraggableList;
class PaletteItem : public Component {
public:
    PaletteItem(PluginEditor* e, PaletteDraggableList* parent, ValueTree tree);
    ~PaletteItem();

    void paint(Graphics& g) override;
    void resized() override;

    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    bool hitTest(int x, int y) override;

    void deleteItem();

    bool checkIsSubpatch(String const& patchAsString);

    void lookAndFeelChanged() override;

    Image patchToTempImage(String const& patch);

    ImageWithOffset dragImage;

    std::pair<std::vector<bool>, std::vector<bool>> countIolets(String const& patchAsString);

    ValueTree itemTree;
    Label nameLabel;
    TextButton deleteButton;
    PluginEditor* editor;
    PaletteDraggableList* paletteComp;
    String paletteName, palettePatch;
    bool isSubpatch;
    std::vector<bool> inlets, outlets;
    bool isRepositioning = false;
};
