// A subpatch that is open in a tab can be turned into a graph and back, from the object in its
// parent patch or from the tab itself. Both replace the object's GUI, which used to close the
// tab. A subpatch turned into a graph from its own tab also kept that tab after being deleted.

#include "Canvas.h"
#include "Object.h"

class SubpatchTabTest : public PlugDataUnitTest {
public:
    SubpatchTabTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Subpatch Tab Test")
    {
    }

private:
    static constexpr int settleTime = 100;

    // Synchronising a canvas and replacing its object's GUI are separate queued
    // updates. A fixed delay can expire between them on a busy message thread.
    // Always run the assertions, including when the expected state times out.
    void waitFor(std::function<bool()> ready, std::function<void()> check, double deadline = 0.0)
    {
        if (deadline == 0.0)
            deadline = Time::getMillisecondCounterHiRes() + 5000.0;

        if (ready() || Time::getMillisecondCounterHiRes() >= deadline) {
            check();
            return;
        }

        Timer::callAfterDelay(20, [this, ready = std::move(ready), check = std::move(check), deadline] {
            waitFor(ready, check, deadline);
        });
    }

    void perform() override
    {
        beginTest("Turning a graph into a subpatch keeps its tab open");

        auto& tabbar = editor->getTabComponent();
        parent = tabbar.openPatch(String("#N canvas 0 50 450 300 12;\n"
                                         "#N canvas 0 50 450 300 graph 0;\n"
                                         "#X coords 0 1 1 0 100 80 1 0 0;\n"
                                         "#X restore 50 50 pd graph;\n"));
        if (!parent || parent->objects.empty() || !parent->objects[0]->gui) {
            signalDone(false);
            return;
        }

        object = parent->objects[0];
        tab = tabbar.openPatch(object->gui->getPatch());

        Timer::callAfterDelay(settleTime, [this] { turnIntoSubpatch(); });
    }

    void turnIntoSubpatch()
    {
        if (!canContinue())
            return;

        for (auto& parameter : object->gui->getParameters().getParameters()) {
            if (parameter.name == "Is graph")
                parameter.valuePtr->setValue(false);
        }

        waitFor([this] { return !tab || (!showsGraph() && !getValue<bool>(tab->isGraphChild)); }, [this] {
            expect(tab != nullptr, "the tab must stay open");
            expect(!showsGraph(), "the object in the parent must become a plain subpatch");
            if (tab)
                expect(!getValue<bool>(tab->isGraphChild), "the tab must know it is no longer a graph");
            turnIntoGraph();
        });
    }

    void turnIntoGraph()
    {
        if (!canContinue())
            return;

        beginTest("Turning a subpatch into a graph from its own tab updates the parent");

        tab->isGraphChild = true;

        waitFor([this] { return !tab || showsGraph(); }, [this] {
            expect(tab != nullptr, "the tab must stay open");
            expect(showsGraph(), "the object in the hidden parent must become a graph");
            editor->getTabComponent().showTab(parent, parent->patch.splitViewIndex);
            deleteObject();
        });
    }

    void deleteObject()
    {
        if (!canContinue())
            return;

        beginTest("Deleting the object closes its tab");

        parent->setSelected(object, true);
        parent->removeSelection();

        waitFor([this] { return tab == nullptr; }, [this] {
            expect(tab == nullptr, "the tab must close with its object");
            finish(true);
        });
    }

    // A graph draws its subpatch on a canvas inside the object
    bool showsGraph() const
    {
        return object && object->gui && TestHelpers::findChildOfType<Canvas>(object->gui.get()) != nullptr;
    }

    bool canContinue()
    {
        if (parent && object && object->gui && tab)
            return true;

        finish(false);
        return false;
    }

    void finish(bool const completed)
    {
        auto& tabbar = editor->getTabComponent();
        if (tab)
            tabbar.closeTab(tab);
        if (parent)
            tabbar.closeTab(parent);

        signalDone(completed);
    }

    Component::SafePointer<Canvas> parent, tab;
    Component::SafePointer<Object> object;
};
