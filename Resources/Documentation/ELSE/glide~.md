---
title: glide~
description: signal glide/portamento

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs, Effects

arguments:
- type: float
  description: glide time in ms
  default: 0

inlets:
  1st:
  - type: float/signal
    description: input signal
  2nd:
  - type: float/signal
    description: glide time in ms

outlets:
  1st:
  - type: signal
    description: glided signal

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

[glide~] generates a glide/portamento from its signal input changes. The glide time in ms.

