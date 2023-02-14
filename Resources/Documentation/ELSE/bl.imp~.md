---
title: bl.imp~

description: bandlimited two-sided impulse oscillator

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
  - type: float/signal
    description: phase sync (reset internal phase)
  - type: float/signal
    description: phase offset (modulation input)
    
outlets:
  1st:
  - type: signal
    description: two sided impulse signal

draft: false
---

[bl.imp~] is an impulse oscillator like [else/imp~], but it is bandlimited.
