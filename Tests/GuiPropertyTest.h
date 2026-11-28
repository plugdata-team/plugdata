#include "Constants.h"
#include "Pd/Library.h"
#include "Objects/TextObject.h"
#include "Objects/LuaObject.h"

// Sends every GUI object each method from its documentation, then changes every inspector property.
// Pd messages must not add undo steps. Each inspector change must reach Pd as one undo step.

class GuiPropertyTest : public PlugDataUnitTest
{
public:
    GuiPropertyTest(PluginEditor* editor) : PlugDataUnitTest(editor, "GUI Property Test")
    {
    }

private:
    static constexpr char const* objectsToCreate[] = {
        "#X obj 30 30 bng 25 250 50 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000;",
        "#X obj 100 30 tgl 25 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1;",
        "#X obj 170 30 nbx 5 14 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0 256;",
        "#X obj 250 30 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 #fcfcfc #000000 #000000 0 1;",
        "#X obj 30 120 vsl 17 128 0 127 0 0 empty empty empty 0 -9 0 10 #fcfcfc #000000 #000000 0 1;",
        "#X obj 100 120 hradio 17 1 0 8 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0;",
        "#X obj 220 120 vradio 17 1 0 8 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0;",
        "#X obj 320 120 cnv 15 100 60 empty empty empty 20 12 0 14 #e0e0e0 #404040 0;",
        "#X obj 30 260 vu 17 120 empty empty -1 -8 0 10 #404040 #000000 1 0;",
        "#X obj 100 260 numbox~;",
        "#X obj 200 260 knob;",
        "#X obj 280 260 function;",
        "#X obj 380 260 scope~;",
        "#X obj 30 380 colors;",
        "#X obj 100 380 messbox;",
        "#X obj 200 380 note;",
        "#X obj 300 380 popmenu;",
        "#X obj 30 460 keyboard;",
        "#X obj 150 460 pad;",
        "#X floatatom 250 460 5 0 0 0 - - - 0;",
        "#X symbolatom 330 460 10 0 0 0 - - - 0;",
        "#X listbox 30 530 20 0 0 0 - - - 0;",
        "#X obj 30 600 button;",
        "#X obj 120 600 pic;",
        "#X obj 220 600 bicoeff;",
        "#X obj 500 30 circle;",
        "#X msg 400 600 hello world;",
        "#X text 500 600 a comment;",
        "#X obj 600 600 print gui;",
        "#N canvas 0 50 450 300 sub 0;\n#X restore 30 700 pd sub;",
        "#N canvas 0 50 450 300 gop 0;\n#X coords 0 -1 1 1 85 60 1 0 0;\n#X restore 150 700 pd gop;",
        "#N canvas 0 50 450 250 (subpatch) 0;\n#X array gui_test_array 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 300 700 graph;",
    };

    void perform() override
    {
        beginTest("Open the GUI patch");
        String text = "#N canvas 100 100 900 900 12;\n";
        for (auto const* line : objectsToCreate)
            text += String(line) + "\n";

        cnv = editor->getTabComponent().openPatch(text);
        if (!cnv) {
            expect(false, "the patch must open");
            signalDone(false);
            return;
        }

        Timer::callAfterDelay(200, [this] {
            cnv->locked.setValue(false);
            cnv->performSynchronise();
            for (auto* obj : cnv->objects)
                objects.add(obj);

            beginTest("Every GUI object is created");
            if (objects.size() != static_cast<int>(std::size(objectsToCreate))) {
                expect(false, "expected " + String(static_cast<int>(std::size(objectsToCreate))) + " objects, got " + String(objects.size()));
                signalDone(false);
                return;
            }
            runNext();
        });
    }

    Object* objectAt(int const index)
    {
        return index < objects.size() ? objects[index].getComponent() : nullptr;
    }

    int undoSize()
    {
        return pd::Interface::getUndoSize(cnv->patch.getRawPointer());
    }

    String content()
    {
        return cnv->patch.getCanvasContent();
    }

    void after(int const ms, std::function<void()> fn)
    {
        Timer::callAfterDelay(ms, std::move(fn));
    }

    void advance()
    {
        item++;
        runNext();
    }

    void nextObject()
    {
        objIndex++;
        item = 0;
        runNext();
    }

    void finish()
    {
        if (cnv)
            editor->getTabComponent().closeTab(cnv);
        signalDone(true);
    }

    void runNext()
    {
        if (!cnv || objIndex >= objects.size()) {
            finish();
            return;
        }

        auto* obj = objectAt(objIndex);
        if (!obj || !obj->gui) {
            nextObject();
            return;
        }

        auto const type = obj->gui->getType().toString();
        if (item == 0) {
            beginTest(type + " messages");
            auto const& info = editor->pd->objectLibrary->getObjectInfo(obj->gui->getTypeWithOriginPrefix());
            if (type != "canvas") // Subpatches and graphs have no class documentation
                expect(info.title.isNotEmpty(), type + " must have documentation");
            messages = documentedMessages(info);
        }
        if (item < messages.size()) {
            sendAndCheck(obj, messages[item]);
            return;
        }

        auto const propertyIndex = item - messages.size();
        if (propertyIndex < obj->gui->getParameters().getParameters().size()) {
            if (propertyIndex == 0)
                beginTest(type + " properties");
            checkProperty(propertyIndex);
            return;
        }

        nextObject();
    }

    // Turns each documented method, like "label_pos <float, float>", into a message with arguments of those types
    StringArray documentedMessages(pd::Library::ObjectReferenceTable const& info) const
    {
        StringArray result;
        for (auto const& method : info.methods) {
            auto const name = method.type.upToFirstOccurrenceOf(" ", false, false);
            auto const types = StringArray::fromTokens(method.type.fromFirstOccurrenceOf("<", false, false).upToFirstOccurrenceOf(">", false, false), ",", "");
            auto message = name;
            for (int i = 0; i < types.size(); i++)
                message += " " + argumentFor(name, types[i].trim(), i);
            result.add(message);
        }
        return result;
    }

    // Symbols are unique per object and method, so no object ends up sending to itself or another one
    String argumentFor(String const& method, String const& type, int const index) const
    {
        if (type == "float")
            return String(index == 0 ? 1 : index * 30);
        if (method == "open")
            return "else/green.gif"; // The image [pic]'s help file opens
        if (type == "symbol")
            return method.containsIgnoreCase("color") || method == "hex" ? "#ff0000" : "gui_" + String(objIndex) + "_" + method;
        return "2 30 60";
    }

    void sendTo(Object* obj, String const& message)
    {
        auto const selector = message.upToFirstOccurrenceOf(" ", false, false);
        auto const args = message.fromFirstOccurrenceOf(" ", false, false);

        // pdlua objects get messages from a patch through a proxy for their first inlet
        if (dynamic_cast<LuaObject*>(obj->gui.get())) {
            if (auto lua = obj->gui->ptr.get<t_pdlua>(); lua && lua->inlets > 0) {
                editor->pd->sendDirectMessage(lua->proxy_in, selector.toRawUTF8(), pd::Atom::atomsFromString(args));
                return;
            }
        }
        obj->gui->sendMessage(selector.toRawUTF8(), pd::Atom::atomsFromString(args));
    }

    void sendAndCheck(Object* obj, String const& message)
    {
        editor->pd->flushConsoleMessages();
        editor->getSidebarForPanel(Sidebar::ConsolePanel)->clearConsole();
        auto const undos = undoSize();
        sendTo(obj, message);

        after(30, [this, undos, message] {
            auto* target = objectAt(objIndex);
            if (!target || !target->gui) {
                expect(false, "the object vanished after '" + message + "'");
                nextObject();
                return;
            }

            editor->pd->flushConsoleMessages();
            StringArray errors;
            for (auto const& [object, text, type, length, repeats] : editor->pd->getConsoleMessages()) {
                if (type == 1)
                    errors.add(text);
            }
            expect(errors.isEmpty(), "'" + message + "' printed errors: " + errors.joinIntoString(" | "));
            expect(undoSize() == undos, "'" + message + "' must not add an undo step");

            auto const names = namesOf(target);
            auto const before = valuesOf(target);
            target->gui->updateProperties();
            auto const synced = valuesOf(target);
            for (int i = 0; i < before.size(); ++i)
                expect(sameValue(before[i], synced[i]), "'" + message + "' left " + names[i] + " at " + describe(before[i]) + " in the inspector, Pd has " + describe(synced[i]));

            advance();
        });
    }

    void checkProperty(int const index)
    {
        auto* obj = objectAt(objIndex);
        auto const param = obj->gui->getParameters().getParameters()[index];
        if (!param.valuePtr || param.type == tCustom) {
            advance();
            return;
        }

        auto const name = obj->gui->getType().toString() + " " + param.name;
        auto const undos = undoSize();
        auto const before = content();
        auto const value = nextValue(param, "gui_" + String(objIndex) + "_" + String(index));
        param.valuePtr->setValue(value);

        after(30, [this, index, undos, before, value, name] {
            auto* target = objectAt(objIndex);
            if (!target || !target->gui) {
                expect(false, name + ": the object vanished");
                nextObject();
                return;
            }

            auto const edited = content();
            expect(undoSize() == undos + 1, name + " must add exactly one undo step, the count went from " + String(undos) + " to " + String(undoSize()));
            expect(edited != before, name + " must change the patch");

            auto const shown = valuesOf(target)[index];
            target->gui->updateProperties();
            auto const reached = valuesOf(target)[index];
            expect(sameValue(shown, reached), name + " shows " + describe(shown) + " after setting it to " + describe(value) + ", but Pd has " + describe(reached));

            editor->invokeDirectly(CommandIDs::Undo, false);
            after(30, [this, undos, before, edited, name] {
                if (content() != before)
                    logDifference(before, content());
                expect(content() == before, name + " undo must restore the previous patch");

                editor->invokeDirectly(CommandIDs::Redo, false);
                after(30, [this, edited, name] {
                    if (content() != edited)
                        logDifference(edited, content());
                    expect(content() == edited, name + " redo must reapply the change");
                    advance();
                });
            });
        });
    }

    static Array<var> valuesOf(Object* obj)
    {
        Array<var> values;
        for (auto const& param : obj->gui->getParameters().getParameters())
            values.add(param.valuePtr ? param.valuePtr->getValue() : var());
        return values;
    }

    static String describe(var const& value)
    {
        return value.isArray() ? JSON::toString(value, true) : value.toString();
    }

    static StringArray namesOf(Object* obj)
    {
        StringArray names;
        for (auto const& param : obj->gui->getParameters().getParameters())
            names.add(param.name);
        return names;
    }

    void logDifference(String const& expected, String const& actual)
    {
        auto const a = StringArray::fromLines(expected), b = StringArray::fromLines(actual);
        for (int i = 0; i < std::max(a.size(), b.size()); i++) {
            if (a[i] != b[i])
                logMessage("line " + String(i) + ": expected '" + a[i] + "', got '" + b[i] + "'");
        }
    }

    static bool sameValue(var const& a, var const& b)
    {
        if (a.isArray() && b.isArray()) {
            auto const* x = a.getArray();
            auto const* y = b.getArray();
            if (x->size() != y->size())
                return false;
            for (int i = 0; i < x->size(); ++i) {
                if (!sameValue(x->getUnchecked(i), y->getUnchecked(i)))
                    return false;
            }
            return true;
        }
        if (a.isString() || b.isString())
            return a.toString() == b.toString();
        return std::abs(double(a) - double(b)) < 1e-3;
    }

    static var nextValue(ObjectParameter const& p, String const& symbol)
    {
        auto const& current = p.valuePtr->getValue();
        switch (p.type) {
        case tColour:
        case tColourAlpha: {
            auto const first = colourToVar(Colour(0xff13a5c7)), second = colourToVar(Colour(0xff0c7d2a));
            return current == first ? second : first;
        }
        case tString:
            return current.toString() == symbol ? var(symbol + "b") : var(symbol);
        case tFont:
            return current.toString() == "Menlo" ? var("Helvetica") : var("Menlo");
        case tBool:
            return var(double(current) == 0.0 ? 1 : 0);
        case tCombo: {
            if (p.options.size() < 2)
                return current;
            if (current.isString())
                return p.options[(p.options.indexOf(current.toString()) + 1) % p.options.size()];
            return var((int(current) + 1) % p.options.size());
        }
        case tRangeFloat:
        case tRangeInt: {
            if (!current.isArray())
                return current;
            auto values = *current.getArray();
            values.set(0, var(double(values[0]) + 1.0));
            return var(values);
        }
        default: {
            auto value = double(current) + 1.0;
            if (p.clip && (value < p.min || value > p.max))
                value -= 2.0;
            return p.type == tInt ? var(int(value)) : var(value);
        }
        }
    }

    Component::SafePointer<Canvas> cnv;
    HeapArray<Component::SafePointer<Object>> objects;
    StringArray messages;
    int objIndex = 0, item = 0;
};
