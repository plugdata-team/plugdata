---
title: pitch.shift~

description: pitch shifter

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: transposition in cents
  type: float
  default: 0
- description: grain size in ms (minimum 5)
  type: float
  default: 75

inlets:
  1st:
  - type: signal
    description: input to pitch shifter
  2nd:
  - type: float
    description: transposition in cents
  3rd:
  - type: float
    description: grain size in ms

outlets:
  1st:
  - type: signal
    description: output of pitch shifter

draft: false
---

[pitch.shift~] is a pitch shifter abstraction based on granulation. You can set a transposition factor in cents and a grain size in ms.

