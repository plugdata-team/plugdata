/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct CommentComponent : public GUIComponent
{
    CommentComponent(const pd::Gui& pdGui, Box* box, bool newObject) : GUIComponent(pdGui, box, newObject)
    {
        addAndMakeVisible(input);
        input.setText(gui.getText(), dontSendNotification);
        input.setInterceptsMouseClicks(false, false);

        setInterceptsMouseClicks(false, false);

        input.onTextChange = [this, box]()
        {
            String name = input.getText();
            box->cnv->pd->enqueueFunction(
                [this, box, name]() mutable
                {
                    auto* newName = name.toRawUTF8();
                    libpd_renameobj(box->cnv->patch.getPointer(), static_cast<t_gobj*>(gui.getPointer()), newName, input.getText().getNumBytesAsUTF8());

                    MessageManager::callAsync([box]() { box->updateBounds(false); });
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

        initialise(newObject);

        // Our component doesn't intercept mouse events, so dragging will be okay
        box->addMouseListener(this, false);
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (box->cnv->isSelected(box) && !box->selectionChanged)
        {
            shouldOpenEditor = true;
        }
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

    void checkBoxBounds() override
    {
        int numLines = getNumLines(gui.getText(), box->getWidth() - Box::doubleMargin);
        box->setSize(box->getWidth(), (numLines * (box->font.getHeight() + 4)) + Box::doubleMargin);
    }

    bool shouldOpenEditor = false;
    bool isLocked = false;
};
