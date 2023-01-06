---
title: bl.tri~

description: Bandlimited triangular oscillator

categories:
- object

pdcategory: Audio Oscillators and Tables

arguments:
1st:
- description: frequency in Hz
  type: float
  default: 0
2nd:
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
