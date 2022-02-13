/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <JuceHeader.h>

enum ParameterType
{
    tString,
    tInt,
    tFloat,
    tColour,
    tBool
};

using ObjectParameter = std::tuple<String, ParameterType, void*>;  // name, type and pointer to value

using ObjectParameters = std::pair<std::vector<ObjectParameter>, std::function<void(int)>>;  // List of elements and update function

struct Inspector : public Component, public TableListBoxModel
{
    Inspector() : font(14.0f), numRows(0)
    {
        loadData({});

        // Create our table component and add it to this component.
        addAndMakeVisible(&table);
        table.setModel(this);

        // give it a border
        table.setColour(ListBox::outlineColourId, Colours::transparentBlack);
        table.setColour(ListBox::textColourId, Colours::white);

        table.setOutlineThickness(1);

        table.getHeader().addColumn("Name", 1, 50, 30, -1, TableHeaderComponent::notSortable);
        table.getHeader().addColumn("Value", 2, 80, 30, -1, TableHeaderComponent::notSortable);
        table.setHeaderHeight(25);
        
        setColour(ListBox::textColourId, Colours::white);
        setColour(ListBox::outlineColourId, Colours::white);

        table.getHeader().setStretchToFitActive(true);

        table.getHeader().setColour(TableHeaderComponent::textColourId, Colours::white);
        table.getHeader().setColour(TableHeaderComponent::backgroundColourId, findColour(Slider::thumbColourId));

        table.setMultipleSelectionEnabled(true);
    }

    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
    int getNumRows() override
    {
        return numRows;
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected)
        {
            g.fillAll(findColour(Slider::thumbColourId));
        }
        else
        {
            g.fillAll((row % 2) ? findColour(ComboBox::backgroundColourId) : findColour(ResizableWindow::backgroundColourId));
        }
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
    {
        g.setColour(Colours::white);
        g.setFont(font);

        if (rowNumber < items.size())
        {
            const auto [name, type, ptr] = items[rowNumber];
            g.drawText(name, 6, 0, width - 4, height, Justification::centredLeft, true);
        }

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(width - 1, 0, 1, height);
    }

    // Sorting is disabled
    void sortOrderChanged(int newSortColumnId, bool isForwards) override
    {
    }

    // This is overloaded from TableListBoxModel, and must update any custom components that we're using
    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;

        // Draw names regularly
        if (columnId == 1) return nullptr;

        auto [name, type, ptr] = items[rowNumber];

        switch (type)
        {
            case tString:
                return new EditableComponent<String>(callback, static_cast<String*>(ptr), rowNumber);
            case tFloat:
                return new EditableComponent<float>(callback, static_cast<float*>(ptr), rowNumber);
            case tInt:
                return new EditableComponent<int>(callback, static_cast<int*>(ptr), rowNumber);
            case tColour:
                return new ColourComponent(callback, static_cast<String*>(ptr), rowNumber);
            case tBool:
                return new ToggleComponent(callback, static_cast<bool*>(ptr), rowNumber);
        }

        // for any other column, just return 0, as we'll be painting these columns directly.
        jassert(existingComponentToUpdate == nullptr);
        return nullptr;
    }

    // This is overloaded from TableListBoxModel, and should choose the best width for the specified
    // column.
    int getColumnAutoSizeWidth(int columnId) override
    {
        if (columnId == 1)
        {
            return getWidth() / 3;  // Name column
        }
        else
        {
            return static_cast<int>(getWidth() * 1.5f);  // Value column
        }
    }

    void resized() override
    {
        table.setBounds(getLocalBounds().withWidth(getWidth() + 2));
    }

    void deselect()
    {
        loadData({});
    }

    void loadData(const ObjectParameters& params)
    {
        auto& [parameters, cb] = params;

        callback = cb;
        items = parameters;

        numRows = static_cast<int>(items.size());
        table.updateContent();
        repaint();
    }

    TableListBox table;  // the table component itself
    Font font;

    std::vector<ObjectParameter> items;  // Currently loaded items in inspector
    std::function<void(int)> callback;   // Callback when param changes, with int argument

    int numRows;  // The number of rows of data we've got

    struct ToggleComponent : public Component
    {
        ToggleComponent(std::function<void(int)> cb, bool* value, int rowIdx) : callback(std::move(cb)), row(rowIdx)
        {
            toggleButton.setClickingTogglesState(true);

            toggleButton.setToggleState(*value, sendNotification);
            toggleButton.setButtonText((*value) ? "true" : "false");
            toggleButton.setConnectedEdges(12);

            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this, value]()
            {
                *value = toggleButton.getToggleState();
                toggleButton.setButtonText((*value) ? "true" : "false");
                callback(row);
            };
        }

        void resized() override
        {
            toggleButton.setBounds(getLocalBounds());
        }

       private:
        std::function<void(int)> callback;
        int row;
        TextButton toggleButton;
    };

    struct ColourComponent : public Component, public ChangeListener
    {
        ColourComponent(std::function<void(int)> cb, String* value, int rowIdx) : callback(std::move(cb)), currentColour(value), row(rowIdx)
        {
            if (value && value->length() > 2)
            {
                button.setButtonText(String("#") + (*value).substring(2));
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);

            addAndMakeVisible(button);
            updateColour();

            button.onClick = [this]()
            {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(ComboBox::backgroundColourId));

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
            auto* cs = dynamic_cast<ColourSelector*>(source);

            auto colour = cs->getCurrentColour();
            *currentColour = colour.toString();

            updateColour();
            callback(row);
        }

        ~ColourComponent() override = default;

        void resized() override
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
    struct EditableComponent : public Label
    {
        EditableComponent(std::function<void(int)> cb, T* value, int rowIdx) : callback(std::move(cb)), row(rowIdx)
        {
            setEditable(false, true);

            setText(String(*value), dontSendNotification);

            onTextChange = [this, value]()
            {
                if constexpr (std::is_floating_point<T>::value)
                {
                    *value = getText().getFloatValue();
                }
                else if constexpr (std::is_integral<T>::value)
                {
                    *value = getText().getIntValue();
                }
                else
                {
                    *value = getText();
                }

                callback(row);
            };
        }

        TextEditor* createEditorComponent() override
        {
            auto* editor = Label::createEditorComponent();

            if constexpr (std::is_floating_point<T>::value)
            {
                editor->setInputRestrictions(0, "0123456789.-");
            }
            else if constexpr (std::is_integral<T>::value)
            {
                editor->setInputRestrictions(0, "0123456789");
            }

            return editor;
        }

       private:
        std::function<void(int)> callback;

        int row;
    };

   private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Inspector)
};
