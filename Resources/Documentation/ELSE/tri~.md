---
title: tri~

description: Triangular wave oscillator

categories:
 - object

pdcategory: General

arguments:
  1st:
  - type: float
    description: frequency in hertz
    default: 0
  2nd:
  - type: float
    description: initial phase offset
    default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in hz
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

[tri~] is a triangular wave oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
