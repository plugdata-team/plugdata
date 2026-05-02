/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>

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

class StatusbarButtonGroup final : public Component {
public:
    explicit StatusbarButtonGroup(String const& iconText)
        : mainButton(iconText)
    {
        mainButton.setClickingTogglesState(true);
        addAndMakeVisible(mainButton);
        addAndMakeVisible(chevron);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        constexpr int chevronWidth = 16;
        mainButton.setBounds(b.removeFromLeft(b.getWidth() - chevronWidth));
        chevron.setBounds(b.expanded(2));
    }

    void setTooltip(String const& tooltip, String const& chevronTooltip)
    {
        mainButton.setTooltip(tooltip);
        chevron.setTooltip(chevronTooltip);
    }

    void setEnabled(bool shouldBeEnabled)
    {
        Component::setEnabled(shouldBeEnabled);
        mainButton.setEnabled(shouldBeEnabled);
        chevron.setEnabled(shouldBeEnabled);
    }

    SmallIconButton mainButton;
    SmallIconButton chevron = SmallIconButton(Icons::ThinDown);
};

class ZoomLabel final : public Component
    , public SettableTooltipClient {
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

    void setMenuTooltip(String const& tooltip)
    {
        menuButton.setTooltip(tooltip);
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
    explicit CanvasModePicker(PluginEditor* editor)
    {
        auto getKeyboardShortcutsForCommand = [editor](CommandID commandID) -> String {
            String shortcutKey;
            for (auto& keypress : editor->commandManager.getKeyMappings()->getKeyPressesAssignedToCommand(commandID)) {
                auto key = keypress.getTextDescriptionWithIcons();

                if (shortcutKey.isNotEmpty())
                    shortcutKey << ", ";

                if (key.length() == 1 && key[0] < 128)
                    shortcutKey << "shortcut: '" << key << '\'';
                else
                    shortcutKey << key;
            }
            return shortcutKey;
        };

        bool locked = false;
        if (auto* cnv = editor->getCurrentCanvas()) {
            locked = getValue<bool>(cnv->locked);
        }

        buttons = {
            new CalloutMenuButton(Icons::Edit, "Edit mode", false, locked ? getKeyboardShortcutsForCommand(CommandIDs::Lock) : ""),
            new CalloutMenuButton(Icons::Lock, "Run mode", false, locked ? "" : getKeyboardShortcutsForCommand(CommandIDs::Lock)),
            new CalloutMenuButton(Icons::Presentation, "Presentation mode", true, getKeyboardShortcutsForCommand(CommandIDs::TogglePresentationMode)),
            new CalloutMenuButton(Icons::PluginMode, "Plugin mode", true, getKeyboardShortcutsForCommand(CommandIDs::TogglePluginMode))
        };

        auto width = 0;
        for (auto* button : buttons) {
            width = std::max(width, button->getIdealWidth());
            addAndMakeVisible(*button);
        }

        buttons[0]->onClick = [this, editor]() mutable {
            if (auto* cnv = editor->getCurrentCanvas()) {
                cnv->locked = false;
                cnv->presentationMode = false;
            }
            updateModeIcon(0);
            closeCalloutBox();
        };
        buttons[1]->onClick = [this, editor]() mutable {
            if (auto* cnv = editor->getCurrentCanvas()) {
                cnv->locked = true;
                cnv->presentationMode = false;
            }
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

#if JUCE_MAC
        setSize(width + 52, 130);
#else
        setSize(width + 46, 130);
#endif
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
    OwnedArray<CalloutMenuButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CanvasModePicker)
};

Statusbar::Statusbar(PluginProcessor* processor, PluginEditor* e)
    : NVGComponent(this)
    , pd(processor)
    , editor(e)
{
    updateCachedRenderingMode();

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

    zoomSelector->setTooltip("Zoom level");
    zoomSelector->setMenuTooltip("Zoom options");
    gridGroup->setTooltip("Toggle grid", "Grid settings");
    overlayGroup->setTooltip("Toggle overlay alt-mode", "Overlay settings");
    editModeGroup->setTooltip("Toggle edit/run mode", "Other canvas modes");

    setSize(getWidth(), statusbarHeight);
}

Statusbar::~Statusbar() = default;

void Statusbar::updateCachedRenderingMode()
{
    bool const shouldUseCachedRendering = SettingsFile::getInstance()->getProperty<bool>("floating_panels");
    if (cachedRenderingEnabled == shouldUseCachedRendering)
        return;

    cachedRenderingEnabled = shouldUseCachedRendering;
    setCachedComponentImage(shouldUseCachedRendering ? new NVGSurface::InvalidationListener(editor->nvgSurface, this) : nullptr);
}

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
    if (editor->usesFloatingPanels()) {
        auto const b = getLocalBounds().reduced(5);
        StackShadow::drawShadowForRect(g, b.reduced(3.0f), 10, Corners::largeCornerRadius, 0.4f, 1);
        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);
        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }
    else {
        g.setColour(PlugDataColours::toolbarOutlineColour);
        auto outlineLeft = editor->leftSidebar->isHidden() ? editor->leftSidebar->getRight() - 1.0f : 0.0f;
        auto outlineRight = editor->rightSidebar->isHidden() ? editor->rightSidebar->getX() + 1.0f : getWidth();
        g.drawLine(outlineLeft, 0.5f, outlineRight, 0.5f);
    }

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
    updateCachedRenderingMode();

    bool const floatingPanels = SettingsFile::getInstance()->getProperty<bool>("floating_panels");
    auto b = floatingPanels ? getLocalBounds().reduced(6, 0) : getLocalBounds().withTrimmedLeft(6);
    constexpr int spacing = 10;

    zoomSelector->setBounds(b.removeFromLeft(55));
    b.removeFromLeft(spacing);

    gridGroup->setBounds(b.removeFromLeft(34));
    b.removeFromLeft(spacing);

    overlayGroup->setBounds(b.removeFromLeft(34));
    b.removeFromLeft(spacing);

    editModeGroup->setBounds(b.removeFromLeft(34));
}
