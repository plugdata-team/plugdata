---
title: phasor~
description: phase ramp generator
categories:
- object
see_also:
- osc~
- cos~
- tabread4~
pdcategory: vanilla, Signal Generators, Envelopes and LFOs
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
    description: phase ramp (in the range of 0 to 1)
arguments:
  - type: float
    description: initial frequency value in Hz 
  default: 0
draft: false
---
The phasor~ object outputs phase ramps whose values are from 0 to 1 and it repeats this cycle depending on the frequency input. It looks like a sawtooth signal but it's traditionally used for table lookup via cos~ or tabread4~. The frequency input can be either a float or a signal. Positive frequency values generate upwards ramps and negative values generate downwards ramps.

The right inlet resets the phase with values from 0 to 1 (where '1' is the same as '0' and '0.5' is half the cycle
