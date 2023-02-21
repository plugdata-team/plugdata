---
title: fold~
description: fold signals between two values

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- type: list
  description: 2 floats set min and max, 1 float sets max value (min to 0)
  default: -1 1

inlets:
  1st:
  - type: signal
    description: input values to be folded
  2nd:
  - type: float/signal
    description: lowest fold value
  3rd:
  - type: float/signal
    description: highest fold value
outlets:
  1st:
  - type: signal
    description: folded values

draft: false
---

[fold~] folds between a low and high value. This is like a mirroring function, where an out of bounds value folds back until it is in the given range.

