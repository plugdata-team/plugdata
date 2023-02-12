---
title: duck~

description: sidechain compression

categories:
- object

pdcategory: ELSE, Effects, Mixing and Routing

arguments:
- description: threshold in dB
  type: float
  default: -60
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
  2nd:
  - type: signal
    description: control signal

outlets:
  1st:
  - type: signal
    description: ducked signal alone or mixed with control signal

flags:
  - name: -mix
    description: mixes with control signal

methods:
  - type: thresh <float>
    description: sets threshold in dB
  - type: ratio <float>
    description: sets attenuation ratio
  - type: attack <float>
    description: sets attack time in ms
  - type: release <float>
    description: sets release time in ms
  - type: mix <float>
    description: non-0 mixes with control signal


draft: false
---

[duck~] is an abstraction that performs a duck volume effect (a.k.a sidechain compression). It attenuates an input signal according to the level of a control signal. you can also mix the ducked signal with the control input signal.

