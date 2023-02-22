#pragma once

#include <JuceHeader.h>

template<typename ObjectClass>
class OwnedArrayBroadcaster : public OwnedArray<ObjectClass>
    , public Component
    , public ChangeBroadcaster {
public:
    enum class ChangeType {
        Added,
        Removed,
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
                std::cout << "mousedown OWNED" << std::endl;
                add(obj_);
                attachedToMouse_ = false;
                obj_ = nullptr;
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