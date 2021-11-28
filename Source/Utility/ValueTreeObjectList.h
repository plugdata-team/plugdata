/*
  ==============================================================================

    Utilities.h
    Created: 2 Oct 2017 12:14:45pm
    Author:  David Rowland

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
template<typename ObjectType, typename CriticalSectionType = juce::DummyCriticalSection>
class ValueTreeObjectList   : public juce::ValueTree::Listener
{
public:
	ValueTreeObjectList (const juce::ValueTree& parentTree)
		: parent (parentTree)
	{
		parent.addListener (this);
	}

	virtual ~ValueTreeObjectList()
	{
		jassert (objects.size() == 0); // must call freeObjects() in the subclass destructor!
	}

	// call in the sub-class when being created
	void rebuildObjects()
	{
		jassert (objects.size() == 0); // must only call this method once at construction

		for (const auto& v : parent)
			if (isSuitableType (v))
				if (ObjectType* newObject = createNewObject (v))
				{
					objects.add (newObject);
					newObjectAdded (newObject);
				}
	}

	// call in the sub-class when being destroyed
	void freeObjects()
	{
		parent.removeListener (this);
		deleteAllObjects();
	}

	// call in the sub-class when being destroyed
	ObjectType* getObject(ValueTree v)
	{
		for (int i = 0; i < objects.size(); ++i)
			if (objects.getUnchecked(i)->state == v)
				return objects.getUnchecked(i);

		return nullptr;
	}

	//==============================================================================
	virtual bool isSuitableType (const juce::ValueTree&) const = 0;
	virtual ObjectType* createNewObject (const juce::ValueTree&) = 0;
	virtual void deleteObject (ObjectType*) = 0;

	virtual void newObjectAdded (ObjectType*) = 0;
	virtual void objectRemoved (ObjectType*) = 0;
	virtual void objectOrderChanged() = 0;

	//==============================================================================
	void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& tree) override
	{
		if (isChildTree (tree))
		{
			const int index = parent.indexOf (tree);
			(void) index;
			jassert (index >= 0);

			if (ObjectType* newObject = createNewObject (tree))
			{
				{
					const ScopedLockType sl (arrayLock);

					if (index == parent.getNumChildren() - 1)
						objects.add (newObject);
					else
						objects.addSorted (*this, newObject);
				}
				newObjectAdded (newObject);
			}
			else
				jassertfalse;
		}
	}

	void valueTreeChildRemoved (juce::ValueTree& exParent, juce::ValueTree& tree, int) override
	{
		if (parent == exParent && isSuitableType (tree))
		{
			const int oldIndex = indexOf (tree);

			if (oldIndex >= 0)
			{
				ObjectType* o;
				{
					const ScopedLockType sl (arrayLock);
					o = objects.removeAndReturn (oldIndex);
				}
				objectRemoved (o);
				deleteObject (o);
			}
		}
	}

	void valueTreeChildOrderChanged (juce::ValueTree& tree, int, int) override
	{
		if (tree == parent)
		{
			{
				const ScopedLockType sl (arrayLock);
				sortArray();
			}
			objectOrderChanged();
		}
	}

	void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override {}
	void valueTreeParentChanged (juce::ValueTree&) override {}

	void valueTreeRedirected (juce::ValueTree&) override
	{
		jassertfalse;    // may need to add handling if this is hit
	}

	juce::Array<ObjectType*> objects;
	CriticalSectionType arrayLock;
	typedef typename CriticalSectionType::ScopedLockType ScopedLockType;

protected:
	juce::ValueTree parent;

	void deleteAllObjects()
	{
		const ScopedLockType sl (arrayLock);

		while (objects.size() > 0)
			deleteObject (objects.removeAndReturn (objects.size() - 1));
	}

	bool isChildTree (juce::ValueTree& v) const
	{
		return isSuitableType (v) && v.getParent() == parent;
	}

	int indexOf (const juce::ValueTree& v) const noexcept
	{
		for (int i = 0; i < objects.size(); ++i)
			if (objects.getUnchecked(i)->state == v)
				return i;

		return -1;
	}

	void sortArray()
	{
		objects.sort (*this);
	}

public:
	int compareElements (ObjectType* first, ObjectType* second) const
	{
		int index1 = parent.indexOf (first->state);
		int index2 = parent.indexOf (second->state);
		return index1 - index2;
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeObjectList)
};
