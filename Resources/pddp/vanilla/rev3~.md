---
title: rev3~
description: hard-core, 2-in, 4-out reverberator
categories:
- object
see_also:
- rev1~
- rev2~
pdcategory: Extra
last_update: '0.37.1'
inlets:
  1st:
  - type: signal
    description: left channel reverb input.
  2nd:
  - type: signal
    description: right channel reverb input.
  3rd:
  - type: float
    description: level in dB.
  4th:
  - type: float
    description: liveness (internal feedback percentage).
  5th:
  - type: float
    description: Crossover frequency in Hz.
  6th:
  - type: float
    description: High Freuqnecy damping in percentage.
outlets:
  1st:
  - type: signal
    description: first reverb output.
  2nd:
  - type: signal
    description: second reverb output.
  3rd:
  - type: signal
    description: third reverb output.
  4th:
  - type: signal
    description: fourth reverb output.
arguments:
- type: float
  description: level in dB 
  default: 0
- type: float
  description: liveness / internal feedback percentage 