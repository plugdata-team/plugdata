#include "Components/MarkupDisplay.h"

// Renders every block type the MarkupDisplay component supports.
//
// MarkupDisplay is only reachable through the help browser, so its parser and
// renderers never run in the other tests. This feeds it one document per
// input format:
//
//  - setMarkdownString(): headings, bold/italic, labelled and bare links,
//    markdown images, lists, tables with separator rows, fenced code blocks
//    (both multi-line and inline pairs).
//  - setMarkupString(): the native markup features that markdown conversion
//    can't produce - admonitions (INFO/HINT/IMPORTANT/CAUTION/WARNING/quote),
//    ^header^ tables, {{image}} blocks, <img> HTML images, [[link|label]]
//    blocks, and <c#...>/<c:...> colour tags.
//
// Images resolve through a FileSource stub, and each document is painted via
// createComponentSnapshot() so the per-block paint() and layout code runs too.

class MarkupDisplayTest : public PlugDataUnitTest, public MarkupDisplay::FileSource
{
public:
    MarkupDisplayTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Markup Display Test")
    {
    }

private:
    Image getImageForFilename(String filename) override
    {
        Image image(Image::ARGB, 40, 40, true);
        Graphics g(image);
        g.fillAll(Colours::orange);
        return image;
    }

    void perform() override
    {
        beginTest("Markdown parsing and rendering");

        auto display = std::make_unique<MarkupDisplay::MarkupDisplayComponent>();
        display->setFileSource(this);
        editor->addAndMakeVisible(display.get());
        display->setBounds(0, 0, 500, 1200);

        display->setMarkdownString(
            "# Heading 1\n"
            "## Heading 2\n"
            "### Heading 3\n"
            "#### Heading 4\n"
            "##### Heading 5\n"
            "\n"
            "Some **bold text**, some *italic text*, some __also bold__ text,\n"
            "and a x<sub>subscript</sub> too.\n"
            "\n"
            "A [labelled link](https://plugdata.org) and a bare one: <https://plugdata.org>\n"
            "\n"
            "![an image](test_image.png)\n"
            "\n"
            "* first list item\n"
            "+ second list item\n"
            "- third list item with a [link](https://plugdata.org)\n"
            "\n"
            "| Column A | Column B |\n"
            "| --- | --- |\n"
            "| value 1 | value 2 |\n"
            "| value 3 | value 4 |\n"
            "\n"
            "```\n"
            "a fenced code block\n"
            "with two lines\n"
            "```\n"
            "\n"
            "An inline ```code``` fragment.\n"
            "\n"
            "The end.\n");
        display->createComponentSnapshot(display->getLocalBounds());

        beginTest("Native markup parsing and rendering");

        display->setMarkupString(
            "# Native markup\n"
            "\n"
            "INFO: an info admonition\n"
            "HINT: a hint admonition\n"
            "IMPORTANT: an important admonition\n"
            "CAUTION: a caution admonition\n"
            "WARNING: a warning admonition with a [[https://plugdata.org|link]]\n"
            "> a quote admonition\n"
            "\n"
            "^ Header A ^ Header B ^\n"
            "| cell 1 | cell 2 |\n"
            "| cell 3 | cell 4 |\n"
            "\n"
            "{{test_image.png}}\n"
            "[[https://plugdata.org|{{test_image.png}}]]\n"
            "<img src=\"test_image.png\" width=\"100\">\n"
            "\n"
            "[[https://plugdata.org|a labelled link block]]\n"
            "[[https://plugdata.org]]\n"
            "\n"
            "Some <c#FF0000>red text</c> and <c:blue>named colour text</c> in a line.\n"
            "\n"
            "The end.\n");
        display->createComponentSnapshot(display->getLocalBounds());

        editor->removeChildComponent(display.get());
        display.reset();

        signalDone(true);
    }
};
