/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
EXTERN char* pd_version;
}

class AboutPanel : public Component {

    Image logo = ImageFileFormat::loadFrom(BinaryData::plugdata_logo_png, BinaryData::plugdata_logo_pngSize);

public:
    void paint(Graphics& g) override
    {
        Fonts::drawStyledText(g, "plugdata " + String(ProjectInfo::versionString), 150, 20, 300, 30, findColour(PlugDataColour::panelTextColourId), Bold, 30);

        g.setFont(Font(16));
        g.drawFittedText("By Timothy Schoen", 150, 50, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("Based on " + String(pd_version).upToFirstOccurrenceOf("(", false, false) + " by Miller Puckette and others", 150, 70, getWidth() - 150, 30, Justification::left, 1);

        g.drawFittedText("ELSE v1.0-rc7 by Alexandre Porres", 150, 110, getWidth() - 150, 30, Justification::left, 1);
        g.drawFittedText("cyclone v0.7-0 by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Porres, Derek Kwan, Matt Barber and others", 150, 130, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Based on Camomile (Pierre Guillot)", 150, 170, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inspired by Kiwi (Elliot Paris, Pierre Guillot, Jean Millot)", 150, 190, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Inter font by Rasmus Andersson", 150, 210, getWidth() - 150, 50, Justification::left, 2);
        g.drawFittedText("Made with JUCE", 150, 230, getWidth() - 150, 50, Justification::left, 2);

        g.drawFittedText("Special thanks to: Deskew Technologies, BonefishSam, DSBHproject, CFDAF, AurelienConil, jamescorrea, Soundworlds-JO, vasilymilovidov, grabanton, polarity, rubenlorenzo, chee, kreth608, Joshua A.C. Newman, ludnny, duddex, mungbean and droussel for supporting this project", 150, 270, getWidth() - 200, 80, Justification::left, 3);

        g.drawFittedText("This program is published under the terms of the GPL3 license", 150, 340, getWidth() - 150, 50, Justification::left, 2);

        Rectangle<float> logoBounds = { 20.0f, 25.0f, 110.0f, 110.0f };

        g.drawImage(logo, logoBounds);
    }
};
