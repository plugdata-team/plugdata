/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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

    ~CloneObject() override
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
            auto const* sym = clone->x_s;

            if (!sym || !sym->s_name)
                return "";

            return String::fromUTF8(sym->s_name);
        }

        return {};
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        //  If locked and it's a left click
        if (isLocked && !e.mods.isRightButtonDown()) {
            openSubpatch();
        } else {
            TextBase::mouseDown(e);
        }
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open", [_this = SafePointer(this)] { if(_this) _this->openSubpatch(); });
    }

    void openClonePatch(int const idx, bool const shouldVis)
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

        auto const* glist = patch->getPointer().get();

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

        auto const abstraction = canvas_isabstraction(glist);
        File path;

        if (abstraction) {
            path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
        }

        auto const* newCanvas = cnv->editor->getTabComponent().openPatch(patch);
        newCanvas->patch.setCurrentFile(URL(path));
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("vis"): {
            if (atoms.size() >= 2) {
                openClonePatch(atoms[0].getFloat(), atoms[1].getFloat());
            }
            break;
        }
        default:
            break;
        }
    }
};
