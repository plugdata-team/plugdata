---
title: glide2

description: control glide/portamento

categories:
- object

pdcategory: ELSE, Envelopes and LFOs, Effects

arguments:
- description: glide up and down time in ms
  type: list
  default: 0 0

inlets:
  1st:
  - type: float
    description: input signal
  2nd:
  - type: float
    description: glide up time in ms
  3rd:
  - type: float
    description: glide down time in ms

outlets:
  1st:
  - type: signal
    description: glided signal

flags:
  - name: -exp <float>
    description: sets exponential factor (default 1 - linear)
  - name: -rate <float>
    description: sets refresh rate in ms (default 5, minimum 1)

methods:
  - type: reset
    description: resets glide to input value
  - type: exp <float>
    description: sets exponential factor
  - type: rate <float>
    description: refresh rate in ms

draft: false
---

[glide2] generates a glide/portamento from its float input changes. The glide time in ms.

