/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// ELSE pic
class PictureObject final : public ObjectBase {

    Value filename = SynchronousValue();
    Value latch = SynchronousValue();
    Value outline = SynchronousValue();
    Value reportSize = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    File imageFile;
    Image img;
    std::vector<std::pair<std::unique_ptr<NVGImage>, Rectangle<int>>> imageBuffers;
    bool imageNeedsReload = false;

public:
    PictureObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        if (auto pic = this->ptr.get<t_fake_pic>()) {
            if (pic->x_filename) {
                auto filePath = String::fromUTF8(pic->x_filename->s_name);

                // Call async prevents a bug when renaming an object to pic
                MessageManager::callAsync([_this = SafePointer(this), filePath]() {
                    if (_this) {
                        _this->openFile(filePath);
                    }
                });
            }
        }

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamString("File", cGeneral, &filename, "");
        objectParameters.addParamBool("Latch", cGeneral, &latch, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Outline", cAppearance, &outline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Report Size", cAppearance, &reportSize, { "No", "Yes" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
    }

    ~PictureObject()
    {
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
                if(pic->x_send != gensym("") && pic->x_send->s_thing) pd_float(pic->x_send->s_thing, 1.0f);
            }
        } else {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_bang(pic->x_outlet);
                if(pic->x_send != gensym("") && pic->x_send->s_thing) pd_bang(pic->x_send->s_thing);
            }
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(latch)) {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_float(pic->x_outlet, 0.0f);
                if(pic->x_send != gensym("") && pic->x_send->s_thing) pd_float(pic->x_send->s_thing, 0.0f);
            }
        }
    }

    void update() override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {

            if (pic->x_filename) {
                filename = String::fromUTF8(pic->x_filename->s_name);
            }

            latch = pic->x_latch;
            outline = pic->x_outline;
            reportSize = pic->x_size;

            auto sndSym = pic->x_snd_set ? String::fromUTF8(pic->x_snd_raw->s_name) : getBinbufSymbol(3);
            auto rcvSym = pic->x_rcv_set ? String::fromUTF8(pic->x_rcv_raw->s_name) : getBinbufSymbol(4);

            sendSymbol = sndSym != "empty" ? sndSym : "";
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            sizeProperty = Array<var> { var(pic->x_width), var(pic->x_height) };
        }

        repaint();
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("latch"): {
            if (auto pic = ptr.get<t_fake_pic>())
                latch = pic->x_latch;
            break;
        }
        case hash("offset"): {
            repaint();
            break;
        }
        case hash("outline"): {
            if (auto pic = ptr.get<t_fake_pic>())
                outline = pic->x_outline;
            break;
        }
        case hash("open"): {
            if (numAtoms >= 1)
                openFile(atoms[0].toString());
            break;
        }
        }
    }

    void updateImage(NVGcontext* nvg)
    {
        imageBuffers.clear();
        
        if(!img.isValid() && File(imageFile).existsAsFile())
        {
            img = ImageFileFormat::loadFrom(imageFile).convertedToFormat(Image::ARGB);
        }

        int imageWidth = img.getWidth();
        int imageHeight = img.getHeight();
        int x = 0;
        while (x < imageWidth) {
            int y = 0;
            int width = std::min(8192, imageWidth - x);
            while (y < imageHeight) {
                int height = std::min(8192, imageHeight - y);
                auto bounds = Rectangle<int>(x, y, width, height);
                auto clip = img.getClippedImage(bounds);

                auto partialImage = std::make_unique<NVGImage>(nvg, width, height, [&clip](Graphics& g) {
                    g.drawImageAt(clip, 0, 0);
                });

                partialImage->onImageInvalidate = [this]() {
                    imageNeedsReload = true;
                    repaint();
                };

                imageBuffers.emplace_back(std::move(partialImage), bounds);
                y += 8192;
            }
            x += 8192;
        }
        
        img = Image(); // Clear image from CPU memory after upload

        imageNeedsReload = false;
    }

    void render(NVGcontext* nvg) override
    {
        if (imageNeedsReload)
            updateImage(nvg);

        auto b = getLocalBounds().toFloat();

        NVGScopedState scopedState(nvg);
        nvgIntersectScissor(nvg, 0, 0, getWidth(), getHeight());
        if (imageBuffers.empty()) {
            nvgFontSize(nvg, 20);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId)));
            nvgText(nvg, b.getCentreX(), b.getCentreY(), "?", 0);
        } else {

            int offsetX = 0, offsetY = 0;
            if (auto pic = ptr.get<t_fake_pic>()) {
                offsetX = pic->x_offset_x;
                offsetY = pic->x_offset_y;
            }

            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, offsetX, offsetY);
            for (auto& [image, bounds] : imageBuffers) {
                nvgFillPaint(nvg, nvgImagePattern(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, image->getImageId(), 1.0f));
                nvgFillRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
            }
        }

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (getValue<bool>(outline)) {
            nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(0, 0, 0, 0), convertColour(outlineColour), Corners::objectCornerRadius);
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto pic = ptr.get<t_fake_pic>()) {
                pic->x_width = width;
                pic->x_height = height;
            }
            
            object->updateBounds();
        } else if (value.refersToSameSourceAs(filename)) {
            openFile(filename.toString());
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
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "send", { pd->generateSymbol(symbol) });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "receive", { pd->generateSymbol(symbol) });
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {
            auto* patch = cnv->patch.getPointer().get();
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
            auto* patch = cnv->patch.getPointer().get();
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
            setParameterExcludingListener(sizeProperty, Array<var> { var(pic->x_width), var(pic->x_height) });
        }
    }

    void openFile(String const& location)
    {
        if (location.isEmpty() || location == "empty")
            return;

        auto findFile = [this](String const& name) {
            if(auto patch = cnv->patch.getPointer()) {
                if ((name.startsWith("/") || name.startsWith("./") || name.startsWith("../")) && File(name).existsAsFile()) {
                    return File(name);
                }
                
                char dir[MAXPDSTRING];
                char* file;
                
                int fd = canvas_open(patch.get(), name.toRawUTF8(), "", dir, &file, MAXPDSTRING, 0);
                if(fd >= 0){
                    return File(dir).getChildFile(file);
                }
            }
            return File(name);
        };

        imageFile = findFile(location);

        auto pathString = imageFile.getFullPathName();
        auto fileNameString = imageFile.getFileName();

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
                t_atom coordinates[2];
                SETFLOAT(coordinates, img.getWidth());
                SETFLOAT(coordinates + 1, img.getHeight());
                outlet_list(pic->x_outlet, pd->generateSymbol("list"), 2, coordinates);
            }
        }

        object->updateBounds();
        repaint();
    }
};
