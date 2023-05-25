/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Dialogs/Dialogs.h"

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
            Dialogs::showTextEditorDialog(getText(), "qlist", [this](String const& lastText, bool hasChanged) {
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

        for (auto const& line : lines) {
            if (line.isEmpty())
                continue;

            auto words = StringArray::fromTokens(line, " ", "\"");
            for (auto const& word : words) {
                atoms.emplace_back();
                if (word.containsOnly("0123456789e.-+") && word != "-") {
                    SETFLOAT(&atoms.back(), word.getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
                }
            }

            atoms.emplace_back();
            SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
        }

        pd->lockAudioThread();

        pd->setThis();

        binbuf_clear(textbuf.b_binbuf);

        t_binbuf* z = binbuf_new();
        binbuf_restore(z, atoms.size(), atoms.data());
        binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
        binbuf_free(z);

        pd->unlockAudioThread();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("click"),
            hash("close")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
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
            Dialogs::showTextEditorDialog(getText(), name, [this](String const& lastText, bool hasChanged) {
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

        for (auto const& line : lines) {
            if (line.isEmpty())
                continue;

            auto words = StringArray::fromTokens(line, " ", "\"");
            for (auto const& word : words) {
                atoms.emplace_back();
                if (word.containsOnly("0123456789e.-+") && word != "-") {
                    SETFLOAT(&atoms.back(), word.getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
                }
            }

            atoms.emplace_back();
            SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
        }

        pd->setThis();

        pd->lockAudioThread();

        binbuf_clear(textbuf.b_binbuf);

        t_binbuf* z = binbuf_new();
        binbuf_restore(z, atoms.size(), atoms.data());
        binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
        binbuf_free(z);

        pd->unlockAudioThread();
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
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
