/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/PropertiesPanel.h"

extern "C" {
void garray_arraydialog(t_fake_garray* x, t_symbol* name, t_floatarg fsize, t_floatarg fflags, t_floatarg deleteit);
}


class GraphicalArray : public Component, public Value::Listener, public pd::MessageListener {
public:
    Object* object;

    enum DrawType {
        Points,
        Polygon,
        Curve
    };
        
    Value name = SynchronousValue();
    Value size = SynchronousValue();
    Value drawMode = SynchronousValue();
    Value saveContents = SynchronousValue();
    Value range = SynchronousValue();
    
    std::function<void()> reloadGraphs = [](){};

    GraphicalArray(PluginProcessor* instance, void* ptr, Object* parent)
        : object(parent)
        , arr(ptr, instance)
        , edited(false)
        , pd(instance)
    {
        vec.reserve(8192);
        temp.reserve(8192);
        try {
            read(vec);
        } catch (...) {
            error = true;
        }
        
        updateParameters();
        
        for(auto* value : std::vector<Value*>{&name, &size, &drawMode, &saveContents, &range})
        {
            // TODO: implement undo/redo for these values!
            value->addListener(this);
        }
        
        pd->registerMessageListener(arr.getRawUnchecked<void>(), this);

        setInterceptsMouseClicks(true, false);
        setOpaque(false);
    }
    
    ~GraphicalArray()
    {
        pd->unregisterMessageListener(arr.getRawUnchecked<void>(), this);
    }

    void setArray(void* array)
    {
        if (!array)
            return;

        // TODO: Not great design

        // Manually call destructor
        arr.~WeakReference();

        // Initialise new weakreference in place, to prevent calling copy constructor
        new (&arr) pd::WeakReference(array, pd);
    }

    std::vector<float> rescale(std::vector<float> const& v, unsigned const newSize)
    {
        if (v.empty()) {
            return {};
        }

        std::vector<float> result(newSize);
        const std::size_t oldSize = v.size();
        for (unsigned i = 0; i < newSize; i++) {
            auto const idx = i * (oldSize - 1) / newSize;
            auto const mod = i * (oldSize - 1) % newSize;

            if (mod == 0)
                result[i] = v[idx];
            else {
                float const part = float(mod) / float(newSize);
                result[i] = v[idx] * (1.0 - part) + v[idx + 1] * part;
            }
        }
        return result;
    }

    void paintGraph(Graphics& g)
    {
        auto const h = static_cast<float>(getHeight());
        auto const w = static_cast<float>(getWidth());
        std::vector<float> points = vec;

        if (!points.empty()) {
            std::array<float, 2> scale = getScale();
            bool invert = false;

            if (scale[0] >= scale[1]) {
                invert = true;
                std::swap(scale[0], scale[1]);
            }

            // More than a point per pixel will cause insane loads, and isn't actually helpful
            // Instead, linearly interpolate the vector to a max size of width in pixels
            if (vec.size() >= w) {
                points = rescale(points, w);
            }

            float const dh = h / (scale[1] - scale[0]);
            float const dw = w / static_cast<float>(points.size() - 1);

            switch (getDrawType()) {
            case DrawType::Curve: {

                Path p;
                p.startNewSubPath(0, h - (std::clamp(points[0], scale[0], scale[1]) - scale[0]) * dh);

                for (size_t i = 1; i < points.size() - 2; i += 3) {
                    float const y1 = h - (std::clamp(points[i], scale[0], scale[1]) - scale[0]) * dh;
                    float const y2 = h - (std::clamp(points[i + 1], scale[0], scale[1]) - scale[0]) * dh;
                    float const y3 = h - (std::clamp(points[i + 2], scale[0], scale[1]) - scale[0]) * dh;
                    p.cubicTo(static_cast<float>(i) * dw, y1, static_cast<float>(i + 1) * dw, y2, static_cast<float>(i + 2) * dw, y3);
                }

                if (invert)
                    p.applyTransform(AffineTransform::verticalFlip(getHeight()));

                g.setColour(getContentColour());
                g.strokePath(p, PathStrokeType(getLineWidth()));
                break;
            }
            case DrawType::Polygon: {
                int startY = h - (std::clamp(points[0], scale[0], scale[1]) - scale[0]) * dh;
                Point<float> lastPoint = Point<float>(0, startY);
                Point<float> newPoint;

                Path p;
                for (size_t i = 1; i < points.size(); i++) {
                    float const y = h - (std::clamp(points[i], scale[0], scale[1]) - scale[0]) * dh;
                    newPoint = Point<float>(static_cast<float>(i) * dw, y);

                    p.addLineSegment({ lastPoint, newPoint }, getLineWidth());
                    lastPoint = newPoint;
                }

                if (invert)
                    p.applyTransform(AffineTransform::verticalFlip(getHeight()));

                g.setColour(getContentColour());
                g.fillPath(p);
                break;
            }
            case DrawType::Points: {
                g.setColour(getContentColour());

                float const dw_points = w / static_cast<float>(points.size());

                for (size_t i = 0; i < points.size(); i++) {
                    float y = h - (std::clamp(points[i], scale[0], scale[1]) - scale[0]) * dh;
                    if (invert)
                        y = getHeight() - y;
                    g.drawLine(static_cast<float>(i) * dw_points, y, static_cast<float>(i + 1) * dw_points, y, getLineWidth());
                }
                break;
            }
            default:
                break;
            }
        }
    }
    
    void receiveMessage(String const& symbol, std::vector<pd::Atom> const& atoms) override
    {
        switch(hash(symbol)) {
            case hash("edit"): {
                if (atoms.empty()) break;
                MessageManager::callAsync([_this = SafePointer(this), shouldBeEditable = static_cast<bool>(atoms[0].getFloat())]() {
                        _this->editable = shouldBeEditable;
                        _this->setInterceptsMouseClicks(shouldBeEditable, false);
                });
                break;
            }
            case hash("rename"): {
                if (atoms.empty()) break;
                MessageManager::callAsync([_this = SafePointer(this), newName = atoms[0].toString()]() {
                    if (!_this)
                        return;
                    
                    _this->object->cnv->setSelected(_this->object, false);
                    _this->object->cnv->editor->sidebar->hideParameters();
                    _this->name = newName;
                });
                
                break;
            }
            case hash("color"): {
                MessageManager::callAsync([_this = SafePointer(this)] {
                    if(_this) _this->repaint();
                });
                break;
            }
            case hash("width"): {
                MessageManager::callAsync([_this = SafePointer(this)] {
                    if(_this) _this->repaint();
                });
                break;
            }
            case hash("style"): {
                MessageManager::callAsync([_this = SafePointer(this), newDrawMode = static_cast<int>(atoms[0].getFloat())] {
                    if(_this)
                    {
                        _this->drawMode = newDrawMode + 1;
                        _this->updateSettings();
                    }
                });
                break;
            }
            case hash("xpix"): {
                // TODO: implement ticks
            }
            case hash("ypix"): {
                // TODO: implement ticks
            }
        }
    }

    void paint(Graphics& g) override
    {
        if (error) {
            // TODO: error colour
            Fonts::drawText(g, "array " + getUnexpandedName() + " is invalid", 0, 0, getWidth(), getHeight(), object->findColour(PlugDataColour::canvasTextColourId), 15, Justification::centred);
            error = false;
        } else {
            paintGraph(g);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (error || !getEditMode())
            return;
        edited = true;

        auto const s = static_cast<float>(vec.size() - 1);
        auto const w = static_cast<float>(getWidth());
        auto const x = static_cast<float>(e.x);

        lastIndex = std::round(std::clamp(x / w, 0.f, 1.f) * s);

        mouseDrag(e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (error || !getEditMode())
            return;

        auto const s = static_cast<float>(vec.size() - 1);
        auto const w = static_cast<float>(getWidth());
        auto const h = static_cast<float>(getHeight());
        auto const x = static_cast<float>(e.x);
        auto const y = static_cast<float>(e.y);

        std::array<float, 2> scale = getScale();

        int const index = static_cast<int>(std::round(std::clamp(x / w, 0.f, 1.f) * s));

        float start = vec[lastIndex];
        float current = (1.f - std::clamp(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];

        int interpStart = std::min(index, lastIndex);
        int interpEnd = std::max(index, lastIndex);

        float min = index == interpStart ? current : start;
        float max = index == interpStart ? start : current;

        // Fix to make sure we don't leave any gaps while dragging
        for (int n = interpStart; n <= interpEnd; n++) {
            vec[n] = jmap<float>(n, interpStart, interpEnd + 1, min, max);
        }

        // Don't want to touch vec on the other thread, so we copy the vector into the lambda
        auto changed = std::vector<float>(vec.begin() + interpStart, vec.begin() + interpEnd + 1);

        lastIndex = index;

        pd->lockAudioThread();
        for (int n = 0; n < changed.size(); n++) {
            write(interpStart + n, changed[n]);
        }

        if (auto ptr = arr.get<t_fake_garray>()) {
            pd->sendDirectMessage(ptr.get(), stringArray);
        }

        pd->unlockAudioThread();
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (error || !getEditMode())
            return;
        
        if (auto ptr = arr.get<t_fake_garray>()) {
            plugdata_forward_message(ptr->x_glist, gensym("redraw"), 0, NULL);
        }
        
        edited = false;
    }

    void update()
    {
        // Check if size has changed
        int currentSize = getArraySize();
        if (vec.size() != currentSize) {
            vec.resize(currentSize);
        }
        
        size = currentSize;

        if (!edited) {
            error = false;
            try {
                read(temp);
            } catch (...) {
                error = true;
            }
            if (temp != vec) {
                vec.swap(temp);
                repaint();
            }
        }
    }

    bool willSaveContent() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            return ptr->x_saveit;
        }

        return false;
    }

    // Gets the name of the array. Currently not used anywhere
    String getExpandedName() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            return String::fromUTF8(ptr->x_realname->s_name);
        }

        return {};
    }

    // Gets the text label of the array
    String getUnexpandedName() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            return String::fromUTF8(ptr->x_name->s_name);
        }

        return {};
    }

    int getLineWidth()
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            if (ptr->x_scalar) {
                t_scalar* scalar = ptr->x_scalar;
                t_template* scalartplte = template_findbyname(scalar->sc_template);
                if (scalartplte) {
                    int result = (int)template_getfloat(scalartplte, gensym("linewidth"), scalar->sc_vec, 1);
                    return result;
                }
            }
        }

        return 1;
    }
    DrawType getDrawType() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            if (ptr->x_scalar) {
                t_scalar* scalar = ptr->x_scalar;
                t_template* scalartplte = template_findbyname(scalar->sc_template);
                if (scalartplte) {
                    int result = (int)template_getfloat(scalartplte, gensym("style"), scalar->sc_vec, 0);
                    return static_cast<DrawType>(result);
                }
            }
        }

        return DrawType::Points;
    }

    // Gets the scale of the array
    std::array<float, 2> getScale() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            t_canvas const* cnv = ptr->x_glist;
            if (cnv) {
                float min = cnv->gl_y2;
                float max = cnv->gl_y1;

                if (approximatelyEqual(min, max))
                    max += 1e-6;

                return { min, max };
            }
        }

        return { -1.0f, 1.0f };
    }

    bool getEditMode() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            return ptr->x_edit;
        }

        return true;
    }

    void setEditMode(bool editMode)
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            ptr->x_edit = editMode;
        }
    }

    // Gets the scale of the array.
    int getArraySize() const
    {
        if (auto ptr = arr.get<t_garray>()) {
            return garray_getarray(ptr.get())->a_n;
        }

        return 0;
    }

    Colour getContentColour()
    {
        if (auto garray = arr.get<t_fake_garray>()) {
            auto* scalar = garray->x_scalar;
            auto* templ = template_findbyname(scalar->sc_template);

            int colour = template_getfloat(templ, gensym("color"), scalar->sc_vec, 1);

            if (colour <= 0) {
                return object->findColour(PlugDataColour::guiObjectInternalOutlineColour);
            }

            auto rangecolor = [](int n) /* 0 to 9 in 5 steps */
            {
                int n2 = (n == 9 ? 8 : n); /* 0 to 8 */
                int ret = (n2 << 5);       /* 0 to 256 in 9 steps */
                if (ret > 255)
                    ret = 255;
                return (ret);
            };

            int red = rangecolor(colour / 100);
            int green = rangecolor((colour / 10) % 10);
            int blue = rangecolor(colour % 10);

            return Colour(red, green, blue);
        }

        return object->findColour(PlugDataColour::guiObjectInternalOutlineColour);
    }
        
    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(name) || value.refersToSameSourceAs(size) || value.refersToSameSourceAs(drawMode) || value.refersToSameSourceAs(saveContents)) {
            updateSettings();
        } else if (value.refersToSameSourceAs(range)) {
            auto min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(range.getValue().getArray()->getReference(1));
            setScale({ min, max });
            repaint();
        }
    }
        
    void updateSettings()
    {
        auto arrName = name.getValue().toString();
        auto arrSize = std::max(0, getValue<int>(size));
        auto arrDrawMode = getValue<int>(drawMode) - 1;

        if (arrSize != getValue<int>(size)) {
            size = arrSize;
        }

        // This flag is swapped for some reason
        if (arrDrawMode == 0) {
            arrDrawMode = 1;
        } else if (arrDrawMode == 1) {
            arrDrawMode = 0;
        }

        auto arrSaveContents = getValue<bool>(saveContents);

        int flags = arrSaveContents + 2 * static_cast<int>(arrDrawMode);

        t_symbol* name = pd->generateSymbol(arrName);
        if (auto garray = arr.get<t_fake_garray>()) {
            garray_arraydialog(garray.get(), name, arrSize, static_cast<float>(flags), 0.0f);
        }

        object->gui->updateLabel();
    }
    
    void deleteArray()
    {
        if (auto garray = arr.get<t_fake_garray>()) {
            glist_delete(garray->x_glist, &garray->x_gobj);
        }
        
        reloadGraphs();
    }
        
    void updateParameters()
    {
        auto scale = getScale();
        range = var(Array<var> { var(scale[0]), var(scale[1]) });
        size = var(static_cast<int>(getArraySize()));
        saveContents = willSaveContent();
        name = String(getUnexpandedName());
        drawMode = static_cast<int>(getDrawType()) + 1;
        repaint();
    }

    void setScale(std::array<float, 2> scale)
    {
        auto& [min, max] = scale;
        if (auto ptr = arr.get<t_fake_garray>()) {
            t_canvas* cnv = ptr->x_glist;
            if (cnv) {
                cnv->gl_y2 = min;
                cnv->gl_y1 = max;
                return;
            }
        }
    }

    // Gets the values from the array.
    void read(std::vector<float>& output) const
    {
        if (auto ptr = arr.get<t_garray>()) {
            int const size = garray_getarray(ptr.get())->a_n;
            output.resize(static_cast<size_t>(size));

            t_word* vec = ((t_word*)garray_vec(ptr.get()));
            for (int i = 0; i < size; i++)
                output[i] = vec[i].w_float;
        }
    }

    // Writes a value to the array.
    void write(const size_t pos, float const input)
    {
        if (auto ptr = arr.get<t_garray>()) {
            t_word* vec = ((t_word*)garray_vec(ptr.get()));
            vec[pos].w_float = input;
        }
    }

    pd::WeakReference arr;

    std::vector<float> vec;
    std::vector<float> temp;
    std::atomic<bool> edited;
    bool error = false;
    const String stringArray = "array";

    int lastIndex = 0;

    PluginProcessor* pd;
    bool editable = true;
};

struct ArrayPropertiesPanel : public PropertiesPanelProperty, public Value::Listener
{
    class AddArrayButton : public Component {

        bool mouseIsOver = false;

    public:
        std::function<void()> onClick = []() {};

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds();
            auto textBounds = bounds;
            auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto colour = findColour(PlugDataColour::sidebarTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                PlugDataLook::fillSmoothedRectangle(g, bounds.toFloat(), Corners::defaultCornerRadius);

                colour = findColour(PlugDataColour::sidebarActiveTextColourId);
            }

            Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
            Fonts::drawText(g, "Add new array", textBounds, colour, 14);
        }

        bool hitTest(int x, int y) override
        {
            if (getLocalBounds().reduced(5, 2).contains(x, y)) {
                return true;
            }
            return false;
        }

        void mouseEnter(MouseEvent const& e) override
        {
            mouseIsOver = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            mouseIsOver = false;
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            onClick();
        }
    };
    
    OwnedArray<PropertiesPanelProperty> properties;
    Array<SafePointer<GraphicalArray>> graphs;
    
    AddArrayButton addButton;
    OwnedArray<SmallIconButton> deleteButtons;
    Array<Value> nameValues;
        
    ArrayPropertiesPanel(std::function<void()> addArrayCallback)
    : PropertiesPanelProperty("array")
    {
        setHideLabel(true);
        
        addAndMakeVisible(addButton);
        addButton.onClick = addArrayCallback;
    }
    
    void reloadGraphs(const Array<SafePointer<GraphicalArray>>& safeGraphs)
    {
        properties.clear();
        nameValues.clear();
        deleteButtons.clear();
        
        graphs = safeGraphs;
        
        for(auto graph : graphs)
        {
            addAndMakeVisible(properties.add(new PropertiesPanel::EditableComponent<String>("Name", graph->name)));
            addAndMakeVisible(properties.add(new PropertiesPanel::EditableComponent<int>("Size", graph->size)));
            addAndMakeVisible(properties.add(new PropertiesPanel::RangeComponent("Range", graph->range, false)));
            addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("Save contents", graph->saveContents, {"No", "Yes"})));
            addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Draw Style", graph->drawMode, {"Points", "Polygon", "Bezier curve"})));
            
            // To detect name changes, so we can redraw the array title
            nameValues.add(Value());
            auto& nameValue = nameValues.getReference(nameValues.size() - 1);
            nameValue.referTo(graph->name);
            nameValue.addListener(this);
            auto* deleteButton = deleteButtons.add(new SmallIconButton(Icons::Clear));
            deleteButton->onClick = [graph](){
                graph->deleteArray();
            };
            addAndMakeVisible(deleteButton);
        }
        
        auto newHeight = (156 * graphs.size()) + 34;
        setPreferredHeight(newHeight);
        if(auto* propertiesPanel = findParentComponentOfClass<PropertiesPanel>())
        {
            propertiesPanel->updatePropHolderLayout();
        }
        
        repaint();
    }
    
    void valueChanged(Value& v) override
    {
        repaint();
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        
        auto numGraphs = properties.size() / 5;
        for(int i = 0; i < numGraphs; i++)
        {
            if(!graphs[i]) continue;
            
            auto start = (i * 156) - 6;
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(0.0f, start + 25, getWidth(), 130, Corners::largeCornerRadius);
           
            Fonts::drawStyledText(g,  graphs[i]->name.toString(), 8, start - 2, getWidth() - 16, 25, findColour(PlugDataColour::sidebarTextColourId), Semibold, 14.5f);
        }
        
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        for (int i = 0; i < properties.size(); i++) {
            if((i % 5) == 4) continue;
            auto y = properties[i]->getBottom();
            g.drawHorizontalLine(y, 0, getWidth());
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().translated(0, -6);
        for(int i = 0; i < properties.size(); i++)
        {
            if((i % 5) == 0)  {
                auto deleteButtonBounds = b.removeFromTop(26).removeFromRight(28);
                deleteButtons[i / 5]->setBounds(deleteButtonBounds);
            }
            properties[i]->setBounds(b.removeFromTop(26));
        }
        
        addButton.setBounds(getLocalBounds().removeFromBottom(36).reduced(0, 8));
    }
};

class ArrayListView : public PropertiesPanel, public Value::Listener
{
public:
    ArrayListView(pd::Instance* instance, void* arr) : array(arr, instance)
    {
        update();
    }
    
    void parentSizeChanged() override
    {
        setContentWidth(getWidth() - 100);
    }
    
    void update()
    {
        clear();
        arrayValues.clear();
        
        Array<PropertiesPanelProperty*> properties;
        
        if (auto ptr = array.get<t_fake_garray>()) {
            auto* arr = garray_getarray(ptr.cast<t_garray>());
            auto* vec = ((t_word*)garray_vec(ptr.cast<t_garray>()));
            
            auto numProperties = arr->a_n;
            properties.resize(numProperties);
            
            for(int i = 0; i < numProperties; i++)
            {
                auto& value = *arrayValues.add(new Value(vec[i].w_float));
                value.addListener(this);
                auto* property = new EditableComponent<float>(String(i), value);
                auto* label = dynamic_cast<DraggableNumber*>(property->label.get());

                property->setRangeMin(ptr->x_glist->gl_y2);
                property->setRangeMax(ptr->x_glist->gl_y1);
                
                // Only send this after drag end so it doesn't interrupt the drag action
                label->dragEnd = [this](){
                    if (auto ptr = array.get<t_fake_garray>()) {
                        plugdata_forward_message(ptr->x_glist, gensym("redraw"), 0, NULL);
                    };
                };
                properties.set(i, property);
            }
        }
        
        addSection("", properties);
    }
    
private:
    void valueChanged(Value& v) override
    {
        if (auto ptr = array.get<t_fake_garray>()) {
            auto* vec = ((t_word*)garray_vec(ptr.cast<t_garray>()));
            
            for(int i = 0; i < arrayValues.size(); i++)
            {
                auto& value = *arrayValues[i];
                if(v.refersToSameSourceAs(value))
                {
                    vec[i].w_float = getValue<float>(value);
                    break;
                }
            }
        }
    }
    
    OwnedArray<Value> arrayValues;
    pd::WeakReference array;
};

class ArrayEditorDialog : public Component {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;
    
    ComboBox selectedArrayCombo;
    SettingsToolbarButton listViewButton = SettingsToolbarButton(Icons::List, "List");
    SettingsToolbarButton graphViewButton = SettingsToolbarButton(Icons::Graph, "Graph");

public:
    std::function<void()> onClose;
    OwnedArray<GraphicalArray> graphs;
    OwnedArray<ArrayListView> lists;
    PluginProcessor* pd;

    ArrayEditorDialog(PluginProcessor* instance, std::vector<void*> arrays, Object* parent)
        : resizer(this, &constrainer)
        , pd(instance)
    {
        for (auto* arr : arrays) {
            auto* graph = graphs.add(new GraphicalArray(pd, arr, parent));
            addChildComponent(graph);
            
            auto* list = lists.add(new ArrayListView(pd, arr));
            addChildComponent(list);
        }
        graphs[0]->setVisible(true);

        for(int i = 0; i < graphs.size(); i++)
        {
            selectedArrayCombo.addItem(graphs[i]->getUnexpandedName(), i + 1);
        }
        selectedArrayCombo.setSelectedItemIndex(0);
        selectedArrayCombo.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        selectedArrayCombo.setColour(ComboBox::backgroundColourId, findColour(PlugDataColour::toolbarHoverColourId).withAlpha(0.8f));
        
        addAndMakeVisible(selectedArrayCombo);
        
        graphViewButton.setRadioGroupId(hash("array_radio_button"));
        listViewButton.setRadioGroupId(hash("array_radio_button"));
        graphViewButton.setClickingTogglesState(true);
        listViewButton.setClickingTogglesState(true);
        graphViewButton.setToggleState(true, dontSendNotification);
        
        addAndMakeVisible(listViewButton);
        addAndMakeVisible(graphViewButton);
        
        listViewButton.onClick = [this](){
            updateVisibleGraph();
        };
        graphViewButton.onClick = [this](){
            updateVisibleGraph();
        };
        selectedArrayCombo.onChange = [this](){
            updateVisibleGraph();
        };
        
        
        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 300);

        closeButton->onClick = [this]() {
            MessageManager::callAsync([this]() {
                onClose();
            });
        };

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(600, 400));

        addAndMakeVisible(resizer);
        updateGraphs();
        updateVisibleGraph();
    }
    
    void updateVisibleGraph()
    {
        for(int i = 0; i < graphs.size(); i++)
        {
            graphs[i]->setVisible(i == selectedArrayCombo.getSelectedItemIndex() && graphViewButton.getToggleState());
        }
        for(int i = 0; i < graphs.size(); i++)
        {
            lists[i]->setVisible(i == selectedArrayCombo.getSelectedItemIndex() && listViewButton.getToggleState());
        }
    }

    void resized() override
    {
        auto toolbarHeight = 38;
        auto buttonWidth = 120;
        auto centre = getWidth() / 2;
        
        graphViewButton.setBounds(centre - buttonWidth, 1, buttonWidth, toolbarHeight - 2);
        listViewButton.setBounds(centre, 1, buttonWidth, toolbarHeight - 2);

        selectedArrayCombo.setBounds(8, 8, toolbarHeight * 2.5f, toolbarHeight - 16);
        
        resizer.setBounds(getLocalBounds());

        auto closeButtonBounds = getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5);
        closeButton->setBounds(closeButtonBounds);

        for (auto* graph : graphs) {
            graph->setBounds(getLocalBounds().withTrimmedTop(40));
        }
        for (auto* list : lists) {
            list->setBounds(getLocalBounds().withTrimmedTop(40));
        }
    }

    void updateGraphs()
    {
        if (!pd->tryLockAudioThread())
            return;

        for (auto* graph : graphs) {
            graph->update();
        }
        for (auto* list : lists) {
            list->update();
        }

        pd->unlockAudioThread();
    }

    void mouseDown(MouseEvent const& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius, 1.0f);
    }

    void paint(Graphics& g) override
    {
        auto toolbarHeight = 38;
        auto b = getLocalBounds();
        auto titlebarBounds = b.removeFromTop(toolbarHeight).toFloat();
        auto arrayBounds = b.toFloat();

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(toolbarPath);

        Path arrayPath;
        arrayPath.addRoundedRectangle(arrayBounds.getX(), arrayBounds.getY(), arrayBounds.getWidth(), arrayBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillPath(arrayPath);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(toolbarHeight, 0, getWidth());
    }
};

class ArrayObject final : public ObjectBase {
public:
    
    SafePointer<ArrayPropertiesPanel> propertiesPanel = nullptr;
    Value sizeProperty = SynchronousValue();
    
    // Array component
    ArrayObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        reinitialiseGraphs();
        
        setInterceptsMouseClicks(false, true);

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamCustom([_this = SafePointer(this)](){
            
            if(!_this) return static_cast<ArrayPropertiesPanel*>(nullptr);
            
            Array<SafePointer<GraphicalArray>> safeGraphs;
            for(auto* graph : _this->graphs)
            {
                safeGraphs.add(graph);
            }
            
            auto* panel = new ArrayPropertiesPanel([_this](){
                if(_this) _this->addArray();
            });
            
            panel->reloadGraphs(safeGraphs);
            _this->propertiesPanel = panel;
                        
            return panel;
        });
        
        updateLabel();
        
        onConstrainerCreate = [this]() {
            constrainer->setSizeLimits(50 - Object::doubleMargin, 40 - Object::doubleMargin, 99999, 99999);
        };
    }
    
    void reinitialiseGraphs()
    {
        // Close the dialog they could be holding a pointer to an old graph object
        if(dialog != nullptr) dialog.reset(nullptr);
        
        auto arrays = getArrays();
        graphs.clear();
        
        for (int i = 0; i < arrays.size(); i++) {
            auto* graph = graphs.add(new GraphicalArray(cnv->pd, arrays[i], object));
            graph->setBounds(getLocalBounds());
            graph->reloadGraphs = [this]() {
                MessageManager::callAsync([_this = SafePointer(this)]() {
                    if(_this) _this->reinitialiseGraphs();
                });
            };
            addAndMakeVisible(graph);
        }
        
        updateGraphs();
        
        Array<SafePointer<GraphicalArray>> safeGraphs;
        for(auto* graph : graphs) safeGraphs.add(graph);
        
        if(propertiesPanel) propertiesPanel->reloadGraphs(safeGraphs);
        
        updateLabel();
    }
    
    void addArray()
    {
        if (auto glist = ptr.get<_glist>()) {
            graph_array(glist.get(), pd::Interface::getUnusedArrayName(), gensym("float"), 100, 0);
        }
        reinitialiseGraphs();
    }
    
    void updateGraphs()
    {
        pd->lockAudioThread();

        for (auto* graph : graphs) {
            // Update values
            graph->update();
        }

        pd->unlockAudioThread();
    }

    void updateLabel() override
    {
        int fontHeight = 14.0f;

        auto title = String();
        for(auto* graph : graphs)
        {
            title += graph->getUnexpandedName() + (graph != graphs.getLast() ? "," : "");
        }
  
        if (title.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<ObjectLabel>();
            }

            auto bounds = object->getBounds().reduced(Object::margin).removeFromTop(fontHeight + 2);

            bounds.translate(2, -(fontHeight + 2));

            label->setFont(Font(fontHeight));
            label->setBounds(bounds);
            label->setText(title, dontSendNotification);

            label->setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));

            object->cnv->addAndMakeVisible(label.get());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto glist = ptr.get<_glist>()) {

            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, &glist->gl_obj.te_g, &x, &y, &w, &h);

            return { x, y, glist->gl_pixwidth, glist->gl_pixheight };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto glist = ptr.get<t_glist>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, glist.cast<t_gobj>(), b.getX(), b.getY());

            glist->gl_pixwidth = b.getWidth();
            glist->gl_pixheight = b.getHeight();
        }
    }

    void resized() override
    {
        for (auto* graph : graphs) {
            graph->setBounds(getLocalBounds());
        }
    }

    void update() override
    {
        for(auto* graph : graphs)
        {
            graph->updateParameters();
        }
        if (auto glist = ptr.get<t_glist>()) {
            sizeProperty = Array<var> { var(glist->gl_pixwidth), var(glist->gl_pixheight) };
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto glist = ptr.get<t_glist>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(glist->gl_pixwidth), var(glist->gl_pixheight) });
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto glist = ptr.get<t_glist>()) {
                glist->gl_pixwidth = width;
                glist->gl_pixheight = height;
            }

            object->updateBounds();
        } 
        else {
            ObjectBase::valueChanged(value);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    std::vector<void*> getArrays() const
    {
        if (auto c = ptr.get<t_canvas>()) {
            std::vector<void*> arrays;

            t_gobj* x = reinterpret_cast<t_gobj*>(c->gl_list);
            if(x) {
                arrays.push_back(x);
                
                while ((x = x->g_next)) {
                    arrays.push_back(x);
                }
            }

            return arrays;
        }

        return {};
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        if (dialog) {
            dialog->toFront(true);
            return;
        }

        dialog = std::make_unique<ArrayEditorDialog>(cnv->pd, getArrays(), object);
        dialog->onClose = [this]() {
            dialog.reset(nullptr);
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom> const& atoms) override
    {
        if(symbol == "redraw")
        {
            updateGraphs();
            if (dialog) {
                dialog->updateGraphs();
            }
        }
    }

private:
    
    OwnedArray<GraphicalArray> graphs;
    std::unique_ptr<ArrayEditorDialog> dialog = nullptr;
};

class ArrayDefineObject final : public TextBase {
    std::unique_ptr<ArrayEditorDialog> editor = nullptr;

public:
    ArrayDefineObject(pd::WeakReference obj, Object* parent)
        : TextBase(obj, parent, true)
    {
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        openArrayEditor();
    }

    bool canOpenFromMenu() override
    {
        if (auto c = ptr.get<t_canvas>()) {
            return c->gl_list != nullptr;
        }

        return false;
    }

    void openArrayEditor()
    {
        if (editor) {
            editor->toFront(true);
            return;
        }

        if (auto c = ptr.get<t_canvas>()) {
            std::vector<void*> arrays;

            t_glist* x = c.get();
            t_gobj* gl = (x->gl_list ? pd_checkglist(&x->gl_list->g_pd)->gl_list : 0);

            if (gl) {
                arrays.push_back(gl);
                while ((gl = gl->g_next)) {
                    arrays.push_back(x);
                }
            }

            if (arrays.size() && arrays[0] != nullptr) {
                editor = std::make_unique<ArrayEditorDialog>(cnv->pd, arrays, object);
                editor->onClose = [this]() {
                    editor.reset(nullptr);
                };
            } else {
                pd->logWarning("array define: cannot open non-existent array");
            }
        }
    }
    
    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom> const& atoms) override
    {
    }
    
    void openFromMenu() override
    {
        openArrayEditor();
    }
};
