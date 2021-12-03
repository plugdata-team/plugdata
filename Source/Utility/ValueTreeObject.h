/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#pragma once

//==============================================================================
/** Mirrors a ValueTree in Objects
 */

#include <JuceHeader.h>


class ValueTreeObject : public ValueTree::Listener
{
public:
    ValueTreeObject (const ValueTree& state);

    ValueTree& getState() { return state; }
        
    virtual ~ValueTreeObject() {};
    
    virtual ValueTreeObject* factory(const Identifier&, const ValueTree&) { return nullptr; };

public:
    const OwnedArray<ValueTreeObject>& getChildren() const    { return children; }

    template <class TargetClass>
    TargetClass* findParentOfType() const
    {
        auto* p = parent;
        while (p != nullptr)
        {
            if (auto* res = dynamic_cast<TargetClass*> (parent))
                return res;

            p = p->parent;
        }

        return nullptr;
    }

    template <class TargetClass>
    Array<TargetClass*> findChildrenOfClass(bool recursive = false) const
    {
        Array<TargetClass*> res;

        for (auto* c : children) {
            if (auto* t = dynamic_cast<TargetClass*> (c))
                res.add (t);
            
            if(recursive)  {
                auto r = c->findChildrenOfClass<TargetClass>(true);
                res.addArray(r);
            }
        }

        return res;
    }

    template <class TargetClass>
    int countChildrenOfClass() const
    {
        int count = 0;

        for (auto* c : children)
            if (auto* t = dynamic_cast<TargetClass*> (c))
                count++;

        return count;
    }

    template <class TargetClass>
    TargetClass* findChildOfClass (int idx) const
    {
        int count = 0;

        for (auto* c : children)
        {
            if (auto* t = dynamic_cast<TargetClass*> (c))
            {
                if (count == idx)
                    return t;

                count++;
            }
        }

        return nullptr;
    }
    
    void rebuildObjects() {
        for (auto c : state)
        {
            if (auto* newObj = factory (c.getType(), c))
            {
                newObj->parent = this;
                children.add (newObj);
            }
            else
            {
                jassertfalse; // type missing in factory
            }
        }
    }
    
    template <class TargetClass>
    TargetClass* findObjectForTree(ValueTree object) {
        for(auto& child : children) {
            if(child->getState() == object) {
                return dynamic_cast<TargetClass*>(child);
            }
        }
        return nullptr;
    }
    
    // A bunch of functions to easily access the valuetree
    
    const var& getProperty (const Identifier& name) const noexcept
    {
        return state.getProperty(name);
    }

    ValueTree& setProperty (const Identifier& name, const var& newValue)
    {
        return state.setProperty(name, newValue, undoManager);
    }
    
    ValueTree getChildWithName (const Identifier& type) const
    {
        return state.getChildWithName(type);
    }

    ValueTree getOrCreateChildWithName (const Identifier& type)
    {
        return state.getOrCreateChildWithName(type, undoManager);
    }

    ValueTree getChildWithProperty (const Identifier& propertyName, const var& propertyValue) const
    {
        return state.getChildWithProperty(propertyName, propertyValue);
    }

    bool isAChildOf (const ValueTree& possibleParent) const noexcept
    {
        return state.isAChildOf(possibleParent);
    }

    int indexOf (const ValueTree& child) const noexcept
    {
        return state.indexOf(child);
    }

    void addChild (const ValueTree& child, int index)
    {
        state.addChild(child, index, undoManager);
    }

    void appendChild (const ValueTree& child)
    {
        addChild(child, -1);
    }

    void removeChild (int childIndex)
    {
        state.removeChild(childIndex, undoManager);
    }

    void removeChild (const ValueTree& child)
    {
        state.removeChild(child, undoManager);
    }

    void removeAllChildren()
    {
        state.removeAllChildren(undoManager);
    }

    void moveChild (int currentIndex, int newIndex)
    {
        state.moveChild(currentIndex, newIndex, undoManager);
    }
    
    ValueTree getChild (int index) const
    {
        return state.getChild(index);
    }
    
    ValueTree& setPropertyExcludingListener (ValueTree::Listener* listenerToExclude, const Identifier& name,
                                                        const var& newValue)
    {
        return state.setPropertyExcludingListener(listenerToExclude, name, newValue, undoManager);
    }
    

    ValueTree getParent() const noexcept
    {
        return state.getParent();
    }
    
    void sendPropertyChangeMessage (const Identifier& property)
    {
        state.sendPropertyChangeMessage(property);
    }
    
    bool hasProperty (const Identifier& name) const noexcept
    {
        return state.hasProperty(name);
    }
    
    UndoManager* undoManager = nullptr;
    

private:
    void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override;
    void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override;
    
    virtual void valueTreeChanged() {};

    
    ValueTree state;
    ValueTreeObject* parent = nullptr;

    OwnedArray<ValueTreeObject> children;
    
private:
    
    JUCE_LEAK_DETECTOR (ValueTreeObject)
};
