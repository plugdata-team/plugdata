/*
 // Copyright (c) 2024 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class PopMenu final : public ObjectBase {

    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value parameterName = SynchronousValue();
    Value variableName = SynchronousValue();
    Value labelNoSelection = SynchronousValue();

    Value fontSize = SynchronousValue();
    Value savestate = SynchronousValue();
    Value loadbang = SynchronousValue();

    NVGcolor fgCol;
    NVGcolor bgCol;

    StringArray items;
    String currentText;
    int currentItem = -1;

public:
    PopMenu(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);
        objectParameters.addParamSendSymbol(&sendSymbol);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamString("Parameter", cGeneral, &parameterName);
        objectParameters.addParamString("Variable", cGeneral, &variableName);
        objectParameters.addParamString("No selection label", cGeneral, &labelNoSelection);
        objectParameters.addParamInt("Font size", cGeneral, &fontSize, 13, true, 0);
        objectParameters.addParamBool("Save state", cGeneral, &savestate, { "No", "Yes" });
        objectParameters.addParamBool("Loadbang", cGeneral, &loadbang, { "No", "Yes" });

        updateColours();
    }

    static Colour convertTclColour(String const& colourStr)
    {
        if (tclColours.count(colourStr)) {
            return tclColours[colourStr];
        }
        return Colour::fromString(colourStr.replace("#", "ff"));
    }

    void updateColours()
    {
        bgCol = nvgColour(getValue<Colour>(secondaryColour));
        fgCol = nvgColour(getValue<Colour>(primaryColour));
        repaint();
    }

    void showMenu()
    {
#if ENABLE_TESTING
        return;
#endif
        auto menu = PopupMenu();

        for (int i = 0; i < items.size(); i++) {
            menu.addItem(i + 1, items[i], true, i == currentItem);
        }
        if (items.size() == 0) {
            menu.addItem(1, "(No options)", false, false);
        }

        menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [_this = SafePointer(this)](int const item) {
            if (item && _this) {
                _this->currentItem = item - 1;
                _this->currentText = _this->items[item - 1];
                _this->sendFloatValue(item - 1);
                _this->updateTextLayout();
            }
        });
    }

    void mouseDown(MouseEvent const& e) override
    {
        showMenu();
    }

    String getSendSymbol() const
    {
        if (auto menu = ptr.get<t_fake_menu>()) {
            if (!menu->x_snd_raw || !menu->x_snd_raw->s_name)
                return "";

            auto sym = String::fromUTF8(menu->x_snd_raw->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    String getReceiveSymbol() const
    {
        if (auto menu = ptr.get<t_fake_menu>()) {
            if (!menu->x_rcv_raw || !menu->x_rcv_raw->s_name)
                return "";

            auto sym = String::fromUTF8(menu->x_rcv_raw->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    void update() override
    {
        items.clear();
        if (auto menu = ptr.get<t_fake_menu>()) {
            for (int i = 0; i < menu->x_n_items; i++) // Loop for menu items
                items.add(String::fromUTF8(menu->x_items[i]->s_name));

            primaryColour = colourToVar(convertTclColour(String::fromUTF8(menu->x_fg->s_name)));
            secondaryColour = colourToVar(convertTclColour(String::fromUTF8(menu->x_bg->s_name)));
            sizeProperty = VarArray(menu->x_width, menu->x_height);
            savestate = menu->x_savestate;
            loadbang = menu->x_lb;
            fontSize = menu->x_fontsize;
            currentItem = menu->x_idx;
            if (menu->x_idx >= 0 && menu->x_idx < items.size()) {
                currentText = items[currentItem];
            }
            labelNoSelection = (menu->x_label == gensym("empty") || !menu->x_label) ? String("") : String::fromUTF8(menu->x_label->s_name);

            sendSymbol = getSendSymbol();
            receiveSymbol = getReceiveSymbol();

            auto varName = menu->x_var_raw ? String::fromUTF8(menu->x_var_raw->s_name) : String("");
            if (varName == "empty")
                varName = "";
            variableName = varName;

            auto paramName = menu->x_param ? String::fromUTF8(menu->x_param->s_name) : String("");
            if (paramName == "empty")
                paramName = "";
            parameterName = paramName;
        }

        updateColours();
        updateTextLayout();
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getRawPointer();
            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);
            return Rectangle<int>(x, y, w + 1, h + 1);
        }

        return { };
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto menu = ptr.get<t_fake_menu>()) {
            auto* patch = cnv->patch.getRawPointer();
            pd::Interface::moveObject(patch, menu.cast<t_gobj>(), b.getX(), b.getY());
            menu->x_width = b.getWidth() - 1;
            menu->x_height = b.getHeight() - 1;
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto menu = ptr.get<t_fake_menu>()) {
            setParameterExcludingListener(sizeProperty, VarArray(var(menu->x_width), var(menu->x_height)));
        }
    }

    void resized() override
    {
        updateTextLayout();
    }

    void updateTextLayout()
    {
        // The label is drawn directly in render() from the current state, so just trigger a repaint.
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto const& colours = getThemeColours();

        auto b = getLocalBounds().toFloat();

        nanovg::nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), bgCol, nvgColour(object->isSelected() ? colours.objectSelectedOutlineColour : colours.objectOutlineColour), getPlugDataLook(*this).getObjectCornerRadius());

        auto textBounds = getLocalBounds().reduced(2).translated(2, 0);
        if (!textBounds.isEmpty()) {
            auto const text = currentItem >= 0 ? currentText : getValue<String>(labelNoSelection);
            auto const colour = Colour(fgCol.r, fgCol.g, fgCol.b, fgCol.a);
            auto const font = Fonts::getCurrentFont().withHeight(getValue<int>(fontSize) * 1.5f);

            auto& llgc = *cnv->editor->getNanoLLGC();
            Graphics g(llgc);
            NVGGraphicsContext::ScopedAnchoredDraw anchor(llgc, textBounds.toFloat());
            g.setFont(font);
            g.setColour(colour);
            g.drawText(text, textBounds.toFloat(), Justification::centredLeft);
        }

        auto const triangleBounds = b.removeFromRight(20).withSizeKeepingCentre(20, std::min(getHeight(), 12));

        nanovg::nvgStrokeColor(nvg, fgCol);
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgMoveTo(nvg, triangleBounds.getCentreX() - 3, triangleBounds.getY() + 3);
        nanovg::nvgLineTo(nvg, triangleBounds.getCentreX(), triangleBounds.getY());
        nanovg::nvgLineTo(nvg, triangleBounds.getCentreX() + 3, triangleBounds.getY() + 3);
        nanovg::nvgStroke(nvg);

        nanovg::nvgBeginPath(nvg);
        nanovg::nvgMoveTo(nvg, triangleBounds.getCentreX() - 3, triangleBounds.getBottom() - 3);
        nanovg::nvgLineTo(nvg, triangleBounds.getCentreX(), triangleBounds.getBottom());
        nanovg::nvgLineTo(nvg, triangleBounds.getCentreX() + 3, triangleBounds.getBottom() - 3);
        nanovg::nvgStroke(nvg);
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();

            auto const& arr = *sizeProperty.getValue().getArray();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            constrainer->setFixedAspectRatio(static_cast<float>(width) / height);

            setParameterExcludingListener(sizeProperty, VarArray(width, height));
            if (auto menu = ptr.get<t_fake_menu>()) {
                menu->x_width = width;
                menu->x_height = height;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            sendMessage("send", { pd->generateSymbol(sendSymbol.toString()) });
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            sendMessage("receive", { pd->generateSymbol(sendSymbol.toString()) });
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto const colour = "#" + getValue<Colour>(primaryColour).toString().substring(2);
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_fg = pd->generateSymbol(colour);
            updateColours();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto const colour = "#" + getValue<Colour>(secondaryColour).toString().substring(2);
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_bg = pd->generateSymbol(colour);
            updateColours();
        } else if (value.refersToSameSourceAs(parameterName)) {
            if (auto knb = ptr.get<t_fake_menu>())
                knb->x_param = pd->generateSymbol(parameterName.toString());
        } else if (value.refersToSameSourceAs(variableName)) {
            if (auto knb = ptr.get<t_fake_menu>()) {
                auto* s = pd->generateSymbol(variableName.toString());

                if (s == gensym(""))
                    s = gensym("empty");
                t_symbol* var = s == gensym("empty") ? gensym("") : canvas_realizedollar(knb->x_glist, s);
                if (var != knb->x_var) {
                    knb->x_var_set = 1;
                    knb->x_var_raw = s;
                    knb->x_var = var;
                }
            }
        } else if (value.refersToSameSourceAs(savestate)) {
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_savestate = getValue<bool>(savestate);
        } else if (value.refersToSameSourceAs(loadbang)) {
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_lb = getValue<bool>(loadbang);
        } else if (value.refersToSameSourceAs(labelNoSelection)) {
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_label = pd->generateSymbol(getValue<String>(labelNoSelection));
            updateTextLayout();
            repaint();
        } else if (value.refersToSameSourceAs(fontSize)) {
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_fontsize = getValue<int>(fontSize);
            updateTextLayout();
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("set"): {
            if (atoms.size() >= 1 && atoms[0].isFloat()) {
                currentItem = std::clamp(static_cast<int>(atoms[0].getFloat()), -1, items.size() - 1);
                if (currentItem >= 0) {
                    currentText = items[currentItem];
                }
                updateTextLayout();
            }
            break;
        }
        case hash("clear"): {
            items.clear();
            currentText = "";
            updateTextLayout();
            break;
        }
        case hash("add"): {
            items.clear();
            if (auto menu = ptr.get<t_fake_menu>()) {
                for (int i = 0; i < menu->x_n_items; i++) // Loop for menu items
                    items.add(String::fromUTF8(menu->x_items[i]->s_name));

                if (menu->x_idx >= 0 && menu->x_idx < items.size()) {
                    currentText = items[currentItem];
                }
            }
            updateTextLayout();
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol())
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol())
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("fg"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol()) {
                primaryColour = colourToVar(convertTclColour(atoms[0].toString()));
            }
            break;
        }
        case hash("bg"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol()) {
                secondaryColour = colourToVar(convertTclColour(atoms[0].toString()));
            }
            break;
        }
        case hash("label"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol()) {
                labelNoSelection = atoms[0].toString();
            }
            break;
        }
        case hash("fontsize"): {
            if (atoms.size() >= 1 && atoms[0].isFloat()) {
                fontSize = atoms[0].getFloat();
            }
            updateTextLayout();
            break;
        }
        default:
            break;
        }
    }

    bool hideInlet() override
    {
        auto const rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && rSymbol != "empty";
    }

    bool hideOutlet() override
    {
        auto const sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && sSymbol != "empty";
    }
};
