---
title: lag2~

description: non-linear lag

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: lag uptime in ms
  default: 0
- type: float
  description: lag downtime in ms
  default: 0

inlets:
  1st:
  - type: float/signal
    description: input signal
  2nd:
  - type: float/signal
    description: lag uptime in ms
  3rd:
  - type: float/signal
    description: lag downtime in ms

outlets:
  1st:
  - type: signal
    description: lagged signal

methods:
  - type: reset
    description: resets the filter

draft: false
---

[lag2~] is like [lag~] but has a different time for ramp up and ramp down. The lag time in ms is how long it takes for the signal to converge to within 0.01% of the target value. Note that the rising ramp is different than the descending ramp, unlike [glide2~].

