/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>

#include "Components/TouchPopupMenu.h"
#include "Buttons.h"
#include "PluginEditor.h"
#include "Objects/ObjectBase.h"

class TouchSelectionHelper final : public Component
    , public NVGComponent {

public:
    explicit TouchSelectionHelper(PluginEditor* e)
        : NVGComponent(this)
        , editor(e)
    {
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::ExportState))); // This icon doubles as a "open" icon in the mobile app
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Help)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Trash)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::More)));

        setCachedComponentImage(new NVGSurface::InvalidationListener(e->nvgSurface, this));

        actionButtons[0]->onClick = [this] {
            auto* cnv = editor->getCurrentCanvas();
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() == 1 && selection[0]->gui) {
                selection[0]->gui->openSubpatch();
            }
        };
        actionButtons[1]->onClick = [this] {
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::ShowHelp);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->invoke(info, true);
        };
        actionButtons[2]->onClick = [this] {
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::Delete);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->invoke(info, true);
        };

        actionButtons[3]->onClick = [this]() {
            auto* cnv = editor->getCurrentCanvas();
            // Info about selection status
            auto selectedObjects = cnv->getSelectionOfType<Object>();
            auto selectedConnections = cnv->getSelectionOfType<Connection>();
            bool const hasSelection = selectedObjects.not_empty();
            bool const multiple = selectedObjects.size() > 1;
            bool const locked = getValue<bool>(cnv->locked);

            auto object = Component::SafePointer<Object>(hasSelection ? selectedObjects.front() : nullptr);

            // Find top-level object, so we never trigger it on an object inside a graph
            if (object && object->findParentComponentOfClass<Object>()) {
                while (auto* nextObject = object->findParentComponentOfClass<Object>()) {
                    object = nextObject;
                }
            }
            
            bool active = object != nullptr && !locked;


            
            TouchPopupMenu touchMenu;
            touchMenu.addItem("Cut", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::Cut);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, active);
            
            touchMenu.addItem("Copy", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::Copy);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, active);
            
            touchMenu.addItem("Paste", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::Paste);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, !locked);
            
            touchMenu.addItem("Duplicate", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::Duplicate);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, active);
            
            touchMenu.addItem("Encapsulate", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::Encapsulate);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, active);
            
            touchMenu.addItem("Tidy Connection", [this](){
                editor->grabKeyboardFocus();
                ApplicationCommandTarget::InvocationInfo info(CommandIDs::ConnectionPathfind);
                info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                editor->commandManager.invoke(info, true);
            }, selectedConnections.size());
            
            TouchPopupMenu alignMenu;
            alignMenu.addItem("Align left", [cnv](){
                cnv->alignObjects(Align::Left);
            }, active);
            alignMenu.addItem("Align centre", [cnv](){
                cnv->alignObjects(Align::HCentre);
            }, active);
            alignMenu.addItem("Align right", [cnv](){
                cnv->alignObjects(Align::Right);
            }, active);
            alignMenu.addItem("Space horizonally", [cnv](){
                cnv->alignObjects(Align::HDistribute);
            }, active);
            alignMenu.addItem("Align top", [cnv](){
                cnv->alignObjects(Align::Top);
            }, active);
            alignMenu.addItem("Align middle", [cnv](){
                cnv->alignObjects(Align::VCentre);
            }, active);
            alignMenu.addItem("Align bottom", [cnv](){
                cnv->alignObjects(Align::Bottom);
            }, active);
            alignMenu.addItem("Space vertically", [cnv](){
                cnv->alignObjects(Align::VDistribute);
            }, active);
      
            TouchPopupMenu orderMenu;
            orderMenu.addItem("To Front", [cnv, selectedObjects]{
                auto objects = cnv->patch.getObjects();
                cnv->patch.startUndoSequence("ToFront");
                for (auto& o : objects) {
                    for (auto* selectedObject : selectedObjects) {
                        if (o == selectedObject->getPointer()) {
                            selectedObject->toFront(false);
                            if (selectedObject->gui)
                                selectedObject->gui->moveToFront();
                        }
                    }
                }
                cnv->patch.endUndoSequence("ToFront");
                cnv->synchronise();
            }, active);
            orderMenu.addItem("Move forward", [cnv, selectedObjects]{
                auto objects = cnv->patch.getObjects();
                cnv->patch.startUndoSequence("MoveForward");
                for (auto& o : objects) {
                    for (auto* selectedObject : selectedObjects) {
                        if (o == selectedObject->getPointer()) {
                            selectedObject->toFront(false);
                            if (selectedObject->gui)
                                selectedObject->gui->moveForward();
                        }
                    }
                }
                cnv->patch.endUndoSequence("MoveForward");
                cnv->synchronise();
            }, active);
            orderMenu.addItem("Move backward", [cnv, selectedObjects]{
                auto objects = cnv->patch.getObjects();

                cnv->patch.startUndoSequence("MoveBackward");
                for (int i = objects.size() - 1; i >= 0; i--) {
                    for (auto* selectedObject : selectedObjects) {
                        if (objects[i] == selectedObject->getPointer()) {
                            selectedObject->toBack();
                            if (selectedObject->gui)
                                selectedObject->gui->moveBackward();
                        }
                    }
                }
                cnv->patch.endUndoSequence("MoveBackward");
                cnv->synchronise();
            }, active);
            orderMenu.addItem("To Back", [cnv, selectedObjects]{
                auto objects = cnv->patch.getObjects();

                cnv->patch.startUndoSequence("ToBack");
                for (int i = objects.size() - 1; i >= 0; i--) {
                    for (auto* selectedObject : selectedObjects) {
                        if (objects[i] == selectedObject->getPointer()) {
                            selectedObject->toBack();
                            if (selectedObject->gui)
                                selectedObject->gui->moveToBack();
                        }
                    }
                }
                cnv->patch.endUndoSequence("ToBack");
                cnv->synchronise();
            }, active);
            
            touchMenu.addSubMenu("Align", alignMenu, active);
            touchMenu.addSubMenu("Order", orderMenu, active);
            
            touchMenu.addItem("Properties", [this](){
                showObjectProperties();
            });
            
            touchMenu.showMenu(editor, actionButtons[3], "Canvas menu");
        };
        
    }
        
    void showObjectProperties()
    {
        auto* cnv = editor->getCurrentCanvas();
        auto objects = cnv->getSelectionOfType<Object>();
        auto toShow = SmallArray<Component*>();

        if (objects.empty()) {
            SmallArray<ObjectParameters, 6> parameters = { cnv->getInspectorParameters() };
            toShow.add(cnv);
            editor->sidebar->forceShowParameters(toShow, parameters);
        } else if (objects.size() == 1) {
            auto* object = objects[0];
            if(object && object->gui) {
                // this makes sure that objects can handle the "properties" message as well if they like, for example for [else/properties]
                if (auto gobj = object->gui->ptr.get<t_gobj>()) {
                    auto const* pdClass = pd_class(&object->getPointer()->g_pd);
                    if (auto const propertiesFn = class_getpropertiesfn(pdClass)) {
                        propertiesFn(gobj.get(), cnv->patch.getRawPointer());
                    }
                }
                
                SmallArray<ObjectParameters, 6> parameters = { object->gui->getParameters() };
                toShow.add(object);
                editor->sidebar->forceShowParameters(toShow, parameters);
            }
        }
    }

    void show()
    {
        actionButtons[1]->setEnabled(editor->isCommandActive(CommandIDs::ShowHelp));
        actionButtons[2]->setEnabled(editor->isCommandActive(CommandIDs::Delete));
        setVisible(true);
        toFront(false);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(5);

        for (auto* button : actionButtons) {
            button->setBounds(b.removeFromLeft(48));
        }
    }

    void render(NVGcontext* nvg) override
    {
        componentImage.renderJUCEComponent(nvg, *this, 2.0f);
    }

private:
    void paint(Graphics& g) override
    {
        auto const b = getLocalBounds().reduced(5);

        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(hash("touch_selection_helper"), g, p, Colour(0, 0, 0).withAlpha(0.4f), 9, { 0, 1 });

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    PluginEditor* editor;
    NVGImage componentImage;
    OwnedArray<MainToolbarButton> actionButtons;
};
