/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// ELSE pic
class PictureObject final : public ObjectBase {

    Value path, latch, outline, reportSize, sendSymbol, receiveSymbol;
    Value sizeProperty;
    
    File imageFile;
    Image img;

public:
    PictureObject(void* ptr, Object* object)
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
    }

    void mouseDown(MouseEvent const& e) override
    {
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
            sendSymbol = pic->x_snd_raw == pd->generateSymbol("empty") ? "" : String::fromUTF8(pic->x_snd_raw->s_name);
            receiveSymbol = pic->x_rcv_raw == pd->generateSymbol("empty") ? "" : String::fromUTF8(pic->x_rcv_raw->s_name);
            
            sizeProperty = Array<var>{var(pic->x_width), var(pic->x_height)};
        }

        repaint();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("latch"),
            hash("outline"),
            hash("open"),
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {

        switch (hash(symbol)) {
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
            if (atoms.size() >= 1)
                openFile(atoms[0].getSymbol());
            break;
        }
        }
    }

    void paint(Graphics& g) override
    {
        if (imageFile.existsAsFile()) {
            g.drawImageAt(img, 0, 0);
        } else {
            Fonts::drawText(g, "?", getLocalBounds(), object->findColour(PlugDataColour::canvasTextColourId), 30, Justification::centred);
        }

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (getValue<bool>(outline)) {
            g.setColour(outlineColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());
            
            setParameterExcludingListener(sizeProperty, Array<var>{var(width), var(height)});
            
            if (auto pic = ptr.get<t_fake_pic>())
            {
                pic->x_width = width;
                pic->x_height = height;
            }

            object->updateBounds();
        }
        else if (value.refersToSameSourceAs(path)) {
            openFile(path.toString());
        } else if (value.refersToSameSourceAs(latch)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_latch = getValue<int>(latch);
        } else if (value.refersToSameSourceAs(outline)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_outline = getValue<int>(latch);
        } else if (value.refersToSameSourceAs(reportSize)) {
            if (auto pic = ptr.get<t_fake_pic>())
                pic->x_size = getValue<int>(reportSize);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "send", { symbol });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            if (auto pic = ptr.get<t_pd>())
                pd->sendDirectMessage(pic.get(), "receive", { symbol });
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto pic = ptr.get<t_fake_pic>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            libpd_moveobj(patch, pic.cast<t_gobj>(), b.getX(), b.getY());

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
            libpd_get_object_bounds(patch, pic.cast<t_gobj>(), &x, &y, &w, &h);
            return { x, y, w, h };
        }

        return {};
    }
    
    void objectSizeChanged() override
    {
        setPdBounds(object->getObjectBounds());
        
        if (auto pic = ptr.get<t_fake_pic>()) {
            setParameterExcludingListener(sizeProperty, Array<var>{var(pic->x_width), var(pic->x_height)});
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
            libpd_get_search_paths(paths, &numItems);

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
