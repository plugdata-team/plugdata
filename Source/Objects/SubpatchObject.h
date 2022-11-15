/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct SubpatchObject final : public TextBase
, public Value::Listener {
    SubpatchObject(void* obj, Object* object)
    : TextBase(obj, object)
    , subpatch({ ptr, cnv->pd })
    {
        isGraphChild = false;
        hideNameAndArgs = static_cast<bool>(subpatch.getPointer()->gl_hidetext);
        
        isGraphChild.addListener(this);
        hideNameAndArgs.addListener(this);
        
        object->hvccMode.addListener(this);
        
        if(static_cast<bool>(object->hvccMode.getValue())) {
            checkHvccCompatibility(cnv->pd, subpatch);
        }
    }
    
    ~SubpatchObject()
    {
        object->hvccMode.removeListener(this);
        closeOpenedSubpatchers();
    }
    
    void updateValue() override
    {
        // Change from subpatch to graph
        if (static_cast<t_canvas*>(ptr)->gl_isgraph) {
            cnv->setSelected(object, false);
            object->cnv->main.sidebar.hideParameters();
            object->setType(objectText, ptr);
        }
    };
    
    void mouseDown(MouseEvent const& e) override
    {
        //  If locked and it's a left click
        if (locked && !ModifierKeys::getCurrentModifiers().isRightButtonDown()) {
            openSubpatch();
            
            return;
        } else {
            TextBase::mouseDown(e);
        }
    }
    
    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    void lock(bool isLocked) override
    {
        locked = isLocked;
    }
    
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    ObjectParameters getParameters() override
    {
        return { { "Is graph", tBool, cGeneral, &isGraphChild, { "No", "Yes" } }, { "Hide name and arguments", tBool, cGeneral, &hideNameAndArgs, { "No", "Yes" } } };
    };
    
    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(isGraphChild)) {
            subpatch.getPointer()->gl_isgraph = static_cast<bool>(isGraphChild.getValue());
            updateValue();
        } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
            subpatch.getPointer()->gl_hidetext = static_cast<bool>(hideNameAndArgs.getValue());
            repaint();
        }
        else if (v.refersToSameSourceAs(object->hvccMode)) {
            if(static_cast<bool>(v.getValue())) {
                checkHvccCompatibility(cnv->pd, subpatch);
            }
        }
    }
    
    bool canOpenFromMenu() override
    {
        return true;
    }
    
    void openFromMenu() override
    {
        openSubpatch();
    }
    
    static void checkHvccCompatibility(pd::Instance* instance, pd::Patch& patch) {
        for(auto* object : patch.getObjects()) {
            const String name = libpd_get_object_class_name(object);
            
            if(name == "canvas" || name == "graph") {
                auto patch = pd::Patch(object, instance);
                checkHvccCompatibility(instance, patch);
            }
            else if(!Object::hvccObjects.contains(name)) {
                instance->logError(String("Warning: object \"" + name + "\" is not supported in Compiled Mode").toRawUTF8());
            }
        }
    }
    
protected:
    pd::Patch subpatch;
    Value isGraphChild = Value(var(false));
    Value hideNameAndArgs = Value(var(false));
    
    bool locked = false;
};
