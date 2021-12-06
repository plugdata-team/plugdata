

/*
 MIT License

 Copyright (c) 2018 Janos Buttgereit

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 ==============================================================================

 BEGIN_JUCE_MODULE_DECLARATION

 ID:            log_component
 vendor:        ntlab
 version:       1.0.0
 name:          Console
 description:   Displays stdout, stderr and custom logging messages on the GUI
 dependencies:  juce_gui_basics
 website:       git.fh-muenster.de/NTLab/CodeReUse/Console4JUCE
 license:       MIT

 END_JUCE_MODULE_DECLARATION

 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>
#include <thread>
#include "LookAndFeel.h"


/**
 * A component that takes over stdout and stderr and displays it inside
 */

struct LogContainer : public Component
{
    int totalheight = 0;
    int colourCount = 0;
    std::vector<Colour> colours;
    
    
    LogContainer(std::vector<Colour> colourscheme) : colours(colourscheme) {
        
    }
    
    
    void paint(Graphics & g) override {
        
        for(int i = totalheight; i < getHeight(); i += 24)
        {
            colourCount = !colourCount;
            g.setColour(colours[colourCount]);
            g.fillRect(0, i, getWidth(), 24);
        }
    };
    
};
    
class Console : public Component, public Logger, private AsyncUpdater
{

public:

	/**
	 * Creates a new log component. By default, stderr and stdout are captured immediately after
	 * construction
	 */
	Console (bool captureStdErrImmediately = false, bool captureStdOutImmediately = false);

	~Console();

	/** Clears the log */
	void clear();

	/** Posts a message directly to the Console, is also called if this is set as the current juce logger */
	void logMessage (const String &message) override;

	/** Sets a new colour for all stdout prints. Default is blue */
	void setNewLogMessageColour (const Colour& newColour);

	/** Sets a new colour for all stdout prints. Default is black */
	void setNewStdOutColour (const Colour& newStdOutColour);

	/** Sets a new colour for all stderr prints. Default is red */
	void setNewStdErrColour (const Colour& newStdErrColour);

	/** Redirects stdout to this component. Call releaseStdOut to restore the prior state */
	bool captureStdOut();

	/** Redirects stderr to this component. Call releaseStdErr to restore the prior state */
	bool captureStdErr();

	/** Restores the original stdout */
	void releaseStdOut (bool printRestoreMessage = true);

	/** Restores the original stderr */
	void releaseStdErr (bool printRestoreMessage = true);

	void resized() override;

private:
    
    Viewport viewport;

	int totalHeight = 0;
	int colourCount = 1;

	// filedescriptors to restore the standard console output streams
	static int originalStdout, originalStderr;

	// pipes to redirect the streams to this component
	static int logStdOutputPipe[2];
	static int logErrOutputPipe[2];

	static std::unique_ptr<std::thread> stdOutReaderThread;
	static std::unique_ptr<std::thread> stdErrReaderThread;

	// indicates if it is the current stdout
	static Console* currentStdOutTarget;
	static Console* currentStdErrTarget;

	static const int tmpBufLen = 512;
    
    std::vector<Colour> colours = {MainLook::secondBackground, MainLook::firstBackground};
    
    LogContainer logContainer;

    StatusbarLook statusbarLook = StatusbarLook(false, 1.4);
    TextButton clearButton;

	Colour stdOutColour = Colours::white;
	Colour stdErrColour = Colours::orange;
	Colour logMessageColour = Colours::lightblue;
	Colour backgroundColour = Colours::white;

	// this is where the text is stored
	int numLinesToStore = 90;
	int numLinesToRemoveWhenFull = 20;
	int numLinesStored = 0;
	int numNewLinesSinceUpdate = 0;

	OwnedArray<Label> items;
	StringArray lines;
	Array<Colour> lineColours;
	CriticalSection linesLock;

	static bool createAndAssignPipe (int *pipeIDs, FILE *stream);
	static void deletePipeAndEndThread (int original, FILE *stream, std::unique_ptr<std::thread>& thread);

	static void readStdOut();

	static void readStdErr();

	void paint(Graphics &g) override;

	void addFromStd (char *stringBufferToAdd, size_t bufferSize, Colour colourOfString);

	void handleAsyncUpdate() override;
};


