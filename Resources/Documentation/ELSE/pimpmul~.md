---
title: pimpmul~
description: [pimp~] multiplier

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description: initial multiplier
  default: 0

inlets:
  1st:
  - type: signal
    description: phase signal to multiply
  2nd:
  - type: float/signal
    description: frequency multiplier
outlets:
  1st:
  - type: signal
    description: multiplied phase signal
  2nd:
  - type: signal
    description: impulse at period transitions

draft: false
---

[pimpmul~] is a "pimp~" multiplier. The [pimp~] object from ELSE is both a [phasor~] and an [imp~] oscillator. Use [pimpul~] to keep [phasor~] or [pimp~] objects in sync with different frequency ratios.

