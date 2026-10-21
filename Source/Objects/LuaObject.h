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

// Turns the properties a Lua object declares in its properties() method into regular object parameters,
// and sends inspector changes back to the Lua method that belongs to each property
class LuaProperties {
    struct Property {
        std::unique_ptr<Value> value;
        ParameterType type;
        String method;
    };

    ObjectBase* object;
    Value::Listener* listener;
    void* target = nullptr;
    ObjectParameters* parameters = nullptr;

    // Values are reused across loads, so the inspector's pointers to them stay valid
    HeapArray<Property> properties;
    size_t numLoaded = 0;
    ParameterCategory currentCategory;
    int numFrames = 0;

    // The properties that are currently running their object's Lua properties method, only set during load()
    static inline LuaProperties* active = nullptr;

public:
    LuaProperties(ObjectBase* object, Value::Listener* listener)
        : object(object)
        , listener(listener)
    {
    }

    // Runs the object's Lua properties method, which adds its properties to params through propertiesCallback
    void load(t_pdlua* pdlua, ObjectParameters& params)
    {
        auto const propertiesFn = pdlua->pd.te_pd->c_propertiesfn;
        if (!propertiesFn)
            return;

        ScopedValueSetter<LuaProperties*> const activeSetter(active, this);
        pdlua->properties.plugdata_properties_callback = &propertiesCallback;
        target = pdlua;
        numLoaded = 0;
        numFrames = 0;
        parameters = &params;
        currentCategory = cGeneral;
        propertiesFn(&pdlua->pd.te_g, nullptr);
        parameters = nullptr;
    }

    void propertyChanged(Value& v)
    {
        for (size_t i = 0; i < numLoaded; i++) {
            auto const& [value, type, method] = properties[i];
            if (!value->refersToSameSourceAs(v))
                continue;

            auto* instance = object->pd;
            auto* methodSym = instance->generateSymbol(method);
            if (type == tColour) {
                auto* hex = instance->generateSymbol("#" + getValue<Colour>(v).toString().substring(2));
                object->sendMessage("dialog", { instance->generateSymbol("colorpicker"), methodSym, hex });
            } else if (type == tString) {
                object->sendMessage("dialog", { instance->generateSymbol("text"), methodSym, instance->generateSymbol(v.toString()) });
            } else // combo/number/check are all just float
            {
                object->sendMessage("dialog", { instance->generateSymbol("number"), methodSym, getValue<float>(v) });
            }
            return;
        }
    }

private:
    Value* nextValue(ParameterType const type, String const& method, var const& init)
    {
        if (numLoaded == properties.size()) {
            properties.emplace_back(Property { std::make_unique<Value>(SynchronousValue()) });
            properties[numLoaded].value->addListener(listener);
        }

        auto& [value, propertyType, propertyMethod] = properties[numLoaded++];
        propertyType = type;
        propertyMethod = method;
        setValueExcludingListener(*value, init, listener);
        return value.get();
    }

    static void propertiesCallback(void* pdlua, t_symbol* sym, int const argc, t_atom* argv)
    {
        auto* self = active;
        if (!self || self->target != pdlua)
            return;

        auto const symbol = hash(sym->s_name);
        auto const atoms = pd::Atom::fromAtoms(argc, argv);
        auto& category = self->currentCategory;

        if (symbol == hash("add_frame_property") && atoms.size() >= 1) {
            auto const name = atoms[0].toString();
            category = name == cDimensions.name ? cDimensions : ParameterCategory { .name = name, .order = cGeneral.order + ++self->numFrames };
            return;
        }
        if (atoms.size() < 3)
            return;

        auto& parameters = *self->parameters;
        auto const label = atoms[0].toString();
        auto const method = atoms[1].toString();

        if (symbol == hash("add_check_property")) {
            auto const init = var(atoms[2].getFloat() != 0.0f);
            parameters.addParamBool(label, category, self->nextValue(tBool, method, init), { "No", "Yes" }, init);
        } else if (symbol == hash("add_text_property")) {
            auto const init = var(atoms[2].toString());
            parameters.addParamString(label, category, self->nextValue(tString, method, init), init);
        } else if (symbol == hash("add_color_property")) {
            // A string default is a literal colour, instead of a theme colour ID
            auto const init = Colour::fromString("ff" + atoms[2].toString().fromFirstOccurrenceOf("#", false, false));
            parameters.addParamColour(label, category, self->nextValue(tColour, method, colourToVar(init)), init.toString());
        } else if (symbol == hash("add_int_property") && atoms.size() >= 5) {
            // Lua's default range is +-1e36, which doesn't fit in an int
            auto const toInt = [](float const f) { return static_cast<int>(jlimit<double>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), f)); };
            auto const init = var(static_cast<int>(atoms[2].getFloat()));
            parameters.addParamInt(label, category, self->nextValue(tInt, method, init), init, true, toInt(atoms[3].getFloat()), toInt(atoms[4].getFloat()));
        } else if (symbol == hash("add_float_property") && atoms.size() >= 5) {
            auto const init = var(atoms[2].getFloat());
            parameters.addParamFloat(label, category, self->nextValue(tFloat, method, init), init, true, atoms[3].getFloat(), atoms[4].getFloat());
        } else if (symbol == hash("add_combo_property") && atoms.size() >= 4) {
            StringArray options;
            for (int i = 3; i < atoms.size(); i++) {
                options.add(atoms[i].toString());
            }
            auto const init = var(static_cast<int>(atoms[2].getFloat()));
            parameters.addParamCombo(label, category, self->nextValue(tCombo, method, init), options, init);
        }
    }
};

class LuaObject final : public ObjectBase
    , private Value::Listener, private AsyncUpdater {
    Colour currentColour;

    t_symbol* pdluaxSymbol;
    t_symbol* pdluaxjitSymbol;

    bool isSelected = false;
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

    LuaProperties properties;

    int currentTouchIndex = -1;

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
        , properties(this, &propertyListener)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            pdlua->gfx.plugdata_draw_callback = &drawCallback;
            allDrawTargets[pdlua.get()].add(this);

            libpd_set_instance(&pd_maininstance);
            pdluaxSymbol = gensym("pdluax");
            pdluaxjitSymbol = gensym("pdluaxjit");
            pd->setThis();
        }

        parentHierarchyChanged();
    }

    ~LuaObject() override
    {
        pd->lockAudioThread();
        auto& listeners = allDrawTargets[ptr.getRawUnchecked<t_pdlua>()];
        listeners.erase(std::ranges::remove(listeners, this).begin(), listeners.end());
        pd->unlockAudioThread();

        zoomScale.removeListener(this);
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

    void handleAsyncUpdate() override
    {
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        NVGScopedState scopedState(nvg);

        frameSwapLock.enter();
        auto frames = currentFrame;
        frameSwapLock.exit();


        auto b = getLocalBounds();
        nanovg::nvgIntersectRoundedScissor(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), getPlugDataLook(*this).getObjectCornerRadius());

        for (auto& [layer, layerMessages] : frames) {
            for (auto& guiMessage : layerMessages) {
                handleGuiMessage(nvg, layer, guiMessage.symbol, guiMessage.size, guiMessage.data.data());
            }
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
            return nanovg::nvgRGBA(
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
            nanovg::nvgTransformInverse(inverse, g->xform);

            Point<float> s, e;
            // Is it always the case that the gradient should be transformed from (0, 0) to (0, 1)?
            nanovg::nvgTransformPoint(&s.x, &s.y, inverse, 0, 0);
            nanovg::nvgTransformPoint(&e.x, &e.y, inverse, 0, 1);

            NVGpaint paint;
            if (p->type == NSVG_PAINT_LINEAR_GRADIENT)
                paint = nanovg::nvgLinearGradient(nvg, s.x, s.y, e.x, e.y, icol, ocol);
            else
                paint = nanovg::nvgRadialGradient(nvg, s.x, s.y, 0.0, 160, icol, ocol);
            return paint;
        };

        int shapeIndex = 0;
        // Iterate shape linked list
        for (NSVGshape* shape = svg->shapes; shape; shape = shape->next, shapeIndex++) {
            // Visibility
            if (!(shape->flags & NSVG_FLAGS_VISIBLE))
                continue;

            nanovg::nvgSave(nvg);

            // Opacity
            if (shape->opacity < 1.0)
                nanovg::nvgGlobalAlpha(nvg, shape->opacity);

            // Build path
            nanovg::nvgBeginPath(nvg);

            // Iterate path linked list
            for (NSVGpath* path = shape->paths; path; path = path->next) {
                nanovg::nvgMoveTo(nvg, path->pts[0], path->pts[1]);
                nanovg::nvgPathWinding(nvg, NVG_NONZERO);
                for (int i = 1; i < path->npts; i += 3) {
                    float* p = &path->pts[2 * i];
                    nanovg::nvgBezierTo(nvg, p[0], p[1], p[2], p[3], p[4], p[5]);
                }

                // Close path
                if (path->closed)
                    nanovg::nvgClosePath(nvg);
            }

            // Fill shape
            if (shape->fill.type) {
                switch (shape->fill.type) {
                case NSVG_PAINT_COLOR: {
                    nanovg::nvgFillColor(nvg, getNVGColor(shape->fill.color));
                } break;
                case NSVG_PAINT_LINEAR_GRADIENT:
                case NSVG_PAINT_RADIAL_GRADIENT: {
                    nanovg::nvgFillPaint(nvg, getPaint(nvg, &shape->fill));
                } break;
                }
                nanovg::nvgFill(nvg);
            }

            // Stroke shape
            if (shape->stroke.type) {
                nanovg::nvgStrokeWidth(nvg, shape->strokeWidth);
                // strokeDashOffset, strokeDashArray, strokeDashCount not yet supported
                nanovg::nvgLineCap(nvg, (NVGlineCap)shape->strokeLineCap);
                nanovg::nvgLineJoin(nvg, (int)shape->strokeLineJoin);

                switch (shape->stroke.type) {
                case NSVG_PAINT_COLOR: {
                    nanovg::nvgStrokeColor(nvg, getNVGColor(shape->stroke.color));
                } break;
                case NSVG_PAINT_LINEAR_GRADIENT: {
                    nanovg::nvgStrokePaint(nvg, getPaint(nvg, &shape->stroke));
                } break;
                }
                nanovg::nvgStroke(nvg);
            }

            nanovg::nvgRestore(nvg);
        }
    }

    void handleGuiMessage(NVGcontext* nvg, int const layer, t_symbol* sym, int const argc, t_atom* argv)
    {
        auto const& colours = getThemeColours();

        auto const hashsym = hash(sym->s_name);
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch (hashsym) {
        case hash("lua_start_paint"): {
            if (getLocalBounds().isEmpty())
                break;

            nanovg::nvgSave(nvg);

            return;
        }
        case hash("lua_end_paint"): {
            nanovg::nvgRestore(nvg);
            return;
        }
        default:
            break;
        }

        switch (hashsym) {
        case hash("lua_set_color"): {
            if (argc == 1) {
                int const colourID = std::min<int>(atom_getfloat(argv), 2);

                currentColour = StackArray<Colour, 3> { colours.guiObjectBackgroundColour,
                    colours.canvasTextColour,
                    colours.guiObjectInternalOutlineColour }[colourID];
                nanovg::nvgFillColor(nvg, nvgColour(currentColour));
                nanovg::nvgStrokeColor(nvg, nvgColour(currentColour));
            }
            if (argc >= 3) {
                Colour const color(static_cast<uint8>(atom_getfloat(argv)),
                    static_cast<uint8>(atom_getfloat(argv + 1)),
                    static_cast<uint8>(atom_getfloat(argv + 2)));

                currentColour = color.withAlpha(argc >= 4 ? atom_getfloat(argv + 3) : 1.0f);
                nanovg::nvgFillColor(nvg, nvgColour(currentColour));
                nanovg::nvgStrokeColor(nvg, nvgColour(currentColour));
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

                nanovg::nvgStrokeWidth(nvg, lineThickness);
                nanovg::nvgBeginPath(nvg);
                nanovg::nvgMoveTo(nvg, x1, y1);
                nanovg::nvgLineTo(nvg, x2, y2);
                nanovg::nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_fill_ellipse"): {
            if (argc >= 3) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);

                nanovg::nvgBeginPath(nvg);
                nanovg::nvgEllipse(nvg, x + w / 2, y + h / 2, w / 2, h / 2);
                nanovg::nvgFill(nvg);
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

                nanovg::nvgStrokeWidth(nvg, lineThickness);
                nanovg::nvgBeginPath(nvg);
                nanovg::nvgEllipse(nvg, x + w / 2, y + h / 2, w / 2, h / 2);
                nanovg::nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_fill_rect"): {
            if (argc >= 4) {
                float const x = atom_getfloat(argv);
                float const y = atom_getfloat(argv + 1);
                float const w = atom_getfloat(argv + 2);
                float const h = atom_getfloat(argv + 3);

                nanovg::nvgFillRect(nvg, x, y, w, h);
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

                nanovg::nvgStrokeWidth(nvg, lineThickness);
                nanovg::nvgStrokeRect(nvg, x, y, w, h);
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

                nanovg::nvgFillRoundedRect(nvg, x, y, w, h, cornerRadius);
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

                nanovg::nvgStrokeWidth(nvg, lineThickness);
                nanovg::nvgBeginPath(nvg);
                nanovg::nvgRoundedRect(nvg, x, y, w, h, cornerRadius);
                nanovg::nvgStroke(nvg);
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

                nanovg::nvgStrokeWidth(nvg, lineThickness);
                nanovg::nvgBeginPath(nvg);
                nanovg::nvgMoveTo(nvg, x1, y1);
                nanovg::nvgLineTo(nvg, x2, y2);
                nanovg::nvgStroke(nvg);
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
                auto const text = String::fromUTF8(atom_getsymbol(argv)->s_name);

                float ax = x, ay = y;

                AttributedString txt;
                txt.append(text, Fonts::getDefaultFont().withPointHeight(fontHeight), currentColour);

                TextLayout layout;
                layout.createLayout(txt, w);
                auto textW = layout.getWidth();
                auto textH = layout.getHeight();

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

                auto* llgc = cnv->editor->getNanoLLGC();
                Graphics g(*llgc);
                NVGGraphicsContext::ScopedAnchoredDraw anchor(*llgc, getLocalBounds().toFloat());
                layout.draw(g, Rectangle<float>(ax, ay, w, textH + 5));
            }
            break;
        }
        case hash("lua_fill_path"): {
            nanovg::nvgBeginPath(nvg);
            nanovg::nvgMoveTo(nvg, atom_getfloat(argv), atom_getfloat(argv + 1));
            for (int i = 1; i < argc / 2; i++) {
                float const x = atom_getfloat(argv + i * 2);
                float const y = atom_getfloat(argv + i * 2 + 1);
                nanovg::nvgLineTo(nvg, x, y);
            }

            nanovg::nvgClosePath(nvg);
            nanovg::nvgFill(nvg);
            break;
        }
        case hash("lua_stroke_path"): {
            nanovg::nvgBeginPath(nvg);
            auto const strokeWidth = atom_getfloat(argv);

            int const numPoints = (argc - 1) / 2;
            nanovg::nvgMoveTo(nvg, atom_getfloat(argv + 1), atom_getfloat(argv + 2));
            for (int i = 1; i < numPoints; i++) {
                float const x = atom_getfloat(argv + i * 2 + 1);
                float const y = atom_getfloat(argv + i * 2 + 2);
                nanovg::nvgLineTo(nvg, x, y);
            }

            nanovg::nvgStrokeWidth(nvg, strokeWidth);
            nanovg::nvgStroke(nvg);
            break;
        }
        case hash("lua_fill_all"): {
            auto const bounds = getLocalBounds();
            auto const outlineColour = nvgColour(isSelected ? colours.objectSelectedOutlineColour : colours.objectOutlineColour);

            nanovg::nvgDrawRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), nvgColour(currentColour), outlineColour, getPlugDataLook(*this).getObjectCornerRadius());

            // Make sure subsequent draw calls cannot render over the outline
            nanovg::nvgIntersectRoundedScissor(nvg, bounds.getX() + 0.75f, bounds.getY() + 0.75f, bounds.getWidth() - 1.5f, bounds.getHeight() - 1.5f, getPlugDataLook(*this).getObjectCornerRadius());
            break;
        }
        case hash("lua_draw_svg"): {
            if (argc >= 3) {
                float const x = atom_getfloat(argv + 1);
                float const y = atom_getfloat(argv + 2);
                nanovg::nvgSave(nvg);
                nanovg::nvgTranslate(nvg, x, y);
                drawSVG(nvg, atom_getsymbol(argv)->s_name);
                nanovg::nvgRestore(nvg);
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
                nanovg::nvgSave(nvg);
                nanovg::nvgTranslate(nvg, x, y);
                images.at(pathHash).first.render(nvg, images[pathHash].second);
                nanovg::nvgRestore(nvg);
            }
            break;
        }
        case hash("lua_translate"): {
            if (argc >= 2) {
                float const tx = atom_getfloat(argv);
                float const ty = atom_getfloat(argv + 1);
                nanovg::nvgTranslate(nvg, tx, ty);
            }
            break;
        }
        case hash("lua_scale"): {
            if (argc >= 2) {
                float const sx = atom_getfloat(argv);
                float const sy = atom_getfloat(argv + 1);
                nanovg::nvgScale(nvg, sx, sy);
            }
            break;
        }
        case hash("lua_reset_transform"): {
            nanovg::nvgRestore(nvg);
            nanovg::nvgSave(nvg);
            break;
        }
        default:
            break;
        }
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
                object->triggerAsyncUpdate();
            }
        }
    }

    ObjectParameters getParameters() override
    {
        auto parameters = ObjectBase::getParameters();
        if (auto pdlua = ptr.get<t_pdlua>())
            properties.load(pdlua.get(), parameters); // callback adds to this instead of object->objectParameters
        return parameters;
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
        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText().toString(), onClose, onSave, scaleFactor, true, fileToOpen));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }

    void propertyChanged(Value& v) override
    {
        properties.propertyChanged(v);
    }
};

// A non-GUI Lua object, that we would still like to have clickable for opening the editor
class LuaTextObject final : public TextObjectBase {
public:
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;
    t_symbol* pdluaxSymbol;
    t_symbol* pdluaxjitSymbol;
    LuaProperties properties;

    LuaTextObject(pd::WeakReference ptr, Object* object)
        : TextObjectBase(ptr, object)
        , properties(this, &propertyListener)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            libpd_set_instance(&pd_maininstance);
            pdluaxSymbol = gensym("pdluax");
            pdluaxjitSymbol = gensym("pdluaxjit");
            pd->setThis();
        }
    }

    ObjectParameters getParameters() override
    {
        auto parameters = ObjectBase::getParameters();
        if (auto pdlua = ptr.get<t_pdlua>())
            properties.load(pdlua.get(), parameters); // callback adds to this instead of object->objectParameters
        return parameters;
    }

    void propertyChanged(Value& v) override
    {
        TextObjectBase::propertyChanged(v);
        properties.propertyChanged(v);
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
        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText().toString(), onClose, onSave, scaleFactor, true, fileToOpen));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }
};
