---
title: accum

description: accumulate to a value

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: sets initial value
  default: 0
inlets:
  1st:
  - type: bang
    description: output accumulated value
  - type: float
    description: sets a new value to be accumulated and outputs it
  2nd:
  - type: float
    description: add to current value
  3rd:
  - type: float
    description: multiply with current value
outlets:
  1st:
  - type: float
    description: accumulated value

methods:
  - type: set <float>
    description: sets a new value to be accumulated

draft: false
---

[accum] accumulates to a value by either adding an increment value to it or multiplying it by a given factor.

