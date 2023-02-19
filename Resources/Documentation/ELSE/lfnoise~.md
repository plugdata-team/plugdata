---
title: lfnoise~

description: low frequency noise

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators, Envelopes and LFOs

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: interpolation 0 (off) or 1 (on)
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency input in Hz
  2nd:
  - type: signal
    description: impulse forces a new random value

outlets:
  1st:
  - type: signal
    description: impulse forces a new random value

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: interp <float>
    description: interpolation 0 (off, default) or 1 (on)
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[lfnoise~] is a low frequency (band limited) noise with or without interpolation. It generates random values (between -1 and 1) at a rate in Hz (negative frequencies accepted). It also has a 'sync' inlet that forces the object to generate a new random value when receiving an impulse. Use the seed message if you want a reproducible output.

