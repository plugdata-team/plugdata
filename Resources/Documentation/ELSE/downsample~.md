---
title: downsample~

description: Downsample a signal

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: rate in hertz
  default: Pd's sample rate
- type: float
  description: interpolation 0 (off) or 1 (on)
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be downsampled
  - type: bang
    description: restart cycle and sync output
  2nd:
  - type: float/signal
    description: rate (in hertz) used to downsample the input signal

outlets:
  1st:
  - type: signal
    description: downsampled signal

methods:
  - type: interp <float>
    description: interpolation 0 (off, default) or 1 (on)

---

[downsample~] samples a signal received in the left inlet at a frequency rate in hertz. It can operate with or without interpolation.

