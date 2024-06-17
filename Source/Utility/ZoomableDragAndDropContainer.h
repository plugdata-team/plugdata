/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

//==============================================================================
/**
    Enables drag-and-drop behaviour for a component and all its sub-components.

    For a component to be able to make or receive drag-and-drop events, one of its parent
    components must derive from this class. It's probably best for the top-level
    component to implement it.

    Then to start a drag operation, any sub-component can just call the startDragging()
    method, and this object will take over, tracking the mouse and sending appropriate
    callbacks to any child components derived from DragAndDropTarget which the mouse
    moves over.

    Note: If all that you need to do is to respond to files being drag-and-dropped from
    the operating system onto your component, you don't need any of these classes: you can do this
    simply by overriding FileDragAndDropTarget::filesDropped().

    @see DragAndDropTarget

    @tags{GUI}
*/
class Canvas;
class TabComponent;
class ZoomableDragAndDropContainer {
public:
    /** Creates a ZoomableDragAndDropContainer.

        The object that derives from this class must also be a Component.
    */
    ZoomableDragAndDropContainer();

    /** Destructor. */
    virtual ~ZoomableDragAndDropContainer();
    /** Begins a drag-and-drop operation.

        This starts a drag-and-drop operation - call it when the user drags the
        mouse in your drag-source component, and this object will track mouse
        movements until the user lets go of the mouse button, and will send
        appropriate messages to DragAndDropTarget objects that the mouse moves
        over.

        findParentDragContainerFor() is a handy method to call to find the
        drag container to use for a component.

        @param sourceDescription                 a string or value to use as the description of the thing being dragged -
                                                 this will be passed to the objects that might be dropped-onto so they can
                                                 decide whether they want to handle it
        @param sourceComponent                   the component that is being dragged
        @param dragImage                         the image to drag around underneath the mouse. If this is a null image,
                                                 a snapshot of the sourceComponent will be used instead.
        @param allowDraggingToOtherJuceWindows   if true, the dragged component will appear as a desktop
                                                 window, and can be dragged to DragAndDropTargets that are the
                                                 children of components other than this one.
        @param imageOffsetFromMouse              if an image has been passed-in, this specifies the offset
                                                 at which the image should be drawn from the mouse. If it isn't
                                                 specified, then the image will be centred around the mouse. If
                                                 an image hasn't been passed-in, this will be ignored.
        @param inputSourceCausingDrag            the mouse input source which started the drag. When calling
                                                 from within a mouseDown or mouseDrag event, you can pass
                                                 MouseEvent::source to this method. If this param is nullptr then JUCE
                                                 will use the mouse input source which is currently dragging. If there
                                                 are several dragging mouse input sources (which can often occur on mobile)
                                                 then JUCE will use the mouseInputSource which is closest to the sourceComponent.
    */
    void startDragging(var const& sourceDescription,
        Component* sourceComponent,
        ScaledImage const& dragImage = ScaledImage(),
        ScaledImage const& dragInvalidImage = ScaledImage(),
        bool allowDraggingToOtherJuceWindows = false,
        Point<int> const* imageOffsetFromMouse = nullptr,
        MouseInputSource const* inputSourceCausingDrag = nullptr,
        bool canZoom = false);

    [[deprecated("This overload does not allow the image's scale to be specified. Use the other overload of startDragging instead.")]] void startDragging(var const& sourceDescription,
        Component* sourceComponent,
        Image dragImage,
        Image dragInvalidImage,
        bool allowDraggingToOtherJuceWindows = false,
        Point<int> const* imageOffsetFromMouse = nullptr,
        MouseInputSource const* inputSourceCausingDrag = nullptr,
        bool canZoom = false)
    {
        startDragging(sourceDescription,
            sourceComponent,
            ScaledImage(dragImage),
            ScaledImage(dragInvalidImage),
            allowDraggingToOtherJuceWindows,
            imageOffsetFromMouse,
            inputSourceCausingDrag,
            canZoom);
    }

    /** Returns true if something is currently being dragged. */
    bool isDragAndDropActive() const;

    static ZoomableDragAndDropContainer* findParentDragContainerFor(Component* childComponent);

    virtual TabComponent& getTabComponent() = 0;

protected:
    /** Override this if you want to be able to perform an external drag of a set of files
        when the user drags outside of this container component.

        This method will be called when a drag operation moves outside the JUCE window,
        and if you want it to then perform a file drag-and-drop, add the filenames you want
        to the array passed in, and return true.

        @param sourceDetails    information about the source of the drag operation
        @param files            on return, the filenames you want to drag
        @param canMoveFiles     on return, true if it's ok for the receiver to move the files; false if
                                it must make a copy of them (see the performExternalDragDropOfFiles() method)
        @see performExternalDragDropOfFiles, shouldDropTextWhenDraggedExternally
    */
    virtual bool shouldDropFilesWhenDraggedExternally(DragAndDropTarget::SourceDetails const& sourceDetails,
        StringArray& files, bool& canMoveFiles);

    /** Override this if you want to be able to perform an external drag of text
        when the user drags outside of this container component.

        This method will be called when a drag operation moves outside the JUCE window,
        and if you want it to then perform a text drag-and-drop, copy the text you want to
        be dragged into the argument provided and return true.

        @param sourceDetails    information about the source of the drag operation
        @param text             on return, the text you want to drag
        @see shouldDropFilesWhenDraggedExternally
    */
    virtual bool shouldDropTextWhenDraggedExternally(DragAndDropTarget::SourceDetails const& sourceDetails,
        String& text);

    /** Subclasses can override this to be told when a drag starts. */
    virtual void dragOperationStarted(DragAndDropTarget::SourceDetails const&);

    /** Subclasses can override this to be told when a drag finishes. */
    virtual void dragOperationEnded(DragAndDropTarget::SourceDetails const&);

    virtual DragAndDropTarget* findNextDragAndDropTarget(Point<int> screenPos);

private:
    class DragImageComponent;
    OwnedArray<DragImageComponent> dragImageComponents;

    float zoomScale;

    MouseInputSource const* getMouseInputSourceForDrag(Component* sourceComponent, MouseInputSource const* inputSourceCausingDrag);
    bool isAlreadyDragging(Component* sourceComponent) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZoomableDragAndDropContainer)
};
