---
title: osc~
description: cosine wave oscillator
categories:
- object
see_also:
- phasor~
- cos~
- tabread4~
pdcategory: vanilla, Signal Generators
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: frequency value in Hz
  2nd:
  - type: float
    description: phase cycle reset (from 0 to 1)
outlets:
  1st:
  - type: signal
    description: cosine waveform (in the range of -1 to 1)
arguments:
  - type: float
    description: initial frequency value in Hz 
  default: 0
draft: false
---
The osc~ object outputs a cosine wave. The frequency input can be either a float or a signal. The right inlet resets the phase with values from 0 to 1 (where '1' is the same as '0' and '0.5' is half the cycle
