#include "Connection.h"
#include "Iolet.h"

// Simulates the direct mouse interactions a user performs on canvas content:
// dragging objects (with grid snapping enabled), resizing GUI objects by
// their corner, making connections both by clicking and by dragging between
// iolets, manipulating a segmented connection, and opening/closing the object
// text editor.
//
// These paths are unreachable from the other tests (which create objects and
// connections through the patch API), so ObjectGrid::performMove/performResize,
// Object::mouseDown/mouseDrag/mouseUp, Iolet's connect-by-click and
// connect-by-drag state machines and Connection's mouse handling otherwise
// never run. Synthetic MouseEvents are delivered directly to the components,
// with event times set so length-of-press checks behave like a real drag.

class ObjectInteractionTest : public PlugDataUnitTest
{
public:
    ObjectInteractionTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Object Interaction Test")
    {
    }

private:
    void perform() override
    {
        auto* settings = SettingsFile::getInstance();
        previousGridEnabled = settings->getProperty<int>("grid_enabled");
        previousGridType = settings->getProperty<int>("grid_type");
        // Enable every grid mode, so both absolute-grid and relative
        // object-edge snapping run during the drag
        settings->setProperty("grid_enabled", 1);
        settings->setProperty("grid_type", 3);

        cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 700 500 12;\n"
            "#X obj 50 50 f;\n"
            "#X obj 300 60 f;\n"
            "#X obj 50 150 osc~ 440;\n"
            "#X obj 300 150 dac~;\n"
            "#X obj 50 250 cnv 15 100 80 empty empty empty 20 12 0 14 #e0e0e0 #404040 0;\n"
            "#X obj 300 250 +~;\n"));

        if (!cnv || cnv->objects.size() != 6) {
            signalDone(false);
            return;
        }

        // Patches opened from existing content start locked (run mode); all the
        // interactions below need edit mode. Objects cache the lock state via a
        // listener callback, so give it a tick to propagate before dragging.
        cnv->locked.setValue(false);

        int i = 0;
        for (auto* obj : cnv->objects) {
            objects[i++] = obj;
        }

        buildSteps();
        Timer::callAfterDelay(50, [this] { runNextStep(0); });
    }

    static MouseEvent makeEvent(Component* c, Point<float> const pos, Point<float> const downPos, bool const dragged, int const clicks = 1)
    {
        auto const now = Time::getCurrentTime();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(),
                          pos, ModifierKeys::leftButtonModifier,
                          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                          now, downPos, now - RelativeTime::milliseconds(300), clicks, dragged);
    }

    // The outlet row starts after the inlets in Object::iolets
    static Iolet* getOutlet(Object* obj, int const idx) { return obj->iolets[obj->numInputs + idx]; }
    static Iolet* getInlet(Object* obj, int const idx) { return obj->iolets[idx]; }

    void buildSteps()
    {
        steps.clear();

        // --- Drag object 0 towards near-alignment with object 1, so the
        //     object-edge snap (tolerance 6px) engages alongside the grid ---
        steps.add([this] {
            beginTest("Drag an object with grid snapping enabled");
            cnv->deselectAll();
            auto* obj = objects[0].getComponent();
            positionBeforeDrag = obj->getPosition();
            dragStart = obj->getLocalBounds().getCentre().toFloat();
            obj->mouseDown(makeEvent(obj, dragStart, dragStart, false));
        });
        for (int step = 1; step <= 4; step++) {
            steps.add([this, step] {
                if (auto* obj = objects[0].getComponent()) {
                    auto const offset = Point<float>(20.0f * step, 2.0f * step);
                    obj->mouseDrag(makeEvent(obj, dragStart + offset, dragStart, true));
                }
            });
        }
        steps.add([this] {
            if (auto* obj = objects[0].getComponent()) {
                obj->mouseUp(makeEvent(obj, dragStart + Point<float>(80.0f, 8.0f), dragStart, true));
                expect(obj->getPosition() != positionBeforeDrag, "the drag must have moved the object");
            }
        });

        // --- Resize the [cnv] object by its bottom-right corner ---
        steps.add([this] {
            beginTest("Resize a GUI object by its corner");
            cnv->deselectAll();
            cnv->setSelected(objects[4].getComponent(), true);
            // Objects cache their selected state via an async change broadcast,
            // which mouseMove needs to arm the resize zone. Flush it synchronously
            // so the zone arms deterministically on the next step.
            cnv->selectedComponents.sendSynchronousChangeMessage();
        });
        steps.add([this] {
            auto* obj = objects[4].getComponent();
            expect(obj->isSelected(), "the object must be selected before resizing");
            widthBeforeResize = obj->getWidth();
            // mouseMove sets the resize zone; only works on selected objects
            dragStart = Point<float>(obj->getWidth() - Object::margin, obj->getHeight() - Object::margin);
            obj->mouseMove(makeEvent(obj, dragStart, dragStart, false));
            obj->mouseDown(makeEvent(obj, dragStart, dragStart, false));
        });
        for (int step = 1; step <= 3; step++) {
            steps.add([this, step] {
                if (auto* obj = objects[4].getComponent())
                    obj->mouseDrag(makeEvent(obj, dragStart + Point<float>(10.0f * step, 7.0f * step), dragStart, true));
            });
        }
        steps.add([this] {
            if (auto* obj = objects[4].getComponent()) {
                obj->mouseUp(makeEvent(obj, dragStart + Point<float>(30.0f, 21.0f), dragStart, true));
                expect(obj->getWidth() > widthBeforeResize, "the resize must have grown the object");
            }
        });

        // --- Connect [osc~] to [dac~] by clicking outlet, then inlet ---
        steps.add([this] {
            beginTest("Connect by clicking an outlet, then an inlet");
            expect(!getValue<bool>(cnv->locked), "canvas must be unlocked");
            clickIolet(getOutlet(objects[2].getComponent(), 0));
            expect(!cnv->connectionsBeingCreated.empty(), "pending connection must exist right after the outlet click");
        });
        steps.add([this] {
            expect(!cnv->connectionsBeingCreated.empty(), "clicking an outlet must start a pending connection");
            clickIolet(getInlet(objects[3].getComponent(), 0));
        });

        // --- Connect [osc~] to [+~] by dragging from outlet to inlet ---
        steps.add([this] {
            beginTest("Connect by dragging from an outlet to an inlet");
            // The connection exists on the pd side; sync the components now
            // so we can assert on it
            cnv->performSynchronise();
            expect(cnv->connections.size() == 1, "connect-by-click must have created a connection, have " + String(static_cast<int>(cnv->connections.size())));
            auto* outlet = getOutlet(objects[2].getComponent(), 0);
            auto const centre = outlet->getLocalBounds().getCentre().toFloat();
            dragStart = centre;
            // A drag with >100ms of mouse press queues the connection start
            outlet->mouseDrag(makeEvent(outlet, centre + Point<float>(5.0f, 5.0f), centre, true));
        });
        steps.add([this] {
            // Now connectingWithDrag is set; drag over the target inlet so
            // findNearestIolet targets it
            auto* outlet = getOutlet(objects[2].getComponent(), 0);
            auto* target = getInlet(objects[5].getComponent(), 0);
            auto const targetPos = outlet->getLocalPoint(target, target->getLocalBounds().getCentre().toFloat());
            outlet->mouseDrag(makeEvent(outlet, targetPos, dragStart, true));
        });
        steps.add([this] {
            auto* outlet = getOutlet(objects[2].getComponent(), 0);
            auto* target = getInlet(objects[5].getComponent(), 0);
            auto const targetPos = outlet->getLocalPoint(target, target->getLocalBounds().getCentre().toFloat());
            outlet->mouseUp(makeEvent(outlet, targetPos, dragStart, true));
        });

        // --- Select a connection, make it segmented and drag a segment ---
        steps.add([this] {
            beginTest("Manipulate a segmented connection");
            expect(cnv->connections.size() >= 2, "connect-by-drag must have created a second connection, have " + String(static_cast<int>(cnv->connections.size())));
            if (cnv->connections.size()) {
                auto* connection = *cnv->connections.begin();
                cnv->deselectAll();
                cnv->setSelected(connection, true);
                connection->setSegmented(true);

                auto const middle = connection->getLocalBounds().getCentre().toFloat();
                dragStart = middle;
                connection->mouseDown(makeEvent(connection, middle, middle, false));
                connection->mouseDrag(makeEvent(connection, middle + Point<float>(0.0f, 15.0f), middle, true));
                connection->mouseUp(makeEvent(connection, middle + Point<float>(0.0f, 15.0f), middle, true));
            }
        });

        // --- Open and close the object text editor ---
        steps.add([this] {
            beginTest("Open and close the object text editor");
            cnv->deselectAll();
            if (auto* obj = objects[0].getComponent()) {
                obj->showEditor();
            }
        });
        steps.add([this] {
            cnv->hideAllActiveEditors();
        });
    }

    void clickIolet(Iolet* iolet)
    {
        auto const centre = iolet->getLocalBounds().getCentre().toFloat();
        iolet->mouseEnter(makeEvent(iolet, centre, centre, false));
        iolet->mouseUp(makeEvent(iolet, centre, centre, false));
        iolet->mouseExit(makeEvent(iolet, centre, centre, false));
    }

    void runNextStep(int const stepIndex)
    {
        if (!cnv) {
            signalDone(false);
            return;
        }
        if (stepIndex >= steps.size()) {
            finish();
            return;
        }

        steps[stepIndex]();

        Timer::callAfterDelay(30, [this, stepIndex] {
            runNextStep(stepIndex + 1);
        });
    }

    void finish()
    {
        auto* settings = SettingsFile::getInstance();
        settings->setProperty("grid_enabled", previousGridEnabled);
        settings->setProperty("grid_type", previousGridType);

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        signalDone(true);
    }

    HeapArray<std::function<void()>> steps;
    Component::SafePointer<Canvas> cnv;
    Component::SafePointer<Object> objects[6];
    Point<float> dragStart;
    Point<int> positionBeforeDrag;
    int widthBeforeResize = 0;
    int previousGridEnabled = 0, previousGridType = 0;
};
