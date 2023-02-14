---
title: rampnoise~

description: ramp noise

categories:
 - object

pdcategory: ELSE, Random and Noise

arguments:
- type: float
  description: frequency in Hz
  default: 0

inlets: 
  1st:
  - type: float
    description: frequency in Hz

outlets:
  1st:
  - type: signal
    description: bandlimited step noise

flags:
  - name: -seed <float>
    description: sets seed

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rampnoise~] is a low frequency (band limited) noise generator with interpolation, therefore it ramps from one random value to another, resulting in random ramps. Random values are between -1 and 1 at a rate in Hz (negative frequencies accepted). Use the seed message if you want a reproducible output.
The [rampnoise~] object produces frequencies limited to a band from 0 up to the frequency it is running. It can go up to the sample rate, and when that happens, it's basically a white noise generator.
