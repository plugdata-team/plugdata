/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Dialogs/ObjectReferenceDialog.h"

// Sidebar reference panel
class ReferencePanel final : public Component
    , public ObjectReferenceSurface {

public:
    explicit ReferencePanel(PluginEditor* pluginEditor)
        : referenceView(*pluginEditor->pd->objectLibrary, ObjectReferenceView::Layout::Compact)
    {
        addAndMakeVisible(referenceView);
    }

    void selectionChanged(String const& name)
    {
        currentObject = name;
        if (isVisible())
            showCurrentObject();
    }

    Colour getReferenceBackgroundColour() const override { return getThemeColours(*this).sidebarBackgroundColour; }
    Colour getReferenceTextColour() const override { return getThemeColours(*this).sidebarTextColour; }

    void visibilityChanged() override
    {
        if (isVisible())
            showCurrentObject();
    }

    void paint(Graphics& g) override
    {
        g.setColour(getThemeColours(*this).sidebarBackgroundColour);
        g.fillRect(getLocalBounds());
    }

    void paintOverChildren(Graphics& g) override
    {
        if (referenceView.isShowingObject())
            return;

        Fonts::drawText(g, "No object selected", getLocalBounds().withTrimmedTop(24),
            getThemeColours(*this).sidebarTextColour.withAlpha(0.55f), 14, Justification::centredTop);
    }

    void resized() override
    {
        referenceView.setBounds(getLocalBounds());
    }

private:
    void showCurrentObject()
    {
        if (currentObject == referenceView.getObjectName())
            return;

        referenceView.showObject(currentObject);
        repaint();
    }

    ObjectReferenceView referenceView;
    String currentObject;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReferencePanel)
};
