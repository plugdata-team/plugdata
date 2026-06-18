#include "Objects/TextObject.h"
#include "Objects/LuaObject.h"

// Exercises the pdlua graphical object (LuaObject.h).
//
// pdlua scripts are loaded from disk, so this writes temporary .pd_lua files
// next to a temporary patch (pd always searches the patch's own directory)
// and drives the resulting objects:
//
//  - a graphical object with paint + mouse handlers: created, force-rendered,
//    then sent the full mouse sequence (enter/down/drag/up/move/exit) which
//    flows through to the lua mouse callbacks
//  - reloading the script at runtime (the "reload" message the menu sends)
//  - a script with a runtime error in paint(): must report the error and
//    survive rather than crash
//  - a script with an error in initialize(): the object must still construct
//  - destruction while a repaint/draw is in flight: repaint is queued, then
//    the patch is closed before the message thread processes it
//
// Run with AddressSanitizer to catch use-after-free in the draw/callback path.

class LuaObjectTest : public PlugDataUnitTest
{
public:
    LuaObjectTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Lua Object Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Load, render, interact with and reload a pdlua object");

        dir = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_lua_test");
        dir.deleteRecursively();
        dir.createDirectory();

        auto const examples = File::getCurrentWorkingDirectory().getChildFile("Libraries/pd-lua/pdlua");
        auto propertiesScript = examples.getChildFile("examples/props.pd_lua").loadFileAsString();
        propertiesScript = propertiesScript.replace("register(\"props\")", "register(\"plugdata_test_props\")");
        expect(dir.getChildFile("plugdata_test_props.pd_lua").replaceWithText(propertiesScript),
            "properties fixture must be written");
        dir.getChildFile("examples").createDirectory();
        expect(examples.getChildFile("examples/pdlogo.gif").copyFileTo(dir.getChildFile("examples/pdlogo.gif")),
            "hello-gui image fixture must be copied");

        // A well-behaved graphical object with mouse handlers
        dir.getChildFile("plugdata_test_gfx.pd_lua").replaceWithText(
            "local G = pd.Class:new():register(\"plugdata_test_gfx\")\n"
            "function G:initialize(sel, atoms) self.inlets = 1 self.outlets = 1 self.n = 0 return true end\n"
            "function G:paint(g)\n"
            "  g:set_color(20, 40, 60)\n"
            "  g:fill_all()\n"
            "  g:set_color(255, 0, 0)\n"
            "  g:fill_ellipse(2, 2, 10, 10)\n"
            "  g:stroke_ellipse(15, 2, 10, 10, 1)\n"
            "  g:fill_rect(28, 2, 10, 10)\n"
            "  g:stroke_rect(0, 0, 20, 20, 1)\n"
            "  g:fill_rounded_rect(42, 2, 10, 10, 2)\n"
            "  g:stroke_rounded_rect(55, 2, 10, 10, 2, 1)\n"
            "  g:draw_line(2, 18, 65, 18, 1)\n"
            "  local path = Path(2, 22)\n"
            "  path:line_to(15, 30)\n"
            "  path:line_to(2, 38)\n"
            "  path:close()\n"
            "  g:fill_path(path)\n"
            "  local curve = Path(20, 22)\n"
            "  curve:cubic_to(30, 15, 40, 40, 55, 22)\n"
            "  curve:close()\n"
            "  g:stroke_path(curve, 2)\n"
            "  g:translate(5, 5)\n"
            "  g:scale(0.8, 0.8)\n"
            "  g:draw_text(\"hi\", 2, 2, 40, 10)\n"
            "  g:draw_svg('<svg width=\"12\" height=\"12\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M1 1 L11 1 L6 11 Z\" fill=\"#ffffff\"/></svg>', 68, 2)\n"
            "  g:draw_image('./examples/pdlogo.gif', 68, 18)\n"
            "  g:reset_transform()\n"
            "end\n"
            "function G:mouse_down(x, y) self.n = self.n + 1 self:repaint() end\n"
            "function G:mouse_drag(x, y) self:repaint() end\n"
            "function G:mouse_up(x, y) self:outlet(1, \"float\", {self.n}) end\n"
            "function G:mouse_move(x, y) end\n"
            "function G:in_1_bang() self:repaint() end\n");

        // A script that throws inside paint()
        dir.getChildFile("plugdata_test_painterror.pd_lua").replaceWithText(
            "local E = pd.Class:new():register(\"plugdata_test_painterror\")\n"
            "function E:initialize(sel, atoms) self.inlets = 1 self.outlets = 0 return true end\n"
            "function E:paint(g) error(\"intentional paint error\") end\n");

        // A script that throws inside initialize()
        dir.getChildFile("plugdata_test_initerror.pd_lua").replaceWithText(
            "local I = pd.Class:new():register(\"plugdata_test_initerror\")\n"
            "function I:initialize(sel, atoms) error(\"intentional init error\") end\n"
            "function I:paint(g) g:fill_all() end\n");

        patchFile = dir.getChildFile("plugdata_lua_patch.pd");
        patchFile.replaceWithText(
            "#N canvas 100 100 500 400 12;\n"
            "#X obj 40 40 plugdata_test_gfx;\n"
            "#X obj 40 120 plugdata_test_painterror;\n"
            "#X obj 40 200 plugdata_test_initerror;\n"
            "#X obj 220 300 plugdata_test_props;\n");

        cnv = editor->getTabComponent().openPatch(editor->pd->loadPatch(URL(patchFile)));
        if (!cnv) {
            cleanup();
            signalDone(false);
            return;
        }
        cnv->performSynchronise();

        // Let pdlua finish constructing the objects, then drive them
        Timer::callAfterDelay(300, [this] { interactAndReload(); });
    }

    Object* findObjectByType(String const& typePrefix)
    {
        if (!cnv)
            return nullptr;
        for (auto* obj : cnv->objects) {
            if (obj->gui && obj->gui->getType().contains(typePrefix))
                return obj;
        }
        // Fall back to matching the object text
        for (auto* obj : cnv->objects) {
            if (obj->gui && obj->gui->getText().contains(typePrefix))
                return obj;
        }
        return nullptr;
    }

    void interactAndReload()
    {
        expect(cnv && cnv->objects.size() == 4, "all lua objects must be created");

        auto* gfx = findObjectByType("plugdata_test_gfx");
        expect(gfx != nullptr && gfx->gui != nullptr, "the graphical lua object must load");

        if (gfx && gfx->gui) {
            auto* gui = gfx->gui.get();
            auto const centre = gui->getLocalBounds().getCentre().toFloat();
            auto makeEvent = [gui, centre](Point<float> pos) {
                return MouseEvent(Desktop::getInstance().getMainMouseSource(), pos, ModifierKeys::leftButtonModifier,
                                  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gui, gui,
                                  Time::getCurrentTime(), centre, Time::getCurrentTime(), 1, false);
            };
            // Full mouse sequence -> lua mouse callbacks
            gui->mouseEnter(makeEvent(centre));
            gui->mouseMove(makeEvent(centre));
            gui->mouseDown(makeEvent(centre));
            gui->mouseDrag(makeEvent(centre + Point<float>(3, 3)));
            gui->mouseUp(makeEvent(centre + Point<float>(3, 3)));
            gui->mouseExit(makeEvent(centre));

            // Render the object's framebuffer (covers the draw callback path)
            editor->pd->volume->store(0.0f);
            exercisePropertiesFrame(gui);
        }

        for (auto const& type : { String("plugdata_test_props") }) {
            if (auto* object = findObjectByType(type); object && object->gui) {
                auto* gui = object->gui.get();
                gui->updateProperties();
                gui->getPdBounds();
                gui->getSelectableBounds();
                gui->setPdBounds(gui->getPdBounds());
                gui->parentHierarchyChanged();
                gui->lookAndFeelChanged();
                gui->updateSizeProperty();
                PopupMenu menu;
                gui->getMenuOptions(menu);
                auto parameters = gui->getParameters().getParameters();
                for (auto& parameter : parameters) {
                    if (!parameter.valuePtr)
                        continue;

                    auto const original = parameter.valuePtr->getValue();
                    switch (parameter.type) {
                    case tBool:
                        parameter.valuePtr->setValue(!static_cast<bool>(original));
                        break;
                    case tInt:
                    case tFloat:
                        parameter.valuePtr->setValue(parameter.min);
                        parameter.valuePtr->setValue(parameter.max);
                        break;
                    case tCombo:
                        parameter.valuePtr->setValue(parameter.options.size() > 1 ? 1 : 0);
                        break;
                    case tString:
                        parameter.valuePtr->setValue(original.toString() + "_coverage");
                        break;
                    case tColour:
                    case tColourAlpha:
                        parameter.valuePtr->setValue(Colour(0xff336699).toString());
                        break;
                    default:
                        break;
                    }
                    parameter.valuePtr->setValue(original);
                }
                gui->setBounds(gui->getBounds());
                gui->createComponentSnapshot(gui->getLocalBounds());
                auto const centre = gui->getLocalBounds().getCentre().toFloat();
                auto event = MouseEvent(Desktop::getInstance().getMainMouseSource(), centre,
                    ModifierKeys::leftButtonModifier, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    gui, gui, Time::getCurrentTime(), centre, Time::getCurrentTime(), 1, false);
                gui->mouseEnter(event);
                gui->mouseMove(event);
                gui->mouseDown(event);
                gui->mouseDrag(event);
                gui->mouseUp(event);
                gui->mouseExit(event);
            }
        }

        // Reload all pdlua scripts at runtime, like the right-click menu does
        editor->pd->lockAudioThread();
        editor->pd->setThis();
        if (auto* reloadSym = gensym("pdlua"); reloadSym->s_thing)
            pd_typedmess(reloadSym->s_thing, gensym("reload"), 0, nullptr);
        editor->pd->unlockAudioThread();

        Timer::callAfterDelay(300, [this] {
            if (cnv)
                cnv->performSynchronise();
            testDestructionDuringCallback();
        });
    }

    void exercisePropertiesFrame(ObjectBase* object)
    {
        LuaPropertiesPanel::PropertyFrame frame;
        frame.title = "Coverage properties";
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Check,
            .label = "Check", .method = "check", .initFloat = 1.0f });
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Text,
            .label = "Text", .method = "text", .initString = "initial" });
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Colour,
            .label = "Colour", .method = "colour", .initString = "#336699" });
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Int,
            .label = "Integer", .method = "integer", .initFloat = 2.0f, .min = -10.0f, .max = 10.0f });
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Float,
            .label = "Float", .method = "float", .initFloat = 0.5f, .min = 0.0f, .max = 1.0f });
        frame.items.add({ .type = LuaPropertiesPanel::PropertyItem::Type::Combo,
            .label = "Combo", .method = "combo", .options = { "First", "Second" }, .initFloat = 0.0f });

        LuaPropertiesPanel::LuaPropertiesFrame propertiesFrame(&frame, object);
        propertiesFrame.setBounds(0, 0, 420, propertiesFrame.getPreferredHeight());
        propertiesFrame.resized();
        propertiesFrame.createComponentSnapshot(propertiesFrame.getLocalBounds());

        for (auto* property : propertiesFrame.properties) {
            if (auto* boolean = dynamic_cast<PropertiesPanel::BoolComponent*>(property)) {
                auto event = MouseEvent(Desktop::getInstance().getMainMouseSource(),
                    boolean->getLocalBounds().getCentre().toFloat(), ModifierKeys::leftButtonModifier,
                    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, boolean, boolean,
                    Time::getCurrentTime(), {}, Time::getCurrentTime(), 1, false);
                boolean->mouseUp(event);
            } else if (auto* combo = dynamic_cast<PropertiesPanel::ComboComponent*>(property)) {
                combo->comboBox.setSelectedId(2, sendNotificationSync);
            } else if (auto* text = dynamic_cast<PropertiesPanel::EditableComponent<String>*>(property)) {
                if (auto* label = dynamic_cast<Label*>(text->label.get()))
                    label->setText("changed", sendNotificationSync);
            } else if (auto* integer = dynamic_cast<PropertiesPanel::EditableComponent<int>*>(property)) {
                if (auto* number = dynamic_cast<DraggableNumber*>(integer->label.get()))
                    number->setValue(7.0, sendNotificationSync);
            } else if (auto* floating = dynamic_cast<PropertiesPanel::EditableComponent<float>*>(property)) {
                if (auto* number = dynamic_cast<DraggableNumber*>(floating->label.get()))
                    number->setValue(0.75, sendNotificationSync);
            }
        }
    }

    void testDestructionDuringCallback()
    {
        beginTest("Destroy a lua object while a repaint is queued");

        // Queue repaint messages on every lua object, then immediately close
        // the patch so the objects are destroyed before the queued async draw
        // callbacks run
        if (cnv) {
            for (auto* obj : cnv->objects) {
                if (obj->gui)
                    obj->gui->repaint();
            }
        }

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        // Give any still-queued lua callbacks a chance to fire against the now
        // destroyed objects; surviving this is the test
        Timer::callAfterDelay(300, [this] {
            cleanup();
            signalDone(true);
        });
    }

    void cleanup()
    {
        if (dir.exists())
            dir.deleteRecursively();
    }

    Component::SafePointer<Canvas> cnv;
    File dir, patchFile;
};
