#pragma once

#include "../Utility/ObjectDragAndDrop.h"

class ObjectsListBox;
class ListBoxObjectItem : public ObjectDragAndDrop
{
public:
    ListBoxObjectItem(ObjectsListBox* parent, int rowNumber, bool isSelected, std::function<void(bool shouldFade)> dismissDialog);

    void paint(juce::Graphics& g) override;

    bool hitTest(int x, int y);

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    String getItemName() const;

    String getObjectString() override;
    void dismiss(bool withAnimation) override;

    void refresh(String objectName, String objectDescription, int rowNumber, bool isSelected);

private:
    int row;
    String objectName;
    String objectDescription;
    bool rowIsSelected = false;
    ObjectsListBox* objectsListBox;

    bool mouseHover = false;

    std::function<void(bool shouldFade)> dismissMenu;
};