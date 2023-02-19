---
title: rev2~
description: simple 1-in, 4-out reverberator
categories:
- object
see_also:
- rev1~
- rev3~
pdcategory: vanilla, Effects
last_update: '0.37'
inlets:
  1st:
  - type: signal
    description: reverb input
  2nd:
  - type: float
    description: level in dB
  3rd:
  - type: float
    description: liveness (internal feedback %)
  4th:
  - type: float
    description: crossover frequency in Hz
  5th:
  - type: float
    description: high frequency damping in %
outlets:
  1st:
  - type: signal
    description: first reverb output
  2nd:
  - type: signal
    description: second reverb output
  3rd:
  - type: signal
    description: third reverb output
  4th:
  - type: signal
    description: fourth reverb output
arguments:
- type: float
  description: level in dB 
  default: 0
- type: float
  description: liveness / internal feedback %
  default: 0
- type: float
  description: crossover frequency in Hz
  default: 3000
- type: float
  description: high frequency damping in %
  default: 0
draft: false
---

