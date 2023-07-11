#include "TabBarButtonComponent.h"
#include "Tabbar.h"
#include "Dialogs/Dialogs.h"
#include "Utility/StackShadow.h"

TabBarButtonComponent::TabBarButtonComponent(TabComponent* tabComponent, String const& name, TabbedButtonBar& bar)
    : TabBarButton(name, bar)
    , tabComponent(tabComponent)
{
    setTriggeredOnMouseDown(true);

    setTooltip(name);

    closeTabButton.setButtonText(Icons::Clear);
    closeTabButton.getProperties().set("Style", "Icon");
    closeTabButton.getProperties().set("FontScale", 0.44f);
    closeTabButton.setColour(TextButton::buttonColourId, Colour());
    closeTabButton.setColour(TextButton::buttonOnColourId, Colour());
    closeTabButton.setColour(ComboBox::outlineColourId, Colour());
    closeTabButton.setConnectedEdges(12);
    closeTabButton.setSize(28, 28);
    closeTabButton.addMouseListener(this, false);
    closeTabButton.onClick = [this, tabComponent]() mutable {
        // We cant use the index from earlier because it might have changed!
        const int tabIdx = getIndex();
        auto* cnv = tabComponent->getCanvas(tabIdx);
        auto* editor = tabComponent->getEditor();

        if (tabIdx == -1)
            return;

        if (cnv) {
            MessageManager::callAsync([this, cnv = SafePointer(cnv), editor = SafePointer(editor)]() mutable {
                // Don't show save dialog, if patch is still open in another view
                if (cnv && cnv->patch.isDirty()) {
                    Dialogs::showSaveDialog(&editor->openedDialog, editor, cnv->patch.getTitle(),
                        [this, cnv, editor](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2)
                                editor->saveProject([this, cnv, editor]() mutable { editor->closeTab(cnv); });
                            else if (result == 1)
                                editor->closeTab(cnv);
                        });
                } else {
                    editor->closeTab(cnv);
                }
            });
        }
    };
    addChildComponent(closeTabButton);
}

void TabBarButtonComponent::setTabText(String const& text)
{
    setTooltip(text);
    setButtonText (text);
}

TabBarButtonComponent::~TabBarButtonComponent()
{
    closeTabButton.removeMouseListener(this);
}


TabComponent* TabBarButtonComponent::getTabComponent()
{
    return tabComponent;
}

void TabBarButtonComponent::mouseEnter(MouseEvent const& e)
{
    closeTabButton.setVisible(true);
    repaint();
}

void TabBarButtonComponent::mouseExit(MouseEvent const& e)
{
    closeTabButton.setVisible(false);
    repaint();
}

void TabBarButtonComponent::tabTextChanged(String const& newCurrentTabName)
{
    isDirty = true; 
}

void TabBarButtonComponent::lookAndFeelChanged()
{
    isDirty = true;
}

void TabBarButtonComponent::resized()
{
    closeTabButton.setCentrePosition(getBounds().getCentre().withX(getBounds().getWidth() - 15).translated(0, -1));
    isDirty = true;
}

Image TabBarButtonComponent::generateTabBarButtonImage()
{
    auto snapshot = createComponentSnapshot(getLocalBounds());

    // we need to expand the bounds, but reset the position to top left
    // then we offset the mouse drag by the same amount
    // this is to allow area for the shadow to render correctly
    auto bounds = getLocalBounds().expanded(boundsOffset).withPosition(0,0);
    auto image = Image(Image::PixelFormat::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    auto g = Graphics(image);
    Path path;
    path.addRoundedRectangle(bounds.reduced(14), 5.0f);
    StackShadow::renderDropShadow(g, path, Colour(0, 0, 0).withAlpha(0.3f), 6, { 0, 2 });
    g.setOpacity(1.0f);
    g.drawImage(snapshot, bounds.toFloat(), RectanglePlacement::doNotResize | RectanglePlacement::centred);

#if ENABLE_TABBAR_DEBUGGING == 1
    g.setColour(Colours::red);
    g.drawRect(bounds.toFloat(), 1.0f);
#endif

    return image;
}

void TabBarButtonComponent::mouseDrag(MouseEvent const& e)
{
    if(e.getDistanceFromDragStart() > 10) {
        setVisible(false);
        closeTabButton.setVisible(false);
        var tabIndex = getIndex();
        auto dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        if (isDirty) {
            tabImage = generateTabBarButtonImage();
            isDirty = false;
        }
    
        auto offset = e.getPosition() * -1 - Point<int>(boundsOffset,boundsOffset);
        dragContainer->startDragging(tabIndex, this, tabImage, true, &offset);
    }
}

void TabBarButtonComponent::mouseUp(MouseEvent const& e)
{
    setVisible(true);
}

void TabBarButtonComponent::drawTabButton(Graphics& g)
{
    bool isActive = getToggleState();

    if (isActive) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
    } else if (isMouseOver(true)) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId).interpolatedWith(findColour(PlugDataColour::tabBackgroundColourId), 0.4f));
    } else {
        g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
    }

    g.fillRoundedRectangle(getLocalBounds().reduced(4).toFloat(), Corners::defaultCornerRadius);
}

void TabBarButtonComponent::drawTabButtonText(Graphics& g)
{
    auto area = getLocalBounds().reduced(4, 2).toFloat();

    TabBarButton unusedButton("unused", getTabbedButtonBar());

    Font font(getLookAndFeel().getTabButtonFont(unusedButton, area.getHeight()));
    font.setUnderline(hasKeyboardFocus(false));

    AffineTransform t;

    switch (getTabbedButtonBar().getOrientation()) {
    case TabbedButtonBar::TabsAtLeft:
        t = t.rotated(MathConstants<float>::pi * -0.5f).translated(area.getX(), area.getBottom());
        break;
    case TabbedButtonBar::TabsAtRight:
        t = t.rotated(MathConstants<float>::pi * 0.5f).translated(area.getRight(), area.getY());
        break;
    case TabbedButtonBar::TabsAtTop:
    case TabbedButtonBar::TabsAtBottom:
        t = t.translated(area.getX(), area.getY());
        break;
    default:
        jassertfalse;
        break;
    }

    g.setColour(findColour(PlugDataColour::tabTextColourId));
    g.setFont(font);
    g.addTransform(t);

    auto buttonText = getButtonText().trim();

    auto textAreaWithCloseButton = area.withWidth(area.getWidth() - 26);
    if (font.getStringWidthFloat(buttonText) > textAreaWithCloseButton.getWidth()) {
        area = textAreaWithCloseButton;
    }
    if (font.getStringWidthFloat(buttonText) * 0.5f > textAreaWithCloseButton.getWidth()) {
        buttonText = buttonText.substring(0,10);
    }

    g.drawFittedText(buttonText,
        area.getX(), area.getY(), (int)area.getWidth(), (int)area.getHeight(),
        Justification::centred,
        jmax(1, ((int)area.getHeight()) / 12));
}

void TabBarButtonComponent::paint(Graphics& g)
{
    drawTabButton(g);
    drawTabButtonText(g);
}
