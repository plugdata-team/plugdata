#pragma once

// OwnedArray that broadcasts changes
// Used for syncing canvases in splitview
template<typename ObjectClass>
class OwnedArrayBroadcaster : public OwnedArray<ObjectClass>
    , public Component
    , public ComponentListener
    , public ChangeBroadcaster {
public:
    enum class ChangeType {
        Added,
        Removed,
        Changed
    };

    ObjectClass* add(ObjectClass* newObject)
    {
        changeType_ = ChangeType::Added;

        if constexpr (std::is_same_v<ObjectClass, Object>) {
            // newObject is Object
            obj_ = static_cast<Object*>(newObject);

            if (obj_ != nullptr) {
                if (!attachedToMouse_)
                    OwnedArray<Object>::add(obj_);
                if (obj_->attachedToMouse) {
                    attachedToMouse_ = true;
                } else {
                    sendSynchronousChangeMessage();
                }
                // Add listener to update cloned objects if the other changes
                obj_->addComponentListener(this);
            }
            return newObject;
        } else if constexpr (std::is_same_v<ObjectClass, Connection>) {
            // newObject is Connection
            con_ = static_cast<Connection*>(newObject);

            if (con_ != nullptr) {
                OwnedArray<Connection>::add(con_);
                sendSynchronousChangeMessage();
            }
            return newObject;
        }
    }

    ObjectClass* set(int indexToChange, ObjectClass* newObject, bool deleteOldElement = true)
    {
        if constexpr (std::is_same_v<ObjectClass, Object>) {
            obj_ = static_cast<Object*>(newObject);

            OwnedArray<Object>::set(indexToChange, newObject, deleteOldElement);

            // Add listener to update cloned objects if the other changes
            obj_->addComponentListener(this);
        }
        return newObject;
    }

    void remove(int indexToRemove, bool deleteObject = true)
    {
        changeType_ = ChangeType::Removed;
        index_ = indexToRemove;
        OwnedArray<ObjectClass>::remove(indexToRemove, deleteObject);
        sendSynchronousChangeMessage();
    }

    void removeObject(ObjectClass const* objectToRemove, bool deleteObject = true)
    {
        changeType_ = ChangeType::Removed;
        index_ = this->indexOf(objectToRemove);
        OwnedArray<ObjectClass>::removeObject(objectToRemove, deleteObject);
        sendSynchronousChangeMessage();
    }

    void attachCanvas(Component* canvas)
    {
        // Mouse listener, to create objects if they was attached to mouse
        canvas->addMouseListener(this, true);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if constexpr (std::is_same_v<ObjectClass, Object>) {
            if (obj_ && attachedToMouse_) {
                add(obj_);
                attachedToMouse_ = false;
                obj_ = nullptr;
            }
        }
    }

    virtual void componentNameChanged(Component& component) override
    {
        if constexpr (std::is_same_v<ObjectClass, Object>) {

            obj_ = static_cast<Object*>(&component);

            if (obj_ != nullptr) {
                changeType_ = ChangeType::Changed;
                
                index_ = this->indexOf(obj_);

                // Call change Async to make sure pd is synced before updating Object
                sendChangeMessage(); 
            }
        }
    }

    ChangeType changeType_;
    int index_;
    bool attachedToMouse_;
    Object* obj_ = nullptr;
    Connection* con_ = nullptr;
};