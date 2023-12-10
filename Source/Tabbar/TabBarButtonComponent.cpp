/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "TabBarButtonComponent.h"
#include "Tabbar.h"
#include "Dialogs/Dialogs.h"
#include "Utility/StackShadow.h"
#include "Utility/Fonts.h"

// #define ENABLE_TABBAR_DEBUGGING 1

class CloseTabButton : public SmallIconButton {

    using SmallIconButton::SmallIconButton;

    void paint(Graphics& g) override
    {
        auto font = Fonts::getIconFont().withHeight(12);
        g.setFont(font);

        if (!isEnabled()) {
            g.setColour(Colours::grey);
        } else if (getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
        } else if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f));
        } else {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
        }

        int const yIndent = jmin(4, proportionOfHeight(0.3f));
        int const cornerSize = jmin(getHeight(), getWidth()) / 2;

        int const fontHeight = roundToInt(font.getHeight() * 0.6f);
        int const leftIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
        int const rightIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
        int const textWidth = getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);
    }
};

TabBarButtonComponent::TabBarButtonComponent(TabComponent* tabbar, String const& name, TabbedButtonBar& bar)
    : TabBarButton(name, bar)
    , tabComponent(tabbar)
    , ghostTabAnimator(&dynamic_cast<ButtonBar*>(&bar)->ghostTabAnimator)
{
    ghostTabAnimator->addChangeListener(this);

    setTooltip(name);

    closeTabButton = std::make_unique<CloseTabButton>(Icons::Clear);
    closeTabButton->setColour(TextButton::buttonColourId, Colour());
    closeTabButton->setColour(TextButton::buttonOnColourId, Colour());
    closeTabButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
    closeTabButton->setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
    closeTabButton->setColour(ComboBox::outlineColourId, Colour());
    closeTabButton->setConnectedEdges(12);
    closeTabButton->setSize(28, 28);
    closeTabButton->addMouseListener(this, false);
    closeTabButton->onClick = [this]() mutable {
        closeTab();
    };

    addChildComponent(closeTabButton.get());
    updateCloseButtonState();
}

TabBarButtonComponent::~TabBarButtonComponent()
{
    closeTabButton->removeMouseListener(this);
    ghostTabAnimator->removeChangeListener(this);
}

void TabBarButtonComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == ghostTabAnimator) {
        if (!ghostTabAnimator->isAnimating() && closeButtonUpdatePending) {
            closeTabButton->setVisible(isMouseOver(true) || getToggleState());
            closeButtonUpdatePending = false;
        }
    }
}

void TabBarButtonComponent::updateCloseButtonState()
{
    if (!ghostTabAnimator->isAnimating()) {
        closeTabButton->setVisible(isMouseOver(true) || getToggleState());
    } else {
        closeButtonUpdatePending = true;
    }
}

void TabBarButtonComponent::closeTab()
{
    // We cant use the index from earlier because it might have changed!
    int const tabIdx = getIndex();
    auto* cnv = tabComponent->getCanvas(tabIdx);
    auto* editor = tabComponent->getEditor();

    if (tabIdx == -1)
        return;

    if (cnv) {
        MessageManager::callAsync([_cnv = SafePointer(cnv), _editor = SafePointer(editor)]() mutable {
            // Don't show save dialog, if patch is still open in another view
            if (_cnv && _cnv->patch.isDirty()) {
                Dialogs::showAskToSaveDialog(
                    &_editor->openedDialog, _editor, _cnv->patch.getTitle(),
                    [_cnv, _editor](int result) mutable {
                        if (!_cnv)
                            return;
                        if (result == 2)
                            _editor->saveProject([_cnv, _editor]() mutable { _editor->closeTab(_cnv); });
                        else if (result == 1)
                            _editor->closeTab(_cnv);
                    },
                    0, true);
            } else {
                _editor->closeTab(_cnv);
            }
        });
    }
}

void TabBarButtonComponent::setFocusForTabSplit()
{
    for (auto* split : getTabComponent()->getEditor()->splitView.splits) {
        if (split->getTabComponent() == getTabComponent()) {
            getTabComponent()->getEditor()->splitView.setFocus(split);
        }
    }
}

void TabBarButtonComponent::setTabText(String const& text)
{
    setTooltip(text);
    setButtonText(text);
}

TabComponent* TabBarButtonComponent::getTabComponent()
{
    return tabComponent;
}

void TabBarButtonComponent::mouseEnter(MouseEvent const& e)
{
    updateCloseButtonState();
    repaint();
}

void TabBarButtonComponent::mouseExit(MouseEvent const& e)
{
    updateCloseButtonState();
    repaint();
}

void TabBarButtonComponent::tabTextChanged(String const& newCurrentTabName)
{
}

void TabBarButtonComponent::lookAndFeelChanged()
{
    closeTabButton->setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
    closeTabButton->setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
}

void TabBarButtonComponent::resized()
{
    closeTabButton->setCentrePosition(getBounds().getCentre().withX(getBounds().getWidth() - 15).translated(0, 1));
}

ScaledImage TabBarButtonComponent::generateTabBarButtonImage()
{
    auto scale = 2.0f;
    // we calculate the best size for the tab DnD image
    auto text = getButtonText();
    Font font(Fonts::getDefaultFont());
    auto length = font.getStringWidth(getButtonText()) + 32;
    auto const boundsOffset = 10;

    // we need to expand the bounds, but reset the position to top left
    // then we offset the mouse drag by the same amount
    // this is to allow area for the shadow to render correctly
    auto textBounds = Rectangle<int>(0, 0, length, 28);
    auto bounds = textBounds.expanded(boundsOffset).withZeroOrigin();
    auto image = Image(Image::PixelFormat::ARGB, bounds.getWidth() * scale, bounds.getHeight() * scale, true);
    auto g = Graphics(image);
    g.addTransform(AffineTransform::scale(scale));
    Path path;
    path.addRoundedRectangle(bounds.reduced(14), 5.0f);
    StackShadow::renderDropShadow(g, path, Colour(0, 0, 0).withAlpha(0.3f), 6, { 0, 2 }, scale);
    g.setOpacity(1.0f);
    drawTabButton(g, textBounds.withPosition(10, 10));

    drawTabButtonText(g, textBounds.withPosition(3, 5));
    // g.drawImage(snapshot, bounds.toFloat(), RectanglePlacement::doNotResize | RectanglePlacement::centred);

#if ENABLE_TABBAR_DEBUGGING == 1
    g.setColour(Colours::red);
    g.drawRect(bounds.toFloat(), 1.0f);
#endif

    return ScaledImage(image, scale);
}

void TabBarButtonComponent::mouseDown(MouseEvent const& e)
{
    if (e.originalComponent != this)
        return;

    if (e.mods.isMiddleButtonDown()) {
        closeTab();
    }

    if (e.mods.isPopupMenu()) {
        PopupMenu tabMenu;

#if JUCE_MAC
        String revealTip = "Reveal in Finder";
#elif JUCE_WINDOWS
        String revealTip = "Reveal in Explorer";
#else
        String revealTip = "Reveal in file browser";
#endif

        auto* cnv = getTabComponent()->getCanvas(getIndex());
        if (!cnv)
            return;

        bool canReveal = cnv->patch.getCurrentFile().existsAsFile();

        tabMenu.addItem(revealTip, canReveal, false, [cnv]() {
            cnv->patch.getCurrentFile().revealToUser();
        });

        tabMenu.addSeparator();

        auto canSplitTab = cnv->editor->getSplitView()->splits.size() > 1 || getTabComponent()->getNumTabs() > 1;
        tabMenu.addItem("Split left", canSplitTab, false, [cnv]() {
            auto splitIdx = cnv->editor->splitView.getTabComponentSplitIndex(cnv->getTabbar());
            auto* currentSplit = cnv->editor->splitView.splits[splitIdx];
            auto* targetSplit = cnv->editor->splitView.splits[0];
            currentSplit->moveToSplit(targetSplit, cnv);
        });
        tabMenu.addItem("Split right", canSplitTab, false, [cnv]() {
            auto splitIdx = cnv->editor->splitView.getTabComponentSplitIndex(cnv->getTabbar());
            auto* currentSplit = cnv->editor->splitView.splits[splitIdx];
            auto* targetSplit = cnv->editor->splitView.splits.size() > 1 ? cnv->editor->splitView.splits[1] : nullptr;
            currentSplit->moveToSplit(targetSplit, cnv);
        });

        tabMenu.addSeparator();

        tabMenu.addItem("Close patch", true, false, [cnv]() {
            cnv->editor->closeTab(cnv);
        });

        tabMenu.addItem("Close all other patches", true, false, [cnv]() {
            cnv->editor->closeAllTabs(false, cnv);
        });

        tabMenu.addItem("Close all patches", true, false, [cnv]() {
            cnv->editor->closeAllTabs(false);
        });

        // Show the popup menu at the mouse position
        tabMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(150).withMaximumNumColumns(1).withParentComponent(getTabComponent()->getEditor()));
    } else if (e.mods.isLeftButtonDown()) {
        getTabComponent()->setCurrentTabIndex(getIndex());
    }
}

void TabBarButtonComponent::mouseDrag(MouseEvent const& e)
{
    if (e.getDistanceFromDragStart() > 10 && !isDragging) {
        isDragging = true;
        closeTabButton->setVisible(false);
        var tabIndex = getIndex();
        auto dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        tabImage = generateTabBarButtonImage();
        dragContainer->startDragging(tabIndex, this, tabImage, tabImage, true, nullptr);
    }
}

void TabBarButtonComponent::mouseUp(MouseEvent const& e)
{
    // we need to set visibility in the LNF due to using Juce overflow extra menu which uses visibility internally
    getProperties().set("dragged", var(false));
    isDragging = false;
}

// FIXME: we are only using this to draw the DnD tab image
void TabBarButtonComponent::drawTabButton(Graphics& g, Rectangle<int> customBounds)
{
    bool isActive = getToggleState();

    if (isActive) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
    } else if (isMouseOver(true)) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId).interpolatedWith(findColour(PlugDataColour::tabBackgroundColourId), 0.4f));
    } else {
        g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
    }

    auto bounds = getLocalBounds();

    if (!customBounds.isEmpty())
        bounds = customBounds;

    g.fillRoundedRectangle(bounds.reduced(4).toFloat(), Corners::defaultCornerRadius);
}

// FIXME: we are only using this to draw the DnD tab image
void TabBarButtonComponent::drawTabButtonText(Graphics& g, Rectangle<int> customBounds)
{
    auto bounds = getLocalBounds();
    if (!customBounds.isEmpty())
        bounds = customBounds;

    auto area = bounds.reduced(4, 2).toFloat();

    Font font(getLookAndFeel().getTabButtonFont(*this, area.getHeight()));
    font.setUnderline(hasKeyboardFocus(false));

    AffineTransform t = AffineTransform::translation(area.getX(), area.getY());

    g.setColour(findColour(PlugDataColour::tabTextColourId));
    g.setFont(font);
    g.addTransform(t);

    auto buttonText = getButtonText().trim();

    g.drawFittedText(buttonText,
        area.getX(), area.getY() - 2, (int)area.getWidth(), (int)area.getHeight(),
        Justification::centred,
        jmax(1, ((int)area.getHeight()) / 12));
}
