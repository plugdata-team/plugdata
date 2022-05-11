
struct GraphicalArray : public Component, public Timer
{
   public:
    Box* box;

    GraphicalArray(PlugDataAudioProcessor* instance, pd::Array& graph, Box* parent) : array(graph), edited(false), pd(instance), box(parent)
    {
        if (graph.getName().empty()) return;

        vec.reserve(8192);
        temp.reserve(8192);
        try
        {
            array.read(vec);
        }
        catch (...)
        {
            error = true;
        }
        startTimer(100);
        setInterceptsMouseClicks(true, false);
        setOpaque(false);
    }

    void paint(Graphics& g) override
    {
        g.setColour(box->findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        if (error)
        {
            g.drawText("array " + array.getName() + " is invalid", 0, 0, getWidth(), getHeight(), Justification::centred);
        }
        else
        {
            const auto h = static_cast<float>(getHeight());
            const auto w = static_cast<float>(getWidth());
            if (!vec.empty())
            {
                const std::array<float, 2> scale = array.getScale();
                if (array.isDrawingCurve())
                {
                    const float dh = h / (scale[1] - scale[0]);
                    const float dw = w / static_cast<float>(vec.size() - 1);
                    Path p;
                    p.startNewSubPath(0, h - (std::clamp(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                    for (size_t i = 1; i < vec.size() - 1; i += 2)
                    {
                        const float y1 = h - (std::clamp(vec[i - 1], scale[0], scale[1]) - scale[0]) * dh;
                        const float y2 = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                        const float y3 = h - (std::clamp(vec[i + 1], scale[0], scale[1]) - scale[0]) * dh;
                        p.cubicTo(static_cast<float>(i - 1) * dw, y1, static_cast<float>(i) * dw, y2, static_cast<float>(i + 1) * dw, y3);
                    }
                    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
                    g.strokePath(p, PathStrokeType(1));
                }
                else if (array.isDrawingLine())
                {
                    const float dh = h / (scale[1] - scale[0]);
                    const float dw = w / static_cast<float>(vec.size() - 1);
                    Path p;
                    p.startNewSubPath(0, h - (std::clamp(vec[0], scale[0], scale[1]) - scale[0]) * dh);
                    for (size_t i = 1; i < vec.size(); ++i)
                    {
                        const float y = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                        p.lineTo(static_cast<float>(i) * dw, y);
                    }
                    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
                    g.strokePath(p, PathStrokeType(1));
                }
                else
                {
                    const float dh = h / (scale[1] - scale[0]);
                    const float dw = w / static_cast<float>(vec.size());
                    g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
                    for (size_t i = 0; i < vec.size(); ++i)
                    {
                        const float y = h - (std::clamp(vec[i], scale[0], scale[1]) - scale[0]) * dh;
                        g.drawLine(static_cast<float>(i) * dw, y, static_cast<float>(i + 1) * dw, y);
                    }
                }
            }
        }
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (error) return;
        edited = true;

        const auto s = static_cast<float>(vec.size() - 1);
        const auto w = static_cast<float>(getWidth());
        const auto x = static_cast<float>(e.x);

        lastIndex = static_cast<size_t>(std::round(std::clamp(x / w, 0.f, 1.f) * s));

        mouseDrag(e);
    }

    void mouseDrag(const MouseEvent& e) override
    {
        if (error) return;
        const auto s = static_cast<float>(vec.size() - 1);
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());
        const auto x = static_cast<float>(e.x);
        const auto y = static_cast<float>(e.y);

        const std::array<float, 2> scale = array.getScale();
        const int index = static_cast<int>(std::round(std::clamp(x / w, 0.f, 1.f) * s));

        float start = vec[lastIndex];
        float current = (1.f - std::clamp(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];

        int interpStart = std::min(index, lastIndex);
        int interpEnd = std::max(index, lastIndex);

        float min = index == interpStart ? current : start;
        float max = index == interpStart ? start : current;

        // const CriticalSection* cs = pd->getCallbackLock();

        // Fix to make sure we don't leave any gaps while dragging
        for (int n = interpStart; n <= interpEnd; n++)
        {
            vec[n] = jmap<float>(n, interpStart, interpEnd + 1, min, max);
        }

        // Don't want to touch vec on the other thread, so we copy the vector into the lambda
        auto changed = std::vector<float>(vec.begin() + interpStart, vec.begin() + interpEnd);

        pd->enqueueFunction(
            [this, interpStart, changed]() mutable
            {
                try
                {
                    for (int n = 0; n < changed.size(); n++)
                    {
                        array.write(interpStart + n, changed[n]);
                    }
                }
                catch (...)
                {
                    error = true;
                }
            });

        lastIndex = index;

        pd->enqueueMessages(stringArray, array.getName(), {});
        repaint();
    }

    void mouseUp(const MouseEvent& e) override
    {
        if (error) return;
        edited = false;
    }

    void timerCallback() override
    {
        if (!edited)
        {
            error = false;
            try
            {
                array.read(temp);
            }
            catch (...)
            {
                error = true;
            }
            if (temp != vec)
            {
                vec.swap(temp);
                repaint();
            }
        }
    }

    size_t getArraySize() const noexcept
    {
        return vec.size();
    }

    pd::Array array;
    std::vector<float> vec;
    std::vector<float> temp;
    std::atomic<bool> edited;
    bool error = false;
    const std::string stringArray = std::string("array");

    int lastIndex = 0;

    PlugDataAudioProcessor* pd;
};

struct ArrayComponent : public GUIComponent
{
   public:
    // Array component
    ArrayComponent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject), graph(gui.getArray()), array(box->cnv->pd, graph, box)
    {
        setInterceptsMouseClicks(false, true);
        array.setBounds(getLocalBounds());
        addAndMakeVisible(&array);

        initialise(newObject);
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(100, maxSize, box->getWidth());
        int h = jlimit(40, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void resized() override
    {
        array.setBounds(getLocalBounds());
    }

   private:
    pd::Array graph;
    GraphicalArray array;
};
