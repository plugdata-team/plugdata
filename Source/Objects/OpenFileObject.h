/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Else "openfile" component
/*
struct OpenFileObject final : public TextBase {

    OpenFileObject(void* ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    ~OpenFileObject()
    {
    }

    void mouseUp(MouseEvent const& e) override
    {
        t_atom args[1];
        SETFLOAT(args, 1);

        pd_typedmess((t_pd*)ptr, pd->generateSymbol("_up"), 1, args);
    }
};
*/
