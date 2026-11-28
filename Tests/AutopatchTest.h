// Checks the connections every kind of autopatching makes. plugdata connects a new object to the selected object
// or puts it into the selected connection, snaps shift-dragged objects into and out of connections, and starts a
// connection from every selected object on shift-click. Pd connects the selection (Connect Selection) and fans out
// a connection when shift is pressed while dragging it.

#include "Canvas.h"
#include "Connection.h"
#include "Constants.h"
#include "Iolet.h"
#include "Object.h"

class AutopatchTest : public PlugDataUnitTest {
public:
    AutopatchTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Autopatch Test")
    {
    }

private:
    struct Case {
        String name, patch;
        std::function<void()> action;
        StringArray expected;
    };

    void perform() override
    {
        addCases();
        runCase(0);
    }

    void addCases()
    {
        String const f = "#X obj 50 50 f;\n";
        String const oscillator = "#X obj 50 50 osc~ 440;\n";
        String const chain = "#X obj 50 50 f;\n#X obj 50 120 + 1;\n#X obj 50 190 print a;\n#X connect 0 0 1 0;\n#X connect 1 0 2 0;\n";
        String const connected = "#X obj 50 50 f;\n#X obj 50 250 print a;\n#X obj 250 150 + 1;\n#X connect 0 0 1 0;\n";
        String const twoSources = "#X obj 50 50 f;\n#X obj 150 50 i;\n#X obj 100 200 print a;\n";

        // plugdata
        add("A new object connects to the selected object", f, [this] {
            select({ "f" });
            invoke(ObjectIDs::NewBang);
        }, { "f 0 -> bng 0" });
        add("A typed object connects to the selected object", oscillator, [this] {
            select({ "osc~ 440" });
            if (invoke(ObjectIDs::NewObject))
                type(cnv->objects.back(), "*~ 0.5");
        }, { "osc~ 0 -> *~ 0" });
        add("A new object doesn't connect to an incompatible outlet", oscillator, [this] {
            select({ "osc~ 440" });
            invoke(ObjectIDs::NewToggle);
        }, {});
        add("Turning autopatching off stops it", f, [this] {
            auto const was = editor->autoconnect.getValue();
            editor->autoconnect = false;
            select({ "f" });
            invoke(ObjectIDs::NewBang);
            editor->autoconnect = was;
        }, {});
        add("A new object goes into the selected connection", connected, [this] {
            selectConnection("f", "print a");
            if (invoke(ObjectIDs::NewObject))
                type(cnv->objects.back(), "- 1");
        }, { "f 0 -> - 0", "- 0 -> print a 0" });
        add("A new object doesn't go into a connection it can't carry", "#X obj 50 50 osc~;\n#X obj 50 250 dac~;\n#X connect 0 0 1 0;\n", [this] {
            selectConnection("osc~", "dac~");
            if (invoke(ObjectIDs::NewObject))
                type(cnv->objects.back(), "f");
        }, { "osc~ 0 -> dac~ 0" });
        add("Shift-dragging an object onto a connection inserts it", connected, [this] {
            auto* obj = find("+ 1");
            auto* connection = findConnection("f", "print a");
            if (obj && connection) {
                auto const target = (connection->getStartPoint() + connection->getEndPoint()) / 2.0f;
                auto const offset = target - obj->iolets[0]->getCanvasBounds().getCentre().toFloat();
                auto const centre = obj->getLocalBounds().getCentre().toFloat();
                shiftDrag(obj, centre, centre + offset);
            }
        }, { "f 0 -> + 0", "+ 0 -> print a 0" });
        add("Shift-dragging an object out of a chain joins it", chain, [this] {
            if (auto* obj = find("+ 1")) {
                auto const centre = obj->getLocalBounds().getCentre().toFloat();
                shiftDrag(obj, centre, centre + Point<float>(120, 0));
            }
        }, { "f 0 -> print a 0" });
        add("Shift-clicking an outlet starts connections from every selected object", twoSources, [this] {
            select({ "f", "i" });
            auto* source = find("f");
            auto* sink = find("print a");
            if (!source || !sink)
                return;
            ModifierKeys::currentModifiers = ModifierKeys::shiftModifier;
            source->iolets[source->numInputs]->createConnection();
            ModifierKeys::currentModifiers = {};
            sink->iolets[0]->createConnection();
            cnv->cancelConnectionCreation();
        }, { "f 0 -> print a 0", "i 0 -> print a 0" });

        // Pd, from a connection dragged with shift pressed
        add("Shift while connecting connects every selected sink", "#X obj 150 50 f;\n#X obj 50 200 print a;\n#X obj 150 200 print b;\n#X obj 250 200 print c;\n", [this] {
            select({ "print a", "print b", "print c" });
            connectWithShift("f", 0, "print a", 0);
        }, { "f 0 -> print a 0", "f 0 -> print b 0", "f 0 -> print c 0" });
        add("Shift while connecting connects every selected source", twoSources, [this] {
            select({ "f", "i" });
            connectWithShift("f", 0, "print a", 0);
        }, { "f 0 -> print a 0", "i 0 -> print a 0" });
        add("Shift while connecting two selected objects connects their next iolets", "#X obj 50 50 unpack f f f;\n#X obj 50 200 pack f f f;\n", [this] {
            select({ "unpack f f f", "pack f f f" });
            connectWithShift("unpack f f f", 0, "pack f f f", 0);
        }, { "unpack 0 -> pack 0", "unpack 1 -> pack 1", "unpack 2 -> pack 2" });

        // Pd's Connect Selection
        add("Connect Selection connects two objects from top to bottom", "#X obj 50 200 print a;\n#X obj 50 50 f;\n", [this] {
            select({ "f", "print a" });
            cnv->connectSelection();
        }, { "f 0 -> print a 0" });
        add("Connect Selection doesn't connect incompatible objects", "#X obj 50 50 osc~;\n#X obj 50 200 f;\n", [this] {
            select({ "osc~", "f" });
            cnv->connectSelection();
        }, {});
        add("Connect Selection inserts an object into the selected connection", connected, [this] {
            select({ "+ 1" });
            selectConnection("f", "print a");
            cnv->connectSelection();
        }, { "f 0 -> + 0", "+ 0 -> print a 0" });
        add("Connect Selection disconnects a single object", chain, [this] {
            select({ "+ 1" });
            cnv->connectSelection();
        }, {});
        add("Connect Selection bypasses the middle of three chained objects", chain, [this] {
            select({ "f", "+ 1", "print a" });
            cnv->connectSelection();
        }, { "f 0 -> print a 0" });
        add("Connect Selection inserts the third of three objects", connected, [this] {
            select({ "f", "+ 1", "print a" });
            cnv->connectSelection();
        }, { "f 0 -> + 0", "+ 0 -> print a 0" });
    }

    void add(String const& name, String const& patch, std::function<void()> action, StringArray expected)
    {
        cases.add({ name, patch, std::move(action), std::move(expected) });
    }

    void after(std::function<void()> fn)
    {
        Timer::callAfterDelay(150, std::move(fn));
    }

    void runCase(int const index)
    {
        if (index >= static_cast<int>(cases.size())) {
            signalDone(true);
            return;
        }

        beginTest(cases[index].name);
        cnv = editor->getTabComponent().openPatch(String("#N canvas 0 50 600 400 12;\n") + cases[index].patch);
        if (!cnv) {
            expect(false, "the patch must open");
            runCase(index + 1);
            return;
        }
        cnv->locked.setValue(false);

        // Objects pick up the unlocked state a tick later
        after([this, index] {
            if (!cnv) {
                runCase(index + 1);
                return;
            }
            cnv->performSynchronise();
            cases[index].action();
            after([this, index] {
                if (cnv) {
                    cnv->performSynchronise();
                    expectConnections(cases[index].expected);
                    editor->getTabComponent().closeTab(cnv);
                }
                runCase(index + 1);
            });
        });
    }

    static String label(t_object const* obj)
    {
        auto const tokens = StringArray::fromTokens(pd::Interface::getObjectText(obj).toString(), false);
        return tokens[0] == "print" ? tokens[0] + " " + tokens[1] : tokens[0];
    }

    void expectConnections(StringArray expected)
    {
        StringArray actual;
        for (auto const& [ptr, inIdx, in, outIdx, out] : cnv->patch.getConnections())
            actual.add(label(out) + " " + String(outIdx) + " -> " + label(in) + " " + String(inIdx));
        actual.sort(false);
        expected.sort(false);
        expect(actual == expected, "expected [" + expected.joinIntoString(", ") + "], got [" + actual.joinIntoString(", ") + "]");
        expectEquals(static_cast<int>(cnv->connections.size()), actual.size(), "the canvas must show every connection");
    }

    bool invoke(CommandID const command)
    {
        auto const handled = editor->invoke(ApplicationCommandTarget::InvocationInfo(command), false);
        expect(handled, "the command must run");
        return handled;
    }

    Object* find(String const& text)
    {
        for (auto* obj : cnv->objects) {
            if (obj->gui && obj->gui->getText().toString() == text)
                return obj;
        }
        expect(false, "no object [" + text + "]");
        return nullptr;
    }

    Connection* findConnection(String const& from, String const& to)
    {
        for (auto* c : cnv->connections) {
            if (c->outobj->gui && c->inobj->gui && c->outobj->gui->getText().toString() == from && c->inobj->gui->getText().toString() == to)
                return c;
        }
        expect(false, "no connection [" + from + "] -> [" + to + "]");
        return nullptr;
    }

    void select(StringArray const& texts)
    {
        cnv->deselectAll();
        for (auto const& text : texts) {
            if (auto* obj = find(text))
                cnv->setSelected(obj, true);
        }
        cnv->selectedComponents.sendSynchronousChangeMessage();
    }

    void selectConnection(String const& from, String const& to)
    {
        if (auto* c = findConnection(from, to))
            cnv->setSelected(c, true);
        cnv->selectedComponents.sendSynchronousChangeMessage();
    }

    void type(Object* obj, String const& text)
    {
        if (auto* ed = TestHelpers::findChildOfType<TextEditor>(obj))
            ed->setText(text);
        else
            expect(false, "no text editor to type [" + text + "] into");
        obj->hideEditor();
    }

    static MouseEvent makeEvent(Component* c, Point<float> pos, Point<float> downPos, ModifierKeys mods = ModifierKeys::leftButtonModifier)
    {
        auto const now = Time::getCurrentTime();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), pos, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c, now, downPos, now, 1, pos != downPos);
    }

    // Dragging also reads shift from the global modifiers, which turns grid snapping off
    void shiftDrag(Component* c, Point<float> from, Point<float> to)
    {
        auto const was = ModifierKeys::currentModifiers;
        ModifierKeys::currentModifiers = ModifierKeys::shiftModifier;
        auto const mods = ModifierKeys::leftButtonModifier | ModifierKeys::shiftModifier;
        c->mouseMove(makeEvent(c, from, from, mods));
        c->mouseDown(makeEvent(c, from, from, mods));
        c->mouseDrag(makeEvent(c, to, from, mods));
        c->mouseUp(makeEvent(c, to, from, mods));
        ModifierKeys::currentModifiers = was;
    }

    // Drags a connection onto an inlet, then presses shift, which hands the selection to Pd's autopatching
    void connectWithShift(String const& from, int const outlet, String const& to, int const inlet)
    {
        auto* source = find(from);
        auto* sink = find(to);
        if (!source || !sink)
            return;
        auto* target = sink->iolets[inlet];
        auto const centre = target->getLocalBounds().getCentre().toFloat();
        source->iolets[source->numInputs + outlet]->createConnection();
        target->mouseEnter(makeEvent(target, centre, centre));
        cnv->shiftKeyChanged(true);
        cnv->shiftKeyChanged(false);
        target->mouseExit(makeEvent(target, centre, centre));
        cnv->cancelConnectionCreation();
    }

    HeapArray<Case> cases;
    Component::SafePointer<Canvas> cnv;
};
