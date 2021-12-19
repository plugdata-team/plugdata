/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-9 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LookAndFeel.h"

enum ParameterType
{
    tString,
    tInt,
    tFloat,
    tColour
};

using ObjectParameter = std::tuple<String, ParameterType, void*>;  // name, type and pointer to value
using ObjectParameters = std::pair<std::vector<ObjectParameter>, std::function<void(int)>>;


//==============================================================================
/**
    This class shows how to implement a TableListBoxModel to show in a TableListBox.
*/


class Inspector    : public Component,
                              public TableListBoxModel
{
public:
    //==============================================================================
    Inspector()
        : font (14.0f)
    {
        // Load some data from an embedded XML file..
        loadData({});

        // Create our table component and add it to this component..
        addAndMakeVisible (&table);
        table.setModel (this);

        // give it a border
        table.setColour (ListBox::outlineColourId, Colours::grey);
        table.setOutlineThickness (1);

        // Add some columns to the table header, based on the column list in our database..
        forEachXmlChildElement (*columnList, columnXml)
        {
            table.getHeader().addColumn (columnXml->getStringAttribute ("name"),
                                         columnXml->getIntAttribute ("columnId"),
                                         columnXml->getIntAttribute ("width"),
                                         50, 400,
                                         TableHeaderComponent::defaultFlags);
        }
        
        setColour(ListBox::textColourId, Colours::white);
        setColour(ListBox::outlineColourId, Colours::white);
        //setColour(ListBox::outlineColourId, Colours::white);

        // we could now change some initial settings..
        table.getHeader().setSortColumnId (1, true); // sort forwards by the ID column
        table.getHeader().setColumnVisible (7, false); // hide the "length" column until the user shows it

        // un-comment this line to have a go of stretch-to-fit mode
        table.getHeader().setStretchToFitActive (true);

        table.setMultipleSelectionEnabled (true);
    }

    ~Inspector()
    {
    }

    //==============================================================================
    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
    int getNumRows()
    {
        return numRows;
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground (Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected)
    {
        if (rowIsSelected)
            g.fillAll (MainLook::highlightColour);
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell (Graphics& g,
                    int rowNumber,
                    int columnId,
                    int width, int height,
                    bool /*rowIsSelected*/)
    {
        g.setColour (Colours::white);
        g.setFont (font);

        const XmlElement* rowElement = dataList->getChildElement (rowNumber);

        if (rowElement != 0)
        {
            const String text (rowElement->getStringAttribute (getAttributeNameForColumnId (columnId)));

            g.drawText (text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }

        g.setColour (Colours::black.withAlpha (0.2f));
        g.fillRect (width - 1, 0, 1, height);
    }

    // This is overloaded from TableListBoxModel, and tells us that the user has clicked a table header
    // to change the sort order.
    void sortOrderChanged (int newSortColumnId, bool isForwards)
    {
        if (newSortColumnId != 0)
        {
            //DemoDataSorter sorter (getAttributeNameForColumnId (newSortColumnId), isForwards);
            //dataList->sortChildElements (sorter);

            table.updateContent();
        }
    }

    // This is overloaded from TableListBoxModel, and must update any custom components that we're using
    Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                        Component* existingComponentToUpdate)
    {
        // Draw names regularly
        if(columnId == 1) return 0;
        
        auto type = dataList->getChildElement(rowNumber)->getIntAttribute("Type");
        auto value = dataList->getChildElement(rowNumber)->getStringAttribute("Value");
        auto* ptr = (void*)dataList->getChildElement(rowNumber)->getStringAttribute("Pointer").getLargeIntValue();
        
        
        
        switch (type) {
            case tString:
                return new EditableComponent<String>(*this, (String*)ptr,rowNumber);
                break;
            case tFloat:
                return new EditableComponent<float>(*this, (float*)ptr, rowNumber);
                break;
            case tInt:
                return new EditableComponent<int>(*this, (int*)ptr, rowNumber);
                break;
            case tColour:
                return new ColourComponent(*this, (String*)ptr, rowNumber);
                break;
        }
        
        // for any other column, just return 0, as we'll be painting these columns directly.
        jassert (existingComponentToUpdate == 0);
        return 0;
    }

    // This is overloaded from TableListBoxModel, and should choose the best width for the specified
    // column.
    int getColumnAutoSizeWidth (int columnId)
    {
        if (columnId == 5)
            return 100; // (this is the ratings column, containing a custom component)

        int widest = 32;

        // find the widest bit of text in this column..
        for (int i = getNumRows(); --i >= 0;)
        {
            const XmlElement* rowElement = dataList->getChildElement (i);

            if (rowElement != 0)
            {
                const String text (rowElement->getStringAttribute (getAttributeNameForColumnId (columnId)));

                widest = jmax (widest, font.getStringWidth (text));
            }
        }

        return widest + 8;
    }

    // A couple of quick methods to set and get the "rating" value when the user
    // changes the combo box
    int getRating (const int rowNumber) const
    {
        return dataList->getChildElement (rowNumber)->getIntAttribute ("Rating");
    }

    void setRating (const int rowNumber, const int newRating)
    {
        dataList->getChildElement (rowNumber)->setAttribute ("Rating", newRating);
    }

    //==============================================================================
    void resized()
    {
        // position our table with a gap around its edge
        table.setBounds(getLocalBounds());
    }

    void loadData(ObjectParameters params)
    {
        
        //XmlDocument dataDoc (String ((const char*) BinaryData::demo_table_data_xml));
        //demoData = dataDoc.getDocumentElement();
        
        auto* nameColumn = new XmlElement("COLUMN");
        nameColumn->setAttribute("columnId", "1");
        nameColumn->setAttribute("name", "Name");
        nameColumn->setAttribute("width", "50");
    
        auto* valueColumn = new XmlElement("COLUMN");
        valueColumn->setAttribute("columnId", "2");
        valueColumn->setAttribute("name", "Value");
        valueColumn->setAttribute("width", "80");
        
        auto* columns = new XmlElement("HEADERS");
        columns->addChildElement(nameColumn);
        columns->addChildElement(valueColumn);
        
        auto* rows = new XmlElement("DATA");
        auto& [parameters, cb] = params;
        
        callback = cb;
        
        for(auto& [name, type, ptr] : parameters) {
            
            auto* dataElement = new XmlElement("DATA");
            dataElement->setAttribute("Name", name);
            
    
            dataElement->setAttribute("Type", type);
            dataElement->setAttribute("Pointer", String((long)ptr));
            
            switch (type) {
                case tColour:
                dataElement->setAttribute("Value", *(String*)ptr);
                    break;
                case tFloat:
                dataElement->setAttribute("Value", *((float*)ptr));
                    break;
                case tInt:
                dataElement->setAttribute("Value", *((int*)ptr));
                    break;
                case tString:
                dataElement->setAttribute("Value", *(String*)ptr);
                    break;
            }
            
            
            rows->addChildElement(dataElement);
        }
        
        dataList = rows;
        columnList = columns;

        numRows = dataList->getNumChildElements();
        table.updateContent();
        repaint();
    }
    

private:
    TableListBox table;     // the table component itself
    Font font;

    ScopedPointer<XmlElement> demoData;   // This is the XML document loaded from the embedded file "demo table data.xml"
    XmlElement* columnList; // A pointer to the sub-node of demoData that contains the list of columns
    XmlElement* dataList;   // A pointer to the sub-node of demoData that contains the list of data rows
    std::function<void(int)> callback;
    
    int numRows;            // The number of rows of data we've got


    class ColourComponent    : public Component, public ChangeListener
    {
        

    public:
        ColourComponent (Inspector& owner_, String* value, int rowIdx)
        : owner (owner_), currentColour(value), row(rowIdx)
        {
            button.setButtonText(*value);
            
            addAndMakeVisible(button);
            
            button.onClick = [this]() {
                
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>();
                colourSelector->setName ("background");
                colourSelector->setCurrentColour (findColour (TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize (300, 400);
                
                CallOutBox::launchAsynchronously (std::move(colourSelector), button.getScreenBounds(), nullptr);
                
            };
        }
        
        void changeListenerCallback (ChangeBroadcaster* source) override
        {
            ColourSelector* cs = dynamic_cast <ColourSelector*> (source);
            *currentColour = cs->getCurrentColour().toString();
            owner.callback(row);
        }

        ~ColourComponent()
        {
        }

        void resized()
        {
            button.setBounds(getLocalBounds());
        }



    private:
        Inspector& owner;
        TextButton button;
        String* currentColour;
        
        int row;
    };

    template<typename T>
    class EditableComponent : public Label
    {
    public:
        EditableComponent (Inspector& owner_, T* value, int rowIdx)
            : owner (owner_), row(rowIdx)
        {
            setEditable(false, true);
            setText(String(*value), dontSendNotification);
            
            
            onTextChange = [this, value]() {
                
                if constexpr (std::is_floating_point<T>::value) {
                    *value = getText().getFloatValue();
                }
                else if constexpr (std::is_integral<T>::value) {
                    *value = getText().getIntValue();
                }
                else {
                    *value = getText();
                }
                
                
                owner.callback(row);
                
            };
            
            
        }

        ~EditableComponent()
        {
        }

        TextEditor* createEditorComponent() override {
            TextEditor* editor = new TextEditor;
            
            if constexpr (std::is_floating_point<T>::value) {
                editor->setInputRestrictions(0, "0123456789.-");
            }
            else if constexpr (std::is_integral<T>::value) {
                editor->setInputRestrictions(0, "0123456789");
            }
            
            return editor;
        }


    private:
        Inspector& owner;

        int row;
    };
    
    /*
    class DemoDataSorter
    {
    public:
        DemoDataSorter (const String attributeToSort_, bool forwards)
            : attributeToSort (attributeToSort_),
              direction (forwards ? 1 : -1)
        {
        }

        int compareElements (XmlElement* first, XmlElement* second) const
        {
            int result = first->getStringAttribute (attributeToSort)
                           .compareLexicographically (second->getStringAttribute (attributeToSort));

            if (result == 0)
                result = first->getStringAttribute ("ID")
                           .compareLexicographically (second->getStringAttribute ("ID"));

            return direction * result;
        }

    private:
        String attributeToSort;
        int direction;
    }; */



    // (a utility method to search our XML for the attribute that matches a column ID)
    const String getAttributeNameForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("name");
        }

        return String();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Inspector)
};
