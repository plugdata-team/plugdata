/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ExporterConsole : public Component
{
public:
    ExporterConsole()
    {
        viewport.setViewedComponent(this, false);
        viewport.setScrollBarsShown(true, false, false, false);
        setVisible(true);
        setWantsKeyboardFocus(true);
        setMouseCursor(MouseCursor::IBeamCursor);
    }
    
    void clear()
    {
        string.clear();
        plainText.clear();
        glyphPositions.clear();
        
        layout.createLayout(string, viewport.getWidth() - 8);
        viewport.setViewPositionProportionately(0.0f, 0.0f);
        selectionStart = selectionEnd = 0;
        
        layout.createLayout(string, viewport.getWidth() - 8);
        setSize(viewport.getWidth(), layout.getHeight() + 4);
        
        repaint();
    }
    
    void append(String const& text)
    {
        auto shouldAutoScroll = viewport.getViewPositionY() + viewport.getViewHeight() > getHeight() - 10;
        
        parseAnsiText(text);
        layout.createLayout(string, viewport.getWidth() - 8);
        setSize(viewport.getWidth(), layout.getHeight() + 4);

        if(shouldAutoScroll)
            viewport.setViewPositionProportionately(0.0f, 1.0f);
    }
    
    void paint(Graphics& g) override
    {
        // Draw selection highlight first
        if (hasSelection())
        {
            g.setColour(findColour(TextEditor::highlightColourId));
            
            auto selStart = jmin(selectionStart, selectionEnd);
            auto selEnd = jmax(selectionStart, selectionEnd);
            
            for (const auto& rect : getSelectionRectangles(selStart, selEnd))
            {
                g.fillRect(rect);
            }
        }
        
        layout.draw(g, getLocalBounds().reduced(4, 0).translated(0, -12).toFloat());
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        updateGlyphPositions();
        
        if(e.mods.isRightButtonDown())
        {
            copySelectionToClipboard();
            return;
        }
        if(e.mods.isShiftDown())
        {
            selectionEnd = getCharacterIndexAt(e.position);
            repaint();
            return;
        }
        
        selectionStart = selectionEnd = getCharacterIndexAt(e.position);
        repaint();
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        selectionEnd = getCharacterIndexAt(e.position);
        repaint();
    }
    
    void mouseDoubleClick(const MouseEvent& e) override
    {
        int index = getCharacterIndexAt(e.position);
        selectWord(index);
    }
    
    bool keyPressed(const KeyPress& key) override
    {
        if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
            if (hasSelection())
            {
                copySelectionToClipboard();
                return true;
            }
        }
        else if (key == KeyPress('a', ModifierKeys::commandModifier, 0)) {
            updateGlyphPositions();
            
            selectionStart = 0;
            selectionEnd = plainText.length();
            repaint();
            return true;
        }
        
        return false;
    }
    
    Viewport& getViewport()
    {
        return viewport;
    }
    
private:
    struct GlyphInfo
    {
        Rectangle<float> bounds;
        int charIndex;
    };
    
    void parseAnsiText(const String& text)
    {
        String fullText = ansiBuffer + text;
        ansiBuffer.clear();
        
        String currentSegment;
        Colour currentColour = findColour(PlugDataColour::panelTextColourId);
        Font currentFont = Fonts::getMonospaceFont();
        
        for (int i = 0; i < fullText.length(); ++i)
        {
            // Check for ANSI escape sequence start
            if (fullText[i] == '\x1b' && i + 1 < fullText.length() && fullText[i + 1] == '[')
            {
                // Append any text before this escape sequence
                if (currentSegment.isNotEmpty())
                {
                    appendWithWordWrap(currentSegment, currentFont, currentColour);
                    currentSegment = "";
                }
                
                // Find the end of the escape sequence
                int sequenceStart = i + 2;
                int sequenceEnd = sequenceStart;
                
                if(sequenceStart >= fullText.length())
                {
                    ansiBuffer = fullText.substring(i);
                    break;
                }
                
                while (sequenceEnd < fullText.length() &&
                       fullText[sequenceEnd] >= 0x30 && fullText[sequenceEnd] <= 0x3F)
                {
                    ++sequenceEnd;
                }
                
                if (sequenceEnd < fullText.length() &&
                    fullText[sequenceEnd] >= 0x40 && fullText[sequenceEnd] <= 0x7E)
                {
                    char command = fullText[sequenceEnd];
                    
                    if (command == 'm') // SGR (Select Graphic Rendition)
                    {
                        auto params = fullText.substring(sequenceStart, sequenceEnd);
                        parseAnsiSGR(params, currentColour, currentFont);
                    }
                    
                    i = sequenceEnd; // Skip past the entire sequence
                }
                else {
                    ansiBuffer = fullText.substring(i);
                    break;
                }
            }
            else
            {
                currentSegment += fullText[i];
            }
        }
        
        if (currentSegment.isNotEmpty())
        {
            appendWithWordWrap(currentSegment, currentFont, currentColour);
        }
        
        plainText = string.getText();
        needsGlyphPositionUpdate = true;
    }
    
    void parseAnsiSGR(const String& params, Colour& currentColour, Font& currentFont)
    {
        auto defaultTextColour = findColour(PlugDataColour::panelTextColourId);
        currentColour = defaultTextColour;
        currentFont = Fonts::getMonospaceFont();
                
        StringArray codes;
        codes.addTokens(params, ";", "");
        
        for (const auto& code : codes)
        {
            int value = code.getIntValue();
            switch (value)
            {
                case 0:
                    currentColour = findColour(PlugDataColour::panelTextColourId);
                    break;
                case 1:
                    currentFont = Fonts::getMonospaceBoldFont();
                    break;
                case 30:
                    currentColour = defaultTextColour.interpolatedWith(Colours::black, 0.5);
                    break;
                case 31:
                    currentColour = defaultTextColour.interpolatedWith(Colours::red, 0.5);
                    break;
                case 32:
                    currentColour = defaultTextColour.interpolatedWith(Colours::green, 0.5);
                    break;
                case 33:
                    currentColour = defaultTextColour.interpolatedWith(Colours::yellow, 0.5);
                    break;
                case 34:
                    currentColour = defaultTextColour.interpolatedWith(Colours::blue, 0.5);
                    break;
                case 35:
                    currentColour = defaultTextColour.interpolatedWith(Colours::magenta, 0.5);
                    break;
                case 36:
                    currentColour = defaultTextColour.interpolatedWith(Colours::cyan, 0.5);
                    break;
                case 37:
                    currentColour = defaultTextColour.interpolatedWith(Colours::white, 0.5);
                    break;
                    
                case 90:
                    currentColour = defaultTextColour.interpolatedWith(Colours::darkgrey, 0.7);
                    break;
                case 91:
                    currentColour = defaultTextColour.interpolatedWith(Colours::red, 0.7);
                    break;
                case 92:
                    currentColour = defaultTextColour.interpolatedWith(Colours::green, 0.7);
                    break;
                case 93:
                    currentColour = defaultTextColour.interpolatedWith(Colours::yellow, 0.7);
                    break;
                case 94:
                    currentColour = defaultTextColour.interpolatedWith(Colours::blue, 0.7);
                    break;
                case 95:
                    currentColour = defaultTextColour.interpolatedWith(Colours::magenta, 0.7);
                    break;
                case 96:
                    currentColour = defaultTextColour.interpolatedWith(Colours::cyan, 0.7);
                    break;
                case 97:
                    currentColour = defaultTextColour.interpolatedWith(Colours::white, 0.7);
                    break;
                default:
                    break;
            }
        }
    }
    
    void appendWithWordWrap(const String& text, Font const& font, Colour const& colour)
    {
        StringArray words;
        
        // Split on spaces while preserving the spaces
        int lastPos = 0;
        for (int i = 0; i < text.length(); ++i)
        {
            if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r')
            {
                if (i > lastPos)
                {
                    words.add(text.substring(lastPos, i));
                }
                
                // Check if \r is part of \r\n (Windows line ending)
                if (text[i] == '\r' && i + 1 < text.length() && text[i + 1] == '\n')
                {
                    words.add("\r\n");  // Treat as single unit
                    ++i;  // Skip the \n
                }
                else
                {
                    words.add(String::charToString(text[i]));
                }
                
                lastPos = i + 1;
            }
        }
        
        // Add remaining text
        if (lastPos < text.length())
        {
            words.add(text.substring(lastPos));
        }
        
        float maxWidth = viewport.getWidth() - 10;
        
        for (int wordIdx = 0; wordIdx < words.size(); ++wordIdx)
        {
            const auto& word = words[wordIdx];
            
            if (word == "\n" || word == "\r\n")
            {
                string.append("\n", font, colour);
                plainText += "\n";
                currentLineWidth = 0;
                continue;
            }
            
            if (word == "\r")
            {
                // Standalone \r means overwrite current line
                int lastNewline = plainText.lastIndexOf("\n");
                if (lastNewline >= 0)
                {
                    plainText = plainText.substring(0, lastNewline + 1);
                    string.eraseEnd(lastNewline + 1);
                }
                else
                {
                    plainText.clear();
                    string.clear();
                }
                currentLineWidth = 0;
                continue;
            }
            
            float wordWidth = Fonts::getStringWidth(word, font);
            
            // Look ahead: if this is whitespace, check if the next word would fit
            if ((word == " " || word == "\t") && wordIdx + 1 < words.size())
            {
                float nextWordWidth = Fonts::getStringWidth(words[wordIdx + 1], font);
                
                // If current line + space + next word exceeds width, break NOW
                if (currentLineWidth + wordWidth + nextWordWidth > maxWidth && currentLineWidth > 0)
                {
                    string.append("\n", font, colour);
                    plainText += "\n";
                    currentLineWidth = 0;
                    continue; // Skip the space
                }
            }
            
            // If adding this word would exceed the width, insert a newline
            if (currentLineWidth + wordWidth > maxWidth && currentLineWidth > 0 && word != " " && word != "\t")
            {
                string.append("\n", font, colour);
                plainText += "\n";
                currentLineWidth = 0;
            }
            
            string.append(word, font, colour);
            plainText += word;
            currentLineWidth += wordWidth;
        }
    }
    
    void updateGlyphPositions()
    {
        if(!needsGlyphPositionUpdate) return;
        
        glyphPositions.clear();
        int charIndex = 0;
        for (auto& line : layout) {
            float lastX = 4.0f;
            for (auto* run : line.runs) {
                auto const runText = string.getText().substring(run->stringRange.getStart(), run->stringRange.getEnd());
                int glyphIndex = 0;
                for (int i = 0; i < runText.length(); ++i) {
                    if (CharacterFunctions::isWhitespace(runText[i])) {
                        GlyphInfo info;
                        
                        info.bounds = line.getLineBounds().withX(lastX).withWidth(Fonts::getStringWidth(runText.substring(i, i + 1), run->font)).translated(0, -12);
                        info.charIndex = charIndex++;
                        glyphPositions.add(info);
                        lastX = info.bounds.getRight();
                        continue;
                    }
                    while (glyphIndex < run->glyphs.size() &&
                           run->glyphs.getReference(glyphIndex).glyphCode == 1
                           )
                        glyphIndex++;
                    
                    if (glyphIndex < run->glyphs.size()) {
                        auto const& glyph = run->glyphs.getReference(glyphIndex);
                        auto const position = glyph.anchor + line.lineOrigin;
                        GlyphInfo info;
                        info.bounds = Rectangle<float>(position.x + 4, position.y - run->font.getAscent() - 12, glyph.width, run->font.getHeight());
                        info.charIndex = charIndex++;
                        glyphPositions.add(info);
                        glyphIndex++;
                        lastX = info.bounds.getRight();
                    }
                }
            }
        }
        
        needsGlyphPositionUpdate = false;
    }
    
    int getCharacterIndexAt(Point<float> position)
    {
        int closestIndex = 0;
        for (const auto& glyph : glyphPositions)
        {
            bool pastCharacter = position.x > (glyph.bounds.getX() - 2);
            bool sameLine = glyph.bounds.withX(position.x - 1).withWidth(2).contains(position);
            
            if (sameLine && pastCharacter)
            {
                closestIndex = glyph.charIndex;
            }
        }
        
        return jlimit(0, plainText.length(), closestIndex);
    }
    
    SmallArray<Rectangle<float>> getSelectionRectangles(int start, int end)
    {
        SmallArray<Rectangle<float>> rectangles;
        
        if (glyphPositions.empty())
            return rectangles;
        
        float currentY = -1;
        float lineLeft = std::numeric_limits<float>::max();
        float lineRight = 0;
        float glyphHeight = 0.0f;
        
        for (int i = start; i < end && i < glyphPositions.size(); ++i)
        {
            const auto& glyph = glyphPositions[i];
            auto const glyphY = glyph.bounds.getY();
            glyphHeight = glyph.bounds.getHeight();
            
            if (std::abs(glyphY - currentY) > glyphHeight * 0.5f)
            {
                rectangles.add(Rectangle<float>(lineLeft, currentY,
                                                lineRight - lineLeft, glyphHeight));
                lineLeft = std::numeric_limits<float>::max();
                lineRight = 0;
            }
            
            currentY = glyphY;
            lineLeft = jmin(lineLeft, glyph.bounds.getX());
            lineRight = jmax(lineRight, glyph.bounds.getRight());
        }
        
        if (currentY >= 0)
        {
            rectangles.add(Rectangle<float>(lineLeft, currentY,
                                            lineRight - lineLeft, glyphHeight));
        }
        
        return rectangles;
    }
    
    bool hasSelection() const
    {
        return selectionStart != selectionEnd;
    }
    
    void selectWord(int index)
    {
        if (index < 0 || index >= plainText.length())
            return;
        
        int start = index;
        int end = index;
        
        while (start > 0 && !CharacterFunctions::isWhitespace(plainText[start - 1]))
            --start;
        
        while (end < plainText.length() && !CharacterFunctions::isWhitespace(plainText[end]))
            ++end;
        
        selectionStart = start;
        selectionEnd = end;
        repaint();
    }
    
    void copySelectionToClipboard()
    {
        if (!hasSelection())
            return;
        
        int start = jmin(selectionStart, selectionEnd);
        int end = jmax(selectionStart, selectionEnd);
        
        String selectedText = plainText.substring(start, end);
        SystemClipboard::copyTextToClipboard(selectedText);
    }
    
    Viewport viewport;
    AttributedString string;
    TextLayout layout;
    String plainText;
    String ansiBuffer;
    HeapArray<GlyphInfo> glyphPositions;
    
    int selectionStart = 0;
    int selectionEnd = 0;
    float currentLineWidth = 0;
    
    bool needsGlyphPositionUpdate = false;
};

class ExportingProgressView final : public Component
    , public Thread
    , public Timer {
    
    ExporterConsole console;
    ChildProcess* processToMonitor;

public:
    enum ExportState {
        Exporting,
        Flashing,
        Success,
        Failure,
        BootloaderFlashSuccess,
        BootloaderFlashFailure,
        NotExporting
    };

    TextButton continueButton = TextButton("Continue");

    AtomicValue<ExportState> state = NotExporting;

    String userInteractionMessage;
        
    String allConsoleOutput;
    CriticalSection allConsoleOutputLock;

    static constexpr int maxLength = 8192;
    char processOutput[maxLength];

    ExportingProgressView()
        : Thread("Console thread")
    {
        setVisible(false);
        addChildComponent(continueButton);
        addAndMakeVisible(console.getViewport());

        continueButton.onClick = [this] {
            showState(NotExporting);
        };
    }

    // For the spinning animation
    void timerCallback() override
    {
        repaint();
    }
        
    bool hasConsoleMessage(StringArray const& messagesToFind)
    {
        ScopedLock lock(allConsoleOutputLock);
        for(auto& message : messagesToFind)
        {
            if(allConsoleOutput.contains(message))
            {
                return true;
            }
        }
        return false;
    }
        
    void run() override
    {
        while (processToMonitor && !threadShouldExit()) {
            if (int const len = processToMonitor->readProcessOutput(processOutput, maxLength)) {
                auto newOutput = String::fromUTF8(processOutput, len);
                
                allConsoleOutputLock.enter();
                allConsoleOutput += newOutput;
                allConsoleOutputLock.exit();
                
                logToConsole(newOutput);
            }

            Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 100);
        }
    }

    void monitorProcessOutput(ChildProcess* process)
    {
        startTimer(20);
        processToMonitor = process;
        startThread();
    }

    void flushConsole()
    {
        while (processToMonitor) {
            int const len = processToMonitor->readProcessOutput(processOutput, maxLength);
            if (!len)
                break;

            logToConsole(String::fromUTF8(processOutput, len));
        }
    }

    void stopMonitoring()
    {
        flushConsole();
        stopThread(-1);
        stopTimer();
    }

    void showState(ExportState const newState)
    {
        state = newState;

        MessageManager::callAsync([this] {
            setVisible(state < NotExporting);
            continueButton.setVisible(state >= Success);
            if (state == Exporting || state == Flashing)
                console.clear();
            if (console.isShowing()) {
                console.grabKeyboardFocus();
            }

            resized();
            repaint();
        });
    }

    void logToConsole(String const& text)
    {
        if (text.isNotEmpty()) {
            MessageManager::callAsync([_this = SafePointer(this), text] {
                if (!_this)
                    return;

                _this->console.append(text);
            });
        }
    }

    void paint(Graphics& g) override
    {
        auto const b = getLocalBounds();

        Path background;
        background.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);

        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillPath(background);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(background, PathStrokeType(1.0f));

        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRoundedRectangle(console.getViewport().getBounds().expanded(6).toFloat(), Corners::defaultCornerRadius);

        if (state == Exporting) {
            Fonts::drawStyledText(g, "Exporting...", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

            getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 118, 32, 32);
        } else if (state == Flashing) {
            Fonts::drawStyledText(g, "Flashing...", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

            getLookAndFeel().drawSpinningWaitAnimation(g, findColour(PlugDataColour::panelTextColourId), getWidth() / 2 - 16, getHeight() / 2 + 118, 32, 32);
        } else if (state == Success) {
            Fonts::drawStyledText(g, "Export successful", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);

        } else if (state == Failure) {
            Fonts::drawStyledText(g, "Exporting failed", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        } else if (state == BootloaderFlashSuccess) {
            Fonts::drawStyledText(g, "Bootloader flashed", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        } else if (state == BootloaderFlashFailure) {
            Fonts::drawStyledText(g, "Bootloader flash failed", 0, 20, getWidth(), 32, findColour(PlugDataColour::panelTextColourId), Bold, 32, Justification::centred);
        }
    }
    void resized() override
    {
        console.getViewport().setBounds(proportionOfWidth(0.05f), 80, proportionOfWidth(0.9f), getHeight() - 172);
        continueButton.setBounds(proportionOfWidth(0.42f), getHeight() - 60, proportionOfWidth(0.12f), 24);
    }
};
