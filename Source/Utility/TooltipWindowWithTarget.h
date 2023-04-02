/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "StackShadow.h"
#include "../Constants.h"

template<typename T, typename Excluded>
class TooltipWindowWithTarget : public TooltipWindow
{
    
public:
    TooltipWindowWithTarget(Component* target, LookAndFeel* lnf) : TooltipWindow(target, 500),
        tooltipShadow(DropShadow(Colour(0, 0, 0).withAlpha(0.2f), 4, { 0, 0 }), Corners::defaultCornerRadius)
    {
        setOpaque(false);
        setLookAndFeel(lnf);
        tooltipShadow.setOwner(this);
    }
    
    void hide(bool hidden) {
        // TooltipWindow already uses the setVisible flag internally, we can't use that, so we use setAlpha instead
        setAlpha(!hidden);
        tooltipShadow.setOwner(hidden ? nullptr : this);
    }
    
private:
    String getTipFor(Component& c) override
    {
        if(!std::is_same<Excluded, void>() && (dynamic_cast<Excluded*>(&c) || c.findParentComponentOfClass<Excluded>())) {
            return "";
        }
        
        if(dynamic_cast<T*>(&c) || c.findParentComponentOfClass<T>()) {
            return TooltipWindow::getTipFor(c);
        }
        
        return "";
    }
    
    StackDropShadower tooltipShadow;
};
