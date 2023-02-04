---
title: rampsmooth~

description: smooth signals linearly

categories:
 - object

pdcategory: cyclone, Envelopes and LFOs

arguments:
- type: float
  description: initial ramp-uo time in samples
  default: 0
- type: float
  description: initial ramp-down time in samples
  default: 0

inlets:
  1st:
  - type: signal
    description: input signal to smooth out 
  2nd:
  - type: float
    description: ramp-up time in samples
  3rd:
  - type: float
    description: ramp-down time in samples

outlets:
  1st:
  - type: signal
    description: smoothed signal

methods:
  - type: ramp_down <float>
    description: set the ramp-down time in samples
  - type: ramp_up <float>
    description: set the ramp-up time in samples
  - type: ramp <float>
    description: set both ramp values 

draft: true
---

[rampsmooth~] smooths an incoming signal across a number 'n' of samples. Each time an incoming value changes, it begins a linear ramp of the given 'n' samples to reach this value. It is also useful for envelope following and lowpass filtering.
