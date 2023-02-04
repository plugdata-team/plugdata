---
title: bl.wavetable~

description: bandlimited wavetable oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: array name (required)
    type: symbol
    default: none
  - description: sets frequency in Hz
    type: float
    default: 0
  - description: sets phase offset
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
    description: a periodically repeating waveform

methods:
  - type: set <symbol>
    description: sets an entire array to be used as a waveform

draft: false
---

[bl.wavetable~] is a wavetable oscillator like [else/wavetable~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator. This object also minimizes aliasing caused by hard sync and phase modulation.
