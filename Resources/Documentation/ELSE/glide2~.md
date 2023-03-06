---
title: glide2~
description: signal glide/portamento

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs, Effects

arguments:
- type: float
  description: ramp-up time in ms
  default: 0
- type: float
  description: ramp-down time in ms
  default: 0

inlets:
  1st:
  - type: signal
    description: incoming signal to smooth out
  2nd:
  - type: float
    description: ramp-up time in ms
  3rd:
  - type: float
    description: ramp-down time in ms

outlets:
  1st:
  - type: signal
    description: smoothed out/filtered signal

flags:
  - name: -exp <float>
    description: sets exponential factor (default 1 - linear)

methods:
  - type: reset
    description: resets glide to input value
  - type: exp <float>
    description: sets exponential factor

draft: false
---

[glide2~] is just like [glide~] but has distinct ramp times for both up and down.

