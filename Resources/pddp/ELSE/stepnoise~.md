---
title: stepnoise~

description: Step noise

categories:
 - object
 
pdcategory: Noise 

arguments:
- type: float
  description: frequency in hertz
  default: 0

flags:
- name: -seed <f>
  description: sets seed (default: unique internal)

inlets:
  1st:
  - type: float/signal
    description: frequency input in hertz
  - type: seed <f>
    description: a float sets seed, no float sets a unique internal

outlets:
  1st:
  - type: signal
    description: bandlimited step noise

draft: false
---

[stepnoise~] is a low frequency (band-limited) noise generator without interpolation; therefore, it holds at a random value, resulting in random steps. Random values are between -1 and 1 at a rate in hertz (negative frequencies accepted). Use the seed message if you want a reproducible output.
The [stepnoise~] object produces frequencies limited to a band from 0 up to the frequency it is running. It can go up to the sample rate, and when that happens, it's basically a white noise generator.
