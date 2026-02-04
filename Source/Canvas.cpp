/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Sidebar/Sidebar.h"
#include "Statusbar.h"
#include "Canvas.h"
#include "Object.h"
#include "Connection.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Components/SuggestionComponent.h"
#include "CanvasViewport.h"

#include "Objects/ObjectBase.h"

#include "Dialogs/Dialogs.h"
#include "Components/GraphArea.h"
#include "Components/CanvasBorderResizer.h"

#include "Components/CanvasSearchHighlight.h"

extern "C" {
void canvas_setgraph(t_glist* x, int flag, int nogoprect);
}

class ObjectsResizer final : public Component
    , NVGComponent
    , Value::Listener {
public:
    enum class ResizerMode { Horizontal,
        Vertical };

    ObjectsResizer(Canvas* parentCanvas, std::function<float(Rectangle<int> bounds)> onResize, std::function<void(Point<int> pos)> onMove, ResizerMode const mode = ResizerMode::Horizontal)
        : NVGComponent(this)
        , border(this, &constrainer)
        , cnv(parentCanvas)
        , mode(mode)
        , onResize(std::move(onResize))
        , onMove(std::move(onMove))
    {
        cnv->addAndMakeVisible(this);
        setAlwaysOnTop(true);

        cnv->zoomScale.addListener(this);

        auto selectedObjectBounds = Rectangle<int>();

        auto smallestObjectWidthOrHeight = std::numeric_limits<int>::max();
        auto largestObjectWidthOrHeight = 0;

        // work out the bounds for all objects, and also the bounds for the largest object (used for min size of constrainer)
        for (auto* obj : cnv->getSelectionOfType<Object>()) {
            obj->hideHandles(true);

            selectedObjectBounds = selectedObjectBounds.getUnion(obj->getBounds().reduced(Object::margin, Object::margin));

            // Find the smallest object to make the min constrainer size
            if (mode == ResizerMode::Horizontal) {
                auto const objWidth = obj->getObjectBounds().getWidth();
                if (objWidth < smallestObjectWidthOrHeight) {
                    smallestObjectWidthOrHeight = objWidth;
                }
                if (objWidth > largestObjectWidthOrHeight) {
                    largestObjectWidthOrHeight = objWidth;
                }
            } else {
                auto const objHeight = obj->getObjectBounds().getHeight();
                if (objHeight < smallestObjectWidthOrHeight) {
                    smallestObjectWidthOrHeight = objHeight;
                }
                if (objHeight > largestObjectWidthOrHeight) {
                    largestObjectWidthOrHeight = objHeight;
                }
            }
        }

        setBorderScale(cnv->zoomScale);

        if (mode == ResizerMode::Horizontal) {
            constrainer.setMinimumWidth(largestObjectWidthOrHeight + tabMargin * 2);
        } else {
            constrainer.setMinimumHeight(largestObjectWidthOrHeight + tabMargin * 2);
        }

        addAndMakeVisible(border);

        setBounds(selectedObjectBounds.expanded(tabMargin));
    }

    ~ObjectsResizer() override
    {
        for (auto* obj : cnv->getSelectionOfType<Object>()) {
            obj->hideHandles(false);
        }

        if (cnv) {
            cnv->zoomScale.removeListener(this);
        }
    }

    void setBorderScale(Value const& canvasScale)
    {
        auto const scale = getValue<float>(canvasScale);
        auto const borderSize = std::max(12.0f, 12 / scale);
        if (mode == ResizerMode::Horizontal) {
            border.setBorderThickness(juce::BorderSize<int>(0, borderSize, 0, borderSize));
        } else {
            border.setBorderThickness(juce::BorderSize<int>(borderSize, 0, borderSize, 0));
        }
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(cnv->zoomScale)) {
            setBorderScale(cnv->zoomScale);
        }
    }

    bool hitTest(int x, int y) override
    {
        if (cnv->panningModifierDown() || ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown())
            return false;

        return true;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.mods.isLeftButtonDown())
            originalPos = getPosition();

        // We can allow launching the right-click menu, however we would need to turn off alignment etc...
        // if (e.mods.isRightButtonDown()){
        //    cnv->objectLayer.getComponentAt(e.getEventRelativeTo(cnv).getPosition())->mouseDown(e);
        //}
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.originalComponent == &border)
            return;
        if (e.mods.isLeftButtonDown()) {
            auto const delta = e.getPosition() - e.getMouseDownPosition();
            auto const newPos = originalPos + delta;
            setTopLeftPosition(newPos);
        }
    }

    void moved() override
    {
        onMove(getPosition());
    }

    void resized() override
    {
        border.setBounds(getLocalBounds().expanded(tabMargin));
        spacer = onResize(getBounds().reduced(tabMargin));
    }

    void render(NVGcontext* nvg) override
    {
        // draw background and outline
        auto const b = getBounds().reduced(tabMargin);
        auto iCol = cnv->selectedOutlineCol;

        iCol.a = 5; // Make the inner colour semi-transparent
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), iCol, cnv->selectedOutlineCol, Corners::objectCornerRadius);

        // Draw handles at edge
        auto getCorners = [this] {
            auto const rect = getBounds().reduced(tabMargin);
            constexpr float offset = 2.0f;

            Array<Rectangle<float>> corners = { Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopLeft().toFloat()).translated(offset, offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomLeft().toFloat()).translated(offset, -offset),
                Rectangle<float>(9.0f, 9.0f).withCentre(rect.getBottomRight().toFloat()).translated(-offset, -offset), Rectangle<float>(9.0f, 9.0f).withCentre(rect.getTopRight().toFloat()).translated(-offset, offset) };

            return corners;
        };

        auto& resizeHandleImage = cnv->resizeHandleImage;
        int angle = 360;
        for (auto& corner : getCorners()) {
            NVGScopedState scopedState(nvg);
            // Rotate around centre
            nvgTranslate(nvg, corner.getCentreX(), corner.getCentreY());
            nvgRotate(nvg, degreesToRadians<float>(angle));
            nvgTranslate(nvg, -4.5f, -4.5f);

            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, 9, 9);
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, 0, 0, 9, 9, 0, resizeHandleImage.getImageId(), cnv->selectedOutlineCol));
            nvgFill(nvg);
            angle -= 90;
        }
// #define SPACER_TEXT
#ifdef SPACER_TEXT
        nvgBeginPath(nvg);
        auto textPos = getPosition().translated(0, -25);
        nvgFontSize(nvg, 20.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(nvg, nvgRGBA(240, 240, 240, 255));
        nvgText(nvg, textPos.x, textPos.y, String("Spacer size: " + String(spacer + 1.0f)).toRawUTF8(), nullptr);
#endif
    }

private:
    ResizableBorderComponent border;
    ComponentBoundsConstrainer constrainer;
    Canvas* cnv;
    int tabMargin = Object::margin;

    Point<int> originalPos;
    float spacer = 0;

    ResizerMode mode;

    std::function<float(Rectangle<int>)> onResize;
    std::function<void(Point<int>)> onMove;
};

Canvas::Canvas(PluginEditor* parent, pd::Patch::Ptr p, Component* parentGraph)
    : NVGComponent(this)
    , editor(parent)
    , pd(parent->pd)
    , refCountedPatch(p)
    , patch(*p)
    , canvasOrigin(Point<int>(infiniteCanvasSize / 2, infiniteCanvasSize / 2))
    , graphArea(nullptr)
    , pathUpdater(new ConnectionPathUpdater(this))
    , globalMouseListener(this)
{
    selectedComponents.addChangeListener(this);

    addAndMakeVisible(objectLayer);
    addAndMakeVisible(connectionLayer);

    objectLayer.setInterceptsMouseClicks(false, true);
    connectionLayer.setInterceptsMouseClicks(false, true);

    if (auto patchPtr = patch.getPointer()) {
        isGraphChild = glist_isgraph(patchPtr.get());
        hideNameAndArgs = static_cast<bool>(patchPtr->gl_hidetext);
        xRange = VarArray { var(patchPtr->gl_x1), var(patchPtr->gl_x2) };
        yRange = VarArray { var(patchPtr->gl_y2), var(patchPtr->gl_y1) };
    }

    pd->registerMessageListener(patch.getUncheckedPointer(), this);

    isGraphChild.addListener(this);
    hideNameAndArgs.addListener(this);
    xRange.addListener(this);
    yRange.addListener(this);

    auto const patchBounds = patch.getBounds();
    patchWidth = patchBounds.getWidth();
    patchHeight = patchBounds.getHeight();

    patchWidth.addListener(this);
    patchHeight.addListener(this);

    globalMouseListener.globalMouseMove = [this](MouseEvent const& e) {
        lastMouseX = e.x;
        lastMouseY = e.y;
    };
    globalMouseListener.globalMouseDrag = [this](MouseEvent const& e) {
        lastMouseX = e.x;
        lastMouseY = e.y;
    };

    suggestor = std::make_unique<SuggestionComponent>();
    canvasBorderResizer = std::make_unique<BorderResizer>(this);
    canvasBorderResizer->onDrag = [this] {
        patchWidth = std::max(0, canvasBorderResizer->getBounds().getCentreX() - canvasOrigin.x);
        patchHeight = std::max(0, canvasBorderResizer->getBounds().getCentreY() - canvasOrigin.y);
    };

    canvasBorderResizer->setCentrePosition(canvasOrigin.x + patchBounds.getWidth(), canvasOrigin.y + patchBounds.getHeight());
    addAndMakeVisible(canvasBorderResizer.get());

    // Check if canvas belongs to a graph
    if (parentGraph) {
        setLookAndFeel(&editor->getLookAndFeel());
        parentGraph->addAndMakeVisible(this);
        setInterceptsMouseClicks(false, true);
        isGraph = true;
    } else {
        isGraph = false;
    }
    if (!isGraph) {
        auto* canvasViewport = new CanvasViewport(editor, this);

        canvasViewport->setViewedComponent(this, false);

        canvasViewport->onScroll = [this] {
            if (suggestor) {
                suggestor->updateBounds();
            }
            if (graphArea) {
                graphArea->updateBounds();
            }
        };

        canvasViewport->setScrollBarsShown(true, true, true, true);

        viewport.reset(canvasViewport); // Owned by the tabbar, but doesn't exist for graph!
        restoreViewportState();
    }

    commandLocked.referTo(pd->commandLocked);
    commandLocked.addListener(this);

    // pd->commandLocked doesn't get updated when a canvas isn't active
    // So we set it to false here when a canvas is remade
    // Otherwise the last canvas could have set it true, and it would still be
    // in that state without command actually being locked
    if (!isGraph)
        commandLocked.setValue(false);

    // init border for testing
    settingsChanged("border", SettingsFile::getInstance()->getPropertyAsValue("border"));

    // Add draggable border for setting graph position
    if (getValue<bool>(isGraphChild) && !isGraph) {
        graphArea = std::make_unique<GraphArea>(this);
        addAndMakeVisible(*graphArea);
        graphArea->setAlwaysOnTop(true);
    }

    if (!isGraph) {
        editor->nvgSurface.addBufferedObject(this);
    }

    setSize(infiniteCanvasSize, infiniteCanvasSize);

    // initialize to default zoom
    auto const defaultZoom = SettingsFile::getInstance()->getPropertyAsValue("default_zoom");
    zoomScale.setValue(getValue<float>(defaultZoom) / 100.0f);
    zoomScale.addListener(this);

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);

    setWantsKeyboardFocus(true);

    if (!isGraph) {
        presentationMode.addListener(this);
    } else {
        presentationMode = false;
    }
    performSynchronise();

    // Start in unlocked mode if the patch is empty
    if (objects.empty()) {
        locked = false;
        if (auto patchPtr = patch.getPointer())
            patchPtr->gl_edit = false;
    } else {
        if (auto patchPtr = patch.getPointer())
            locked = !patchPtr->gl_edit;
    }

    locked.addListener(this);

    editor->addModifierKeyListener(this);

    updateOverlays();
    orderConnections();

    parameters.addParamBool("Is graph", cGeneral, &isGraphChild, { "No", "Yes" }, 0);
    parameters.addParamBool("Hide name and arguments", cGeneral, &hideNameAndArgs, { "No", "Yes" }, 0);
    parameters.addParamRange("X range", cGeneral, &xRange, { 0.0f, 1.0f });
    parameters.addParamRange("Y range", cGeneral, &yRange, { 1.0f, 0.0f });

    auto onInteractionFn = [this](bool const state) {
        dimensionsAreBeingEdited = state;
        repaint();
    };

    parameters.addParamInt("Width", cDimensions, &patchWidth, 527, true, 0, 1 << 30, onInteractionFn);
    parameters.addParamInt("Height", cDimensions, &patchHeight, 327, true, 0, 1 << 30, onInteractionFn);

    if (!isGraph) {
        patch.setVisible(true);
    }

    lookAndFeelChanged();
}

Canvas::~Canvas()
{
    for (auto* object : objects) {
        object->hideEditor();
    }

    if (!isGraph) {
        editor->nvgSurface.removeBufferedObject(this);
    }

    saveViewportState();
    zoomScale.removeListener(this);
    editor->removeModifierKeyListener(this);
    pd->unregisterMessageListener(this);
    patch.setVisible(false);
    selectedComponents.removeChangeListener(this);
}

void Canvas::changeListenerCallback(ChangeBroadcaster* c)
{
    if (c == &selectedComponents) {
        auto isSelectedDifferent = [](SelectedItemSet<WeakReference<Component>> const& set1, SelectedItemSet<WeakReference<Component>> const& set2) -> bool {
            if (set1.getNumSelected() != set2.getNumSelected())
                return true;
            for (int i = 0; i < set1.getNumSelected(); i++) {
                if (!set2.isSelected(set1.getSelectedItem(i)))
                    return true;
            }
            return false; // identical
        };

        if (isSelectedDifferent(selectedComponents, previousSelectedComponents)) {
            previousSelectedComponents = selectedComponents;
            editor->updateSelection(this);
        }
    }
}

void Canvas::lookAndFeelChanged()
{
    // Canvas colours
    auto const& lnf = editor->getLookAndFeel();
    canvasBackgroundColJuce = lnf.findColour(PlugDataColour::canvasBackgroundColourId);
    canvasBackgroundCol = convertColour(canvasBackgroundColJuce);
    canvasMarkingsColJuce = lnf.findColour(PlugDataColour::canvasDotsColourId).interpolatedWith(canvasBackgroundColJuce, 0.2f);
    canvasMarkingsCol = convertColour(canvasMarkingsColJuce);
    canvasTextColJuce = lnf.findColour(PlugDataColour::canvasTextColourId);

    // Object colours
    objectOutlineCol = convertColour(lnf.findColour(PlugDataColour::objectOutlineColourId));
    outlineCol = convertColour(lnf.findColour(PlugDataColour::outlineColourId));
    textObjectBackgroundCol = convertColour(lnf.findColour(PlugDataColour::textObjectBackgroundColourId));
    ioletLockedCol = convertColour(canvasBackgroundColJuce.contrasting(0.5f));

    commentTextCol = convertColour(lnf.findColour(PlugDataColour::commentTextColourId));

    guiObjectInternalOutlineColJuce = lnf.findColour(PlugDataColour::guiObjectInternalOutlineColour);
    guiObjectInternalOutlineCol = convertColour(guiObjectInternalOutlineColJuce);
    guiObjectBackgroundColJuce = lnf.findColour(PlugDataColour::guiObjectBackgroundColourId);
    guiObjectBackgroundCol = convertColour(guiObjectBackgroundColJuce);

    auto const selectedColJuce = lnf.findColour(PlugDataColour::objectSelectedOutlineColourId);
    selectedOutlineCol = convertColour(selectedColJuce);
    transparentObjectBackgroundCol = convertColour(canvasBackgroundColJuce.contrasting(0.35f).withAlpha(0.1f));
    indexTextCol = convertColour(selectedColJuce.contrasting());

    graphAreaCol = convertColour(lnf.findColour(PlugDataColour::graphAreaColourId));

    // Lasso colours
    lassoCol = convertColour(selectedColJuce.withAlpha(0.075f));
    lassoOutlineCol = convertColour(canvasBackgroundColJuce.interpolatedWith(selectedColJuce, 0.65f));

    // Presentation mode colors
    auto const presentationBackgroundColJuce = lnf.findColour(PlugDataColour::presentationBackgroundColourId);
    presentationBackgroundCol = convertColour(presentationBackgroundColJuce);
    presentationWindowOutlineCol = convertColour(presentationBackgroundColJuce.contrasting(0.3f));

    // Connection / Iolet colours
    auto const dataColJuce = lnf.findColour(PlugDataColour::dataColourId);
    dataCol = convertColour(dataColJuce);
    auto const sigColJuce = lnf.findColour(PlugDataColour::signalColourId);
    sigCol = convertColour(sigColJuce);
    auto const gemColJuce = lnf.findColour(PlugDataColour::gemColourId);
    gemCol = convertColour(gemColJuce);
    auto const baseColJuce = lnf.findColour(PlugDataColour::connectionColourId);
    baseCol = convertColour(baseColJuce);

    dataColBrighter = convertColour(dataColJuce.brighter());
    sigColBrighter = convertColour(sigColJuce.brighter());
    gemColBrigher = convertColour(gemColJuce.brighter());
    baseColBrigher = convertColour(baseColJuce.brighter());

    dotsLargeImage.setDirty(); // Make sure bg colour actually gets updated
}

void Canvas::parentHierarchyChanged()
{
    // If the canvas has been added back into the editor, update the look and feel
    // We need to do this because canvases are removed from the parent hierarchy when not visible
    // TODO: consider setting a flag when look and feel actually changes, and read that here
    if (getParentComponent()) {
        sendLookAndFeelChange();
    }
}

void Canvas::updateFramebuffers(NVGcontext* nvg)
{
    auto const pixelScale = editor->getRenderScale();
    auto zoom = getValue<float>(zoomScale);

    constexpr int resizerLogicalSize = 9;
    float const viewScale = pixelScale * zoom;
    int const resizerBufferSize = resizerLogicalSize * viewScale;

    if (resizeHandleImage.needsUpdate(resizerBufferSize, resizerBufferSize)) {
        resizeHandleImage = NVGImage(nvg, resizerBufferSize, resizerBufferSize, [viewScale](Graphics& g) {
            g.addTransform(AffineTransform::scale(viewScale, viewScale));
            auto const b = Rectangle<int>(0, 0, 9, 9);
            // use the path with a hole in it to exclude the inner rounded rect from painting
            Path outerArea;
            outerArea.addRectangle(b);
            outerArea.setUsingNonZeroWinding(false);

            Path innerArea;
            auto const innerRect = b.translated(Object::margin / 2, Object::margin / 2);
            innerArea.addRoundedRectangle(innerRect, Corners::objectCornerRadius);
            outerArea.addPath(innerArea);
            g.reduceClipRegion(outerArea);

            g.setColour(Colours::white); // For alpha image colour isn't important
            g.fillRoundedRectangle(0.0f, 0.0f, 9.0f, 9.0f, Corners::resizeHanleCornerRadius); }, NVGImage::AlphaImage);
    }

    auto gridLogicalSize = objectGrid.gridSize ? objectGrid.gridSize : 25;
    auto gridSizeCommon = 300;
    auto const gridBufferSize = gridSizeCommon * pixelScale * zoom;

    if (dotsLargeImage.needsUpdate(gridBufferSize, gridBufferSize) || lastObjectGridSize != gridLogicalSize) {
        lastObjectGridSize = gridLogicalSize;

        dotsLargeImage = NVGImage(nvg, gridBufferSize, gridBufferSize, [this, zoom, viewScale, gridLogicalSize, gridSizeCommon](Graphics& g) {
            g.addTransform(AffineTransform::scale(viewScale, viewScale));
            float const ellipseRadius = zoom < 1.0f ? jmap(zoom, 0.25f, 1.0f, 3.0f, 1.0f) : 1.0f;

            int decim = 0;
            switch (gridLogicalSize) {
            case 5:
            case 10:
                if (zoom < 1.0f)
                    decim = 4;
                if (zoom < 0.5f)
                    decim = 6;
                break;
            case 15:
                if (zoom < 1.0f)
                    decim = 4;
                if (zoom < 0.5f)
                    decim = 8;
                break;
            case 20:
            case 25:
                if (zoom < 1.0f)
                    decim = 3;
                if (zoom < 0.5f)
                    decim = 6;
                break;
            case 30:
                if (zoom < 1.0f)
                    decim = 12;
                if (zoom < 0.5f)
                    decim = 12;
                break;
            default: break;
            }

            auto const majorDotColour = canvasMarkingsColJuce.withAlpha(std::min(zoom * 0.8f, 1.0f));

            g.setColour(majorDotColour);
            // Draw ellipses on the grid
            for (int x = 0; x <= gridSizeCommon; x += gridLogicalSize)
            {
                for (int y = 0; y <= gridSizeCommon; y += gridLogicalSize)
                {
                    if (decim != 0) {
                        if (x % decim && y % decim)
                            continue;
                        g.setColour(majorDotColour);
                        if (x % decim == 0 && y % decim == 0)
                            g.setColour(canvasMarkingsColJuce);
                    }
                    // Add half smallest dot offset so the dot isn't at the edge of the texture
                    // We remove this when we position the texture on the canvas
                    float const centerX = static_cast<float>(x) + 2.5f;
                    float const centerY = static_cast<float>(y) + 2.5f;
                    g.fillEllipse(centerX - ellipseRadius, centerY - ellipseRadius, ellipseRadius * 2.0f, ellipseRadius * 2.0f);
                }
            } }, NVGImage::RepeatImage, canvasBackgroundColJuce);
    }
}

// Callback from canvasViewport to perform actual rendering
void Canvas::performRender(NVGcontext* nvg, Rectangle<int> invalidRegion)
{    
    constexpr auto halfSize = infiniteCanvasSize / 2;
    auto const zoom = getValue<float>(zoomScale);
    bool const isLocked = getValue<bool>(locked);
    nvgSave(nvg);

    if (viewport) {
        nvgTranslate(nvg, -viewport->getViewPositionX(), -viewport->getViewPositionY());
        nvgScale(nvg, zoom, zoom);
        invalidRegion = invalidRegion.translated(viewport->getViewPositionX(), viewport->getViewPositionY());
        invalidRegion /= zoom;

        if (isLocked) {
            nvgFillColor(nvg, canvasBackgroundCol);
            nvgFillRect(nvg, invalidRegion.getX(), invalidRegion.getY(), invalidRegion.getWidth(), invalidRegion.getHeight());
        } else {
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, infiniteCanvasSize, infiniteCanvasSize);

            // Use least common multiple of grid sizes: 5,10,15,20,25,30 for texture size for now
            // We repeat the texture on GPU, this is so the texture does not become too small for GPU processing
            // There will be a best fit depending on CPU/GPU calcuations.
            // But currently 300 works well on GPU.
            {
                constexpr auto gridSizeCommon = 300;
                NVGScopedState scopedState(nvg);
                // offset image texture by 2.5f so no dots are on the edge of the texture
                nvgTranslate(nvg, canvasOrigin.x - 2.5f, canvasOrigin.x - 2.5f);

                nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, gridSizeCommon, gridSizeCommon, 0, dotsLargeImage.getImageId(), 1));
                nvgFill(nvg);
            }
        }
    }

    currentRenderArea = invalidRegion;

    auto drawBorder = [this, nvg, zoom](bool const bg, bool const fg) {
        if (viewport && (showOrigin || showBorder) && !::getValue<bool>(presentationMode)) {
            NVGScopedState scopedState(nvg);
            nvgBeginPath(nvg);

            auto const borderWidth = getValue<float>(patchWidth);
            auto const borderHeight = getValue<float>(patchHeight);
            constexpr auto pos = Point<int>(halfSize, halfSize);

            auto scaledStrokeSize = zoom < 1.0f ? jmap(zoom, 1.0f, 0.25f, 1.5f, 4.0f) : 1.5f;
            if (zoom < 0.3f && editor->getRenderScale() <= 1.0f)
                scaledStrokeSize = jmap(zoom, 0.3f, 0.25f, 4.0f, 8.0f);

            if (bg) {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, pos.x, pos.y);
                nvgLineTo(nvg, pos.x, pos.y + (showOrigin ? halfSize : borderHeight));
                nvgMoveTo(nvg, pos.x, pos.y);
                nvgLineTo(nvg, pos.x + (showOrigin ? halfSize : borderWidth), pos.y);

                if (showBorder) {
                    nvgMoveTo(nvg, pos.x + borderWidth, pos.y);
                    nvgLineTo(nvg, pos.x + borderWidth, pos.y + borderHeight);
                    nvgLineTo(nvg, pos.x, pos.y + borderHeight);
                }
                nvgLineStyle(nvg, NVG_LINE_SOLID);
                nvgStrokeColor(nvg, canvasBackgroundCol);
                nvgStrokeWidth(nvg, 8.0f);
                nvgStroke(nvg);

                nvgFillColor(nvg, canvasBackgroundCol);
                nvgFillRect(nvg, pos.x - 1.0f, pos.y - 1.0f, 2, 2);
            }

            nvgStrokeColor(nvg, canvasMarkingsCol);
            nvgStrokeWidth(nvg, scaledStrokeSize);
            nvgDashLength(nvg, 8.0f);
            nvgLineStyle(nvg, NVG_LINE_DASHED);

            if (fg) {
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, pos.x, pos.y);
                nvgLineTo(nvg, pos.x, pos.y + (showOrigin ? halfSize : borderHeight));
                nvgStroke(nvg);
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, pos.x, pos.y);
                nvgLineTo(nvg, pos.x + (showOrigin ? halfSize : borderWidth), pos.y);
                nvgStroke(nvg);
            }
            if (showBorder && fg) {
                nvgStrokeWidth(nvg, scaledStrokeSize);
                nvgLineStyle(nvg, NVG_LINE_DASHED);
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, pos.x + borderWidth, pos.y + borderHeight);
                nvgLineTo(nvg, pos.x + borderWidth, pos.y);
                nvgStroke(nvg);
                nvgBeginPath(nvg);
                nvgMoveTo(nvg, pos.x + borderWidth, pos.y + borderHeight);
                nvgLineTo(nvg, pos.x, pos.y + borderHeight);
                nvgStroke(nvg);

                canvasBorderResizer->render(nvg);
            }
        }
    };

    if (!dimensionsAreBeingEdited)
        drawBorder(true, true);
    else
        drawBorder(true, false);

    // Render objects like [drawcurve], [fillcurve] etc. at the back
    for (auto drawable : drawables) {
        if (drawable) {
            auto const* component = dynamic_cast<Component*>(drawable.get());
            if (invalidRegion.intersects(component->getBounds())) {
                drawable->render(nvg);
            }
        }
    }

    if (::getValue<bool>(presentationMode) || isGraph) {
        renderAllObjects(nvg, invalidRegion);
        // render presentation mode as clipped 'virtual' plugin view
        if (::getValue<bool>(presentationMode)) {
            auto const borderWidth = getValue<float>(patchWidth);
            auto const borderHeight = getValue<float>(patchHeight);
            constexpr auto pos = Point<int>(halfSize, halfSize);
            auto const scale = getValue<float>(zoomScale);
            auto const windowCorner = Corners::windowCornerRadius / scale;

            NVGScopedState scopedState(nvg);

            // background colour to crop outside of border area
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, infiniteCanvasSize, infiniteCanvasSize);
            nvgPathWinding(nvg, NVG_HOLE);
            nvgRoundedRect(nvg, pos.getX(), pos.getY(), borderWidth, borderHeight, windowCorner);
            nvgFillColor(nvg, presentationBackgroundCol);
            nvgFill(nvg);

            // background drop shadow to simulate a virtual plugin
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, infiniteCanvasSize, infiniteCanvasSize);
            nvgPathWinding(nvg, NVG_HOLE);
            nvgRoundedRect(nvg, pos.getX(), pos.getY(), borderWidth, borderHeight, windowCorner);

            int const shadowSize = 24 / scale;
            auto borderArea = Rectangle<int>(0, 0, borderWidth, borderHeight).expanded(shadowSize);
            if (presentationShadowImage.needsUpdate(borderArea.getWidth(), borderArea.getHeight())) {
                presentationShadowImage = NVGImage(nvg, borderArea.getWidth(), borderArea.getHeight(), [borderArea, shadowSize, windowCorner](Graphics& g) {
                    auto shadowPath = Path();
                    shadowPath.addRoundedRectangle(borderArea.reduced(shadowSize).withPosition(shadowSize, shadowSize), windowCorner);
                    StackShadow::renderDropShadow(0, g, shadowPath, Colours::white.withAlpha(0.3f), shadowSize, Point<int>(0, 2)); }, NVGImage::AlphaImage);
            }
            auto const shadowImage = nvgImageAlphaPattern(nvg, pos.getX() - shadowSize, pos.getY() - shadowSize, borderArea.getWidth(), borderArea.getHeight(), 0, presentationShadowImage.getImageId(), convertColour(Colours::black));

            nvgStrokeColor(nvg, presentationWindowOutlineCol);
            nvgStrokeWidth(nvg, 0.5f / scale);
            nvgFillPaint(nvg, shadowImage);
            nvgFill(nvg);
            nvgStroke(nvg);
        }
    }
    // render connections infront or behind objects depending on lock mode or overlay setting
    else {
        if (connectionsBehind) {
            renderAllConnections(nvg, invalidRegion);
            renderAllObjects(nvg, invalidRegion);
        } else {
            renderAllObjects(nvg, invalidRegion);
            renderAllConnections(nvg, invalidRegion);
        }
    }

    for (auto* connection : connectionsBeingCreated) {
        NVGScopedState scopedState(nvg);
        connection->render(nvg);
    }

    if (graphArea) {
        NVGScopedState scopedState(nvg);
        nvgTranslate(nvg, graphArea->getX(), graphArea->getY());
        graphArea->render(nvg);
    }

    objectGrid.render(nvg);

    if (viewport && lasso.isVisible() && !lasso.getBounds().isEmpty()) {
        auto lassoBounds = lasso.getBounds();
        lassoBounds = lassoBounds.withSize(jmax(lasso.getWidth(), 2), jmax(lasso.getHeight(), 2));
        nvgDrawRoundedRect(nvg, lassoBounds.getX(), lassoBounds.getY(), lassoBounds.getWidth(), lassoBounds.getHeight(), lassoCol, lassoOutlineCol, 0.0f);
    }

    suggestor->renderAutocompletion(nvg);

    // Draw the search panel's selected object over all other objects
    // Because objects can be underneath others
    if (canvasSearchHighlight)
        canvasSearchHighlight->render(nvg);

    if (dimensionsAreBeingEdited) {
        bool borderWasShown = showBorder;
        showBorder = true;
        drawBorder(false, true);
        showBorder = borderWasShown;
    }

    if (objectsDistributeResizer)
        objectsDistributeResizer->render(nvg);

    nvgRestore(nvg);

    // Draw scrollbars
    if (viewport) {
        reinterpret_cast<CanvasViewport*>(viewport.get())->render(nvg, viewport->getLocalArea(this, invalidRegion));
    }
}

void Canvas::renderAllObjects(NVGcontext* nvg, Rectangle<int> const area)
{
    for (auto* obj : objects) {
        {
            auto b = obj->getBounds();
            if (b.intersects(area) && obj->isVisible()) {
                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, b.getX(), b.getY());
                obj->render(nvg);
            }
        }

        // Draw label in canvas coordinates
        obj->renderLabel(nvg);
    }
}
void Canvas::renderAllConnections(NVGcontext* nvg, Rectangle<int> const area)
{
    if (!connectionLayer.isVisible())
        return;

    // TODO: Can we clean this up? We will want to have selected connections in-front,
    //  and take precedence over non-selected for resize handles

    SmallArray<Connection*> connectionsToDraw;
    SmallArray<Connection*> connectionsToDrawSelected;
    Connection* hovered = nullptr;

    for (auto* connection : connections) {
        NVGScopedState scopedState(nvg);
        if (connection->intersectsRectangle(area) && connection->isVisible()) {
            if (connection->isMouseHovering())
                hovered = connection;
            else if (!connection->isSelected())
                connection->render(nvg);
            else
                connectionsToDrawSelected.add(connection);
            if (showConnectionOrder) {
                connectionsToDraw.add(connection);
            }
        }
    }
    // Draw all selected connections in front
    if (connectionsToDrawSelected.not_empty()) {
        for (auto* connection : connectionsToDrawSelected) {
            NVGScopedState scopedState(nvg);
            connection->render(nvg);
        }
    }

    if (hovered) {
        NVGScopedState scopedState(nvg);
        hovered->render(nvg);
    }

    if (connectionsToDraw.not_empty()) {
        for (auto const* connection : connectionsToDraw) {
            NVGScopedState scopedState(nvg);
            connection->renderConnectionOrder(nvg);
        }
    }
}

void Canvas::settingsChanged(String const& name, var const& value)
{
    switch (hash(name)) {
    case hash("grid_size"):
        repaint();
        break;
    case hash("border"):
        showBorder = static_cast<int>(value);
        repaint();
        break;
    case hash("edit"):
    case hash("lock"):
    case hash("run"):
    case hash("alt"):
    case hash("alt_mode"): {
        updateOverlays();
        break;
    }
    default:
        break;
    }
}

bool Canvas::shouldShowObjectActivity() const
{
    return showObjectActivity && !presentationMode.getValue() && !isGraph;
}

bool Canvas::shouldShowIndex() const
{
    return showIndex && !presentationMode.getValue();
}

bool Canvas::shouldShowConnectionDirection() const
{
    return showConnectionDirection;
}

bool Canvas::shouldShowConnectionActivity() const
{
    return showConnectionActivity;
}

int Canvas::getOverlays() const
{
    int overlayState = 0;

    auto const overlaysTree = SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays");

    auto const altModeEnabled = overlaysTree.getProperty("alt_mode") && !isGraph;

    if (!locked.getValue()) {
        overlayState = overlaysTree.getProperty("edit");
    }
    if (locked.getValue() || commandLocked.getValue()) {
        overlayState = overlaysTree.getProperty("lock");
    }
    if (altModeEnabled) {
        overlayState = overlaysTree.getProperty("alt");
    }

    return overlayState;
}

void Canvas::updateOverlays()
{
    int const overlayState = getOverlays();

    showBorder = overlayState & Border;
    showOrigin = overlayState & Origin;
    showConnectionOrder = overlayState & Order;
    connectionsBehind = overlayState & Behind;
    showObjectActivity = overlayState & ActivationState;
    showIndex = overlayState & Index;
    showConnectionDirection = overlayState & Direction;
    showConnectionActivity = overlayState & ConnectionActivity;

    set_plugdata_activity_enabled(showObjectActivity);
    orderConnections();

    repaint();
}

void Canvas::jumpToOrigin()
{
    if (viewport)
        viewport->setViewPosition((canvasOrigin + Point<int>(1, 1)).transformedBy(getTransform()));
}

void Canvas::restoreViewportState()
{
    if (viewport) {
        viewport->setViewPosition((patch.lastViewportPosition + canvasOrigin).transformedBy(getTransform()));
        zoomScale.setValue(patch.lastViewportScale);
        setTransform(AffineTransform().scaled(patch.lastViewportScale));
    }
}

void Canvas::saveViewportState()
{
    if (viewport) {
        patch.lastViewportPosition = viewport->getViewPosition().transformedBy(getTransform().inverted()) - canvasOrigin;
        patch.lastViewportScale = getValue<float>(zoomScale);
    }
}

void Canvas::zoomToFitAll()
{
    if (objects.empty() || !viewport)
        return;

    auto scale = getValue<float>(zoomScale);

    auto regionOfInterest = Rectangle<int>(canvasOrigin.x, canvasOrigin.y, 20, 20);

    if (!presentationMode.getValue()) {
        for (auto const* object : objects) {
            regionOfInterest = regionOfInterest.getUnion(object->getBounds().reduced(Object::margin));
        }
    }

    // Add a bit of margin to make it nice
    regionOfInterest = regionOfInterest.expanded(16);

    auto const viewArea = viewport->getViewArea() / scale;

    auto const roiHeight = static_cast<float>(regionOfInterest.getHeight());
    auto const roiWidth = static_cast<float>(regionOfInterest.getWidth());

    auto const scaleWidth = viewArea.getWidth() / roiWidth;
    auto const scaleHeight = viewArea.getHeight() / roiHeight;
    scale = jmin(scaleWidth, scaleHeight);
    scale = std::clamp(scale, 0.05f, 3.0f);

    auto transform = getTransform();
    transform = transform.scaled(scale);
    setTransform(transform);

    scale = std::sqrt(std::abs(transform.getDeterminant()));
    zoomScale.setValue(scale);

    auto const viewportCentre = viewport->getViewArea().withZeroOrigin().getCentre();
    auto const newViewPos = regionOfInterest.transformedBy(getTransform()).getCentre() - viewportCentre;
    viewport->setViewPosition(newViewPos);
}

void Canvas::tabChanged()
{
    patch.setCurrent();

    synchronise();
    updateDrawables();

    for (auto const* obj : objects) {
        if (!obj->gui)
            continue;

        obj->gui->tabChanged();
    }

    editor->statusbar->updateZoomLevel();
    editor->repaint(); // Make sure everything it up to date
}

void Canvas::save(std::function<void()> const& nestedCallback)
{
    auto const* canvasToSave = this;
    if (patch.isSubpatch()) {
        for (auto const& parentCanvas : editor->getCanvases()) {
            if (patch.getRoot() == parentCanvas->patch.getRawPointer()) {
                canvasToSave = parentCanvas;
            }
        }
    }

    if (canvasToSave->patch.getCurrentFile().existsAsFile()) {
        canvasToSave->patch.savePatch();
        SettingsFile::getInstance()->addToRecentlyOpened(canvasToSave->patch.getCurrentURL());
        pd->titleChanged();
        nestedCallback();
    } else {
        saveAs(nestedCallback);
    }
}

void Canvas::saveAs(std::function<void()> const& nestedCallback)
{
    Dialogs::showSaveDialog([this, nestedCallback](URL const& resultURL) mutable {
        auto result = resultURL.getLocalFile();
        if (result.getFullPathName().isNotEmpty()) {
            if (result.exists())
                result.deleteFile();
            
            patch.savePatch(resultURL);
            SettingsFile::getInstance()->addToRecentlyOpened(resultURL);
            pd->titleChanged();
        }

        nestedCallback();
    },
        "*.pd", "Patch", this);
}

void Canvas::handleAsyncUpdate()
{
    performSynchronise();
}

void Canvas::synchronise()
{
    triggerAsyncUpdate();
}

void Canvas::synchroniseAllCanvases()
{
    for (auto* editorWindow : pd->getEditors()) {
        for (auto* canvas : editorWindow->getTabComponent().getVisibleCanvases()) {
            canvas->synchronise();
        }
    }
}

void Canvas::synchroniseSplitCanvas()
{
    for (auto* e : pd->getEditors()) {
        for (auto* canvas : e->getTabComponent().getVisibleCanvases()) {
            canvas->synchronise();
        }
    }
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::performSynchronise()
{
    static bool alreadyFlushed = false;
    bool const needsFlush = !alreadyFlushed;
    ScopedValueSetter<bool> flushGuard(alreadyFlushed, true);
    // By flushing twice, we can make sure that any message sent before this point will be dequeued
    if (needsFlush && !isGraph)
        pd->doubleFlushMessageQueue();

    // Remove deleted connections
    for (int n = connections.size() - 1; n >= 0; n--) {
        if (!connections[n]->getPointer()) {
            connections.remove_at(n);
        }
    }

    // Remove deleted objects
    for (int n = objects.size() - 1; n >= 0; n--) {
        // If the object is showing it's initial editor, meaning no object was assigned yet, allow it to exist without pointing to an object
        if (auto* object = objects[n]; !object->getPointer() && !object->isInitialEditorShown()) {
            setSelected(object, false, false);
            objects.remove_at(n);
        }
    }

    // Check for connections that need to be remade because of invalid iolets
    for (int n = connections.size() - 1; n >= 0; n--) {
        if (!connections[n]->inlet || !connections[n]->outlet) {
            connections.remove_at(n);
        }
    }

    auto pdObjects = patch.getObjects();
    objects.reserve(pdObjects.size());

    for (auto object : pdObjects) {
        auto const* it = std::ranges::find_if(objects, [&object](Object const* b) { return b->getPointer() && b->getPointer() == object.getRawUnchecked<void>(); });
        if (!object.isValid())
            continue;

        if (it == objects.end()) {
            auto* newObject = objects.add(object, this);
            newObject->toFront(false);

            if (newObject->gui && newObject->gui->getLabel())
                newObject->gui->getLabel()->toFront(false);
        } else {
            auto* object = *it;

            // Check if number of inlets/outlets is correct
            object->updateIolets();
            object->updateBounds();

            object->toFront(false);
            if (object->gui && object->gui->getLabel())
                object->gui->getLabel()->toFront(false);
            
            if (object->gui)
                object->gui->updateProperties();
        }
    }

    // Make sure objects have the same order
    std::ranges::sort(objects,
        [&pdObjects](Object const* first, Object const* second) mutable {
            return pdObjects.index_of(first->getPointer()) < pdObjects.index_of(second->getPointer());
        });

    auto pdConnections = isGraph ? pd::Connections() : patch.getConnections();
    connections.reserve(pdConnections.size());

    for (auto& connection : pdConnections) {
        auto& [ptr, inno, inobj, outno, outobj] = connection;

        Iolet *inlet = nullptr, *outlet = nullptr;

        // Find the objects that this connection is connected to
        for (auto* obj : objects) {
            if (outobj && &outobj->te_g == obj->getPointer()) {

                // Check if we have enough outlets, should never return false
                if (isPositiveAndBelow(obj->numInputs + outno, obj->iolets.size())) {
                    outlet = obj->iolets[obj->numInputs + outno];
                } else {
                    break;
                }
            }
            if (inobj && &inobj->te_g == obj->getPointer()) {

                // Check if we have enough inlets, should never return false
                if (isPositiveAndBelow(inno, obj->iolets.size())) {
                    inlet = obj->iolets[inno];
                } else {
                    break;
                }
            }
        }

        // This shouldn't be necessary, but just to be sure...
        if (!inlet || !outlet) {
            jassertfalse;
            continue;
        }

        auto const* it = std::ranges::find_if(connections,
            [c_ptr = ptr](auto* c) {
                return c_ptr == c->getPointer();
            });

        if (it == connections.end()) {
            connections.add(this, inlet, outlet, ptr);
        } else {
            // This is necessary to make resorting a subpatchers iolets work
            // And it can't hurt to check if the connection is valid anyway
            if (auto& c = **it; c.inlet != inlet || c.outlet != outlet) {
                int const idx = connections.index_of(*it);
                connections.remove_one(*it);
                connections.insert(idx, this, inlet, outlet, ptr);
            } else {
                c.popPathState();
            }
        }
    }

    if (!isGraph) {
        setTransform(AffineTransform().scaled(getValue<float>(zoomScale)));
    }

    if (graphArea)
        graphArea->updateBounds();

    editor->updateCommandStatus();
    repaint();

    needsSearchUpdate = true;

    pd->updateObjectImplementations();
    cancelPendingUpdate(); // if an update got retriggered, cancel it
}

void Canvas::updateDrawables()
{
    for (auto const* object : objects) {
        if (object->gui) {
            object->gui->updateDrawables();
        }
    }
}

void Canvas::shiftKeyChanged(bool const isHeld)
{
    shiftDown = isHeld;

    if (!isGraph) {
        SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").setProperty("alt_mode", altDown && shiftDown, nullptr);
    }

    if (!isHeld)
        return;

    if (connectionsBeingCreated.size() == 1) {
        Iolet* connectingOutlet = connectionsBeingCreated[0]->getIolet();
        Iolet* targetInlet = nullptr;
        for (auto const& object : objects) {
            for (auto const& iolet : object->iolets) {
                if (iolet->isTargeted && iolet != connectingOutlet) {
                    targetInlet = iolet;
                    break;
                }
            }
        }

        if (targetInlet) {
            bool const inverted = connectingOutlet->isInlet;
            if (inverted)
                std::swap(connectingOutlet, targetInlet);

            if (auto x = patch.getPointer()) {
                auto* outObj = connectingOutlet->object->getPointer();
                auto* inObj = targetInlet->object->getPointer();
                auto const outletIndex = connectingOutlet->ioletIdx;
                auto const inletIndex = targetInlet->ioletIdx;

                SmallArray<t_gobj*> selectedObjects;
                for (auto const* object : getSelectionOfType<Object>()) {
                    if (auto* ptr = object->getPointer()) {
                        selectedObjects.add(ptr);
                    }
                }

                // If we autopatch from inlet to outlet with multiple selection, pure-data can't handle it
                if (inverted && selectedObjects.size() > 1)
                    return;

                t_outconnect const* connection = nullptr;
                if (auto selectedConnections = getSelectionOfType<Connection>(); selectedConnections.size() == 1) {
                    connection = selectedConnections[0]->getPointer();
                }

                pd::Interface::shiftAutopatch(x.get(), inObj, inletIndex, outObj, outletIndex, selectedObjects, connection);
            }
        }

        synchronise();
    }
}

void Canvas::commandKeyChanged(bool const isHeld)
{
    commandLocked = isHeld;
}

void Canvas::middleMouseChanged(bool isHeld)
{
    checkPanDragMode();
}

void Canvas::altKeyChanged(bool const isHeld)
{
    altDown = isHeld;

    if (!isGraph) {
        SettingsFile::getInstance()->getValueTree().getChildWithName("Overlays").setProperty("alt_mode", altDown && shiftDown, nullptr);
    }
}

void Canvas::mouseDown(MouseEvent const& e)
{
    if (isGraph)
        return;

    PopupMenu::dismissAllActiveMenus();

    if (checkPanDragMode())
        return;

    if (objectsDistributeResizer) {
        objectsDistributeResizer.reset();
    }

    auto* source = e.originalComponent;

    // Left-click
    if (!e.mods.isRightButtonDown()) {

        if (source == this) {
            dragState.duplicateOffset = { 0, 0 };
            dragState.lastDuplicateOffset = { 0, 0 };
            dragState.wasDuplicated = false;
            cancelConnectionCreation();

            if (SettingsFile::getInstance()->getProperty<bool>("cmd_click_switches_mode")) {
                if (e.mods.isCommandDown()) {
                    // Lock if cmd + click on canvas
                    deselectAll();

                    presentationMode.setValue(false);

                    // when command + click on canvas, swap between locked / edit mode
                    locked.setValue(!locked.getValue());
                    locked.getValueSource().sendChangeMessage(true);

                    updateOverlays();
                }
            }

            if (!e.mods.isShiftDown()) {
                deselectAll();
            }

            if (!(e.source.isTouch() && e.source.getIndex() != 0) && !getValue<bool>(locked)) {
                lasso.beginLasso(e.getEventRelativeTo(this), this);
                isDraggingLasso = true;
            }
        }

        // Update selected object in sidebar when we click a object
        if (source && source->findParentComponentOfClass<Object>()) {
            updateSidebarSelection();
        }

        editor->updateCommandStatus();
    }
    // Right click
    else {
        Dialogs::showCanvasRightClickMenu(this, source, e.getScreenPosition());
    }
}

bool Canvas::hitTest(int const x, int const y)
{
    // allow panning to happen anywhere, even when in presentation mode
    if (panningModifierDown())
        return true;

    // disregard mouse drag if outside of patch
    if (::getValue<bool>(presentationMode)) {
        if (isPointOutsidePluginArea(Point<int>(x, y)))
            return false;
    }
    return true;
}

void Canvas::mouseDrag(MouseEvent const& e)
{
    if (canvasRateReducer.tooFast() || panningModifierDown())
        return;

    if (connectingWithDrag) {
        for (auto* obj : objects) {
            for (auto const& iolet : obj->iolets) {
                iolet->mouseDrag(e.getEventRelativeTo(iolet));
            }
        }
    }

    // Ignore on graphs or when locked
    if ((isGraph || locked == var(true) || commandLocked == var(true)) && !ObjectBase::isBeingEdited()) {
        bool hasToggled = false;

        // Behaviour for dragging over toggles, bang and radiogroup to toggle them
        for (auto const* object : objects) {
            if (!object->getBounds().contains(e.getEventRelativeTo(this).getPosition()) || !object->gui)
                continue;

            if (auto* obj = object->gui.get()) {
                obj->toggleObject(e.getEventRelativeTo(obj).getPosition());
                hasToggled = true;
                break;
            }
        }

        if (!hasToggled) {
            for (auto const* object : objects) {
                if (auto* obj = object->gui.get()) {
                    obj->untoggleObject();
                }
            }
        }

        return;
    }

    auto const viewportEvent = e.getEventRelativeTo(viewport.get());
    if (viewport && !ObjectBase::isBeingEdited() && autoscroll(viewportEvent)) {
        beginDragAutoRepeat(25);
    }

    // Drag lasso
    if (!(e.source.isTouch() && e.source.getIndex() != 0)) {
        lasso.dragLasso(e);
        lasso.setBounds(lasso.getBounds().withWidth(jmax(2, lasso.getWidth())).withHeight(jmax(2, lasso.getHeight())));
    }
}

bool Canvas::autoscroll(MouseEvent const& e)
{
    if (!viewport)
        return false;

    auto x = viewport->getViewPositionX();
    auto y = viewport->getViewPositionY();
    auto const oldY = y;
    auto const oldX = x;

    auto const pos = e.getPosition();

    if (pos.x > viewport->getWidth()) {
        x += std::clamp((pos.x - viewport->getWidth()) / 6, 1, 14);
    } else if (pos.x < 0) {
        x -= std::clamp(-pos.x / 6, 1, 14);
    }
    if (pos.y > viewport->getHeight()) {
        y += std::clamp((pos.y - viewport->getHeight()) / 6, 1, 14);
    } else if (pos.y < 0) {
        y -= std::clamp(-pos.y / 6, 1, 14);
    }

    if (x != oldX || y != oldY) {
        viewport->setViewPosition(x, y);
        return true;
    }

    return false;
}

void Canvas::mouseUp(MouseEvent const& e)
{
    setPanDragMode(false);
    setMouseCursor(MouseCursor::NormalCursor);

    connectionCancelled = false;

    // Double-click canvas to create new object
    if (e.mods.isLeftButtonDown() && e.getNumberOfClicks() == 2 && e.originalComponent == this && !isGraph && !getValue<bool>(locked)) {
        auto* newObject = objects.add(this, "", e.getPosition());
        deselectAll();
        setSelected(newObject, true); // Select newly created object
    }

    // Make sure the drag-over toggle action is ended
    if (!isDraggingLasso) {
        for (auto const* object : objects) {
            if (auto* obj = object->gui.get()) {
                obj->untoggleObject();
            }
        }
    }

    updateSidebarSelection();

    editor->updateCommandStatus();

    lasso.endLasso();
    isDraggingLasso = false;
    for (auto* object : objects)
        object->originalBounds = Rectangle<int>(0, 0, 0, 0);

    if (connectingWithDrag) {
        for (auto* obj : objects) {
            for (auto const& iolet : obj->iolets) {
                auto relativeEvent = e.getEventRelativeTo(this);
                if (iolet->getCanvasBounds().expanded(20).contains(relativeEvent.getPosition())) {
                    iolet->mouseUp(relativeEvent);
                }
            }
        }
    }
}

void Canvas::updateSidebarSelection()
{
    // Post to message queue so that sidebar parameters are updated AFTER objects resize is run
    // Otherwise position XY is not populated

    MessageManager::callAsync([_this = SafePointer(this), this] {
        if (!_this)
            return;

        auto lassoSelection = getSelectionOfType<Object>();
        SmallArray<ObjectParameters, 6> allParameters;
        auto toShow = SmallArray<Component*>();

        bool showOnSelect = false;

        for (auto* object : lassoSelection) {
            if (!object->gui)
                continue;
            auto parameters = object->gui ? object->gui->getParameters() : ObjectParameters();
            showOnSelect = object->gui && object->gui->showParametersWhenSelected();
            allParameters.add(parameters);
            toShow.add(object);
        }

        editor->sidebar->showParameters(toShow, allParameters, showOnSelect);
    });
}

bool Canvas::keyPressed(KeyPress const& key)
{
    if (editor->getCurrentCanvas() != this || isGraph)
        return false;

    int const keycode = key.getKeyCode();

    auto moveSelection = [this](int const x, int const y) {
        auto objects = getSelectionOfType<Object>();
        if (objects.empty())
            return false;

        SmallArray<t_gobj*> pdObjects;
        for (auto const* object : objects) {
            if (auto* ptr = object->getPointer()) {
                pdObjects.add(ptr);
            }
        }

        patch.moveObjects(pdObjects, x, y);

        // Update object bounds and store the total bounds of the selection
        auto totalBounds = Rectangle<int>();
        for (auto* object : objects) {
            object->updateBounds();
            totalBounds = totalBounds.getUnion(object->getBounds());
        }

        // TODO: consider calculating the totalBounds with object->getBounds().reduced(Object::margin)
        // then adding viewport padding in screen pixels so it's consistent regardless of scale
        auto const scale = ::getValue<float>(zoomScale);
        constexpr auto viewportPadding = 10;

        auto viewX = viewport->getViewPositionX() / scale;
        auto viewY = viewport->getViewPositionY() / scale;
        auto const viewWidth = (viewport->getWidth() - viewportPadding) / scale;
        auto const viewHeight = (viewport->getHeight() - viewportPadding) / scale;
        if (x < 0 && totalBounds.getX() < viewX) {
            viewX = totalBounds.getX();
        } else if (totalBounds.getRight() > viewX + viewWidth) {
            viewX = totalBounds.getRight() - viewWidth;
        }
        if (y < 0 && totalBounds.getY() < viewY) {
            viewY = totalBounds.getY();
        } else if (totalBounds.getBottom() > viewY + viewHeight) {
            viewY = totalBounds.getBottom() - viewHeight;
        }
        viewport->setViewPosition(viewX * scale, viewY * scale);
        return true;
    };

    // Cancel connections being created by ESC key
    if (keycode == KeyPress::escapeKey && !connectionsBeingCreated.empty()) {
        cancelConnectionCreation();
        return true;
    }

    // Move objects with arrow keys
    int moveDistance = objectGrid.gridSize;
    if (key.getModifiers().isShiftDown()) {
        moveDistance = 1;
    } else if (key.getModifiers().isCommandDown()) {
        moveDistance *= 4;
    }

    if (keycode == KeyPress::leftKey) {
        moveSelection(-moveDistance, 0);
        return false;
    }
    if (keycode == KeyPress::rightKey) {
        moveSelection(moveDistance, 0);
        return false;
    }
    if (keycode == KeyPress::upKey) {
        moveSelection(0, -moveDistance);
        return false;
    }
    if (keycode == KeyPress::downKey) {
        moveSelection(0, moveDistance);
        return false;
    }
    if (keycode == KeyPress::tabKey) {
        cycleSelection();
        return false;
    }

    return false;
}

void Canvas::deselectAll(bool const broadcastChange)
{
    if (!broadcastChange)
        selectedComponents.removeChangeListener(this);

    selectedComponents.deselectAll();
    editor->sidebar->hideParameters();

    if (!broadcastChange) {
        // Add back the listener, but make sure it's added back 'after' the last event on the message queue
        MessageManager::callAsync([this] { selectedComponents.addChangeListener(this); });
    }
}

void Canvas::hideAllActiveEditors()
{
    for (auto* object : objects) {
        object->hideEditor();
    }
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    SmallArray<t_gobj*> objects;
    for (auto const* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            objects.add(ptr);
        }
    }

    // Tell pd to copy
    patch.copy(objects);
    patch.deselectAll();
}

void Canvas::focusGained(FocusChangeType cause)
{
    pd->openedEditors.move(pd->openedEditors.indexOf(editor), 0);

    pd->enqueueFunctionAsync([pd = this->pd, patchPtr = patch.getUncheckedPointer(), hasFocus = static_cast<float>(hasKeyboardFocus(true))] {
        auto* activeGui = pd->generateSymbol("#active_gui")->s_thing;
        auto* hammarGui = pd->generateSymbol("#hammergui")->s_thing;
        if (activeGui || hammarGui) {
            // canvas.active listener
            char buf[MAXPDSTRING];
            snprintf(buf, MAXPDSTRING - 1, ".x%lx.c", reinterpret_cast<unsigned long>(patchPtr));
            pd->lockAudioThread();
            pd->sendTypedMessage(activeGui, "_focus", { pd->generateSymbol(buf), hasFocus });
            pd->sendTypedMessage(hammarGui, "_focus", { pd->generateSymbol(buf), hasFocus });
            pd->unlockAudioThread();
        }
    });
}

void Canvas::focusLost(FocusChangeType cause)
{
    pd->enqueueFunctionAsync([pd = this->pd, patchPtr = patch.getUncheckedPointer(), hasFocus = static_cast<float>(hasKeyboardFocus(true))] {
        auto* activeGui = pd->generateSymbol("#active_gui")->s_thing;
        auto* hammarGui = pd->generateSymbol("#hammergui")->s_thing;
        if (activeGui || hammarGui) {
            // canvas.active listener
            char buf[MAXPDSTRING];
            snprintf(buf, MAXPDSTRING - 1, ".x%lx.c", reinterpret_cast<unsigned long>(patchPtr));
            pd->lockAudioThread();
            pd->sendTypedMessage(activeGui, "_focus", { pd->generateSymbol(buf), hasFocus });
            pd->sendTypedMessage(hammarGui, "_focus", { pd->generateSymbol(buf), hasFocus });
            pd->unlockAudioThread();
        }
    });
}

void Canvas::dragAndDropPaste(String const& patchString, Point<int> const mousePos, int const patchWidth, int const patchHeight, String const& name)
{
    locked = false;
    presentationMode = false;

    // force the valueChanged to run, and wait for them to return
    locked.getValueSource().sendChangeMessage(true);
    presentationMode.getValueSource().sendChangeMessage(true);

    MessageManager::callAsync([_this = SafePointer(this)] {
        if (_this)
            _this->grabKeyboardFocus();
    });

    auto undoText = String("Add object");
    if (name.isNotEmpty())
        undoText = String("Add " + name.toLowerCase());

    patch.startUndoSequence(undoText);

    auto const patchSize = Point<int>(patchWidth, patchHeight);
    String const translatedObjects = pd::Patch::translatePatchAsString(patchString, mousePos - patchSize / 2.0f);

    if (auto patchPtr = patch.getPointer()) {
        pd::Interface::paste(patchPtr.get(), translatedObjects.toRawUTF8());
    }

    deselectAll();

    // Load state from pd
    performSynchronise();

    patch.setCurrent();

    SmallArray<t_gobj*> pastedObjects;

    if (auto patchPtr = patch.getPointer()) {
        for (auto* object : objects) {
            auto* objectPtr = object->getPointer();
            if (objectPtr && glist_isselected(patchPtr.get(), objectPtr)) {
                setSelected(object, true);
                pastedObjects.emplace_back(objectPtr);
            }
        }
    }

    patch.deselectAll();
    pastedObjects.clear();
    patch.endUndoSequence(undoText);

    updateSidebarSelection();
}

void Canvas::pasteSelection()
{
    patch.startUndoSequence("Paste object/s");

    // Paste at mousePos, adds padding if pasted the same place
    auto const mousePosition = getMouseXYRelative() - canvasOrigin;
    if (mousePosition == pastedPosition) {
        pastedPadding.addXY(10, 10);
    } else {
        pastedPadding.setXY(-10, -10);
    }
    pastedPosition = mousePosition;

    // Tell pd to paste with offset applied to the clipboard string
    patch.paste(Point<int>(pastedPosition.x + pastedPadding.x, pastedPosition.y + pastedPadding.y));

    deselectAll();

    // Load state from pd
    performSynchronise();

    patch.setCurrent();

    SmallArray<t_gobj*> pastedObjects;

    if (auto patchPtr = patch.getPointer()) {
        for (auto* object : objects) {
            auto* objectPtr = object->getPointer();
            if (objectPtr && glist_isselected(patchPtr.get(), objectPtr)) {
                setSelected(object, true);
                pastedObjects.emplace_back(objectPtr);
            }
        }
    }

    patch.deselectAll();
    pastedObjects.clear();
    patch.endUndoSequence("Paste object/s");

    updateSidebarSelection();
}

void Canvas::duplicateSelection()
{
    auto selection = getSelectionOfType<Object>();

    patch.startUndoSequence("Duplicate object/s");

    SmallArray<t_gobj*> objectsToDuplicate;
    for (auto const* object : selection) {
        if (auto* ptr = object->getPointer()) {
            objectsToDuplicate.add(ptr);
        }
    }

    // If absolute grid is enabled, snap duplication to grid
    if (dragState.duplicateOffset.isOrigin() && SettingsFile::getInstance()->getProperty<bool>("grid_enabled") && SettingsFile::getInstance()->getProperty<int>("grid_type") & 1) {
        dragState.duplicateOffset = { objectGrid.gridSize - 10, objectGrid.gridSize - 10 };
    }

    // If we previously duplicated and dragged before, and then drag again, the new offset should be relative
    // to the offset we already applied with the previous drag
    if (dragState.lastDuplicateOffset != dragState.duplicateOffset) {
        dragState.duplicateOffset += dragState.lastDuplicateOffset;
    }

    dragState.lastDuplicateOffset = dragState.duplicateOffset;

    t_outconnect* connection = nullptr;
    auto selectedConnections = getSelectionOfType<Connection>();
    SafePointer<Connection> connectionSelectedOriginally = nullptr;
    if (selectedConnections.size() == 1) {
        connectionSelectedOriginally = selectedConnections[0];
        connection = selectedConnections[0]->getPointer();
    }

    // Tell pd to duplicate
    patch.duplicate(objectsToDuplicate, connection);

    deselectAll();

    // Load state from pd immediately
    performSynchronise();

    auto* patchPtr = patch.getRawPointer();
    if (!patchPtr)
        return;

    // Store the duplicated objects for later selection
    SmallArray<Object*> duplicated;
    for (auto* object : objects) {
        auto* objectPtr = object->getPointer();
        if (objectPtr && glist_isselected(patchPtr, objectPtr)) {
            duplicated.add(object);
        }
    }

    // Move duplicated objects if they overlap exisisting objects
    SmallArray<t_gobj*> moveObjects;
    for (auto const* dup : duplicated) {
        moveObjects.add(dup->getPointer());
    }

    patch.moveObjects(moveObjects, dragState.duplicateOffset.x, dragState.duplicateOffset.y);

    for (auto* object : objects) {
        object->updateBounds();
    }

    // Select the newly duplicated objects, and calculate new viewport position
    Rectangle<int> selectionBounds;
    for (auto* obj : duplicated) {
        setSelected(obj, true);
        selectionBounds = selectionBounds.getUnion(obj->getBounds());
    }

    selectionBounds = selectionBounds.transformedBy(getTransform());

    // Adjust the viewport position to ensure the duplicated objects are visible
    auto const viewportPos = viewport->getViewPosition();
    auto const viewWidth = viewport->getWidth();
    auto const viewHeight = viewport->getHeight();
    if (!selectionBounds.isEmpty()) {
        int deltaX = 0, deltaY = 0;

        if (selectionBounds.getRight() > viewportPos.getX() + viewWidth) {
            deltaX = selectionBounds.getRight() - (viewportPos.getX() + viewWidth);
        } else if (selectionBounds.getX() < viewportPos.getX()) {
            deltaX = selectionBounds.getX() - viewportPos.getX();
        }

        if (selectionBounds.getBottom() > viewportPos.getY() + viewHeight) {
            deltaY = selectionBounds.getBottom() - (viewportPos.getY() + viewHeight);
        } else if (selectionBounds.getY() < viewportPos.getY()) {
            deltaY = selectionBounds.getY() - viewportPos.getY();
        }

        // Set the new viewport position
        viewport->setViewPosition(viewportPos + Point<int>(deltaX, deltaY));
    }

    dragState.wasDuplicated = true;

    patch.endUndoSequence("Duplicate object/s");
    patch.deselectAll();

    if (connectionSelectedOriginally) {
        setSelected(connectionSelectedOriginally.getComponent(), true);
    }
}

void Canvas::removeSelection()
{
    patch.startUndoSequence("Remove object/s");
    // Make sure object isn't selected and stop updating gui
    editor->sidebar->hideParameters();

    // Find selected objects and make them selected in pd
    SmallArray<t_gobj*> objects;
    for (auto const* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            objects.add(ptr);
        }
    }

    auto wasDeleted = [&objects](t_gobj* ptr) {
        return objects.contains(ptr);
    };

    // remove selection
    patch.removeObjects(objects);

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto const* con : connections) {
        if (con->isSelected()) {
            auto* outPtr = con->outobj->getPointer();
            auto* inPtr = con->inobj->getPointer();
            auto* checkedOutPtr = pd::Interface::checkObject(outPtr);
            auto* checkedInPtr = pd::Interface::checkObject(inPtr);
            if (checkedOutPtr && checkedInPtr && !(wasDeleted(outPtr) || wasDeleted(inPtr))) {
                patch.removeConnection(checkedOutPtr, con->outIdx, checkedInPtr, con->inIdx, con->getPathState());
            }
        }
    }

    patch.finishRemove(); // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.endUndoSequence("Remove object/s");

    patch.deselectAll();

    synchroniseSplitCanvas();
}

void Canvas::removeSelectedConnections()
{
    patch.startUndoSequence("Remove connection/s");

    for (auto const* con : connections) {
        if (con->isSelected()) {
            auto* checkedOutPtr = pd::Interface::checkObject(con->outobj->getPointer());
            auto* checkedInPtr = pd::Interface::checkObject(con->inobj->getPointer());
            if (!checkedInPtr || !checkedOutPtr)
                continue;

            patch.removeConnection(checkedOutPtr, con->outIdx, checkedInPtr, con->inIdx, con->getPathState());
        }
    }

    patch.endUndoSequence("Remove connection/s");

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    synchroniseSplitCanvas();
}

void Canvas::cycleSelection()
{
    if (connectionsBeingCreated.size() == 1) {
        connectionsBeingCreated[0]->toNextIolet();
        return;
    }
    // Get the selected objects
    if (auto selectedObjects = getSelectionOfType<Object>(); selectedObjects.size() == 1) {
        // Find the index of the currently selected object
        auto const currentIdx = objects.index_of(selectedObjects[0]);
        setSelected(selectedObjects[0], false);

        // Calculate the next index (wrap around if at the end)
        auto const nextIdx = (currentIdx + 1) % objects.size();
        setSelected(objects[nextIdx], true);

        return;
    }

    // Get the selected connections if no objects are selected
    if (auto selectedConnections = getSelectionOfType<Connection>(); selectedConnections.size() == 1) {
        // Find the index of the currently selected connection
        auto const currentIdx = connections.index_of(selectedConnections[0]);
        setSelected(selectedConnections[0], false);

        // Calculate the next index (wrap around if at the end)
        auto const nextIdx = (currentIdx + 1) % connections.size();
        setSelected(connections[nextIdx], true);
    }
}

void Canvas::tidySelection()
{
    SmallArray<t_gobj*> selectedObjects;
    for (auto const* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            selectedObjects.add(ptr);
        }
    }

    if (auto patchPtr = patch.getPointer()) {
        pd::Interface::tidy(patchPtr.get(), selectedObjects);
    }

    synchronise();
}

void Canvas::triggerizeSelection()
{
    SmallArray<t_gobj*> selectedObjects;
    for (auto const* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            selectedObjects.add(ptr);
        }
    }

    t_outconnect const* connection = nullptr;
    auto selectedConnections = getSelectionOfType<Connection>();
    if (selectedConnections.size() == 1) {
        connection = selectedConnections[0]->getPointer();
    }

    t_gobj const* triggerizedObject = nullptr;
    if (auto patchPtr = patch.getPointer()) {
        triggerizedObject = pd::Interface::triggerize(patchPtr.get(), selectedObjects, connection);
    }

    performSynchronise();

    if (triggerizedObject) {
        for (auto* object : objects) {
            if (object->getPointer() == triggerizedObject) {
                setSelected(object, true);
                object->showEditor();
                hideSuggestions();
            }
        }
    }
}

void Canvas::encapsulateSelection()
{
    auto selectedObjects = getSelectionOfType<Object>();

    // Sort by index in pd patch
    selectedObjects.sort([this](auto const* a, auto const* b) -> bool {
        return objects.index_of(a) < objects.index_of(b);
    });

    // If two connections have the same target inlet/outlet, we only need 1 [inlet/outlet] object
    auto usedIolets = SmallArray<Iolet*>();
    auto targetIolets = UnorderedMap<Iolet*, SmallArray<Iolet*>>();

    auto newInternalConnections = String();
    auto newExternalConnections = UnorderedMap<int, SmallArray<Iolet*>>();

    // First, find all the incoming and outgoing connections
    for (auto* connection : connections) {
        if (selectedObjects.contains(connection->inobj.get()) && !selectedObjects.contains(connection->outobj.get())) {
            auto* inlet = connection->inlet.get();
            targetIolets[inlet].add(connection->outlet.get());
            usedIolets.add_unique(inlet);
        }
    }
    for (auto* connection : connections) {
        if (selectedObjects.contains(connection->outobj.get()) && !selectedObjects.contains(connection->inobj.get())) {
            auto* outlet = connection->outlet.get();
            targetIolets[outlet].add(connection->inlet.get());
            usedIolets.add_unique(outlet);
        }
    }

    auto newEdgeObjects = String();

    usedIolets.sort([](auto* a, auto* b) -> bool {
        // Inlets before outlets
        if (a->isInlet != b->isInlet)
            return a->isInlet;

        auto apos = a->getCanvasBounds().getPosition();
        auto bpos = b->getCanvasBounds().getPosition();

        if (apos.x == bpos.x) {
            return apos.y < bpos.y;
        }

        return apos.x < bpos.x;
    });

    int i = 0;
    int numIn = 0;
    for (auto* iolet : usedIolets) {
        auto type = String(iolet->isInlet ? "inlet" : "outlet") + String(iolet->isSignal ? "~" : "");
        auto* targetEdge = targetIolets[iolet][0];
        auto pos = targetEdge->object->getObjectBounds().getPosition();
        newEdgeObjects += "#X obj " + String(pos.x) + " " + String(pos.y) + " " + type + ";\n";

        int objIdx = selectedObjects.index_of(iolet->object);
        int ioletObjectIdx = selectedObjects.size() + i;
        if (iolet->isInlet) {
            newInternalConnections += "#X connect " + String(ioletObjectIdx) + " 0 " + String(objIdx) + " " + String(iolet->ioletIdx) + ";\n";
            numIn++;
        } else {
            newInternalConnections += "#X connect " + String(objIdx) + " " + String(iolet->ioletIdx) + " " + String(ioletObjectIdx) + " 0;\n";
        }

        for (auto* target : targetIolets[iolet]) {
            newExternalConnections[i].add(target);
        }

        i++;
    }

    patch.deselectAll();

    auto bounds = Rectangle<int>();
    SmallArray<t_gobj*> objects;
    for (auto* object : selectedObjects) {
        if (auto* ptr = object->getPointer()) {
            bounds = bounds.getUnion(object->getBounds());
            objects.add(ptr);
        }
    }
    auto centre = bounds.getCentre() - canvasOrigin;

    auto copypasta = String("#N canvas 733 172 450 300 0 1;\n") + "$$_COPY_HERE_$$" + newEdgeObjects + newInternalConnections + "#X restore " + String(centre.x) + " " + String(centre.y) + " pd;\n";

    // Apply the changed on Pd's thread
    if (auto patchPtr = patch.getPointer()) {
        int size;
        char const* text = pd::Interface::copy(patchPtr.get(), &size, objects);
        auto copied = String::fromUTF8(text, size);

        // Wrap it in an undo sequence, to allow undoing everything in 1 step
        patch.startUndoSequence("Encapsulate");

        pd::Interface::removeObjects(patchPtr.get(), objects);

        auto replacement = copypasta.replace("$$_COPY_HERE_$$", copied);

        pd::Interface::paste(patchPtr.get(), replacement.toRawUTF8());
        auto lastObject = patch.getObjects().back();
        if (!lastObject.isValid())
            return;

        auto* newObject = pd::Interface::checkObject(lastObject.getRaw<t_pd>());
        if (!newObject) {
            patch.endUndoSequence("Encapsulate");
            pd->unlockAudioThread();
            return;
        }

        for (auto& [idx, iolets] : newExternalConnections) {
            for (auto* iolet : iolets) {
                if (auto* externalObject = reinterpret_cast<t_object*>(iolet->object->getPointer())) {
                    if (iolet->isInlet) {
                        pd::Interface::createConnection(patchPtr.get(), newObject, idx - numIn, externalObject, iolet->ioletIdx);
                    } else {
                        pd::Interface::createConnection(patchPtr.get(), externalObject, iolet->ioletIdx, newObject, idx);
                    }
                }
            }
        }

        patch.endUndoSequence("Encapsulate");
    }

    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();
}

void Canvas::connectSelection()
{
    SmallArray<t_gobj*> selectedObjects;
    for (auto const* object : getSelectionOfType<Object>()) {
        if (auto* ptr = object->getPointer()) {
            selectedObjects.add(ptr);
        }
    }

    t_outconnect const* connection = nullptr;
    if (auto selectedConnections = getSelectionOfType<Connection>(); selectedConnections.size() == 1) {
        connection = selectedConnections[0]->getPointer();
    }

    if (auto patchPtr = patch.getPointer()) {
        pd::Interface::connectSelection(patchPtr.get(), selectedObjects, connection);
    }

    synchronise();
}

void Canvas::cancelConnectionCreation()
{
    connectionsBeingCreated.clear();
    if (connectingWithDrag) {
        connectingWithDrag = false;
        connectionCancelled = true;
        if (nearestIolet) {
            nearestIolet->isTargeted = false;
            nearestIolet->repaint();
            nearestIolet = nullptr;
        }
    }
}

void Canvas::alignObjects(Align const alignment)
{
    auto selectedObjects = getSelectionOfType<Object>();

    if (selectedObjects.size() < 2)
        return;

    auto sortByXPos = [](SmallArray<Object*>& objects) {
        objects.sort([](auto const& a, auto const& b) {
            auto aX = a->getBounds().getX();
            auto bX = b->getBounds().getX();
            if (aX == bX) {
                return a->getBounds().getY() < b->getBounds().getY();
            }
            return aX < bX;
        });
    };

    auto sortByYPos = [](SmallArray<Object*>& objects) {
        objects.sort([](auto const& a, auto const& b) {
            auto aY = a->getBounds().getY();
            auto bY = b->getBounds().getY();
            if (aY == bY)
                return a->getBounds().getX() < b->getBounds().getX();

            return aY < bY;
        });
    };

    auto getBoundingBox = [](SmallArray<Object*>& objects) -> Rectangle<int> {
        auto totalBounds = Rectangle<int>();
        for (auto const* object : objects) {
            if (object->getPointer()) {
                totalBounds = totalBounds.getUnion(object->getBounds());
            }
        }
        return totalBounds;
    };

    patch.startUndoSequence("Align objects");

    // mark canvas as dirty, and set undo for all positions
    if (auto patchPtr = patch.getPointer()) {
        canvas_dirty(patchPtr.get(), 1);
        for (auto const object : objects) {
            if (auto* ptr = object->getPointer())
                pd::Interface::undoApply(patchPtr.get(), ptr);
        }
    }

    // get the bounding box of all selected objects
    auto const selectedBounds = getBoundingBox(selectedObjects);

    auto onMove = [this, selectedObjects](Point<int> const position) {
        // Calculate the bounding box of all selected objects
        Rectangle<int> totalSize;

        for (auto const obj : selectedObjects) {
            totalSize = totalSize.getUnion(obj->getBounds());
        }

        // Determine the offset for each object from the top-left of the total bounding box
        Array<Point<int>> offsets;
        for (auto const obj : selectedObjects) {
            offsets.add(obj->getPosition() - totalSize.getPosition());
        }

        // Move each object to the new position, maintaining its relative offset
        for (int i = 0; i < selectedObjects.size(); i++) {
            auto const* obj = selectedObjects[i];
            patch.moveObjectTo(obj->getPointer(), position.x + offsets[i].x, position.y + offsets[i].y);
        }

        synchronise();
    };

    switch (alignment) {
    case Align::Left: {
        auto const leftPos = selectedBounds.getTopLeft().getX();
        for (auto const* object : selectedObjects) {
            patch.moveObjectTo(object->getPointer(), leftPos, object->getBounds().getY());
        }
        break;
    }
    case Align::Right: {
        auto const rightPos = selectedBounds.getRight();
        for (auto const* object : selectedObjects) {
            auto objectBounds = object->getBounds();
            patch.moveObjectTo(object->getPointer(), rightPos - objectBounds.getWidth(), objectBounds.getY());
        }
        break;
    }
    case Align::VCentre: {
        auto const centrePos = selectedBounds.getCentreX();
        for (auto const* object : selectedObjects) {
            auto objectBounds = object->getBounds();
            patch.moveObjectTo(object->getPointer(), centrePos - objectBounds.withZeroOrigin().getCentreX(), objectBounds.getY());
        }
        break;
    }
    case Align::Top: {
        auto const topPos = selectedBounds.getTopLeft().y;
        for (auto const* object : selectedObjects) {
            patch.moveObjectTo(object->getPointer(), object->getX(), topPos);
        }
        break;
    }
    case Align::Bottom: {
        auto const bottomPos = selectedBounds.getBottom();
        for (auto const* object : selectedObjects) {
            auto objectBounds = object->getBounds();
            patch.moveObjectTo(object->getPointer(), objectBounds.getX(), bottomPos - objectBounds.getHeight());
        }
        break;
    }
    case Align::HCentre: {
        auto const centerPos = selectedBounds.getCentreY();
        for (auto const* object : selectedObjects) {
            auto objectBounds = object->getBounds();
            patch.moveObjectTo(object->getPointer(), objectBounds.getX(), centerPos - objectBounds.withZeroOrigin().getCentreY());
        }
        break;
    }
    case Align::HDistribute: {
        sortByXPos(selectedObjects);

        auto onResize = [this, selectedObjects](Rectangle<int> const newBounds) {
            int totalObjectsWidth = 0;
            for (auto const* obj : selectedObjects) {
                totalObjectsWidth += obj->getBounds().getWidth() - Object::doubleMargin;
            }

            int const totalSpacing = newBounds.getWidth() - totalObjectsWidth;
            float const spacer = selectedObjects.size() > 1 ? static_cast<float>(totalSpacing) / (selectedObjects.size() - 1) : 0;

            float offsetX = newBounds.getX() - Object::margin;
            for (int i = 0; i < selectedObjects.size(); i++) {
                auto const* obj = selectedObjects[i];

                // Set the object position
                if (i == selectedObjects.size() - 1) {
                    patch.moveObjectTo(obj->getPointer(), newBounds.getRight() - obj->getWidth() + Object::margin, obj->getBounds().getY());
                } else {
                    patch.moveObjectTo(obj->getPointer(), std::max(newBounds.toFloat().getX() - Object::margin, offsetX), obj->getBounds().getY());
                    offsetX += obj->getBounds().getWidth() + spacer - Object::doubleMargin;
                }
            }
            synchronise();
            return spacer;
        };
        objectsDistributeResizer.reset(std::make_unique<ObjectsResizer>(this, onResize, onMove).release());
        break;
    }
    case Align::VDistribute: {
        sortByYPos(selectedObjects);

        auto onResize = [this, selectedObjects](Rectangle<int> const newBounds) {
            int totalObjectsHeight = 0;
            for (auto const* obj : selectedObjects) {
                totalObjectsHeight += obj->getBounds().getHeight() - Object::doubleMargin;
            }

            int const totalSpacing = newBounds.getHeight() - totalObjectsHeight;
            float const spacer = selectedObjects.size() > 1 ? static_cast<float>(totalSpacing) / (selectedObjects.size() - 1) : 0;

            float offsetY = newBounds.getY() - Object::margin;
            for (int i = 0; i < selectedObjects.size(); i++) {
                auto const* obj = selectedObjects[i];

                // Set the object position
                if (i == selectedObjects.size() - 1) {
                    patch.moveObjectTo(obj->getPointer(), obj->getBounds().getX(), newBounds.getBottom() - obj->getHeight() + Object::margin);
                } else {
                    patch.moveObjectTo(obj->getPointer(), obj->getBounds().getX(), std::max(newBounds.toFloat().getY() - Object::margin, offsetY));
                    offsetY += obj->getBounds().getHeight() + spacer - Object::doubleMargin;
                }
            }
            synchronise();
            return spacer;
        };
        objectsDistributeResizer.reset(std::make_unique<ObjectsResizer>(this, onResize, onMove, ObjectsResizer::ResizerMode::Vertical).release());
        break;
    }
    default:
        break;
    }

    performSynchronise();

    for (auto* connection : connections) {
        connection->forceUpdate();
    }

    patch.endUndoSequence("Align objects");
}

void Canvas::undo()
{

    // If there is an object with an active editor, we interpret undo as wanting to undo the creation of that object editor
    // This is because the initial object editor is not communicated with Pd, so we can't rely on patch undo to do that
    // If we don't do this, it will undo the old last action before creating this editor, which would be confusing
    for (auto const object : objects) {
        if (object->isInitialEditorShown()) {
            object->hideEditor();
        }
    }

    // Tell pd to undo the last action
    patch.undo();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();

    synchroniseSplitCanvas();
    updateSidebarSelection();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();

    // Load state from pd
    synchronise();
    handleUpdateNowIfNeeded();

    patch.deselectAll();

    synchroniseSplitCanvas();
    updateSidebarSelection();
}

void Canvas::valueChanged(Value& v)
{
    // Update zoom
    if (v.refersToSameSourceAs(zoomScale)) {
        editor->statusbar->updateZoomLevel();
        patch.lastViewportScale = getValue<float>(zoomScale);
        hideSuggestions();
    } else if (v.refersToSameSourceAs(patchWidth)) {
        // limit canvas width to smallest object (11px)
        patchWidth = jmax(11, getValue<int>(patchWidth));
        if (auto cnv = patch.getPointer()) {
            auto x1 = static_cast<float>(cnv->gl_screenx1);
            auto y1 = static_cast<float>(cnv->gl_screeny1);
            auto x2 = static_cast<float>(getValue<int>(patchWidth) + x1);
            auto y2 = static_cast<float>(cnv->gl_screeny2);

            char buf[MAXPDSTRING];
            snprintf(buf, MAXPDSTRING - 1, ".x%lx", reinterpret_cast<unsigned long>(cnv.get()));
            pd->sendMessage(buf, "setbounds", { x1, y1, x2, y2 });
        }
        repaint();
    } else if (v.refersToSameSourceAs(patchHeight)) {
        patchHeight = jmax(11, getValue<int>(patchHeight));
        if (auto cnv = patch.getPointer()) {
            auto x1 = static_cast<float>(cnv->gl_screenx1);
            auto y1 = static_cast<float>(cnv->gl_screeny1);
            auto x2 = static_cast<float>(cnv->gl_screenx2);
            auto y2 = static_cast<float>(getValue<int>(patchHeight) + y1);

            char buf[MAXPDSTRING];
            snprintf(buf, MAXPDSTRING - 1, ".x%lx", reinterpret_cast<unsigned long>(cnv.get()));
            pd->sendMessage(buf, "setbounds", { x1, y1, x2, y2 });
        }
        repaint();
    }
    // When lock changes
    else if (v.refersToSameSourceAs(locked)) {
        bool const editMode = !getValue<bool>(v);

        if (auto ptr = patch.getPointer()) {
            pd->sendDirectMessage(ptr.get(), "editmode", { static_cast<float>(editMode) });
        }

        cancelConnectionCreation();
        deselectAll();

        // Makes sure no objects keep keyboard focus after locking/unlocking
        if (isShowing() && isVisible())
            grabKeyboardFocus();

        editor->updateCommandStatus();
        updateOverlays();
        orderConnections();
    } else if (v.refersToSameSourceAs(commandLocked)) {
        updateOverlays();
        repaint();
    }
    // Should only get called when the canvas isn't a real graph
    else if (v.refersToSameSourceAs(presentationMode)) {
        connectionLayer.setVisible(!getValue<bool>(presentationMode));
        deselectAll();
    } else if (v.refersToSameSourceAs(hideNameAndArgs)) {
        if (!patch.getPointer())
            return;

        int hideText = getValue<bool>(hideNameAndArgs);
        if (auto glist = patch.getPointer()) {
            hideText = glist->gl_isgraph && hideText;
            canvas_setgraph(glist.get(), glist->gl_isgraph + 2 * hideText, 0);
        }

        hideNameAndArgs = hideText;
    } else if (v.refersToSameSourceAs(isGraphChild)) {
        if (!patch.getPointer())
            return;

        int const graphChild = getValue<bool>(isGraphChild);

        if (auto glist = patch.getPointer()) {
            canvas_setgraph(glist.get(), graphChild + 2 * (graphChild && glist->gl_hidetext), 0);
        }

        if (!graphChild) {
            hideNameAndArgs = false;
        }

        if (graphChild && !isGraph) {
            graphArea = std::make_unique<GraphArea>(this);
            addAndMakeVisible(*graphArea);
            graphArea->setAlwaysOnTop(true);
            graphArea->updateBounds();
        } else {
            graphArea.reset(nullptr);
        }

        updateOverlays();
        repaint();
    } else if (v.refersToSameSourceAs(xRange)) {
        if (auto glist = patch.getPointer()) {
            glist->gl_x1 = static_cast<float>(xRange.getValue().getArray()->getReference(0));
            glist->gl_x2 = static_cast<float>(xRange.getValue().getArray()->getReference(1));
        }
        updateDrawables();
    } else if (v.refersToSameSourceAs(yRange)) {
        if (auto glist = patch.getPointer()) {
            glist->gl_y2 = static_cast<float>(yRange.getValue().getArray()->getReference(0));
            glist->gl_y1 = static_cast<float>(yRange.getValue().getArray()->getReference(1));
        }
        updateDrawables();
    }
}

void Canvas::orderConnections()
{
    // move connection layer to back when canvas is locked & connections behind is active
    if (connectionsBehind) {
        connectionLayer.toBack();
    } else
        objectLayer.toBack();

    repaint();
}

void Canvas::showSuggestions(Object* object, TextEditor* textEditor)
{
    suggestor->createCalloutBox(object, textEditor);
}
void Canvas::hideSuggestions()
{
    suggestor->removeCalloutBox();
}

// Makes component selected
void Canvas::setSelected(Component* component, bool const shouldNowBeSelected, bool const updateCommandStatus, bool const broadcastChange)
{
    if (!broadcastChange) {
        selectedComponents.removeChangeListener(this);
    }

    if (!shouldNowBeSelected) {
        selectedComponents.deselect(component);
    } else {
        selectedComponents.addToSelection(component);
    }

    if (updateCommandStatus) {
        editor->updateCommandStatus();
    }

    if (!broadcastChange) {
        // Add back the listener, but make sure it's added back 'after' the last event on the message queue
        MessageManager::callAsync([this] { selectedComponents.addChangeListener(this); });
    }
}

SelectedItemSet<WeakReference<Component>>& Canvas::getLassoSelection()
{
    return selectedComponents;
}

bool Canvas::checkPanDragMode()
{
    auto const panDragEnabled = panningModifierDown();
    setPanDragMode(panDragEnabled);

    return panDragEnabled;
}

bool Canvas::setPanDragMode(bool const shouldPan)
{
    if (auto* v = dynamic_cast<CanvasViewport*>(viewport.get())) {
        v->enableMousePanning(shouldPan);
        return true;
    }
    return false;
}

bool Canvas::isPointOutsidePluginArea(Point<int> const point) const
{
    auto const borderWidth = getValue<float>(patchWidth);
    auto const borderHeight = getValue<float>(patchHeight);
    constexpr auto halfSize = infiniteCanvasSize / 2;
    constexpr auto pos = Point<int>(halfSize, halfSize);

    auto const pluginBounds = Rectangle<int>(pos.x, pos.y, borderWidth, borderHeight);

    return !pluginBounds.contains(point);
}

void Canvas::findLassoItemsInArea(Array<WeakReference<Component>>& itemsFound, Rectangle<int> const& area)
{
    auto const lassoBounds = area.withWidth(jmax(2, area.getWidth())).withHeight(jmax(2, area.getHeight()));

    if (!altDown) { // Alt enable connection only mode
        for (auto* object : objects) {
            if (lassoBounds.intersects(object->getSelectableBounds())) {
                itemsFound.add(object);
            } else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown()) {
                setSelected(object, false, false);
            }
        }
    }

    auto const anyModifiersDown = ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown();
    auto const canSelectConnections = itemsFound.isEmpty() || anyModifiersDown;

    for (auto const& connection : connections) {
        // If total bounds don't intersect, there can't be an intersection with the line
        // This is cheaper than checking the path intersection, so do this first
        if (!connection->getBounds().intersects(lassoBounds)) {
            setSelected(connection, false, false);
            continue;
        }

        // Check if path intersects with lasso
        if (canSelectConnections && connection->intersects(lassoBounds.toFloat())) {
            itemsFound.add(connection);
        } else if (!anyModifiersDown) {
            setSelected(connection, false, false);
        }
    }
}

ObjectParameters& Canvas::getInspectorParameters()
{
    return parameters;
}

bool Canvas::panningModifierDown() const
{
#if JUCE_IOS
    return OSUtils::ScrollTracker::isScrolling();
#endif
    
    auto const& commandManager = editor->commandManager;
    // check the command manager for the keycode that is assigned to pan drag key
    auto const panDragKeycode = commandManager.getKeyMappings()->getKeyPressesAssignedToCommand(CommandIDs::PanDragKey).getFirst().getKeyCode();

    // get the current modifier keys, removing the left mouse button modifier (as that is what is needed to activate a pan drag with key down)
    auto const currentMods = ModifierKeys(ModifierKeys::getCurrentModifiers().getRawFlags() & ~ModifierKeys::leftButtonModifier);

    bool isPanDragKeysActive = false;

    if (KeyPress::isKeyCurrentlyDown(panDragKeycode)) {
        // construct a fake keypress with the current pan drag keycode key, with current modifiers, to test if it matches the command id's code & mods
        auto const keyWithMod = KeyPress(panDragKeycode, currentMods, 0);
        isPanDragKeysActive = commandManager.getKeyMappings()->containsMapping(CommandIDs::PanDragKey, keyWithMod);
    }

    return isPanDragKeysActive || ModifierKeys::getCurrentModifiers().isMiddleButtonDown();
}

void Canvas::receiveMessage(t_symbol* symbol, SmallArray<pd::Atom> const& atoms)
{
    switch (hash(symbol->s_name)) {
    case hash("sync"):
    case hash("obj"):
    case hash("msg"):
    case hash("floatatom"):
    case hash("listbox"):
    case hash("symbolatom"):
    case hash("text"):
    case hash("graph"):
    case hash("scalar"):
    case hash("bng"):
    case hash("toggle"):
    case hash("vslider"):
    case hash("hslider"):
    case hash("hdial"):
    case hash("vdial"):
    case hash("hradio"):
    case hash("vradio"):
    case hash("vumeter"):
    case hash("mycnv"):
    case hash("numbox"):
    case hash("connect"):
    case hash("clear"):
    case hash("cut"):
    case hash("disconnect"): {
        // This will trigger an asyncupdater, so it's thread-safe to do this here
        synchronise();
        break;
    }
    case hash("editmode"): {
        if (::getValue<bool>(commandLocked))
            return;

        if (atoms.size() >= 1) {
            if (int const flag = atoms[0].getFloat(); flag % 2 == 0) {
                locked = true;
            } else {
                locked = false;
                presentationMode = false;
            }
        }
        break;
    }
    case hash("setbounds"): {
        if (atoms.size() >= 4) {
            auto const width = atoms[2].getFloat() - atoms[0].getFloat();
            auto const height = atoms[3].getFloat() - atoms[1].getFloat();
            setValueExcludingListener(patchWidth, static_cast<int>(width), this);
            setValueExcludingListener(patchHeight, static_cast<int>(height), this);
            repaint();
        }

        break;
    }
    case hash("coords"):
    case hash("donecanvasdialog"): {
        synchroniseSplitCanvas();
        break;
    }
    default:
        break;
    }
}

void Canvas::resized()
{
    connectionLayer.setBounds(getLocalBounds());
    objectLayer.setBounds(getLocalBounds());
}

void Canvas::activateCanvasSearchHighlight(Object* obj)
{
    canvasSearchHighlight.reset(std::make_unique<CanvasSearchHighlight>(this, obj).release());
}

void Canvas::removeCanvasSearchHighlight()
{
    if (canvasSearchHighlight)
        canvasSearchHighlight.reset(nullptr);
}
