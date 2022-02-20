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

// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component, public KeyListener, public TextEditor::InputFilter
{
    class Suggestion : public TextButton
    {
        int type = -1;
        Array<Colour> colours = {findColour(ScrollBar::thumbColourId), Colours::yellow};

        Array<String> letters = {"pd", "~"};

        String objectDescription;

       public:
        Suggestion()
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
            TextButton::paint(g);

            if (objectDescription.isNotEmpty())
            {
                auto font = getLookAndFeel().getTextButtonFont(*this, getHeight());
                auto textLength = font.getStringWidth(getButtonText()) + 22;

                g.setColour(findColour(ComboBox::outlineColourId));

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
            Suggestion* but = buttons.add(new Suggestion);
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
        setVisible(false);
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
        setVisible(editor->getText().isNotEmpty());
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

        resizer.setBounds(getWidth() - 16, getHeight() - 16, 16, 16);

        port->setViewPosition(0, yScroll);
        repaint();
    }

   private:
    void paint(Graphics& g) override
    {
        g.setColour(findColour(ComboBox::backgroundColourId));
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
            if (library.objectDescriptions.find(found[i]) != library.objectDescriptions.end())
            {
                buttons[i]->setText(found[i], library.objectDescriptions[found[i]]);
            }
            else
            {
                buttons[i]->setText(found[i], "");
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

        String fullName = found[currentidx];

        highlightEnd = fullName.length();

        if (!mutableInput.containsNonWhitespaceChars() || (e.getText() + mutableInput).contains(" "))

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

    Array<Colour> colours = {findColour(ComboBox::backgroundColourId), findColour(ResizableWindow::backgroundColourId)};

    Colour bordercolor = Colour(142, 152, 155);

    int highlightStart = 0;
    int highlightEnd = 0;

    bool isCompleting = false;
};

// Graph bounds component
struct GraphArea : public Component, public ComponentDragger
{
    ResizableBorderComponent resizer;
    Canvas* canvas;

    Rectangle<int> startRect;

    explicit GraphArea(Canvas* parent) : resizer(this, nullptr), canvas(parent)
    {
        addAndMakeVisible(resizer);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(Slider::thumbColourId));
        g.drawRect(getLocalBounds());

        g.setColour(findColour(Slider::thumbColourId).darker(0.8f));
        g.drawRect(getLocalBounds().reduced(6));
    }

    bool hitTest(int x, int y) override
    {
        return !getLocalBounds().reduced(8).contains(Point<int>{x, y});
    }

    void mouseMove(const MouseEvent& e) override
    {
        setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
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
        updatePosition();
        repaint();
    }

    void resized() override
    {
        updatePosition();
        resizer.setBounds(getLocalBounds());
        repaint();
    }

    void updatePosition()
    {
        t_canvas* cnv = canvas->patch.getPointer();
        // Lock first?
        if (cnv)
        {
            cnv->gl_pixwidth = getWidth() / pd::Patch::zoom;
            cnv->gl_pixheight = getHeight() / pd::Patch::zoom;
            cnv->gl_xmargin = (getX() - 4) / pd::Patch::zoom;
            cnv->gl_ymargin = (getY() - 4) / pd::Patch::zoom;
        }
    }
};

Canvas::Canvas(PlugDataPluginEditor& parent, pd::Patch patch, bool graph, bool graphChild) : main(parent), pd(&parent.pd), patch(std::move(patch))
{
    isGraph = graph;
    isGraphChild = graphChild;

    suggestor = new SuggestionComponent;

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
    }

    setSize(600, 400);

    if (!isGraph)
    {
        // Apply zooming
        setTransform(parent.transform);
    }

    // Add lasso component
    addAndMakeVisible(&lasso);
    lasso.setAlwaysOnTop(true);
    lasso.setColour(LassoComponent<Box>::lassoFillColourId, findColour(ScrollBar::ColourIds::thumbColourId).withAlpha(0.3f));

    setWantsKeyboardFocus(true);

    if (!isGraph)
    {
        viewport = new Viewport;  // Owned by the tabbar, but doesn't exist for graph!
        viewport->setViewedComponent(this, false);
        viewport->setBufferedToImage(true);
    }

    addChildComponent(suggestor);

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
        g.fillAll(findColour(ComboBox::backgroundColourId));

        g.setColour(findColour(ResizableWindow::backgroundColourId));
        g.fillRect(zeroPosition.x, zeroPosition.y, getWidth(), getHeight());

        // draw origin
        g.setColour(Colour(100, 100, 100));
        g.drawLine(zeroPosition.x - 1, zeroPosition.y, zeroPosition.x - 1, getHeight());
        g.drawLine(zeroPosition.x, zeroPosition.y - 1, getWidth(), zeroPosition.y - 1);
    }
}

void Canvas::focusGained(FocusChangeType cause)
{
    // This is necessary because in some cases, setting the canvas as current right before an action isn't enough
    pd->setThis();
    if (patch.getPointer())
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
    patch.updateExtraInfo();

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

    if (!isGraph)
    {
        // Remove deprecated connections
        for (int n = connections.size() - 1; n >= 0; n--)
        {
            auto connection = connections[n];

            if (isObjectDeprecated(connection->start->box->pdObject.get()) || isObjectDeprecated(connection->end->box->pdObject.get()))
            {
                connections.remove(n);
            }
            else
            {
                auto* start = static_cast<t_text*>(connection->start->box->pdObject->getPointer());
                auto* end = static_cast<t_text*>(connection->end->box->pdObject->getPointer());

                if (!canvas_isconnected(patch.getPointer(), start, connection->outIdx, end, connection->inIdx))
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
            auto b = object.getBounds();
            auto name = String(object.getText());

            auto type = pd::Gui::getType(object.getPointer());
            auto isGui = type != pd::Type::Undefined;
            auto* pdObject = isGui ? new pd::Gui(object.getPointer(), &patch, pd) : new pd::Object(object);

            if (type == pd::Type::Message)
                name = "msg";
            else if (type == pd::Type::AtomNumber)
                name = "floatatom";
            else if (type == pd::Type::AtomSymbol)
                name = "symbolatom";

            // Some of these GUI objects have a lot of extra symbols that we don't want to show
            auto guiSimplify = [](String& target, const StringArray& selectors)
            {
                for (auto& str : selectors)
                {
                    if (target.startsWith(str))
                    {
                        target = str;
                        return;
                    }
                }
            };

            b.translate(zeroPosition.x, zeroPosition.y);

            // These objects have extra info (like size and colours) in their names that we want to hide
            guiSimplify(name, {"bng", "tgl", "nbx", "hsl", "vsl", "hradio", "vradio", "pad", "cnv"});

            auto* newBox = boxes.add(new Box(pdObject, this, name, b.getPosition()));
            newBox->toFront(false);

            if (newBox->graphics && newBox->graphics->label) newBox->graphics->label->toFront(false);

            // Don't show non-patchable (internal) objects
            if (!pd::Patch::checkObject(&object)) newBox->setVisible(false);
        }
        else
        {
            auto* box = *it;
            auto b = object.getBounds();

            b.translate(zeroPosition.x, zeroPosition.y);

            // Only update positions if we need to and there is a significant difference
            // There may be rounding errors when scaling the gui, this makes the experience smoother
            if (updatePosition && box->getPosition().getDistanceFrom(b.getPosition()) > 8)
            {
                box->setTopLeftPosition(b.getPosition());
            }

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

    if (!isGraph)
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

                                       if (!c->start || !c->end) return false;

                                       bool sameStart = c->start->box == boxes[srcno];
                                       bool sameEnd = c->end->box == boxes[sinkno];

                                       return c->inIdx == inno && c->outIdx == outno && sameStart && sameEnd;
                                   });

            if (it == connections.end())
            {
                connections.add(new Connection(this, srcEdges[boxes[srcno]->numInputs + outno], sinkEdges[inno], true));
            }
            else
            {
                auto& c = *(*it);

                auto currentId = c.getId();
                if (c.lastId.isNotEmpty() && c.lastId != currentId)
                {
                    patch.setExtraInfoId(c.lastId, currentId);
                    c.lastId = currentId;
                }

                auto info = patch.getExtraInfo(currentId);
                if (info.getSize()) c.setState(info);
                c.repaint();
            }
        }

        setTransform(main.transform);

        templates.clear();
        templates.addArray(findDrawables());

        for (auto& tmpl : templates)
        {
            addAndMakeVisible(tmpl);
            tmpl->setAlwaysOnTop(true);
            tmpl->update();
        }
    }

    // patch.deselectAll();

    // Resize canvas to fit objects
    checkBounds();
}

void Canvas::mouseDown(const MouseEvent& e)
{
    if (suggestor->openedEditor && e.originalComponent != suggestor->openedEditor)
    {
        suggestor->currentBox->hideEditor();
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

        auto b = subpatch->getBounds();

        if (isGraphChild)
        {
            newCanvas->graphArea->setBounds(b.getX(), b.getY(), std::max(b.getWidth(), 60), std::max(b.getHeight(), 60));
        }

        main.addTab(newCanvas);
        newCanvas->checkBounds();
    };

    auto* source = e.originalComponent;

    // Ignore if locked
    if (locked == true)
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
        if (locked == true)
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
        bool hasSelection = lassoSelection.getNumSelected();
        bool multiple = lassoSelection.getNumSelected() > 1;

        auto* box = getSingleSelection();

        bool isSubpatch = box && (box->graphics && (box->graphics->getGui().getType() == pd::Type::GraphOnParent || box->graphics->getGui().getType() == pd::Type::Subpatch));

        bool isGui = hasSelection && !multiple && box->graphics && !box->graphics->fakeGui();

        // Create popup menu
        popupMenu.clear();
        popupMenu.addItem(1, "Open", hasSelection && !multiple && isSubpatch);  // for opening subpatches
        // popupMenu.addItem(10, "Edit", isGui);
        popupMenu.addSeparator();
        popupMenu.addItem(4, "Cut", hasSelection);
        popupMenu.addItem(5, "Copy", hasSelection);
        popupMenu.addItem(6, "Duplicate", hasSelection);
        popupMenu.addItem(7, "Delete", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(8, "To Front", hasSelection);
        popupMenu.addSeparator();
        popupMenu.addItem(9, "Help", hasSelection);  // Experimental: opening help files

        auto callback = [this, &lassoSelection, openSubpatch](int result)
        {
            if (result < 1) return;

            switch (result)
            {
                case 1:
                {
                    openSubpatch(getSingleSelection());
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
                    lassoSelection.getSelectedItem(0)->toFront(false);
                    break;

                case 9:
                {  // Open help

                    pd->setThis();
                    // Find name of help file
                    auto helpPatch = getSingleSelection()->pdObject->getHelp();

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
    if (isGraph || locked == true) return;

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
    if (viewport->autoScroll(viewportEvent.x * scale, viewportEvent.y * scale, 100, scrollSpeed))
    {
        beginDragAutoRepeat(40);
    }

    // Drag lasso
    if (dynamic_cast<Connection*>(source))
    {
        lasso.dragLasso(e.getEventRelativeTo(this));
    }
    else if (source == this)
    {
        connectingEdge = nullptr;
        lasso.dragLasso(e);

        if (e.getDistanceFromDragStart() < 5) return;
    }

    if (connectingWithDrag) repaint();
}

void Canvas::mouseUp(const MouseEvent& e)
{
    if (auto* box = dynamic_cast<Box*>(e.originalComponent))
    {
        if (!ModifierKeys::getCurrentModifiers().isShiftDown() && !ModifierKeys::getCurrentModifiers().isCommandDown() && e.getDistanceFromDragStart() < 2)
        {
            deselectAll();
        }

        if (locked == false && !isGraph && box->getParentComponent() == this)
        {
            setSelected(box, true);
        }
    }

    // Releasing a connect by drag action
    if (connectingWithDrag)
    {
        auto pos = e.getEventRelativeTo(this).getPosition();
        auto* nearest = Edge::findNearestEdge(this, pos);

        if (nearest) nearest->createConnection();

        connectingEdge = nullptr;
        connectingWithDrag = false;

        repaint();
    }

    auto& lassoSelection = getLassoSelection();

    // Pass parameters of selected box to inspector
    if (auto* box = getSingleSelection())
    {
        auto params = box->graphics ? box->graphics->getParameters() : ObjectParameters();

        if (!params.empty())
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

    lasso.endLasso();
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
    // Pd Template drawing: not the most efficient implementation but it seems to work!
    // Graphs are drawn from their parent, so pd drawings are always on top of other objects
    if (!isGraph)
    {
        // findDrawables(g);
    }

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
    // For deciding where to place a new object
    lastMousePos = getLocalBounds().reduced(20).getConstrainedPoint(e.getPosition());

    if (connectingEdge)
    {
        repaint();
    }
}

bool Canvas::keyPressed(const KeyPress& key)
{
    if (main.getCurrentCanvas() != this) return false;
    if (isGraph) return false;

    patch.keyPress(key.getKeyCode(), key.getModifiers().isShiftDown());

    return false;
}

void Canvas::deselectAll()
{
    // Deselect boxes
    for (auto c : selectedComponents)
        if (c) c->repaint();

    selectedComponents.deselectAll();

    // Deselect connections
    for (auto& connection : connections)
    {
        if (connection->isSelected)
        {
            connection->isSelected = false;
            connection->repaint();
        }
    }

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
    for (auto& con : connections)
    {
        if (con->isSelected)
        {
            if (!(objects.contains(con->outObj->get()) || objects.contains(con->inObj->get())))
            {
                patch.removeConnection(con->outObj->get(), con->outIdx, con->inObj->get(), con->inIdx);
            }
        }
    }

    patch.finishRemove();  // Makes sure that the extra removed connections will be grouped in the same undo action

    deselectAll();

    // Load state from pd, don't update positions
    synchronise(false);

    // patch.deselectAll();

    // Update GUI
    main.updateValues();
    main.updateUndoState();
}

void Canvas::undo()
{
    // Tell pd to undo the last action
    patch.undo();

    // Load state from pd
    synchronise();

    patch.deselectAll();

    main.updateUndoState();
}

void Canvas::redo()
{
    // Tell pd to undo the last action
    patch.redo();

    // Load state from pd
    synchronise();

    patch.deselectAll();
    main.updateUndoState();
}

void Canvas::checkBounds()
{
    if (isGraph || !viewport) return;

    float scale = (1.0f / static_cast<float>(pd->zoomScale.getValue()));

    auto viewBounds = Rectangle<int>(zeroPosition.x, zeroPosition.y, viewport->getMaximumVisibleWidth() * scale, viewport->getMaximumVisibleHeight() * scale);

    for (auto obj : boxes)
    {
        viewBounds = obj->getBounds().getUnion(viewBounds);
    }

    for (auto& box : boxes)
    {
        box->setBounds(box->getBounds().translated(-viewBounds.getX(), -viewBounds.getY()));
    }

    zeroPosition -= {viewBounds.getX(), viewBounds.getY()};
    setSize(viewBounds.getWidth(), viewBounds.getHeight());

    if (graphArea)
    {
        graphArea->setBounds(patch.getBounds());
    }
}

void Canvas::valueChanged(Value& v)
{
    // When lock changes
    if (v.refersToSameSourceAs(locked))
    {
        deselectAll();

        for (auto& connection : connections)
        {
            if (connection->isSelected)
            {
                connection->isSelected = false;
                connection->repaint();
            }
        }
    }

    else if (v.refersToSameSourceAs(connectionStyle))
    {
        for (auto* connection : connections)
        {
            connection->resized();
            connection->repaint();
        }
    }
}

void Canvas::showSuggestions(Box* box, TextEditor* editor)
{
    suggestor->createCalloutBox(box, editor);
    suggestor->setTopLeftPosition(box->getX(), box->getBounds().getBottom());
    // suggestor->resized();
    // suggestor->setVisible(true);
}
void Canvas::hideSuggestions()
{
    suggestor->setVisible(false);

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

        if (auto* box = dynamic_cast<Box*>(component))
        {
            // ?
        }
        if (auto* con = dynamic_cast<Connection*>(component))
        {
            con->isSelected = true;
        }

        component->repaint();
    }

    if (isAlreadySelected && !shouldNowBeSelected)
    {
        removeSelectedComponent(component);

        if (auto* box = dynamic_cast<Box*>(component))
        {
            // ?
        }
        if (auto* con = dynamic_cast<Connection*>(component))
        {
            // con->isSelected = false;
        }

        component->repaint();
    }
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

    mouseDownWithinTarget = e.getMouseDownPosition();

    componentBeingDragged = component;

    totalDragDelta = {0, 0};

    component->repaint();
}

// Call from component's mouseUp
void Canvas::handleMouseUp(Component* component, const MouseEvent& e)
{
    if (didStartDragging)
    {
        // Ignore when locked
        if (locked == true) return;

        auto objects = std::vector<pd::Object*>();

        for (auto* component : getLassoSelection())
        {
            if (auto* box = dynamic_cast<Box*>(component))
            {
                if (box->pdObject) objects.push_back(box->pdObject.get());
            }
        }

        // When done dragging objects, update positions to pd
        patch.moveObjects(objects, totalDragDelta.x, totalDragDelta.y);

        // Check if canvas is large enough
        checkBounds();

        // Update undo state
        main.updateUndoState();
    }

    if (didStartDragging) didStartDragging = false;

    component->repaint();
}

// Call from component's mouseDrag
void Canvas::handleMouseDrag(const MouseEvent& e)
{
    /** Ensure tiny movements don't start a drag. */
    if (!didStartDragging && e.getDistanceFromDragStart() < minimumMovementToStartDrag) return;

    didStartDragging = true;

    Point<int> delta = e.getPosition() - mouseDownWithinTarget;

    for (auto comp : selectedComponents)
    {
        Rectangle<int> bounds(comp->getBounds());

        bounds += delta;

        comp->setBounds(bounds);
    }

    for (auto& tmpl : templates)
    {
        tmpl->updateIfMoved();
    }

    totalDragDelta += delta;
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

        if (!con->isSelected && intersect)
        {
            con->isSelected = true;
            con->repaint();
            itemsFound.add(con);
        }
    }
}

// TODO: this is incorrect!!
Box* Canvas::getSingleSelection()
{
    if (selectedComponents.getNumSelected() == 1)
    {
        if (auto* box = dynamic_cast<Box*>(selectedComponents.getSelectedItem(0)))
        {
            return box;
        }
    }

    return nullptr;
}
