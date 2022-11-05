---
title: square~

description: Square oscillator

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: frequency in hertz
  default: 0
- type: float
  description: initial pulse width
  default: 0.5
- type: float
  description: initial phase offset
  default: 0


inlets:
  1st:
  - type: float/signal
    description: frequency in hz
  2nd:
  - type: float/signal
    description: pulse width (0-1)
  3rd:
  - type: float/signal
    description: phase sync (resets internal phase)
  4th:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: square wave signal

draft: false
---

[square~] is a square oscillator that accepts negative frequencies, has inlets for pulse width, phase sync and phase modulation.
