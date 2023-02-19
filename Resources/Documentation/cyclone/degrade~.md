---
title: degrade~
description: signal quality reducer
categories:
 - object
pdcategory: cyclone, Effects
arguments:
- type: float
  description: sample rate
  default: 1
- type: float
  description: bit depth
  default: 24
inlets:
  1st:
  - type: signal
    description: signal to be degraded (resampled and quantized)
  2nd:
  - type: float/signal
    description: sample rate (0-1)
  3rd:
  - type: float/signal
    description: bit depth (1-24)
outlets:
  1st:
  - type: signal
    description: degraded (resampled and quantized) signal

draft: false
---

[degrade~] takes any given signal and reduces the sampling rate and bit-depth as specified/desired.

