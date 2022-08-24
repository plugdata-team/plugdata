#include "../Dialogs/Dialogs.h"

struct t_fake_textbuf {
    t_object b_ob;
    t_binbuf* b_binbuf;
    t_canvas* b_canvas;
    t_guiconnect* b_guiconnect;
    t_symbol* b_sym;
};

struct t_fake_text_define {
    t_fake_textbuf x_textbuf;
    t_outlet* x_out;
    t_outlet* x_notifyout;
    t_symbol* x_bindsym;
    t_scalar* x_scalar;   /* faux scalar (struct text-scalar) to point to */
    t_gpointer x_gp;      /* pointer to it */
    t_canvas* x_canvas;   /* owning canvas whose stub we use for x_gp */
    unsigned char x_keep; /* whether to embed contents in patch on save */
};

// void binbuf_gettext(const t_binbuf *x, char **bufp, int *lengthp);

// Actual text object, marked final for optimisation
struct TextDefineObject final : public TextBase {

    std::unique_ptr<Component> textEditor;

    TextDefineObject(void* obj, Box* parent, bool isValid = true)
        : TextBase(obj, parent, isValid)
        , textEditor(nullptr)
    {
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        openTextEditor();
    }

    void openTextEditor()
    {
        if (textEditor) {
            textEditor->toFront(true);
            return;
        }

        auto name = String::fromUTF8(static_cast<t_fake_text_define*>(ptr)->x_bindsym->s_name);

        textEditor.reset(
            Dialogs::showTextEditorDialog(getText(), name, [this](String lastText, bool hasChanged) {
                if (!hasChanged) {
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showSaveDialog(textEditor.get(), "", [this, lastText](int result) mutable {
                    if (result == 2) {
                        setText(lastText);
                        textEditor.reset(nullptr);
                    }
                    if (result == 1) {
                        textEditor.reset(nullptr);
                    }
                });
            }));
    }

    void setText(String text)
    {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;

        text = text.removeCharacters("\r");
        auto lines = StringArray::fromTokens(text, ";", "\"");
        auto atoms = std::vector<t_atom>();
        atoms.reserve(lines.size());

        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].isEmpty())
                continue;

            auto words = StringArray::fromTokens(lines[i], " ", "\"");
            for (int j = 0; j < words.size(); j++) {
                atoms.emplace_back();
                if (words[j].containsOnly("0123456789e.-+") && words[j] != "-") {
                    SETFLOAT(&atoms.back(), words[j].getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), gensym((words[j].toRawUTF8())));
                }
            }

            atoms.emplace_back();
            SETSYMBOL(&atoms.back(), gensym(";"));
        }

        pd->enqueueFunction([this, atoms, &textbuf]() mutable {
            binbuf_clear(textbuf.b_binbuf);

            t_binbuf* z = binbuf_new();
            binbuf_restore(z, atoms.size(), atoms.data());
            binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
        });
    }

    String getText()
    {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;
        auto* binbuf = textbuf.b_binbuf;

        char* bufp;
        int lenp;

        binbuf_gettext(binbuf, &bufp, &lenp);

        return String::fromUTF8(bufp, lenp);
    }
};
