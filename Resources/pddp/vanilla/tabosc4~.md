---
title: tabosc4~
description: 4-point interpolating oscillator
categories:
- object
see_also:
- osc~
- phasor~
- tabwrite~
- tabread4~
- array
pdcategory: Audio Oscillators And Tables
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: frequency value in Hz.
  - type: set <symbol>
    description: set table name with the waveform.
  2nd:
  - type: float
    description: phase cycle reset (from 0 to 1).
outlets:
  1st:
  - type: signal
    description: wavetable oscillator output.
arguments:
  - type: float
    description: initial frequency value in Hz (default 0).
draft: false
---
tabosc4~ is a traditional computer music style wavetable lookup oscillator using 4-point polynomial interpolation. The table should have a power of two points plus three "guard points", one at the beginning and two at the end, which should be wraparound copies of the last point and the first two points, respectively. The "sinesum" and "cosinesum" methods for arrays do this automatically for you if you just want to specify partial strengths.

For good results use 512 points for up to about 15 partials, or 32*npartials (rounded up to a power of 2) for more than 15 partials. Don't send new "sinesum" messages to tables while you're running - instead, use "set" messages to switch between tables.

The frequency input can be either a float or a signal. The right inlet resets the phase with values from 0 to 1 (where '1' is the same as '0' and '0.5' is half the cycle).