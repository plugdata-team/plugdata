/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// ELSE pic
class PictureObject final : public ObjectBase {

    Value path, latch, outline, reportSize, sendSymbol, receiveSymbol;
    File imageFile;
    Image img;

public:
    PictureObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
    {
        auto* pic = static_cast<t_fake_pic*>(ptr);

        if (pic && pic->x_filename) {
            auto filePath = String::fromUTF8(pic->x_filename->s_name);

            // Call async prevents a bug when renaming an object to pic
            MessageManager::callAsync([_this = SafePointer(this), filePath]() {
                if (_this) {
                    _this->openFile(filePath);
                }
            });
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (getValue<bool>(latch)) {
            auto* pic = static_cast<t_fake_pic*>(ptr);
            pd->enqueueFunction([pic]() {
                outlet_float(pic->x_outlet, 1.0f);
            });
        } else {
            auto* pic = static_cast<t_fake_pic*>(ptr);
            pd->enqueueFunction([pic]() {
                outlet_bang(pic->x_outlet);
            });
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(latch)) {
            auto* pic = static_cast<t_fake_pic*>(ptr);
            pd->enqueueFunction([pic]() {
                outlet_float(pic->x_outlet, 0.0f);
            });
        }
    }

    void update() override
    {
        auto* pic = static_cast<t_fake_pic*>(ptr);

        if (pic->x_fullname) {
            path = String::fromUTF8(pic->x_fullname->s_name);
        }

        latch = pic->x_latch;
        outline = pic->x_outline;
        reportSize = pic->x_size;
        sendSymbol = pic->x_snd_raw == pd->generateSymbol("empty") ? "" : String::fromUTF8(pic->x_snd_raw->s_name);
        receiveSymbol = pic->x_rcv_raw == pd->generateSymbol("empty") ? "" : String::fromUTF8(pic->x_rcv_raw->s_name);

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
        auto* pic = static_cast<t_fake_pic*>(ptr);

        switch (hash(symbol)) {
        case hash("latch"): {
            latch = pic->x_latch;
            break;
        }
        case hash("outline"): {
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

    ObjectParameters getParameters() override
    {
        return {
            { "File", tString, cGeneral, &path, {} },
            { "Latch", tBool, cGeneral, &latch, { "No", "Yes" } },
            { "Outline", tBool, cAppearance, &outline, { "No", "Yes" } },
            { "Report Size", tBool, cAppearance, &reportSize, { "No", "Yes" } },
            { "Receive symbol", tString, cGeneral, &receiveSymbol, {} },
            { "Send symbol", tString, cGeneral, &sendSymbol, {} }
        };
    };

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
        auto* pic = static_cast<t_fake_pic*>(ptr);

        if (value.refersToSameSourceAs(path)) {
            openFile(path.toString());
        } else if (value.refersToSameSourceAs(latch)) {
            pic->x_latch = getValue<int>(latch);
        } else if (value.refersToSameSourceAs(outline)) {
            pic->x_outline = getValue<int>(latch);
        } else if (value.refersToSameSourceAs(reportSize)) {
            pic->x_size = getValue<int>(reportSize);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            pd->enqueueDirectMessages(ptr, "send", { symbol });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            pd->enqueueDirectMessages(ptr, "receive", { symbol });
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* pic = static_cast<t_fake_pic*>(ptr);
        pic->x_width = b.getWidth();
        pic->x_height = b.getHeight();
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->unlockAudioThread();

        return bounds;
    }

    void openFile(String location)
    {
        if (location.isEmpty() || location == "empty")
            return;

        auto findFile = [this](String const& name) {
            if ((name.startsWith("/") || name.startsWith("./") || name.startsWith("../")) && File(name).existsAsFile()) {
                return File(name);
            }
            if (File(String::fromUTF8(canvas_getdir(cnv->patch.getPointer())->s_name)).getChildFile(name).existsAsFile()) {
                return File(String::fromUTF8(canvas_getdir(cnv->patch.getPointer())->s_name)).getChildFile(name);
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

        auto* pic = static_cast<t_fake_pic*>(ptr);

        imageFile = findFile(location);

        auto pathString = imageFile.getFullPathName();
        auto fileNameString = imageFile.getFileName();

        auto* rawFileName = fileNameString.toRawUTF8();
        auto* rawPath = pathString.toRawUTF8();

        pic->x_filename = pd->generateSymbol(rawFileName);
        pic->x_fullname = pd->generateSymbol(rawPath);

        img = ImageFileFormat::loadFrom(imageFile);

        pic->x_width = img.getWidth();
        pic->x_height = img.getHeight();

        if (getValue<bool>(reportSize)) {
            t_atom coordinates[2];
            SETFLOAT(coordinates, img.getWidth());
            SETFLOAT(coordinates + 1, img.getHeight());
            outlet_list(pic->x_outlet, pd->generateSymbol("list"), 2, coordinates);
        }

        object->updateBounds();
        repaint();
    }
};
