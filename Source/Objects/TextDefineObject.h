/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Dialogs/Dialogs.h"

class TextFileObject final : public TextBase {

    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

public:
    TextFileObject(pd::WeakReference obj, Object* parent)
        : TextBase(obj, parent, true)
        , textEditor(nullptr)
    {
    }

    void lock(bool const isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        openTextEditor();
    }

    void openTextEditor()
    {
        if (textEditor) {
            textEditor->toFront(true);
            return;
        }

        auto onClose = [this](String const& lastText, bool const hasChanged) {
            if (!hasChanged) {
                textEditor.reset(nullptr);
                return;
            }

            Dialogs::showAskToSaveDialog(
                &saveDialog, textEditor.get(), "", [this, lastText](int const result) mutable {
                    if (result == 2) {
                        setText(lastText);
                        textEditor.reset(nullptr);
                    }
                    if (result == 1) {
                        textEditor.reset(nullptr);
                    }
                },
                15, false);
        };

        auto onSave = [this](String const& lastText) {
            setText(lastText);
        };

        textEditor.reset(Dialogs::showTextEditorDialog(getText(), "qlist", onClose, onSave));
    }

    void setText(String text)
    {
        // remove repeating spaces
        while (text.contains("  ")) {
            text = text.replace("  ", " ");
        }
        text = text.replace("\r ", "\r");
        text = text.replace(";\r", ";");
        text = text.replace("\r;", ";");
        text = text.replace(" ;", ";");
        text = text.replace("; ", ";");
        text = text.replaceCharacters("\r", " ");
        text = text.trimStart();
        auto lines = StringArray::fromTokens(text, ";", "\"");
        auto atoms = SmallArray<t_atom>();
        atoms.reserve(lines.size());

        int count = 0;
        for (auto const& line : lines) {
            count++;
            auto words = StringArray::fromTokens(line, " ", "\"");
            for (auto const& word : words) {
                atoms.emplace_back();
                // check if string is a valid number
                auto charptr = word.getCharPointer();
                auto ptr = charptr;
                CharacterFunctions::readDoubleValue(ptr); // Remove double value from string
                if (ptr - charptr == word.getNumBytesAsUTF8() && ptr - charptr != 0) {
                    SETFLOAT(&atoms.back(), word.getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
                }
            }

            if (count != lines.size()) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
            }
        }

        if (auto qlist = ptr.get<t_fake_qlist>()) {

            auto const& textbuf = qlist->x_textbuf;

            binbuf_clear(textbuf.b_binbuf);

            t_binbuf* z = binbuf_new();
            binbuf_restore(z, atoms.size(), atoms.data());
            binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("click"): {
            MessageManager::callAsync([this] { openTextEditor(); });
        }
        case hash("close"): {
            textEditor.reset(nullptr);
        }
        default:
            break;
        }
    }

    String getText() override
    {
        if (auto textDefine = ptr.get<t_fake_text_define>()) {
            auto const& textbuf = textDefine->x_textbuf;
            auto const* binbuf = textbuf.b_binbuf;

            char* bufp;
            int lenp;

            binbuf_gettext(binbuf, &bufp, &lenp);

            return String::fromUTF8(bufp, lenp);
        }

        return {};
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open text editor", [_this = SafePointer(this)] { if(_this) _this->openTextEditor(); });
    }
};

class TextDefineObject final : public TextBase {

    std::unique_ptr<Component> textEditor;
    std::unique_ptr<Dialog> saveDialog;

public:
    TextDefineObject(pd::WeakReference obj, Object* parent)
        : TextBase(obj, parent, true)
        , textEditor(nullptr)
    {
    }

    void lock(bool const isLocked) override
    {
        setInterceptsMouseClicks(isLocked, false);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        openTextEditor();
    }

    void openTextEditor()
    {
        if (textEditor) {
            textEditor->toFront(true);
            return;
        }

        String name;
        if (auto textDefine = ptr.get<t_fake_text_define>()) {
            name = String::fromUTF8(textDefine->x_bindsym->s_name);
        } else {
            return;
        }

        auto onClose = [this](String const& lastText, bool const hasChanged) {
            if (!hasChanged) {
                textEditor.reset(nullptr);
                return;
            }

            Dialogs::showAskToSaveDialog(
                &saveDialog, textEditor.get(), "", [this, lastText](int const result) mutable {
                    if (result == 2) {
                        setText(lastText);
                        textEditor.reset(nullptr);

                        // enable notification on second outlet //
                        if (auto textDefine = ptr.get<t_fake_text_define>()) {
                            char const* target = textDefine->x_bindsym->s_name;
                            pd->sendMessage(target, "notify", {});
                        }
                    }
                    if (result == 1) {
                        textEditor.reset(nullptr);
                    }
                },
                15, false);
        };

        auto onSave = [this](String const& lastText) {
            setText(lastText);
            if (auto textDefine = ptr.get<t_fake_text_define>()) {
                char const* target = textDefine->x_bindsym->s_name;
                pd->sendMessage(target, "notify", {});
            }
        };

        textEditor.reset(Dialogs::showTextEditorDialog(getText(), name, onClose, onSave));
    }

    void setText(String text)
    {

        // remove repeating spaces
        while (text.contains("  ")) {
            text = text.replace("  ", " ");
        }
        text = text.replace("\r ", "\r");
        text = text.replace(";\r", ";");
        text = text.replace("\r;", ";");
        text = text.replace(" ;", ";");
        text = text.replace("; ", ";");
        text = text.replaceCharacters("\r", " ");
        text = text.trimStart();
        auto lines = StringArray::fromTokens(text, ";", "\"");
        auto atoms = SmallArray<t_atom>();
        atoms.reserve(lines.size());

        int count = 0;
        for (auto const& line : lines) {
            count++;
            auto words = StringArray::fromTokens(line, " ", "\"");
            for (auto const& word : words) {
                atoms.emplace_back();
                // check if string is a valid number
                auto charptr = word.getCharPointer();
                auto ptr = charptr;
                CharacterFunctions::readDoubleValue(ptr);
                if (ptr - charptr == word.getNumBytesAsUTF8() && ptr - charptr != 0) {
                    SETFLOAT(&atoms.back(), word.getFloatValue());
                } else {
                    SETSYMBOL(&atoms.back(), pd->generateSymbol(word));
                }
            }

            if (count != lines.size()) {
                atoms.emplace_back();
                SETSYMBOL(&atoms.back(), pd->generateSymbol(";"));
            }
        }

        if (auto textDefine = ptr.get<t_fake_text_define>()) {
            auto const& textbuf = textDefine->x_textbuf;

            binbuf_clear(textbuf.b_binbuf);

            t_binbuf* z = binbuf_new();
            binbuf_restore(z, atoms.size(), atoms.data());
            binbuf_add(textbuf.b_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("click"): {
            MessageManager::callAsync([_this = SafePointer(this)] { if(_this) _this->openTextEditor(); });
        }
        case hash("close"): {
            textEditor.reset(nullptr);
        }
        }
    }

    String getText() override
    {
        if (auto textDefine = ptr.get<t_fake_text_define>()) {
            auto const& textbuf = textDefine->x_textbuf;
            auto const* binbuf = textbuf.b_binbuf;

            char* bufp;
            int lenp;

            binbuf_gettext(binbuf, &bufp, &lenp);

            return String::fromUTF8(bufp, lenp);
        }

        return {};
    }

    void getMenuOptions(PopupMenu& menu) override
    {
        menu.addItem("Open text editor", [_this = SafePointer(this)] { if(_this) _this->openTextEditor(); });
    }
};
