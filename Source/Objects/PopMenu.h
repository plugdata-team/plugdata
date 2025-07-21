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

    Value outline = SynchronousValue();
    Value savestate = SynchronousValue();
    Value loadbang = SynchronousValue();

    CachedTextRender textRenderer;

    NVGcolor fgCol;
    NVGcolor bgCol;

    StringArray items;
    String currentText;
    int currentItem = 0;

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
        //objectParameters.addParamBool("Outline", cGeneral, &outline, { "No", "Yes" });
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
        bgCol = convertColour(Colour::fromString(secondaryColour.toString()));
        fgCol = convertColour(Colour::fromString(primaryColour.toString()));
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

        menu.setLookAndFeel(&object->getLookAndFeel());
        menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [_this = SafePointer(this)](int item) {
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

    void setSendSymbol(String const& symbol) const
    {
        if (auto menu = ptr.get<void>()) {
            pd->sendDirectMessage(menu.get(), "send", { pd::Atom(pd->generateSymbol(symbol)) });
        }
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (auto menu = ptr.get<void>()) {
            pd->sendDirectMessage(menu.get(), "receive", { pd::Atom(pd->generateSymbol(symbol)) });
        }
    }

    void update() override
    {
        items.clear();
        if (auto menu = ptr.get<t_fake_menu>()) {
            for (int i = 0; i < menu->x_n_items; i++) // Loop for menu items
                items.add(String::fromUTF8(menu->x_items[i]->s_name));

            primaryColour = convertTclColour(String::fromUTF8(menu->x_fg->s_name)).toString();
            secondaryColour = convertTclColour(String::fromUTF8(menu->x_bg->s_name)).toString();
            sizeProperty = VarArray(menu->x_width, menu->x_height);
            savestate = menu->x_savestate;
            loadbang = menu->x_lb;
            outline = menu->x_outline;
            currentItem = menu->x_idx;
            if(menu->x_idx >= 0 && menu->x_idx < items.size()) {
                currentText = items[currentItem];
            }
            labelNoSelection = menu->x_label->s_name;
            
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

        return {};
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
        auto const text = currentItem >= 0 ? currentText : getValue<String>(labelNoSelection);
        auto const colour = Colour(fgCol.r, fgCol.g, fgCol.b, fgCol.a);
        auto const font = Fonts::getCurrentFont().withHeight(getHeight() * 0.7f);

        if (textRenderer.prepareLayout(text, font, colour, font.getStringWidth(text) + 12, getValue<int>(sizeProperty), false)) {
            repaint();
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), bgCol, object->isSelected() ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);
        
        auto textBounds = getLocalBounds().reduced(2).translated(2, 0);
        if(!textBounds.isEmpty()) {
            textRenderer.renderText(nvg, textBounds.toFloat(), getImageScale());
        }
        
        auto triangleBounds = b.removeFromRight(20).withSizeKeepingCentre(20, std::min(getHeight(), 12));

        nvgStrokeColor(nvg, fgCol);
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, triangleBounds.getCentreX() - 3, triangleBounds.getY() + 3);
        nvgLineTo(nvg, triangleBounds.getCentreX(), triangleBounds.getY());
        nvgLineTo(nvg, triangleBounds.getCentreX() + 3, triangleBounds.getY() + 3);
        nvgStroke(nvg);

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, triangleBounds.getCentreX() - 3, triangleBounds.getBottom() - 3);
        nvgLineTo(nvg, triangleBounds.getCentreX(), triangleBounds.getBottom());
        nvgLineTo(nvg, triangleBounds.getCentreX() + 3, triangleBounds.getBottom() - 3);
        nvgStroke(nvg);
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
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto const colour = "#" + primaryColour.toString().substring(2);
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_fg = pd->generateSymbol(colour);
            updateColours();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto const colour = "#" + secondaryColour.toString().substring(2);
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
        } else if (value.refersToSameSourceAs(outline)) {
            if (auto menu = ptr.get<t_fake_menu>())
                menu->x_outline = getValue<bool>(outline);
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
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("set"): {
            if (atoms.size() >= 1 && atoms[0].isFloat()) {
                currentItem = std::clamp(static_cast<int>(atoms[0].getFloat()), -1, items.size() - 1);
                if(currentItem >= 0) {
                    currentText = items[currentItem];
                }
                updateTextLayout();
            }
            break;
        }
        case hash("clear"):
        case hash("add"): {
            update();
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
                primaryColour = convertTclColour(atoms[0].toString()).toString();
            }
            break;
        }
        case hash("bg"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol()) {
                secondaryColour = convertTclColour(atoms[0].toString()).toString();
            }
            break;
        }
        case hash("label"): {
            if (atoms.size() >= 1 && atoms[0].isSymbol()) {
                labelNoSelection = atoms[0].toString();
            }
            break;
        }
        default:
            break;
        }
    }

    bool inletIsSymbol() override
    {
        auto const rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && rSymbol != "empty";
    }

    bool outletIsSymbol() override
    {
        auto const sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && sSymbol != "empty";
    }
};
