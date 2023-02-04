---
title: slide~

description: smooth signals logarithmically

categories:
 - object

pdcategory: cyclone, Envelopes and LFOs

arguments:
- type: float
  description: initial slide up value in samples
  default: 1
- type: float
  description: initial slide down value in samples
  default: 1

inlets:
  1st:
  - type: signal
    description: signal to smooth out
  2nd:
  - type: float
    description: slide up value in samples
  3rd:
  - type: float
    description: slide down value in samples

outlets:
  1st:
  - type: signal
    description: smoothed signal

methods:
  - type: slide_down <float>
    description: set the slide down value
  - type: slide_up <float>
    description: set the slide up value
  - type: reset
    description: sets the current output sample value to 0  

draft: false
---

[slide~] filters an input signal logarithmically between changes in signal value. The formula is: y(n) = y(n-1) + ((x(n) - y(n-1))/slide).
Different than [rampsmooth~], [slide~] will smooth audio transitions in a non linear way. It's also useful for envelope following and lowpass filtering.
