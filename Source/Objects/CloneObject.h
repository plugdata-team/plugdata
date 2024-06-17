/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
t_glist* clone_get_instance(t_gobj*, int);
int clone_get_n(t_gobj*);
}

class CloneObject final : public TextBase {

    pd::Patch::Ptr subpatch;

public:
    CloneObject(pd::WeakReference obj, Object* object)
        : TextBase(obj, object)
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            if (clone_get_n(gobj.get()) > 0) {
                auto* patch = clone_get_instance(gobj.get(), 0);
                subpatch = new pd::Patch(pd::WeakReference(patch, cnv->pd), cnv->pd, false);
            } else {
                subpatch = nullptr;
            }
        } else {
            subpatch = new pd::Patch(nullptr, nullptr, false);
        }
    }

    ~CloneObject()
    {
        closeOpenedSubpatchers();
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    String getText() override
    {
        if (auto clone = ptr.get<t_fake_clone>()) {
            auto* sym = clone->x_s;

            if (!sym || !sym->s_name)
                return "";

            return String::fromUTF8(sym->s_name);
        }

        return {};
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

    void openClonePatch(int idx, bool shouldVis)
    {
        pd::Patch::Ptr patch;
        if (auto gobj = ptr.get<t_gobj>()) {
            if (isPositiveAndBelow(idx, clone_get_n(gobj.get()))) {
                auto* patchPtr = clone_get_instance(gobj.get(), idx);
                patch = new pd::Patch(pd::WeakReference(patchPtr, cnv->pd), cnv->pd, false);
            }
        }

        if (!patch)
            return;

        auto* glist = patch->getPointer().get();

        if (!glist)
            return;

        // Check if patch is already opened
        for (auto* cnv : cnv->editor->getCanvases()) {
            if (cnv->patch == *patch) {
                // Close the patch on "vis 0"
                if (!shouldVis) {
                    cnv->editor->getTabComponent().closeTab(cnv);
                }
                // Show the current tab on "vis 1"
                else {
                    cnv->editor->getTabComponent().showTab(cnv);
                }

                return;
            }
        }

        auto abstraction = canvas_isabstraction(glist);
        File path;

        if (abstraction) {
            path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
        }

        auto* newCanvas = cnv->editor->getTabComponent().openPatch(patch);
        newCanvas->patch.setCurrentFile(URL(path));
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("vis"): {
            if (numAtoms >= 2) {
                openClonePatch(atoms[0].getFloat(), atoms[1].getFloat());
            }
            break;
        }
        default:
            break;
        }
    }
};
