/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


extern "C"
{
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

class LuaObject : public ObjectBase, public AsyncUpdater {
    
    std::unique_ptr<Graphics> graphics;
    Colour currentColour;
    
    CriticalSection bufferSwapLock;
    Image firstBuffer;
    Image secondBuffer;
    Image* drawBuffer = &firstBuffer;
    Image* displayBuffer = &secondBuffer;
    bool isSelected = false;
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;
        
    static inline std::map<t_gpointer*, Path> currentPaths = std::map<t_gpointer*, Path>();
    static inline std::map<t_pdlua*, std::vector<LuaObject*>> allDrawTargets = std::map<t_pdlua*, std::vector<LuaObject*>>();
    
public:
    LuaObject(pd::WeakReference obj, Object* parent)
    : ObjectBase(obj, parent)
    {
        if(auto pdlua = ptr.get<t_pdlua>())
        {
            pdlua->gfx.plugdata_draw_callback = &drawCallback;
            allDrawTargets[pdlua.get()].push_back(this);
        }
        
        parentHierarchyChanged();
    }
    
    // We can only attach the zoomscale to canvas after the canvas has been added to its own parent
    // When initialising, this isn't always the case
    void parentHierarchyChanged() override
    {
        // We need to get the actual zoom from the top level canvas, not of the graph this happens to be inside of
        auto* topLevelCanvas = cnv;
        
        while(topLevelCanvas && topLevelCanvas->isGraph)
        {
            topLevelCanvas = topLevelCanvas->findParentComponentOfClass<Canvas>();
        }
        if(topLevelCanvas) {
            zoomScale.referTo(topLevelCanvas->zoomScale);
            zoomScale.addListener(this);
            sendRepaintMessage();
        }
    }
    
    ~LuaObject()
    {
        if(auto pdlua = ptr.get<t_pdlua>())
        {
            auto& listeners = allDrawTargets[pdlua.get()];
            listeners.erase(std::remove(listeners.begin(), listeners.end(), this), listeners.end());
        }
        zoomScale.removeListener(this);
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
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua){
            pdlua_gfx_mouse_down(pdlua, x, y);
        });
    }
    
    void mouseDrag(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua){
            pdlua_gfx_mouse_drag(pdlua, x, y);
        });
    }
    
    void mouseMove(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua){
            pdlua_gfx_mouse_move(pdlua, x, y);
        });
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [x = e.x, y = e.y](t_pdlua* pdlua){
            pdlua_gfx_mouse_up(pdlua, x, y);
        });
    }
    
    void sendRepaintMessage()
    {
        pd->enqueueFunctionAsync<t_pdlua>(ptr, [](t_pdlua* pdlua){
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
    
    void paint(Graphics& g) override
    {
        if(isSelected != object->isSelected())
        {
            isSelected = object->isSelected();
            sendRepaintMessage();
        }
        
        ScopedLock swapLock(bufferSwapLock);
        g.drawImage(*displayBuffer, getLocalBounds().toFloat());
    }
    
    void valueChanged(Value& v) override
    {
        sendRepaintMessage();
    }
    
    void handleAsyncUpdate() override
    {
        repaint();
    }

    static void receiveLuaPathMessage(t_gpointer* path, t_symbol* sym, int argc, t_atom* argv)
    {
        switch(hash(sym->s_name)) {
            case hash("lua_start_path"): {
                if (argc >= 2) {
                    auto& currentPath = currentPaths[path];
                    float x = atom_getfloat(argv);
                    float y = atom_getfloat(argv + 1);
                    currentPath.startNewSubPath(x, y);
                }
                break;
            }
            case hash("lua_line_to"): {
                if (argc >= 2) {
                    auto& currentPath = currentPaths[path];
                    float x = atom_getfloat(argv);
                    float y = atom_getfloat(argv + 1);
                    currentPath.lineTo(x, y);
                }
                break;
            }
            case hash("lua_quad_to"): {
                if (argc >= 4) { // Assuming quad_to takes 3 arguments
                    auto& currentPath = currentPaths[path];
                    float x1 = atom_getfloat(argv);
                    float y1 = atom_getfloat(argv + 1);
                    float x2 = atom_getfloat(argv + 2);
                    float y2 = atom_getfloat(argv + 3);
                    currentPath.quadraticTo(x1, y1, x2, y2);
                }
                break;
            }
            case hash("lua_cubic_to"): {
                if (argc >= 5) { // Assuming cubic_to takes 4 arguments
                    auto& currentPath = currentPaths[path];
                    float x1 = atom_getfloat(argv);
                    float y1 = atom_getfloat(argv + 1);
                    float x2 = atom_getfloat(argv + 2);
                    float y2 = atom_getfloat(argv + 3);
                    float x3 = atom_getfloat(argv + 4);
                    float y3 = atom_getfloat(argv + 5);
                    currentPath.cubicTo(x1, y1, x2, y2, x3, y3);
                }
                break;
            }
            case hash("lua_close_path"): {
                auto& currentPath = currentPaths[path];
                currentPath.closeSubPath();
                break;
            }
            case hash("lua_free_path"): {
                currentPaths.erase(path);
                break;
            }
            default: 
                break;
        }
    }
    
    void receiveLuaPaintMessage(t_symbol* sym, int argc, t_atom* argv)
    {
        auto hashsym = hash(sym->s_name);
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch(hashsym)
        {
            case hash("lua_start_paint"): {
                if(getLocalBounds().isEmpty()) break;
                auto scale = getValue<float>(zoomScale) * 2.0f; // Multiply by 2 for hi-dpi screens
                int imageWidth = std::ceil(getWidth() * scale);
                int imageHeight = std::ceil(getHeight() * scale);
                if(!imageWidth || !imageHeight) return;
                
                if(drawBuffer->getWidth() != imageWidth || drawBuffer->getHeight() != imageHeight)
                {
                    *drawBuffer = Image(Image::PixelFormat::ARGB, imageWidth, imageHeight, true);
                }
                else{
                    drawBuffer->clear(Rectangle<int>(imageWidth, imageHeight));
                }
                
                graphics = std::make_unique<Graphics>(*drawBuffer);
                graphics->saveState();
                graphics->addTransform(AffineTransform::scale(scale)); // for hi-dpi displays and canvas zooming
                graphics->saveState();
                return;
            }
            case hash("lua_end_paint"): {
                if(!graphics) break;
                graphics->restoreState();
                graphics.reset(nullptr);

                // swap buffers
                {
                    ScopedLock swapLock(bufferSwapLock);
                    auto* oldDrawBuffer = drawBuffer;
                    drawBuffer = displayBuffer;
                    displayBuffer = oldDrawBuffer;
                }
                
                triggerAsyncUpdate();
                return;
            }
            case hash("lua_resized"): {
                if (argc >= 2) {
                    if (auto pdlua = ptr.get<t_pdlua>()) {
                        pdlua->gfx.width = atom_getfloat(argv);
                        pdlua->gfx.height = atom_getfloat(argv + 1);
                    }
                    MessageManager::callAsync([_object = SafePointer(object)](){
                        if(_object) _object->updateBounds();
                    });
                }
                return;
            }
        }
        
        if(!graphics) return; // If there is no active graphics context at this point, return
        
        // Functions that do need an active graphics context
        switch (hashsym) {
            case hash("lua_set_color"): {
                if (argc == 1) {
                    int colourID = atom_getfloat(argv);
                    
                    auto& lnf = LookAndFeel::getDefaultLookAndFeel();
                    currentColour = Array<Colour>{lnf.findColour(PlugDataColour::guiObjectBackgroundColourId), lnf.findColour(PlugDataColour::canvasTextColourId), lnf.findColour(PlugDataColour::guiObjectInternalOutlineColour)}[colourID];
                    graphics->setColour(currentColour);
                }
                if (argc >= 3) {
                    Colour color(static_cast<uint8>(atom_getfloat(argv)),
                                 static_cast<uint8>(atom_getfloat(argv + 1)),
                                 static_cast<uint8>(atom_getfloat(argv + 2)));
                    
                    currentColour = color.withAlpha(argc >= 4 ? atom_getfloat(argv + 3) : 1.0f);
                    graphics->setColour(currentColour);
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
                    
                    graphics->drawLine(x1, y1, x2, y2, lineThickness);
                }
                break;
            }
            case hash("lua_fill_ellipse"): {
                if (argc >= 3) {
                    float x = atom_getfloat(argv);
                    float y = atom_getfloat(argv + 1);
                    float w = atom_getfloat(argv + 2);
                    float h = atom_getfloat(argv + 3);
                    graphics->fillEllipse(Rectangle<float>(x, y, w, h));
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
                    graphics->drawEllipse(Rectangle<float>(x, y, w, h), lineThickness);
                }
                break;
            }
            case hash("lua_fill_rect"): {
                if (argc >= 4) {
                    float x = atom_getfloat(argv);
                    float y = atom_getfloat(argv + 1);
                    float w = atom_getfloat(argv + 2);
                    float h = atom_getfloat(argv + 3);
                    graphics->fillRect(Rectangle<float>(x, y, w, h));
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
                    graphics->drawRect(Rectangle<float>(x, y, w, h), lineThickness);
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
                    graphics->fillRoundedRectangle(Rectangle<float>(x, y, w, h), cornerRadius);
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
                    graphics->drawRoundedRectangle(Rectangle<float>(x, y, w, h), cornerRadius, lineThickness);
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
                    graphics->drawLine(x1, y1, x2, y2, lineThickness);
                }
                break;
            }
            case hash("lua_draw_text"): {
                if (argc >= 4) {
                    auto text = String::fromUTF8(atom_getsymbol(argv)->s_name);
                    auto string = AttributedString(text);
                    float x = atom_getfloat(argv + 1);
                    float y = atom_getfloat(argv + 2);
                    float w = atom_getfloat(argv + 3);
                    float fontHeight = atom_getfloat(argv + 4) * 1.3; // Increase fontsize to match pd-vanilla
                    
                    string.setFont(Font(fontHeight));
                    string.setColour(currentColour);
                    string.setJustification(Justification::topLeft);
                    
                    TextLayout layout;
                    layout.createLayout(string, w);
                    layout.draw(*graphics, {x, y, w, layout.getHeight()});
                }
                break;
            }
            case hash("lua_fill_path"): {
                if (argc >= 1) {
                    auto& currentPath = currentPaths[argv[0].a_w.w_gpointer];
                    graphics->fillPath(currentPath);
                }
                break;
            }
            case hash("lua_stroke_path"): {
                if (argc >= 2) {
                    auto& currentPath = currentPaths[argv[0].a_w.w_gpointer];
                    graphics->strokePath(currentPath, PathStrokeType(atom_getfloat(argv + 1)));
                }
                break;
            }
            case hash("lua_fill_all"): {
                auto colour = currentColour;
                
                graphics->fillRoundedRectangle(getLocalBounds().toFloat(), Corners::objectCornerRadius);
                
                auto outlineColour = object->findColour(isSelected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);
                graphics->setColour(outlineColour);
                graphics->drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f).withTrimmedRight(0.5f).withTrimmedBottom(0.5f), Corners::objectCornerRadius, 1.0f);
                
                graphics->setColour(colour);
                
                break;
            }
                
            case hash("lua_translate"): {
                if (argc >= 2) {
                    float tx = atom_getfloat(argv);
                    float ty = atom_getfloat(argv + 1);
                    graphics->addTransform(AffineTransform::translation(tx, ty));
                }
                break;
            }
            case hash("lua_scale"): {
                if (argc >= 2) {
                    float sx = atom_getfloat(argv);
                    float sy = atom_getfloat(argv + 1);
                    graphics->addTransform(AffineTransform::scale(sx, sy));
                }
                break;
            }
            case hash("lua_reset_transform"): {
                graphics->restoreState();
                graphics->saveState();
                break;
            }
            default:
                break;
        }
    }
    
    static void drawCallback(void* target, t_symbol* sym, int argc, t_atom* argv)
    {
        if(target == nullptr && argc != 0)
        {
            receiveLuaPathMessage(argv[0].a_w.w_gpointer, sym, argc-1, argv+1);
            return;
        }
        
        for(auto* object : allDrawTargets[static_cast<t_pdlua*>(target)])
        {
            object->receiveLuaPaintMessage(sym, argc, argv);
        }
    }
    
    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int argc) override
    {
        if(symbol == hash("open_textfile") && argc >= 1)
        {
            openTextEditor(File(atoms[0].toString()));
        }
    }
    
    void openTextEditor(File fileToOpen)
    {
        if (textEditor) {
            textEditor->toFront(true);
            return;
        }
        textEditor.reset(
            Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), [this, fileToOpen](String const& newText, bool hasChanged) {
                if (!hasChanged) {
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showAskToSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, newText, fileToOpen](int result) mutable {
                        if (result == 2) {
                            fileToOpen.replaceWithText(newText);
                            if(auto pdlua = ptr.get<t_pd>())
                            {
                                // Reload the lua script
                                pd_typedmess(pdlua.get(), pd->generateSymbol("_reload"), 0, nullptr);
                                
                                // Recreate this object
                                auto* patch = cnv->patch.getPointer().get();
                                pd::Interface::recreateTextObject(patch, pdlua.cast<t_gobj>());
                            }
                            textEditor.reset(nullptr);
                            cnv->performSynchronise();
                        }
                        if (result == 1) {
                            textEditor.reset(nullptr);
                        }
                    },
                    15, false);
            }));
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
        if(symbol == hash("open_textfile") && numAtoms >= 1)
        {
            openTextEditor(File(atoms[0].toString()));
        }
    }
    
    void openTextEditor(File fileToOpen)
    {
        if (textEditor) {
            textEditor->toFront(true);
            return;
        }
        textEditor.reset(
            Dialogs::showTextEditorDialog(fileToOpen.loadFileAsString(), "lua: " + getText(), [this, fileToOpen](String const& newText, bool hasChanged) {
                if (!hasChanged) {
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showAskToSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, newText, fileToOpen](int result) mutable {
                        if (result == 2) {
                            fileToOpen.replaceWithText(newText);
                            if(auto pdlua = ptr.get<t_pd>())
                            {
                                // Reload the lua script
                                pd_typedmess(pdlua.get(), pd->generateSymbol("_reload"), 0, nullptr);
                                
                                // Recreate this object
                                auto* patch = cnv->patch.getPointer().get();
                                pd::Interface::recreateTextObject(patch, pdlua.cast<t_gobj>());
                            }
                            textEditor.reset(nullptr);
                            cnv->performSynchronise();
                        }
                        if (result == 1) {
                            textEditor.reset(nullptr);
                        }
                    },
                    15, false);
            }));
    }
};
