---
title: delay~
description: delay a signal
categories:
 - object
pdcategory: cyclone, Effects
arguments:
- type: float
  description: sets the maximum delay in samples
  default: 512
- type: float
  description: sets the initial delay in samples
  default: 0
inlets:
  1st:
  - type: signal
    description: the signal to be delayed
  2nd:
  - type: float
    description: delay in samples (no interpolation)
  - type: signal
    description: delay in samples (with interpolation)
outlets:
  1st:
  - type: signal
    description: the delayed signal

methods:
  - type: clear
    description: clears delay buffer
  - type: maxsize <float>
    description: sets a new maximum delay size in samples
  - type: ramp <float>
    description: sets a ramp time to a new target delay value in ms

---

[delay~] delays a signal by a number of samples (thus the delay time depends on the sample rate).

