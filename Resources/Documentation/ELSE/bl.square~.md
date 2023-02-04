---
title: bl.square~

description: bandlimited square oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description:  frequency in Hz
    type: float
    default: 0
  - description:  initial pulse width
    type: float
    default: 0.5
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
    description: square wave signal

draft: false
---

[bl.square~] is a square oscillator like [else/square~], but it is bandlimited.
