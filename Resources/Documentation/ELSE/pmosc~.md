---
title: pmosc~
description: phase modulation oscillator pair
categories:
 - object
pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: carrier frequency in Hz
  default: 0
- type: float
  description: modulation frequency in Hz
  default: 0
- type: float
  description: modulation index
  default: 0
- type: float
  description: phase input of modulator oscillator
  default: 0

inlets:
  1st:
  - type: float/signal
    description: carrier frequency in Hz
  2nd:
  - type: float/signal
    description: modulation frequency in Hz
  3rd:
  - type: float/signal
    description: modulation index
  4th:
  - type: float/signal
    description: phase input of modulator oscillator

outlets:
  1st:
  - type: signal
    description: phase modulation output

draft: false
---

[pmosc~] is a pair of sinusoidal oscillators, where one modulates the phase input of another.

