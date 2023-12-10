/*
 // Copyright (c) 2021-2022 Timothy Schoen and Pierre Guillot
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

// for objects that have a transparent background but shouldn't show the edit mode dots through the transparent areas
class ObjectBackground : private ComponentListener
{
    struct BackgroundComponent : public Component
    {
        BackgroundComponent(PlugDataColour backgroundColour) : colour(backgroundColour) {};
        
        void paint(Graphics& g) override
        {
            g.setColour(findColour(colour));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);
        }
        
        PlugDataColour colour;
    };
public:
    ObjectBackground(Object* parent, PlugDataColour colour) : object(parent), backgroundComponent(colour)
    {
        object->addComponentListener(this);
        object->cnv->addAndMakeVisible(backgroundComponent);
        backgroundComponent.setBounds(object->getBounds().reduced(Object::margin));
        backgroundComponent.toBack();
    }
    
    ~ObjectBackground() override
    {
        if(object) object->removeComponentListener(this);
    }
private:
    void componentMovedOrResized(Component& c, bool wasMoved, bool wasResized) override
    {
        backgroundComponent.setBounds(object->getBounds().reduced(Object::margin));
        backgroundComponent.toBack();
    }
    
    void componentBroughtToFront (Component& component) override
    {
        backgroundComponent.toBack();
    }
    void componentVisibilityChanged (Component& component) override
    {
        backgroundComponent.toBack();
    }

    void componentChildrenChanged (Component& component) override
    {
        backgroundComponent.toBack();
    }
    
    void componentParentHierarchyChanged (Component& component) override
    {
        backgroundComponent.toBack();
    }
    
    Component::SafePointer<Object> object;
    BackgroundComponent backgroundComponent;
};
