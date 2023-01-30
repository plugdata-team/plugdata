---
title: bl.tri~

description: bandlimited triangular oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
- description: frequency in Hz
  type: float
  default: 0
- description: initial phase offset
  type: float
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
    description: triangular wave signal

draft: false
---

[bl.tri~] is a triangular oscillator like [else/tri~], but it is bandlimited.
