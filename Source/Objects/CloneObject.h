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
    CloneObject(void* obj, Object* object)
        : TextBase(obj, object)
    {
        if(auto gobj = ptr.get<t_gobj>())
        {
            if (clone_get_n(gobj.get()) > 0) {
                subpatch = new pd::Patch(clone_get_instance(gobj.get(), 0), cnv->pd, false);
            } else {
                subpatch = new pd::Patch(nullptr, nullptr, false);
            }
        }
        else {
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
        if(auto clone = ptr.get<t_fake_clone>())
        {
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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("vis")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("vis"): {
            if (atoms.size() > 2) {
                // TODO: implement this!
            }
            break;
        }
        default:
            break;
        }
    }
};
