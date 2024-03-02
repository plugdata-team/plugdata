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

struct LuaGuiMessage {
    
    t_symbol* symbol;
    t_atom data[8];
    int size;
    
    LuaGuiMessage() = default;
    
    LuaGuiMessage(t_symbol* sym, int argc, t_atom* argv)
    : symbol(sym)
    {
        size = std::min(argc, 8);
        std::copy(argv, argv + size, data);
    }
    
    LuaGuiMessage(LuaGuiMessage const& other) noexcept
    {
        symbol = other.symbol;
        size = other.size;
        std::copy(other.data, other.data + other.size, data);
    }
    
    LuaGuiMessage& operator=(LuaGuiMessage const& other) noexcept
    {
        // Check for self-assignment
        if (this != &other) {
            symbol = other.symbol;
            size = other.size;
            std::copy(other.data, other.data + other.size, data);
        }
        
        return *this;
    }
};

class LuaObject : public ObjectBase, public Timer {
    
    std::unique_ptr<Graphics> graphics;
    Colour currentColour;
    std::map<t_symbol*, Path> currentPaths;
    Image firstBuffer;
    Image secondBuffer;
    Image* drawBuffer = &firstBuffer;
    Image* displayBuffer = &secondBuffer;
    bool isSelected = false;
    moodycamel::ReaderWriterQueue<LuaGuiMessage> guiQueue = moodycamel::ReaderWriterQueue<LuaGuiMessage>(40);
    Value zoomScale;
    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;
    
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
        startTimerHz(60); // Check for paint messages at 60hz (but we only really repaint when needed)
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
        g.drawImage(*displayBuffer, getLocalBounds().toFloat());
    }
    
    void valueChanged(Value& v) override
    {
        sendRepaintMessage();
    }
    
    void timerCallback() override
    {
        LuaGuiMessage guiMessage;
        while(guiQueue.try_dequeue(guiMessage))
        {
            handleGuiMessage(guiMessage);
        }

        if(isSelected != object->isSelected())
        {
            isSelected = object->isSelected();
            sendRepaintMessage();
        }
    }
    
    void handleGuiMessage(LuaGuiMessage& message)
    {
        pd::Atom atoms[8];
        int numAtoms = message.size;
        for (int at = 0; at < numAtoms; at++) {
            atoms[at] = pd::Atom(message.data + at);
        }
        auto hashsym = hash(message.symbol->s_name);
        
        // First check functions that don't need an active graphics context, of modify the active graphics context
        switch(hashsym)
        {
            case hash("lua_start_paint"): {
                auto scale = getValue<float>(zoomScale) * 2.0f; // Multiply by 2 for hi-dpi screens
                if(firstBuffer.getBounds() != getLocalBounds() * scale)
                {
                    firstBuffer = Image(Image::PixelFormat::ARGB, getWidth() * scale, getHeight() * scale, true);
                    secondBuffer = Image(Image::PixelFormat::ARGB, getWidth() * scale, getHeight() * scale, true);
                }
                else{
                    drawBuffer->clear(getLocalBounds() * scale);
                }
                
                graphics = std::make_unique<Graphics>(*drawBuffer);
                graphics->saveState();
                graphics->addTransform(AffineTransform::scale(scale)); // for hi-dpi displays and canvas zooming
                graphics->saveState();
                //graphics->addTransform(AffineTransform::translation(1, 1));
                return;
            }
            case hash("lua_end_paint"): {
                graphics->restoreState();
                graphics.reset(nullptr);

                // swap buffers
                auto* oldDrawBuffer = drawBuffer;
                drawBuffer = displayBuffer;
                displayBuffer = oldDrawBuffer;
                
                repaint();
                return;
            }
            case hash("lua_resized"): {
                if (numAtoms >= 2) {
                    if (auto pdlua = ptr.get<t_pdlua>()) {
                        pdlua->gfx.width = atoms[0].getFloat();
                        pdlua->gfx.height = atoms[1].getFloat();
                    }
                    object->updateBounds();
                }
                std::cout << "RESIZED" << std::endl;
                return;
            }
            case hash("lua_start_path"): {
                if (numAtoms >= 3) {
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    float x = atoms[1].getFloat();
                    float y = atoms[2].getFloat();
                    currentPath.startNewSubPath(x, y);
                }
                break;
            }
            case hash("lua_line_to"): {
                if (numAtoms >= 3) {
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    float x = atoms[1].getFloat();
                    float y = atoms[2].getFloat();
                    currentPath.lineTo(x, y);
                }
                break;
            }
            case hash("lua_quad_to"): {
                if (numAtoms >= 5) { // Assuming quad_to takes 3 arguments
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    float x1 = atoms[1].getFloat();
                    float y1 = atoms[2].getFloat();
                    float x2 = atoms[3].getFloat();
                    float y2 = atoms[4].getFloat();
                    currentPath.quadraticTo(x1, y1, x2, y2);
                }
                break;
            }
            case hash("lua_cubic_to"): {
                if (numAtoms >= 6) { // Assuming cubic_to takes 4 arguments
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    float x1 = atoms[1].getFloat();
                    float y1 = atoms[2].getFloat();
                    float x2 = atoms[3].getFloat();
                    float y2 = atoms[4].getFloat();
                    float x3 = atoms[5].getFloat();
                    float y3 = atoms[6].getFloat();
                    currentPath.cubicTo(x1, y1, x2, y2, x3, y3);
                }
                break;
            }
            case hash("lua_close_path"): {
                if (numAtoms >= 1) {
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    currentPath.closeSubPath();
                }
                break;
            }
            case hash("lua_free_path"): {
                if (numAtoms >= 1) {
                    currentPaths.erase(atoms[0].getSymbol());
                }
                break;
            }
        }
        
        if(!graphics) return;
        
        // Functions that do need an active graphics context
        switch (hashsym) {
            case hash("lua_set_color"): {
                if (numAtoms == 1) {
                    int colourID = atoms[0].getFloat();
                    currentColour = Array<Colour>{object->findColour(PlugDataColour::guiObjectBackgroundColourId), object->findColour(PlugDataColour::canvasTextColourId), object->findColour(PlugDataColour::guiObjectInternalOutlineColour)}[colourID];
                    graphics->setColour(currentColour);
                }
                if (numAtoms >= 3) {
                    Colour color(static_cast<uint8>(atoms[0].getFloat()),
                                 static_cast<uint8>(atoms[1].getFloat()),
                                 static_cast<uint8>(atoms[2].getFloat()));
                    
                    currentColour = color.withAlpha(numAtoms >= 4 ? atoms[3].getFloat() : 1.0f);
                    graphics->setColour(currentColour);
                }
                break;
            }
            case hash("lua_stroke_line"): {
                if (numAtoms >= 4) {
                    float x1 = atoms[0].getFloat();
                    float y1 = atoms[1].getFloat();
                    float x2 = atoms[2].getFloat();
                    float y2 = atoms[3].getFloat();
                    float lineThickness = atoms[4].getFloat();
                    
                    graphics->drawLine(x1, y1, x2, y2, lineThickness);
                }
                break;
            }
            case hash("lua_fill_ellipse"): {
                if (numAtoms >= 3) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    graphics->fillEllipse(Rectangle<float>(x, y, w, h));
                }
                break;
            }
            case hash("lua_stroke_ellipse"): {
                if (numAtoms >= 4) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    float lineThickness = atoms[4].getFloat();
                    graphics->drawEllipse(Rectangle<float>(x, y, w, h), lineThickness);
                }
                break;
            }
            case hash("lua_fill_rect"): {
                if (numAtoms >= 4) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    graphics->fillRect(Rectangle<float>(x, y, w, h));
                }
                break;
            }
            case hash("lua_stroke_rect"): {
                if (numAtoms >= 5) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    float lineThickness = atoms[4].getFloat();
                    graphics->drawRect(Rectangle<float>(x, y, w, h), lineThickness);
                }
                break;
            }
            case hash("lua_fill_rounded_rect"): {
                if (numAtoms >= 4) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    float cornerRadius = atoms[4].getFloat();
                    graphics->fillRoundedRectangle(Rectangle<float>(x, y, w, h), cornerRadius);
                }
                break;
            }

            case hash("lua_stroke_rounded_rect"): {
                if (numAtoms >= 6) {
                    float x = atoms[0].getFloat();
                    float y = atoms[1].getFloat();
                    float w = atoms[2].getFloat();
                    float h = atoms[3].getFloat();
                    float cornerRadius = atoms[4].getFloat();
                    float lineThickness = atoms[5].getFloat();
                    graphics->drawRoundedRectangle(Rectangle<float>(x, y, w, h), cornerRadius, lineThickness);
                }
                break;
            }
            case hash("lua_draw_line"): {
                if (numAtoms >= 4) {
                    float x1 = atoms[0].getFloat();
                    float y1 = atoms[1].getFloat();
                    float x2 = atoms[2].getFloat();
                    float y2 = atoms[3].getFloat();
                    float lineThickness = atoms[4].getFloat();
                    graphics->drawLine(x1, y1, x2, y2, lineThickness);
                }
                break;
            }
            case hash("lua_draw_text"): {
                if (numAtoms >= 4) {
                    auto text = AttributedString(atoms[0].toString());
                    float x = atoms[1].getFloat();
                    float y = atoms[2].getFloat();
                    float w = atoms[3].getFloat();
                    float fontHeight = atoms[4].getFloat();
                    
                    text.setFont(Font(fontHeight));
                    text.setColour(currentColour);
                    text.setJustification(Justification::topLeft);
                    
                    TextLayout layout;
                    layout.createLayout(text, w);
                    layout.draw(*graphics, {x, y, w, layout.getHeight()});
                }
                break;
            }
            case hash("lua_fill_path"): {
                if (numAtoms >= 1) {
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    graphics->fillPath(currentPath);
                }
                break;
            }
            case hash("lua_stroke_path"): {
                if (numAtoms >= 2) {
                    auto& currentPath = currentPaths[atoms[0].getSymbol()];
                    graphics->strokePath(currentPath, PathStrokeType(atoms[1].getFloat()));
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
                if (numAtoms >= 2) {
                    float tx = atoms[0].getFloat();
                    float ty = atoms[1].getFloat();
                    graphics->addTransform(AffineTransform::translation(tx, ty));
                }
                break;
            }
            case hash("lua_scale"): {
                if (numAtoms >= 2) {
                    float sx = atoms[0].getFloat();
                    float sy = atoms[1].getFloat();
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
    
    void receiveLuaPaintMessage(t_symbol* sym, int argc, t_atom* argv)
    {
        guiQueue.enqueue({sym, argc, argv});
    }
    
    static void drawCallback(void* target, t_symbol* sym, int argc, t_atom* argv)
    {
        for(auto* object : allDrawTargets[static_cast<t_pdlua*>(target)])
        {
            object->receiveLuaPaintMessage(sym, argc, argv);
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
