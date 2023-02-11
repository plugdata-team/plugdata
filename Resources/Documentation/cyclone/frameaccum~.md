---
title: frameaccum~
description: FFT frame accumulation
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: ''1' wrap phase between -PI and PI. '0' — accumulate values without bounds
  default: 0
inlets:
  1st:
  - type: signal
    description: incoming FFT frame
outlets:
  1st:
  - type: signal
    description: accumulated FFT frame

---

[frameaccum~] was specially designed to compute a running phase of FFT frames by keeping an accumulated sum of the values in each sample of an incoming signal block.

