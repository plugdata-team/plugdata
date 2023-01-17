---
title: frameaccum~
description: FFT frame accumulation
categories:
 - object
pdcategory: General
arguments:
- type: float
  description: "1" enables phase wrapping between -PI and PI. "0" disables and values are accumulated without bounds
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

