---
title: bl.imp2~

description: Bandlimited two-sided impulse oscillator

categories:
- object

pdcategory: Audio Oscillators and Tables

arguments:
  1st:
  - description: frequency in Hz
    type: float
    default: 0
  2nd:
  - description:
    type: float
    default: .5
  3rd:
  - description: initial phase offset (0 to 1)
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
    description: two sided impulse signal

draft: false
---

[bl.imp2~] is a two sided impulse oscillator like [else/imp2~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator. 

TODO: Because this is an abstraction and Pd is still limited in the way it handles it, this object does not have any arguments.
