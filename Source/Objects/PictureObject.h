/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// ELSE pic
class PictureObject final : public ObjectBase, public NVGContextListener {

    Value path = SynchronousValue();
    Value latch = SynchronousValue();
    Value outline = SynchronousValue();
    Value reportSize = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    File imageFile;
    Image img;
    std::vector<std::pair<int, Rectangle<int>>> imageBuffers;
    bool imageNeedsReload = false;
    uint8 pixelDataBuffer[8192 * 8192 * 4];
    
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
        objectParameters.addParamString("File", cGeneral, &path, "");
        objectParameters.addParamBool("Latch", cGeneral, &latch, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Outline", cAppearance, &outline, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Report Size", cAppearance, &reportSize, { "No", "Yes" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
        
        cnv->editor->nvgSurface.addNVGContextListener(this);
    }
    
    ~PictureObject()
    {
        // TODO: delete image buffers!
        
        cnv->editor->nvgSurface.addNVGContextListener(this);
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
            }
        } else {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_bang(pic->x_outlet);
            }
        }
    }
    
    void nvgContextDeleted(NVGcontext* nvg) override {
        imageNeedsReload = true;
    };

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(latch)) {
            if (auto pic = ptr.get<t_fake_pic>()) {
                outlet_float(pic->x_outlet, 0.0f);
            }
        }
    }

    void update() override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {

            if (pic->x_fullname) {
                path = String::fromUTF8(pic->x_fullname->s_name);
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

    int convertImage(NVGcontext* nvg, Image& image, Rectangle<int> bounds)
    {
        Image::BitmapData imageData(image, Image::BitmapData::readOnly);
        for (int y = 0; y < bounds.getHeight(); y++)
        {
            auto* scanLine = (uint32*) imageData.getLinePointer(y + bounds.getY());
            
            for (int x = 0; x < bounds.getWidth(); x++)
            {
                uint32 argb = scanLine[x + bounds.getX()];
                int bufferPos = (y * bounds.getWidth() + x) * 4;
                
                pixelDataBuffer[bufferPos + 0] = (argb >> 16) & 0xFF; // Red
                pixelDataBuffer[bufferPos + 1] = (argb >> 8) & 0xFF;  // Green
                pixelDataBuffer[bufferPos + 2] = argb & 0xFF;         // Blue
                pixelDataBuffer[bufferPos + 3] = (argb >> 24) & 0xFF; // Alpha
            }
        }
        
        return nvgCreateImageRGBA(nvg, bounds.getWidth(), bounds.getHeight(), NVG_IMAGE_PREMULTIPLIED, pixelDataBuffer);
    }
    
    void updateImage(NVGcontext* nvg)
    {
        for(auto& [image, bounds] : imageBuffers)
        {
            nvgDeleteImage(nvg, image);
        }
        imageBuffers.clear();
        
        int imageWidth = img.getWidth();
        int imageHeight = img.getHeight();
        int x = 0;
        while(x < imageWidth)
        {
            int y = 0;
            int width = std::min(8192, imageWidth - x);
            while(y < imageHeight)
            {
                int height = std::min(8192, imageHeight - y);
                auto bounds = Rectangle<int>(x, y, width, height);
                imageBuffers.emplace_back(convertImage(nvg, img, bounds), bounds);
                y += 8192;
                
            }
            x += 8192;
        }
        
        imageNeedsReload = false;
    }
    
    void render(NVGcontext* nvg) override
    {
        if(imageNeedsReload) updateImage(nvg);
        
        auto b = getLocalBounds().toFloat();
        
        nvgSave(nvg);
        nvgIntersectScissor(nvg, 0, 0, getWidth(), getHeight());
        if(imageBuffers.empty())
        {
            nvgFontSize(nvg, 20);
            nvgFontFace(nvg, "Inter-Regular");
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(nvg, convertColour(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::canvasTextColourId)));
            nvgText(nvg, b.getCentreX(), b.getCentreY(), "?", 0);
        }
        else {
            for(auto& [image, bounds] : imageBuffers)
            {
                nvgBeginPath(nvg);
                nvgRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
                nvgFillPaint(nvg, nvgImagePattern(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, image, 1.0f));
                nvgFill(nvg);
            }
        }
        
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = LookAndFeel::getDefaultLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (getValue<bool>(outline)) {
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
            nvgStrokeWidth(nvg, 1.0f);
            nvgStrokeColor(nvg, convertColour(outlineColour));
            nvgStroke(nvg);
        }
        
        nvgRestore(nvg);
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
        } else if (value.refersToSameSourceAs(path)) {
            openFile(path.toString());
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
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return File();

            if ((name.startsWith("/") || name.startsWith("./") || name.startsWith("../")) && File(name).existsAsFile()) {
                return File(name);
            }

            if (File(String::fromUTF8(canvas_getdir(patch)->s_name)).getChildFile(name).existsAsFile()) {
                return File(String::fromUTF8(canvas_getdir(patch)->s_name)).getChildFile(name);
            }

            // Get pd's search paths
            char* paths[1024];
            int numItems;
            pd::Interface::getSearchPaths(paths, &numItems);

            for (int i = 0; i < numItems; i++) {
                auto file = File(String::fromUTF8(paths[i])).getChildFile(name);

                if (file.existsAsFile()) {
                    return file;
                }
            }

            return File(name);
        };

        imageFile = findFile(location);

        auto pathString = imageFile.getFullPathName();
        auto fileNameString = imageFile.getFileName();

        auto* rawFileName = fileNameString.toRawUTF8();
        auto* rawPath = pathString.toRawUTF8();

        img = ImageFileFormat::loadFrom(imageFile);
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
