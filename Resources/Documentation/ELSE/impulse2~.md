---
title: impulse2~, imp2~
description: two-sided impulse oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: initial width
  default: 0.5
- type: float
  description: initial phase offset
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: pulse width (from 0 to 1)
  3rd:
  - type: float/signal
    description: phase sync (resets internal phase)
  4th:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: sided impulse signal

draft: false
---

A variant of [impulse~], [impulse2~] (or [imp2~] for short) is a two-sided impulse oscillator that accepts negative frequencies, has inlets for pulse width, phase sync and phase modulation.

