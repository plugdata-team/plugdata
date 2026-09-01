// Shows the sidebar inspector for one of every common object type and clicks
// through the property rows it builds.
//
// The inspector is the main in-editor consumer of PropertiesPanel: every
// object type contributes a different mix of property components (draggable
// numbers, toggles, combo boxes, colour swatches, text fields, range
// editors), and almost none of that runs in the other tests. For each object
// this selects it, pushes its parameters through both the regular
// (updateSidebarSelection) and forced (forceShowParameters) paths, then
// delivers a click to every visible component inside the sidebar - which also
// opens and exercises the colour picker callout. Menus and callouts left open
// by a click are dismissed before moving on.

#include "Components/ColourPicker.h"

class InspectorTest : public PlugDataUnitTest
{
public:
    InspectorTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Inspector Test")
    {
    }

private:
    void perform() override
    {
        cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 900 600 12;\n"
            "#X obj 30 30 bng 25 250 50 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000;\n"
            "#X obj 70 30 tgl 25 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 110 30 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 110 60 nbx 5 14 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0 256;\n"
            "#X obj 250 30 cnv 15 100 60 empty empty empty 20 12 0 14 #e0e0e0 #404040 0;\n"
            "#X obj 30 200 vu 17 120 empty empty -1 -8 0 10 #404040 #000000 1 0;\n"
            "#X obj 70 200 hradio 17 1 0 8 empty empty empty 0 -8 0 10 #fcfcfc #000000 #000000 0;\n"
            "#X obj 30 350 knob;\n"
            "#X obj 90 350 function;\n"
            "#X obj 150 350 scope~;\n"
            "#X msg 300 350 a message;\n"
            "#X floatatom 380 350 5 0 0 0 - - -;\n"
            "#X text 450 350 a comment;\n"
            "#X obj 520 350 metro 100;\n"));

        if (!cnv || cnv->objects.empty()) {
            signalDone(false);
            return;
        }

        for (auto* obj : cnv->objects)
            objects.add(obj);

        inspectNextObject(0);
    }

    void inspectNextObject(int const index)
    {
        if (!cnv || index >= static_cast<int>(objects.size())) {
            finish();
            return;
        }

        auto* obj = objects[index].getComponent();
        if (!obj || !obj->gui) {
            inspectNextObject(index + 1);
            return;
        }

        beginTest("Inspect [" + obj->gui->getType() + "]");

        // The normal path: selection change posts the parameters to the sidebar
        cnv->deselectAll();
        cnv->setSelected(obj, true);
        cnv->updateSidebarSelection();

        Timer::callAfterDelay(50, [this, index] {
            auto* obj = objects[index].getComponent();
            auto* sidebar = editor->getSidebarForPanel(Sidebar::InspectorPanel);
            if (!cnv || !obj || !obj->gui || !sidebar) {
                inspectNextObject(index + 1);
                return;
            }

            // The forced path, so the inspector is visible regardless of the
            // user's auto-show setting
            SmallArray<Component*> toShow = { obj };
            SmallArray<ObjectParameters, 6> params = { obj->gui->getParameters() };
            sidebar->forceShowParameters(toShow, params);

            Timer::callAfterDelay(50, [this, index, sidebar] {
                HeapArray<Component::SafePointer<Component>> targets;
                collectComponents(sidebar, targets);
                for (auto& target : targets) {
                    if (auto* component = target.getComponent()) {
                        if (component->isShowing() && component->isEnabled())
                            simulateClick(component);
                    }
                }

                PopupMenu::dismissAllActiveMenus();
                ModalComponentManager::getInstance()->cancelAllModalComponents();

                Timer::callAfterDelay(30, [this, index] {
                    inspectNextObject(index + 1);
                });
            });
        });
    }

    static void collectComponents(Component* c, HeapArray<Component::SafePointer<Component>>& targets)
    {
        for (auto* child : c->getChildren()) {
            if (!child->isShowing())
                continue;
            targets.add(child);
            collectComponents(child, targets);
        }
    }

    static void simulateClick(Component* c)
    {
        bool allowsMouseClicks, allowsMouseClicksOnChildren;
        c->getInterceptsMouseClicks(allowsMouseClicks, allowsMouseClicksOnChildren);
        if (!allowsMouseClicks)
            return;

        auto const centre = c->getLocalBounds().getCentre().toFloat();
        MouseEvent e(Desktop::getInstance().getMainMouseSource(),
                     centre, ModifierKeys::leftButtonModifier,
                     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                     Time::getCurrentTime(), centre, Time::getCurrentTime(), 1, false);
        c->mouseDown(e);
        c->mouseUp(e);
    }

    void finish()
    {
        ColourPicker::getInstance()->clearSingletonInstance();
        if (auto* sidebar = editor->getSidebarForPanel(Sidebar::InspectorPanel))
            sidebar->clearInspector();

        auto& tabbar = editor->getTabComponent();
        while (auto* c = tabbar.getCurrentCanvas())
            tabbar.closeTab(c);

        signalDone(true);
    }

    Component::SafePointer<Canvas> cnv;
    HeapArray<Component::SafePointer<Object>> objects;
};
