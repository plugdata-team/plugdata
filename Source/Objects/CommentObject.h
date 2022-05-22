/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct CommentObject : public GUIObject
{
    CommentObject(void* obj, Box* box) : GUIObject(obj, box)
    {
        addAndMakeVisible(input);
        input.setText(getText(), dontSendNotification);
        input.setInterceptsMouseClicks(false, false);
        input.setMinimumHorizontalScale(0.9f);

        setInterceptsMouseClicks(false, false);

        input.onTextChange = [this, box]()
        {
            String name = input.getText();
            cnv->pd->enqueueFunction(
                [this, box, name]() mutable
                {
                    auto* newName = name.toRawUTF8();
                    libpd_renameobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), newName, input.getText().getNumBytesAsUTF8());

                    MessageManager::callAsync([box]() { box->updateBounds(); });
                });
        };

        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            if (editor)
            {
                editor->setMultiLine(true, true);
                editor->setReturnKeyStartsNewLine(true);
            }
        };

        initialise();

        // Our component doesn't intercept mouse events, so dragging will be okay
        box->addMouseListener(this, false);
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (cnv->isSelected(box) && !box->selectionChanged)
        {
            shouldOpenEditor = true;
        }
    }

    // Makes it transparent
    void paint(Graphics& g) override
    {
    }

    void mouseUp(const MouseEvent& e) override
    {
        // Edit messages when unlocked, edit atoms when locked
        if (!isLocked && shouldOpenEditor && !e.mouseWasDraggedSinceMouseDown())
        {
            input.showEditor();
            shouldOpenEditor = false;
        }
    }

    void lock(bool locked) override
    {
        isLocked = locked;
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
    }

    /*
    bool usesCharWidth() override
    {
        return true;
    } */

    void updateBounds() override
    {
        box->setBounds(getBounds().expanded(Box::margin));
    }

    void checkBoxBounds() override
    {
        int numLines = getNumLines(getText(), box->getWidth());
        box->setSize(box->getWidth(), (numLines * 19) + Box::doubleMargin);
    }

    Label input;
    bool shouldOpenEditor = false;
    bool isLocked = false;
};
