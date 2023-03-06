---
title: parabolic~

description: parabolic oscillator

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
    description: parabolic wave signal

draft: false
---

[parabolic~] is a parabolic oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation. A parabolic waveform is similar to a sinusoid but it is not band limited (has aliasing).

