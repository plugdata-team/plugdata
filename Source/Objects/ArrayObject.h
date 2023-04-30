/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
#include "x_libpd_extra_utils.h"

void garray_arraydialog(t_garray* x, t_symbol* name, t_floatarg fsize, t_floatarg fflags, t_floatarg deleteit);
}

class PdArray {
public:
    enum DrawType {
        Points,
        Polygon,
        Curve
    };

    PdArray(void* arrayPtr, void* instancePtr)
        : ptr(arrayPtr)
        , instance(instancePtr)
    {
    }

    // The default constructor.
    PdArray() = default;

    bool willSaveContent() const
    {
        return libpd_array_get_saveit(ptr);
    }

    // Gets the name of the array.
    String getExpandedName() const
    {
        return String::fromUTF8(libpd_array_get_name(ptr));
    }

    // Gets the text label of the array.
    String getUnexpandedName() const
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        return String::fromUTF8(libpd_array_get_unexpanded_name(ptr));
    }

    PdArray::DrawType getDrawType() const
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));

        return static_cast<DrawType>(libpd_array_get_style(ptr));
    }

    // Gets the scale of the array.
    std::array<float, 2> getScale() const
    {
        float min = -1, max = 1;
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        libpd_array_get_scale(ptr, &min, &max);

        if (min == max)
            max += 1e-6;

        return { min, max };
    }

    bool getEditMode()
    {
        return libpd_array_get_editmode(ptr);
    }

    void setEditMode(bool editMode)
    {
        libpd_array_set_editmode(ptr, editMode);
    }

    // Gets the scale of the array.
    int size() const
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        return libpd_array_get_size(ptr);
    }

    void setScale(std::array<float, 2> scale)
    {
        auto& [min, max] = scale;
        libpd_set_instance(static_cast<t_pdinstance*>(instance));

        libpd_array_set_scale(ptr, min, max);
    }

    // Gets the values of the array.
    void read(std::vector<float>& output) const
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        int const size = libpd_array_get_size(ptr);
        output.resize(static_cast<size_t>(size));
        libpd_array_read(output.data(), ptr, 0, size);
    }

    // Writes the values of the array.
    void write(std::vector<float> const& input)
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        libpd_array_write(ptr, 0, input.data(), static_cast<int>(input.size()));
    }

    // Writes a value of the array.
    void write(const size_t pos, float const input)
    {
        libpd_set_instance(static_cast<t_pdinstance*>(instance));
        libpd_array_write(ptr, static_cast<int>(pos), &input, 1);
    }

    void* ptr = nullptr;
    void* instance = nullptr;
};

class GraphicalArray : public Component {
public:
    Object* object;

    GraphicalArray(PluginProcessor* instance, PdArray& arr, Object* parent)
        : array(arr)
        , edited(false)
        , pd(instance)
        , object(parent)
    {
        if (!array.ptr)
            return;

        vec.reserve(8192);
        temp.reserve(8192);
        try {
            array.read(vec);
        } catch (...) {
            error = true;
        }

        setInterceptsMouseClicks(true, false);
        setOpaque(false);

        MessageManager::callAsync([this] {
            object->getConstrainer()->setMinimumSize(100 - Object::doubleMargin, 40 - Object::doubleMargin);
        });
    }

    void setArray(PdArray& graph)
    {
        if (!array.ptr)
            return;
        array = graph;
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
            std::array<float, 2> scale = array.getScale();
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

            switch (array.getDrawType()) {
            case PdArray::DrawType::Curve: {

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

                g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
                g.strokePath(p, PathStrokeType(1));
                break;
            }
            case PdArray::DrawType::Polygon: {
                int startY = h - (std::clamp(points[0], scale[0], scale[1]) - scale[0]) * dh;
                Point<float> lastPoint = Point<float>(0, startY);
                Point<float> newPoint;

                Path p;
                for (size_t i = 1; i < points.size(); i++) {
                    float const y = h - (std::clamp(points[i], scale[0], scale[1]) - scale[0]) * dh;
                    newPoint = Point<float>(static_cast<float>(i) * dw, y);

                    p.addLineSegment({ lastPoint, newPoint }, 1.0f);
                    lastPoint = newPoint;
                }

                if (invert)
                    p.applyTransform(AffineTransform::verticalFlip(getHeight()));

                g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
                g.fillPath(p);
                break;
            }
            case PdArray::DrawType::Points: {
                g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));

                float const dw_points = w / static_cast<float>(points.size());

                for (size_t i = 0; i < points.size(); i++) {
                    float y = h - (std::clamp(points[i], scale[0], scale[1]) - scale[0]) * dh;
                    if (invert)
                        y = getHeight() - y;
                    g.drawLine(static_cast<float>(i) * dw_points, y, static_cast<float>(i + 1) * dw_points, y, 2.0f);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        if (error) {
            // TODO: error colour
            Fonts::drawText(g, "array " + array.getUnexpandedName() + " is invalid", 0, 0, getWidth(), getHeight(), object->findColour(PlugDataColour::canvasTextColourId), 15, Justification::centred);
            error = false;
        } else {
            paintGraph(g);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (error || !array.getEditMode())
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
        if (error || !array.getEditMode())
            return;

        auto const s = static_cast<float>(vec.size() - 1);
        auto const w = static_cast<float>(getWidth());
        auto const h = static_cast<float>(getHeight());
        auto const x = static_cast<float>(e.x);
        auto const y = static_cast<float>(e.y);

        std::array<float, 2> scale = array.getScale();

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

        pd->enqueueFunction(
            [_this = SafePointer(this), interpStart, changed]() mutable {
                try {
                    for (int n = 0; n < changed.size(); n++) {
                        _this->array.write(interpStart + n, changed[n]);
                    }
                } catch (...) {
                    _this->error = true;
                }
            });

        lastIndex = index;

        pd->enqueueDirectMessages(array.ptr, stringArray);
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (error || !array.getEditMode())
            return;
        edited = false;
    }

    void update()
    {
        if (!edited) {
            error = false;
            try {
                array.read(temp);
            } catch (...) {
                error = true;
            }
            if (temp != vec) {
                vec.swap(temp);
                repaint();
            }
        }
    }

    PdArray array;
    std::vector<float> vec;
    std::vector<float> temp;
    std::atomic<bool> edited;
    bool error = false;
    const String stringArray = "array";

    int lastIndex = 0;

    PluginProcessor* pd;
};

class ArrayEditorDialog : public Component
    , public Timer {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;

public:
    std::function<void()> onClose;
    GraphicalArray graph;
    PluginProcessor* pd;
    String title;

    ArrayEditorDialog(PluginProcessor* instance, PdArray& arr, Object* parent)
        : resizer(this, &constrainer)
        , title(arr.getExpandedName())
        , graph(instance, arr, parent)
        , pd(instance)
    {

        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(DocumentWindow::closeButton));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 200);

        closeButton->onClick = [this]() {
            MessageManager::callAsync([this]() {
                onClose();
            });
        };

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(600, 400));

        addAndMakeVisible(graph);
        addAndMakeVisible(resizer);

        startTimer(40);
    }

    void resized() override
    {
        resizer.setBounds(getLocalBounds());
        closeButton->setBounds(getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5));
        graph.setBounds(getLocalBounds().withTrimmedTop(40));
    }

    void timerCallback() override
    {
        if (!pd->tryLockAudioThread())
            return;

        int currentSize = graph.array.size();
        if (graph.vec.size() != currentSize) {

            graph.vec.resize(currentSize);
        }
        graph.update();

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
        g.setColour(findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.drawHorizontalLine(39, 0, getWidth());

        if (!title.isEmpty()) {
            Fonts::drawText(g, title, 0, 0, getWidth(), 40, findColour(PlugDataColour::canvasTextColourId), 15, Justification::centred);
        }
    }
};

class ArrayObject final : public ObjectBase
    , public Timer {
public:
    // Array component
    ArrayObject(void* obj, Object* object)
        : ObjectBase(obj, object)
        , array(getArray())
        , graph(cnv->pd, array, object)
    {
        setInterceptsMouseClicks(false, true);
        graph.setBounds(getLocalBounds());
        addAndMakeVisible(&graph);

        startTimer(20);
    }

    void timerCallback() override
    {
        // Check if size has changed
        int currentSize = graph.array.size();
        if (graph.vec.size() != currentSize) {

            graph.vec.resize(currentSize);
            size = currentSize;
        }

        // Update values
        graph.update();
    }

    void updateLabel() override
    {
        int fontHeight = 14.0f;

        const String text = array.getExpandedName();

        if (text.isNotEmpty()) {
            if (!label) {
                label = std::make_unique<ObjectLabel>(object);
            }

            auto bounds = object->getBounds().reduced(Object::margin).removeFromTop(fontHeight + 2);

            bounds.translate(2, -(fontHeight + 2));

            label->setFont(Font(fontHeight));
            label->setBounds(bounds);
            label->setText(text, dontSendNotification);

            label->setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));

            object->cnv->addAndMakeVisible(label.get());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto* glist = static_cast<_glist*>(ptr);
        auto bounds = Rectangle<int>(x, y, glist->gl_pixwidth, glist->gl_pixheight);

        pd->unlockAudioThread();

        return bounds;
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Name", tString, cGeneral, &name, {} },
            { "Size", tInt, cGeneral, &size, {} },
            { "Draw Mode", tCombo, cGeneral, &drawMode, { "Points", "Polygon", "Bezier Curve" } },
            { "Y Range", tRange, cGeneral, &range, {} },
            { "Save Contents", tBool, cGeneral, &saveContents, { "No", "Yes" } },
        };
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* array = static_cast<_glist*>(ptr);
        array->gl_pixwidth = b.getWidth();
        array->gl_pixheight = b.getHeight();
    }

    void resized() override
    {
        graph.setBounds(getLocalBounds());
    }

    void update() override
    {
        auto scale = array.getScale();
        Array<var> arr = { var(scale[0]), var(scale[1]) };
        range = var(arr);
        size = var(static_cast<int>(graph.array.size()));
        saveContents = array.willSaveContent();
        name = String(array.getUnexpandedName());
        drawMode = static_cast<int>(array.getDrawType()) + 1;

        labelColour = object->findColour(PlugDataColour::canvasTextColourId).toString();
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

        cnv->pd->enqueueFunction(
            [this, _this = SafePointer(this), arrName, arrSize, flags]() mutable {
                if (!_this)
                    return;

                auto* garray = reinterpret_cast<t_garray*>(static_cast<t_canvas*>(ptr)->gl_list);
                garray_arraydialog(garray, _this->pd->generateSymbol(arrName), arrSize, static_cast<float>(flags), 0.0f);

                MessageManager::callAsync(
                    [this, _this]() {
                        if (!_this)
                            return;
                        array = getArray();
                        graph.setArray(array);
                        updateLabel();
                    });
            });

        graph.repaint();
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(name) || value.refersToSameSourceAs(size) || value.refersToSameSourceAs(drawMode) || value.refersToSameSourceAs(saveContents)) {
            updateSettings();
        } else if (value.refersToSameSourceAs(range)) {
            auto min = static_cast<float>(range.getValue().getArray()->getReference(0));
            auto max = static_cast<float>(range.getValue().getArray()->getReference(1));
            graph.array.setScale({ min, max });
            graph.repaint();
        } else {
            ObjectBase::valueChanged(value);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    PdArray getArray() const
    {
        auto* c = static_cast<t_canvas*>(ptr);
        auto* glist = reinterpret_cast<t_garray*>(c->gl_list);

        return { glist, cnv->pd->m_instance };
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

        dialog = std::make_unique<ArrayEditorDialog>(cnv->pd, array, object);
        dialog->onClose = [this]() {
            dialog.reset(nullptr);
        };
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("symbol"),
            hash("list"),
            hash("edit")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("symbol"):
        case hash("list"): {
            break;
        }
        case hash("edit"): {
            if (!atoms.empty()) {
                editable = atoms[0].getFloat();
                setInterceptsMouseClicks(false, editable);
            }
        }
        default:
            break;
        }
    }

private:
    Value name, size, drawMode, saveContents, range;

    PdArray array;
    GraphicalArray graph;
    std::unique_ptr<ArrayEditorDialog> dialog = nullptr;

    Value labelColour;
    bool editable = true;
};

// Actual text object, marked final for optimisation
class ArrayDefineObject final : public TextBase {
    std::unique_ptr<ArrayEditorDialog> editor = nullptr;

public:
    ArrayDefineObject(void* obj, Object* parent, bool isValid = true)
        : TextBase(obj, parent, isValid)
    {
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        openArrayEditor();
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openArrayEditor()
    {
        if (editor) {
            editor->toFront(true);
            return;
        }

        auto* c = reinterpret_cast<t_canvas*>(static_cast<t_canvas*>(ptr)->gl_list);
        auto* glist = reinterpret_cast<t_garray*>(c->gl_list);
        auto array = PdArray(glist, cnv->pd->m_instance);

        editor = std::make_unique<ArrayEditorDialog>(cnv->pd, array, object);
        editor->onClose = [this]() {
            editor.reset(nullptr);
        };
    }

    void openFromMenu() override
    {
        openArrayEditor();
    }
};
