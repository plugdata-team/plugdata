---
title: fbsine~
description: feedback sine oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: initial feedback value
  default: 0
- type: float
  description: initial phase offset
  default: 0
- type: float
  description: mean filter: on <1> or off <0>
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: feedback value
  3rd:
  - type: float/signal
    description: phase sync (resets internal phase)
  4th:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: sine wave signal

methods:
  - type: filter <float>
    description: turns mean filter on <1> or off <0>

draft: false
---

[fbsine~] is a sinusoidal oscillator with phase modulation feedback. Like [sine~], it accepts negative frequencies, has inlets for phase modulation and phase sync. Additionally, it has a feedback value.

