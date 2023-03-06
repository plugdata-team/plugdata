---
title: hv.flanger~

description: mono flanger effect unit

categories:
- object

pdcategory: heavylib, Effects

arguments:
- type: float
  description: delay time
  default: 

inlets:
  1st:
  - type: signal
    description: input signal
  2nd:
  - type: mix <float>
    description: dry/wet (0 to 1)
  - type: feedback <float>
    description: -1 to 1
  - type: speed <float>
    description: 0 to 20 Hz
  - type: intensity <float>
    description: 0 to 1

outlets:
  1st:
  - type: signal
    description: flanger output

draft: false
---

