/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include "Canvas.h"

extern "C"
{
#include <m_pd.h>
#include <m_imp.h>
}

#include "Box.h"
#include "Connection.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"

// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component, public KeyListener, public TextEditor::InputFilter
{
    class Suggestion : public TextButton
    {
        int idx = 0;
        int type = -1;
        Array<Colour> colours = {findColour(ScrollBar::thumbColourId), Colours::yellow};

        Array<String> letters = {"pd", "~"};

        String objectDescription;

       public:
        Suggestion(int i) : idx(i)
        {
            setText("", "");
            setWantsKeyboardFocus(true);
            setConnectedEdges(12);
            setClickingTogglesState(true);
            setRadioGroupId(1001);
            setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
        }

        void setText(const String& name, const String& description)
        {
            objectDescription = description;
            setButtonText(name);
            type = name.contains("~") ? 1 : 0;

            repaint();
        }

        void paint(Graphics& g) override
        {
             auto colour = idx & 1 ? PlugDataColour::toolbarColourId :  PlugDataColour::canvasColourId;
            
            getLookAndFeel().drawButtonBackground(g, *this, findColour(getToggleState() ? Slider::thumbColourId : colour), isMouseOver(), isMouseButtonDown());

            
            getLookAndFeel().drawButtonText(g, *this, isMouseOver(), false);

            if (objectDescription.isNotEmpty())
            {
                auto font = getLookAndFeel().getTextButtonFont(*this, getHeight());
                auto textLength = font.getStringWidth(getButtonText()) + 22;

                g.setColour(findColour(PlugDataColour::canvasOutlineColourId));

                auto yIndent = jmin(4, proportionOfHeight(0.3f));
                auto cornerSize = jmin(getHeight(), getWidth()) / 2;
                auto fontHeight = roundToInt(font.getHeight() * 0.5f);
                auto leftIndent = textLength + 10;
                auto rightIndent = jmin(fontHeight, 2 + cornerSize / 2);
                auto textWidth = getWidth() - leftIndent - rightIndent;

                g.drawText("- " + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2), Justification::left);
            }

            if (type == -1) return;

            g.setColour(colours[type].withAlpha(float(0.8)));
            Rectangle<int> iconbound = getLocalBounds().reduced(4);
            iconbound.setWidth(getHeight() - 8);
            iconbound.translate(3, 0);
            g.fillRect(iconbound);

            g.setColour(Colours::white);
            g.drawFittedText(letters[type], iconbound.reduced(1), Justification::centred, 1);
        }
    };

   public:
    bool selecting = false;

    SuggestionComponent() : resizer(this, &constrainer), currentBox(nullptr)
    {
        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();

        for (int i = 0; i < 20; i++)
        {
            Suggestion* but = buttons.add(new Suggestion(i));
            buttonholder->addAndMakeVisible(buttons[i]);

            but->setClickingTogglesState(true);
            but->setRadioGroupId(110);
            but->setName("suggestions:button");

            // Colour pattern
            but->setColour(TextButton::buttonColourId, colours[i % 2]);
        }

        // Set up viewport
        port = std::make_unique<Viewport>();
        port->setScrollBarsShown(true, false);
        port->setViewedComponent(buttonholder.get(), false);
        port->setInterceptsMouseClicks(true, true);
        port->setViewportIgnoreDragFlag(true);
        addAndMakeVisible(port.get());

        constrainer.setSizeLimits(150, 120, 500, 400);
        setSize(250, 115);

        addAndMakeVisible(resizer);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
    }

    ~SuggestionComponent() override
    {
        buttons.clear();
    }

    void createCalloutBox(Box* box, TextEditor* editor)
    {
        currentBox = box;
        openedEditor = editor;

        editor->setInputFilter(this, false);
        editor->addKeyListener(this);

        // Should run after the input filter
        editor->onTextChange = [this, editor, box]()
        {
            if (isCompleting && !editor->getText().containsChar(' '))
            {
                editor->setHighlightedRegion({highlightStart, highlightEnd});
            }
            auto width = editor->getTextWidth() + 10;

            if (width > box->getWidth())
            {
                box->setSize(width, box->getHeight());
            }
        };

        for (int i = 0; i < buttons.size(); i++)
        {
            auto* but = buttons[i];
            but->setAlwaysOnTop(true);

            but->onClick = [this, i, editor]() mutable
            {
                move(0, i);
                if (!editor->isVisible()) editor->setVisible(true);
                editor->grabKeyboardFocus();
            };
        }

        // buttons[0]->setToggleState(true, sendNotification);
        // setVisible(editor->getText().isNotEmpty());

        addToDesktop(ComponentPeer::StyleFlags::windowIsTemporary | ComponentPeer::StyleFlags::windowIgnoresKeyPresses);
        repaint();
    }

    void move(int offset, int setto = -1)
    {
        if (!openedEditor) return;

        // Calculate new selected index
        if (setto == -1)
            currentidx += offset;
        else
            currentidx = setto;

        if (numOptions == 0) return;

        // Limit it to minimum of the number of buttons and the number of suggestions
        int numButtons = std::min(20, numOptions);
        currentidx = (currentidx + numButtons) % numButtons;

        auto* but = buttons[currentidx];

        // If we use setto, the toggle state should already be set
        if (setto == -1) but->setToggleState(true, dontSendNotification);

        if (openedEditor)
        {
            String newText = buttons[currentidx]->getButtonText();
            openedEditor->setText(newText, dontSendNotification);
            highlightEnd = newText.length();
            openedEditor->setHighlightedRegion({highlightStart, highlightEnd});
        }

        // Auto-scroll item into viewport bounds
        if (port->getViewPositionY() > but->getY())
        {
            port->setViewPosition(0, but->getY());
        }
        else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight())
        {
            port->setViewPosition(0, but->getY() - (but->getHeight() * 4));
        }
    }

    TextEditor* openedEditor = nullptr;
    Box* currentBox;

    void resized() override
    {
        int yScroll = port->getViewPositionY();
        port->setBounds(getLocalBounds());
        buttonholder->setBounds(0, 0, getWidth(), std::min(numOptions, 20) * 22 + 2);

        for (int i = 0; i < buttons.size(); i++) buttons[i]->setBounds(2, (i * 22) + 2, getWidth() - 2, 23);

        const int resizerSize = 12;

        resizer.setBounds(getWidth() - (resizerSize + 1), getHeight() - (resizerSize + 1), resizerSize, resizerSize);

        port->setViewPosition(0, yScroll);
        repaint();
    }

   private:
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(port->getBounds());
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(bordercolor);
        g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), 3.0f, 2.5f);
    }

    bool keyPressed(const KeyPress& key, Component* originatingComponent) override
    {
        if (key == KeyPress::upKey || key == KeyPress::downKey)
        {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }
        return false;
    }

    String filterNewText(TextEditor& e, const String& newInput) override
    {
        String mutableInput = newInput;

        // Find start of highlighted region
        // This is the start of the last auto-completion suggestion
        // This region will automatically be removed after this function because it's selected
        int start = e.getHighlightedRegion().getLength() > 0 ? e.getHighlightedRegion().getStart() : e.getText().length();

        // Reconstruct users typing
        String typedText = e.getText().substring(0, start) + mutableInput;
        highlightStart = typedText.length();

        auto& library = currentBox->cnv->pd->objectLibrary;
        // Update suggestions
        auto found = library.autocomplete(typedText.toStdString());

        for (int i = 0; i < std::min<int>(buttons.size(), found.size()); i++)
        {
            auto& [name, autocomplete] = found[i];

            if (library.objectDescriptions.find(name) != library.objectDescriptions.end())
            {
                buttons[i]->setText(name, library.objectDescriptions[name]);
            }
            else
            {
                buttons[i]->setText(name, "");
            }
        }

        for (int i = found.size(); i < buttons.size(); i++) buttons[i]->setText("", "");

        numOptions = found.size();

        setVisible(typedText.isNotEmpty() && numOptions);

        constrainer.setSizeLimits(150, 100, 500, 400);
        resized();

        // Get length of user-typed text
        int textlen = e.getText().substring(0, start).length();

        // Retrieve best suggestion
        if (currentidx >= found.size() || textlen == 0)
        {
            highlightEnd = 0;
            return mutableInput;
        }

        String fullName = found[currentidx].first;

        highlightEnd = fullName.length();

        if (!mutableInput.containsNonWhitespaceChars() || (e.getText() + mutableInput).contains(" ") || !found[currentidx].second)

        {
            isCompleting = false;
            return mutableInput;
        }

        isCompleting = true;
        mutableInput = fullName.substring(textlen);

        return mutableInput;
    }

    bool running = false;
    int numOptions = 0;
    int currentidx = 0;

    std::unique_ptr<Viewport> port;
    std::unique_ptr<Component> buttonholder;
    OwnedArray<Suggestion> buttons;

    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;

    Array<Colour> colours = {findColour(PlugDataColour::toolbarColourId), findColour(PlugDataColour::canvasColourId)};

    Colour bordercolor = Colour(142, 152, 155);

    int highlightStart = 0;
    int highlightEnd = 0;

    bool isCompleting = false;
};

// Graph bounds component
struct GraphArea : public Component, public ComponentDragger
{
    ResizableCornerComponent resizer;
    Canvas* canvas;

    explicit GraphArea(Canvas* parent) : resizer(this, nullptr), canvas(parent)
    {
        addAndMakeVisible(resizer);
        updateBounds();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(Slider::thumbColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), 2.0f, 4.0f);
    }

    bool hitTest(int x, int y) override
    {
        return !getLocalBounds().reduced(8).contains(Point<int>{x, y});
    }

    void mouseMove(const MouseEvent& e) override
    {
        if (canvas->locked == var(false))
        {
            setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
        }
    }

    void mouseDown(const MouseEvent& e) override
    {
        startDraggingComponent(this, e);
    }

    void mouseDrag(const MouseEvent& e) override
    {
        dragComponent(this, e, nullptr);
    }

    void mouseUp(const MouseEvent& e) override
    {
        setPdBounds();
        repaint();
    }

    void resized() override
    {
        int handleSize = 20;

        setPdBounds();
        resizer.setBounds(getWidth() - handleSize, getHeight() - handleSize, handleSize, handleSize);

        repaint();
    }

    void setPdBounds()
    {
        t_canvas* cnv = canvas->patch.getPointer();
        // TODO: make this thread safe
        if (cnv)
        {
            cnv->gl_pixwidth = getWidth();
            cnv->gl_pixheight = getHeight();
            cnv->gl_xmargin = getX() - canvas->canvasOrigin.x;
            cnv->gl_ymargin = getY() - canvas->canvasOrigin.y;
        }
    }

    void updateBounds()
    {
        setBounds(canvas->patch.getBounds().translated(canvas->canvasOrigin.x, canvas->canvasOrigin.y));
    }
};

Canvas::Canvas(PlugDataPluginEditor& parent, pd::Patch p, bool graph, bool graphChild) : main(parent), pd(&parent.pd), patch(std::move(p)), storage(patch.getPointer(), pd)
{
    isGraph = graph;
    isGraphChild = graphChild;

    suggestor = new SuggestionComponent;

    commandLocked.referTo(pd->commandLocked);
    commandLocked.addListener(this);
    
    locked.referTo(pd->locked);
    locked.addListener(this);

    connectionStyle.referTo(parent.pd.settingsTree.getPropertyAsValue("ConnectionStyle", nullptr));
    connectionStyle.addListener(this);

    tabbar = &parent.tabbar;

    // Add draggable border for setting graph position
    if (isGraphChild)
    {
        graphArea = new GraphArea(this);
        addAndMakeVisible(graphArea);
        graphArea->setAlwaysOnTop(true);
    }

    setSize(600, 400);

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha(0.3f));

    setWantsKeyboardFocus(true);

    if (!isGraph)
    {
        viewport = new Viewport;  // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);
        viewport->setBufferedToImage(true); // makes scrolling much smoother

        // Apply zooming
        setTransform(parent.transform);
        presentationMode.referTo(parent.statusbar.presentationMode);
        presentationMode.addListener(this);
    }
    else
    {
        presentationMode = false;
    }

    synchronise();
}

Canvas::~Canvas()
{
    delete graphArea;
    delete suggestor;
}

void Canvas::paint(Graphics& g)
{
    if (!isGraph)
    {
        g.fillAll(findColour(PlugDataColour::toolbarColourId));

        g.setColour(findColour(PlugDataColour::canvasColourId));
        g.fillRect(canvasOrigin.x, canvasOrigin.y, getWidth(), getHeight());

        // draw origin
        g.setColour(Colour(100, 100, 100));
        g.drawLine(canvasOrigin.x - 1, canvasOrigin.y - 1, canvasOrigin.x - 1, getHeight() + 2);
        g.drawLine(canvasOrigin.x - 1, canvasOrigin.y - 1, getWidth() + 2, canvasOrigin.y - 1);
    }

    
    if (locked == var(false) && commandLocked == var(false) && !isGraph)
    {
        const int gridSize = 25;
        const Rectangle<int> clipBounds = g.getClipBounds();

        g.setColour(findColour(PlugDataColour::toolbarColourId).contrasting(0.4));

        for (int x = canvasOrigin.getX() + gridSize; x < clipBounds.getRight(); x += gridSize)
        {
            for (int y = canvasOrigin.getY() + gridSize; y < clipBounds.getBottom(); y += gridSize)
            {
                g.fillRect(x, y, 1, 1);
            }
        }
    }
}

void Canvas::focusGained(FocusChangeType cause)
{
    // This is necessary because in some cases, setting the canvas as current right before an action isn't enough
    pd->setThis();
    if (patch.getPointer() && !isGraph)
    {
        patch.setCurrent(true);
    }
}

// Synchronise state with pure-data
// Used for loading and for complicated actions like undo/redo
void Canvas::synchronise(bool updatePosition)
{
    pd->waitForStateUpdate();
    deselectAll();

    patch.setCurrent(true);

    auto objects = patch.getObjects();
    auto isObjectDeprecated = [&](pd::Object* obj)
    {
        // NOTE: replace with std::ranges::all_of when available
        for (auto& pdobj : objects)
        {
            if (pdobj == *obj)
            {
                return false;
            }
        }
        return true;
    };

    if (!(isGraph || presentationMode == var(true)))
    {
        // Remove deprecated connections
        for (int n = connections.size() - 1; n >= 0; n--)
        {
            auto connection = connections[n];

            if (isObjectDeprecated(connection->inlet->box->pdObject.get()) || isObjectDeprecated(connection->outlet->box->pdObject.get()))
            {
                connections.remove(n);
            }
            else
            {
                auto* inlet = static_cast<t_text*>(connection->inlet->box->pdObject->getPointer());
                auto* outlet = static_cast<t_text*>(connection->outlet->box->pdObject->getPointer());

                if (!canvas_isconnected(patch.getPointer(), outlet, connection->outIdx, inlet, connection->inIdx))
                {
                    connections.remove(n);
                }
            }
        }
    }

    // Clear deleted boxes
    for (int n = boxes.size() - 1; n >= 0; n--)
    {
        auto* box = boxes[n];
        if (box->pdObject && isObjectDeprecated(box->pdObject.get()))
        {
            boxes.remove(n);
        }
    }

    for (auto& object : objects)
    {
        auto it = std::find_if(boxes.begin(), boxes.end(), [&object](Box* b) { return b->pdObject && *b->pdObject == object; });

        if (it == boxes.end())
        {
            auto name = String(object.getText());

            auto type = pd::Gui::getType(object.getPointer());
            auto isGui = type != pd::Type::Undefined;
            auto* pdObject = isGui ? new pd::Gui(object.getPointer(), &patch, pd) : new pd::Object(object);

            auto* newBox = boxes.add(new Box(pdObject, this, name));
            newBox->toFront(false);

            if (newBox->graphics && newBox->graphics->label) newBox->graphics->label->toFront(false);

            // Don't show non-patchable (internal) objects
            if (!pd::Patch::checkObject(&object)) newBox->setVisible(false);
        }
        else
        {
            auto* box = *it;

            // Only update positions if we need to and there is a significant difference
            // There may be rounding errors when scaling the gui, this makes the experience smoother
            if (updatePosition) box->updateBounds(false);

            box->toFront(false);
            if (box->graphics && box->graphics->label) box->graphics->label->toFront(false);

            // Don't show non-patchable (internal) objects
            if (!pd::Patch::checkObject(&object)) box->setVisible(false);
        }
    }

    // Make sure objects have the same order
    std::sort(boxes.begin(), boxes.end(),
              [&objects](Box* first, Box* second) mutable
              {
                  size_t idx1 = std::find(objects.begin(), objects.end(), *first->pdObject) - objects.begin();
                  size_t idx2 = std::find(objects.begin(), objects.end(), *second->pdObject) - objects.begin();

                  return idx1 < idx2;
              });

    auto pdConnections = patch.getConnections();

    if (!(isGraph || presentationMode == var(true)))
    {
        for (auto& connection : pdConnections)
        {
            auto& [inno, inobj, outno, outobj] = connection;

            int srcno = patch.getIndex(&inobj->te_g);
            int sinkno = patch.getIndex(&outobj->te_g);

            auto& srcEdges = boxes[srcno]->edges;
            auto& sinkEdges = boxes[sinkno]->edges;

            // TEMP: remove when we're sure this works
            if (srcno >= boxes.size() || sinkno >= boxes.size() || outno >= srcEdges.size() || inno >= sinkEdges.size())
            {
                pd->logError("Error: impossible connection");
                continue;
            }

            auto it = std::find_if(connections.begin(), connections.end(),
                                   [this, &connection, &srcno, &sinkno](Connection* c)
                                   {
                                       auto& [inno, inobj, outno, outobj] = connection;

                                       if (!c->inlet || !c->outlet) return false;

                                       bool sameStart = c->outlet->box == boxes[srcno];
                                       bool sameEnd = c->inlet->box == boxes[sinkno];

                                       return c->inIdx == inno && c->outIdx == outno && sameStart && sameEnd;
                                   });

            if (it == connections.end())
            {
                connections.add(new Connection(this, srcEdges[boxes[srcno]->numInputs + outno], sinkEdges[inno], true));
            }
            else
            {
                // Update storage ids for connections
                auto& c = *(*it);

                auto currentId = c.getId();
                if (c.lastId.isNotEmpty() && c.lastId != currentId)
                {
                    storage.setInfoId(c.lastId, currentId);
                }
                
                c.lastId = currentId;

                auto info = storage.getInfo(currentId, "Path");
                if (info.length()) c.setState(info);
                
                c.repaint();
                

            }
        }
        
        storage.confirmIds();

        setTransform(main.transform);
        updateDrawables();
    }

    // patch.deselectAll();

    // Resize canvas to fit objects
    checkBounds();

    for (auto& tmpl : templates)
    {
        tmpl->updateIfMoved();
    }

    main.updateCommandStatus();
    repaint();
}


void Canvas::mouseDown(const MouseEvent& e)
{
    if (suggestor->openedEditor && e.originalComponent != suggestor->openedEditor)
    {
        suggestor->currentBox->hideEditor();
        deselectAll();
        return;
    }
    
    auto openSubpatch = [this](Box* parent)
    {
        if (!parent->graphics) return;

        auto* subpatch = parent->graphics->getPatch();

        for (int n = 0; n < tabbar->getNumTabs(); n++)
        {
            auto* tabCanvas = main.getCanvas(n);
            if (tabCanvas->patch == *subpatch)
            {
                tabbar->setCurrentTabIndex(n);
                return;
            }
        }
        bool isGraphChild = parent->graphics->getGui().getType() == pd::Type::GraphOnParent;
        auto* newCanvas = main.canvases.add(new Canvas(main, *subpatch, false, isGraphChild));

        main.addTab(newCanvas);
        newCanvas->checkBounds();
    };

    auto* source = e.originalComponent;

    // Ignore if locked
    if (locked == var(true))
    {
        if (!ModifierKeys::getCurrentModifiers().isRightButtonDown())
        {
            auto* box = dynamic_cast<Box*>(source);

            if (box && box->graphics && box->graphics->getGui().getType() == pd::Type::Subpatch)
            {
                openSubpatch(box);
            }
        }
        return;
    }

    // Select parent box when clicking on graphs
    if (isGraph)
    {
        auto* box = findParentComponentOfClass<Box>();
        box->cnv->setSelected(box, true);
        return;
    }

    // Left-click
    if (!ModifierKeys::getCurrentModifiers().isRightButtonDown())
    {
        if (locked == var(true))
        {
            if (auto* box = dynamic_cast<Box*>(source))
            {
                openSubpatch(box);
                return;
            }
        }
        // Connecting objects by dragging
        if (source == this || source == graphArea)
        {
            if (connectingEdge)
            {
                connectingEdge = nullptr;
                repaint();
            }

            lasso.beginLasso(e.getEventRelativeTo(this), this);
            if (!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown())
            {
                deselectAll();
            }
        }
    }
    // Right click
    else
    {
        // Info about selection status
        auto& lassoSelection = getLassoSelection();

        auto selectedBoxes = getSelectionOfType<Box>();

        bool hasSelection = !selectedBoxes.isEmpty();
        bool multiple = selectedBoxes.size() > 1;

        Box* box = nullptr;
        if (hasSelection && !multiple) box = selectedBoxes.getFirst();

        bool isSubpatch = box && (box->graphics && (box->graphics->getGui().getType() == pd::Type::GraphOnParent || box->graphics->getGui().getType() == pd::Type::Subpatch));

        bool isGui = hasSelection && !multiple && box->graphics && !box->graphics->fakeGui();

        // Create popup menu
        popupMenu.clear();

        popupMenu.addItem(1, "Open", hasSelection && !multiple && isSubpatch);  // for opening subpatches
        // popupMenu.addItem(10, "Edit", isGui);
        popupMenu.addSeparator();

        popupMenu.addCommandItem(&main, CommandIDs::Cut);
        popupMenu.addCommandItem(&main, CommandIDs::Copy);
        popupMenu.addCommandItem(&main, CommandIDs::Paste);
        popupMenu.addCommandItem(&main, CommandIDs::Duplicate);
        popupMenu.addCommandItem(&main, CommandIDs::Delete);
        popupMenu.addSeparator();

        popupMenu.addItem(8, "To Front", box != nullptr);
        popupMenu.addSeparator();
        popupMenu.addItem(9, "Help", box != nullptr);

        auto callback = [this, &lassoSelection, openSubpatch, box](int result)
        {
            if (result < 1) return;

            switch (result)
            {
                case 1:
                {
                    openSubpatch(box);
                    break;
                }
                case 4:  // Cut
                    copySelection();
                    removeSelection();
                    break;
                case 5:  // Copy
                    copySelection();
                    break;
                case 6:
                {  // Duplicate
                    duplicateSelection();
                    break;
                }
                case 7:  // Remove
                    removeSelection();
                    break;

                case 8:  // To Front
                    box->toFront(false);
                    box->pdObject->toFront();
                    break;

                case 9:
                {  // Open help

                    pd->setThis();
                    // Find name of help file
                    auto helpPatch = box->pdObject->getHelp();

                    if (!helpPatch.getPointer())
                    {
                        pd->logMessage("Couldn't find help file");
                        return;
                    }

                    auto* newCnv = main.canvases.add(new Canvas(main, helpPatch));
                    main.addTab(newCnv, true);

                    break;
                }

                default:
                    break;
            }
        };

        if (auto* box = dynamic_cast<Box*>(source))
        {
            if (box->getCurrentTextEditor()) return;
        }

        popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(&main).withTargetScreenArea(Rectangle<int>(e.getScreenX(), e.getScreenY(), 2, 2)), ModalCallbackFunction::create(callback));
    }
}

void Canvas::mouseDrag(const MouseEvent& e)
{
    // Ignore on graphs or when locked
    if (isGraph || locked == var(true)) return;

    auto* source = e.originalComponent;

    auto viewportEvent = e.getEventRelativeTo(viewport);

    float scrollSpeed = 8.5f;

    // Middle mouse pan
    if (ModifierKeys::getCurrentModifiers().isMiddleButtonDown())
    {
        beginDragAutoRepeat(40);

        auto delta = Point<int>{viewportEvent.getDistanceFromDragStartX(), viewportEvent.getDistanceFromDragStartY()};

        viewport->setViewPosition(viewport->getViewPositionX() + delta.x * (1.0f / scrollSpeed), viewport->getViewPositionY() + delta.y * (1.0f / scrollSpeed));

        return;  // Middle mouse button cancels any other drag actions
    }

    // For fixing coords when zooming
    float scale = (1.0f / static_cast<float>(pd->zoomScale.getValue()));

    // Auto scroll when dragging close to the edge
    if (viewport->autoScroll(viewportEvent.x * scale, viewportEvent.y * scale, 50, scrollSpeed))
    {
        beginDragAutoRepeat(40);
    }

    // Drag lasso
    lasso.dragLasso(e);

    if (connectingWithDrag)
    {
        auto* nearest = Edge::findNearestEdge(this, e.getEventRelativeTo(this).getPosition(), !connectingEdge->isInlet, connectingEdge->box);

        if (connectingWithDrag && nearest && nearestEdge != nearest)
        {
            nearest->isHovered = true;

            if (nearestEdge)
            {
                nearestEdge->isHovered = false;
                nearestEdge->repaint();
            }

            nearestEdge = nearest;
            nearestEdge->repaint();
        }

        repaint();
    }
}

void Canvas::mouseUp(const MouseEvent& e)
{
    if (auto* box = dynamic_cast<Box*>(e.originalComponent))
    {
        if (!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown() && e.getDistanceFromDragStart() < 2)
        {
            deselectAll();
        }

        if (locked == var(false) && !isGraph && box->getParentComponent() == this)
        {
            setSelected(box, true);
        }
    }

    // Releasing a connect by drag action
    if (connectingWithDrag && connectingEdge)
    {
        if (nearestEdge)
        {
            nearestEdge->isHovered = false;
            nearestEdge->repaint();
        }
        auto pos = e.getEventRelativeTo(this).getPosition();
        auto* nearest = Edge::findNearestEdge(this, pos, !connectingEdge->isInlet, connectingEdge->box);

        if (nearest) nearest->createConnection();

        connectingEdge = nullptr;
        connectingWithDrag = false;

        repaint();
    }
    else if(connectingWithDrag && !connectingEdge) {
        connectingWithDrag = false;
    }

    auto lassoSelection = getSelectionOfType<Box>();

    if (lassoSelection.size() == 1)
    {
        auto* box = lassoSelection.getFirst();
        auto params = box->graphics ? box->graphics->getParameters() : ObjectParameters();

        if (!params.empty() || main.sidebar.isPinned())
        {
            main.sidebar.showParameters(params);
        }
        else
        {
            main.sidebar.hideParameters();
        }
    }
    else
    {
        main.sidebar.hideParameters();
    }

    main.updateCommandStatus();

    lasso.endLasso();
}

// Updates pd objects that use the drawing feature
void Canvas::updateDrawables()
{
    templates.clear();
    templates.addArray(findDrawables());

    for (auto& tmpl : templates)
    {
        addAndMakeVisible(tmpl);
        tmpl->setAlwaysOnTop(true);
        tmpl->update();
    }
}

Array<DrawableTemplate*> Canvas::findDrawables()
{
    // Find all drawables (from objects like drawpolygon, filledcurve, etc.)
    // Pd draws this over all siblings, even when drawn inside a graph!

    Array<DrawableTemplate*> result;

    for (auto& box : boxes)
    {
        if (!box->pdObject) continue;

        auto* gobj = static_cast<t_gobj*>(box->pdObject->getPointer());

        // Recurse for graphs
        if (gobj->g_pd == canvas_class)
        {
            if (box->graphics && box->graphics->getGui().getType() == pd::Type::GraphOnParent)
            {
                auto* canvas = box->graphics->getCanvas();

                auto pos = canvas->getLocalPoint(canvas->main.getCurrentCanvas(), canvas->getPosition()) * -1;
                auto bounds = canvas->getParentComponent()->getLocalBounds().withPosition(pos);

                auto subdrawables = canvas->findDrawables();
                result.addArray(subdrawables);
            }
        }
        // Scalar found!
        if (gobj->g_pd == scalar_class)
        {
            auto* x = reinterpret_cast<t_scalar*>(gobj);
            auto* templ = template_findbyname(x->sc_template);
            auto* templatecanvas = template_findcanvas(templ);
            t_gobj* y;
            t_float basex, basey;
            scalar_getbasexy(x, &basex, &basey);

            if (!templatecanvas) continue;

            for (y = templatecanvas->gl_list; y; y = y->g_next)
            {
                const t_parentwidgetbehavior* wb = pd_getparentwidget(&y->g_pd);
                if (!wb) continue;

                result.add(new DrawableTemplate(x, y, this, static_cast<int>(basex), static_cast<int>(basey)));
            }
        }
    }

    return result;
}

void Canvas::paintOverChildren(Graphics& g)
{
    // Draw connections in the making over everything else
    if (connectingEdge)
    {
        Point<float> mousePos = getMouseXYRelative().toFloat();
        Point<int> edgePos = connectingEdge->getCanvasBounds().getCentre();

        Path path;
        path.startNewSubPath(edgePos.toFloat());
        path.lineTo(mousePos);

        g.setColour(Colours::grey);
        g.strokePath(path, PathStrokeType(3.0f));
    }
}

void Canvas::mouseMove(const MouseEvent& e)
{
    if (connectingEdge)
    {
        repaint();
    }
}

bool Canvas::keyPressed(const KeyPress& key)
{
    if (main.getCurrentCanvas() != this || isGraph) return false;

    int keycode = key.getKeyCode();
    // Ignore backspace, arrow keys, return key and more that might cause actions in pd
    if (KeyPress::backspaceKey || KeyPress::leftKey || KeyPress::rightKey || KeyPress::upKey || KeyPress::downKey || KeyPress::pageUpKey || KeyPress::pageDownKey || KeyPress::homeKey || KeyPress::escapeKey || KeyPress::deleteKey || KeyPress::returnKey || KeyPress::tabKey)
    {
        return false;
    }

    patch.keyPress(keycode, key.getModifiers().isShiftDown());

    return false;
}

void Canvas::deselectAll()
{
    // Deselect boxes
    for (auto c : selectedComponents)
        if (c) c->repaint();

    selectedComponents.deselectAll();
    main.sidebar.hideParameters();
}

void Canvas::copySelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to copy
    patch.copy();
    patch.deselectAll();
}

void Canvas::pasteSelection()
{
    // Tell pd to paste
    patch.paste();

    // Load state from pd, don't update positions
    synchronise(false);

    for (auto* box : boxes)
    {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(box->pdObject->getPointer())))
        {
            setSelected(box, true);
        }
    }

    patch.deselectAll();
}

void Canvas::duplicateSelection()
{
    // Tell pd to select all objects that are currently selected
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
            patch.selectObject(box->pdObject.get());
        }
    }

    // Tell pd to duplicate
    patch.duplicate();

    // Load state from pd, don't update positions
    synchronise(false);

    // Select the newly duplicated objects
    for (auto* box : boxes)
    {
        if (glist_isselected(patch.getPointer(), static_cast<t_gobj*>(box->pdObject->getPointer())))
        {
            setSelected(box, true);
        }
    }

    patch.deselectAll();
}

void Canvas::removeSelection()
{
    // Make sure object isn't selected and stop updating gui
    main.sidebar.hideParameters();

    // Make sure nothing is selected
    patch.deselectAll();
    storage.ensureDeselected();

    // Find selected objects and make them selected in pd
    Array<pd::Object*> objects;
    for (auto* sel : getLassoSelection())
    {
        if (auto* box = dynamic_cast<Box*>(sel))
        {
            if (box->pdObject)
            {
                patch.selectObject(box->pdObject.get());
                objects.add(box->pdObject.get());
            }
        }
    }

    // remove selection
    patch.removeSelection();

    // Remove connection afterwards and make sure they aren't already deleted
    for (auto* con : connections)
    {
        if (isSelected(con))
        {
            if (!(objects.contains(con->outlet->box->pdObject.get()) || objects.contains(con->inlet->box->pdObject.get())))
            {
                patch.removeConnection(con->outlet->box->pdObject.get(), con->outIdx, con->inlet->box->pdObject.get(), con->inIdx);
            }
        }
    }

    patch.finishRemove();  // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    patch.deselectAll();
}

void Canvas::undo()
{
    // Performs undo on storage data if the next undo event if a dummy
    storage.undoIfNeeded();
    
    // Tell pd to undo the last action
    patch.undo();

    // Load state from pd
    synchronise();

    patch.deselectAll();
}

void Canvas::redo()
{
    // Performs redo on storage data if the next redo event if a dummy
    storage.redoIfNeeded();
    
    // Tell pd to undo the last action
    patch.redo();

    // Load state from pd
    synchronise();

    patch.deselectAll();
}

void Canvas::checkBounds()
{
    if (isGraph || !viewport) return;
    
    updatingBounds = true;

    float scale = (1.0f / static_cast<float>(pd->zoomScale.getValue()));

    auto viewBounds = Rectangle<int>(canvasOrigin.x, canvasOrigin.y, viewport->getMaximumVisibleWidth() * scale, viewport->getMaximumVisibleHeight() * scale);

    for (auto obj : boxes)
    {
        viewBounds = obj->getBounds().reduced(Box::margin).getUnion(viewBounds);
    }

    canvasOrigin -= {viewBounds.getX(), viewBounds.getY()};
    setSize(viewBounds.getWidth(), viewBounds.getHeight());

    for (auto& box : boxes)
    {
        box->updateBounds(false);
    }
    

    if (graphArea)
    {
        graphArea->updateBounds();
    }

    for (auto& tmpl : templates)
    {
        tmpl->updateIfMoved();
    }
    
    updatingBounds = false;
}

void Canvas::valueChanged(Value& v)
{
    // When lock changes
    if (v.refersToSameSourceAs(locked))
    {
        if(connectingEdge) connectingEdge = nullptr;
        deselectAll();
        repaint();
    }
    else if(v.refersToSameSourceAs(commandLocked)) {
        repaint();
    }
    // Should only get called when the canvas isn't a real graph
    else if (v.refersToSameSourceAs(presentationMode))
    {
        deselectAll();

        if (presentationMode == var(true)) connections.clear();

        synchronise();
    }
    else if (v.refersToSameSourceAs(connectionStyle))
    {
        for (auto* connection : connections)
        {
            connection->updatePath();
            connection->repaint();
        }
    }
}

void Canvas::showSuggestions(Box* box, TextEditor* editor)
{
    suggestor->createCalloutBox(box, editor);
    suggestor->setTopLeftPosition(box->getScreenX(), box->getScreenBounds().getBottom());
}
void Canvas::hideSuggestions()
{
    if (suggestor->isOnDesktop())
    {
        suggestor->removeFromDesktop();
    }

    suggestor->openedEditor = nullptr;
    suggestor->currentBox = nullptr;
}

// Makes component selected
void Canvas::setSelected(Component* component, bool shouldNowBeSelected)
{
    bool isAlreadySelected = isSelected(component);

    if (!isAlreadySelected && shouldNowBeSelected)
    {
        selectedComponents.addToSelection(component);
        component->repaint();
    }

    if (isAlreadySelected && !shouldNowBeSelected)
    {
        removeSelectedComponent(component);
        component->repaint();
    }

    main.updateCommandStatus();
}

bool Canvas::isSelected(Component* component) const
{
    return std::find(selectedComponents.begin(), selectedComponents.end(), component) != selectedComponents.end();
}

void Canvas::handleMouseDown(Component* component, const MouseEvent& e)
{
    if (!isSelected(component))
    {
        if (!(e.mods.isShiftDown() || e.mods.isCommandDown())) deselectAll();

        setSelected(component, true);
    }

    if(auto* box = dynamic_cast<Box*>(component)) {
        componentBeingDragged = box;
    }
    
    for(auto* box : getSelectionOfType<Box>()) {
        box->mouseDownPos = box->getPosition();
    }

    component->repaint();
}

// Call from component's mouseUp
void Canvas::handleMouseUp(Component* component, const MouseEvent& e)
{
    if (didStartDragging)
    {
        // Ignore when locked
        if (locked == var(true)) return;

        auto objects = std::vector<pd::Object*>();

        for (auto* component : getLassoSelection())
        {
            if (auto* box = dynamic_cast<Box*>(component))
            {
                if (box->pdObject) objects.push_back(box->pdObject.get());
            }
        }

        // When done dragging objects, update positions to pd
        patch.moveObjects(objects, e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY());

        // Check if canvas is large enough
        checkBounds();

        // Update undo state
        main.updateCommandStatus();
    }

    if (didStartDragging) didStartDragging = false;
    
    componentBeingDragged = nullptr;

    component->repaint();
}

int Canvas::shouldGridLock(const MouseEvent& e, Component* toDrag) {
    gridEnabled = true;
    if(!static_cast<bool>(gridEnabled.getValue())) return 0;
    
    return 0;
    
    /*
    if(e.getDistanceFromDragStartX() > 20) {
        gridIsLocked = false;
        return 0;
    }
    else {
        if(!gridIsLocked) {
            gridIsLocked = true;
            gridLockPosition = toDrag->getPosition();
            
        }
        
        return 1;
    } */
}

// Call from component's mouseDrag
void Canvas::handleMouseDrag(const MouseEvent& e)
{
    /** Ensure tiny movements don't start a drag. */
    if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag) return;

    didStartDragging = true;

    if(shouldGridLock(e, componentBeingDragged)) {
        //delta = gridLockPosition;
    }

    for (auto* box : getSelectionOfType<Box>())
    {
        box->setTopLeftPosition(box->mouseDownPos + Point<int>(e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY()));
    }

    for (auto& tmpl : templates)
    {
        tmpl->updateIfMoved();
    }
}

SelectedItemSet<Component*>& Canvas::getLassoSelection()
{
    return selectedComponents;
}

void Canvas::removeSelectedComponent(Component* component)
{
    selectedComponents.deselect(component);
}

void Canvas::findLassoItemsInArea(Array<Component*>& itemsFound, const Rectangle<int>& area)
{
    for (auto* element : boxes)
    {
        if (area.intersects(element->getBounds()))
        {
            itemsFound.add(element);
            setSelected(element, true);
            element->repaint();
        }
        else if (!ModifierKeys::getCurrentModifiers().isAnyModifierKeyDown())
        {
            setSelected(element, false);
        }
    }

    for (auto& con : connections)
    {
        bool intersect = false;

        // Check intersection with path
        for (int i = 0; i < 200; i++)
        {
            float position = static_cast<float>(i) / 200.0f;
            auto point = con->toDraw.getPointAlongPath(position * con->toDraw.getLength());

            if (!point.isFinite()) continue;

            if (lasso.getBounds().contains(point.toInt() + con->getPosition()))
            {
                intersect = true;
            }
        }

        if (intersect)
        {
            con->repaint();
            itemsFound.add(con);
            setSelected(con, true);
        }
    }
}
