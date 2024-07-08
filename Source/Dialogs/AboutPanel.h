/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
EXTERN char* pd_version;
}

class AboutPanel : public Component {
    WidePanelButton viewWebsite = WidePanelButton(Icons::OpenLink);
    WidePanelButton viewOnGithub = WidePanelButton(Icons::OpenLink);
    WidePanelButton reportIssue = WidePanelButton(Icons::OpenLink);
    WidePanelButton sponsor = WidePanelButton(Icons::OpenLink);

    WidePanelButton showCredits = WidePanelButton(Icons::Forward, 15);
    WidePanelButton showLicense = WidePanelButton(Icons::Forward, 15);

    class CreditsViewport : public BouncingViewport {
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
    class CreditsPanel : public Component {
        std::vector<std::pair<String, String>> const contributors = {
            { "Timothy Schoen", "Lead development, UI/UX design" },
            { "Alex Mitchell", "Development, UI/UX design" },
            { "Joshua A.C. Newman", "Community management, logo and identity design" },
            { "Bas de Bruin", "Logo design" },
            { "Alexandre Porres", "ELSE and cyclone development" },
            { "dreamer", "Hvcc development" },
            { "tomara-x", "Documentation, testing" }
        };

        StringArray const sponsors = {
            "Deskew Technologies",
            "Naskomusic",
            "epsil0ndelta",
            "polarity",
            "chee",
            "ghjameslo",
            "ludnny",
            "merspir",
            "phevosccp",
            "Radialarray",
            "bla9kdog",
            "EwanBristow1400",
            "JoshuaACNewman",
            "vasilymilovidov",
            "LuigiCosi",
            "notagoodidea",
            "o-g-sus",
            "pyrodogg",
            "alexandrotrevino",
            "angelfaraldo",
            "bruzketta",
            "brylie",
            "cotik1",
            "DavidWahlund",
            "Enkerli",
            "grabanton",
            "jamescorrea",
            "motekulo",
            "nawarajkhatri",
            "NothanUmber",
            "olbotta",
            "KPY7030P",
            "duddex",
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
            StackShadow::renderDropShadow(g, firstShadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);

            for (int i = 0; i < contributors.size(); i++) {
                auto rowBounds = bounds.removeFromTop(48);
                auto first = i == 0;
                auto last = i == (contributors.size() - 1);
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

            Fonts::drawStyledText(g, "Sponsors", bounds.getX(), bounds.getY() - 8, bounds.getWidth(), 15.0f, findColour(PlugDataColour::panelTextColourId), Semibold, 15.0f);

            bounds.removeFromTop(16);

            Path secondShadowPath;
            secondShadowPath.addRoundedRectangle(Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth(), sponsors.size() * 32).reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(g, secondShadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);
            for (int i = 0; i < sponsors.size(); i++) {
                auto rowBounds = bounds.removeFromTop(36);
                auto first = i == 0;
                auto last = i == (sponsors.size() - 1);
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
        
        int getDesiredHeight()
        {
            return sponsors.size() * 36;
        }
    };

    class LicensePanel : public Component {
        TextEditor license;

        String licenseText = "Copyright Timothy Schoen\n\n"
                             "This app is licensed under the GNU General Public License version 3 (GPL-3.0). You are free to use, modify, and distribute the software, provided that any derivative works also carry the same license and the source code remains accessible.\n"
                             "This application comes with absolutely no warranty.";

    public:
        LicensePanel()
        {
            license.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            license.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            license.setReadOnly(true);
            license.setMultiLine(true);
            license.setText(licenseText);
            license.setFont(Font(15));
            license.setLineSpacing(1.1f);
            addAndMakeVisible(license);
        }

        void resized() override
        {
            license.setBounds(getLocalBounds().withTrimmedTop(32).reduced(16, 8));
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

    Image logo = ImageFileFormat::loadFrom(BinaryData::plugdata_large_logo_png, BinaryData::plugdata_large_logo_pngSize);

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

        viewWebsite.onClick = []() {
            URL("https://plugdata.org").launchInDefaultBrowser();
        };

        viewOnGithub.onClick = []() {
            URL("https://github.com/plugdata-team/plugdata").launchInDefaultBrowser();
        };

        reportIssue.onClick = []() {
            URL("https://github.com/plugdata-team/plugdata/issues").launchInDefaultBrowser();
        };

        sponsor.onClick = []() {
            URL("https://github.com/sponsors/timothyschoen").launchInDefaultBrowser();
        };

        backButton.setButtonText(Icons::Back);
        backButton.onClick = [this]() {
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

        showCredits.onClick = [this]() {
            creditsViewport.setVisible(true);
            backButton.setVisible(true);
        };

        showLicense.onClick = [this]() {
            licenseComponent.setVisible(true);
            backButton.setVisible(true);
        };
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

        Fonts::drawStyledText(g, "plugdata", 0, 100, getWidth(), 30, findColour(PlugDataColour::panelTextColourId), Bold, 30, Justification::centred);

        g.setFont(Font(16));
        g.drawFittedText("By Timothy Schoen", 0, 132, getWidth(), 30, Justification::centred, 1);

        g.setColour(findColour(PlugDataColour::dataColourId).withAlpha(0.2f));
        auto versionBounds = getLocalBounds().withTrimmedTop(162).removeFromTop(32).withSizeKeepingCentre(64, 24);
        g.fillRoundedRectangle(versionBounds.toFloat(), 12.0f);
        Fonts::drawStyledText(g, "v" + String(ProjectInfo::versionString), versionBounds.getX(), versionBounds.getY(), versionBounds.getWidth(), versionBounds.getHeight(), findColour(PlugDataColour::panelTextColourId), Semibold, 16, Justification::centred);

        Rectangle<float> logoBounds = getLocalBounds().removeFromTop(120.0f).withSizeKeepingCentre(84.0f, 84.0f).toFloat();

        g.setImageResamplingQuality(Graphics::highResamplingQuality);
        g.drawImage(logo, logoBounds);
        g.setImageResamplingQuality(Graphics::mediumResamplingQuality);

        for (auto& shadow : std::vector<Rectangle<int>> { viewWebsite.getBounds().getUnion(viewOnGithub.getBounds()), reportIssue.getBounds(), sponsor.getBounds(), showCredits.getBounds().getUnion(showLicense.getBounds()) }) {
            Path shadowPath;
            shadowPath.addRoundedRectangle(shadow.reduced(4), Corners::largeCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.32f), 8);
        }

        backButton.setBounds(2, 0, 40, 40);
    }

    void resized() override
    {
        creditsViewport.setBounds(getLocalBounds());
        creditsComponent.setSize(getWidth(), creditsComponent.getDesiredHeight() + 450);
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
