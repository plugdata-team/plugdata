#include "Dialogs/AddObjectMenu.h"

// Drops every entry of the add-object menu onto a canvas and checks it actually
// became an object.
//
// The menu stores each entry as a patch string with creation arguments baked in
// (`metro 1 120 permin`, `svfilter~ 1729 0.42`), and those strings are only ever
// exercised when a user drags that particular tile out. A typo in one, an object
// that no longer ships, or an argument list a newer ELSE rejects would otherwise
// go unnoticed until someone reached for it.
//
// Each entry goes through Canvas::dragAndDropPaste - the same call the drag
// handler makes - so the patch string is parsed exactly as it would be in use.
// Pd turns an object it cannot create into a bare text box, which plugdata
// reports as type "invalid", and that is what this looks for.

class AddObjectMenuTest : public PlugDataUnitTest {
public:
    AddObjectMenuTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Add Object Menu Test")
    {
    }

private:
    void checkList(Canvas* cnv, HeapArray<std::pair<String, HeapArray<std::tuple<String, String, String, String, ObjectIDs>>>> const& list, String const& listName)
    {
        for (auto const& [categoryName, category] : list) {
            for (auto const& [icon, patch, tooltip, name, objectID] : category) {
                beginTest(listName + " / " + categoryName + " / " + name);

                // Resolve exactly like the menu does: GUI objects expand to their themed
                // format, and arrays get a free name picked at drop time
                auto const patchString = resolveObjectPatch(editor, ObjectList::getObjectPatch(patch, name));

                auto const before = static_cast<int>(cnv->objects.size());
                cnv->dragAndDropPaste(patchString, { 200, 200 }, 100, 100);

                auto const after = static_cast<int>(cnv->objects.size());
                expect(after > before, name + " created nothing: " + patchString);

                for (int i = before; i < after; i++) {
                    auto* object = cnv->objects[i];
                    if (!object || !object->gui) {
                        expect(false, name + " has no gui: " + patchString);
                        continue;
                    }

                    if(objectID == NewObject) {
                        expect(object->gui->getType().toString() == "invalid",
                            name + " failed to instantiate: " + patchString);
                    }
                    else {
                        expect(object->gui->getType().toString() != "invalid",
                            name + " failed to instantiate: " + patchString);
                    }

                }

                // Keep the canvas from growing without bound as we work through the list
                if (cnv->objects.size() > 64) {
                    cnv->deselectAll();
                    for (auto* object : cnv->objects)
                        cnv->setSelected(object, true, false, false);
                    cnv->removeSelection();
                    cnv->performSynchronise();
                }
            }
        }
    }

    void perform() override
    {
        auto* cnv = editor->getTabComponent().newPatch();
        cnv->locked = false;

        checkList(cnv, ObjectList::defaultObjectList, "default");
        checkList(cnv, ObjectList::heavyObjectList, "hvcc");

        signalDone(true);
    }
};
