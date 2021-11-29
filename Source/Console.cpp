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


#include "Console.h"


#include  <fcntl.h>                              //
#include  <stdio.h>                              //
#include  <stdlib.h>                             //
#include  <string.h>                             //
#include  <sys/types.h>                          //
#include  <sys/wait.h>                           //
#include  <sys/stat.h>                           //
//#include  <termios.h>                            //
#include  <unistd.h>
#ifdef JUCE_WINDOWS
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#define fileno _fileno
#define dup _dup
#define dup2 _dup2
#define read _read
#define close _close
#endif



Console::Console (bool captureStdErrImmediately, bool captureStdOutImmediately) : logContainer(colours)
{
#ifdef JUCE_WINDOWS

	if (AllocConsole())
	{
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);
	}

#endif
    viewport.setViewedComponent(&logContainer);
    viewport.setScrollBarsShown(true, false);

	// save the original stdout and stderr to restore it later
	if (originalStdout == -3)
		originalStdout = dup (fileno (stdout));

	if (originalStderr == -4)
		originalStderr = dup (fileno (stderr));

	std::ios::sync_with_stdio();

	if (captureStdErrImmediately)
		captureStdErr();

	if (captureStdOutImmediately)
		captureStdOut();
    
    clearButton.setButtonText(CharPointer_UTF8("\xef\x80\x8d"));
    clearButton.setConnectedEdges(12);
    clearButton.setLookAndFeel(&statusbar_look);
    clearButton.onClick = [this]() {
        clear();
    };
    
    addAndMakeVisible(clearButton);
    addAndMakeVisible(viewport);
}

Console::~Console()
{
	releaseStdOut();
	releaseStdErr();

    clearButton.setLookAndFeel(nullptr);
    
	if (Logger::getCurrentLogger() == this)
		Logger::setCurrentLogger (nullptr);
}

void Console::clear()
{
	const ScopedLock scopedLock (linesLock);
	items.clear();
	lines.clear();
	lineColours.clearQuick();
	numLinesStored = 0;
	numNewLinesSinceUpdate = 0;
    resized();
}

void Console::logMessage (const String &message)
{
	{
		const ScopedLock scopedLock (linesLock);
		lines.add (message + "\n");
		lineColours.add (logMessageColour);
		++numNewLinesSinceUpdate;
		++numLinesStored;
	}
	triggerAsyncUpdate();
}

void Console::setNewLogMessageColour (const Colour& newLogMessageColour)
{
	logMessageColour = newLogMessageColour;
}

void Console::setNewStdOutColour (const Colour& newStdOutColour)
{
	stdOutColour = newStdOutColour;
}

void Console::setNewStdErrColour (const Colour& newStdErrColour)
{
	stdErrColour = newStdErrColour;
}

bool Console::captureStdOut()
{
	if (currentStdOutTarget == this)
		return true;

	if (createAndAssignPipe (logStdOutputPipe, stdout) == false)
		return false;

	currentStdOutTarget = this;

	if (stdOutReaderThread == nullptr)
		stdOutReaderThread.reset (new std::thread (Console::readStdOut));

	return true;
}

bool Console::captureStdErr()
{
	if (currentStdErrTarget == this)
		return true;

	if (createAndAssignPipe (logErrOutputPipe, stderr) == false)
		return false;

	currentStdErrTarget = this;

	if (stdErrReaderThread == nullptr)
		stdErrReaderThread.reset (new std::thread (Console::readStdErr));

	return true;
}

void Console::releaseStdOut (bool printRestoreMessage)
{
	if (currentStdOutTarget != this)
		return;

	currentStdOutTarget = nullptr;
	deletePipeAndEndThread (originalStdout, stdout, stdOutReaderThread);

	if (printRestoreMessage)
		std::cout << "Console restored stdout" << std::endl;
}

void Console::releaseStdErr (bool printRestoreMessage)
{
	if (currentStdErrTarget != this)
		return;

	currentStdErrTarget = nullptr;
	deletePipeAndEndThread (originalStderr, stderr, stdErrReaderThread);

	if (printRestoreMessage)
		std::cout << "Console restored stderr" << std::endl;
}

void Console::resized()
{
	int fontHeight = items.size() > 0 ? items[0]->getFont().getHeight() : 0;
	int boundWidth = getWidth();
	totalHeight = 0;

	for(int i = 0; i < items.size(); i++)
	{
		int width = items[i]->getFont().getStringWidth(items[i]->getText()) * 1.3;
		int height = 12 + fontHeight * ((width / (boundWidth + (boundWidth == 0))) + 1);
		items[i]->setBounds(0, totalHeight, boundWidth, height);
		totalHeight += height;
	}
    logContainer.totalheight = totalHeight;
	logContainer.setBounds(0, 0, getWidth(), std::max(totalHeight, getHeight()-30));
    
    viewport.setBounds(0, 0, getWidth(), getHeight()-30);
    clearButton.setBounds(getWidth() - 50, getHeight()-30, 30, 30);
	repaint();
}


void Console::paint(Graphics &g)
{
    logContainer.colourCount = colourCount;
    g.setColour(Colours::grey);
    g.drawLine(0, getHeight()-35, getWidth(), getHeight()-35);
    
    
    g.setColour(MainLook::background_1);
    g.fillRect(0, getHeight()-35, getWidth(), 35);
}

bool Console::createAndAssignPipe (int *pipeIDs, FILE *stream)
{
	fflush (stream);
#ifdef JUCE_WINDOWS
	int retValue = _pipe (pipeIDs, tmpBufLen, _O_TEXT);
#else
	int retValue = pipe (pipeIDs);
#endif

	if (retValue != 0)
		return false;

	dup2 (pipeIDs[1], fileno (stream));
	close (pipeIDs[1]);
	std::ios::sync_with_stdio();
	// no buffering - will make new content appear as soon as possible
	setvbuf (stream, NULL, _IONBF, 0);
	return true;
}

void Console::deletePipeAndEndThread (int original, FILE *stream, std::unique_ptr<std::thread>& thread)
{
	// send some empty string to trigger the read thread and let it come to an end
	fprintf (stream, " ");
	fflush (stream);
	thread->join();
	// redirect stdout to its original destination
	dup2 (original, fileno (stream));
	// delete the read thread
	thread.reset (nullptr);
}

void Console::readStdOut()
{
	char tmpStdOutBuf[tmpBufLen];

	while (true)
	{
		fflush(stdout);
		size_t numCharsRead = read (logStdOutputPipe[0], tmpStdOutBuf, tmpBufLen - 1);

		if (currentStdOutTarget == nullptr)
			break;

		tmpStdOutBuf[numCharsRead] = '\0';
		currentStdOutTarget->addFromStd (tmpStdOutBuf, numCharsRead, currentStdOutTarget->stdOutColour);
	}
}

void Console::readStdErr()
{
	char tmpStdErrBuf[tmpBufLen];

	while (true)
	{
		fflush(stderr);
		size_t numCharsRead = read (logErrOutputPipe[0], tmpStdErrBuf, tmpBufLen - 1);

		if (currentStdErrTarget == nullptr)
			break;

		tmpStdErrBuf[numCharsRead] = '\0';
		currentStdErrTarget->addFromStd (tmpStdErrBuf, numCharsRead, currentStdErrTarget->stdErrColour);
	}
}

void Console::addFromStd (char *stringBufferToAdd, size_t bufferSize, Colour colourOfString)
{
	linesLock.enter();
	int numNewLines = lines.addTokens (stringBufferToAdd, "\n", "");

	for (int i = 1; i < numNewLines + 1; i++)
	{
		if(!lines.getReference(lines.size() - i).containsNonWhitespaceChars() || lines.getReference(lines.size() - i).isEmpty())
		{
			lines.remove(lines.size() - i);
			numNewLines--;
		}
	}

	for (int i = 0; i < numNewLines; i++)
	{
		lineColours.add (colourOfString);
	}

	numNewLinesSinceUpdate += numNewLines;
	numLinesStored += numNewLines;

	// add linebreaks
	if (stringBufferToAdd[bufferSize - 1] == '\n')
	{
		// only add a linebreak to the last line if it really had one - may contain an incomplete line
		lines.getReference (numLinesStored - 1) += "\n";
	}

	for (int i = numLinesStored - numNewLines; i < numLinesStored - 1; i++)
	{
		lines.getReference (i) += "\n";
	}

	linesLock.exit();
	triggerAsyncUpdate();
}

void Console::handleAsyncUpdate()
{
	const ScopedLock scopedLock (linesLock);

	// check if the lines should be cleared
	if (numLinesStored > numLinesToStore)
	{
		// remove 20 or more lines at the beginning
		int numLinesToRemove = numLinesStored - numLinesToStore;

		if (numLinesToRemove < numLinesToRemoveWhenFull)
			numLinesToRemove = numLinesToRemoveWhenFull;

		lines.removeRange (0, numLinesToRemove);
		lineColours.removeRange (0, numLinesToRemove);
		numLinesStored = lines.size();
		// clear the editor and flag all lines as new lines
		items.clear();
		numNewLinesSinceUpdate = numLinesStored;
	}

	// append new lines
	Colour lastColour = Colours::white;

	for (int i = numLinesStored - numNewLinesSinceUpdate; i < numLinesStored; i++)
	{
		if (lines[i].isNotEmpty() && lines[i].containsNonWhitespaceChars())
		{
			Label* newlabel = items.add(new Label);
			newlabel->setColour (Label::textColourId, lastColour);

			if (lineColours[i] != lastColour)
			{
				lastColour = lineColours[i];
				newlabel->setColour (Label::textColourId, lastColour);
			}

			colourCount = i % 2;
			newlabel->setColour(Label::backgroundColourId, colours[colourCount]);
			newlabel->setFont (Font (Font::getDefaultSansSerifFontName(), 12.0f, Font::plain));
			logContainer.addAndMakeVisible(newlabel);
			newlabel->setText(lines[i], sendNotification);
			resized();
		}
	}

	numNewLinesSinceUpdate = 0;
    viewport.setViewPosition(0, numLinesStored * 20);
}

// static members
int Console::originalStdout = -3;
int Console::originalStderr = -4;
int Console::logStdOutputPipe[2];
int Console::logErrOutputPipe[2];
std::unique_ptr<std::thread> Console::stdOutReaderThread;
std::unique_ptr<std::thread> Console::stdErrReaderThread;

Console* Console::currentStdOutTarget = nullptr;
Console* Console::currentStdErrTarget = nullptr;


