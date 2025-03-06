/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

extern "C" {
#include <pd-lua/lua/lua.h>
#include <pd-lua/svg/nanosvg.h>

#define PLUGDATA 1
#include <pd-lua/pdlua.h>
#undef PLUGDATA

void pdlua_gfx_mouse_down(t_pdlua* o, int x, int y);
void pdlua_gfx_mouse_up(t_pdlua* o, int x, int y);
void pdlua_gfx_mouse_move(t_pdlua* o, int x, int y);
void pdlua_gfx_mouse_drag(t_pdlua* o, int x, int y);
void pdlua_gfx_repaint(t_pdlua* o, int firsttime);
}

class LuaObject final : public ObjectBase
    , private Value::Listener {
    Colour currentColour;

    bool isSelected = false;
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

    UnorderedSegmentedMap<int, NVGFramebuffer> framebuffers;

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

    UnorderedSegmentedMap<int, HeapArray<LuaGuiMessage>> guiCommandBuffer;
    UnorderedSegmentedMap<int, moodycamel::ReaderWriterQueue<LuaGuiMessage>> guiMessageQueue;

    static inline UnorderedMap<t_pdlua*, SmallArray<LuaObject*>> allDrawTargets = UnorderedMap<t_pdlua*, SmallArray<LuaObject*>>();

public:
    LuaObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            pdlua->gfx.plugdata_draw_callback = &drawCallback;
            allDrawTargets[pdlua.get()].add(this);
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

        while (auto const* nextCnv = topLevelCanvas->findParentComponentOfClass<Canvas>()) {
            topLevelCanvas = nextCnv;
        }
        if (topLevelCanvas) {
            zoomScale.referTo(topLevelCanvas->zoomScale);
            zoomScale.addListener(this);
            sendRepaintMessage();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_pdlua>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.cast<t_gobj>(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, gobj->gfx.width + 2, gobj->gfx.height + 2);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_pdlua>()) {
            auto* patch = object->cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, gobj.cast<t_gobj>(), b.getX(), b.getY());
            gobj->gfx.width = b.getWidth() - 2;
            gobj->gfx.height = b.getHeight() - 2;
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
            if (auto obj = _this->ptr.get<t_pd>()) {
                _this->pd->sendDirectMessage(obj.get(), "menu-open", {});
            }
        });
        menu.addItem("Reload lua object", [_this = SafePointer(this)] {
            if (!_this)
                return;
            if (auto pdlua = _this->ptr.get<t_pd>()) {
                // Reload the lua script
                _this->pd->sendMessage("pdluax", "reload", {});

                // Recreate this object
                if (auto patch = _this->cnv->patch.getPointer()) {
                    pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                }
                _this->cnv->synchronise();
            }
        });
    }

    bool hideInGraph() override
    {
        return false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua_gfx_mouse_down(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseDrag(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua_gfx_mouse_drag(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseMove(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua_gfx_mouse_move(pdlua, x, y);
            sys_unlock();
        });
    }

    void mouseUp(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            sys_lock();
            pdlua_gfx_mouse_up(pdlua, x, y);
            sys_unlock();
        });
    }

    void sendRepaintMessage()
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [](t_pdlua* pdlua) {
            sys_lock();
            pdlua_gfx_repaint(pdlua, 0);
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
        for (auto& [layer, fb] : framebuffers) {
            fb.render(nvg, Rectangle<int>(getWidth() + 1, getHeight()));
        }
    }

    void valueChanged(Value& v) override
    {
        sendRepaintMessage();
    }

    void handleGuiMessage(int const layer, t_symbol* sym, int const argc, t_atom* argv)
    {
        NVGcontext* nvg = cnv->editor->nvgSurface.getRawContext();
        if (!nvg)
            return;

        auto const hashsym = hash(sym->s_name);
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch (hashsym) {
        case hash("lua_start_paint"): {
            if (getLocalBounds().isEmpty())
                break;
            auto const scale = getValue<float>(zoomScale) * 2.0f; // Multiply by 2 for hi-dpi screens
            int const imageWidth = std::ceil(getWidth() * scale);
            int const imageHeight = std::ceil(getHeight() * scale);
            if (!imageWidth || !imageHeight)
                return;

            framebuffers[layer].bind(nvg, imageWidth, imageHeight);

            nvgViewport(0, 0, imageWidth, imageHeight);
            nvgClear(nvg);
            nvgBeginFrame(nvg, getWidth(), getHeight(), scale);
            nvgSave(nvg);
            return;
        }
        case hash("lua_end_paint"): {
            if (!framebuffers[layer].isValid())
                return;

            auto const scale = getValue<float>(zoomScale) * 2.0f; // Multiply by 2 for hi-dpi screens
            nvgGlobalScissor(nvg, 0, 0, getWidth() * scale, getHeight() * scale);
            nvgEndFrame(nvg);
            framebuffers[layer].unbind();
            repaint();
            return;
        }
        case hash("lua_resized"): {
            if (argc >= 2) {
                if (auto pdlua = ptr.get<t_pdlua>()) {
                    pdlua->gfx.width = atom_getfloat(argv);
                    pdlua->gfx.height = atom_getfloat(argv + 1);
                }
                MessageManager::callAsync([_object = SafePointer(object)] {
                    if (_object)
                        _object->updateBounds();
                });
            }
            return;
        }
        }

        switch (hashsym) {
        case hash("lua_set_color"): {
            if (argc == 1) {
                int const colourID = std::min<int>(atom_getfloat(argv), 2);

                currentColour = StackArray<Colour, 3> { cnv->guiObjectBackgroundColJuce, cnv->canvasTextColJuce, cnv->guiObjectInternalOutlineColJuce }[colourID];
                nvgFillColor(nvg, convertColour(currentColour));
                nvgStrokeColor(nvg, convertColour(currentColour));
            }
            if (argc >= 3) {
                Colour const color(static_cast<uint8>(atom_getfloat(argv)),
                    static_cast<uint8>(atom_getfloat(argv + 1)),
                    static_cast<uint8>(atom_getfloat(argv + 2)));

                currentColour = color.withAlpha(argc >= 4 ? atom_getfloat(argv + 3) : 1.0f);
                nvgFillColor(nvg, convertColour(currentColour));
                nvgStrokeColor(nvg, convertColour(currentColour));
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

                nvgBeginPath(nvg);
                nvgFontSize(nvg, fontHeight);
                nvgTextAlign(nvg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
                nvgTextBox(nvg, x, y, w, atom_getsymbol(argv)->s_name, nullptr);
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
            auto const outlineColour = isSelected ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

            nvgDrawRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), convertColour(currentColour), outlineColour, Corners::objectCornerRadius);
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
        case hash("lua_draw_svg"): {
            if (argc >= 1) {
                auto* image = nsvgParse(const_cast<char*>(atom_getsymbol(argv)->s_name), "px", 96);
                drawSVG(nvg, image);
            }
            break;
        }
        default:
            break;
        }
    }

    // We need to update the framebuffer in a place where the current graphics context is active (for multi-window support, thanks to Alex for figuring that out)
    // but we also need to be outside of calls to beginFrame/endFrame
    // So we have this separate callback function that occurs after activating the GPU context, but before starting the frame
    void updateFramebuffers() override
    {
        LuaGuiMessage guiMessage;
        for (auto& [layer, layerQueue] : guiMessageQueue) {
            if (layer == -1) // non-layer related messages
            {
                while (layerQueue.try_dequeue(guiMessage)) {
                    handleGuiMessage(layer, guiMessage.symbol, guiMessage.size, guiMessage.data.data());
                }
                continue;
            }

            while (layerQueue.try_dequeue(guiMessage)) {
                guiCommandBuffer[layer].add(guiMessage);
            }

            auto const* startMesage = pd->generateSymbol("lua_start_paint");
            auto const* endMessage = pd->generateSymbol("lua_end_paint");

            int startIdx = -1, endIdx = -1;
            bool updateScene = false;
            for (int i = guiCommandBuffer[layer].size() - 1; i >= 0; i--) {
                if (guiCommandBuffer[layer][i].symbol == startMesage)
                    startIdx = i;
                if (guiCommandBuffer[layer][i].symbol == endMessage)
                    endIdx = i + 1;

                if (startIdx != -1 && endIdx != -1) {
                    updateScene = true;
                    break;
                }
            }

            if (updateScene) {
                if (endIdx > startIdx) {
                    for (int i = startIdx; i < endIdx; i++) {
                        handleGuiMessage(layer, guiCommandBuffer[layer][i].symbol, guiCommandBuffer[layer][i].size, guiCommandBuffer[layer][i].data.data());
                    }
                }
                guiCommandBuffer[layer].erase(guiCommandBuffer[layer].begin(), guiCommandBuffer[layer].begin() + endIdx);
            }

            if (isSelected != object->isSelected() || !framebuffers[layer].isValid()) {
                isSelected = object->isSelected();
                sendRepaintMessage();
            }
        }
    }

    static void drawCallback(void* target, int const layer, t_symbol* sym, int argc, t_atom* argv)
    {
        for (auto* object : allDrawTargets[static_cast<t_pdlua*>(target)]) {
            object->guiMessageQueue[layer].enqueue({ sym, argc, argv });
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
                            _this->pd->sendMessage("pdluax", "reload", {});
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
                pd->sendMessage("pdluax", "reload", {});
            }
            sendRepaintMessage();
        };

        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), onClose, onSave, true));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }
                
    static void drawSVG(NVGcontext* nvg, NSVGimage* svg) {
        auto getNVGColor = [](uint32_t color) -> NVGcolor {
            return nvgRGBA(
                (color >> 0) & 0xff,
                (color >> 8) & 0xff,
                (color >> 16) & 0xff,
                (color >> 24) & 0xff);
        };
        
        auto getLineCrossing = [](Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3) -> float {
            auto b = p2 - p0;
            auto d = p1 - p0;
            auto e = p3 - p2;
            float m = d.x * e.y - d.y * e.x;
            // Check if lines are parallel, or if either pair of points are equal
            if (fabsf(m) < 1e-6)
                return NAN;
            return -(d.x * b.y - d.y * b.x) / m;
        };
        
        auto getPaint = [&getNVGColor](NVGcontext* nvg, NSVGpaint* p) -> NVGpaint{
            assert(p->type == NSVG_PAINT_LINEAR_GRADIENT || p->type == NSVG_PAINT_RADIAL_GRADIENT);
            NSVGgradient *g = p->gradient;
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
        for (NSVGshape *shape = svg->shapes; shape; shape = shape->next, shapeIndex++) {

            // Visibility
            if (!(shape->flags & NSVG_FLAGS_VISIBLE))
                continue;

            nvgSave(nvg);

            // Opacity
            if (shape->opacity < 1.0)
                nvgGlobalAlpha(nvg, shape->opacity);

            // Build path
            nvgBeginPath(nvg);

            // Iterate path linked list
            for (NSVGpath *path = shape->paths; path; path = path->next) {

                nvgMoveTo(nvg, path->pts[0], path->pts[1]);
                for (int i = 1; i < path->npts; i += 3) {
                    float *p = &path->pts[2*i];
                    nvgBezierTo(nvg, p[0], p[1], p[2], p[3], p[4], p[5]);
                }

                // Close path
                if (path->closed)
                    nvgClosePath(nvg);

                // Compute whether this is a hole or a solid.
                // Assume that no paths are crossing (usually true for normal SVG graphics).
                // Also assume that the topology is the same if we use straight lines rather than Beziers (not always the case but usually true).
                // Using the even-odd fill rule, if we draw a line from a point on the path to a point outside the boundary (e.g. top left) and count the number of times it crosses another path, the parity of this count determines whether the path is a hole (odd) or solid (even).
                int crossings = 0;
                Point<float> p0 = Point<float>(path->pts[0], path->pts[1]);
                Point<float> p1 = Point<float>(path->bounds[0] - 1.0, path->bounds[1] - 1.0);
                // Iterate all other paths
                for (NSVGpath *path2 = shape->paths; path2; path2 = path2->next) {
                    if (path2 == path)
                        continue;

                    // Iterate all lines on the path
                    if (path2->npts < 4)
                        continue;
                    for (int i = 1; i < path2->npts + 3; i += 3) {
                        float *p = &path2->pts[2*i];
                        // The previous point
                        Point<float> p2 = Point<float>(p[-2], p[-1]);
                        // The current point
                        Point<float> p3 = (i < path2->npts) ? Point<float>(p[4], p[5]) : Point<float>(path2->pts[0], path2->pts[1]);
                        float crossing = getLineCrossing(p0, p1, p2, p3);
                        float crossing2 = getLineCrossing(p2, p3, p0, p1);
                        if (0.0 <= crossing && crossing < 1.0 && 0.0 <= crossing2) {
                            crossings++;
                        }
                    }
                }

                if (crossings % 2 == 0)
                    nvgPathWinding(nvg, NVG_SOLID);
                else
                    nvgPathWinding(nvg, NVG_HOLE);

    /*
                // Shoelace algorithm for computing the area, and thus the winding direction
                float area = 0.0;
     Point<float> p0 = Point<float>(path->pts[0], path->pts[1]);
                for (int i = 1; i < path->npts; i += 3) {
                    float *p = &path->pts[2*i];
     Point<float> p1 = (i < path->npts) ? Point<float>(p[4], p[5]) : Point<float>(path->pts[0], path->pts[1]);
                    area += 0.5 * (p1.x - p0.x) * (p1.y + p0.y);
                    printf("%f %f, %f %f\n", p0.x, p0.y, p1.x, p1.y);
                    p0 = p1;
                }
                printf("%f\n", area);

                if (area < 0.0)
                    nvgPathWinding(vg, NVG_CCW);
                else
                    nvgPathWinding(vg, NVG_CW);
    */
            }

            // Fill shape
            if (shape->fill.type) {
                switch (shape->fill.type) {
                    case NSVG_PAINT_COLOR: {
                        NVGcolor color = getNVGColor(shape->fill.color);
                        nvgFillColor(nvg, color);
                    } break;
                    case NSVG_PAINT_LINEAR_GRADIENT:
                    case NSVG_PAINT_RADIAL_GRADIENT: {
                        NSVGgradient *g = shape->fill.gradient;
                        (void)g;
                        nvgFillPaint(nvg, getPaint(nvg, &shape->fill));
                    } break;
                }
                nvgFill(nvg);
            }

            // Stroke shape
            if (shape->stroke.type) {
                nvgStrokeWidth(nvg, shape->strokeWidth);
                // strokeDashOffset, strokeDashArray, strokeDashCount not yet supported
                nvgLineCap(nvg, (NVGlineCap) shape->strokeLineCap);
                nvgLineJoin(nvg, (int) shape->strokeLineJoin);

                switch (shape->stroke.type) {
                    case NSVG_PAINT_COLOR: {
                        NVGcolor color = getNVGColor(shape->stroke.color);
                        nvgStrokeColor(nvg, color);
                    } break;
                    case NSVG_PAINT_LINEAR_GRADIENT: {
                        // NSVGgradient *g = shape->stroke.gradient;
                        // printf("        lin grad: %f\t%f\n", g->fx, g->fy);
                    } break;
                }
                nvgStroke(nvg);
            }

            nvgRestore(nvg);
        }
    }
};

// A non-GUI Lua object, that we would still like to have clickable for opening the editor
class LuaTextObject final : public TextBase {
public:
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

    LuaTextObject(pd::WeakReference ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (getValue<bool>(object->locked)) {
            if (auto obj = ptr.get<t_pd>()) {
                pd->sendDirectMessage(obj.get(), "menu-open", {});
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
        menu.addItem("Open lua editor", [_this = SafePointer(this)] {
            if (!_this)
                return;
            if (auto obj = _this->ptr.get<t_pd>()) {
                _this->pd->sendDirectMessage(obj.get(), "menu-open", {});
            }
        });

        menu.addItem("Reload lua object", [_this = SafePointer(this)] {
            if (!_this)
                return;
            if (auto pdlua = _this->ptr.get<t_pd>()) {
                // Reload the lua script
                _this->pd->sendMessage("pdluax", "reload", {});

                // Recreate this object
                if (auto patch = _this->cnv->patch.getPointer()) {
                    pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                }
            }
        });
    }

    void openTextEditor(File fileToOpen)
    {
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
                            pd->sendMessage("pdluax", "reload", {});
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
                pd->sendMessage("pdluax", "reload", {});
            }
        };

        textEditor.reset(Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), onClose, onSave, true));

        if (textEditor)
            cnv->editor->openTextEditors.add_unique(ptr);
    }
};
