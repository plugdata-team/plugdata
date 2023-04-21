/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Dialogs/Dialogs.h"

struct t_fake_textbuf {
    t_object b_ob;
    t_binbuf* b_binbuf;
    t_canvas* b_canvas;
    t_guiconnect* b_guiconnect;
    t_symbol* b_sym;
};

struct t_fake_qlist {
    t_fake_textbuf x_textbuf;
    t_outlet* x_bangout;
    int x_onset; /* playback position */
    t_clock* x_clock;
    t_float x_tempo;
    double x_whenclockset;
    t_float x_clockdelay;
    int x_rewound; /* we've been rewound since last start */
    int x_innext;  /* we're currently inside the "next" routine */
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

class TextFileObject final : public TextBase {

    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

public:
    TextFileObject(void* obj, Object* parent, bool isValid = true)
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

        textEditor.reset(
            Dialogs::showTextEditorDialog(getText(), "qlist", [this](String lastText, bool hasChanged) {
                if (!hasChanged) {
                    textEditor.reset(nullptr);
                    return;
                }

                Dialogs::showSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, lastText](int result) mutable {
                        if (result == 2) {
                            setText(lastText);
                            textEditor.reset(nullptr);
                        }
                        if (result == 1) {
                            textEditor.reset(nullptr);
                        }
                    },
                    15);
            }));
    }

    void setText(String text)
    {
        auto& textbuf = static_cast<t_fake_qlist*>(ptr)->x_textbuf;

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
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(words[j]));
                }
            }

            atoms.emplace_back();
            SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
        }

        pd->enqueueFunction([_this = SafePointer(this), atoms, &textbuf]() mutable {
            if (!_this || _this->cnv->patch.objectWasDeleted(_this->ptr))
                return;
            _this->pd->setThis();

            binbuf_clear(textbuf.b_binbuf);

            t_binbuf* z = binbuf_new();
            binbuf_restore(z, atoms.size(), atoms.data());
            binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
        });
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("click"),
            hash("close")
        };
    }

    void receiveObjectMessage(hash32 const& symbolHash, std::vector<pd::Atom>& atoms) override
    {
        switch (symbolHash) {
        case hash("click"): {
            MessageManager::callAsync([this]() { openTextEditor(); });
        }
        case hash("close"): {
            textEditor.reset(nullptr);
        }
        }
    }

    String getText() override
    {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;
        auto* binbuf = textbuf.b_binbuf;

        char* bufp;
        int lenp;

        binbuf_gettext(binbuf, &bufp, &lenp);

        return String::fromUTF8(bufp, lenp);
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openTextEditor();
    }
};

// Actual text object, marked final for optimisation
class TextDefineObject final : public TextBase {

    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

public:
    TextDefineObject(void* obj, Object* parent, bool isValid = true)
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

                Dialogs::showSaveDialog(
                    &saveDialog, textEditor.get(), "", [this, lastText](int result) mutable {
                        if (result == 2) {
                            setText(lastText);
                            textEditor.reset(nullptr);

                            // enable notification on second outlet //
                            const char* target = static_cast<t_fake_text_define*>(ptr)->x_bindsym->s_name;
                            pd->sendMessage(target, "notify", {});
                        }
                        if (result == 1) {
                            textEditor.reset(nullptr);
                        }
                    },
                    15);
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
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(words[j]));
                }
            }

            atoms.emplace_back();
            SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
        }

        pd->enqueueFunction([_this = SafePointer(this), atoms, &textbuf]() mutable {
            if (!_this || _this->cnv->patch.objectWasDeleted(_this->ptr))
                return;
            _this->pd->setThis();

            binbuf_clear(textbuf.b_binbuf);

            t_binbuf* z = binbuf_new();
            binbuf_restore(z, atoms.size(), atoms.data());
            binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
        });
    }

    void receiveObjectMessage(hash32 const& symbolHash, std::vector<pd::Atom>& atoms) override
    {
        switch (symbolHash) {
        case hash("click"): {
            MessageManager::callAsync([this]() { openTextEditor(); });
        }
        case hash("close"): {
            textEditor.reset(nullptr);
        }
        }
    }

    String getText() override
    {
        auto& textbuf = static_cast<t_fake_text_define*>(ptr)->x_textbuf;
        auto* binbuf = textbuf.b_binbuf;

        char* bufp;
        int lenp;

        binbuf_gettext(binbuf, &bufp, &lenp);

        return String::fromUTF8(bufp, lenp);
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openTextEditor();
    }
};
