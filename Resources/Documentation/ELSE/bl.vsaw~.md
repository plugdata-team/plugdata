---
title: bl.vsaw~

description: bandlimited sawtooth-triangular oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: frequency in Hz
    type: float
    default: 0
  - description: initial width
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
    description: variable sawtooth-triangle wave signal

draft: false
---

[bl.vsaw~] is a variable sawtooth waveform oscillator that also becomes a triangular osccilator just like [else/vsaw~], but it is bandlimited.
