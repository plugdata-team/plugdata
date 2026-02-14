//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : driver/asiosample/asiosmpl.h
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Sample
//	test implementation of ASIO
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

#ifndef _asiosmpl_
#define _asiosmpl_

#include "asiosys.h"

#define TESTWAVES 1
// when true, will feed the left input (to host) with
// a sine wave, and the right one with a sawtooth

enum
{
	kBlockFrames = 256,
	kNumInputs = 16,
	kNumOutputs = 16
};

#if WINDOWS

#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include "ole2.h"
#endif

#include "combase.h"
#include "iasiodrv.h"

class AsioSample : public IASIO, public CUnknown
{
public:
	AsioSample(LPUNKNOWN pUnk, HRESULT *phr);
	~AsioSample();

	DECLARE_IUNKNOWN
    //STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {      \
    //    return GetOwner()->QueryInterface(riid,ppv);            \
    //};                                                          \
    //STDMETHODIMP_(ULONG) AddRef() {                             \
    //    return GetOwner()->AddRef();                            \
    //};                                                          \
    //STDMETHODIMP_(ULONG) Release() {                            \
    //    return GetOwner()->Release();                           \
    //};

	// Factory method
	static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid,void **ppvObject);
#else

#include "asiodrvr.h"

//---------------------------------------------------------------------------------------------
class AsioSample : public AsioDriver
{
public:
	AsioSample ();
	~AsioSample ();
#endif

	ASIOBool init (void* sysRef);
	void getDriverName (char *name);	// max 32 bytes incl. terminating zero
	long getDriverVersion ();
	void getErrorMessage (char *string);	// max 128 bytes incl.

	ASIOError start ();
	ASIOError stop ();

	ASIOError getChannels (long *numInputChannels, long *numOutputChannels);
	ASIOError getLatencies (long *inputLatency, long *outputLatency);
	ASIOError getBufferSize (long *minSize, long *maxSize,
		long *preferredSize, long *granularity);

	ASIOError canSampleRate (ASIOSampleRate sampleRate);
	ASIOError getSampleRate (ASIOSampleRate *sampleRate);
	ASIOError setSampleRate (ASIOSampleRate sampleRate);
	ASIOError getClockSources (ASIOClockSource *clocks, long *numSources);
	ASIOError setClockSource (long index);

	ASIOError getSamplePosition (ASIOSamples *sPos, ASIOTimeStamp *tStamp);
	ASIOError getChannelInfo (ASIOChannelInfo *info);

	ASIOError createBuffers (ASIOBufferInfo *bufferInfos, long numChannels,
		long bufferSize, ASIOCallbacks *callbacks);
	ASIOError disposeBuffers ();

	ASIOError controlPanel ();
	ASIOError future (long selector, void *opt);
	ASIOError outputReady ();

	void bufferSwitch ();
	long getMilliSeconds () {return milliSeconds;}

private:
friend void myTimer();

	bool inputOpen ();
#if TESTWAVES
	void makeSine (short *wave);
	void makeSaw (short *wave);
#endif
	void inputClose ();
	void input ();

	bool outputOpen ();
	void outputClose ();
	void output ();

	void timerOn ();
	void timerOff ();
	void bufferSwitchX ();

	double samplePosition;
	double sampleRate;
	ASIOCallbacks *callbacks;
	ASIOTime asioTime;
	ASIOTimeStamp theSystemTime;
	short *inputBuffers[kNumInputs * 2];
	short *outputBuffers[kNumOutputs * 2];
#if TESTWAVES
	short *sineWave, *sawTooth;
#endif
	long inMap[kNumInputs];
	long outMap[kNumOutputs];
	long blockFrames;
	long inputLatency;
	long outputLatency;
	long activeInputs;
	long activeOutputs;
	long toggle;
	long milliSeconds;
	bool active, started;
	bool timeInfoMode, tcRead;
	char errorMessage[128];
};

#endif
