---
title: bl.imp2~

description: bandlimited two-sided impulse oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - type: float
    description: frequency in Hz
    default: 0
  - type: float
    description: initial width
    default: 0.5
  - type: float
    description: initial phase offset
    default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  - type: float/signal
    description: pulse width (0-1)
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

[bl.imp2~] is a two sided impulse oscillator like [else/imp2~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator. 

