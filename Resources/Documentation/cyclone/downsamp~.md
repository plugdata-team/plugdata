---
title: downsamp~
description: downsample a signal
categories:
 - object
pdcategory: cyclone, Effects
arguments:
- type: float
  description: rate as number of samples
  default: 1
inlets:
  1st:
  - type: signal
    description: signal to be downsampled
  2nd:
  - type: float/signal
    description: rate (in samples) used to downsample the input signal
outlets:
  1st:
  - type: signal
    description: downsampled signal

---

[downsamp~] samples and holds a signal received in the left inlet at a rate expressed in samples. No interpolation of the output is performed.

