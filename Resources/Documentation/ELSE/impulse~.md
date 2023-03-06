---
title: impulse~, imp~
description: impulse oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: initial phase offset
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: impulse oscillator signal

draft: false
---

The [impulse~] object (or [imp~] for short) is an impulse oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.

