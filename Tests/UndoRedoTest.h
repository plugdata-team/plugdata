#include "Constants.h"

// Applies every kind of undoable edit, then undoes and redoes all of it with the Undo/Redo commands.
// The patch starts in plugin mode with hidden text on parent: leaving plugin mode used to hide it from Pd.

class UndoRedoTest : public PlugDataUnitTest
{
public:
    UndoRedoTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Undo Redo Test")
    {
    }

private:
    static constexpr auto patchText = "#N canvas 100 100 950 650 12;\n"
                                      "#X obj 30 20 loadbang;\n"
                                      "#X msg 30 50 pluginmode;\n"
                                      "#X obj 30 80 s pd;\n"
                                      "#X obj 50 150 osc~ 440;\n"
                                      "#X obj 50 200 *~ 0.5;\n"
                                      "#X obj 50 250 dac~;\n"
                                      "#X obj 250 150 metro 250;\n"
                                      "#X obj 250 200 f;\n"
                                      "#X obj 200 260 print a;\n"
                                      "#X obj 320 260 print b;\n"
                                      "#X obj 450 150 tgl 19 0 empty empty empty 0 -10 0 12 #fcfcfc #000000 #000000 0 1;\n"
                                      "#N canvas 0 50 450 300 sub 0;\n"
                                      "#X obj 50 50 inlet;\n"
                                      "#X obj 50 100 outlet;\n"
                                      "#X connect 0 0 1 0;\n"
                                      "#X restore 450 220 pd sub;\n"
                                      "#X obj 600 150 i;\n"
                                      "#X obj 700 300 print d;\n"
                                      "#X obj 800 100 t b;\n"
                                      "#X obj 800 320 print e;\n"
                                      "#X obj 650 450 + 1;\n"
                                      "#X connect 0 0 1 0;\n"
                                      "#X connect 1 0 2 0;\n"
                                      "#X connect 3 0 4 0;\n"
                                      "#X connect 4 0 5 0;\n"
                                      "#X connect 4 0 5 1;\n"
                                      "#X connect 6 0 7 0;\n"
                                      "#X connect 7 0 8 0;\n"
                                      "#X connect 7 0 9 0;\n"
                                      "#X connect 12 0 13 0;\n"
                                      "#X connect 14 0 15 0;\n"
                                      "#X coords 0 0 1 1 200 140 2 0 0;\n";

    void perform() override
    {
        beginTest("Undo and redo every kind of edit");
        editor->getTabComponent().openPatch(String(patchText));

        Timer::callAfterDelay(200, [this] {
            expect(editor->isInPluginMode(), "the patch must open in plugin mode");
            if (editor->isInPluginMode())
                editor->invokeDirectly(CommandIDs::TogglePluginMode, false);

            Timer::callAfterDelay(200, [this] {
                cnv = editor->getCurrentCanvas();
                if (!cnv) {
                    expect(false, "leaving plugin mode must show the patch");
                    signalDone(false);
                    return;
                }
                cnv->locked.setValue(false);
                cnv->performSynchronise();
                original = lastContent = cnv->patch.getCanvasContent();
                buildSteps();
                runStep(0);
            });
        });
    }

    Object* find(String const& text)
    {
        for (auto* obj : cnv->objects)
            if (obj->gui && obj->gui->getText().toString() == text)
                return obj;
        for (auto* obj : cnv->objects)
            if (obj->gui && obj->getType(false).toString() == text)
                return obj;
        expect(false, "no object [" + text + "]");
        return nullptr;
    }

    Connection* findConnection(String const& from, String const& to)
    {
        for (auto* c : cnv->connections)
            if (c->outobj->gui && c->inobj->gui && c->outobj->gui->getText().toString() == from && c->inobj->gui->getText().toString() == to)
                return c;
        expect(false, "no connection [" + from + "] -> [" + to + "]");
        return nullptr;
    }

    void select(StringArray const& texts)
    {
        cnv->deselectAll();
        for (auto const& text : texts)
            if (auto* obj = find(text))
                cnv->setSelected(obj, true);
        cnv->selectedComponents.sendSynchronousChangeMessage();
    }

    static MouseEvent makeEvent(Component* c, Point<float> pos, Point<float> downPos, ModifierKeys mods = ModifierKeys::leftButtonModifier, int clicks = 1)
    {
        auto const now = Time::getCurrentTime();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), pos, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c, now, downPos, now, clicks, pos != downPos);
    }

    void drag(Component* c, Point<float> from, Point<float> to, ModifierKeys mods = ModifierKeys::leftButtonModifier)
    {
        c->mouseMove(makeEvent(c, from, from, mods));
        c->mouseDown(makeEvent(c, from, from, mods));
        c->mouseDrag(makeEvent(c, to, from, mods));
        c->mouseUp(makeEvent(c, to, from, mods));
    }

    void type(Object* obj, String const& text)
    {
        if (auto* ed = TestHelpers::findChildOfType<TextEditor>(obj))
            ed->setText(text);
        else
            expect(false, "no text editor to type [" + text + "] into");
        obj->hideEditor();
    }

    void clickConnection(Connection* c, Point<float> canvasPoint, int clicks)
    {
        auto const pos = c->getLocalPoint(cnv, canvasPoint);
        c->mouseDown(makeEvent(c, pos, pos, ModifierKeys::leftButtonModifier, clicks));
        c->mouseUp(makeEvent(c, pos, pos, ModifierKeys::leftButtonModifier, clicks));
    }

    void clickIolet(Iolet* iolet)
    {
        auto const centre = iolet->getLocalBounds().getCentre().toFloat();
        iolet->mouseEnter(makeEvent(iolet, centre, centre));
        iolet->mouseUp(makeEvent(iolet, centre, centre));
        iolet->mouseExit(makeEvent(iolet, centre, centre));
    }

    // Each action gets its own message-loop tick, and must change the patch
    void addAction(String const& name, std::function<void()> action)
    {
        steps.add([this, name, action] {
            if (previousAction.isNotEmpty() && cnv->patch.getCanvasContent() == lastContent)
                actionsWithoutEffect.add(previousAction);
            previousAction = name;
            lastContent = cnv->patch.getCanvasContent();
            action();
        });
    }

    void buildSteps()
    {
        addAction("create", [this] {
            auto const pos = cnv->canvasOrigin.toFloat() + Point<float>(450, 520);
            cnv->mouseDown(makeEvent(cnv, pos, pos, ModifierKeys::leftButtonModifier, 2));
            cnv->mouseUp(makeEvent(cnv, pos, pos, ModifierKeys::leftButtonModifier, 2));
            type(cnv->objects.back(), "f 5");
        });
        addAction("rename", [this] {
            if (auto* obj = find("metro 250")) {
                obj->showEditor();
                type(obj, "metro 500");
            }
        });
        addAction("rename subpatch", [this] {
            if (auto* obj = find("pd sub")) {
                obj->showEditor();
                type(obj, "pd sub2");
            }
        });
        addAction("move", [this] {
            cnv->deselectAll();
            if (auto* obj = find("osc~ 440")) {
                auto const centre = obj->getLocalBounds().getCentre().toFloat();
                drag(obj, centre, centre + Point<float>(30, 12));
            }
        });
        steps.add([this] { select({ "tgl" }); });
        addAction("resize", [this] {
            if (auto* obj = find("tgl")) {
                auto const from = Point<float>(obj->getWidth() - Object::margin, obj->getHeight() - Object::margin);
                drag(obj, from, from + Point<float>(20, 20));
            }
        });
        addAction("change property", [this] {
            if (auto* obj = find("tgl")) {
                for (auto& param : obj->gui->getParameters().getParameters())
                    if (param.name == "Non-zero value")
                        param.valuePtr->setValue(5.0f);
            }
        });
        steps.add([this] {
            select({ "print b" });
            cnv->copySelection();
        });
        addAction("paste", [this] {
            if (!SystemClipboard::getTextFromClipboard().contains("#X")) {
                logMessage("system clipboard unavailable - skipping paste");
                previousAction = {};
                return;
            }
            cnv->pasteSelection();
        });
        addAction("duplicate", [this] {
            select({ "print a" });
            cnv->duplicateSelection();
        });
        addAction("smart patching", [this] {
            select({ "i", "f 5" });
            cnv->connectSelection();
        });
        addAction("align", [this] {
            select({ "*~ 0.5", "print a" });
            cnv->alignObjects(Align::Left);
        });
        addAction("tidy", [this] {
            select({ "osc~ 440", "metro 500" });
            cnv->tidySelection();
        });
        addAction("triggerize", [this] {
            select({ "f" });
            cnv->triggerizeSelection();
            cnv->hideSuggestions();
            cnv->hideAllActiveEditors();
        });
        addAction("encapsulate", [this] {
            select({ "print a", "print b" });
            cnv->encapsulateSelection();
        });
        addAction("shift-drag into connection", [this] {
            auto* settings = SettingsFile::getInstance();
            auto const gridWas = settings->getProperty<int>("grid_enabled");
            settings->setProperty("grid_enabled", 0);

            auto* obj = find("+ 1");
            auto* connection = findConnection("t b", "print e");
            if (obj && connection) {
                cnv->deselectAll();
                auto const target = (connection->getStartPoint() + connection->getEndPoint()) / 2.0f;
                auto const offset = target - obj->iolets[0]->getCanvasBounds().getCentre().toFloat();
                auto const centre = obj->getLocalBounds().getCentre().toFloat();
                drag(obj, centre, centre + offset, ModifierKeys::leftButtonModifier | ModifierKeys::shiftModifier);
            }
            settings->setProperty("grid_enabled", gridWas);
        });
        addAction("change order", [this] {
            // Same steps as the "To Front" item in the object's right-click menu
            if (auto* obj = find("tgl")) {
                cnv->patch.startUndoSequence("ToFront");
                obj->gui->moveToFront();
                cnv->patch.endUndoSequence("ToFront");
                cnv->synchronise();
            }
        });
        steps.add([this] {
            if (auto* obj = find("metro 500"))
                clickIolet(obj->iolets[obj->numInputs]);
        });
        addAction("create connection", [this] {
            if (auto* obj = find("print d"))
                clickIolet(obj->iolets[0]);
        });
        addAction("delete connection", [this] {
            if (auto* c = findConnection("*~ 0.5", "dac~")) {
                cnv->deselectAll();
                cnv->setSelected(c, true);
                cnv->selectedComponents.sendSynchronousChangeMessage();
                cnv->removeSelection();
            }
        });
        addAction("delete object", [this] {
            select({ "osc~ 440" });
            cnv->removeSelection();
        });

        addAction("curved connection", [this] {
            if (auto* c = findConnection("i", "print d")) {
                cnv->deselectAll();
                cnv->setSelected(c, true);
                editor->invokeDirectly(CommandIDs::ConnectionStyle, false);
            }
        });
        addAction("add connection corner", [this] {
            if (auto* c = findConnection("i", "print d")) {
                cnv->deselectAll();
                clickConnection(c, (c->getStartPoint() + c->getEndPoint()) / 2.5f, 2);
            }
        });
        addAction("drag connection segment", [this] {
            if (auto* c = findConnection("i", "print d")) {
                auto const middle = (c->getStartPoint() + c->getEndPoint()) / 2.0f;
                auto const from = c->getLocalPoint(cnv, middle);
                c->mouseDown(makeEvent(c, from, from));
                c->mouseDrag(makeEvent(c, from + Point<float>(25, 0), from));
                c->mouseUp(makeEvent(c, from + Point<float>(25, 0), from));
            }
        });
        addAction("remove connection corner", [this] {
            if (auto* c = findConnection("i", "print d")) {
                auto const start = c->getStartPoint();
                auto const end = c->getEndPoint();
                // The first corner sits level with the outlet, on the dragged segment
                clickConnection(c, { (start.x + end.x) / 2.0f + 25.0f, start.y }, 2);
            }
        });

        steps.add([this] {
            if (cnv->patch.getCanvasContent() == lastContent)
                actionsWithoutEffect.add(previousAction);
            expect(actionsWithoutEffect.isEmpty(), "these actions didn't change the patch: " + actionsWithoutEffect.joinIntoString(", "));

            cnv->deselectAll();
            edited = cnv->patch.getCanvasContent();
            undoAll();
        });
    }

    bool invoke(CommandID command)
    {
        return editor->invoke(ApplicationCommandTarget::InvocationInfo(command), false);
    }

    void undoAll()
    {
        if (numUndos < 200 && invoke(CommandIDs::Undo)) {
            numUndos++;
            Timer::callAfterDelay(80, [this] { undoAll(); });
            return;
        }
        expectContent(original, "undoing all " + String(numUndos) + " steps must restore the original patch");
        redoAll();
    }

    void redoAll()
    {
        if (numRedos < numUndos && invoke(CommandIDs::Redo)) {
            numRedos++;
            Timer::callAfterDelay(80, [this] { redoAll(); });
            return;
        }
        expectEquals(numRedos, numUndos, "every undone step must be redoable");
        expectContent(edited, "redoing all steps must restore the edited patch");

        editor->getTabComponent().closeTab(cnv);
        signalDone(true);
    }

    void expectContent(String const& expected, String const& message)
    {
        auto const actual = cnv->patch.getCanvasContent();
        if (actual != expected) {
            auto const a = StringArray::fromLines(expected), b = StringArray::fromLines(actual);
            for (int i = 0; i < std::max(a.size(), b.size()); i++)
                if (a[i] != b[i])
                    logMessage("line " + String(i) + ": expected '" + a[i] + "', got '" + b[i] + "'");
        }
        expect(actual == expected, message);
    }

    void runStep(int const index)
    {
        if (!cnv) {
            signalDone(false);
            return;
        }
        if (index >= steps.size())
            return;
        steps[index]();
        Timer::callAfterDelay(150, [this, index] { runStep(index + 1); });
    }

    HeapArray<std::function<void()>> steps;
    Component::SafePointer<Canvas> cnv;
    String original, edited, lastContent, previousAction;
    StringArray actionsWithoutEffect;
    int numUndos = 0, numRedos = 0;
};
