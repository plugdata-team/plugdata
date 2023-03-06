---
title: saw~

description: sawtooth oscillator

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
  - type: signal
    description: frequency in Hz
  2nd:
  - type: signal
    description: phase sync (resets internal phase)
  3rd:
  - type: signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: sawtooth wave signal

draft: false
---

[saw~] is a sawtooth oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
