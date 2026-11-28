// Builds a random tree of subpatches, graphs and abstractions, then opens, closes, saves, reloads and
// Builds a random tree of subpatches, graphs and abstractions, then opens, closes, saves, reloads and
// deletes parts of it. After every step, each tab must show a live patch that Pd considers open, and
// each canvas in the tree that Pd considers open must have a tab.

#include "Canvas.h"
#include "Object.h"

class NestedPatchTest : public PlugDataUnitTest {
public:
    NestedPatchTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Nested Patch Test")
    {
    }

private:
    struct Node {
        int parent, index; // index: position in the parent's object list
        String path;
        bool isAbstraction;
    };

    struct Step {
        String name;
        std::function<void()> action;
    };

    static constexpr int maxDepth = 4;
    static constexpr int numRandomSteps = 40;

    static constexpr auto innerText = "#N canvas 0 50 450 300 12;\n#X obj 10 10 f;\n#X obj 10 40 print inner;\n"
                                      "#N canvas 0 50 450 300 deep 0;\n#X obj 10 10 f;\n#X obj 10 40 print deep;\n#X connect 0 0 1 0;\n#X restore 30 100 pd deep;\n"
                                      "#X connect 0 0 1 0;\n";
    static constexpr auto outerText = "#N canvas 0 50 450 300 12;\n#X obj 10 10 f;\n#X obj 10 40 print outer;\n"
                                      "#N canvas 0 50 450 300 holder 0;\n#X obj 10 10 f;\n#X obj 10 40 print holder;\n#X obj 30 100 nest_inner;\n#X connect 0 0 1 0;\n#X restore 30 100 pd holder;\n"
                                      "#X connect 0 0 1 0;\n";

    void perform() override
    {
        beginTest("Open the generated patch");
        for (auto* cnv : editor->getCanvases())
            existing.add(cnv);
        for (auto const& patch : editor->pd->patches)
            existingPatches.add(patch.get());

        dir = File::getSpecialLocation(File::tempDirectory).getChildFile("nested_patch_test");
        dir.deleteRecursively();
        dir.createDirectory();
        dir.getChildFile("nest_inner.pd").replaceWithText(innerText);
        dir.getChildFile("nest_outer.pd").replaceWithText(outerText);

        nodes.add({ -1, 0, "root", false });
        rootFile = dir.getChildFile("nest_root.pd");
        rootFile.replaceWithText("#N canvas 0 50 800 600 12;\n" + generateCanvas(0, 0));

        root = editor->pd->loadPatch(URL(rootFile));
        rootTab = root ? editor->getTabComponent().openPatch(root) : nullptr;
        if (!rootTab || !glistOf(static_cast<int>(nodes.size()) - 1)) {
            expect(false, "the generated patch must load completely");
            finish();
            return;
        }

        buildSteps();
        runStep(0);
    }

    // The root always gets a subpatch and an abstraction, deeper canvases get one or two random children
    String generateCanvas(int const self, int const depth)
    {
        String text = "#X obj 10 10 f;\n#X obj 10 40 print " + nodes[self].path.fromLastOccurrenceOf("/", false, false) + ";\n";
        int const numChildren = depth == 0 ? 2 : depth < maxDepth ? 1 + rng.nextInt(2) : 0;
        for (int i = 0; i < numChildren; i++) {
            auto const position = String(30 + i * 160) + " 100";
            auto const kind = depth == 0 ? i * 2 : rng.nextInt(3); // 0: subpatch, 1: graph, 2: abstraction
            if (kind == 2) {
                auto const outer = addNode(self, 2 + i, "outer", true);
                auto const holder = addNode(outer, 2, "holder", false);
                auto const inner = addNode(holder, 2, "inner", true);
                addNode(inner, 2, "deep", false);
                text += "#X obj " + position + " nest_outer;\n";
                continue;
            }
            auto const name = "n" + String(nodes.size());
            auto const child = addNode(self, 2 + i, name, false);
            text += "#N canvas 0 50 450 300 " + name + " 0;\n" + generateCanvas(child, depth + 1);
            if (kind == 1)
                text += "#X coords 0 -1 1 1 140 90 1 0 0;\n";
            text += "#X restore " + position + " pd " + name + ";\n";
        }
        return text + "#X connect 0 0 1 0;\n";
    }

    int addNode(int const parent, int const index, String const& name, bool const isAbstraction)
    {
        nodes.add({ parent, index, nodes[parent].path + "/" + name, isAbstraction });
        return static_cast<int>(nodes.size()) - 1;
    }

    void buildSteps()
    {
        step("", [this] { beginTest("Open and close tabs at random"); });
        for (int i = 0; i < numRandomSteps; i++)
            step("random action", [this] { randomAction(); });

        int const sub = deepestNodeOutsideAbstractions();
        int const outer = firstNodeNamed("outer", 0);
        int const deep = firstNodeNamed("deep", outer);
        int const holder = firstNodeNamed("holder", outer);
        int const inner = firstNodeNamed("inner", outer);

        step("", [this] {
            beginTest("Saving from a subpatch writes the root file");
            expect(!pdDirty(glistOf(0)), "opening and closing tabs must not dirty the patch");
        });
        addEdit(sub, false, "marker_sub", [this, sub](Canvas* cnv) {
            expect(pdDirty(glistOf(0)) && rootTab && rootTab->patch.isDirty(), "an edit in a subpatch must dirty the root patch");
            expectSaves(cnv, rootFile, "marker_sub", nodes[sub].path);
        });
        step("saving " + nodes[sub].path, [this] {
            expect(!pdDirty(glistOf(0)) && rootTab && !rootTab->patch.isDirty(), "saving must clean the root patch");
        });

        // Undo pastes the subpatch back, which must keep it dirty if it had unsaved changes
        step("", [this] { beginTest("Undoing a subpatch's property keeps unsaved changes inside it"); });
        addEdit(sub, false, "marker_unsaved", [this, sub](Canvas*) {
            auto* parent = tabOf(glistOf(nodes[sub].parent));
            Object* obj = nullptr;
            for (auto* o : parent->objects) {
                if (o->getPointer() == reinterpret_cast<t_gobj*>(glistOf(sub)))
                    obj = o;
            }
            if (!obj || !obj->gui) {
                expect(false, nodes[sub].path + " must have an object in its parent's tab");
                return;
            }
            for (auto& param : obj->gui->getParameters().getParameters()) {
                if (param.name != "Width (chars)" && param.name != "Size")
                    continue;
                auto const v = param.valuePtr->getValue();
                param.valuePtr->setValue(v.isArray() ? var(VarArray { int(v[0]) + 10, v[1] }) : var(int(v) + 10));
                parent->undo();
                expect(pdDirty(glistOf(0)), "undoing a property of " + nodes[sub].path + " must keep the edit inside it unsaved");
                expectSaves(rootTab, rootFile, "marker_unsaved", "the root");
                break;
            }
        });

        step("", [this] {
            beginTest("Saving from inside an abstraction writes the abstraction");
            closeTabsExceptRoot();
        });
        addEdit(deep, false, "marker_abs", [this, deep](Canvas* cnv) {
            auto* inner = tabOf(glistOf(nodes[deep].parent));
            expect(pdDirty(glistOf(deep)), "an edit inside an abstraction must dirty the abstraction");
            expect(inner && inner->patch.isDirty(), "the abstraction's tab must show that it is dirty");
            expect(!pdDirty(glistOf(0)) && rootTab && !rootTab->patch.isDirty(), "an edit inside an abstraction must not dirty the patch that uses it");
            originalRoot = rootFile.loadFileAsString();
            expectSaves(cnv, dir.getChildFile("nest_inner.pd"), "marker_abs", nodes[deep].path);
        });
        step("reloading nest_inner", [this] {
            expect(rootFile.loadFileAsString() == originalRoot, "saving an abstraction must not write the patch that uses it");
            expectInstancesContain("deep", 3, "the reload must give every instance the edit");
        });

        step("", [this] {
            beginTest("Saving from a canvas opened from search writes its abstraction");
            closeTabsExceptRoot();
        });
        addEdit(holder, true, "marker_search", [this, holder](Canvas* cnv) {
            expectSaves(cnv, dir.getChildFile("nest_outer.pd"), "marker_search", nodes[holder].path);
        });
        step("", [this] { closeTabsExceptRoot(); });
        addEdit(inner, true, "marker_search_inner", [this, inner](Canvas* cnv) {
            expectSaves(cnv, dir.getChildFile("nest_inner.pd"), "marker_search_inner", nodes[inner].path);
        });

        step("opening every nest_inner instance", [this] {
            beginTest("Reloading an abstraction with tabs open inside its instances");
            absRoot = editor->pd->loadPatch(URL(dir.getChildFile("nest_inner.pd")));
            absTab = absRoot ? editor->getTabComponent().openPatch(absRoot) : nullptr;
            for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
                if (nodes[i].path.endsWith("/deep"))
                    rng.nextBool() ? openViaParents(i) : openDirectly(i);
            }
        });
        step("editing nest_inner", [this] {
            if (absTab) {
                absTab->patch.createObject(10, 250, "print marker_root");
                absTab->performSynchronise();
                expectSaves(absTab, dir.getChildFile("nest_inner.pd"), "marker_root", "nest_inner.pd");
            }
        });
        step("reloading nest_inner", [this] {
            for (auto& cnv : ourTabs()) {
                if (cnv) {
                    editor->getTabComponent().showTab(cnv, 0);
                    cnv->performSynchronise();
                }
            }
        });

        step("deleting a subpatch", [this] {
            beginTest("Deleting a subpatch closes the tabs inside it");
            deleteRandomSubtree();
        });
        step("undoing the deletion", [this] {
            if (deletedParent)
                deletedParent->undo();
        });

        int const child = firstChildOf(1); // node 1 is always a subpatch with children
        step("opening " + nodes[child].path + " from its parent", [this, child] {
            beginTest("Closing a tab while a subpatch inside it is showing");
            closeTabsExceptRoot();
            editor->getTabComponent().showTab(rootTab, 0);
            if (auto* parent = openViaParents(nodes[child].parent)) {
                editor->getTabComponent().showTab(parent, 0);
                openViaParents(child);
            }
        });
        step("closing " + nodes[nodes[child].parent].path + " while " + nodes[child].path + " is showing", [this, child] {
            auto& tabs = editor->getTabComponent();
            expect(tabOf(glistOf(child)) == tabs.getCurrentCanvas(), "the subpatch must be showing");
            if (auto* parent = tabOf(glistOf(nodes[child].parent)))
                tabs.closeTab(parent);
            expect(editor->getCanvases().contains(tabs.getCurrentCanvas()), "closing the tab must show another open tab");
        });

        step("closing the root", [this] {
            beginTest("Closing the root patch closes every tab inside it");
            for (int i = 0; i < 4; i++)
                rng.nextBool() ? openViaParents(rng.nextInt(static_cast<int>(nodes.size()))) : openDirectly(rng.nextInt(static_cast<int>(nodes.size())));
            if (absTab)
                editor->getTabComponent().closeTab(absTab);
            editor->getTabComponent().closeTab(rootTab);
            root = absRoot = nullptr;
        });
        step("closing the root", [this] {
            for (auto* cnv : editor->getCanvases())
                expect(existing.contains(cnv), "a tab outlived its root patch: " + cnv->patch.getTitle());
        });
    }

    void step(String const& name, std::function<void()> action)
    {
        steps.add({ name, std::move(action) });
    }

    // Adds an object to a canvas in one step, and checks the tab in the next
    void addEdit(int const node, bool const fromSearch, String const& marker, std::function<void(Canvas*)> check)
    {
        step("opening " + nodes[node].path + (fromSearch ? " from search" : ""), [this, node, fromSearch, marker] {
            auto* cnv = fromSearch ? openDirectly(node) : openViaParents(node);
            expect(cnv != nullptr, nodes[node].path + " must open");
            if (cnv) {
                cnv->patch.createObject(10, 200, "print " + marker);
                cnv->performSynchronise();
            }
        });
        step("editing " + nodes[node].path, [this, node, check] {
            if (auto* cnv = tabOf(glistOf(node)))
                check(cnv);
        });
    }

    void runStep(int const index)
    {
        if (index > 0)
            checkTabs(steps[index - 1].name);
        if (index >= steps.size()) {
            finish();
            return;
        }
        current = index;
        steps[index].action();
        Timer::callAfterDelay(80, [this, index] { runStep(index + 1); });
    }

    // Describes the current step for the checks that follow it
    void describeStep(String const& name)
    {
        steps[current].name = name;
    }

    void randomAction()
    {
        auto& tabs = editor->getTabComponent();
        auto const node = rng.nextInt(static_cast<int>(nodes.size()));
        auto const canvases = editor->getCanvases();
        auto* cnv = canvases[rng.nextInt(static_cast<int>(canvases.size()))];
        switch (rng.nextInt(8)) {
        case 0:
            describeStep("opening " + nodes[node].path + " via its parents");
            openViaParents(node);
            break;
        case 1:
            describeStep("opening " + nodes[node].path + " from search");
            openDirectly(node);
            break;
        case 5:
            describeStep("showing " + describe(cnv));
            tabs.showTab(cnv, rng.nextInt(2));
            break;
        case 6:
            describeStep("closing and reopening " + nodes[node].path);
            if (auto* open = tabOf(glistOf(node)); open && open != rootTab)
                tabs.closeTab(open);
            openDirectly(node);
            break;
        default:
            if (cnv != rootTab && !existing.contains(cnv)) {
                describeStep("closing " + describe(cnv));
                tabs.closeTab(cnv);
            }
            break;
        }
    }

    // Deletes a canvas that has a tab open inside it, from its parent's tab
    void deleteRandomSubtree()
    {
        SmallArray<int> candidates;
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            if (depthOf(i) >= 2)
                candidates.add(i);
        }
        if (candidates.empty())
            return;

        auto const leaf = candidates[rng.nextInt(candidates.size())];
        auto target = leaf;
        while (depthOf(target) > 1 && rng.nextBool())
            target = nodes[target].parent;
        if (target == leaf)
            target = nodes[leaf].parent;

        rng.nextBool() ? openViaParents(leaf) : openDirectly(leaf);
        deletedParent = openViaParents(nodes[target].parent);
        if (!deletedParent)
            return;

        describeStep("deleting " + nodes[target].path + " with " + nodes[leaf].path + " open");
        deletedParent->performSynchronise();
        deletedParent->deselectAll();
        for (auto* obj : deletedParent->objects) {
            if (obj->getPointer() == reinterpret_cast<t_gobj*>(glistOf(target)))
                deletedParent->setSelected(obj, true);
        }
        deletedParent->removeSelection();
    }

    int depthOf(int node) const
    {
        int depth = 0;
        while (nodes[node].parent >= 0) {
            node = nodes[node].parent;
            depth++;
        }
        return depth;
    }

    int deepestNodeOutsideAbstractions() const
    {
        int deepest = 0;
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            if (!nodes[i].path.contains("outer") && depthOf(i) > depthOf(deepest))
                deepest = i;
        }
        return deepest;
    }

    int firstChildOf(int const parent) const
    {
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            if (nodes[i].parent == parent)
                return i;
        }
        return 0;
    }

    int firstNodeNamed(String const& name, int const after) const
    {
        for (int i = after; i < static_cast<int>(nodes.size()); i++) {
            if (nodes[i].path.endsWith("/" + name))
                return i;
        }
        return 0;
    }

    t_canvas* glistOf(int const node) const
    {
        if (node == 0)
            return root ? root->getRawPointer() : nullptr;
        auto const* parent = glistOf(nodes[node].parent);
        if (!parent)
            return nullptr;
        int i = 0;
        for (auto* y = parent->gl_list; y; y = y->g_next, i++) {
            if (i == nodes[node].index)
                return pd_class(&y->g_pd) == canvas_class ? reinterpret_cast<t_canvas*>(y) : nullptr;
        }
        return nullptr;
    }

    Canvas* tabOf(t_canvas const* glist)
    {
        for (auto* cnv : editor->getCanvases()) {
            if (glist && cnv->patch.getRawPointer() == glist)
                return cnv;
        }
        return nullptr;
    }

    // Like double-clicking the subpatch in each parent's tab
    Canvas* openViaParents(int const node)
    {
        auto* glist = glistOf(node);
        if (auto* cnv = tabOf(glist); cnv || node == 0 || !glist)
            return cnv;

        auto* parent = openViaParents(nodes[node].parent);
        if (!parent)
            return nullptr;
        parent->performSynchronise();
        for (auto* obj : parent->objects) {
            if (obj->getPointer() == reinterpret_cast<t_gobj*>(glist) && obj->gui) {
                obj->gui->openSubpatch();
                break;
            }
        }
        return tabOf(glist);
    }

    // Like clicking a search result inside the canvas: opens it without its parents
    Canvas* openDirectly(int const node)
    {
        auto* glist = glistOf(node);
        if (!glist || !glist->gl_list)
            return nullptr;
        editor->highlightSearchTarget(glist->gl_list, true);
        return tabOf(glist);
    }

    // Closing a tab can close the tabs inside it too
    SmallArray<Component::SafePointer<Canvas>> ourTabs()
    {
        SmallArray<Component::SafePointer<Canvas>> tabs;
        for (auto* cnv : editor->getCanvases()) {
            if (!existing.contains(cnv))
                tabs.add(cnv);
        }
        return tabs;
    }

    void closeTabsExceptRoot()
    {
        for (auto& cnv : ourTabs()) {
            if (cnv && cnv != rootTab)
                editor->getTabComponent().closeTab(cnv);
        }
    }

    String describe(Canvas* cnv) const
    {
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            if (glistOf(i) && cnv->patch.getRawPointer() == glistOf(i))
                return nodes[i].path;
        }
        return cnv->patch.getTitle();
    }

    static bool pdDirty(t_canvas* glist)
    {
        return glist && canvas_getrootfor(glist)->gl_dirty;
    }

    void expectSaves(Canvas* cnv, File const& file, String const& marker, String const& from)
    {
        bool saved = false;
        cnv->save([&saved] { saved = true; });
        expect(saved, "saving from " + from + " must not ask for a location");
        expect(file.loadFileAsString().contains(marker), "saving from " + from + " must write " + file.getFileName());
    }

    // Every instance of nest_inner must have this many objects in the named canvas
    void expectInstancesContain(String const& name, int const numObjects, String const& message)
    {
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            auto const* glist = glistOf(i);
            if (!glist || !nodes[i].path.endsWith("/" + name))
                continue;
            int count = 0;
            for (auto const* y = glist->gl_list; y; y = y->g_next)
                count++;
            expectEquals(count, numObjects, message + " (" + nodes[i].path + ")");
        }
    }

    void checkTabs(String const& when)
    {
        if (when.isEmpty())
            return;

        auto* pd = editor->pd;
        pd->lockAudioThread();
        SmallArray<t_canvas*> open;
        for (auto* cnv : editor->getCanvases()) {
            if (existing.contains(cnv))
                continue;
            auto* glist = cnv->patch.getRawPointer();
            expect(glist != nullptr, "after " + when + ": a tab shows a deleted patch (" + cnv->patch.getTitle() + ")");
            if (!glist)
                continue;
            expect(!open.contains(glist), "after " + when + ": two tabs show " + describe(cnv));
            expect(glist->gl_havewindow, "after " + when + ": Pd thinks " + describe(cnv) + " is closed, but it has a tab");
            open.add(glist);
        }
        for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
            auto* glist = glistOf(i);
            if (glist && glist->gl_havewindow && !open.contains(glist))
                expect(false, "after " + when + ": Pd thinks " + nodes[i].path + " is open, but it has no tab");
        }
        for (auto const& patch : pd->patches) {
            if (!existingPatches.contains(patch.get()) && !patch->getRawPointer())
                expect(false, "after " + when + ": a deleted patch is still in the list of open patches (" + patch->getTitle() + ")");
        }
        pd->unlockAudioThread();
    }

    void finish()
    {
        for (auto& cnv : ourTabs()) {
            if (cnv)
                editor->getTabComponent().closeTab(cnv);
        }
        root = absRoot = nullptr;
        MessageManager::callAsync([this] {
            dir.deleteRecursively();
            signalDone(true);
        });
    }

    HeapArray<Node> nodes;
    HeapArray<Step> steps;
    SmallArray<Canvas*> existing;
    SmallArray<pd::Patch*> existingPatches;
    File dir, rootFile;
    String originalRoot;
    pd::Patch::Ptr root, absRoot;
    Component::SafePointer<Canvas> rootTab, absTab, deletedParent;
    int current = 0;
};
