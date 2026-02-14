//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : driver/asiosample/mactimer.cpp
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Sample
// 	sample implementation of asio.
// 	time manager irq
//	**** macOS specific ****
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2025, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include <macheaders.h>
#include <Timer.h>
#include <string.h>
#include "asiosmpl.h"

static TMTask myTmTask;
static bool tmTaskOn = false;
static AsioSample *theSound = 0;

void myTimer();
void myTimer()
{
	if(theSound)
		theSound->bufferSwitch();
	PrimeTime((QElemPtr)&myTmTask, theSound->milliSeconds);
}

void AsioSample::timerOn()
{
	theSound = this;	// for irq
	if(!tmTaskOn)
	{
		memset(&myTmTask, 0, sizeof(TMTask));
		myTmTask.tmAddr = NewTimerProc(myTimer);
		myTmTask.tmWakeUp = 0;
		tmTaskOn = true;
		InsXTime((QElemPtr)&myTmTask);
		PrimeTime((QElemPtr)&myTmTask, theSound->milliSeconds);
	}
}

void AsioSample::timerOff()
{
	if(tmTaskOn)
	{
		RmvTime((QElemPtr)&myTmTask);
		tmTaskOn = false;
	}
	theSound = 0;
}
