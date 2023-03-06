---
title: bucket
description: pass numbers from outlet to outlet
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets number of outlets
  default: 1
- type: float
  description: <0> holds the input and passes it on the next round, <1> outputs the inputs immediately
  default: 0
inlets:
  1st:
  - type: float
    description: the input number
  - type: bang
    description: outputs all stored values without shifting
outlets:
  nth:
  - type: float
    description: numbers being passed

methods:
  - type: set <float>
    description: sets input value and outputs it to all outlets
  - type: freeze
    description: suspends output, but continues to shift input internally
  - type: thaw
    description: resumes the output after being freezed
  - type: l2r
    description: or 'L2R'/'ltor': sets shift from left to right (default)
  - type: r2l
    description: or 'R2L'/'rtol': sets shift from right to left
  - type: roll
    description: shifts stores values in loop (according to direction)

draft: false
---

As [bucket] is input with floats, it outputs them shifting from outlet to outlet in a rotational pattern (or "passing the bucket" as in a bucket brigade).

