---
title: timed.gate~

description: Signal timed gate

categories:
- object

pdcategory:

arguments:
  1st:
  - type: float
    description: gate time in ms
    default: 0, no gate
  2nd:
  - type: float
    description: initial gate amplitude
    default: 1
  3rd:
  - type: float
    description: non-zero sets to retrigger mode
    default: 0

inlets:
  1st:
  - type: signal
    description: impulse trigger with the gate value
  - type: float
    description: control trigger with the gate value
  - type: bang
    description: control trigger with the last/initial gate value
  - type: ms <float>
    description: gate time in ms
  - type: retrigger <float>
    description: non-zero sets to retrigger mode
  2nd:
  - type: float/signal
    description: gate time in ms

outlets:
  1st:
  - type: signal
    description: timed gate

draft: false
---

When receiving an impulse or a float, [timed.gate~] sends a timed gate (with the value of the impulse/float for the given duration, 0 otherwise). The length of the gate is given in ms, if 0 or less is given, no gate is output.