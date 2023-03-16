/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

struct ColourPicker : public ColourSelector
    , public ChangeListener {
    static inline bool isShowing = false;
        
    bool onlyCallbackOnClose;

    ColourPicker(bool noLiveChangeCallback, std::function<void(Colour)> cb)
        : ColourSelector(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace)
        , callback(cb), onlyCallbackOnClose(noLiveChangeCallback)
    {
        setSize(300, 400);
        
        if(!onlyCallbackOnClose) {
            addChangeListener(this);
        }

        auto& lnf = LookAndFeel::getDefaultLookAndFeel();

        setColour(ColourSelector::backgroundColourId, lnf.findColour(PlugDataColour::panelBackgroundColourId));
    }

    ~ColourPicker()
    {
        if(onlyCallbackOnClose) callback(getCurrentColour());
        removeChangeListener(this);
        isShowing = false;
    }

    static void show(bool onlySendCallbackOnClose, Colour currentColour, Rectangle<int> bounds, std::function<void(Colour)> callback)
    {
        if (isShowing)
            return;

        isShowing = true;

        std::unique_ptr<ColourPicker> colourSelector = std::make_unique<ColourPicker>(onlySendCallbackOnClose, callback);

        colourSelector->setCurrentColour(currentColour);
        CallOutBox::launchAsynchronously(std::move(colourSelector), bounds, nullptr);
    }

private:
    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        callback(dynamic_cast<ColourSelector*>(source)->getCurrentColour());
    }

    std::function<void(Colour)> callback = [](Colour) {};
};
