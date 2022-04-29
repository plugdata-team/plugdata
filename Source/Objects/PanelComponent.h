
struct PanelComponent : public GUIComponent
{
    PanelComponent(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
        box->setColour(PlugDataColour::canvasOutlineColourId, Colours::transparentBlack);
        initialise(newObject);
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(20, maxSize, box->getWidth());
        int h = jlimit(20, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
    }

    void updateValue() override{};

    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        params.push_back({"Label", tString, cLabel, &labelText, {}});
        params.push_back({"Label Colour", tColour, cLabel, &labelColour, {}});
        params.push_back({"Label X", tInt, cLabel, &labelX, {}});
        params.push_back({"Label Y", tInt, cLabel, &labelY, {}});
        params.push_back({"Label Height", tInt, cLabel, &labelHeight, {}});
        return params;
    }
};
