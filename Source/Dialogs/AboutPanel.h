/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

extern "C" {
EXTERN char* pd_version;
}

class AboutPanel final : public Component {
    WidePanelButton viewWebsite = WidePanelButton(Icons::OpenLink);
    WidePanelButton viewOnGithub = WidePanelButton(Icons::OpenLink);
    WidePanelButton reportIssue = WidePanelButton(Icons::OpenLink);
    WidePanelButton sponsor = WidePanelButton(Icons::OpenLink);

    WidePanelButton showCredits = WidePanelButton(Icons::Forward, 15);
    WidePanelButton showLicense = WidePanelButton(Icons::Forward, 15);

    class CreditsViewport final : public BouncingViewport {
        void paint(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);
        }

        void paintOverChildren(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().removeFromTop(16).withTrimmedRight(8).toFloat(), Corners::windowCornerRadius);

            // Draw fade for credits viewport
            g.setGradientFill(ColourGradient::vertical(findColour(PlugDataColour::panelBackgroundColourId), 36, findColour(PlugDataColour::panelBackgroundColourId).withAlpha(0.0f), 48));
            g.fillRect(0, 16, getWidth() - 16, 48);
        }
    };
    class CreditsPanel final : public Component {
        HeapArray<std::pair<String, String>> const contributors = {
            { "Timothy Schoen", "Lead development, UI/UX design" },
            { "Alex Mitchell", "Development, UI/UX design" },
            { "Joshua A.C. Newman", "Community management, logo and identity design" },
            { "Bas de Bruin", "Logo execution" },
            { "Alexandre Porres", "ELSE and cyclone development" },
            { "dreamer", "Hvcc development" },
            { "tomara-x", "Documentation, testing" }
        };

        StringArray const corporateSponsors = {
            "Deskew Technologies",
            "Jet Brains"
        };

        StringArray const sponsors = {
            "Nasko",
            "polarity",
            "ludnny",
            "Joshua A.C. Newman",
            "seppog",
            "yuichkun",
            "Ewan Bristow",
            "meownoid",
            "Ben Wesch",
            "tobenaibousi",
            "kilon",
            "motekulo",
            "pyrodogg",
            "rgreset",
            "NothanUmber",
            "alexandrotrevino",
            "anxefaraldo",
            "Enkerli",
            "Epic233-officiale",
            "jamescorrea",
            "KPY7030P",
            "Damian",
            "Dan Friedman",
            "el mono",
            "igozbeubzbeub",
            "Jonathan",
            "Steven Donahue",
            "vasilymilovidov",
            "duddex"
        };

    public:
        CreditsPanel() = default;

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().withTrimmedTop(46).reduced(16, 4);

            Fonts::drawStyledText(g, "Contributors", bounds.getX(), bounds.getY() - 8, bounds.getWidth(), 15.0f, findColour(PlugDataColour::panelTextColourId), Semibold, 15.0f);

            bounds.removeFromTop(16);

            Path firstShadowPath;
            firstShadowPath.addRoundedRectangle(Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth(), contributors.size() * 48).reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(hash("contributors"), g, firstShadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);

            for (int i = 0; i < contributors.size(); i++) {
                auto rowBounds = bounds.removeFromTop(48);
                auto first = i == 0;
                auto last = i == contributors.size() - 1;
                auto& [name, role] = contributors[i];
                Path outline;
                outline.addRoundedRectangle(rowBounds.getX(), rowBounds.getY(), rowBounds.getWidth(), rowBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, first, first, last, last);

                g.setColour(findColour(PlugDataColour::panelForegroundColourId));
                g.fillPath(outline);

                g.setColour(findColour(PlugDataColour::outlineColourId));
                g.strokePath(outline, PathStrokeType(1));

                Fonts::drawText(g, name, rowBounds.reduced(12, 2).translated(0, -8), findColour(PlugDataColour::panelTextColourId), 15);
                Fonts::drawText(g, role, rowBounds.reduced(12, 2).translated(0, 8), findColour(PlugDataColour::panelTextColourId).withAlpha(0.5f), 15);
            }

            bounds.removeFromTop(24);

            Fonts::drawStyledText(g, "Corporate sponsors", bounds.getX(), bounds.getY() - 8, bounds.getWidth(), 15.0f, findColour(PlugDataColour::panelTextColourId), Semibold, 15.0f);

            bounds.removeFromTop(16);

            Path secondShadowPath;
            secondShadowPath.addRoundedRectangle(Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth(), corporateSponsors.size() * 32).reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(hash("corporate_credits_panel"), g, secondShadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);
            for (int i = 0; i < corporateSponsors.size(); i++) {
                auto rowBounds = bounds.removeFromTop(36);
                auto first = i == 0;
                auto last = i == corporateSponsors.size() - 1;
                auto& name = corporateSponsors[i];
                Path outline;
                outline.addRoundedRectangle(rowBounds.getX(), rowBounds.getY(), rowBounds.getWidth(), rowBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, first, first, last, last);

                g.setColour(findColour(PlugDataColour::panelForegroundColourId));
                g.fillPath(outline);

                g.setColour(findColour(PlugDataColour::outlineColourId));
                g.strokePath(outline, PathStrokeType(1));

                Fonts::drawText(g, name, rowBounds.reduced(12, 2), findColour(PlugDataColour::panelTextColourId), 15);

                jassert(!bounds.isEmpty());
            }

            bounds.removeFromTop(24);

            Fonts::drawStyledText(g, "Sponsors", bounds.getX(), bounds.getY() - 8, bounds.getWidth(), 15.0f, findColour(PlugDataColour::panelTextColourId), Semibold, 15.0f);

            bounds.removeFromTop(16);

            Path thirdShadowPath;
            thirdShadowPath.addRoundedRectangle(Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth(), sponsors.size() * 32).reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(hash("credits_panel"), g, thirdShadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);
            for (int i = 0; i < sponsors.size(); i++) {
                auto rowBounds = bounds.removeFromTop(36);
                auto first = i == 0;
                auto last = i == sponsors.size() - 1;
                auto& name = sponsors[i];
                Path outline;
                outline.addRoundedRectangle(rowBounds.getX(), rowBounds.getY(), rowBounds.getWidth(), rowBounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, first, first, last, last);

                g.setColour(findColour(PlugDataColour::panelForegroundColourId));
                g.fillPath(outline);

                g.setColour(findColour(PlugDataColour::outlineColourId));
                g.strokePath(outline, PathStrokeType(1));

                Fonts::drawText(g, name, rowBounds.reduced(12, 2), findColour(PlugDataColour::panelTextColourId), 15);

                jassert(!bounds.isEmpty());
            }
        }

        int getDesiredHeight() const
        {
            return (sponsors.size() + corporateSponsors.size() + 1) * 36;
        }
    };

    class LicensePanel final : public Component {
        TextEditor license;

        static inline String const licenseText = String::fromUTF8("Copyright \xc2\xa9 Timothy Schoen\n"
        "\n"
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU Affero General Public License (AGPL-3.0) as published by the Free Software Foundation.\n"
        "\n"
        "Portions of this program originating from the plugdata repository (excluding third-party submodules) are licensed under the GNU General Public License (GPL-3.0) by their respective contributors. When combined with software licensed under the AGPL-3.0 (including the JUCE framework), the resulting work is distributed under the terms of the AGPL-3.0.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");
        
    public:
        LicensePanel() = default;

        void visibilityChanged() override
        {
            if (!license.isVisible()) {
                license.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                license.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
                license.setReadOnly(true);
                license.setMultiLine(true);
                license.setText(licenseText);
                license.setFont(Font(FontOptions(15)));
                license.setLineSpacing(1.0f);
                addAndMakeVisible(license);
            }
        }

        void resized() override
        {
            license.setBounds(getLocalBounds().withTrimmedTop(32).reduced(16, 4));
        }

        void paint(Graphics& g) override
        {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);
        }
    };

    CreditsViewport creditsViewport;
    CreditsPanel creditsComponent;
    LicensePanel licenseComponent;

    MainToolbarButton backButton;
    
    Image logo = BinaryData::loadImage(BinaryData::plugdata_large_logo_png);

public:
    AboutPanel()
    {
        viewWebsite.setButtonText("Website");
        viewOnGithub.setButtonText("View on Github");
        reportIssue.setButtonText("Report issue");
        sponsor.setButtonText("Sponsor");

        showCredits.setButtonText("Credits");
        showLicense.setButtonText("License");

        addAndMakeVisible(viewWebsite);
        addAndMakeVisible(viewOnGithub);
        addAndMakeVisible(reportIssue);
        addAndMakeVisible(sponsor);

        viewWebsite.setConnectedEdges(Button::ConnectedOnBottom);
        viewOnGithub.setConnectedEdges(Button::ConnectedOnTop);

        viewWebsite.onClick = [] {
            URL("https://plugdata.org").launchInDefaultBrowser();
        };

        viewOnGithub.onClick = [] {
            URL("https://github.com/plugdata-team/plugdata").launchInDefaultBrowser();
        };

        reportIssue.onClick = [] {
            URL("https://github.com/plugdata-team/plugdata/issues").launchInDefaultBrowser();
        };

        sponsor.onClick = [] {
            URL("https://github.com/sponsors/timothyschoen").launchInDefaultBrowser();
        };

        backButton.setButtonText(Icons::Back);
        backButton.onClick = [this] {
            creditsViewport.setVisible(false);
            licenseComponent.setVisible(false);
            backButton.setVisible(false);
        };
        backButton.setAlwaysOnTop(true);
        addChildComponent(backButton);
        addAndMakeVisible(showCredits);
        addAndMakeVisible(showLicense);

        creditsViewport.setScrollBarsShown(true, false);
        creditsViewport.setViewedComponent(&creditsComponent, false);
        creditsComponent.setVisible(true);
        addChildComponent(creditsViewport);
        addChildComponent(licenseComponent);

        showCredits.setConnectedEdges(Button::ConnectedOnBottom);
        showLicense.setConnectedEdges(Button::ConnectedOnTop);

        showCredits.onClick = [this] {
            creditsViewport.setVisible(true);
            backButton.setVisible(true);
        };

        showLicense.onClick = [this] {
            licenseComponent.setVisible(true);
            backButton.setVisible(true);
        };
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        Fonts::drawStyledText(g, "plugdata", 0, 100, getWidth(), 30, findColour(PlugDataColour::panelTextColourId), Bold, 30, Justification::centred);

        g.setFont(Font(FontOptions(16)));
        g.drawFittedText("By Timothy Schoen", 0, 132, getWidth(), 30, Justification::centred, 1);

        g.setColour(findColour(PlugDataColour::dataColourId).withAlpha(0.2f));
        auto const versionBounds = getLocalBounds().withTrimmedTop(162).removeFromTop(32).withSizeKeepingCentre(64, 24);
        g.fillRoundedRectangle(versionBounds.toFloat(), 12.0f);
        Fonts::drawStyledText(g, "v" + String(ProjectInfo::versionString), versionBounds.getX(), versionBounds.getY(), versionBounds.getWidth(), versionBounds.getHeight(), findColour(PlugDataColour::panelTextColourId), Semibold, 16, Justification::centred);

        Rectangle<float> const logoBounds = getLocalBounds().removeFromTop(120.0f).withSizeKeepingCentre(84.0f, 84.0f).toFloat();

        g.setImageResamplingQuality(Graphics::highResamplingQuality);
        g.drawImage(logo, logoBounds);
        g.setImageResamplingQuality(Graphics::mediumResamplingQuality);

        for (auto& shadow : SmallArray<Rectangle<int>> { viewWebsite.getBounds().getUnion(viewOnGithub.getBounds()), reportIssue.getBounds(), sponsor.getBounds(), showCredits.getBounds().getUnion(showLicense.getBounds()) }) {
            Path shadowPath;
            shadowPath.addRoundedRectangle(shadow.reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(hash("about_panel"), g, shadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);
        }

        backButton.setBounds(2, 0, 40, 40);
    }

    void resized() override
    {
        creditsViewport.setBounds(getLocalBounds());
        creditsComponent.setSize(getWidth(), creditsComponent.getDesiredHeight() + 460);
        licenseComponent.setBounds(getLocalBounds());

        auto bounds = getLocalBounds().withTrimmedTop(190).reduced(16, 10);

        viewWebsite.setBounds(bounds.removeFromTop(44).reduced(4));
        viewOnGithub.setBounds(bounds.removeFromTop(44).reduced(4).translated(0, -9));

        bounds.removeFromTop(3);
        reportIssue.setBounds(bounds.removeFromTop(44).reduced(4));

        bounds.removeFromTop(6);
        sponsor.setBounds(bounds.removeFromTop(44).reduced(4));

        bounds.removeFromTop(6);
        showCredits.setBounds(bounds.removeFromTop(44).reduced(4));
        showLicense.setBounds(bounds.removeFromTop(44).reduced(4).translated(0, -9));
    }
};
