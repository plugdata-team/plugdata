/*
 // Copyright (c) 2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class DropzoneObject final : public ObjectBase, public FileDragAndDropTarget, public TextDragAndDropTarget {
    bool isDraggingOver = false;
    Point<int> lastPosition;
    Value sizeProperty = SynchronousValue();

public:
    DropzoneObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        objectParameters.addParamSize(&sizeProperty);
    }

    ~DropzoneObject() override = default;

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto fillColour = Colours::transparentBlack;
        auto outlineColour = cnv->editor->getLookAndFeel().findColour(object->isSelected() && !cnv->isGraph ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::outlineColourId);
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(fillColour), convertColour(outlineColour), Corners::objectCornerRadius);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patch, gobj.get(), b.getX(), b.getY());
            
            t_atom atoms[2];
            SETFLOAT(atoms, b.getWidth() - 1);
            SETFLOAT(atoms + 1, b.getHeight() - 1);
            pd_typedmess(gobj.cast<t_pd>(), pd->generateSymbol("dim"), 2, atoms);
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void update() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();
            
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            sizeProperty = VarArray { var(w), var(h) };
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();
            
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            sizeProperty = VarArray { var(w), var(h) };
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
            
            t_atom atoms[2];
            SETFLOAT(atoms, width);
            SETFLOAT(atoms + 1, height);
            
            if (auto gobj = ptr.get<t_gobj>()) {
                pd_typedmess(gobj.cast<t_pd>(), pd->generateSymbol("dim"), 2, atoms);
            }

            object->updateBounds();
        }
    }

    // Check if top-level canvas is locked to determine if we should respond to mouse events
    bool isLocked() const
    {
        // Find top-level canvas
        auto const* topLevel = findParentComponentOfClass<Canvas>();
        while (auto const* nextCanvas = topLevel->findParentComponentOfClass<Canvas>()) {
            topLevel = nextCanvas;
        }

        return getValue<bool>(topLevel->locked) || getValue<bool>(topLevel->commandLocked) || topLevel->isGraph;
    }
    
    bool isInterestedInFileDrag(const StringArray& files) override {
        return true;
    };

    void fileDragEnter(const StringArray& files, int x, int y) override {
        isDraggingOver = true;
    };

    void fileDragMove(const StringArray& files, int x, int y) override {
        auto bounds = getPdBounds();
        auto* patch = cnv->patch.getRawPointer();
        char cnvName[32];
        snprintf(cnvName, 32, ".x%lx", glist_getcanvas(patch));
        pd->sendMessage("__else_dnd_rcv", "_drag_over", { pd->generateSymbol(cnvName), x + bounds.getX(), y + bounds.getY() });
    };

    void fileDragExit(const StringArray& files) override {
        pd->sendMessage("__else_dnd_rcv", "_drag_leave", {});
        isDraggingOver = false;
    };

    void filesDropped(const StringArray& files, int x, int y) override {
        auto bounds = getPdBounds();
        for(auto& file : files)
        {
            auto* patch = cnv->patch.getRawPointer();
            char cnvName[32];
            snprintf(cnvName, 32, ".x%lx", (uint64_t)glist_getcanvas(patch));
            pd->sendMessage("__else_dnd_rcv", "_drag_drop", { pd->generateSymbol(cnvName), 0.0f, pd->generateSymbol(file.replace("\\", "/")) });
        }
    };
    
    bool isInterestedInTextDrag(const String& text) override {
        return true;
    }
    
    void textDragEnter(const String& text, int x, int y) override {
        isDraggingOver = true;
    }

    void textDragMove(const String& text, int x, int y) override {
        auto bounds = getPdBounds();
        auto* patch = cnv->patch.getRawPointer();
        char cnvName[32];
        snprintf(cnvName, 32, ".x%lx", (uint64_t)glist_getcanvas(patch));
        pd->sendMessage("__else_dnd_rcv", "_drag_over", { pd->generateSymbol(cnvName), x + bounds.getX(), y + bounds.getY() });
    }

    void textDragExit(const String& text) override {
        pd->sendMessage("__else_dnd_rcv", "_drag_leave", {});
        isDraggingOver = false;
    }

    void textDropped (const String& text, int x, int y) override {
        auto bounds = getPdBounds();
        auto* patch = cnv->patch.getRawPointer();
        char cnvName[32];
        snprintf(cnvName, 32, ".x%lx", (uint64_t)glist_getcanvas(patch));
        pd->sendMessage("__else_dnd_rcv", "_drag_drop", { pd->generateSymbol(cnvName), 1.0f, pd->generateSymbol(text) });
    }
};
