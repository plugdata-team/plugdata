/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class PictureObject final : public ObjectBase, public AsyncUpdater {

    Value filename = SynchronousValue();
    Value latch = SynchronousValue();
    Value outline = SynchronousValue();
    Value reportSize = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    File imageFile;
    Image img;
    NVGImage imageBuffer;
    bool imageNeedsReload = false;
    int offsetX = 0, offsetY = 0;

public:
    PictureObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamString("File", cGeneral, &filename, "");
        objectParameters.addParamBool("Latch", cGeneral, &latch, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Outline", cAppearance, &outline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Report Size", cAppearance, &reportSize, { "No", "Yes" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
    }

    ~PictureObject() override
    {
    }

    void handleAsyncUpdate() override
    {
        openFile(filename.toString());
    }

    bool isTransparent() override
    {
        return true;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        if (getValue<bool>(latch)) {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_float(pic->x_outlet, 1.0f);
                if (pic->x_send != gensym("") && pic->x_send->s_thing)
                    pd_float(pic->x_send->s_thing, 1.0f);
            }
        } else {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_bang(pic->x_outlet);
                if (pic->x_send != gensym("") && pic->x_send->s_thing)
                    pd_bang(pic->x_send->s_thing);
            }
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(latch)) {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_float(pic->x_outlet, 0.0f);
                if (pic->x_send != gensym("") && pic->x_send->s_thing)
                    pd_float(pic->x_send->s_thing, 0.0f);
            }
        }
    }

    void update() override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {

            if (pic->x_filename) {
                filename = String::fromUTF8(pic->x_filename->s_name);
                triggerAsyncUpdate();
            }

            latch = pic->x_latch;
            outline = pic->x_outline;
            reportSize = pic->x_size;

            auto sndSym = pic->x_snd_set ? String::fromUTF8(pic->x_snd_raw->s_name) : getBinbufSymbol(3);
            auto rcvSym = pic->x_rcv_set ? String::fromUTF8(pic->x_rcv_raw->s_name) : getBinbufSymbol(4);

            sendSymbol = sndSym != "empty" ? sndSym : "";
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            offsetX = pic->x_offset_x;
            offsetY = pic->x_offset_y;

            sizeProperty = VarArray { var(pic->x_width), var(pic->x_height) };
        }

        repaint();
        object->updateIolets();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("latch"): {
            if (auto pic = ptr.get<t_fake_pic>())
                latch = pic->x_latch;
            break;
        }
        case hash("offset"): {
            if (auto pic = ptr.get<t_fake_pic>()) {
                offsetX = pic->x_offset_x;
                offsetY = pic->x_offset_y;
            }
            repaint();
            break;
        }
        case hash("outline"): {
            if (auto pic = ptr.get<t_fake_pic>())
                outline = pic->x_outline;
            break;
        }
        case hash("open"): {
            if (atoms.size() >= 1)
                filename = atoms[0].toString();
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

    void updateImage(NVGcontext* nvg)
    {
        if (!img.isValid() && File(imageFile).existsAsFile()) {
            img = ImageFileFormat::loadFrom(imageFile).convertedToFormat(Image::ARGB);
        }
 
        if (img.isValid()) {
            imageBuffer = NVGImage(nvg, img.getWidth(), img.getHeight(), [this](Graphics& g) {
                g.drawImageAt(img, 0, 0);
            }, NVGImage::MipMap);
        }

        img = Image(); // Clear image from CPU memory after upload

        imageNeedsReload = false;
    }

    void render(NVGcontext* nvg) override
    {
        handleUpdateNowIfNeeded();
            
        if (imageNeedsReload || !imageBuffer.isValid())
            updateImage(nvg);

        auto const b = getLocalBounds().toFloat();

        NVGScopedState scopedState(nvg);
        nvgIntersectScissor(nvg, 0, 0, getWidth(), getHeight());

        if (!imageBuffer.isValid()) {
            nvgFontSize(nvg, 20);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId)));
            nvgText(nvg, b.getCentreX(), b.getCentreY(), "?", nullptr);
        } else {
            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, offsetX, offsetY);
            imageBuffer.render(nvg, getLocalBounds());
        }

        bool const selected = object->isSelected() && !cnv->isGraph;
        auto const outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (getValue<bool>(outline)) {
            nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), convertColour(outlineColour), Corners::objectCornerRadius);
        }
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

            if (auto pic = ptr.get<t_fake_pic>()) {
                pic->x_width = width;
                pic->x_height = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(filename)) {
            triggerAsyncUpdate();
        } else if (value.refersToSameSourceAs(latch)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_latch = getValue<int>(latch);
        } else if (value.refersToSameSourceAs(outline)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_outline = getValue<int>(outline);
        } else if (value.refersToSameSourceAs(reportSize)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_size = getValue<int>(reportSize);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            if(symbol.isEmpty()) symbol = "empty";
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "send", { pd->generateSymbol(symbol) });
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            if(symbol.isEmpty()) symbol = "empty";
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "receive", { pd->generateSymbol(symbol) });
            object->updateIolets();
        }
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {
            auto* patch = cnv->patch.getRawPointer();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, pic.cast<t_gobj>(), b.getX(), b.getY());

            pic->x_width = b.getWidth();
            pic->x_height = b.getHeight();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {
            auto* patch = cnv->patch.getRawPointer();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, pic.cast<t_gobj>(), &x, &y, &w, &h);
            return { x, y, w, h };
        }

        return {};
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto pic = ptr.get<t_fake_pic>()) {
            setParameterExcludingListener(sizeProperty, VarArray { var(pic->x_width), var(pic->x_height) });
        }
    }

    void openFile(String const& location)
    {
        if (location.isEmpty() || location == "empty")
            return;

        auto findFile = [this](String const& name) {
            if (auto patch = cnv->patch.getPointer()) {
                if ((name.startsWith("/") || name.startsWith("./") || name.startsWith("../")) && File(name).existsAsFile()) {
                    return File(name);
                }

                char dir[MAXPDSTRING];
                char* file;

                int const fd = canvas_open(patch.get(), name.toRawUTF8(), "", dir, &file, MAXPDSTRING, 0);
                if (fd >= 0) {
                    sys_close(fd);
                    return File(dir).getChildFile(file);
                }
            }
            return File(name);
        };

        imageFile = findFile(location);

        auto const pathString = imageFile.getFullPathName();
        auto const fileNameString = imageFile.getFileName();

        auto* rawFileName = fileNameString.toRawUTF8();
        auto* rawPath = pathString.toRawUTF8();

        img = ImageFileFormat::loadFrom(imageFile).convertedToFormat(Image::ARGB);
        imageNeedsReload = true;

        if (auto pic = ptr.get<t_fake_pic>()) {
            pic->x_filename = pd->generateSymbol(rawFileName);
            pic->x_fullname = pd->generateSymbol(rawPath);
            pic->x_width = img.getWidth();
            pic->x_height = img.getHeight();

            if (getValue<bool>(reportSize)) {
                StackArray<t_atom, 2> coordinates;
                SETFLOAT(&coordinates[0], img.getWidth());
                SETFLOAT(&coordinates[1], img.getHeight());
                outlet_list(pic->x_outlet, pd->generateSymbol("list"), 2, coordinates.data());
            }
        }

        object->updateBounds();
        repaint();
    }
};
