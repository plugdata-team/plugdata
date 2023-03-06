---
title: lag~

description: non-linear lag

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: lag time in ms
  default: 0

inlets:
  1st:
  - type: float/signal
    description: input signal
  2nd:
  - type: float/signal
    description: lag time in ms

outlets:
  1st:
  - type: signal
    description: lagged signal

methods:
  - type: reset
    description: resets the filter

draft: false
---

[lag~] is a one pole filter that creates an exponential glide/portamento for its signal input changes. The lag time in ms is how long it takes for the signal to converge to within 0.01% of the target value. Note that the rising ramp is different than the descending ramp, unlike [glide~]. the same kind of filter is used in other objects from the ELSE library (decay~/asr~/adsr~).

