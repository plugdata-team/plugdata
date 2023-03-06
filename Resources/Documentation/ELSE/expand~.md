---
title: expand~

description: expander

categories:
- object

pdcategory: ELSE, Effects, Mixing and Routing

arguments:
- description: threshold in dB
  type: float
  default: -10
- description: attenuation ratio
  type: float
  default: 1
- description: attack time in ms
  type: float
  default: 10
- description: release time in ms
  type: float
  default: 10

inlets:
  1st:
  - type: signal
    description: input signal

outlets:
  1st:
  - type: signal
    description: compressed signal

methods:
  - type: thresh <float>
    description: sets threshold in dB
  - type: ratio <float>
    description: sets attenuation ratio
  - type: attack <float>
    description: sets attack time in ms
  - type: release <float>
    description: sets release time in ms

draft: false
---

[expand~] is an abstraction that performs expanding. It attenuates an input signal below a given threshold.

