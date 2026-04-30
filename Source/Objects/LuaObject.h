/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

extern "C" {
#include <pd-lua/luas/lua/lua.h>

#define NANOSVG_IMPLEMENTATION
#include <pd-lua/svg/nanosvg.h>

#define PLUGDATA 1
#include <pd-lua/pdlua.h>
#undef PLUGDATA
}

struct LuaPropertiesPanel {
    struct PropertyItem {
        enum class Type { Check,
            Text,
            Colour,
            Int,
            Float,
            Combo };
        Type type;
        String label;
        String method;
        String initString;
        StringArray options;
        float initFloat = 0.f;
        float min, max;
    };

    struct PropertyFrame {
        String title;
        SmallArray<PropertyItem> items;
    };

    static inline auto allPropertiesTargets = UnorderedMap<void*, SmallArray<LuaPropertiesPanel*>>();

    static void propertiesCallback(void* target, t_symbol* sym, int argc, t_atom* argv)
    {
        auto symbol = hash(sym->s_name);
        auto atoms = pd::Atom::fromAtoms(argc, argv);

        for (auto* propertiesPanel : allPropertiesTargets[static_cast<t_pdlua*>(target)]) {
            if (symbol == hash("add_frame_property") && atoms.size() >= 1 && propertiesPanel->object) {
                auto* frame = propertiesPanel->newFrame(atoms[0].toString());
                propertiesPanel->object->objectParameters.addParamCustom([object = propertiesPanel->object, frame]() -> PropertiesPanelProperty* {
                    return new LuaPropertiesPanel::LuaPropertiesFrame(frame, object);
                });
            } else if (symbol == hash("add_check_property") && atoms.size() >= 3) {
                propertiesPanel->addProperty({ .type = LuaPropertiesPanel::PropertyItem::Type::Check,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .initFloat = atoms[2].getFloat() });
            } else if (symbol == hash("add_text_property") && atoms.size() >= 3) {
                propertiesPanel->addProperty({
                    .type = LuaPropertiesPanel::PropertyItem::Type::Text,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .initString = atoms[2].toString(),
                });
            } else if (symbol == hash("add_color_property") && atoms.size() >= 3) {
                propertiesPanel->addProperty({ .type = LuaPropertiesPanel::PropertyItem::Type::Colour,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .initString = atoms[2].toString() });
            } else if (symbol == hash("add_int_property") && atoms.size() >= 5) {
                propertiesPanel->addProperty({ .type = LuaPropertiesPanel::PropertyItem::Type::Int,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .initFloat = atoms[2].getFloat(),
                    .min = atoms[3].getFloat(),
                    .max = atoms[4].getFloat()

                });
            } else if (symbol == hash("add_float_property") && atoms.size() >= 5) {
                propertiesPanel->addProperty({ .type = LuaPropertiesPanel::PropertyItem::Type::Float,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .initFloat = atoms[2].getFloat(),
                    .min = atoms[3].getFloat(),
                    .max = atoms[4].getFloat()

                });
            } else if (symbol == hash("add_combo_property") && atoms.size() >= 4) {
                StringArray options;
                for (int i = 3; i < atoms.size(); i++) {
                    options.add(atoms[i].toString());
                }
                propertiesPanel->addProperty({ .type = LuaPropertiesPanel::PropertyItem::Type::Combo,
                    .label = atoms[0].toString(),
                    .method = atoms[1].toString(),
                    .options = options,
                    .initFloat = atoms[2].getFloat() });
            }
        }
    }

    struct LuaPropertiesFrame final : public PropertiesPanelProperty
        , public Value::Listener {
        OwnedArray<PropertiesPanelProperty> properties;
        PropertyFrame frame;
        ObjectBase* object;

        LuaPropertiesFrame(PropertyFrame const* f, ObjectBase* object)
            : PropertiesPanelProperty(f->title)
            , frame(*f)
            , object(object)
        {
            setHideLabel(true);

            for (auto const& item : frame.items) {
                switch (item.type) {
                case PropertyItem::Type::Check: {
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(item.initFloat != 0.f))));
                    auto* prop = new PropertiesPanel::BoolComponent(item.label, value, { "No", "Yes" });
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                case PropertyItem::Type::Text: {
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(item.initString))));
                    auto* prop = new PropertiesPanel::EditableComponent<String>(item.label, value);
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                case PropertyItem::Type::Colour: {
                    auto colour = "ff" + item.initString.fromFirstOccurrenceOf("#", false, false);
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(colour))));
                    auto* prop = new PropertiesPanel::InspectorColourComponent(item.label, value);
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                case PropertyItem::Type::Int: {
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(item.initFloat))));
                    auto* prop = new PropertiesPanel::EditableComponent<int>(item.label, value, true, item.min, item.max);
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                case PropertyItem::Type::Float: {
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(item.initFloat))));
                    auto* prop = new PropertiesPanel::EditableComponent<float>(item.label, value, true, item.min, item.max);
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                case PropertyItem::Type::Combo: {
                    auto& value = *ownedValues.add(std::make_unique<Value>(SynchronousValue(var(item.initFloat))));
                    auto* prop = new PropertiesPanel::ComboComponent(item.label, value, item.options);
                    value.addListener(this);
                    methods.emplace_back(&value, item.type, item.method);
                    addAndMakeVisible(properties.add(prop));
                    break;
                }
                }
            }
            setPreferredHeight(24 + (properties.size() * 30));
        }

        void valueChanged(Value& v) override
        {
            for (auto& [value, type, method] : methods) {
                if (value->refersToSameSourceAs(v)) {
                    auto methodSym = object->pd->generateSymbol(method);
                    if (type == LuaPropertiesPanel::PropertyItem::Type::Colour) {
                        auto* hex = object->pd->generateSymbol("#" + value->toString().substring(2));
                        object->sendMessage("dialog", { object->pd->generateSymbol("colorpicker"), methodSym, hex });
                    } else if (type == LuaPropertiesPanel::PropertyItem::Type::Text) {
                        object->sendMessage("dialog", { object->pd->generateSymbol("text"), methodSym, object->pd->generateSymbol(value->toString()) });
                    } else // combo/number/check are all just float
                    {
                        object->sendMessage("dialog", { object->pd->generateSymbol("number"), methodSym, static_cast<float>(value->getValue()) });
                    }
                    break;
                }
            }
        }

        void paint(Graphics& g) override
        {
            g.fillAll(PlugDataColours::sidebarBackgroundColour);

            // Frame background behind all properties
            g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
            g.fillRoundedRectangle(0.0f, 24.0f, getWidth(), getHeight() - 24, Corners::largeCornerRadius);

            // Frame title
            Fonts::drawStyledText(g, frame.title,
                8, 0, getWidth() - 16, 28,
                PlugDataColours::sidebarTextColour, Semibold, 14.5f);

            // Dividers between properties (skip after last)
            g.setColour(PlugDataColours::toolbarOutlineColour);
            for (int i = 0; i < properties.size() - 1; i++) {
                auto const y = properties[i]->getBottom();
                g.drawHorizontalLine(y, properties[i]->getX() + 10, properties[i]->getRight() - 10);
            }
        }

        void resized() override
        {
            auto b = getLocalBounds().withTrimmedTop(24); // leave room for title
            for (auto* prop : properties)
                prop->setBounds(b.removeFromTop(30));
        }

    private:
        SmallArray<std::tuple<Value*, PropertyItem::Type, String>> methods;
        OwnedArray<Value> ownedValues;
    };

    LuaPropertiesPanel(void* pdlua, ObjectBase* object)
        : object(object)
    {
        allPropertiesTargets[pdlua].add(this);
    }

    PropertyFrame* newFrame(String const& title)
    {
        currentFrame = pendingFrames.add(std::unique_ptr<PropertyFrame> { new PropertyFrame { title, { } } });
        return currentFrame;
    }

    void addProperty(PropertyItem const& item)
    {
        if (currentFrame)
            currentFrame->items.add(item);
    }

    ObjectBase* object;
    OwnedArray<PropertyFrame> pendingFrames;
    PropertyFrame* currentFrame = nullptr;
};

class LuaObject final : public ObjectBase
    , private Value::Listener {
    Colour currentColour;

    t_symbol* pdluaxSymbol;
    t_symbol* pdluaxjitSymbol;

    bool isSelected = false;
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

    LuaPropertiesPanel properties;

    int currentTouchIndex = -1;

    UnorderedSegmentedMap<int, NVGFramebuffer> framebuffers;
    UnorderedSegmentedMap<hash32, std::pair<NVGImage, Rectangle<int>>> images;

    struct LuaGuiMessage {
        t_symbol* symbol;
        SmallArray<t_atom> data;
        int size;

        LuaGuiMessage() { }

        LuaGuiMessage(t_symbol* sym, int const argc, t_atom* argv)
            : symbol(sym)
        {
            data = SmallArray<t_atom>(argv, argv + argc);
            size = argc;
        }

        LuaGuiMessage(LuaGuiMessage const& other) noexcept
        {
            symbol = other.symbol;
            size = other.size;
            data = other.data;
        }

        LuaGuiMessage& operator=(LuaGuiMessage const& other) noexcept
        {
            // Check for self-assignment
            if (this != &other) {
                symbol = other.symbol;
                size = other.size;
                data = other.data;
            }

            return *this;
        }
    };

    CriticalSection frameSwapLock;
    UnorderedSegmentedMap<int, HeapArray<LuaGuiMessage>> guiCommandBuffer;
    UnorderedSegmentedMap<int, HeapArray<LuaGuiMessage>> currentFrame;

    static inline auto allDrawTargets = UnorderedMap<t_pdlua*, SmallArray<LuaObject*>>();

public:
    LuaObject(pd::WeakReference ptr, Object* parent)
        : ObjectBase(ptr, parent)
        , properties(ptr.getRaw<void>(), this)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            pdlua->gfx.plugdata_draw_callback = &drawCallback;
            pdlua->properties.plugdata_properties_callback = &LuaPropertiesPanel::propertiesCallback;
            allDrawTargets[pdlua.get()].add(this);

            libpd_set_instance(&pd_maininstance);
            pdluaxSymbol = gensym("pdluax");
            pdluaxjitSymbol = gensym("pdluaxjit");
            pd->setThis();
        }

        object->editor->nvgSurface.addBufferedObject(this);
        parentHierarchyChanged();
    }

    ~LuaObject() override
    {
        pd->lockAudioThread();
        auto& listeners = allDrawTargets[ptr.getRawUnchecked<t_pdlua>()];
        listeners.erase(std::ranges::remove(listeners, this).begin(), listeners.end());
        pd->unlockAudioThread();

        zoomScale.removeListener(this);
        object->editor->nvgSurface.removeBufferedObject(this);
    }

    // We can only attach the zoomscale to canvas after the canvas has been added to its own parent
    // When initialising, this isn't always the case
    void parentHierarchyChanged() override
    {
        // We need to get the actual zoom from the top level canvas, not of the graph this happens to be inside of
        auto const* topLevelCanvas = cnv;
        while (auto const* nextCanvas = topLevelCanvas->findParentComponentOfClass<Canvas>()) {
            topLevelCanvas = nextCanvas;
        }

        zoomScale.referTo(topLevelCanvas->zoomScale);
        zoomScale.addListener(this);
        sendRepaintMessage();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_pdlua>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.cast<t_gobj>(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, gobj->gfx.width + 1, gobj->gfx.height + 1);
        }

        return { };
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto gobj = ptr.get<t_pdlua>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, gobj.cast<t_gobj>(), b.getX(), b.getY());
            gobj->gfx.width = b.getWidth() - 1;
            gobj->gfx.height = b.getHeight() - 1;
        }

        sendRepaintMessage();
    }

    void updateSizeProperty() override
    {
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open lua editor", [_this = SafePointer(this)] {
            if (!_this)
                return;
            _this->sendMessage("menu-open");
        });
        menu.addItem("Reload lua object", [_this = SafePointer(this)] {
            if (!_this)
                return;

            // prevent potential crash if this was selected
            if (auto* s = _this->cnv->editor->getSidebarForPanel(Sidebar::InspectorPanel))
                s->hideParameters();

            if (auto pdlua = _this->ptr.get<t_pd>()) {
                // Reload the lua script
                if (_this->pdluaxSymbol->s_thing)
                    pd_typedmess(_this->pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                if (_this->pdluaxjitSymbol->s_thing)
                    pd_typedmess(_this->pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);

                // Recreate this object
                if (auto patch = _this->cnv->patch.getPointer()) {
                    pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                }
                _this->cnv->synchronise();
            }
        });
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (currentTouchIndex > 0)
            return; // already has a touch
        currentTouchIndex = e.source.getIndex();

        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_down(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.source.getIndex() != currentTouchIndex)
            return;

        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_drag(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseMove(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_move(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseUp(MouseEvent const& e) override
    {
        currentTouchIndex = -1;

        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_up(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseEnter(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_enter(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseExit(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_mouse_exit(pdlua, x, y);
            sys_unlock();
        });
    }

    void sendRepaintMessage()
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [](t_pdlua* pdlua) {
            sys_lock();
            pdlua->gfx.pdlua_gfx_repaint(pdlua, 0);
            sys_unlock();
        });
    }

    void resized() override
    {
        sendRepaintMessage();
    }

    void lookAndFeelChanged() override
    {
        sendRepaintMessage();
    }

    void render(NVGcontext* nvg) override
    {
        NVGScopedState scopedState(nvg);

        auto scale = nvgCurrentPixelScale(nvg) * getValue<float>(zoomScale);
        nvgScale(nvg, 1.0f / scale, 1.0f / scale);
        nvgTransformQuantize(nvg);

        for (auto& [layer, fb] : framebuffers) {
            fb.render(nvg, Rectangle<int>(std::ceil(getWidth() * scale), std::ceil(getHeight() * scale)));
        }
    }

    void valueChanged(Value& v) override
    {
        sendRepaintMessage();
    }

    // Drawing svg with nanovg + nanosvg borrowed from: https://github.com/VCVRack/Rack/blob/v2/src/window/Svg.cpp
    static void drawSVG(NVGcontext* nvg, char const* svgText)
    {
        std::string svgCopy(svgText);
        auto* svg = nsvgParse(svgCopy.data(), "px", 96);
        if (!svg)
            return;

        auto getNVGColor = [](uint32_t color) -> NVGcolor {
            return nvgRGBA(
                (color >> 0) & 0xff,
                (color >> 8) & 0xff,
                (color >> 16) & 0xff,
                (color >> 24) & 0xff);
        };

        auto getPaint = [&getNVGColor](NVGcontext* nvg, NSVGpaint* p) -> NVGpaint {
            assert(p->type == NSVG_PAINT_LINEAR_GRADIENT || p->type == NSVG_PAINT_RADIAL_GRADIENT);
            NSVGgradient* g = p->gradient;
            assert(g->nstops >= 1);
            NVGcolor icol = getNVGColor(g->stops[0].color);
            NVGcolor ocol = getNVGColor(g->stops[g->nstops - 1].color);

            float inverse[6];
            nvgTransformInverse(inverse, g->xform);

            Point<float> s, e;
            // Is it always the case that the gradient should be transformed from (0, 0) to (0, 1)?
            nvgTransformPoint(&s.x, &s.y, inverse, 0, 0);
            nvgTransformPoint(&e.x, &e.y, inverse, 0, 1);

            NVGpaint paint;
            if (p->type == NSVG_PAINT_LINEAR_GRADIENT)
                paint = nvgLinearGradient(nvg, s.x, s.y, e.x, e.y, icol, ocol);
            else
                paint = nvgRadialGradient(nvg, s.x, s.y, 0.0, 160, icol, ocol);
            return paint;
        };

        int shapeIndex = 0;
        // Iterate shape linked list
        for (NSVGshape* shape = svg->shapes; shape; shape = shape->next, shapeIndex++) {
            // Visibility
            if (!(shape->flags & NSVG_FLAGS_VISIBLE))
                continue;

            nvgSave(nvg);

            // Opacity
            if (shape->opacity < 1.0)
                nvgGlobalAlpha(nvg, shape->opacity);

            // Build path
            nvgBeginPath(nvg);
            nvgPathWinding(nvg, NVG_NONZERO);

            // Iterate path linked list
            for (NSVGpath* path = shape->paths; path; path = path->next) {
                nvgMoveTo(nvg, path->pts[0], path->pts[1]);
                for (int i = 1; i < path->npts; i += 3) {
                    float* p = &path->pts[2 * i];
                    nvgBezierTo(nvg, p[0], p[1], p[2], p[3], p[4], p[5]);
                }

                // Close path
                if (path->closed)
                    nvgClosePath(nvg);
            }

            // Fill shape
            if (shape->fill.type) {
                switch (shape->fill.type) {
                case NSVG_PAINT_COLOR: {
                    nvgFillColor(nvg, getNVGColor(shape->fill.color));
                } break;
                case NSVG_PAINT_LINEAR_GRADIENT:
                case NSVG_PAINT_RADIAL_GRADIENT: {
                    nvgFillPaint(nvg, getPaint(nvg, &shape->fill));
                } break;
                }
                nvgFill(nvg);
            }

            // Stroke shape
            if (shape->stroke.type) {
                nvgStrokeWidth(nvg, shape->strokeWidth);
                // strokeDashOffset, strokeDashArray, strokeDashCount not yet supported
                nvgLineCap(nvg, (NVGlineCap)shape->strokeLineCap);
                nvgLineJoin(nvg, (int)shape->strokeLineJoin);

                switch (shape->stroke.type) {
                case NSVG_PAINT_COLOR: {
                    nvgStrokeColor(nvg, getNVGColor(shape->stroke.color));
                } break;
                case NSVG_PAINT_LINEAR_GRADIENT: {
                    nvgStrokePaint(nvg, getPaint(nvg, &shape->stroke));
                } break;
                }
                nvgStroke(nvg);
            }

            nvgRestore(nvg);
        }
    }

    void handleGuiMessage(NVGcontext* nvg, int const layer, t_symbol* sym, int const argc, t_atom* argv)
    {
        auto const hashsym = hash(sym->s_name);
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch (hashsym) {
        case hash("lua_start_paint"): {
            if (getLocalBounds().isEmpty())
                break;

            auto const pixelScale = nvgCurrentPixelScale(nvg);
            auto const zoom = getValue<float>(zoomScale);
            auto const imageScale = zoom * pixelScale;
            int const imageWidth = std::ceil(getWidth() * imageScale);
            int const imageHeight = std::ceil(getHeight() * imageScale);
            if (!imageWidth || !imageHeight)
                return;

            framebuffers[layer].bind(nvg, imageWidth, imageHeight);

            nvgViewport(0, 0, imageWidth, imageHeight);
            nvgClear(nvg);
            nvgBeginFrame(nvg, getWidth() * zoom, getHeight() * zoom, pixelScale);
            nvgScale(nvg, zoom, zoom);
            nvgSave(nvg);
            return;
        }
        case hash("lua_end_paint"): {
            if (!framebuffers[layer].isValid())
                return;

            auto const pixelScale = nvgCurrentPixelScale(nvg);
            auto const scale = getValue<float>(zoomScale) * pixelScale;
            nvgGlobalScissor(nvg, 0, 0, getWidth() * scale, getHeight() * scale);
            nvgEndFrame(nvg);
            framebuffers[layer].unbind();
            repaint();
            return;
        }
        default:
            break;
        }

        switch (hashsym) {
        case hash("lua_set_color"): {
            if (argc == 1) {
                int const colourID = std::min<int>(atom_getfloat(argv), 2);

                currentColour = StackArray<Colour, 3> { PlugDataColours::guiObjectBackgroundColour,
                    PlugDataColours::canvasTextColour,
                    PlugDataColours::guiObjectInternalOutlineColour }[colourID];
                nvgFillColor(nvg, nvgColour(currentColour));
                nvgStrokeColor(nvg, nvgColour(currentColour));
            }
            if (argc >= 3) {
                Colour const color(static_cast<uint8>(atom_getfloat(argv)),
                    static_cast<uint8>(atom_getfloat(argv + 1)),
                    static_cast<uint8>(atom_getfloat(argv + 2)));

                currentColour = color.withAlpha(argc >= 4 ? atom_getfloat(argv + 3) : 1.0f);
                nvgFillColor(nvg, nvgColour(currentColour));
                nvgStrokeColor(nvg, nvgColour(currentColour));
            }
            break;
        }
        case hash("lua_stroke_line"): {
            if (argc >= 4) {
                float const x1 = atom_getfloat(argv);
                float const y1 = atom_getfloat(argv + 1);
                float const x2 = atom_getfloat(argv + 2);
                float const y2 = atom_getfloat(argv + 3);
                float const lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x1, y1);
                nvgLineTo(nvg, x2, y2);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_fill_ellipse"): {
            if (argc >= 3) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);

                nvgBeginPath(nvg);
                nvgEllipse(nvg, x + w / 2, y + h / 2, w / 2, h / 2);
                nvgFill(nvg);
            }
            break;
        }
        case hash("lua_stroke_ellipse"): {
            if (argc >= 4) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);
                float const lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgEllipse(nvg, x + w / 2, y + h / 2, w / 2, h / 2);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_fill_rect"): {
            if (argc >= 4) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);

                nvgFillRect(nvg, x, y, w, h);
            }
            break;
        }
        case hash("lua_stroke_rect"): {
            if (argc >= 5) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);
                float const lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgStrokeRect(nvg, x, y, w, h);
            }
            break;
        }
        case hash("lua_fill_rounded_rect"): {
            if (argc >= 4) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);
                float const cornerRadius = atom_getfloat(argv + 4);

                nvgFillRoundedRect(nvg, x, y, w, h, cornerRadius);
            }
            break;
        }

        case hash("lua_stroke_rounded_rect"): {
            if (argc >= 6) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);
                float const cornerRadius = atom_getfloat(argv + 4);
                float const lineThickness = atom_getfloat(argv + 5);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgRoundedRect(nvg, x, y, w, h, cornerRadius);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_draw_line"): {
            if (argc >= 4) {
                float const x1 = atom_getfloat(argv);
                float const y1 = atom_getfloat(argv + 1);
                float const x2 = atom_getfloat(argv + 2);
                float const y2 = atom_getfloat(argv + 3);
                float const lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, x1, y1);
                nvgLineTo(nvg, x2, y2);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_draw_text"): {
            if (argc >= 4) {
                float const x = atom_getfloat(argv + 1);
                float const y = atom_getfloat(argv + 2);
                float const w = atom_getfloat(argv + 3);
                float const fontHeight = atom_getfloat(argv + 4);
                int const alignment = atom_getfloat(argv + 5);

                nvgFontSize(nvg, fontHeight);
                nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

                float bounds[4];
                nvgTextBoxBounds(nvg, 0, 0, w, atom_getsymbol(argv)->s_name, nullptr, bounds);
                float textW = bounds[2] - bounds[0]; // actual rendered width
                float textH = bounds[3] - bounds[1]; // actual rendered height (better than fontHeight)

                float ax = x, ay = y;

                switch (alignment) {
                case 1:
                case 4:
                case 7:
                    ax = x - textW / 2.0f;
                    break; // CENTER_X
                case 2:
                case 5:
                case 8:
                    ax = x - textW;
                    break; // RIGHT
                default:
                    break; // LEFT
                }

                switch (alignment) {
                case 3:
                case 4:
                case 5:
                    ay = y - textH / 2.0f;
                    break; // MIDDLE
                case 6:
                case 7:
                case 8:
                    ay = y - textH;
                    break; // BOTTOM
                default:
                    break; // TOP
                }

                nvgBeginPath(nvg);
                nvgTextBox(nvg, ax, ay, w, atom_getsymbol(argv)->s_name, nullptr);
            }
            break;
        }
        case hash("lua_fill_path"): {
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, atom_getfloat(argv), atom_getfloat(argv + 1));
            for (int i = 1; i < argc / 2; i++) {
                float const x = atom_getfloat(argv + i * 2);
                float const y = atom_getfloat(argv + i * 2 + 1);
                nvgLineTo(nvg, x, y);
            }

            nvgClosePath(nvg);
            nvgFill(nvg);
            break;
        }
        case hash("lua_stroke_path"): {
            nvgBeginPath(nvg);
            auto const strokeWidth = atom_getfloat(argv);

            int const numPoints = (argc - 1) / 2;
            nvgMoveTo(nvg, atom_getfloat(argv + 1), atom_getfloat(argv + 2));
            for (int i = 1; i < numPoints; i++) {
                float const x = atom_getfloat(argv + i * 2 + 1);
                float const y = atom_getfloat(argv + i * 2 + 2);
                nvgLineTo(nvg, x, y);
            }

            nvgStrokeWidth(nvg, strokeWidth);
            nvgStroke(nvg);
            break;
        }
        case hash("lua_fill_all"): {
            auto const bounds = getLocalBounds();
            auto const outlineColour = nvgColour(isSelected ? PlugDataColours::objectSelectedOutlineColour : PlugDataColours::objectOutlineColour);

            nvgDrawRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), nvgColour(currentColour), outlineColour, Corners::objectCornerRadius);

            // Make sure subsequent draw calls cannot render over the outline
            nvgIntersectRoundedScissor(nvg, bounds.getX() + 0.75f, bounds.getY() + 0.75f, bounds.getWidth() - 1.5f, bounds.getHeight() - 1.5f, Corners::objectCornerRadius);
            break;
        }
        case hash("lua_draw_svg"): {
            if (argc >= 3) {
                float const x = atom_getfloat(argv + 1);
                float const y = atom_getfloat(argv + 2);
                nvgSave(nvg);
                nvgTranslate(nvg, x, y);
                drawSVG(nvg, atom_getsymbol(argv)->s_name);
                nvgRestore(nvg);
            }
            break;
        }
        case hash("lua_draw_image"): {
            if (argc >= 3) {
                auto* path = atom_getsymbol(argv)->s_name;
                auto pathHash = hash(path);
                if (!images.contains(pathHash) || !images.at(pathHash).first.isValid()) {
                    auto findFile = [this](String const& name) {
                        if (auto patch = cnv->patch.getPointer()) {
                            if ((name.startsWith("/") || name.startsWith("./") || name.startsWith("../")) && File(name).existsAsFile()) {
                                return File(name);
                            }

                            char dir[MAXPDSTRING];
                            char* file;

                            int const fd = canvas_open(patch.get(), name.toRawUTF8(), "", dir, &file, MAXPDSTRING, 0);
                            if (fd >= 0) {
                                sys_close(fd);
                                return File(dir).getChildFile(file);
                            }
                        }
                        return File(name);
                    };

                    auto file = findFile(String::fromUTF8(path));
                    if (file.existsAsFile()) {
                        auto image = ImageFileFormat::loadFrom(file);
                        images[pathHash].first.loadJUCEImage(nvg, image);
                        images[pathHash].second = image.getBounds();
                    } else {
                        return;
                    }
                }

                float const x = atom_getfloat(argv + 1);
                float const y = atom_getfloat(argv + 2);
                nvgSave(nvg);
                nvgTranslate(nvg, x, y);
                images.at(pathHash).first.render(nvg, images[pathHash].second);
                nvgRestore(nvg);
            }
            break;
        }
        case hash("lua_translate"): {
            if (argc >= 2) {
                float const tx = atom_getfloat(argv);
                float const ty = atom_getfloat(argv + 1);
                nvgTranslate(nvg, tx, ty);
            }
            break;
        }
        case hash("lua_scale"): {
            if (argc >= 2) {
                float const sx = atom_getfloat(argv);
                float const sy = atom_getfloat(argv + 1);
                nvgScale(nvg, sx, sy);
            }
            break;
        }
        case hash("lua_reset_transform"): {
            nvgRestore(nvg);
            nvgSave(nvg);
            break;
        }
        default:
            break;
        }
    }

    // We need to update the framebuffer in a place where the current graphics context is active (for multi-window support, thanks to Alex for figuring that out)
    // but we also need to be outside of calls to beginFrame/endFrame
    // So we have this separate callback function that occurs after activating the GPU context, but before starting the frame
    void updateFramebuffers(NVGcontext* nvg) override
    {
        frameSwapLock.enter();
        auto frames = currentFrame;
        currentFrame.clear();
        frameSwapLock.exit();

        for (auto& [layer, layerMessages] : frames) {
            for (auto& guiMessage : layerMessages) {
                handleGuiMessage(nvg, layer, guiMessage.symbol, guiMessage.size, guiMessage.data.data());
            }
            if (!framebuffers[layer].isValid())
                sendRepaintMessage();
        }

        if (isSelected != object->isSelected()) {
            isSelected = object->isSelected();
            sendRepaintMessage();
        }
    }

    ObjectParameters getParameters() override
    {
        objectParameters.clear();

        if (auto pdlua = ptr.get<t_pdlua>()) {
            if (pdlua->pdlua_class_gfx && pdlua->pdlua_class_gfx->c_propertiesfn) {
                pdlua->pdlua_class_gfx->c_propertiesfn(pdlua.cast<t_gobj>(), nullptr);
            }
        }

        return objectParameters;
    }

    static void drawCallback(void* target, int const layer, t_symbol* sym, int argc, t_atom* argv)
    {
        for (auto* object : allDrawTargets[static_cast<t_pdlua*>(target)]) {
            if (sym == gensym("lua_resized")) {
                if (argc >= 2) {
                    if (auto pdlua = object->ptr.get<t_pdlua>()) {
                        pdlua->gfx.width = atom_getfloat(argv);
                        pdlua->gfx.height = atom_getfloat(argv + 1);
                    }

                    MessageManager::callAsync([_object = SafePointer(object->object)] {
                        if (_object)
                            _object->updateBounds();
                    });
                }
                return;
            }
            object->guiCommandBuffer[layer].add({ sym, argc, argv });
            if (sym == gensym("lua_end_paint")) {
                object->frameSwapLock.enter();
                object->currentFrame[layer] = object->guiCommandBuffer[layer];
                object->frameSwapLock.exit();
                object->guiCommandBuffer[layer].clear();
            }
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        if (symbol == hash("open_textfile") && atoms.size() >= 1) {
            openTextEditor(File(atoms[0].toString()));
        }
    }

    void openTextEditor(File fileToOpen)
    {
        if (fileToOpen.getFileName() == "pd.lua") {
            return;
        }

        if (textEditor) {
            textEditor->toFront(true);
            return;
        }

        if (cnv->editor->openTextEditors.contains(ptr))
            return;

        auto onClose = [_this = SafePointer(this), this, fileToOpen](String const& newText, bool const hasChanged) {
            if (!_this)
                return;

            if (!hasChanged) {
                cnv->editor->openTextEditors.remove_all(ptr);
                textEditor.reset(nullptr);
                return;
            }

            Dialogs::showAskToSaveDialog(
                &saveDialog, textEditor.get(), "", [_this = SafePointer(this), newText, fileToOpen](int const result) mutable {
                    if (!_this)
                        return;
                    if (result == 2) {
                        fileToOpen.replaceWithText(newText);
                        if (auto pdlua = _this->ptr.get<t_pd>()) {
                            if (_this->pdluaxSymbol->s_thing)
                                pd_typedmess(_this->pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                            if (_this->pdluaxjitSymbol->s_thing)
                                pd_typedmess(_this->pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);
                            // Recreate this object
                            if (auto patch = _this->cnv->patch.getPointer()) {
                                pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                            }
                        }
                        _this->cnv->editor->openTextEditors.remove_all(_this->ptr);
                        _this->textEditor.reset(nullptr);
                        _this->cnv->synchronise();
                    }
                    if (result == 1) {
                        _this->cnv->editor->openTextEditors.remove_all(_this->ptr);
                        _this->textEditor.reset(nullptr);
                    }
                },
                15, false);
        };

        auto onSave = [_this = SafePointer(this), this, fileToOpen](String const& newText) {
            if (!_this)
                return;

            fileToOpen.replaceWithText(newText);
            if (auto pdlua = ptr.get<t_pd>()) {
                if (pdluaxSymbol->s_thing)
                    pd_typedmess(pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                if (pdluaxjitSymbol->s_thing)
                    pd_typedmess(pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);
            }
            sendRepaintMessage();
        };

        auto const scaleFactor = getApproximateScaleFactorForComponent(cnv->editor);
        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), onClose, onSave, scaleFactor, true));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }
};

// A non-GUI Lua object, that we would still like to have clickable for opening the editor
class LuaTextObject final : public TextObjectBase {
public:
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;
    t_symbol* pdluaxSymbol;
    t_symbol* pdluaxjitSymbol;
    LuaPropertiesPanel properties;

    LuaTextObject(pd::WeakReference ptr, Object* object)
        : TextObjectBase(ptr, object)
        , properties(ptr.getRaw<void>(), this)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            pdlua->properties.plugdata_properties_callback = &LuaPropertiesPanel::propertiesCallback;

            libpd_set_instance(&pd_maininstance);
            pdluaxSymbol = gensym("pdluax");
            pdluaxjitSymbol = gensym("pdluaxjit");
            pd->setThis();
        }
    }

    ObjectParameters getParameters() override
    {
        objectParameters.clear();

        if (auto pdlua = ptr.get<t_pdlua>()) {
            if (pdlua->pdlua_class && pdlua->pdlua_class->c_propertiesfn) {
                pdlua->pdlua_class->c_propertiesfn(pdlua.cast<t_gobj>(), nullptr);
            }
        }

        return objectParameters;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (getValue<bool>(object->locked)) {
            auto objectText = getText();
            if (objectText != "pdlua" && objectText != "pdluax") {
                sendMessage("menu-open");
            }
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        if (symbol == hash("open_textfile") && atoms.size() >= 1) {
            openTextEditor(File(atoms[0].toString()));
        }
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        auto objectText = getText();
        if (objectText != "pdlua" && objectText != "pdluax") {
            menu.addItem("Open lua editor", [_this = SafePointer(this)] {
                if (!_this)
                    return;
                _this->sendMessage("menu-open");
            });

            menu.addItem("Reload lua object", [_this = SafePointer(this)] {
                if (!_this)
                    return;
                if (auto pdlua = _this->ptr.get<t_pd>()) {
                    // Reload the lua script
                    if (_this->pdluaxSymbol->s_thing)
                        pd_typedmess(_this->pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                    if (_this->pdluaxjitSymbol->s_thing)
                        pd_typedmess(_this->pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);

                    // Recreate this object
                    if (auto patch = _this->cnv->patch.getPointer()) {
                        pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                    }
                }
            });
        }
    }

    void openTextEditor(File fileToOpen)
    {
        if (fileToOpen == ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("pdlua").getChildFile("pd.lua")) {
            return;
        }

        if (textEditor) {
            textEditor->toFront(true);
            return;
        }

        if (cnv->editor->openTextEditors.contains(ptr))
            return;

        auto onClose = [_this = SafePointer(this), this, fileToOpen](String const& newText, bool const hasChanged) {
            if (!_this)
                return;
            if (!hasChanged) {
                cnv->editor->openTextEditors.remove_all(ptr);
                textEditor.reset(nullptr);
                return;
            }

            Dialogs::showAskToSaveDialog(
                &saveDialog, textEditor.get(), "", [this, newText, fileToOpen](int const result) mutable {
                    if (result == 2) {
                        fileToOpen.replaceWithText(newText);
                        if (auto pdlua = ptr.get<t_pd>()) {
                            if (pdluaxSymbol->s_thing)
                                pd_typedmess(pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                            if (pdluaxjitSymbol->s_thing)
                                pd_typedmess(pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);
                            // Recreate this object
                            if (auto patch = cnv->patch.getPointer()) {
                                pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                            }
                        }
                        cnv->editor->openTextEditors.remove_all(ptr);
                        textEditor.reset(nullptr);
                        cnv->synchronise();
                    }
                    if (result == 1) {
                        cnv->editor->openTextEditors.remove_all(ptr);
                        textEditor.reset(nullptr);
                    }
                },
                15, false);
        };

        auto onSave = [_this = SafePointer(this), this, fileToOpen](String const& newText) {
            if (!_this)
                return;
            fileToOpen.replaceWithText(newText);
            if (auto pdlua = ptr.get<t_pd>()) {
                if (pdluaxSymbol->s_thing) {
                    if (pdluaxSymbol->s_thing)
                        pd_typedmess(pdluaxSymbol->s_thing, gensym("reload"), 0, nullptr);
                    if (pdluaxjitSymbol->s_thing)
                        pd_typedmess(pdluaxjitSymbol->s_thing, gensym("reload"), 0, nullptr);
                }
            }
        };

        auto const scaleFactor = getApproximateScaleFactorForComponent(cnv->editor);
        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), onClose, onSave, scaleFactor, true));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }
};
