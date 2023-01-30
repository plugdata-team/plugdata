---
title: interpolate

description: interpolate between values

categories:
- object

pdcategory: ELSE, Data Math

arguments:
- description: sets start value(s)
  type: list
  default: 0

inlets:
  1st:
  - type: float
    description: interpolation value between 0 and 1
  2nd:
  - type: list
    description: sets target values

outlets:
  1st:
  - type: list
    description: interpolated values

flags:
  - name: -exp <float>
    description: sets exponential factor (default 1)

methods:
  - type: start <list>
    description: set start value(s)
  - type: target <list>
    description: set target value(s)
  - type: exp <float>
    description: sets exponential factor

draft: false
---

[interpolate] allows you to use a GUI like a slider (or a MIDI controller) to interpolate between values. Whenever you change the target, you can interpolate to it by moving the slider to the opposite end.

