---
title: vibrato~

description: vibrato

categories:
- object

pdcategory: ELSE, Effects

arguments:
  - type: float
    description: rate in Hz 
    default: 0
  - type: float
    description: transposition depth in cents 
    default: 0

inlets:
  1st:
  - type: signal
    description: input to vibrato
  2nd:
  - type: float
    description: rate in Hz
  3rd:
  - type: float
    description: transposition depth in cents

outlets:
  1st:
  - type: signal
    description: output of vibrato

draft: false
---

[vibrato~] is a pitch shifter abstraction with an LFO controlling the pitch deviation in cents.
