/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Constants.h"
#include "LookAndFeel.h"
#include "Utility/Fonts.h"
#include "Utility/OSUtils.h"
#include "Utility/SettingsFile.h"
#include "Dialogs/Dialogs.h"

struct OnboardingMockupColours {
    Colour canvasBg, sidebarBg, toolbarBg, outline, objectBackground, objectOutline, canvasText, accent;

    static OnboardingMockupColours fromCurrent(Component const& c)
    {
        return { PlugDataColours::canvasBackgroundColour,
                 PlugDataColours::sidebarBackgroundColour,
                 PlugDataColours::toolbarBackgroundColour,
                 PlugDataColours::outlineColour,
                 PlugDataColours::textObjectBackgroundColour,
                 PlugDataColours::objectOutlineColour,
                 PlugDataColours::canvasTextColour,
                 PlugDataColours::toolbarActiveColour };
    }

    static OnboardingMockupColours fromTheme(DynamicObject::Ptr tree)
    {
        return { PlugDataLook::getThemeColour(tree, PlugDataColour::canvasBackgroundColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::sidebarBackgroundColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::toolbarBackgroundColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::outlineColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::textObjectBackgroundColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::objectOutlineColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::canvasTextColourId),
                 PlugDataLook::getThemeColour(tree, PlugDataColour::toolbarActiveColourId) };
    }
};

static void drawDockedWindowMockup(Graphics& g, Rectangle<float> bounds,
                                   OnboardingMockupColours const& c, bool drawObject)
{
    auto window = bounds.reduced(6.0f);

    g.setColour(c.canvasBg);
    g.fillRoundedRectangle(window, Corners::defaultCornerRadius);

    auto titleBar = window.withHeight(14.0f);
    Path titleClip;
    titleClip.addRoundedRectangle(titleBar.getX(), titleBar.getY(),
                                  titleBar.getWidth(), titleBar.getHeight(),
                                  Corners::defaultCornerRadius, Corners::defaultCornerRadius, true, true, false, false);
    g.setColour(c.toolbarBg);
    g.fillPath(titleClip);
    for (int i = 0; i < 3; ++i) {
        g.setColour(c.canvasText.withAlpha(0.45f));
        g.fillEllipse(titleBar.getX() + 6 + i * 7, titleBar.getCentreY() - 2.5f, 5, 5);
    }

    auto inner = window.withTrimmedTop(14.0f);
    auto leftPanel = inner.removeFromLeft(12.0f);
    auto rightPanel = inner.removeFromRight(12.0f);
    auto bottomPanel = inner.removeFromBottom(12.0f);
    auto canvas = inner;

    g.setColour(c.sidebarBg);
    g.fillRect(leftPanel);
    g.fillRect(rightPanel);
    g.fillRect(bottomPanel);

    if (drawObject) {
        auto box = Rectangle<float>(canvas.getCentreX() - 24,
                                    canvas.getCentreY() - 9, 48, 18);
        g.setColour(c.objectBackground);
        g.fillRoundedRectangle(box, 2.0f);
        g.setColour(c.objectOutline);
        g.drawRoundedRectangle(box, 2.0f, 1.0f);
    }

    g.setColour(c.outline);
    g.drawRoundedRectangle(window, Corners::defaultCornerRadius, 1.0f);
    g.drawLine(leftPanel.getRight(), titleBar.getBottom(),
               leftPanel.getRight(), bottomPanel.getY(), 1.0f);
    g.drawLine(rightPanel.getX(), titleBar.getBottom(),
               rightPanel.getX(), bottomPanel.getY(), 1.0f);
    g.drawLine(bottomPanel.getX(), bottomPanel.getY(),
               bottomPanel.getRight(), bottomPanel.getY(), 1.0f);
    g.drawLine(titleBar.getX(), titleBar.getBottom(),
               titleBar.getRight(), titleBar.getBottom(), 1.0f);
}

static void drawFloatingWindowMockup(Graphics& g, Rectangle<float> bounds,
                                     OnboardingMockupColours const& c)
{
    auto window = bounds.reduced(6.0f);

    g.setColour(c.canvasBg);
    g.fillRoundedRectangle(window, Corners::defaultCornerRadius);

    auto titleBar = window.withHeight(14.0f);
    Path titleClip;
    titleClip.addRoundedRectangle(titleBar.getX(), titleBar.getY(),
                                  titleBar.getWidth(), titleBar.getHeight(),
                                  Corners::defaultCornerRadius, Corners::defaultCornerRadius, true, true, false, false);
    g.setColour(c.toolbarBg);
    g.fillPath(titleClip);
    for (int i = 0; i < 3; ++i) {
        g.setColour(c.canvasText.withAlpha(0.45f));
        g.fillEllipse(titleBar.getX() + 6 + i * 7, titleBar.getCentreY() - 2.5f, 5, 5);
    }

    auto inner = window.withTrimmedTop(14.0f);
    auto leftMini = inner.removeFromLeft(20).withSizeKeepingCentre(10, 32);
    auto rightMini = inner.removeFromRight(20).withSizeKeepingCentre(10, 32);
    auto bottomMini = inner.removeFromBottom(20).withSizeKeepingCentre(40, 10);

    for (auto r : { leftMini, rightMini, bottomMini }) {
        g.setColour(c.sidebarBg);
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(c.outline.withAlpha(0.85f));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }

    g.setColour(c.outline);
    g.drawRoundedRectangle(window, Corners::defaultCornerRadius, 1.0f);
    g.drawLine(titleBar.getX(), titleBar.getBottom(),
               titleBar.getRight(), titleBar.getBottom(), 1.0f);
}


class OnboardingPageIndicator final : public Component {
public:
    int numPages = 1;
    int currentPage = 0;

    void paint(Graphics& g) override
    {
        constexpr float dotSize = 7.0f;
        constexpr float spacing = 8.0f;

        auto const bounds = getLocalBounds().toFloat();
        float const totalWidth = numPages * dotSize + (numPages - 1) * spacing;
        float x = bounds.getCentreX() - totalWidth * 0.5f;
        float const y = bounds.getCentreY() - dotSize * 0.5f;

        auto const active = findColour(PlugDataColour::toolbarActiveColourId);
        auto const inactive = findColour(PlugDataColour::panelTextColourId).withAlpha(0.22f);

        for (int i = 0; i < numPages; ++i) {
            g.setColour(i == currentPage ? active : inactive);
            g.fillEllipse(x, y, dotSize, dotSize);
            x += dotSize + spacing;
        }
    }
};

class OnboardingCard : public Component {
public:
    std::function<void()> onClick;
    bool selected = false;

    OnboardingCard() { setMouseCursor(MouseCursor::PointingHandCursor); }

    void mouseEnter(MouseEvent const&) override { hovered = true; repaint(); }
    void mouseExit(MouseEvent const&) override { hovered = false; repaint(); }

    void mouseUp(MouseEvent const& e) override
    {
        if (!e.mouseWasDraggedSinceMouseDown() && onClick)
            onClick();
    }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().toFloat().reduced(3.0f);

        if (hovered || selected) {
            StackShadow::drawShadowForRect(g, bounds.toNearestInt().reduced(3), 9, Corners::largeCornerRadius, 0.32f);
        }

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillRoundedRectangle(bounds, Corners::largeCornerRadius);

        if (selected) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.drawRoundedRectangle(bounds, Corners::largeCornerRadius, 2.5f);
        } else {
            g.setColour(findColour(PlugDataColour::outlineColourId)
                            .withAlpha(hovered ? 1.0f : 0.55f));
            g.drawRoundedRectangle(bounds, Corners::largeCornerRadius, 1.0f);
        }

        paintCardContent(g, bounds.reduced(16.0f));

        if (selected) {
            auto check = Rectangle<float>(bounds.getRight() - 30,
                                          bounds.getY() + 12, 18, 18);
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillEllipse(check);
            g.setColour(Colours::white);
            Path tick;
            tick.startNewSubPath(check.getX() + 5, check.getCentreY() + 1);
            tick.lineTo(check.getCentreX() - 1, check.getBottom() - 5);
            tick.lineTo(check.getRight() - 4, check.getY() + 6);
            g.strokePath(tick, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    virtual void paintCardContent(Graphics& g, Rectangle<float> bounds) = 0;

private:
    bool hovered = false;
};

class OnboardingPage : public Component {
public:
    OnboardingPage(String t, String d)
        : title(std::move(t))
        , description(std::move(d))
    {
    }

    String getPageTitle() const { return title; }

    virtual void apply() { }
    virtual void enter() { }
    virtual bool shouldSkipRemaining() const { return false; }

    std::function<void()> onStateChanged;

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(48, 24);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(27.0f));
        g.drawFittedText(title, bounds.removeFromTop(34),
                         Justification::centred, 1);

        bounds.removeFromTop(8);

        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.75f));
        g.setFont(Fonts::getDefaultFont().withHeight(15.0f));
        g.drawFittedText(description, bounds.removeFromTop(50),
                         Justification::centredTop, 2, 1.0f);
    }

    Rectangle<int> getContentArea() const
    {
        return getLocalBounds().reduced(48, 24).withTrimmedTop(34 + 8 + 38 + 14);
    }

private:
    String title;
    String description;
};

class OnboardingWelcomePage final : public OnboardingPage {
public:
    OnboardingWelcomePage()
        : OnboardingPage("Welcome to plugdata",
                         "Let's take a moment to set things up the way you like. "
                         "You can change all of these later in the settings.")
    {
        logo = BinaryData::loadImage(BinaryData::plugdata_logo_png);
    }

    void paint(Graphics& g) override
    {
        OnboardingPage::paint(g);

        auto area = getContentArea();

        if (logo.isValid()) {
            int const targetH = jmin(150, area.getHeight() - 60);
            int const targetW = (int)(targetH * logo.getWidth() / (float)logo.getHeight());
            auto logoBounds = Rectangle<int>(0, 0, targetW, targetH)
                                  .withCentre(area.getCentre().translated(0, -32));
            g.drawImage(logo, logoBounds.toFloat(), RectanglePlacement::centred);
        }

        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.6f));
        g.setFont(Fonts::getDefaultFont().withHeight(15.0f));
        g.drawText("This will only take a minute.",
                   Rectangle<int>(area.getX(), area.getBottom() - 22,
                                  area.getWidth(), 22),
                   Justification::centred);
    }

private:
    Image logo;
};

class OnboardingUseCaseCard final : public OnboardingCard {
public:
    bool createMode;
    String headline;
    String subline;

    OnboardingUseCaseCard(bool create, String head, String sub)
        : createMode(create), headline(std::move(head)), subline(std::move(sub)) {}

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto labelArea = bounds.removeFromBottom(72.0f);
        auto iconArea = bounds.reduced(34.0f);

        auto textCol = findColour(PlugDataColour::panelTextColourId);
        auto centre = iconArea.getCentre();

        if (createMode) {
            Path pencil;
            pencil.startNewSubPath(centre.x - 24.0f, centre.y + 20.0f);
            pencil.lineTo(centre.x + 16.0f, centre.y - 20.0f);
            pencil.lineTo(centre.x + 26.0f, centre.y - 10.0f);
            pencil.lineTo(centre.x - 14.0f, centre.y + 30.0f);
            pencil.lineTo(centre.x - 28.0f, centre.y + 34.0f);
            pencil.closeSubPath();
            g.setColour(textCol);
            g.fillPath(pencil);
            g.setColour(textCol.withAlpha(0.35f));
            g.drawLine(centre.x - 30.0f, centre.y + 35.0f, centre.x + 30.0f, centre.y + 35.0f, 2.0f);
        } else {
            Path tri;
            tri.addTriangle(centre.x - 18.0f, centre.y - 30.0f,
                            centre.x - 18.0f, centre.y + 30.0f,
                            centre.x + 30.0f, centre.y);
            g.setColour(textCol);
            g.fillPath(tri);
        }

        auto inner = labelArea.reduced(10.0f, 4.0f);
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15.0f));
        g.drawText(headline, inner.removeFromTop(22).toNearestInt(),
                   Justification::centred);
        inner.removeFromTop(2);
        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.65f));
        g.setFont(Fonts::getDefaultFont().withHeight(14.0f));
        g.drawFittedText(subline, inner.toNearestInt(),
                         Justification::centredTop, 2);
    }
};

class OnboardingUseCasePage final : public OnboardingPage {
public:
    OnboardingUseCasePage()
        : OnboardingPage("How will you use plugdata?",
                         "If you're just here to load and run patches made by others, "
                         "we'll skip the setup and get you going. You can always tweak "
                         "things later from the settings.")
    {
        addAndMakeVisible(create);
        addAndMakeVisible(play);
        create.onClick = [this] { setMode(true); };
        play.onClick    = [this] { setMode(false); };
        setMode(true);
    }

    void resized() override
    {
        auto area = getContentArea().reduced(10);
        int const cardW = std::min(280, (area.getWidth() - 24) / 2);
        int const cardH = 190;
        int const totalW = cardW * 2 + 24;
        int const x = area.getCentreX() - totalW / 2;
        int const y = area.getCentreY() - cardH / 2;
        create.setBounds(x, y, cardW, 190);
        play.setBounds(x + cardW + 24, y, cardW, cardH);
    }

    bool shouldSkipRemaining() const override { return !creatorMode; }

private:
    void setMode(bool createPatches)
    {
        creatorMode = createPatches;
        create.selected = createPatches;
        play.selected   = !createPatches;
        create.repaint();
        play.repaint();
        if (onStateChanged) onStateChanged();
    }

    OnboardingUseCaseCard create { true,  "Create patches",
        "Create your own patches. We'll walk through a few preferences." };
    OnboardingUseCaseCard play   { false, "Just play patches",
        "Load and run patches made by others. No setup needed." };
    bool creatorMode = true;
};

class OnboardingThemeCard final : public OnboardingCard {
public:
    String themeName;

    OnboardingThemeCard(String name) : themeName(std::move(name)) {}

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto themeTree = SettingsFile::getInstance()->getTheme(themeName);
        if (themeTree == nullptr) return;

        auto labelArea = bounds.removeFromBottom(28.0f);
        auto preview = bounds;

        auto cols = OnboardingMockupColours::fromTheme(themeTree);
        drawDockedWindowMockup(g, preview, cols, /*drawObject=*/true);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15.0f));
        g.drawText(themeName, labelArea.toNearestInt(), Justification::centred);
    }
};

class OnboardingThemeGrid final : public Component {
public:
    OwnedArray<OnboardingThemeCard> cards;
    std::function<void(String const&)> onSelect;

    void rebuild(StringArray const& names, String const& selectedName)
    {
        cards.clear();
        for (auto const& name : names) {
            auto* card = cards.add(new OnboardingThemeCard(name));
            card->selected = (name == selectedName);
            card->onClick = [this, n = name] { if (onSelect) onSelect(n); };
            addAndMakeVisible(card);
        }
        resized();
    }

    void setSelected(String const& name)
    {
        for (auto* c : cards)
            c->selected = (c->themeName == name);
        repaint();
    }

    void resized() override
    {
        constexpr int cardW = 220;
        constexpr int cardH = 150;
        constexpr int gap = 12;
        int const y = jmax(0, (getHeight() - cardH) / 2);

        for (int i = 0; i < cards.size(); ++i) {
            cards[i]->setBounds(i * (cardW + gap),
                                y,
                                cardW, cardH);
        }
    }

    int getRequiredWidth() const
    {
        constexpr int cardW = 220;
        constexpr int gap = 12;
        return jmax(0, cards.size() * cardW + jmax(0, cards.size() - 1) * gap);
    }
};

class OnboardingThemePage final : public OnboardingPage {
public:
    OnboardingThemePage()
        : OnboardingPage("Pick a theme",
                         "Choose how plugdata should look. Light themes are easy on the "
                         "eyes during the day, dark themes shine in dim studios.")
    {
        grid.onSelect = [this](String const& name) {
            setSelectedTheme(name, true);
            grid.setSelected(name);
        };

        viewport.setViewedComponent(&grid, false);
        viewport.setScrollBarsShown(false, true);
        addAndMakeVisible(viewport);

        auto themes = PlugDataLook::getAllThemes();
        themes.removeString("light");
        themes.removeString("dark");
        themes.insert(0, "dark");
        themes.insert(0, "light");
        selectedTheme = SettingsFile::getInstance()->getProperty<String>("theme");
        selectedAltTheme = getAltThemeFor(selectedTheme);
        grid.rebuild(themes, selectedTheme);
    }

    void resized() override
    {
        auto area = getContentArea();
        viewport.setBounds(area.withSizeKeepingCentre(area.getWidth(), jmin(area.getHeight(), 178)));

        grid.setSize(grid.getRequiredWidth(), jmax(0, viewport.getHeight() - 10));
    }

    void apply() override
    {
        if (selectedTheme.isNotEmpty()) {
            SettingsFile::getInstance()->setProperty("theme", selectedTheme);
            saveActiveThemes();
        }
    }

private:
    static String getAltThemeFor(String const& name)
    {
        if (name == "classic")
            return "classic_dark";
        if (name == "classic_dark")
            return "classic";
        if (name == "light" || name == "warm")
            return "dark";
        return "light";
    }

    void setSelectedTheme(String const& name, bool livePreview)
    {
        selectedTheme = name;
        selectedAltTheme = getAltThemeFor(name);

        if (selectedAltTheme == selectedTheme)
            selectedAltTheme = selectedTheme == "light" ? "dark" : "light";

        saveActiveThemes();

        if (livePreview)
            SettingsFile::getInstance()->setProperty("theme", selectedTheme);
    }

    void saveActiveThemes() const
    {
        auto& activeThemes = SettingsFile::getInstance()->getProperty<VarArray>("active_themes");
        while (activeThemes.size() < 2)
            activeThemes.add(var());
        activeThemes.set(0, selectedTheme);
        activeThemes.set(1, selectedAltTheme);
        PlugDataLook::selectedThemes.set(0, selectedTheme);
        PlugDataLook::selectedThemes.set(1, selectedAltTheme);
    }

    Viewport viewport;
    OnboardingThemeGrid grid;
    String selectedTheme;
    String selectedAltTheme;
};

class OnboardingPanelChoiceCard final : public OnboardingCard {
public:
    bool floating;
    String label;

    OnboardingPanelChoiceCard(bool isFloating, String lbl)
        : floating(isFloating), label(std::move(lbl)) {}

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto labelArea = bounds.removeFromBottom(28.0f);
        auto cols = OnboardingMockupColours::fromCurrent(*this);
        if (floating) drawFloatingWindowMockup(g, bounds, cols);
        else          drawDockedWindowMockup(g, bounds, cols, /*drawObject=*/false);

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15.0f));
        g.drawText(label, labelArea.toNearestInt(), Justification::centred);
    }
};

class OnboardingPanelsPage final : public OnboardingPage {
public:
    OnboardingPanelsPage()
        : OnboardingPage("Panel layout",
                         "Choose your preferred UI layout.")
    {
        addAndMakeVisible(docked);
        addAndMakeVisible(floating);
        docked.onClick = [this] { setFloating(false); };
        floating.onClick = [this] { setFloating(true); };
        setFloating(SettingsFile::getInstance()->getProperty<int>("floating_panels") != 0);
    }

    void resized() override
    {
        auto area = getContentArea().reduced(10);
        int const cardW = std::min(260, (area.getWidth() - 24) / 2);
        int const cardH = std::min(area.getHeight(), 250);
        int const totalW = cardW * 2 + 24;
        int const x = area.getCentreX() - totalW / 2;
        int const y = area.getCentreY() - cardH / 2;
        docked.setBounds(x, y, cardW, cardH);
        floating.setBounds(x + cardW + 24, y, cardW, cardH);
    }

    void apply() override
    {
        SettingsFile::getInstance()->setProperty("floating_panels", var(useFloating));
    }

private:
    void setFloating(bool f)
    {
        useFloating = f;
        docked.selected = !f;
        floating.selected = f;
        docked.repaint();
        floating.repaint();
    }

    OnboardingPanelChoiceCard docked { false, "Docked" };
    OnboardingPanelChoiceCard floating { true, "Floating" };
    bool useFloating = false;
};

class OnboardingKeymapCard final : public OnboardingCard {
public:
    String name;
    StringArray shortcuts;

    OnboardingKeymapCard(String n, StringArray sc)
        : name(std::move(n)), shortcuts(std::move(sc)) {}

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto title = bounds.removeFromTop(28.0f);
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(16.0f));
        g.drawText(name, title.toNearestInt(), Justification::centredLeft);

        g.setColour(findColour(PlugDataColour::outlineColourId).withAlpha(0.6f));
        g.drawLine(bounds.getX(), bounds.getY() + 4,
                   bounds.getRight(), bounds.getY() + 4, 1.0f);
        bounds.removeFromTop(10);

        for (auto const& s : shortcuts) {
            auto parts = StringArray::fromTokens(s, "|", "");
            if (parts.size() != 2) continue;

            auto row = bounds.removeFromTop(28.0f);
            auto keyArea = row.removeFromRight(110.0f);

            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.85f));
            g.setFont(Fonts::getDefaultFont().withHeight(14.0f));
            g.drawText(parts[0].trim(), row.toNearestInt(), Justification::centredLeft);

            auto pill = keyArea.reduced(4.0f, 4.0f);
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(pill, 4.0f);
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(pill, 4.0f, 1.0f);
            g.setColour(findColour(PlugDataColour::panelTextColourId));
            g.setFont(Fonts::getDefaultFont().withHeight(13.0f));
            g.drawText(parts[1].trim(), pill.toNearestInt(), Justification::centred);
        }
    }
};

class OnboardingKeymapPage final : public OnboardingPage {
public:
    explicit OnboardingKeymapPage(PluginEditor* editor)
        : OnboardingPage("Keyboard shortcuts",
                         "Pick the shortcut style you're most comfortable with. ")
        , mappings(editor != nullptr ? editor->commandManager.getKeyMappings() : nullptr)
    {
        addAndMakeVisible(pdCard);
        addAndMakeVisible(maxCard);
        pdCard.onClick = [this] { setKeymap("pd"); };
        maxCard.onClick = [this] { setKeymap("max"); };

        setKeymap(hasMaxObjectShortcuts() ? "max" : "pd");
    }

    void resized() override
    {
        auto area = getContentArea().reduced(10);
        int const cardW = std::min(280, (area.getWidth() - 24) / 2);
        int const cardH = 190;
        int const totalW = cardW * 2 + 24;
        int const x = area.getCentreX() - totalW / 2;
        int const y = area.getCentreY() - cardH / 2;
        pdCard.setBounds(x, y, cardW, cardH);
        maxCard.setBounds(x + cardW + 24, y, cardW, cardH);
    }

    void apply() override
    {
        if (mappings == nullptr)
            return;

        if (selected == "max")
            applyMaxDefaults();
        else
            mappings->resetToDefaultMappings();

        SettingsFile::getInstance()->setKeyMap(mappings->createXml(true)->toString());
        mappings->sendChangeMessage();
    }

private:
    bool hasMaxObjectShortcuts() const
    {
        if (mappings == nullptr)
            return false;

        auto const keys = mappings->getKeyPressesAssignedToCommand(ObjectIDs::NewObject);
        return keys.contains(KeyPress(78, ModifierKeys::noModifiers, 'n'));
    }

    void applyMaxDefaults() const
    {
        mappings->resetToDefaultMappings();

        for (int i = ObjectIDs::NewObject; i < ObjectIDs::NumObjects; i++)
            mappings->clearAllKeyPresses(i);

        mappings->addKeyPress(ObjectIDs::NewObject, KeyPress(78, ModifierKeys::noModifiers, 'n'));
        mappings->addKeyPress(ObjectIDs::NewComment, KeyPress(67, ModifierKeys::noModifiers, 'c'));
        mappings->addKeyPress(ObjectIDs::NewBang, KeyPress(66, ModifierKeys::noModifiers, 'b'));
        mappings->addKeyPress(ObjectIDs::NewMessage, KeyPress(77, ModifierKeys::noModifiers, 'm'));
        mappings->addKeyPress(ObjectIDs::NewToggle, KeyPress(84, ModifierKeys::noModifiers, 't'));
        mappings->addKeyPress(ObjectIDs::NewNumbox, KeyPress(73, ModifierKeys::noModifiers, 'i'));
        mappings->addKeyPress(ObjectIDs::NewFloatAtom, KeyPress(70, ModifierKeys::noModifiers, 'f'));
        mappings->addKeyPress(ObjectIDs::NewVerticalSlider, KeyPress(83, ModifierKeys::noModifiers, 's'));
    }

    void setKeymap(String const& k)
    {
        selected = k;
        pdCard.selected = (k == "pd");
        maxCard.selected = (k == "max");
        pdCard.repaint();
        maxCard.repaint();
    }

    OnboardingKeymapCard pdCard {
        "Pure Data style",
        { "New object | " + shortcutModifier(ModifierKeys::commandModifier) + " + " + numberShortcut(1),
          "New message | " + shortcutModifier(ModifierKeys::commandModifier) + " + " + numberShortcut(2),
          "New number | " + shortcutModifier(ModifierKeys::commandModifier) + " + " + numberShortcut(3),
          "New slider | " + shortcutModifier(ModifierKeys::commandModifier) + " + " + shortcutModifier(ModifierKeys::shiftModifier) + " + V" }
    };

    OnboardingKeymapCard maxCard {
        "Max/MSP style",
        { "New object | N",
          "New message | M",
          "New number | I",
          "New slider | S" }
    };

    String selected = "pd";
    WeakReference<KeyPressMappingSet> mappings;

    static String shortcutModifier(ModifierKeys mod)
    {
        if (mod == ModifierKeys::commandModifier) {
#if JUCE_MAC
            return String::charToString(0x2318);
#else
            return "ctrl";
#endif
        }
        if (mod == ModifierKeys::shiftModifier)
            return "shift";

        return {};
    }

    static String numberShortcut(int number)
    {
        if (OSUtils::getKeyboardLayout() == OSUtils::KeyboardLayout::AZERTY) {
            if (number == 1)
                return "&";
            if (number == 2)
                return String::fromUTF8("é");
            if (number == 3)
                return "\"";
        }

        return String(number);
    }
};

class OnboardingPatchModeCard final : public OnboardingCard {
public:
    bool inWindows;
    String label;

    OnboardingPatchModeCard(bool windows, String lbl)
        : inWindows(windows), label(std::move(lbl)) {}

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto labelArea = bounds.removeFromBottom(26.0f);
        auto canvasBg = findColour(PlugDataColour::canvasBackgroundColourId);
        auto barBg = findColour(PlugDataColour::toolbarBackgroundColourId);
        auto outline = findColour(PlugDataColour::outlineColourId);
        auto accent = findColour(PlugDataColour::toolbarActiveColourId);

        if (!inWindows) {
            auto window = bounds.reduced(18.0f, 12.0f);
            g.setColour(canvasBg);
            g.fillRoundedRectangle(window, 6.0f);
            g.setColour(outline);
            g.drawRoundedRectangle(window, 6.0f, 1.0f);

            auto tabBar = window.removeFromTop(20.0f);
            g.setColour(barBg);
            Path clip;
            clip.addRoundedRectangle(tabBar.getX(), tabBar.getY(),
                                     tabBar.getWidth(), tabBar.getHeight(),
                                     6.0f, 6.0f, true, true, false, false);
            g.fillPath(clip);

            float tabW = tabBar.getWidth() / 3.0f;
            for (int i = 0; i < 3; ++i) {
                auto tab = Rectangle<float>(tabBar.getX() + i * tabW + 4,
                                            tabBar.getY() + 3,
                                            tabW - 8, tabBar.getHeight() - 4);
                g.setColour(i == 0 ? canvasBg : barBg.brighter(0.04f));
                g.fillRoundedRectangle(tab, 3.0f);
                if (i == 0) {
                    g.setColour(accent);
                    g.fillRect(Rectangle<float>(tab.getX(), tab.getBottom() - 2, tab.getWidth(), 2));
                }
            }
            g.setColour(outline);
            g.drawLine(tabBar.getX(), tabBar.getBottom(),
                       tabBar.getRight(), tabBar.getBottom(), 1.0f);
        } else {
            for (int i = 2; i >= 0; --i) {
                auto w = bounds.reduced(6.0f).translated(i * 8.0f - 8.0f, i * 8.0f - 8.0f);
                w = w.withWidth(w.getWidth() * 0.78f).withHeight(w.getHeight() * 0.7f);
                g.setColour(canvasBg);
                g.fillRoundedRectangle(w, 5.0f);
                g.setColour(outline);
                g.drawRoundedRectangle(w, 5.0f, 1.0f);
                auto tb = w.removeFromTop(12.0f);
                g.setColour(barBg);
                Path tbClip;
                tbClip.addRoundedRectangle(tb.getX(), tb.getY(),
                                           tb.getWidth(), tb.getHeight(),
                                           5.0f, 5.0f, true, true, false, false);
                g.fillPath(tbClip);
                for (int d = 0; d < 3; ++d) {
                    g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(0.4f));
                    g.fillEllipse(tb.getX() + 4 + d * 6, tb.getCentreY() - 2, 4, 4);
                }
            }
        }

        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.setFont(Fonts::getSemiBoldFont().withHeight(15.0f));
        g.drawText(label, labelArea.toNearestInt(), Justification::centred);
    }
};

class OnboardingToggleRow final : public Component {
public:
    explicit OnboardingToggleRow(String rowLabel)
        : label(std::move(rowLabel))
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }

    bool getToggleState() const { return selected; }

    void setToggleState(bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    void mouseEnter(MouseEvent const&) override { hovered = true; repaint(); }
    void mouseExit(MouseEvent const&) override { hovered = false; repaint(); }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        selected = !selected;
        repaint();
        if (onClick)
            onClick();
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        auto const bg = findColour(PlugDataColour::panelForegroundColourId);
        auto const outline = findColour(PlugDataColour::outlineColourId);
        auto const text = findColour(PlugDataColour::panelTextColourId);
        auto const accent = findColour(PlugDataColour::toolbarActiveColourId);

        g.setColour(bg.contrasting(hovered ? 0.04f : 0.02f));
        g.fillRoundedRectangle(bounds, 7.0f);
        g.setColour(outline.withAlpha(hovered ? 0.9f : 0.55f));
        g.drawRoundedRectangle(bounds, 7.0f, 1.0f);

        auto textArea = bounds.toNearestInt().reduced(12, 0);
        textArea.removeFromRight(62);
        g.setColour(text);
        g.setFont(Fonts::getSemiBoldFont().withHeight(13.0f));
        g.drawText(label, textArea, Justification::centredLeft);

        auto track = Rectangle<float>(0, 0, 44, 22)
                         .withCentre({ bounds.getRight() - 34.0f, bounds.getCentreY() });
        g.setColour(selected ? accent : findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(track, track.getHeight() * 0.5f);
        g.setColour(selected ? accent.brighter(0.2f) : outline);
        g.drawRoundedRectangle(track, track.getHeight() * 0.5f, 1.0f);

        auto knob = Rectangle<float>(0, 0, 16, 16)
                        .withCentre({ selected ? track.getRight() - 11.0f : track.getX() + 11.0f,
                                      track.getCentreY() });
        g.setColour(selected ? Colours::white : text.withAlpha(0.75f));
        g.fillEllipse(knob);
    }

    std::function<void()> onClick;

private:
    String label;
    bool selected = false;
    bool hovered = false;
};

class OnboardingDisplayPage final : public OnboardingPage {
public:
    OnboardingDisplayPage()
        : OnboardingPage("Miscellaneous",
                         "Choose how patches open, how big the interface is, and whether "
                         "controls should be optimised for touch.")
    {
        addAndMakeVisible(tabsCard);
        addAndMakeVisible(windowsCard);
        tabsCard.onClick    = [this] { setPatchMode(false); };
        windowsCard.onClick = [this] { setPatchMode(true); };
        setPatchMode(SettingsFile::getInstance()->getProperty<int>("open_patches_in_window") != 0);

        addAndMakeVisible(scaleLabel);
        scaleLabel.setText("Interface scale", dontSendNotification);
        scaleLabel.setFont(Fonts::getSemiBoldFont().withHeight(13.0f));
        scaleLabel.setJustificationType(Justification::centredLeft);

        addAndMakeVisible(scaleSlider);
        scaleSlider.setRange(0.5, 2.0, 0.125);
        scaleSlider.setValue(SettingsFile::getInstance()->getProperty<float>("global_scale"),
                             dontSendNotification);
        scaleSlider.setSliderStyle(Slider::LinearHorizontal);
        scaleSlider.setTextBoxStyle(Slider::TextBoxRight, false, 60, 22);
        scaleSlider.textFromValueFunction = [](double v) {
            return String((int)std::round(v * 100.0)) + "%";
        };
        scaleSlider.valueFromTextFunction = [](String const& t) {
            return t.removeCharacters("% ").getDoubleValue() / 100.0;
        };
        scaleSlider.updateText();

        addAndMakeVisible(touchToggle);
        touchToggle.setToggleState(SettingsFile::getInstance()->getProperty<int>("touch_mode") != 0);
    }

    void resized() override
    {
        auto area = getContentArea().reduced(28, 0);

        auto cardRow = area.removeFromTop(jmin(96, jmax(78, area.getHeight() / 2)));
        constexpr int gap = 12;
        int const cardW = (cardRow.getWidth() - gap) / 2;
        tabsCard.setBounds(cardRow.removeFromLeft(cardW));
        cardRow.removeFromLeft(gap);
        windowsCard.setBounds(cardRow);

        area.removeFromTop(10);

        scaleLabel.setBounds(area.removeFromTop(18));
        scaleSlider.setBounds(area.removeFromTop(24));

        area.removeFromTop(8);

        touchToggle.setBounds(area.removeFromTop(32));
    }

    void apply() override
    {
        SettingsFile::getInstance()->setProperty("open_patches_in_window", var(inWindows));
        SettingsFile::getInstance()->setGlobalScale((float)scaleSlider.getValue());
        SettingsFile::getInstance()->setProperty("touch_mode",
                                                 var(touchToggle.getToggleState()));
    }

private:
    void setPatchMode(bool useWindows)
    {
        inWindows = useWindows;
        tabsCard.selected = !useWindows;
        windowsCard.selected = useWindows;
        tabsCard.repaint();
        windowsCard.repaint();
    }

    OnboardingPatchModeCard tabsCard    { false, "Open patches in tabs" };
    OnboardingPatchModeCard windowsCard { true,  "Open patches in new windows" };
    bool inWindows = false;

    Label scaleLabel;
    Slider scaleSlider;
    OnboardingToggleRow touchToggle { "Enable touch mode" };
};

class OnboardingLinkCard final : public OnboardingCard {
public:
    enum class Icon { Docs, Discord, Github };

    OnboardingLinkCard(String text, Icon iconToDraw, String targetUrl)
        : label(std::move(text))
        , icon(iconToDraw)
        , url(std::move(targetUrl))
    {
        onClick = [this] { URL(url).launchInDefaultBrowser(); };
    }

    void paintCardContent(Graphics& g, Rectangle<float> bounds) override
    {
        auto labelArea = bounds.removeFromBottom(38.0f);
        auto iconArea = bounds.reduced(18.0f, 10.0f);
        auto const accent = findColour(PlugDataColour::toolbarActiveColourId);
        auto const text = findColour(PlugDataColour::panelTextColourId);

        g.setColour(accent);

        if (icon == Icon::Docs) {
            auto page = Rectangle<float>(0, 0, 46, 58).withCentre(iconArea.getCentre());
            g.drawRoundedRectangle(page, 4.0f, 2.0f);
            g.drawLine(page.getX() + 10.0f, page.getY() + 18.0f,
                       page.getRight() - 10.0f, page.getY() + 18.0f, 2.0f);
            g.drawLine(page.getX() + 10.0f, page.getY() + 30.0f,
                       page.getRight() - 10.0f, page.getY() + 30.0f, 2.0f);
            g.drawLine(page.getX() + 10.0f, page.getY() + 42.0f,
                       page.getRight() - 18.0f, page.getY() + 42.0f, 2.0f);
        } else if (icon == Icon::Discord) {
            auto bubble = Rectangle<float>(0, 0, 58, 46).withCentre(iconArea.getCentre());
            g.drawRoundedRectangle(bubble, 10.0f, 2.0f);
            g.fillEllipse(bubble.getX() + 16.0f, bubble.getCentreY() - 4.0f, 7.0f, 7.0f);
            g.fillEllipse(bubble.getRight() - 23.0f, bubble.getCentreY() - 4.0f, 7.0f, 7.0f);
            g.drawLine(bubble.getX() + 18.0f, bubble.getBottom() - 11.0f,
                       bubble.getRight() - 18.0f, bubble.getBottom() - 11.0f, 2.0f);
        } else {
            auto centre = iconArea.getCentre();
            g.drawEllipse(Rectangle<float>(0, 0, 54, 54).withCentre(centre), 2.0f);
            g.drawLine(centre.x, centre.y + 16.0f, centre.x, centre.y - 6.0f, 2.0f);
            g.drawLine(centre.x, centre.y - 6.0f, centre.x - 12.0f, centre.y - 14.0f, 2.0f);
            g.drawLine(centre.x, centre.y - 6.0f, centre.x + 12.0f, centre.y - 14.0f, 2.0f);
            g.fillEllipse(centre.x - 4.0f, centre.y + 12.0f, 8.0f, 8.0f);
            g.fillEllipse(centre.x - 16.0f, centre.y - 18.0f, 8.0f, 8.0f);
            g.fillEllipse(centre.x + 8.0f, centre.y - 18.0f, 8.0f, 8.0f);
        }

        g.setColour(text);
        g.setFont(Fonts::getSemiBoldFont().withHeight(14.0f));
        g.drawFittedText(label, labelArea.toNearestInt(), Justification::centred, 2);
    }

private:
    String label;
    Icon icon;
    String url;
};

class OnboardingDocsPage final : public OnboardingPage {
public:
    OnboardingDocsPage()
        : OnboardingPage("You're all set",
                         "If you ever feel stuck, these resources are a good place to look. "
                         "You can change every choice you made here in plugdata's settings.")
        , docs("View documentation", OnboardingLinkCard::Icon::Docs, "https://plugdata.org/documentation")
        , discord("Join our Discord", OnboardingLinkCard::Icon::Discord, "https://discord.gg/eT2RxdF9Nq")
        , github("View on GitHub", OnboardingLinkCard::Icon::Github, "https://github.com/plugdata-team/plugdata")
    {
        addAndMakeVisible(docs);
        addAndMakeVisible(discord);
        addAndMakeVisible(github);
    }

    void resized() override
    {
        auto area = getContentArea().reduced(8, 4);
        constexpr int gap = 14;
        int const cardW = jmin(150, (area.getWidth() - 2 * gap) / 3);
        int const cardH = jmin(150, area.getHeight());
        int const totalW = cardW * 3 + gap * 2;
        int const x = area.getCentreX() - totalW / 2;
        int const y = area.getCentreY() - cardH / 2;

        docs.setBounds(x, y, cardW, cardH);
        discord.setBounds(x + cardW + gap, y, cardW, cardH);
        github.setBounds(x + (cardW + gap) * 2, y, cardW, cardH);
    }

private:
    OnboardingLinkCard docs;
    OnboardingLinkCard discord;
    OnboardingLinkCard github;
};

class OnboardingDialog final : public Component {
public:
    std::function<void()> onClose;

    explicit OnboardingDialog(Dialog* parent, PluginEditor* editor)
        : parentDialog(parent)
    {
        pages.add(new OnboardingWelcomePage());
        pages.add(new OnboardingUseCasePage());
        pages.add(new OnboardingThemePage());
        pages.add(new OnboardingPanelsPage());
        pages.add(new OnboardingKeymapPage(editor));
        pages.add(new OnboardingDisplayPage());
        pages.add(new OnboardingDocsPage());

        for (auto* p : pages) {
            addChildComponent(p);
            p->onStateChanged = [this] { updateNavButtons(); };
        }

        addAndMakeVisible(indicator);
        indicator.numPages = pages.size();

        styleButton(skipButton, "Skip & use defaults");
        skipButton.onClick = [this] { close(); };
        addAndMakeVisible(skipButton);

        styleButton(backButton, "Back");
        backButton.onClick = [this] { goTo(currentPage - 1); };
        addAndMakeVisible(backButton);

        styleButton(nextButton, "Next");
        nextButton.onClick = [this] {
            pages[currentPage]->apply();
            if (pages[currentPage]->shouldSkipRemaining()
                || currentPage == pages.size() - 1) {
                if(pages[currentPage]->shouldSkipRemaining()) {
                    SettingsFile::getInstance()->setProperty("last_welcome_panel", var(1));
                }
                close();
            }
            else {
                goTo(currentPage + 1);
            }
        };
        addAndMakeVisible(nextButton);

        goTo(0);
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto const titlebarBounds = getLocalBounds().removeFromTop(toolbarHeight).toFloat();
        Path tb;
        tb.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(tb);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(toolbarHeight, 0.0f, (float)getWidth());

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(getHeight() - footerHeight, 0.0f, (float)getWidth());

        Fonts::drawStyledText(g, "Onboarding", Rectangle<float>(0.0f, 4.0f, (float)getWidth(), (float)toolbarHeight - 8.0f), PlugDataColours::panelTextColour, Semibold, 15, Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(toolbarHeight);
        auto footer = bounds.removeFromBottom(footerHeight);

        for (auto* p : pages)
            p->setBounds(bounds);

        constexpr int btnW = 84;
        constexpr int btnH = 26;
        constexpr int sidePad = 16;
        constexpr int skipW = 160;

        auto leftZone = footer.removeFromLeft(footer.getWidth() / 3);
        auto rightZone = footer.removeFromRight(footer.getWidth() / 2);

        skipButton.setBounds(leftZone.withSizeKeepingCentre(skipW, btnH).withX(leftZone.getX() + sidePad));
        backButton.setBounds(leftZone.withSizeKeepingCentre(btnW, btnH).withX(leftZone.getX() + sidePad));

        indicator.setBounds(footer.withSizeKeepingCentre(footer.getWidth(), 16));

        auto right = rightZone.withTrimmedRight(sidePad);
        nextButton.setBounds(right.removeFromRight(btnW).withSizeKeepingCentre(btnW, btnH));
    }

private:

    void lookAndFeelChanged() override
    {
        styleButton(skipButton);
        styleButton(backButton);
        styleButton(nextButton);
    }

    void styleButton(TextButton& b, String const& text = "")
    {
        auto const backgroundColour = PlugDataColours::panelBackgroundColour;
        b.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        b.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.10f));
        b.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        if(text.isNotEmpty()) b.setButtonText(text);
    }

    void goTo(int newPage)
    {
        newPage = jlimit(0, pages.size() - 1, newPage);
        for (int i = 0; i < pages.size(); ++i)
            pages[i]->setVisible(i == newPage);

        currentPage = newPage;
        indicator.currentPage = newPage;
        indicator.repaint();

        pages[currentPage]->enter();
        updateNavButtons();
        repaint();
    }

    void updateNavButtons()
    {
        bool const isLast = currentPage == pages.size() - 1;
        bool const isSkipPath = pages[currentPage]->shouldSkipRemaining();

        skipButton.setVisible(currentPage == 0);
        backButton.setVisible(currentPage > 0);
        nextButton.setButtonText(isLast || isSkipPath ? "Get started" : "Next");
    }

    void close()
    {
        SettingsFile::getInstance()->setProperty("onboarding_completed", var(true));
        SettingsFile::getInstance()->saveSettings();
        if (onClose)
            onClose();
        else if (parentDialog != nullptr)
            parentDialog->closeDialog();
    }

    Dialog* parentDialog = nullptr;
    OwnedArray<OnboardingPage> pages;
    int currentPage = 0;

    OnboardingPageIndicator indicator;
    TextButton skipButton, backButton, nextButton;

    static constexpr int toolbarHeight = 40;
    static constexpr int footerHeight = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OnboardingDialog)
};
