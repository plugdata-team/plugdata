
struct CanvasObject : public IEMObject
{
    CanvasObject(void* ptr, Box* box) : IEMObject(ptr, box)
    {
        box->setColour(PlugDataColour::canvasOutlineColourId, Colours::transparentBlack);
        initialise();
    }

    void updateBounds() override
    {
        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        Rectangle<int> bounds = {x, y, static_cast<t_my_canvas*>(ptr)->x_vis_w, static_cast<t_my_canvas*>(ptr)->x_vis_h};

        box->setBounds(bounds.expanded(Box::margin));
    }

    void resized() override
    {
        static_cast<t_my_canvas*>(ptr)->x_vis_w = getWidth() - 1;
        static_cast<t_my_canvas*>(ptr)->x_vis_h = getHeight() - 1;
    }

    void checkBounds() override
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
        // TODO: why not use IEM?
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
