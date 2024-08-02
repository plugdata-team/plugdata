/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
#include <pd-lua/lua/lua.h>

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
    , public Timer {

    Colour currentColour;

    CriticalSection bufferSwapLock;
    bool isSelected = false;
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

    NVGFramebuffer framebuffer;

    struct LuaGuiMessage {
        t_symbol* symbol;
        std::vector<t_atom> data;
        int size;

        LuaGuiMessage() {};

        LuaGuiMessage(t_symbol* sym, int argc, t_atom* argv)
            : symbol(sym)
        {
            data = std::vector<t_atom>(argv, argv + argc);
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

    std::vector<LuaGuiMessage> guiCommandBuffer;
    moodycamel::ReaderWriterQueue<LuaGuiMessage> guiMessageQueue;

    static inline std::map<t_pdlua*, std::vector<LuaObject*>> allDrawTargets = std::map<t_pdlua*, std::vector<LuaObject*>>();

public:
    LuaObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            pdlua->gfx.plugdata_draw_callback = &drawCallback;
            allDrawTargets[pdlua.get()].push_back(this);
        }

        parentHierarchyChanged();
        startTimerHz(60);
    }

    ~LuaObject()
    {
        if (auto pdlua = ptr.get<t_pdlua>()) {
            auto& listeners = allDrawTargets[pdlua.get()];
            listeners.erase(std::remove(listeners.begin(), listeners.end(), this), listeners.end());
        }

        zoomScale.removeListener(this);
    }

    // We can only attach the zoomscale to canvas after the canvas has been added to its own parent
    // When initialising, this isn't always the case
    void parentHierarchyChanged() override
    {
        // We need to get the actual zoom from the top level canvas, not of the graph this happens to be inside of
        auto* topLevelCanvas = cnv;

        while (auto* nextCnv = topLevelCanvas->findParentComponentOfClass<Canvas>()) {
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
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.cast<t_gobj>(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, gobj->gfx.width + 2, gobj->gfx.height + 2);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_pdlua>()) {
            auto* patch = object->cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, gobj.cast<t_gobj>(), b.getX(), b.getY());
            gobj->gfx.width = b.getWidth() - 2;
            gobj->gfx.height = b.getHeight() - 2;
        }

        sendRepaintMessage();
    }

    void updateSizeProperty() override
    {
    }

    void mouseDown(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            pdlua_gfx_mouse_down(pdlua, x, y);
        });
    }

    void mouseDrag(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            pdlua_gfx_mouse_drag(pdlua, x, y);
        });
    }

    void mouseMove(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            pdlua_gfx_mouse_move(pdlua, x, y);
        });
    }

    void mouseUp(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua) {
            pdlua_gfx_mouse_up(pdlua, x, y);
        });
    }

    void sendRepaintMessage()
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [](t_pdlua* pdlua) {
            pdlua_gfx_repaint(pdlua, 0);
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
        framebuffer.render(nvg, Rectangle<int>(getWidth() + 1, getHeight()));
    }

    void valueChanged(Value& v) override
    {
        sendRepaintMessage();
    }

    void handleGuiMessage(t_symbol* sym, int argc, t_atom* argv)
    {
        NVGcontext* nvg = cnv->editor->nvgSurface.getRawContext();
        if (!nvg)
            return;

        auto hashsym = hash(sym->s_name);
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch (hashsym) {
        case hash("lua_start_paint"): {
            if (getLocalBounds().isEmpty())
                break;
            auto scale = getValue<float>(zoomScale) * 2.0f; // Multiply by 2 for hi-dpi screens
            int imageWidth = std::ceil(getWidth() * scale);
            int imageHeight = std::ceil(getHeight() * scale);
            if (!imageWidth || !imageHeight)
                return;

            framebuffer.bind(nvg, imageWidth, imageHeight);

            nvgViewport(0, 0, getWidth() * scale, getHeight() * scale);
            nvgClear(nvg);
            nvgBeginFrame(nvg, getWidth(), getHeight(), scale);
            nvgSave(nvg);
            return;
        }
        case hash("lua_end_paint"): {
            if (!framebuffer.isValid())
                return;

            nvgEndFrame(nvg);
            framebuffer.unbind();
            repaint();
            return;
        }
        case hash("lua_resized"): {
            if (argc >= 2) {
                if (auto pdlua = ptr.get<t_pdlua>()) {
                    pdlua->gfx.width = atom_getfloat(argv);
                    pdlua->gfx.height = atom_getfloat(argv + 1);
                }
                MessageManager::callAsync([_object = SafePointer(object)]() {
                    if (_object)
                        _object->updateBounds();
                });
            }
            return;
        }
        }

        if (!framebuffer.isValid())
            return; // If there is no active framebuffer at this point, return

        switch (hashsym) {
        case hash("lua_set_color"): {
            if (argc == 1) {
                int colourID = atom_getfloat(argv);

                auto& lnf = LookAndFeel::getDefaultLookAndFeel();
                currentColour = Array<Colour> { lnf.findColour(PlugDataColour::guiObjectBackgroundColourId), lnf.findColour(PlugDataColour::canvasTextColourId), lnf.findColour(PlugDataColour::guiObjectInternalOutlineColour) }[colourID];
                nvgFillColor(nvg, convertColour(currentColour));
                nvgStrokeColor(nvg, convertColour(currentColour));
            }
            if (argc >= 3) {
                Colour color(static_cast<uint8>(atom_getfloat(argv)),
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
                float x1 = atom_getfloat(argv);
                float y1 = atom_getfloat(argv + 1);
                float x2 = atom_getfloat(argv + 2);
                float y2 = atom_getfloat(argv + 3);
                float lineThickness = atom_getfloat(argv + 4);

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
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);

                nvgBeginPath(nvg);
                nvgEllipse(nvg, x + (w / 2), y + (h / 2), w / 2, h / 2);
                nvgFill(nvg);
            }
            break;
        }
        case hash("lua_stroke_ellipse"): {
            if (argc >= 4) {
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);
                float lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgEllipse(nvg, x + (w / 2), y + (h / 2), w / 2, h / 2);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_fill_rect"): {
            if (argc >= 4) {
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);

                nvgFillRect(nvg, x, y, w, h);
            }
            break;
        }
        case hash("lua_stroke_rect"): {
            if (argc >= 5) {
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);
                float lineThickness = atom_getfloat(argv + 4);

                nvgStrokeWidth(nvg, lineThickness);
                nvgStrokeRect(nvg, x, y, w, h);
            }
            break;
        }
        case hash("lua_fill_rounded_rect"): {
            if (argc >= 4) {
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);
                float cornerRadius = atom_getfloat(argv + 4);

                nvgFillRoundedRect(nvg, x, y, w, h, cornerRadius);
            }
            break;
        }

        case hash("lua_stroke_rounded_rect"): {
            if (argc >= 6) {
                float x = atom_getfloat(argv);
                float y = atom_getfloat(argv + 1);
                float w = atom_getfloat(argv + 2);
                float h = atom_getfloat(argv + 3);
                float cornerRadius = atom_getfloat(argv + 4);
                float lineThickness = atom_getfloat(argv + 5);

                nvgStrokeWidth(nvg, lineThickness);
                nvgBeginPath(nvg);
                nvgRoundedRect(nvg, x, y, w, h, cornerRadius);
                nvgStroke(nvg);
            }
            break;
        }
        case hash("lua_draw_line"): {
            if (argc >= 4) {
                float x1 = atom_getfloat(argv);
                float y1 = atom_getfloat(argv + 1);
                float x2 = atom_getfloat(argv + 2);
                float y2 = atom_getfloat(argv + 3);
                float lineThickness = atom_getfloat(argv + 4);

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
                float x = atom_getfloat(argv + 1);
                float y = atom_getfloat(argv + 2);
                float w = atom_getfloat(argv + 3);
                float fontHeight = atom_getfloat(argv + 4);

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
                float x = atom_getfloat(argv + (i * 2));
                float y = atom_getfloat(argv + (i * 2) + 1);
                nvgLineTo(nvg, x, y);
            }

            nvgClosePath(nvg);
            nvgFill(nvg);
            break;
        }
        case hash("lua_stroke_path"): {
            nvgBeginPath(nvg);
            auto strokeWidth = atom_getfloat(argv);

            int numPoints = (argc - 1) / 2;
            nvgMoveTo(nvg, atom_getfloat(argv + 1), atom_getfloat(argv + 2));
            for (int i = 1; i < numPoints; i++) {
                float x = atom_getfloat(argv + (i * 2) + 1);
                float y = atom_getfloat(argv + (i * 2) + 2);
                nvgLineTo(nvg, x, y);
            }

            nvgStrokeWidth(nvg, strokeWidth);
            nvgStroke(nvg);
            break;
        }
        case hash("lua_fill_all"): {
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            auto outlineColour = cnv->editor->getLookAndFeel().findColour(isSelected ? PlugDataColour::objectSelectedOutlineColourId :  objectOutlineColourId);

            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::objectCornerRadius);
            nvgFill(nvg);

            nvgStrokeWidth(nvg, 1.0f);
            nvgStrokeColor(nvg, convertColour(outlineColour));
            nvgStroke(nvg);

            nvgStrokeColor(nvg, convertColour(currentColour));
            break;
        }

        case hash("lua_translate"): {
            if (argc >= 2) {
                float tx = atom_getfloat(argv);
                float ty = atom_getfloat(argv + 1);
                nvgTranslate(nvg, tx, ty);
            }
            break;
        }
        case hash("lua_scale"): {
            if (argc >= 2) {
                float sx = atom_getfloat(argv);
                float sy = atom_getfloat(argv + 1);
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

    void timerCallback() override
    {
        LuaGuiMessage guiMessage;
        while (guiMessageQueue.try_dequeue(guiMessage)) {
            guiCommandBuffer.push_back(guiMessage);
        }

        auto* startMesage = pd->generateSymbol("lua_start_paint");
        auto* endMessage = pd->generateSymbol("lua_end_paint");

        int startIdx = -1, endIdx = -1;
        bool updateScene = false;
        for (int i = guiCommandBuffer.size() - 1; i >= 0; i--) {
            if (guiCommandBuffer[i].symbol == startMesage)
                startIdx = i;
            if (guiCommandBuffer[i].symbol == endMessage)
                endIdx = i + 1;

            if (startIdx != -1 && endIdx != -1) {
                updateScene = true;
                break;
            }
        }

        if (updateScene) {
            if (endIdx > startIdx) {
                for (int i = startIdx; i < endIdx; i++) {
                    handleGuiMessage(guiCommandBuffer[i].symbol, guiCommandBuffer[i].size, guiCommandBuffer[i].data.data());
                }
            }
            guiCommandBuffer.erase(guiCommandBuffer.begin(), guiCommandBuffer.begin() + endIdx);
        }

        if (isSelected != object->isSelected() || !framebuffer.isValid()) {
            isSelected = object->isSelected();
            sendRepaintMessage();
        }
    }

    static void drawCallback(void* target, t_symbol* sym, int argc, t_atom* argv)
    {
        for (auto* object : allDrawTargets[static_cast<t_pdlua*>(target)]) {
            object->guiMessageQueue.enqueue({ sym, argc, argv });
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int argc) override
    {
        if (symbol == hash("open_textfile") && argc >= 1) {
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

        textEditor.reset(
            Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), [this, fileToOpen](String const& newText, bool hasChanged) {
                if (!hasChanged) {
                    cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showAskToSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, newText, fileToOpen](int result) mutable {
                        if (result == 2) {
                            fileToOpen.replaceWithText(newText);
                            if (auto pdlua = ptr.get<t_pd>()) {
                                // Reload the lua script
                                pd_typedmess(pdlua.get(), pd->generateSymbol("_reload"), 0, nullptr);

                                // Recreate this object
                                if(auto patch = cnv->patch.getPointer()) {
                                    pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                                }
                            }
                            cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                            textEditor.reset(nullptr);
                            cnv->performSynchronise();
                        }
                        if (result == 1) {
                            cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                            textEditor.reset(nullptr);
                        }
                    },
                    15, false);
            }));
        if (textEditor)
            cnv->editor->openTextEditors.addIfNotAlreadyThere(ptr);
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
            // TODO: open text editor!
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        if (symbol == hash("open_textfile") && numAtoms >= 1) {
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

        textEditor.reset(
            Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), [this, fileToOpen](String const& newText, bool hasChanged) {
                if (!hasChanged) {
                    cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showAskToSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, newText, fileToOpen](int result) mutable {
                        if (result == 2) {
                            fileToOpen.replaceWithText(newText);
                            if (auto pdlua = ptr.get<t_pd>()) {
                                // Reload the lua script
                                pd_typedmess(pdlua.get(), pd->generateSymbol("_reload"), 0, nullptr);

                                // Recreate this object
                                if(auto patch = cnv->patch.getPointer()) {
                                    pd::Interface::recreateTextObject(patch.get(), pdlua.cast<t_gobj>());
                                }
                            }
                            cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                            textEditor.reset(nullptr);
                            cnv->performSynchronise();
                        }
                        if (result == 1) {
                            cnv->editor->openTextEditors.removeAllInstancesOf(ptr);
                            textEditor.reset(nullptr);
                        }
                    },
                    15, false);
            }));
        if (textEditor)
            cnv->editor->openTextEditors.addIfNotAlreadyThere(ptr);
    }
};
