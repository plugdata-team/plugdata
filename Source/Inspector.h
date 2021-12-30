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

#include "LookAndFeel.h"
#include <JuceHeader.h>

enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool
};

using ObjectParameter = std::tuple<String, ParameterType, void*>; // name, type and pointer to value
using ObjectParameters = std::pair<std::vector<ObjectParameter>, std::function<void(int)>>;

//==============================================================================
/**
    This class shows how to implement a TableListBoxModel to show in a TableListBox.
*/


// TODO: this currently uses XML for no reason other then me being too lazy to change the example
// fix in the future

struct Inspector : public Component,
                   public TableListBoxModel {
    //==============================================================================
    Inspector()
        : font(14.0f)
    {
        auto* nameColumn = new XmlElement("COLUMN");
        nameColumn->setAttribute("columnId", "1");
        nameColumn->setAttribute("name", "Name");
        nameColumn->setAttribute("width", "50");

        auto* valueColumn = new XmlElement("COLUMN");
        valueColumn->setAttribute("columnId", "2");
        valueColumn->setAttribute("name", "Value");
        valueColumn->setAttribute("width", "80");

        columnList = new XmlElement("HEADERS");
        columnList->addChildElement(nameColumn);
        columnList->addChildElement(valueColumn);

        dataList = new XmlElement("DATA");

        // Load some data from an embedded XML file..
        loadData({});

        // Create our table component and add it to this component..
        addAndMakeVisible(&table);
        table.setModel(this);

        // give it a border
        table.setColour(ListBox::outlineColourId, Colours::transparentBlack);
        table.setColour(ListBox::textColourId, Colours::white);

        table.setOutlineThickness(1);

        
        
        // Add some columns to the table header, based on the column list in our database..
        forEachXmlChildElement(*columnList, columnXml)
        {
            table.getHeader().addColumn(columnXml->getStringAttribute("name"),
                columnXml->getIntAttribute("columnId"),
                columnXml->getIntAttribute("width"),
                50, 400,
                TableHeaderComponent::defaultFlags);
        }

        setColour(ListBox::textColourId, Colours::white);
        setColour(ListBox::outlineColourId, Colours::white);
        // setColour(ListBox::outlineColourId, Colours::white);

        // we could now change some initial settings..
        table.getHeader().setSortColumnId(1, true); // sort forwards by the ID column
        table.getHeader().setColumnVisible(7, false); // hide the "length" column until the user shows it

        // un-comment this line to have a go of stretch-to-fit mode
        table.getHeader().setStretchToFitActive(true);

        table.getHeader().setColour(TableHeaderComponent::textColourId, Colours::white);
        table.getHeader().setColour(TableHeaderComponent::backgroundColourId, MainLook::highlightColour);

        table.setMultipleSelectionEnabled(true);
    }

    ~Inspector()
    {
        delete columnList;
        delete dataList;
    }

    //==============================================================================
    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
    int getNumRows()
    {
        return numRows;
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected)
    {
        if (rowIsSelected) {
            g.fillAll(MainLook::highlightColour);
        } else {
            g.fillAll((row % 2) ? MainLook::firstBackground : MainLook::secondBackground);
        }
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g,
        int rowNumber,
        int columnId,
        int width, int height,
        bool /*rowIsSelected*/)
    {
        g.setColour(Colours::white);
        g.setFont(font);

        const XmlElement* rowElement = dataList->getChildElement(rowNumber);

        if (rowElement != 0) {
            const String text(rowElement->getStringAttribute(getAttributeNameForColumnId(columnId)));

            g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(width - 1, 0, 1, height);
    }

    // This is overloaded from TableListBoxModel, and tells us that the user has clicked a table header
    // to change the sort order.
    void sortOrderChanged(int newSortColumnId, bool isForwards)
    {
        if (newSortColumnId != 0) {
            DataSorter sorter(getAttributeNameForColumnId(newSortColumnId), isForwards);
            dataList->sortChildElements(sorter);

            table.updateContent();
        }
    }

    void* toPointer(String text) {
#if _MSC_VER

#else

#endif

    }
    // This is overloaded from TableListBoxModel, and must update any custom components that we're using
    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
        Component* existingComponentToUpdate)
    {
        delete existingComponentToUpdate;

        // Draw names regularly
        if (columnId == 1)
            return 0;

        auto type = dataList->getChildElement(rowNumber)->getIntAttribute("Type");
        auto value = dataList->getChildElement(rowNumber)->getStringAttribute("Value");
        auto* ptr = data[rowNumber];

        switch (type) {
        case tString:
            return new EditableComponent<String>(callback, (String*)ptr, rowNumber);
            break;
        case tFloat:
            return new EditableComponent<float>(callback, (float*)ptr, rowNumber);
            break;
        case tInt:
            return new EditableComponent<int>(callback, (int*)ptr, rowNumber);
            break;
        case tColour:
            return new ColourComponent(callback, (String*)ptr, rowNumber);
            break;
        case tBool:
            return new ToggleComponent(callback, (bool*)ptr, rowNumber);
            break;
        }

        // for any other column, just return 0, as we'll be painting these columns directly.
        jassert(existingComponentToUpdate == 0);
        return 0;
    }

    // This is overloaded from TableListBoxModel, and should choose the best width for the specified
    // column.
    int getColumnAutoSizeWidth(int columnId)
    {
        if (columnId == 5)
            return 100; // (this is the ratings column, containing a custom component)

        int widest = 32;

        // find the widest bit of text in this column..
        for (int i = getNumRows(); --i >= 0;) {
            const XmlElement* rowElement = dataList->getChildElement(i);

            if (rowElement != 0) {
                const String text(rowElement->getStringAttribute(getAttributeNameForColumnId(columnId)));

                widest = jmax(widest, font.getStringWidth(text));
            }
        }

        return widest + 8;
    }

    struct DataSorter {
        DataSorter(const juce::String& attributeToSortBy, bool forwards)
            : attributeToSort(attributeToSortBy)
            , direction(forwards ? 1 : -1)
        {
        }

        int compareElements(juce::XmlElement* first, juce::XmlElement* second) const
        {
            auto result = first->getStringAttribute(attributeToSort)
                              .compareNatural(second->getStringAttribute(attributeToSort)); // [1]

            if (result == 0)
                result = first->getStringAttribute("ID")
                             .compareNatural(second->getStringAttribute("ID")); // [2]

            return direction * result; // [3]
        }

    private:
        String attributeToSort;
        int direction;
    };

    //==============================================================================
    void resized()
    {
        // position our table with a gap around its edge
        table.setBounds(getLocalBounds().expanded(2));
    }

    void deselect()
    {
        loadData({});
    }

    void loadData(ObjectParameters params)
    {
        dataList->deleteAllChildElements();

        auto& [parameters, cb] = params;

        callback = cb;

        data.clear();

        for (auto& [name, type, ptr] : parameters) {

            auto* dataElement = new XmlElement("DATA");
            dataElement->setAttribute("Name", name);
            dataElement->setAttribute("Type", type);
            data.push_back(ptr);

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
            case tBool:
                dataElement->setAttribute("Value", *(bool*)ptr);
            }

            dataList->addChildElement(dataElement);
        }

        numRows = dataList->getNumChildElements();
        table.updateContent();
        repaint();
    }

    TableListBox table; // the table component itself
    Font font;

    XmlElement* columnList;
    XmlElement* dataList;

    std::vector<void*> data;
    std::function<void(int)> callback;

    int numRows; // The number of rows of data we've got

    struct ToggleComponent : public Component {
        ToggleComponent(std::function<void(int)> cb, bool* value, int rowIdx)
            : callback(cb)
            , row(rowIdx)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setToggleState(*value, sendNotification);
            toggleButton.setButtonText((*value) ? "true" : "false");
            toggleButton.setConnectedEdges(12);

            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this, value]() {
                *value = toggleButton.getToggleState();
                toggleButton.setButtonText((*value) ? "true" : "false");
                callback(row);
            };
        }

        void resized()
        {
            toggleButton.setBounds(getLocalBounds());
        }

    private:
        std::function<void(int)> callback;
        int row;
        TextButton toggleButton;
    };

    struct ColourComponent : public Component, public ChangeListener {

        ColourComponent(std::function<void(int)> cb, String* value, int rowIdx)
            : callback(cb)
            , currentColour(value)
            , row(rowIdx)
        {
            if (value && value->length() > 2) {
                button.setButtonText(String("#") + (*value).substring(2));
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            updateColour();

            button.onClick = [this]() {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, MainLook::firstBackground);

                colourSelector->setCurrentColour(Colour::fromString(*currentColour));

                CallOutBox::launchAsynchronously(std::move(colourSelector), button.getScreenBounds(), nullptr);
            };
        }

        void updateColour()
        {

            auto colour = Colour::fromString(*currentColour);

            button.setColour(TextButton::buttonColourId, colour);
            button.setColour(TextButton::buttonOnColourId, colour.brighter());

            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;

            // make sure text is readable
            button.setColour(TextButton::textColourOffId, textColour);
            button.setColour(TextButton::textColourOnId, textColour);

            button.setButtonText(String("#") + (*currentColour).substring(2));
        }

        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            ColourSelector* cs = dynamic_cast<ColourSelector*>(source);

            auto colour = cs->getCurrentColour();
            *currentColour = colour.toString();

            updateColour();
            callback(row);
        }

        ~ColourComponent()
        {
        }

        void resized()
        {
            button.setBounds(getLocalBounds());
        }

    private:
        std::function<void(int)> callback;
        TextButton button;
        String* currentColour;

        int row;
    };

    template <typename T>
    struct EditableComponent : public Label {
        EditableComponent(std::function<void(int)> cb, T* value, int rowIdx)
            : callback(cb)
            , row(rowIdx)
        {
            setEditable(false, true);

            setText(String(*value), dontSendNotification);

            onTextChange = [this, value]() {
                if constexpr (std::is_floating_point<T>::value) {
                    *value = getText().getFloatValue();
                } else if constexpr (std::is_integral<T>::value) {
                    *value = getText().getIntValue();
                } else {
                    *value = getText();
                }

                callback(row);
            };
        }

        ~EditableComponent()
        {
        }

        TextEditor* createEditorComponent() override
        {
            auto* editor = Label::createEditorComponent();

            if constexpr (std::is_floating_point<T>::value) {
                editor->setInputRestrictions(0, "0123456789.-");
            } else if constexpr (std::is_integral<T>::value) {
                editor->setInputRestrictions(0, "0123456789");
            }

            return editor;
        }

    private:
        std::function<void(int)> callback;

        int row;
    };

private:
    // (a utility method to search our XML for the attribute that matches a column ID)
    const String getAttributeNameForColumnId(const int columnId) const
    {
        forEachXmlChildElement(*columnList, columnXml)
        {
            if (columnXml->getIntAttribute("columnId") == columnId)
                return columnXml->getStringAttribute("name");
        }

        return String();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Inspector)
};
