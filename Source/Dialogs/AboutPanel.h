/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
EXTERN char* pd_version;
}

class AboutPanel : public Component {

    String creditsText = " Thanks to Alex Mitchell and tomara-x for helping out with development, documentation and testing.\n\n - ELSE v1.0-rc9 by Alexandre Porres\n - cyclone v0.7-0 by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber et al.\n - Based on Camomile by Pierre Guillot\n - Inter font by Rasmus Andersson\n - Made with JUCE\n\n\nSpecial thanks to: Deskew Technologies, PowerUser64, DSBHproject, CFDAF, cotik1 , alfonso73, spamfunnel, ooroor, bla9kdog, KPY7030P, duddex, rubenlorenzo, mungbean, jamescorrea, Soundworlds-JO, vasilymilovidov, polarity, chee, Joshua A.C. Newman and ludnny for supporting this project\n\n\nThis program is published under the terms of the GPL3 license";

    TextEditor credits;
    TextButton viewOnGithub;
    TextButton reportIssue;
    TextButton sponsor;

    Image logo = ImageFileFormat::loadFrom(BinaryData::plugdata_large_logo_png, BinaryData::plugdata_large_logo_pngSize);

public:
    AboutPanel()
    {
        credits.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        credits.setReadOnly(true);
        credits.setMultiLine(true);
        credits.setText(creditsText);
        credits.setFont(Font(15));
        credits.setLineSpacing(1.1f);
        addAndMakeVisible(credits);

        viewOnGithub.setButtonText("View on Github");
        reportIssue.setButtonText("Report issue");
        sponsor.setButtonText("Become a sponsor");

        addAndMakeVisible(viewOnGithub);
        addAndMakeVisible(reportIssue);
        addAndMakeVisible(sponsor);

        viewOnGithub.onClick = []() {
            URL("https://github.com/plugdata-team/plugdata").launchInDefaultBrowser();
        };

        reportIssue.onClick = []() {
            URL("https://github.com/plugdata-team/plugdata/issues").launchInDefaultBrowser();
        };

        sponsor.onClick = []() {
            URL("https://github.com/sponsors/timothyschoen").launchInDefaultBrowser();
        };
    }

    void paint(Graphics& g) override
    {
        auto logoStart = 185.0f;
        auto logoSize = 75.0f;
        auto textStart = logoStart + logoSize + 15.0f;

        Fonts::drawStyledText(g, "plugdata " + String(ProjectInfo::versionString), textStart, 20, getWidth() - textStart, 30, findColour(PlugDataColour::panelTextColourId), Bold, 30, Justification::left);

        g.setFont(Font(16));
        g.drawFittedText("By Timothy Schoen", textStart, 50, getWidth() - textStart, 30, Justification::left, 1);
        g.drawFittedText("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false) + "by Miller Puckette et al.", textStart, 70, getWidth() - textStart, 30, Justification::left, 1);

        Rectangle<float> logoBounds = { logoStart, 25.0f, logoSize, logoSize };

        g.setImageResamplingQuality(Graphics::highResamplingQuality);
        g.drawImage(logo, logoBounds);
        g.setImageResamplingQuality(Graphics::mediumResamplingQuality);

        auto creditsBounds = credits.getBounds().expanded(5);
        g.setColour(findColour(TextEditor::backgroundColourId));
        PlugDataLook::fillSmoothedRectangle(g, creditsBounds.toFloat(), Corners::largeCornerRadius);

        g.setColour(findColour(TextEditor::outlineColourId));
        PlugDataLook::drawSmoothedRectangle(g, PathStrokeType(1.0f), creditsBounds.toFloat(), Corners::largeCornerRadius);
    }

    void resized() override
    {
        credits.setBounds(getLocalBounds().withTrimmedTop(100).withTrimmedBottom(25).reduced(30));
        auto buttonBounds = getLocalBounds().removeFromBottom(54).reduced(25, 15);
        int buttonWidth = (buttonBounds.getWidth() / 3) - 5;

        viewOnGithub.setBounds(buttonBounds.removeFromLeft(buttonWidth));
        buttonBounds.removeFromLeft(5);

        reportIssue.setBounds(buttonBounds.removeFromLeft(buttonWidth));
        buttonBounds.removeFromLeft(5);

        sponsor.setBounds(buttonBounds);
    }
};
