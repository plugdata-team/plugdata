---
title: timed.gate

description: control timed gate

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - type: float
    description: gate time in ms
    default: 0, no gate
  - type: float
    description: initial gate amplitude
    default: 1
  - type: float
    description: non-0 sets to retrigger mode
    default: 0

inlets:
  1st:
  - type: float
    description: control trigger with the gate value
  - type: bang
    description: control trigger with the last/initial gate value

  2nd:
  - type: float
    description: gate time in ms

outlets:
  1st:
  - type: float
    description: timed gate

methods:
  - type: ms <float>
    description: gate time in ms
  - type: retrigger <float>
    description: non-0 sets to retrigger mode

draft: false
---

When receiving a bang or a float, [timed.gate] sends a timed gate (with the value of the float for the given duration, 0 otherwise).
