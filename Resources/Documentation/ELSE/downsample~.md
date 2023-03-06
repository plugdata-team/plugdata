---
title: downsample~

description: downsample a signal

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: rate in Hz
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
    description: rate (in Hz) used to downsample the input signal

outlets:
  1st:
  - type: signal
    description: downsampled signal

methods:
  - type: interp <float>
    description: interpolation 0 (off, default) or 1 (on)

draft: false
---

[downsample~] samples a signal received in the left inlet at a frequency rate in Hz. It can operate with or without interpolation.

