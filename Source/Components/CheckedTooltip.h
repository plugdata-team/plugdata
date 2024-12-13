/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>
#include "Constants.h"

class CheckedTooltip final : public TooltipWindow {

public:
    explicit CheckedTooltip(
        Component* target, std::function<bool(Component*)> checkTooltip = [](Component*) { return true; }, int const timeout = 500)
        : TooltipWindow(target, timeout)
        , checker(std::move(checkTooltip))
    {
    }

    float getDesktopScaleFactor() const override
    {
        return Component::getDesktopScaleFactor();
    }

    void setVisible(bool const shouldBeVisible) override
    {
        if (shouldBeVisible && !isVisible()) {
            if (isCurrentlyBlockedByAnotherModalComponent()) {
                return;
            }
        }
        TooltipWindow::setVisible(shouldBeVisible);
    }

private:
    String getTipFor(Component& c) override
    {
        if (checker(&c)) {
            return TooltipWindow::getTipFor(c);
        }
        return "";
    }

    std::function<bool(Component*)> checker;
};
