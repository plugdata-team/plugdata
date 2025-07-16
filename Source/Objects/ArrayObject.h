/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/PropertiesPanel.h"

extern "C" {
void garray_arraydialog(t_fake_garray* x, t_symbol* name, t_floatarg fsize, t_floatarg fflags, t_floatarg deleteit);
}

class GraphicalArray final : public Component
    , public Value::Listener
    , public pd::MessageListener
    , public NVGComponent {
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
    bool visible = true;
    bool arrayNeedsUpdate = true;
    Path arrayPath;
    NVGCachedPath cachedPath;

    std::function<void()> reloadGraphs = [] { };

    GraphicalArray(PluginProcessor* instance, void* ptr, Object* parent)
        : NVGComponent(this)
        , object(parent)
        , arr(ptr, instance)
        , edited(false)
        , pd(instance)
    {
        vec.reserve(8192);
        read(vec);

        updateParameters();

        for (auto* value : SmallArray<Value*> { &name, &size, &drawMode, &saveContents, &range }) {
            // TODO: implement undo/redo for these values!
            // how does pd even do this? since it's not a gobj?
            value->addListener(this);
        }

        pd->registerMessageListener(arr.getRawUnchecked<void>(), this);

        setInterceptsMouseClicks(true, false);
        setOpaque(false);
    }

    ~GraphicalArray()
    {
        pd->unregisterMessageListener(this);
    }

    static Path createArrayPath(HeapArray<float> points, DrawType style, StackArray<float, 2> scale, float const width, float const height, float const lineWidth)
    {
        bool invert = false;
        if (scale[0] >= scale[1]) {
            invert = true;
            std::swap(scale[0], scale[1]);
        }
        
        // Need at least 4 points to draw a bezier curve
        if (points.size() <= 4 && style == Curve)
            style = Polygon;

        float const dh = (height - (lineWidth + 1)) / (scale[1] - scale[0]);
        float const invh = invert ? 0 : (height - (lineWidth + 1));
        float const yscale = invert ? -1.0f : 1.0f;

        auto yToCoords = [dh, invh, scale, yscale](float y){
            return 1 + (invh - (std::clamp(y, scale[0], scale[1]) - scale[0]) * dh * yscale);
        };
        
        auto const* pointPtr = points.data();
        auto const numPoints = points.size();
        
        StackArray<float, 6> control = {0};
        Path result;
        if (std::isfinite(pointPtr[0])) {
            result.startNewSubPath(0, yToCoords(pointPtr[0]));
        }
        
        int onset = 0;
        float lastX = 0;
        if(style == Curve)
        {
            onset = 2;
            control[4] = 0;
            control[5] = yToCoords(pointPtr[0]);
            pointPtr += 1;
            lastX = width / numPoints;
        }

        float minY = 1e20, maxY = -1e20;
        for (int i = onset; i < numPoints; i++, pointPtr++) {
            switch (style) {
            case Points: {
                float nextX = static_cast<float>(i + 1) / numPoints * width;
                float y = yToCoords(pointPtr[0]);
                minY = std::min(y, minY);
                maxY = std::max(y, maxY);
              
                if (i == 0 || i == numPoints-1 || std::abs(nextX - lastX) >= 1.0f)
                {
                    result.addRectangle(lastX - 0.33f, minY, (nextX - lastX) + 0.33f, std::max((maxY - minY), lineWidth));
                    lastX = nextX;
                    minY = 1e20;
                    maxY = -1e20;
                }
                break;
            }
            case Polygon: {
                float nextX = static_cast<float>(i) / (numPoints - 1) * width;
                if (i != 0 || i == numPoints-1 || std::abs(nextX - lastX) >= 1.0f) {
                    float y1 = yToCoords(pointPtr[0]);
                    if (std::isfinite(y1)) {
                        result.lineTo(nextX, y1);
                    }
                    lastX = nextX;
                }
                break;
            }
            case Curve: {
                float nextX = static_cast<float>(i) / (numPoints - 1) * width;
                if(std::abs(nextX - lastX) < 1.0f && i != 0 && i != numPoints-1)
                    continue;
                
                float y1 = yToCoords(pointPtr[0]);
                float y2 = yToCoords(pointPtr[1]);
                
                // Curve logic taken from tcl/tk source code:
                // https://github.com/tcltk/tk/blob/c9fe293db7a52a34954db92d2bdc5454d4de3897/generic/tkTrig.c#L1363
                control[0] = 0.333 * control[4] + 0.667 * lastX;
                control[1] = 0.333 * control[5] + 0.667 * y1;

                // Set up the last two control points. This is done differently for
                // the last spline of an open curve than for other cases.
                if (i == numPoints-1) {
                    control[4] = nextX;
                    control[5] = y2;
                } else {
                    control[4] = 0.5 * lastX + 0.5 * nextX;
                    control[5] = 0.5 * y1 + 0.5 * y2;
                }

                control[2] = 0.333 * control[4] + 0.667 * lastX;
                control[3] = 0.333 * control[5] + 0.667 * y1;

                auto start = Point<float>(control[0], control[1]);
                auto c1 = Point<float>(control[2], control[3]);
                auto end = Point<float>(control[4], control[5]);

                if (start.isFinite() && c1.isFinite() && end.isFinite()) {
                    result.cubicTo(start, c1, end);
                }
                lastX = nextX;
                break;
            }
            }
        }

        return result;
    }
        
    void updateArrayPath()
    {
        arrayNeedsUpdate = true;
        repaint();
    }

    void paintGraph(Graphics& g)
    {
        if(arrayNeedsUpdate)
        {
            if(vec.not_empty()) {
                arrayPath = createArrayPath(vec, static_cast<DrawType>(getValue<int>(drawMode) - 1), getScale(), getWidth(), getHeight(), getLineWidth());
            }
            arrayNeedsUpdate = false;
        }
        
        if (vec.not_empty()) {
            g.setColour(getContentColour());
            g.strokePath(arrayPath, PathStrokeType(getLineWidth()));
        }
    }

    void paintGraph(NVGcontext* nvg)
    {
        auto arrDrawMode = static_cast<DrawType>(getValue<int>(drawMode) - 1);
        if(arrayNeedsUpdate)
        {
            if(vec.not_empty()) {
                arrayPath = createArrayPath(vec, arrDrawMode, getScale(), getWidth(), getHeight(), getLineWidth());
            }
            cachedPath.clear();
            arrayNeedsUpdate = false;
        }
        NVGScopedState scopedState(nvg);
        auto const arrB = getLocalBounds().reduced(1);
        nvgIntersectRoundedScissor(nvg, arrB.getX(), arrB.getY(), arrB.getWidth(), arrB.getHeight(), Corners::objectCornerRadius);

        if(cachedPath.isValid())
        {
            auto const contentColour = getContentColour();
            if(arrDrawMode == Points) {
                nvgFillColor(nvg, nvgRGBA(contentColour.getRed(), contentColour.getGreen(), contentColour.getBlue(), contentColour.getAlpha()));
                cachedPath.fill();
            }
            else {
                nvgStrokeColor(nvg, nvgRGBA(contentColour.getRed(), contentColour.getGreen(), contentColour.getBlue(), contentColour.getAlpha()));
                nvgStrokeWidth(nvg, getLineWidth());
                cachedPath.stroke();
            }
            return;
        }
        
        if (vec.not_empty()) {
            setJUCEPath(nvg, arrayPath);
            
            auto const contentColour = getContentColour();
            if(arrDrawMode == Points) {
                nvgFillColor(nvg, nvgRGBA(contentColour.getRed(), contentColour.getGreen(), contentColour.getBlue(), contentColour.getAlpha()));
                nvgFill(nvg);
                cachedPath.save(nvg);
            }
            else {
                nvgStrokeColor(nvg, nvgRGBA(contentColour.getRed(), contentColour.getGreen(), contentColour.getBlue(), contentColour.getAlpha()));
                nvgStrokeWidth(nvg, getLineWidth());
                nvgStroke(nvg);
                cachedPath.save(nvg);
            }
        }
    }

    void receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (hash(symbol->s_name)) {
        case hash("edit"): {
            if (atoms.size() <= 0)
                break;
            MessageManager::callAsync([_this = SafePointer(this), shouldBeEditable = static_cast<bool>(atoms[0].getFloat())] {
                if (_this) {
                    _this->editable = shouldBeEditable;
                    _this->setInterceptsMouseClicks(shouldBeEditable, false);
                }
            });
            break;
        }
        case hash("rename"): {
            if (atoms.size() <= 0)
                break;
            MessageManager::callAsync([_this = SafePointer(this), newName = atoms[0].toString()] {
                if (_this) {
                    _this->object->cnv->setSelected(_this->object, false);
                    _this->object->editor->sidebar->hideParameters();
                    _this->name = newName;
                }
            });

            break;
        }
        case hash("color"): {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this)
                    _this->repaint();
            });
            break;
        }
        case hash("width"): {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    _this->updateArrayPath();
                }
            });
            break;
        }
        case hash("style"): {
            MessageManager::callAsync([_this = SafePointer(this), newDrawMode = static_cast<int>(atoms[0].getFloat())] {
                if (_this) {
                    _this->drawMode = newDrawMode + 1;
                    _this->updateSettings();
                    _this->repaint();
                }
            });
            break;
        }
        case hash("xticks"): {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this)
                    _this->repaint();
            });
            break;
        }
        case hash("yticks"): {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this)
                    _this->repaint();
            });
            break;
        }
        case hash("vis"): {
            MessageManager::callAsync([_this = SafePointer(this), visible = atoms[0].getFloat()] {
                if (_this) {
                    _this->visible = static_cast<bool>(visible);
                    _this->repaint();
                }
            });

            break;
        }
        case hash("resize"): {
            MessageManager::callAsync([_this = SafePointer(this), newSize = atoms[0].getFloat()] {
                if (_this) {
                    _this->size = static_cast<int>(newSize);
                    _this->updateSettings();
                    _this->repaint();
                }
            });
            break;
        }
        }
    }

    void render(NVGcontext* nvg) override
    {
        if (error) {
            auto const position = getLocalBounds().getCentre();
            auto const errorText = "array " + getUnexpandedName() + " is invalid";
            nvgFontSize(nvg, 11);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, convertColour(object->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId)));
            nvgText(nvg, position.x, position.y, errorText.toRawUTF8(), nullptr);
            error = false;
        } else if (visible) {
            paintGraph(nvg);
        }
    }

    void paint(Graphics& g) override
    {
        if (error) {
            Fonts::drawText(g, "array " + getUnexpandedName() + " is invalid", 0, 0, getWidth(), getHeight(), Colours::red, 15, Justification::centred);
            error = false;
        } else if (visible) {
            paintGraph(g);
        }
    }
        
    void resized() override
    {
        updateArrayPath();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (error || !getEditMode() || !e.mods.isLeftButtonDown())
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
        if (error || !getEditMode() || !e.mods.isLeftButtonDown())
            return;

        auto const s = static_cast<float>(vec.size() - 1);
        auto const w = static_cast<float>(getWidth());
        auto const h = static_cast<float>(getHeight());
        auto const x = static_cast<float>(e.x);
        auto const y = static_cast<float>(e.y);

        StackArray<float, 2> scale = getScale();

        int const index = static_cast<int>(std::round(std::clamp(x / w, 0.f, 1.f) * s));

        float const start = vec[lastIndex];
        float const current = (1.f - std::clamp(y / h, 0.f, 1.f)) * (scale[1] - scale[0]) + scale[0];

        int const interpStart = std::min(index, lastIndex);
        int const interpEnd = std::max(index, lastIndex);

        float const min = index == interpStart ? current : start;
        float const max = index == interpStart ? start : current;

        // Fix to make sure we don't leave any gaps while dragging
        for (int n = interpStart; n <= interpEnd; n++) {
            vec[n] = jmap<float>(n, interpStart, interpEnd + 1, min, max);
        }
        
        // Don't want to touch vec on the other thread, so we copy the vector into the lambda
        auto changed = HeapArray<float>(vec.begin() + interpStart, vec.begin() + interpEnd + 1);

        lastIndex = index;

        if (auto ptr = arr.get<t_garray>()) {
            for (int n = 0; n < changed.size(); n++) {
                write(ptr.get(), interpStart + n, changed[n]);
            }
            pd->sendDirectMessage(ptr.get(), "array");
        }

        updateArrayPath();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (error || !getEditMode() || !e.mods.isLeftButtonDown())
            return;

        if (auto ptr = arr.get<t_fake_garray>()) {
            plugdata_forward_message(ptr->x_glist, gensym("redraw"), 0, nullptr);
        }

        edited = false;
    }

    void update()
    {
        setValueExcludingListener(size, var(getArraySize()), this);

        if (!edited) {
            if (read(vec)) {
                updateArrayPath();
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

    // Gets the text label of the array
    String getUnexpandedName() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            return String::fromUTF8(ptr->x_name->s_name);
        }

        return {};
    }

    int getLineWidth() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            if (ptr->x_scalar) {
                t_scalar* scalar = ptr->x_scalar;
                if (auto* scalartplte = template_findbyname(scalar->sc_template)) {
                    int const result = static_cast<int>(template_getfloat(scalartplte, gensym("linewidth"), scalar->sc_vec, 1));
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
                if (auto* scalartplte = template_findbyname(scalar->sc_template)) {
                    int result = static_cast<int>(template_getfloat(scalartplte, gensym("style"), scalar->sc_vec, 0));
                    return static_cast<DrawType>(result);
                }
            }
        }

        return DrawType::Points;
    }

    // Gets the scale of the array
    StackArray<float, 2> getScale() const
    {
        if (auto ptr = arr.get<t_fake_garray>()) {
            if (auto const* cnv = ptr->x_glist) {
                float const min = cnv->gl_y2;
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

    // Gets the scale of the array.
    int getArraySize() const
    {
        if (auto ptr = arr.get<t_garray>()) {
            return garray_getarray(ptr.get())->a_n;
        }

        return 0;
    }

    Colour getContentColour() const
    {
        if (auto garray = arr.get<t_fake_garray>()) {
            auto* scalar = garray->x_scalar;
            auto* templ = template_findbyname(scalar->sc_template);

            int const colour = template_getfloat(templ, gensym("color"), scalar->sc_vec, 1);

            if (colour <= 0) {
                return object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId);
            }

            auto rangecolor = [](int const n) /* 0 to 9 in 5 steps */
            {
                int const n2 = n == 9 ? 8 : n; /* 0 to 8 */
                int ret = n2 << 5;             /* 0 to 256 in 9 steps */
                if (ret > 255)
                    ret = 255;
                return ret;
            };

            int const red = rangecolor(colour / 100);
            int const green = rangecolor(colour / 10 % 10);
            int const blue = rangecolor(colour % 10);

            return Colour(red, green, blue);
        }

        return object->cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(name) || value.refersToSameSourceAs(size) || value.refersToSameSourceAs(drawMode) || value.refersToSameSourceAs(saveContents)) {
            updateSettings();
        } else if (value.refersToSameSourceAs(range)) {
            auto const min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto const max = static_cast<float>(range.getValue().getArray()->getReference(1));
            setScale({ min, max });
            updateArrayPath();
            repaint();
        }
    }

    void updateSettings()
    {
        auto const arrName = name.getValue().toString();
        auto const arrSize = std::max(0, getValue<int>(size));
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

        auto const arrSaveContents = getValue<bool>(saveContents);

        int const flags = arrSaveContents + 2 * static_cast<int>(arrDrawMode);

        t_symbol* name = pd->generateSymbol(arrName);
        if (auto garray = arr.get<t_fake_garray>()) {
            garray_arraydialog(garray.get(), name, arrSize, static_cast<float>(flags), 0.0f);
        }

        object->gui->updateLabel();
        updateArrayPath();
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
        range = var(VarArray { var(scale[0]), var(scale[1]) });
        size = var(static_cast<int>(getArraySize()));
        saveContents = willSaveContent();
        name = String(getUnexpandedName());
        drawMode = static_cast<int>(getDrawType()) + 1;
        repaint();
    }

    void setScale(StackArray<float, 2> scale)
    {
        auto& [min, max] = scale.data_;
        if (auto ptr = arr.get<t_fake_garray>()) {
            if (auto* cnv = ptr->x_glist) {
                cnv->gl_y2 = min;
                cnv->gl_y1 = max;
            }
        }
    }

    // Gets the values from the array.
    bool read(HeapArray<float>& output) const
    {
        bool changed = false;
        if (auto ptr = arr.get<t_garray>()) {
            int const size = garray_getarray(ptr.get())->a_n;
            changed = size != vec.size();
            output.resize(static_cast<size_t>(size));

            auto const* atoms = reinterpret_cast<t_word*>(garray_vec(ptr.get()));
            for (int i = 0; i < size; i++) {
                changed = changed || output[i] != atoms[i].w_float;
                output[i] = atoms[i].w_float;
            }
        }

        return changed;
    }

    // Writes a value to the array.
    static void write(t_garray* garray, size_t const pos, float const input)
    {
        if (pos < garray_npoints(garray)) {
            auto const vec = reinterpret_cast<t_word*>(garray_vec(garray));
            vec[pos].w_float = input;
        }
    }

    pd::WeakReference arr;

    HeapArray<float> vec;
    AtomicValue<bool> edited;
    bool error = false;
    int lastIndex = 0;

    PluginProcessor* pd;
    bool editable = true;
};

struct ArrayPropertiesPanel final : public PropertiesPanelProperty
    , public Value::Listener {
    class AddArrayButton final : public Component {

        bool mouseIsOver = false;

    public:
        std::function<void()> onClick = [] { };

        void paint(Graphics& g) override
        {
            auto const bounds = getLocalBounds();
            auto textBounds = bounds;
            auto const iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto const colour = findColour(PlugDataColour::sidebarTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
            }

            Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
            Fonts::drawText(g, "Add new array", textBounds, colour, 14);
        }

        bool hitTest(int const x, int const y) override
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
    SmallArray<SafePointer<GraphicalArray>> graphs;

    AddArrayButton addButton;
    OwnedArray<SmallIconButton> deleteButtons;
    HeapArray<Value> nameValues;

    std::function<void()> syncCanvas = [] { };

    ArrayPropertiesPanel(std::function<void()> const& addArrayCallback, std::function<void()> const& syncCanvasFunc)
        : PropertiesPanelProperty("array")
    {
        setHideLabel(true);

        addAndMakeVisible(addButton);
        addButton.onClick = addArrayCallback;
        syncCanvas = syncCanvasFunc;
    }

    void reloadGraphs(SmallArray<SafePointer<GraphicalArray>> const& safeGraphs)
    {
        properties.clear();
        nameValues.clear();
        deleteButtons.clear();

        graphs = safeGraphs;

        nameValues.reserve(graphs.size());

        for (auto graph : graphs) {
            addAndMakeVisible(properties.add(new PropertiesPanel::EditableComponent<String>("Name", graph->name)));
            addAndMakeVisible(properties.add(new PropertiesPanel::EditableComponent<int>("Size", graph->size)));
            addAndMakeVisible(properties.add(new PropertiesPanel::RangeComponent("Range", graph->range, false)));
            addAndMakeVisible(properties.add(new PropertiesPanel::BoolComponent("Save contents", graph->saveContents, { "No", "Yes" })));
            addAndMakeVisible(properties.add(new PropertiesPanel::ComboComponent("Draw Style", graph->drawMode, { "Points", "Polygon", "Bezier curve" })));

            // To detect name changes, so we can redraw the array title
            nameValues.add(Value());
            auto& nameValue = nameValues[nameValues.size() - 1];
            nameValue.referTo(graph->name);
            nameValue.addListener(this);
            auto* deleteButton = deleteButtons.add(new SmallIconButton(Icons::Clear));
            deleteButton->onClick = [graph] {
                graph->deleteArray();
            };
            addAndMakeVisible(deleteButton);
        }

        auto const newHeight = 184 * graphs.size() + 40;
        setPreferredHeight(newHeight);
        if (auto const* propertiesPanel = findParentComponentOfClass<PropertiesPanel>()) {
            propertiesPanel->updatePropHolderLayout();
        }

        repaint();
    }

    void valueChanged(Value& v) override
    {
        // when array parameters are changed we need to resync the canavs to PD
        syncCanvas();
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));

        auto const numGraphs = properties.size() / 5;
        for (int i = 0; i < numGraphs; i++) {
            if (!graphs[i])
                continue;

            auto const start = i * 184 - 8;
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(0.0f, start + 32, getWidth(), 154, Corners::largeCornerRadius);

            Fonts::drawStyledText(g, graphs[i]->name.toString(), 8, start + 2, getWidth() - 16, 32, findColour(PlugDataColour::sidebarTextColourId), Semibold, 14.5f);
        }

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));

        for (int i = 0; i < properties.size(); i++) {
            if (i % 5 == 4)
                continue;
            auto const y = properties[i]->getBottom();
            g.drawHorizontalLine(y, properties[i]->getX() + 10, properties[i]->getRight() - 10);
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().translated(0, -8);
        for (int i = 0; i < properties.size(); i++) {
            if (i % 5 == 0) {
                auto const deleteButtonBounds = b.removeFromTop(34).removeFromRight(28);
                deleteButtons[i / 5]->setBounds(deleteButtonBounds);
            }
            properties[i]->setBounds(b.removeFromTop(30));
        }

        addButton.setBounds(b.removeFromBottom(32).reduced(0, 4));
    }
};

class ArrayListView final : public PropertiesPanel
    , public Value::Listener {
public:
    ArrayListView(pd::Instance* instance, void* arr)
        : array(arr, instance)
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

        PropertiesArray properties;

        if (auto ptr = array.get<t_fake_garray>()) {
            auto const* arr = garray_getarray(ptr.cast<t_garray>());
            auto const* vec = reinterpret_cast<t_word*>(garray_vec(ptr.cast<t_garray>()));

            auto const numProperties = std::min(arr->a_n, 1<<14); // Limit it to something reasonable to make sure it doesn't take forever to load
            properties.resize(numProperties);

            for (int i = 0; i < numProperties; i++) {
                auto& value = *arrayValues.add(new Value(vec[i].w_float));
                value.addListener(this);
                auto* property = new EditableComponent<float>(String(i), value);
                auto* label = dynamic_cast<DraggableNumber*>(property->label.get());

                property->setRangeMin(ptr->x_glist->gl_y2);
                property->setRangeMax(ptr->x_glist->gl_y1);

                // Only send this after drag end so it doesn't interrupt the drag action
                label->dragEnd = [this] {
                    if (auto ptr = array.get<t_fake_garray>()) {
                        plugdata_forward_message(ptr->x_glist, gensym("redraw"), 0, nullptr);
                    }
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
            auto* vec = reinterpret_cast<t_word*>(garray_vec(ptr.cast<t_garray>()));

            for (int i = 0; i < arrayValues.size(); i++) {
                auto& value = *arrayValues[i];
                if (v.refersToSameSourceAs(value)) {
                    vec[i].w_float = getValue<float>(value);
                    break;
                }
            }
        }
    }

    OwnedArray<Value> arrayValues;
    pd::WeakReference array;
};

class ArrayEditorDialog final : public Component {
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

    ArrayEditorDialog(PluginProcessor* instance, SmallArray<void*> const& arrays, Object* parent)
        : resizer(this, &constrainer)
        , pd(instance)
    {
        for (auto* arr : arrays) {
            auto* graph = graphs.add(new GraphicalArray(pd, arr, parent));
            addChildComponent(graph);

            auto* list = lists.add(new ArrayListView(pd, arr));
            addChildComponent(list);
        }
        if (graphs.size()) {
            graphs[0]->setVisible(true);
        }

        for (int i = 0; i < graphs.size(); i++) {
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

        listViewButton.onClick = [this] {
            updateVisibleGraph();
        };
        graphViewButton.onClick = [this] {
            updateVisibleGraph();
        };
        selectedArrayCombo.onChange = [this] {
            updateVisibleGraph();
        };

        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 300);

        closeButton->onClick = [this] {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    _this->onClose();
                }
            });
        };

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);
        
        resizer.setAllowHostManagedResize(false);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(600, 400));

        addAndMakeVisible(resizer);
        updateGraphs();
        updateVisibleGraph();
    }

    void updateVisibleGraph()
    {
        for (int i = 0; i < graphs.size(); i++) {
            graphs[i]->setVisible(i == selectedArrayCombo.getSelectedItemIndex() && graphViewButton.getToggleState());
        }
        for (int i = 0; i < graphs.size(); i++) {
            lists[i]->setVisible(i == selectedArrayCombo.getSelectedItemIndex() && listViewButton.getToggleState());
        }
    }

    void resized() override
    {
        auto constexpr toolbarHeight = 38;
        auto constexpr buttonWidth = 120;
        auto const centre = getWidth() / 2;

        graphViewButton.setBounds(centre - buttonWidth, 1, buttonWidth, toolbarHeight - 2);
        listViewButton.setBounds(centre, 1, buttonWidth, toolbarHeight - 2);

        selectedArrayCombo.setBounds(8, 8, toolbarHeight * 2.5f, toolbarHeight - 16);

        resizer.setBounds(getLocalBounds());

        auto const closeButtonBounds = getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5);
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
        pd->lockAudioThread();

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
        auto constexpr toolbarHeight = 38;
        auto b = getLocalBounds();
        auto const titlebarBounds = b.removeFromTop(toolbarHeight).toFloat();
        auto const arrayBounds = b.toFloat();

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
    
    GraphTicks ticks;

    // Array component
    ArrayObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        reinitialiseGraphs();

        setInterceptsMouseClicks(false, true);

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamCustom([_this = SafePointer(this)] {
            if (!_this)
                return static_cast<ArrayPropertiesPanel*>(nullptr);

            SmallArray<SafePointer<GraphicalArray>> safeGraphs;
            for (auto* graph : _this->graphs) {
                safeGraphs.add(graph);
            }

            auto* panel = new ArrayPropertiesPanel([_this] {
                if(_this) _this->addArray(); }, [_this] {
                if(_this) _this->cnv->synchronise(); });

            panel->reloadGraphs(safeGraphs);
            _this->propertiesPanel = panel;

            return panel;
        });

        updateLabel();
    }

    void onConstrainerCreate() override
    {
        constrainer->setSizeLimits(50 - Object::doubleMargin, 40 - Object::doubleMargin, 99999, 99999);
    }

    void reinitialiseGraphs()
    {
        // Close the dialog they could be holding a pointer to an old graph object
        if (dialog != nullptr)
            dialog.reset(nullptr);

        auto arrays = getArrays();
        graphs.clear();

        for (int i = 0; i < arrays.size(); i++) {
            auto* graph = graphs.add(new GraphicalArray(cnv->pd, arrays[i], object));
            graph->setBounds(getLocalBounds());
            graph->reloadGraphs = [this] {
                MessageManager::callAsync([_this = SafePointer(this)] {
                    if (_this)
                        _this->reinitialiseGraphs();
                });
            };
            addAndMakeVisible(graph);
        }

        updateGraphs();

        SmallArray<SafePointer<GraphicalArray>> safeGraphs;
        for (auto* graph : graphs)
            safeGraphs.add(graph);

        if (propertiesPanel)
            propertiesPanel->reloadGraphs(safeGraphs);

        updateLabel();
    }

    void addArray()
    {
        if (auto glist = ptr.get<_glist>()) {
            graph_array(glist.get(), pd::Interface::getUnusedArrayName(), gensym("float"), 100, 0);
        }
        cnv->synchronise();
        reinitialiseGraphs();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto const backgroundColour = nvgRGBA(0, 0, 0, 0);
        auto const selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto const outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, object->isSelected() ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        for (auto* graph : graphs) {
            graph->render(nvg);
        }

        nvgStrokeColor(nvg, cnv->guiObjectInternalOutlineCol);
        ticks.render(nvg, b);
    }

    bool isTransparent() override { return true; };

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

        auto title = String();
        for (auto const* graph : graphs) {
            title += graph->getUnexpandedName() + (graph != graphs.getLast() ? "," : "");
        }

        if (title.isNotEmpty()) {
            int constexpr fontHeight = 14.0f;
            ObjectLabel* label;
            if (labels.isEmpty()) {
                label = labels.add(new ObjectLabel());
            } else {
                label = labels[0];
            }

            auto bounds = object->getBounds().reduced(Object::margin).removeFromTop(fontHeight + 2).withWidth(Font(fontHeight).getStringWidth(title));

            bounds.translate(2, -(fontHeight + 2));

            label->setFont(Font(fontHeight));
            label->setBounds(bounds);
            label->setText(title, dontSendNotification);
            label->setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));

            object->cnv->addAndMakeVisible(label);
        } else {
            labels.clear();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto glist = ptr.get<_glist>()) {
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(cnv->patch.getRawPointer(), &glist->gl_obj.te_g, &x, &y, &w, &h);

            return { x, y, glist->gl_pixwidth, glist->gl_pixheight };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto glist = ptr.get<t_glist>()) {
            auto* patch = cnv->patch.getRawPointer();

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
        for (auto* graph : graphs) {
            graph->updateParameters();
        }
        if (auto glist = ptr.get<t_glist>()) {
            sizeProperty = VarArray { var(glist->gl_pixwidth), var(glist->gl_pixheight) };
            ticks.update(glist.get());
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto glist = ptr.get<t_glist>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(glist->gl_pixwidth), var(glist->gl_pixheight) });
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto glist = ptr.get<t_glist>()) {
                glist->gl_pixwidth = width;
                glist->gl_pixheight = height;
            }

            object->updateBounds();
        }
    }

    SmallArray<void*> getArrays() const
    {
        if (auto c = ptr.get<t_canvas>()) {
            SmallArray<void*> arrays;

            if (auto* x = c->gl_list) {
                // We can't compare symbols here, that breaks multi-instance support
                if(String::fromUTF8(x->g_pd->c_name->s_name) == "array") {
                    arrays.add(x);
                }
                while ((x = x->g_next)) {
                    if(String::fromUTF8(x->g_pd->c_name->s_name) == "array") {
                        arrays.add(x);
                    }
                }
            }

            return arrays;
        }

        return {};
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open array editor", [this, _this = SafePointer(this)] {
            if (!_this)
                return;

            if (dialog) {
                dialog->toFront(true);
                return;
            }

            auto arrays = getArrays();
            if (arrays.size()) {
                dialog = std::make_unique<ArrayEditorDialog>(cnv->pd, arrays, object);
                dialog->onClose = [this] {
                    dialog.reset(nullptr);
                };
            } else {
                pd->logWarning("Can't open: contains no arrays");
            }
        });
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("redraw"): {
            updateGraphs();
            if (dialog) {
                dialog->updateGraphs();
            }
            break;
        }
        case hash("yticks"):
        case hash("xticks"): {
            if (auto glist = ptr.get<t_canvas>()) {
                ticks.update(glist.get());
            }
            repaint();
            break;
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

    void lock(bool const isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        openArrayEditor();
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        bool const canOpenMenu = [this] {
            if (auto c = ptr.get<t_canvas>()) {
                return c->gl_list != nullptr;
            }
            return false;
        }();

        menu.addItem("Open array editor", canOpenMenu, false, [_this = SafePointer(this)] {
            if (!_this)
                return;

            _this->openArrayEditor();
        });
    }

    void openArrayEditor()
    {
        if (editor) {
            editor->toFront(true);
            return;
        }

        if (auto c = ptr.get<t_canvas>()) {
            SmallArray<void*> arrays;

            t_glist* x = c.get();
            if (auto* gl = x->gl_list ? pd_checkglist(&x->gl_list->g_pd)->gl_list : nullptr) {
                arrays.add(gl);
                while ((gl = gl->g_next)) {
                    arrays.add(x);
                }
            }

            if (!arrays.empty() && arrays[0] != nullptr) {
                editor = std::make_unique<ArrayEditorDialog>(cnv->pd, arrays, object);
                editor->onClose = [this] {
                    editor.reset(nullptr);
                };
            } else {
                pd->logWarning("array define: cannot open non-existent array");
            }
        }
    }

    void receiveObjectMessage(hash32 symbol, SmallArray<pd::Atom> const& atoms) override
    {
    }
};
