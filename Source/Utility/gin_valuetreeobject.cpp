/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/
#include "gin_valuetreeobject.h"

ValueTreeObject::ValueTreeObject (const juce::ValueTree& state_)
  : state (state_)
{


    state.addListener (this);
}

void ValueTreeObject::valueTreePropertyChanged (juce::ValueTree& p, const juce::Identifier& i)
{
    ignoreUnused (p, i);
    valueTreeChanged();
}

void ValueTreeObject::valueTreeChildAdded (juce::ValueTree& p, juce::ValueTree& c)
{
    if (p == state)
    {
        if (auto* newObj = factory (c.getType(), c))
        {
            newObj->parent = this;
            children.insert (p.indexOf (c), newObj);
        }
        else
        {
            jassertfalse; // type missing in factory
        }
    }
    valueTreeChanged();
}

void ValueTreeObject::valueTreeChildRemoved (juce::ValueTree& p, juce::ValueTree& c, int i)
{
    ignoreUnused (c);

    if (p == state)
        children.remove (i);
    
    valueTreeChanged();
}

void ValueTreeObject::valueTreeChildOrderChanged (juce::ValueTree& p, int oldIndex, int newIndex)
{
    if (p == state)
        children.move (oldIndex, newIndex);
    
    valueTreeChanged();
}

void ValueTreeObject::valueTreeParentChanged (juce::ValueTree&)
{
    valueTreeChanged();
}

void ValueTreeObject::valueTreeRedirected (juce::ValueTree&)
{
    valueTreeChanged();
}
