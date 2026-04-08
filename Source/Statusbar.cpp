/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Statusbar.h"
#include "LookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "CanvasViewport.h"

#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"

class ZoomLabel final : public Component {
public:
    explicit ZoomLabel(Statusbar* parent)
        : statusbar(parent)
    {
        addAndMakeVisible(menuButton);
        menuButton.onClick = [this]() {
            auto* editor = findParentComponentOfClass<PluginEditor>();

            PopupMenu menu;
            for (auto const& opt : StringArray { "25%", "50%", "75%", "100%", "125%", "150%", "175%", "200%", "250%", "300%" }) {
                auto scale = opt.upToFirstOccurrenceOf("%", false, false).getIntValue() / 100.0f;
                menu.addItem(opt, [editor, scale] {
                    if (auto* cnv = editor->getCurrentCanvas())
                        cnv->viewport->magnifyCentred(scale, true);
                });
            }

            menu.addSeparator();
            menu.addItem("Zoom to fit content", [editor] {
                if (auto* cnv = editor->getCurrentCanvas())
                    cnv->zoomToFitAll();
            });
            menu.addItem("Jump to Origin", [editor] {
                if (auto* cnv = editor->getCurrentCanvas())
                    cnv->jumpToOrigin();
            });

            menu.showMenuAsync(PopupMenu::Options()
                    .withMinimumWidth(150)
                    .withMaximumNumColumns(1)
                    .withTargetComponent(&menuButton));
        };

        setRepaintsOnMouseActivity(true);
    }

private:
    void paint(Graphics& g) override
    {
        g.setFont(Fonts::getTabularNumbersFont().withHeight(14));
        if (isEnabled()) {
            g.setColour(PlugDataColours::toolbarTextColour.contrasting(isMouseOver() ? 0.35f : 0.0f));
        } else {
            g.setColour(PlugDataColours::toolbarTextColour.withAlpha(0.65f));
        }
        g.drawFittedText(String(std::clamp<int>(statusbar->currentZoomLevel, 25, 300)) + "%", 6, 0, getWidth() - 2, getHeight(), Justification::centredLeft, 1, 0.95f);
    }

    void enablementChanged() override
    {
        repaint();
    }

    void resized() override
    {
        menuButton.setBounds(getLocalBounds().removeFromRight(16));
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            float const newScale = std::clamp(getValue<float>(cnv->zoomScale) + wheel.deltaY, 0.25f, 3.0f);
            cnv->viewport->magnifyCentred(newScale, false);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!isEnabled() || !e.mods.isLeftButtonDown())
            return;

        auto* editor = findParentComponentOfClass<PluginEditor>();
        if (auto* cnv = editor->getCurrentCanvas()) {
            auto const defaultZoom = SettingsFile::getInstance()->getProperty<float>("default_zoom") / 100.0f;
            cnv->viewport->magnifyCentred(defaultZoom, true);
        }
    }

    SmallIconButton menuButton = SmallIconButton(Icons::ThinDown);
    Statusbar* statusbar;
};

class CanvasModePicker final : public Component {
public:
    struct ModeButton final : public TextButton {
        String const icon;
        String const description;

        ModeButton(String iconString, String descriptionString, bool const toggleButton)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
            setClickingTogglesState(toggleButton);
        }

        void paint(Graphics& g) override
        {
            auto colour = PlugDataColours::toolbarTextColour;
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14.5);

            if (getToggleState()) {
                colour = PlugDataColours::toolbarActiveColour;
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    explicit CanvasModePicker(PluginEditor* editor)
    {
        for (auto* button : buttons) {
            addAndMakeVisible(*button);
        }

        buttons[0]->onClick = [this, editor]() mutable {
            if (auto* cnv = editor->getCurrentCanvas())
                cnv->locked = false;
            updateModeIcon(0);
            closeCalloutBox();
        };
        buttons[1]->onClick = [this, editor]() mutable {
            if (auto* cnv = editor->getCurrentCanvas())
                cnv->locked = true;
            updateModeIcon(1);
            closeCalloutBox();
        };
        buttons[2]->onClick = [this, editor]() mutable {
            if (auto* cnv = editor->getCurrentCanvas()) {
                cnv->locked = true;
                cnv->presentationMode = true;
            }
            updateModeIcon(2);
            closeCalloutBox();
        };
        buttons[3]->onClick = [this, editor]() mutable {
            editor->getTabComponent().openInPluginMode(editor->getCurrentCanvas()->refCountedPatch);
            closeCalloutBox();
        };

        setSize(150, 135);
    }

    void setCalloutBox(CallOutBox* callout)
    {
        currentCallout = callout;
    }

    void closeCalloutBox()
    {
        MessageManager::callAsync([_callout = SafePointer(findParentComponentOfClass<CallOutBox>())]() {
            if (_callout) {
                _callout->dismiss();
            }
        });
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int const buttonHeight = buttonBounds.getHeight() / buttons.size();

        for (auto* button : buttons) {
            button->setBounds(buttonBounds.removeFromTop(buttonHeight));
        }
    }

    std::function<void(int)> updateModeIcon = [](int) { };

private:
    CallOutBox* currentCallout = nullptr;
    OwnedArray<TextButton> buttons = {
        new ModeButton(Icons::Edit, "Edit mode", false),
        new ModeButton(Icons::Lock, "Run mode", false),
        new ModeButton(Icons::Presentation, "Presentation mode", true),
        new ModeButton(Icons::PluginMode, "Plugin mode", true)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CanvasModePicker)
};

// New statusbar
Statusbar::Statusbar(PluginProcessor* processor, PluginEditor* e)
    : NVGComponent(this)
    , pd(processor)
    , editor(e)
{
    setCachedComponentImage(new NVGSurface::InvalidationListener(e->nvgSurface, this));

    zoomSelector = std::make_unique<ZoomLabel>(this);
    addAndMakeVisible(*zoomSelector);

    gridGroup = std::make_unique<StatusbarButtonGroup>(Icons::Magnet);
    addAndMakeVisible(*gridGroup);

    gridGroup->mainButton.setToggleState(
        SettingsFile::getInstance()->getPropertyAsValue("grid_enabled").getValue(),
        dontSendNotification);

    gridGroup->mainButton.onClick = [this] {
        SettingsFile::getInstance()->setProperty("grid_enabled", gridGroup->mainButton.getToggleState());
    };

    gridGroup->chevron.onClick = [this] {
        SnapSettings::show(editor, gridGroup->chevron.getScreenBounds());
    };

    overlayGroup = std::make_unique<StatusbarButtonGroup>(Icons::Eye);
    addAndMakeVisible(*overlayGroup);

    overlayGroup->mainButton.setToggleState(
        SettingsFile::getInstance()->getProperty<DynamicObject>("overlays")->getProperty("alt_mode"),
        dontSendNotification);

    overlayGroup->mainButton.onClick = [this] {
        SettingsFile::getInstance()->getProperty<DynamicObject>("overlays")->setProperty("alt_mode", overlayGroup->mainButton.getToggleState());
        SettingsFile::getInstance()->triggerSettingsChange("overlays");
    };

    overlayGroup->chevron.onClick = [this] {
        OverlayDisplaySettings::show(editor, overlayGroup->chevron.getScreenBounds());
    };

    editModeGroup = std::make_unique<StatusbarButtonGroup>(Icons::Edit);
    editModeGroup->mainButton.setClickingTogglesState(false);
    addAndMakeVisible(*editModeGroup);

    editModeGroup->mainButton.onClick = [this] {
        if (auto* cnv = editor->getCurrentCanvas()) {
            if (getValue<bool>(cnv->presentationMode)) {
                editModeGroup->mainButton.setButtonText(Icons::Edit);
                cnv->presentationMode = false;
            }
            cnv->locked = !getValue<bool>(cnv->locked);
            editModeGroup->mainButton.setButtonText(getValue<bool>(cnv->locked) ? Icons::Lock : Icons::Edit);
        }
    };

    editModeGroup->chevron.onClick = [this] {
        auto modePicker = std::make_unique<CanvasModePicker>(editor);
        modePicker->updateModeIcon = [this](int mode) {
            if (mode == 2) {
                editModeGroup->mainButton.setButtonText(Icons::Presentation);
            } else if (mode == 1) {
                editModeGroup->mainButton.setButtonText(Icons::Lock);
            } else {
                editModeGroup->mainButton.setButtonText(Icons::Edit);
            }
        };
        editor->showCalloutBox(std::move(modePicker), editModeGroup->chevron.getScreenBounds());
    };

    setSize(getWidth(), statusbarHeight);
}

Statusbar::~Statusbar() = default;

void Statusbar::handleAsyncUpdate()
{
    if (auto const* cnv = editor->getCurrentCanvas())
        currentZoomLevel = getValue<float>(cnv->zoomScale) * 100;
    else
        currentZoomLevel = 100.0f;
    repaint();
}

void Statusbar::setEditButtonState(bool locked, bool present)
{
    if (present) {
        editModeGroup->mainButton.setButtonText(Icons::Presentation);
    } else {
        editModeGroup->mainButton.setButtonText(locked ? Icons::Lock : Icons::Edit);
    }
}

void Statusbar::paint(Graphics& g)
{
    auto const b = getLocalBounds().reduced(5);
    StackShadow::drawShadowForRect(g, b.reduced(3.0f), 10, Corners::largeCornerRadius, 0.4f, 1);

    g.setColour(PlugDataColours::toolbarBackgroundColour);
    g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

    g.setColour(PlugDataColours::toolbarOutlineColour);
    g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);

    g.setColour(PlugDataColours::toolbarOutlineColour);

    // Separators between groups
    auto drawSep = [&](Component const& left) {
        auto const x = static_cast<float>(left.getRight() + 3);
        g.drawLine(x, 9.0f, x, getHeight() - 9.0f);
    };

    drawSep(*zoomSelector);
    drawSep(*overlayGroup);
}

void Statusbar::resized()
{
    if (welcomePanelIsShown)
        return;

    auto b = getLocalBounds().reduced(6, 0);
    constexpr int spacing = 10;

    zoomSelector->setBounds(b.removeFromLeft(55));
    b.removeFromLeft(spacing);

    gridGroup->setBounds(b.removeFromLeft(34));
    b.removeFromLeft(spacing);

    overlayGroup->setBounds(b.removeFromLeft(34));
    b.removeFromLeft(spacing);

    editModeGroup->setBounds(b.removeFromLeft(34));
}
