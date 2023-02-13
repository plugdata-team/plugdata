---
title: sine~

description: Sine oscillator

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
    description: sine wave signal

draft: false
---

[sine~] is a sinusoidal oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
