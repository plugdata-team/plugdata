---
title: bl.saw~

description: Bandlimited sawtooth oscillator

categories:
- object

pdcategory: Audio Oscillators and Tables

arguments:
  1st:
  - description:  frequency in Hz
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
    description: sawtooth wave signal

draft: false
---

[bl.saw~] is a sawtooth oscillator like [else/saw~], but it is bandlimited.
