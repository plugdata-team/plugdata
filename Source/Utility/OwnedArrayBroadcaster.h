#pragma once

#include <JuceHeader.h>

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
        // Check if ObjectClass is the same as Object or Connection
        if constexpr (std::is_same_v<ObjectClass, Object>) {
            std::cout << "ObjectClass is Object*" << std::endl;
            obj_ = static_cast<Object*>(newObject);
            if (obj_ != nullptr) {
                if (!attachedToMouse_)
                    OwnedArray<Object>::add(obj_);
                if (obj_->attachedToMouse) {
                    attachedToMouse_ = true;
                } else {
                    sendSynchronousChangeMessage();
                }
                obj_->addComponentListener(this);
            }
            return newObject;
        } else if constexpr (std::is_same_v<ObjectClass, Connection>) {
            std::cout << "ObjectClass is Connection*" << std::endl;
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
            obj_->addComponentListener(this);
            OwnedArray<Object>::set(indexToChange, newObject, deleteOldElement);
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
        cnv_ = canvas;
        cnv_->addMouseListener(this, true);
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
                sendChangeMessage(); // Called Async to make sure pd is synced before updating Object
            }
        }
    }

    Component* cnv_;
    ChangeType changeType_;
    int index_;
    bool attachedToMouse_;
    Object* obj_ = nullptr;
    Connection* con_ = nullptr;
};