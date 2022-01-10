/*
 ==============================================================================
 Copyright (c) 2017 - 2020 Foleys Finest Audio Ltd. - Daniel Walz
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.
 ==============================================================================

 BEGIN_JUCE_MODULE_DECLARATION

 ID:            ff_meters
 vendor:        Foleys Finest Audio UG
 version:       0.9.1
 name:          Meters with GUI and LookAndFeel
 description:   Contains a metering Component, that can display live peak and RMS values 
 dependencies:  juce_audio_basics, juce_gui_basics, juce_events
 website:       http://www.foleysfinest.com/
 license:       BSD V2 3-clause
 linuxLibs:     m

 END_JUCE_MODULE_DECLARATION

 ==============================================================================
 
 @defgroup ff_meters
 
 This module provides a display component to show RMS, MAX and CLIP values.
 There is a MeterInputSource, which can process any AudioBuffer, and provide
 therefore the data for live display.
 
 ==============================================================================
 */

#pragma once

#ifndef USE_FF_AUDIO_METERS
#define USE_FF_AUDIO_METERS 1
#endif

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>

#include <atomic>
#include <vector>
#include <numeric>

#include "LevelMeter/LevelMeterSource.h"
#include "LevelMeter/LevelMeter.h"
#include "Visualisers/OutlineBuffer.h"
#include "Visualisers/StereoFieldBuffer.h"
#include "Visualisers/StereoFieldComponent.h"
#include "LookAndFeel/LevelMeterLookAndFeel.h"

// stay backwards compatible
namespace FFAU=foleys;
