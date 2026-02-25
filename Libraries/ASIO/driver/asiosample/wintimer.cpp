//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : driver/asiosample/wintimer.cpp
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Sample
//	**** Windows specific ****
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

#include <windows.h>
#include "asiosmpl.h"

static DWORD __stdcall ASIOThread (void *param);
static HANDLE ASIOThreadHandle = 0;
static bool done = false;
const double twoRaisedTo32 = 4294967296.;

AsioSample* theDriver = 0;

//------------------------------------------------------------------------------------------
void getNanoSeconds (ASIOTimeStamp* ts)
{
	double nanoSeconds = (double)((unsigned long)timeGetTime ()) * 1000000.;
	ts->hi = (unsigned long)(nanoSeconds / twoRaisedTo32);
	ts->lo = (unsigned long)(nanoSeconds - (ts->hi * twoRaisedTo32));
}

//------------------------------------------------------------------------------------------
void AsioSample::timerOn ()
{
	theDriver = this;
	DWORD asioId;
	done = false;
	ASIOThreadHandle = CreateThread (0, 0, &ASIOThread, 0, 0, &asioId);
}

//------------------------------------------------------------------------------------------
void AsioSample::timerOff ()
{
	done = true;
	if (ASIOThreadHandle)
		WaitForSingleObject(ASIOThreadHandle, 1000);
	ASIOThreadHandle = 0;
}

//------------------------------------------------------------------------------------------
static DWORD __stdcall ASIOThread (void *param)
{
	do
	{
		if (theDriver)
		{
			theDriver->bufferSwitch ();
			Sleep (theDriver->getMilliSeconds ());
		}
		else
		{
			double a = 1000. / 44100.;
			Sleep ((long)(a * (double)kBlockFrames));

		}
	} while (!done);
	return 0;
}
