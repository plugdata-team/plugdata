// ELSE pic
struct PictureObject final : public GUIObject {
    typedef struct _edit_proxy {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _pic* p_cnv;
    } t_edit_proxy;

    typedef struct _pic {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int x_zoom;
        int x_width;
        int x_height;
        int x_snd_set;
        int x_rcv_set;
        int x_edit;
        int x_init;
        int x_def_img;
        int x_sel;
        int x_outline;
        int x_s_flag;
        int x_r_flag;
        int x_flag;
        int x_size;
        int x_latch;
        t_symbol* x_fullname;
        t_symbol* x_filename;
        t_symbol* x_x;
        t_symbol* x_receive;
        t_symbol* x_rcv_raw;
        t_symbol* x_send;
        t_symbol* x_snd_raw;
        t_outlet* x_outlet;
    } t_pic;

    PictureObject(void* ptr, Object* object)
        : GUIObject(ptr, object)
    {
        auto* pic = static_cast<t_pic*>(ptr);

        if (pic && pic->x_filename) {
            auto filePath = String::fromUTF8(pic->x_filename->s_name);
            if (File(filePath).existsAsFile()) {
                path = filePath;
            }
        }
    }

    ObjectParameters defineParameters() override
    {
        return { { "File", tString, cGeneral, &path, {} } };
    };

    void paint(Graphics& g) override
    {
        if (imageFile.existsAsFile()) {
            g.drawImageAt(img, 0, 0);
        } else {
            g.setFont(30);
            g.setColour(object->findColour(PlugDataColour::textColourId));
            g.drawText("?", getLocalBounds(), Justification::centred);
        }

        auto outlineColour = object->findColour(cnv->isSelected(object) && !cnv->isGraph ? PlugDataColour::highlightColourId : PlugDataColour::canvasOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(path)) {
            openFile(path.toString());
        }
    }

    void applyBounds() override
    {
        auto b = object->getObjectBounds();
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* pic = static_cast<t_pic*>(ptr);
        pic->x_width = b.getWidth();
        pic->x_height = b.getHeight();
    }

    void updateBounds() override
    {
        pd->getCallbackLock()->enter();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->getCallbackLock()->exit();

        object->setObjectBounds(bounds);
    }

    void checkBounds() override
    {
        auto* pic = static_cast<t_pic*>(ptr);

        if (!imageFile.existsAsFile()) {
            object->setSize(50, 50);
        } else if (pic->x_height != img.getHeight() || pic->x_width != img.getWidth()) {
            object->setSize(img.getWidth(), img.getHeight());
        }
    }

    void openFile(String location)
    {
        auto* pic = static_cast<t_pic*>(ptr);

        String pathString = location;

        auto searchPath = File(String::fromUTF8(canvas_getdir(cnv->patch.getPointer())->s_name));

        if (searchPath.getChildFile(pathString).existsAsFile()) {
            imageFile = searchPath.getChildFile(pathString);
        } else {
            imageFile = File(pathString);
        }

        pathString = imageFile.getFullPathName();
        auto* charptr = pathString.toRawUTF8();

        pic->x_filename = gensym(charptr);
        pic->x_fullname = gensym(charptr);

        img = ImageFileFormat::loadFrom(imageFile);

        pic->x_width = img.getWidth();
        pic->x_height = img.getHeight();

        object->updateBounds();
        repaint();
    }

    static char const* pic_filepath(t_pic* x, char const* filename)
    {
        static char fname[MAXPDSTRING];
        char* bufptr;
        int fd = open_via_path(canvas_getdir(glist_getcanvas(x->x_glist))->s_name, filename, "", fname, &bufptr, MAXPDSTRING, 1);
        if (fd > 0) {
            fname[strlen(fname)] = '/';
            sys_close(fd);
            return (fname);
        } else
            return (0);
    }

    Value path;
    File imageFile;
    Image img;
};
