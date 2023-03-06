---
title: hv.flanger2~

description: stereo flanger effect unit

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
    description: left input signal
  2nd:
  - type: signal
    description: right input signal
  3rd:
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
    description: left output
  2nd:
  - type: signal
    description: right output

draft: false
---


