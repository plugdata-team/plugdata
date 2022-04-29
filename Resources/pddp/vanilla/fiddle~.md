---
title: fiddle~
description: pitch estimator and sinusoidal peak finder
categories:
- object
see_also:
- sigmund~
pdcategory: "Obsolete"
last_update: '?'
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed.
  - type: amp-range <float, float>
    description: set low and high amplitude threshold
  - type: vibrato <float, float>
    description:  set period in msec and pitch deviation for "cooked" pitch outlet.
  - type: reattack <float, float>
    description:  set period in msec and amplitude in dB to report re-attack.
  - type: npartial <float>
    description:  set partial number to be weighted half as strongly as the fundamental.
  - type: auto <float>
    description: nonzero sets to auto mode (default) and zero sets it off.
  - type: bang
    description: poll current values (useful when auto mode is off).
  - type: npoints <float>
    description:  sets number of points in analysis window.
  - type: print <float>
    description:  print out settings.
outlets:
  1st:
  - type: float
    description: cooked pitch output.
  2nd:
  - type: bang
    description: a bang is sent on new attacks.
  3rd:
  - type: list
    description: continuous raw pitch and amplitude.
  4th:
  - type: float
    description: amplitude in dB.
  5th:
  - type: list
    description: peak number, frequency and amplitude.
arguments:
  - type: float
    description: window size (128-2048, default 1024)
  - type: float
    description: number of pitch outlets (1-3, default 1)
  - type: float
    description: number of peaks to find (1-100, default 20)
  - type: float
    description: number of peaks to output (default 0)
draft: false
---
NOTE: fiddle~ is obsolete! consider using [sigmund~]

The Fiddle object estimates the pitch and amplitude of an incoming sound, both continuously and as a stream of discrete "note" events. Fiddle optionally outputs a list of detected sinusoidal peaks used to make the pitch determination. Fiddle is described theoretically in the 1998 ICMC proceedings, reprinted on msp.ucsd.edu.

Fiddle's creation arguments specify an analysis window size, the maximum polyphony (i.e., the number of simultaneous "pitches" to try to find), the number of peaks in the spectrum to consider, and the number of peaks, if any, to output "raw." The outlets give discrete pitch (a number), detected attacks in the amplitude envelope (a bang), one or more voices of continuous pitch and amplitude, overall amplitude, and optionally a sequence of messages with the peaks.

The analysis hop size is half the window size so in the example shown here, one analysis is done every 512 samples (11.6 msec at 44K1), and the analysis uses the most recent 1024 samples (23.2 msec at 44K1). The minimum frequency that Fiddle will report is 2-1/2 cycles per analysis windows, or about 108 Hz. (just below MIDI 45.)