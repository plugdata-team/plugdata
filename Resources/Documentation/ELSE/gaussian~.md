---
title: gaussian~
description: Gaussian oscillator

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: frequency in hertz
  default: 0
- type: float
  description: initial width
  default: 0
- type: float
  description: initial phase offset
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in hz
  2nd:
  - type: float/signal
    description: width
  3rd:
  - type: float/signal
    description: phase sync (ressets internal phase)
  4th:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: gaussian wave signal

---

[gaussian~] is a gaussian oscillator. It accepts negative frequencies, has inlets for width, phase sync and phase modulation.

