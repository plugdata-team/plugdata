---
title: vibrato~

description: Vibrato

categories:
- object

pdcategory: General

arguments:
  1st:
  - type: float
    description: rate in hertz (default: 0)
  2nd:
  - type: float
    description: transposition depth in cents (default: 0)

inlets:
  1st:
  - type: signal
    description: input to vibrato
  2nd:
  - type: float
    description: rate in hertz
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
