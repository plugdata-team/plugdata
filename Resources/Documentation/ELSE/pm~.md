---
title: pm~

description: phase modulation unit

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: carrier frequency in hertz
  default: 0
- type: float
  description: modulation frequency in hertz or ratio
  default: 0
- type: float
  description: modulation index
  default: 0
- type: float
  description: phase input of modulator oscillator
  default: 0

inlets:
  1st:
  - description: carrier frequency in hz
    type: signal
  2nd:
  - description: modulation frequency in hz
    type: signal
  3rd:
  - description: modulation index
    type: signal
  4th:
  - description: phase input of modulator oscillator
    type: signal

outlets:
  1st:
  - description: phase modulation output
    type: signal

flags:
  - name: -ratio
    description: sets to ratio mode

methods:
  - type: ratio <float>
    description: non zero sets to ratio mode

draft: false
---

[pm~] is a very basic and simple phase modulation unit, which consists of a pair of sinusoidal oscillators, where one modulates the phase input of another.